/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2026 TheSuperHackers
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "MapThumbnail/MapThumbnail.h"

#include "FileUtil.h"
#include "GameData.h"
#include "MapReader.h"
#include "RefPack.h"

#include <stb_image.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>

namespace MapThumbnail
{

namespace
{

const uintmax_t MaxFileSize = 64 * 1024 * 1024;
const int MinSize = 16;
const int MaxSize = 2048;

// Port of MapPreview::interpolateColorForHeight (World Builder) and the radar code it came from:
// higher ground is lightened, lower ground darkened.
Rgb shadeForHeight(Rgb color, float height, float hiZ, float midZ, float loZ)
{
	const float howBright = 0.30f;
	const float howDark = 0.60f;

	if (hiZ == midZ)
		hiZ = midZ + 0.1f;
	if (midZ == loZ)
		loZ = midZ - 0.1f;
	if (hiZ == loZ)
		hiZ = loZ + 0.2f;

	float t;
	Rgb target;
	if (height >= midZ)
	{
		t = (height - midZ) / (hiZ - midZ);
		target.r = color.r + (1.0f - color.r) * howBright;
		target.g = color.g + (1.0f - color.g) * howBright;
		target.b = color.b + (1.0f - color.b) * howBright;
	}
	else
	{
		t = (midZ - height) / (midZ - loZ);
		target.r = color.r - color.r * howDark;
		target.g = color.g - color.g * howDark;
		target.b = color.b - color.b * howDark;
	}

	color.r = std::clamp(color.r + (target.r - color.r) * t, 0.0f, 1.0f);
	color.g = std::clamp(color.g + (target.g - color.g) * t, 0.0f, 1.0f);
	color.b = std::clamp(color.b + (target.b - color.b) * t, 0.0f, 1.0f);
	return color;
}

bool pointInPolygon(const WaterArea &area, float x, float y)
{
	bool inside = false;
	const size_t n = area.x.size();
	for (size_t i = 0, j = n - 1; i < n; j = i++)
	{
		if ((area.y[i] > y) != (area.y[j] > y) &&
			x < (area.x[j] - area.x[i]) * (y - area.y[i]) / (area.y[j] - area.y[i]) + area.x[i])
			inside = !inside;
	}
	return inside;
}

/// The radar style render World Builder uses for the preview TGA and the game uses for its radar,
/// at width x height samples. Rows go top-down with north (larger y) at the top.
Image renderTerrain(const MapData &map, const std::vector<Rgb> &tileColors, int width, int height)
{
	const int count = width * height;
	std::vector<uint8_t> sampleHeight(count);
	std::vector<uint8_t> underwater(count);
	std::vector<Rgb> baseColor(count);

	const float xSample = float(map.playableWidth()) / width;
	const float ySample = float(map.playableHeight()) / height;
	Rgb neutral = colorFromTerrainName(std::string());

	float sum = 0;
	float minHeight = 255;
	float maxHeight = 0;
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			const float cellX = x * xSample + map.border;
			const float cellY = y * ySample + map.border;
			const int cx = std::clamp(int(cellX), 0, map.width - 1);
			const int cy = std::clamp(int(cellY), 0, map.height - 1);
			const int cell = cy * map.width + cx;
			const int i = y * width + x;

			const uint8_t h = map.heights[cell];
			sampleHeight[i] = h;
			sum += h;
			minHeight = std::min(minHeight, float(h));
			maxHeight = std::max(maxHeight, float(h));

			// Like TerrainLogic::isUnderwater, the highest water area containing the point wins.
			const float worldX = MapXYFactor * (cellX - map.border);
			const float worldY = MapXYFactor * (cellY - map.border);
			bool hasWater = false;
			float waterZ = 0;
			for (const WaterArea &area : map.water)
			{
				if ((!hasWater || area.z >= waterZ) && pointInPolygon(area, worldX, worldY))
				{
					waterZ = area.z;
					hasWater = true;
				}
			}
			underwater[i] = hasWater && h * MapHeightScale < waterZ;

			baseColor[i] = neutral;
			if (!map.tiles.empty())
			{
				const int tile = map.tiles[cell] >> 2;
				if (tile >= 0 && size_t(tile) < tileColors.size())
					baseColor[i] = tileColors[tile];
			}
		}
	}
	const float averageHeight = sum / count;

	float mapMin = 255;
	float mapMax = 0;
	for (uint8_t h : map.heights)
	{
		mapMin = std::min(mapMin, float(h));
		mapMax = std::max(mapMax, float(h));
	}

	Rgb waterColor;
	waterColor.r = 0.55f;
	waterColor.g = 0.55f;
	waterColor.b = 1.0f;

	Image image;
	image.width = width;
	image.height = height;
	image.rgba.resize(size_t(count) * 4);

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			const int i = y * width + x;
			const bool wet = underwater[i] != 0;
			Rgb total;
			int samples = 0;

			// Average the 3x3 neighbourhood, as MapPreview::buildMapPreviewTexture does.
			for (int ny = std::max(0, y - 1); ny <= std::min(height - 1, y + 1); ++ny)
			{
				for (int nx = std::max(0, x - 1); nx <= std::min(width - 1, x + 1); ++nx)
				{
					const int n = ny * width + nx;
					Rgb c;
					if (wet)
					{
						if (!underwater[n])
							continue;
						c = shadeForHeight(waterColor, sampleHeight[n], mapMax, averageHeight, mapMin);
					}
					else
					{
						c = shadeForHeight(baseColor[n], sampleHeight[i], maxHeight, averageHeight, minHeight);
					}
					total.r += c.r;
					total.g += c.g;
					total.b += c.b;
					++samples;
				}
			}
			samples = std::max(samples, 1);

			uint8_t *out = &image.rgba[(size_t(height - 1 - y) * width + x) * 4];
			out[0] = uint8_t(std::lround(total.r / samples * 255));
			out[1] = uint8_t(std::lround(total.g / samples * 255));
			out[2] = uint8_t(std::lround(total.b / samples * 255));
			out[3] = 255;
		}
	}
	return image;
}

struct Tap
{
	int index;
	float weight;
};

// Filter taps for one axis: an area average when shrinking, linear interpolation when growing.
std::vector<std::vector<Tap>> resampleTaps(int srcSize, int dstSize)
{
	std::vector<std::vector<Tap>> taps(dstSize);
	const double scale = double(srcSize) / dstSize;
	for (int i = 0; i < dstSize; ++i)
	{
		std::vector<Tap> &t = taps[i];
		if (scale > 1.0)
		{
			const double a = i * scale;
			const double b = (i + 1) * scale;
			for (int j = int(std::floor(a)); j < int(std::ceil(b)) && j < srcSize; ++j)
			{
				const double w = std::min(b, j + 1.0) - std::max(a, double(j));
				if (w > 0)
					t.push_back(Tap{ j, float(w) });
			}
		}
		else
		{
			const double center = (i + 0.5) * scale - 0.5;
			const int j = int(std::floor(center));
			const float f = float(center - j);
			t.push_back(Tap{ std::clamp(j, 0, srcSize - 1), 1.0f - f });
			t.push_back(Tap{ std::clamp(j + 1, 0, srcSize - 1), f });
		}
		float total = 0;
		for (const Tap &tap : t)
			total += tap.weight;
		for (Tap &tap : t)
			tap.weight /= total;
	}
	return taps;
}

Image resize(const Image &src, int width, int height)
{
	if (src.width == width && src.height == height)
		return src;

	const std::vector<std::vector<Tap>> xTaps = resampleTaps(src.width, width);
	const std::vector<std::vector<Tap>> yTaps = resampleTaps(src.height, height);

	// Rows first, into a float buffer, then columns.
	std::vector<float> rows(size_t(width) * src.height * 4);
	for (int y = 0; y < src.height; ++y)
		for (int x = 0; x < width; ++x)
			for (const Tap &tap : xTaps[x])
				for (int c = 0; c < 4; ++c)
					rows[(size_t(y) * width + x) * 4 + c] += tap.weight * src.rgba[(size_t(y) * src.width + tap.index) * 4 + c];

	Image out;
	out.width = width;
	out.height = height;
	out.rgba.resize(size_t(width) * height * 4);
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			float v[4] = { 0, 0, 0, 0 };
			for (const Tap &tap : yTaps[y])
				for (int c = 0; c < 4; ++c)
					v[c] += tap.weight * rows[(size_t(tap.index) * width + x) * 4 + c];
			for (int c = 0; c < 4; ++c)
				out.rgba[(size_t(y) * width + x) * 4 + c] = uint8_t(std::clamp(std::lround(v[c]), 0L, 255L));
		}
	}
	return out;
}

void fitInside(int srcWidth, int srcHeight, int maxSize, int &width, int &height)
{
	if (srcWidth >= srcHeight)
	{
		width = maxSize;
		height = std::max(1, int(std::lround(double(maxSize) * srcHeight / srcWidth)));
	}
	else
	{
		height = maxSize;
		width = std::max(1, int(std::lround(double(maxSize) * srcWidth / srcHeight)));
	}
}

void blend(Image &image, int x, int y, const uint8_t color[3], float alpha)
{
	if (x < 0 || y < 0 || x >= image.width || y >= image.height || alpha <= 0)
		return;
	alpha = std::min(alpha, 1.0f);
	uint8_t *p = &image.rgba[(size_t(y) * image.width + x) * 4];
	for (int c = 0; c < 3; ++c)
		p[c] = uint8_t(std::lround(p[c] + (color[c] - p[c]) * alpha));
}

// The numbered start position markers the skirmish map select screen draws over the preview.
void drawStartPositions(Image &image, const MapData &map)
{
	static const char *const digits[10][5] = {
		{ "111", "101", "101", "101", "111" }, { "010", "110", "010", "010", "111" },
		{ "111", "001", "111", "100", "111" }, { "111", "001", "111", "001", "111" },
		{ "101", "101", "111", "001", "001" }, { "111", "100", "111", "001", "111" },
		{ "111", "100", "111", "101", "111" }, { "111", "001", "001", "001", "001" },
		{ "111", "101", "111", "101", "111" }, { "111", "101", "111", "001", "111" },
	};
	static const uint8_t ring[3] = { 24, 24, 24 };
	static const uint8_t fill[3] = { 250, 250, 245 };

	const float radius = std::max(3.5f, std::min(image.width, image.height) / 22.0f);
	const float ringWidth = std::max(1.0f, radius / 4);
	const int scale = int((radius - ringWidth) * 2 * 0.6f / 5);

	for (const StartPosition &start : map.startPositions)
	{
		const float cx = start.x / MapXYFactor / map.playableWidth() * image.width;
		const float cy = image.height - start.y / MapXYFactor / map.playableHeight() * image.height;

		for (int y = int(cy - radius - 1); y <= int(cy + radius + 1); ++y)
		{
			for (int x = int(cx - radius - 1); x <= int(cx + radius + 1); ++x)
			{
				const float d = std::hypot(x + 0.5f - cx, y + 0.5f - cy);
				blend(image, x, y, ring, radius + 0.5f - d);
				blend(image, x, y, fill, radius - ringWidth + 0.5f - d);
			}
		}

		if (scale < 1)
			continue;
		const char *const *glyph = digits[start.player % 10];
		const int left = int(std::lround(cx - 1.5f * scale));
		const int top = int(std::lround(cy - 2.5f * scale));
		for (int gy = 0; gy < 5; ++gy)
			for (int gx = 0; gx < 3; ++gx)
				if (glyph[gy][gx] == '1')
					for (int sy = 0; sy < scale; ++sy)
						for (int sx = 0; sx < scale; ++sx)
							blend(image, left + gx * scale + sx, top + gy * scale + sy, ring, 1.0f);
	}
}

bool renderInGame(const MapData &map, int maxSize, const Settings &settings, Image &out)
{
	int width, height;
	fitInside(map.playableWidth(), map.playableHeight(), maxSize, width, height);

	// One sample per cell is all the detail there is; scale up from there with filtering.
	int sampleWidth = std::min(width, map.playableWidth());
	int sampleHeight = std::min(height, map.playableHeight());
	fitInside(map.playableWidth(), map.playableHeight(), std::max(sampleWidth, sampleHeight), sampleWidth, sampleHeight);

	const std::vector<Rgb> tileColors = sourceTileColors(map, settings.gameDirs);
	out = resize(renderTerrain(map, tileColors, sampleWidth, sampleHeight), width, height);
	if (settings.showStartPositions)
		drawStartPositions(out, map);
	return true;
}

bool loadPreviewTga(const fs::path &mapPath, int maxSize, Image &out)
{
	// World Builder saves the preview as <map name>.tga next to the map (MapPreview::save).
	fs::path tgaPath;
	const std::string tgaName = pathToUtf8(mapPath.stem()) + ".tga";
	if (!findPathNoCase(mapPath.parent_path(), tgaName, tgaPath))
		return false;

	std::vector<uint8_t> data;
	if (!readWholeFile(tgaPath, data, MaxFileSize))
		return false;

	int w = 0, h = 0, channels = 0;
	std::unique_ptr<uint8_t, void (*)(void *)> pixels(
		stbi_load_from_memory(data.data(), int(data.size()), &w, &h, &channels, 4), stbi_image_free);
	if (!pixels || w <= 0 || h <= 0)
		return false;

	Image tga;
	tga.width = w;
	tga.height = h;
	tga.rgba.assign(pixels.get(), pixels.get() + size_t(w) * h * 4);
	// The game draws the preview without blending, so the alpha channel means nothing.
	for (size_t i = 3; i < tga.rgba.size(); i += 4)
		tga.rgba[i] = 255;

	int width, height;
	fitInside(w, h, maxSize, width, height);
	out = resize(tga, width, height);
	return true;
}

bool parseMapData(const uint8_t *data, size_t size, MapData &map, std::string *error)
{
	std::vector<uint8_t> storage;
	if (!unwrapCompressed(data, size, storage))
	{
		if (error)
			*error = "damaged map: bad compressed data";
		return false;
	}
	return readMap(data, size, map, error);
}

} // namespace

bool isMapData(const uint8_t *data, size_t size)
{
	return size >= 8 && (memcmp(data, "CkMp", 4) == 0 || memcmp(data, "EAR\0", 4) == 0);
}

bool renderMemory(const uint8_t *data, size_t size, int maxSize, const Settings &settings, Image &out, std::string *error)
{
	MapData map;
	if (!parseMapData(data, size, map, error))
		return false;
	return renderInGame(map, std::clamp(maxSize, MinSize, MaxSize), settings, out);
}

bool renderFile(const std::string &mapPath, int maxSize, const Settings &settings, Image &out, std::string *error)
{
	maxSize = std::clamp(maxSize, MinSize, MaxSize);
	const fs::path path = pathFromUtf8(mapPath);

	if (settings.mode == Mode::Tga && loadPreviewTga(path, maxSize, out))
		return true;

	std::vector<uint8_t> data;
	if (!readWholeFile(path, data, MaxFileSize))
	{
		if (error)
			*error = "cannot read " + mapPath;
		return false;
	}
	return renderMemory(data.data(), data.size(), maxSize, settings, out, error);
}

} // namespace MapThumbnail

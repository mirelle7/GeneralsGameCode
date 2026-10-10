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

#include "GameData.h"

#include "FileUtil.h"
#include "MapReader.h"

#include <stb_image.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>

namespace MapThumbnail
{

namespace
{

const int TilePixelExtent = 64; // TILE_PIXEL_EXTENT
const uint32_t MaxGameFileSize = 64 * 1024 * 1024;

uint32_t readBigEndian32(const uint8_t *p)
{
	return (uint32_t(p[0]) << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

// Looks a file up in one .big archive. The format is "BIGF", a little-endian archive size, then a
// big-endian file count and header size, then per file a big-endian offset, size and a C string name.
bool readFromBig(const fs::path &bigPath, const std::string &lowerName, std::vector<uint8_t> &out)
{
	std::ifstream file(bigPath, std::ios::binary);
	uint8_t header[16];
	if (!file.read(reinterpret_cast<char *>(header), sizeof(header)))
		return false;
	if (memcmp(header, "BIGF", 4) != 0 && memcmp(header, "BIG4", 4) != 0)
		return false;

	const uint32_t count = readBigEndian32(header + 8);
	const uint32_t headerSize = readBigEndian32(header + 12);
	if (headerSize < 16 || headerSize > MaxGameFileSize || count > headerSize / 9)
		return false;

	std::vector<uint8_t> index(headerSize - 16);
	if (!file.read(reinterpret_cast<char *>(index.data()), index.size()))
		return false;

	size_t pos = 0;
	for (uint32_t i = 0; i < count; ++i)
	{
		if (index.size() - pos < 9)
			return false;
		const uint32_t offset = readBigEndian32(&index[pos]);
		const uint32_t size = readBigEndian32(&index[pos + 4]);
		pos += 8;
		const uint8_t *nameStart = &index[pos];
		const uint8_t *nameEnd = static_cast<const uint8_t *>(memchr(nameStart, 0, index.size() - pos));
		if (!nameEnd)
			return false;
		std::string name(reinterpret_cast<const char *>(nameStart), nameEnd - nameStart);
		pos += name.size() + 1;

		if (toLower(name) != lowerName)
			continue;
		if (size > MaxGameFileSize)
			return false;
		out.resize(size);
		file.seekg(offset);
		return bool(file.read(reinterpret_cast<char *>(out.data()), size));
	}
	return false;
}

// Archives that hold Terrain.ini or the terrain textures: INI.big, INIZH.big, Terrain.big, TerrainZH.big.
// The other archives are large and never needed, so they are not opened.
std::vector<fs::path> terrainArchives(const fs::path &dir)
{
	std::vector<fs::path> archives;
	std::error_code ec;
	for (fs::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec))
	{
		const std::string name = toLower(pathToUtf8(it->path().filename()));
		if (name.size() > 4 && name.compare(name.size() - 4, 4, ".big") == 0 &&
			(name.find("ini") != std::string::npos || name.find("terrain") != std::string::npos))
			archives.push_back(it->path());
	}
	// Zero Hour's *ZH.big archives sort after the Generals ones they override, so look at them first.
	std::sort(archives.rbegin(), archives.rend());
	return archives;
}

struct TerrainEntry
{
	std::string texture;
	std::string className;
};

// Parses the "Terrain <name> ... Texture = <file> ... End" blocks of Terrain.ini.
std::map<std::string, TerrainEntry> parseTerrainIni(const std::vector<uint8_t> &data)
{
	std::map<std::string, TerrainEntry> terrains;
	std::istringstream in(std::string(data.begin(), data.end()));
	std::string line;
	std::string current;
	while (std::getline(in, line))
	{
		const size_t comment = std::min(line.find(';'), line.find("//"));
		if (comment != std::string::npos)
			line.erase(comment);
		for (char &c : line)
			if (c == '=' || c == '\t' || c == '\r')
				c = ' ';

		std::istringstream words(line);
		std::string key, value;
		words >> key >> value;
		const std::string lowerKey = toLower(key);
		if (lowerKey == "terrain" && !value.empty())
			current = toLower(value);
		else if (lowerKey == "end")
			current.clear();
		else if (!current.empty() && lowerKey == "texture")
			terrains[current].texture = value;
		else if (!current.empty() && lowerKey == "class")
			terrains[current].className = value;
	}
	return terrains;
}

int tilesPerRow(const TextureClass &tc, int imageWidth, int imageHeight)
{
	// Same as WorldHeightMap::countTiles and readTexClass: the largest square of whole tiles that
	// both the image and the texture class have room for.
	int available = std::min(imageWidth, imageHeight) / TilePixelExtent;
	if (available == 0 && imageWidth == TilePixelExtent / 2 && imageHeight == TilePixelExtent / 2)
		available = 1;
	available = std::min(available, 10);
	for (int width = available; width >= 1; --width)
		if (width * width <= tc.numTiles)
			return width;
	return 0;
}

// Averages each 64x64 tile of a terrain texture. Like WorldHeightMap::readTiles, rows are counted in
// file order, which for the usual bottom-up TGA means starting at the bottom of the picture.
bool textureTileColors(const std::vector<uint8_t> &file, const TextureClass &tc, std::vector<Rgb> &colors, std::vector<bool> &known)
{
	int w = 0, h = 0, channels = 0;
	stbi_set_flip_vertically_on_load_thread(1);
	std::unique_ptr<uint8_t, void (*)(void *)> pixels(
		stbi_load_from_memory(file.data(), int(file.size()), &w, &h, &channels, 3), stbi_image_free);
	stbi_set_flip_vertically_on_load_thread(0);
	if (!pixels)
		return false;

	const int rows = tilesPerRow(tc, w, h);
	if (rows == 0)
		return false;
	const int extent = std::min(TilePixelExtent, std::min(w, h));

	for (int tile = 0; tile < rows * rows; ++tile)
	{
		const int index = tc.firstTile + tile;
		if (index < 0 || size_t(index) >= colors.size())
			continue;
		const int x0 = (tile % rows) * TilePixelExtent;
		const int y0 = (tile / rows) * TilePixelExtent;
		double sum[3] = { 0, 0, 0 };
		for (int y = y0; y < y0 + extent; ++y)
		{
			const uint8_t *p = pixels.get() + (size_t(y) * w + x0) * 3;
			for (int x = 0; x < extent; ++x, p += 3)
			{
				sum[0] += p[0];
				sum[1] += p[1];
				sum[2] += p[2];
			}
		}
		const double n = double(extent) * extent * 255.0;
		colors[index].r = float(sum[0] / n);
		colors[index].g = float(sum[1] / n);
		colors[index].b = float(sum[2] / n);
		known[index] = true;
	}
	return true;
}

} // namespace

bool readGameFile(const std::vector<std::string> &gameDirs, const std::string &path, std::vector<uint8_t> &out)
{
	const std::string lowerName = toLower(path);
	for (const std::string &dirText : gameDirs)
	{
		const fs::path dir = pathFromUtf8(dirText);
		fs::path loose;
		if (findPathNoCase(dir, path, loose) && readWholeFile(loose, out, MaxGameFileSize))
			return true;
		for (const fs::path &archive : terrainArchives(dir))
			if (readFromBig(archive, lowerName, out))
				return true;
	}
	return false;
}

Rgb colorFromTerrainName(const std::string &name)
{
	struct Guess
	{
		const char *words[12];
		uint8_t r, g, b;
	};
	static const Guess guesses[] = {
		{ { "water", "river", "lake", "ocean" }, 70, 95, 135 },
		{ { "snow", "ice", "arctic", "frost", "winter" }, 220, 225, 230 },
		{ { "sand", "beach", "desert", "dune" }, 196, 170, 122 },
		{ { "grass", "green", "meadow", "jungle", "forest", "moss", "lawn" }, 92, 116, 58 },
		{ { "cliff", "rock", "mountain", "stone", "boulder", "crag" }, 122, 112, 102 },
		{ { "asphalt", "road", "highway", "street", "concrete", "tarmac", "pavement", "urban", "city", "sidewalk", "parking", "cobble" }, 98, 98, 96 },
		{ { "mud", "dirt", "soil", "farm", "field", "crop", "earth" }, 118, 94, 66 },
		{ { "gravel", "pebble", "rubble" }, 128, 122, 112 },
	};

	const std::string lower = toLower(name);
	for (const Guess &guess : guesses)
	{
		for (const char *word : guess.words)
		{
			if (word && lower.find(word) != std::string::npos)
			{
				Rgb c;
				c.r = guess.r / 255.0f;
				c.g = guess.g / 255.0f;
				c.b = guess.b / 255.0f;
				return c;
			}
		}
	}
	Rgb earth;
	earth.r = 128 / 255.0f;
	earth.g = 116 / 255.0f;
	earth.b = 90 / 255.0f;
	return earth;
}

std::vector<Rgb> sourceTileColors(const MapData &map, const std::vector<std::string> &gameDirs)
{
	int numTiles = 0;
	for (const TextureClass &tc : map.textureClasses)
		if (tc.firstTile >= 0 && tc.numTiles > 0 && tc.numTiles <= 4096 && tc.firstTile <= 65536)
			numTiles = std::max(numTiles, tc.firstTile + tc.numTiles);

	std::vector<Rgb> colors(numTiles);
	std::vector<bool> known(numTiles, false);

	std::map<std::string, TerrainEntry> terrains;
	std::vector<uint8_t> ini;
	if (!gameDirs.empty() && readGameFile(gameDirs, "Data\\INI\\Terrain.ini", ini))
		terrains = parseTerrainIni(ini);

	std::vector<uint8_t> texture;
	for (const TextureClass &tc : map.textureClasses)
	{
		if (tc.firstTile < 0 || tc.numTiles <= 0 || tc.firstTile + tc.numTiles > numTiles)
			continue;

		std::string guessName = tc.name;
		std::map<std::string, TerrainEntry>::const_iterator it = terrains.find(toLower(tc.name));
		if (it != terrains.end())
		{
			if (!it->second.texture.empty() && readGameFile(gameDirs, "Art\\Terrain\\" + it->second.texture, texture))
				textureTileColors(texture, tc, colors, known);
			guessName += " " + it->second.className + " " + it->second.texture;
		}

		const Rgb guess = colorFromTerrainName(guessName);
		for (int i = tc.firstTile; i < tc.firstTile + tc.numTiles; ++i)
		{
			if (!known[i])
			{
				colors[i] = guess;
				known[i] = true;
			}
		}
	}
	return colors;
}

} // namespace MapThumbnail

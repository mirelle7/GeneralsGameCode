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

// Self-contained tests, so the thumbnailer builds and tests without the game's test setup.
// Maps are written here chunk by chunk in the layout WorldHeightMap and friends read.

#include "MapThumbnail/MapThumbnail.h"

#include "FileUtil.h"
#include "GameData.h"
#include "MapReader.h"
#include "RefPack.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <random>
#include <string>
#include <vector>

using namespace MapThumbnail;

namespace
{

int g_failures = 0;

#define CHECK(cond) \
	do { \
		if (!(cond)) { \
			fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
			++g_failures; \
		} \
	} while (0)

class Bytes
{
public:
	std::vector<uint8_t> data;

	void u8(uint8_t v) { data.push_back(v); }
	void u16(uint16_t v) { u8(v & 0xff); u8(v >> 8); }
	void u32(uint32_t v) { u16(v & 0xffff); u16(v >> 16); }
	void f32(float f) { uint32_t v; memcpy(&v, &f, 4); u32(v); }
	void str(const std::string &s) { u16(uint16_t(s.size())); raw(s.data(), s.size()); }
	void raw(const void *p, size_t n) { data.insert(data.end(), (const uint8_t *)p, (const uint8_t *)p + n); }
	void append(const Bytes &b) { data.insert(data.end(), b.data.begin(), b.data.end()); }
};

class MapWriter
{
public:
	uint32_t id(const std::string &name)
	{
		auto it = m_ids.find(name);
		if (it != m_ids.end())
			return it->second;
		uint32_t newId = uint32_t(m_ids.size() + 1);
		m_ids[name] = newId;
		return newId;
	}

	Bytes chunk(const std::string &label, uint16_t version, const Bytes &body)
	{
		Bytes c;
		c.u32(id(label));
		c.u16(version);
		c.u32(uint32_t(body.data.size()));
		c.append(body);
		return c;
	}

	std::vector<uint8_t> file(const Bytes &chunks)
	{
		Bytes f;
		f.raw("CkMp", 4);
		f.u32(uint32_t(m_ids.size()));
		for (auto &entry : m_ids)
		{
			f.u8(uint8_t(entry.first.size()));
			f.raw(entry.first.data(), entry.first.size());
			f.u32(entry.second);
		}
		f.append(chunks);
		return f.data;
	}

private:
	std::map<std::string, uint32_t> m_ids;
};

const int W = 40;      // cells, border included
const int H = 30;
const int Border = 4;  // playable area is 32 x 22

// A map with grass on the west half, sand on the east half, a lake in the south-west corner
// and player 1 starting in the north-east.
std::vector<uint8_t> makeTestMap()
{
	MapWriter mw;

	Bytes height;
	height.u32(W);
	height.u32(H);
	height.u32(Border);
	height.u32(1);
	height.u32(W - 2 * Border);
	height.u32(H - 2 * Border);
	height.u32(W * H);
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x)
			height.u8(x < 14 && y < 12 ? 2 : 20); // the lake bed is low

	Bytes blend;
	const int n = W * H;
	blend.u32(n);
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x)
			blend.u16(uint16_t((x < W / 2 ? 0 : 4) << 2)); // source tile 0 = grass, 4 = sand
	for (int k = 0; k < 3; ++k)
		for (int i = 0; i < n; ++i)
			blend.u16(0); // blend, extra blend and cliff info
	for (int i = 0; i < H * ((W + 7) / 8); ++i)
		blend.u8(0); // cliff flags
	blend.u32(8); // bitmap tiles
	blend.u32(1); // blended tiles
	blend.u32(1); // cliff infos
	blend.u32(2); // texture classes
	blend.u32(0); blend.u32(4); blend.u32(2); blend.u32(0); blend.str("GrassType1");
	blend.u32(4); blend.u32(4); blend.u32(2); blend.u32(0); blend.str("TEDesert1");
	blend.u32(0); // edge tiles
	blend.u32(0); // edge classes

	Bytes triggers;
	triggers.u32(1);
	triggers.str("Lake");
	triggers.str("Default");
	triggers.u32(1);
	triggers.u8(1); // water
	triggers.u8(0); // river
	triggers.u32(0);
	triggers.u32(4);
	const int lake[4][2] = { { -50, -50 }, { 100, -50 }, { 100, 75 }, { -50, 75 } };
	for (auto &p : lake)
	{
		triggers.u32(uint32_t(p[0]));
		triggers.u32(uint32_t(p[1]));
		triggers.u32(5); // above the lake bed (2 * 0.625), below the land (20 * 0.625)
	}

	Bytes object;
	object.f32(280.0f); // world units: cell 28 of the playable 32
	object.f32(180.0f); // cell 18 of the playable 22
	object.f32(0);
	object.f32(0);
	object.u32(0);
	object.str("*Waypoints/Waypoint");
	object.u16(2);
	object.u32((mw.id("waypointID") << 8) | 1);
	object.u32(1);
	object.u32((mw.id("waypointName") << 8) | 3);
	object.str("Player_1_Start");
	Bytes objects = mw.chunk("Object", 3, object);

	Bytes chunks;
	chunks.append(mw.chunk("HeightMapData", 4, height));
	chunks.append(mw.chunk("BlendTileData", 8, blend));
	chunks.append(mw.chunk("ObjectsList", 3, objects));
	chunks.append(mw.chunk("PolygonTriggers", 4, triggers));
	return mw.file(chunks);
}

// RefPack with literal blocks only, which is valid input for any decoder.
std::vector<uint8_t> refPackLiterals(const std::vector<uint8_t> &data)
{
	Bytes out;
	out.u8(0x10);
	out.u8(0xfb);
	out.u8(uint8_t(data.size() >> 16));
	out.u8(uint8_t(data.size() >> 8));
	out.u8(uint8_t(data.size()));
	size_t pos = 0;
	while (data.size() - pos >= 4)
	{
		const size_t run = std::min<size_t>(112, (data.size() - pos) & ~size_t(3));
		out.u8(uint8_t(0xe0 + run / 4 - 1));
		out.raw(&data[pos], run);
		pos += run;
	}
	out.u8(uint8_t(0xfc + (data.size() - pos)));
	out.raw(data.data() + pos, data.size() - pos);
	return out.data;
}

std::vector<uint8_t> earWrap(const std::vector<uint8_t> &data)
{
	Bytes out;
	out.raw("EAR\0", 4);
	out.u32(uint32_t(data.size()));
	const std::vector<uint8_t> packed = refPackLiterals(data);
	out.raw(packed.data(), packed.size());
	return out.data;
}

// Uncompressed 24-bit TGA, bottom-up, filled with one color.
std::vector<uint8_t> solidTga(int w, int h, uint8_t r, uint8_t g, uint8_t b)
{
	Bytes t;
	const uint8_t header[18] = { 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, uint8_t(w), uint8_t(w >> 8), uint8_t(h), uint8_t(h >> 8), 24, 0 };
	t.raw(header, sizeof(header));
	for (int i = 0; i < w * h; ++i)
	{
		t.u8(b);
		t.u8(g);
		t.u8(r);
	}
	return t.data;
}

void writeFile(const fs::path &path, const std::vector<uint8_t> &data)
{
	fs::create_directories(path.parent_path());
	std::ofstream(path, std::ios::binary).write((const char *)data.data(), data.size());
}

const uint8_t *pixel(const Image &image, int x, int y)
{
	return &image.rgba[(size_t(y) * image.width + x) * 4];
}

fs::path tempDir(const char *name)
{
	fs::path dir = fs::temp_directory_path() / (std::string("mapthumbnail_test_") + name);
	fs::remove_all(dir);
	fs::create_directories(dir);
	return dir;
}

void testRefPack()
{
	// Three literals then a 9 byte copy from 3 back, overlapping itself, then the end marker.
	const uint8_t packed[] = { 0x10, 0xfb, 0, 0, 12, 0x1b, 0x02, 'a', 'b', 'c', 0xfc };
	std::vector<uint8_t> out;
	CHECK(refPackDecode(packed, sizeof(packed), out, 1000));
	CHECK(std::string(out.begin(), out.end()) == "abcabcabcabc");

	// A reference before the start of the output is rejected, not read out of bounds.
	const uint8_t badRef[] = { 0x10, 0xfb, 0, 0, 12, 0x1b, 0x09, 'a', 'b', 'c', 0xfc };
	CHECK(!refPackDecode(badRef, sizeof(badRef), out, 1000));

	// More output than the header announced is rejected.
	const uint8_t tooLong[] = { 0x10, 0xfb, 0, 0, 2, 0x1b, 0x02, 'a', 'b', 'c', 0xfc };
	CHECK(!refPackDecode(tooLong, sizeof(tooLong), out, 1000));

	std::vector<uint8_t> text(301);
	for (size_t i = 0; i < text.size(); ++i)
		text[i] = uint8_t(i * 7);
	const std::vector<uint8_t> literals = refPackLiterals(text);
	CHECK(refPackDecode(literals.data(), literals.size(), out, 1000));
	CHECK(out == text);
}

void testReadMap()
{
	const std::vector<uint8_t> file = makeTestMap();
	MapData map;
	std::string error;
	CHECK(readMap(file.data(), file.size(), map, &error));
	CHECK(map.width == W && map.height == H && map.border == Border);
	CHECK(map.playableWidth() == 32 && map.playableHeight() == 22);
	CHECK(map.tiles.size() == size_t(W * H));
	CHECK(map.textureClasses.size() == 2);
	CHECK(map.textureClasses.size() == 2 && map.textureClasses[1].name == "TEDesert1");
	CHECK(map.water.size() == 1 && map.water[0].z == 5.0f);
	CHECK(map.startPositions.size() == 1);
	CHECK(map.startPositions.size() == 1 && map.startPositions[0].player == 1 && map.startPositions[0].x == 280.0f);
}

void testRenderInGame()
{
	const std::vector<uint8_t> file = makeTestMap();
	Settings settings;
	settings.showStartPositions = false;
	Image image;
	std::string error;
	CHECK(renderMemory(file.data(), file.size(), 128, settings, image, &error));
	// 32 x 22 playable cells fit in 128 x 88.
	CHECK(image.width == 128 && image.height == 88);
	CHECK(image.rgba.size() == size_t(128 * 88 * 4));
	if (image.width != 128 || image.height != 88)
		return;

	const uint8_t *lake = pixel(image, 10, 80);  // south-west
	const uint8_t *grass = pixel(image, 30, 10); // north-west
	const uint8_t *sand = pixel(image, 110, 10); // north-east
	CHECK(lake[2] > lake[0] && lake[2] > lake[1]);
	CHECK(grass[1] > grass[0] && grass[1] > grass[2]);
	CHECK(sand[0] > sand[2] && sand[1] > sand[2]);
	CHECK(lake[3] == 255 && grass[3] == 255);

	// The start marker is drawn white at (28, 18) of the 32 x 22 cells: x = 112, y = 88 - 72 = 16.
	settings.showStartPositions = true;
	Image marked;
	CHECK(renderMemory(file.data(), file.size(), 128, settings, marked, &error));
	const uint8_t *before = pixel(image, 112, 14);
	const uint8_t *after = pixel(marked, 112, 14);
	CHECK(after[0] > 200 && after[1] > 200 && after[2] > 200);
	CHECK(before[0] < 200 || before[1] < 200 || before[2] < 200);

	// The same map compressed the way the game ships it renders the same.
	const std::vector<uint8_t> compressed = earWrap(file);
	Image fromCompressed;
	CHECK(isMapData(compressed.data(), compressed.size()));
	CHECK(renderMemory(compressed.data(), compressed.size(), 128, settings, fromCompressed, &error));
	CHECK(fromCompressed.rgba == marked.rgba);
}

void testBrokenInput()
{
	const std::vector<uint8_t> file = makeTestMap();
	Settings settings;
	Image image;

	const char junk[] = "not a map at all";
	CHECK(!isMapData((const uint8_t *)junk, sizeof(junk)));
	CHECK(!renderMemory((const uint8_t *)junk, sizeof(junk), 64, settings, image));

	// Every truncation and a few thousand random corruptions must fail or succeed cleanly.
	for (size_t len = 0; len < file.size(); ++len)
		renderMemory(file.data(), len, 32, settings, image);

	std::mt19937 rng(1234);
	for (int round = 0; round < 3000; ++round)
	{
		std::vector<uint8_t> bad = round % 2 ? file : earWrap(file);
		for (int k = 0; k < 4; ++k)
			bad[rng() % bad.size()] = uint8_t(rng());
		renderMemory(bad.data(), bad.size(), 32, settings, image);
	}
}

void testTgaMode()
{
	const fs::path dir = tempDir("tga");
	writeFile(dir / "Lake Map.map", makeTestMap());
	writeFile(dir / "Lake Map.tga", solidTga(128, 128, 200, 20, 30));

	Settings settings;
	settings.mode = Mode::Tga;
	Image image;
	std::string error;
	CHECK(renderFile(pathToUtf8(dir / "Lake Map.map"), 256, settings, image, &error));
	CHECK(image.width == 256 && image.height == 256);
	if (image.width == 256)
	{
		const uint8_t *p = pixel(image, 100, 100);
		CHECK(p[0] == 200 && p[1] == 20 && p[2] == 30 && p[3] == 255);
	}

	// The in-game mode ignores the TGA.
	settings.mode = Mode::InGame;
	CHECK(renderFile(pathToUtf8(dir / "Lake Map.map"), 256, settings, image, &error));
	CHECK(image.width == 256 && image.height == 176);

	// Without a TGA the in-game render is the fallback.
	fs::remove(dir / "Lake Map.tga");
	settings.mode = Mode::Tga;
	CHECK(renderFile(pathToUtf8(dir / "Lake Map.map"), 256, settings, image, &error));
	CHECK(image.width == 256 && image.height == 176);

	fs::remove_all(dir);
}

void writeBig(const fs::path &path, const std::vector<std::pair<std::string, std::vector<uint8_t>>> &files)
{
	uint32_t headerSize = 16;
	for (auto &f : files)
		headerSize += 8 + uint32_t(f.first.size()) + 1;
	std::vector<uint8_t> out = { 'B', 'I', 'G', 'F', 0, 0, 0, 0 };
	auto be32 = [&](uint32_t v) {
		out.push_back(uint8_t(v >> 24));
		out.push_back(uint8_t(v >> 16));
		out.push_back(uint8_t(v >> 8));
		out.push_back(uint8_t(v));
	};
	be32(uint32_t(files.size()));
	be32(headerSize);
	uint32_t offset = headerSize;
	for (auto &f : files)
	{
		be32(offset);
		be32(uint32_t(f.second.size()));
		out.insert(out.end(), f.first.begin(), f.first.end());
		out.push_back(0);
		offset += uint32_t(f.second.size());
	}
	for (auto &f : files)
		out.insert(out.end(), f.second.begin(), f.second.end());
	writeFile(path, out);
}

void testGameTextures()
{
	const std::vector<uint8_t> file = makeTestMap();
	MapData map;
	CHECK(readMap(file.data(), file.size(), map, nullptr));

	// Without game files the colors come from the names.
	std::vector<Rgb> guessed = sourceTileColors(map, {});
	CHECK(guessed.size() == 8);

	// Terrain.ini loose on disk with odd casing, textures inside a .big archive.
	const fs::path game = tempDir("game");
	const std::string ini =
		"; terrain\n"
		"Terrain GrassType1\n  Texture = MyGrass.tga\n  Class = GRASS\nEnd\n"
		"Terrain TEDesert1\n  Texture = MySand.tga ; comment\nEnd\n";
	writeFile(game / "data" / "ini" / "terrain.INI", std::vector<uint8_t>(ini.begin(), ini.end()));
	writeBig(game / "TerrainZH.big", {
		{ "Art\\Terrain\\MyGrass.tga", solidTga(128, 128, 10, 250, 10) },
		{ "Art\\Terrain\\MySand.tga", solidTga(128, 128, 250, 10, 250) },
	});
	writeBig(game / "W3DZH.big", { { "Art\\Terrain\\MySand.tga", solidTga(64, 64, 0, 0, 0) } });

	std::vector<Rgb> colors = sourceTileColors(map, { pathToUtf8(game) });
	CHECK(colors.size() == 8);
	if (colors.size() == 8)
	{
		CHECK(colors[0].g > 0.95f && colors[0].r < 0.05f);
		CHECK(colors[3].g > 0.95f && colors[3].r < 0.05f);
		CHECK(colors[4].r > 0.95f && colors[4].b > 0.95f && colors[4].g < 0.05f);
	}

	fs::remove_all(game);
}

void testSettings()
{
	const fs::path dir = tempDir("settings");
	const std::string path = pathToUtf8(dir / "sub" / "MapThumbnail.ini");

	Settings settings;
	CHECK(!loadSettingsFile(path, settings));
	CHECK(settings.mode == Mode::InGame);

	settings.mode = Mode::Tga;
	settings.showStartPositions = false;
	settings.gameDirs = { "C:\\Games\\Zero Hour", "/opt/generals" };
	CHECK(saveSettingsFile(path, settings));

	Settings loaded;
	CHECK(loadSettingsFile(path, loaded));
	CHECK(loaded.mode == Mode::Tga);
	CHECK(!loaded.showStartPositions);
	CHECK(loaded.gameDirs == settings.gameDirs);

	Mode mode;
	CHECK(parseMode(" InGame ", mode) && mode == Mode::InGame);
	CHECK(parseMode("TGA", mode) && mode == Mode::Tga);
	CHECK(!parseMode("radar", mode));

	fs::remove_all(dir);
}

} // namespace

int main()
{
	testRefPack();
	testReadMap();
	testRenderInGame();
	testBrokenInput();
	testTgaMode();
	testGameTextures();
	testSettings();

	if (g_failures)
	{
		fprintf(stderr, "%d check(s) failed\n", g_failures);
		return 1;
	}
	printf("All map thumbnail tests passed\n");
	return 0;
}

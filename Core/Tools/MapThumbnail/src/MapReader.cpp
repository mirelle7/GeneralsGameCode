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

#include "MapReader.h"

#include <cstdio>
#include <cstring>
#include <map>

namespace MapThumbnail
{

namespace
{

const int MaxMapCells = 4096;
const int K_HEIGHT_MAP_VERSION_1 = 1;
const int K_HEIGHT_MAP_VERSION_3 = 3;
const int K_HEIGHT_MAP_VERSION_4 = 4;
const int K_BLEND_TILE_VERSION_1 = 1;
const int K_BLEND_TILE_VERSION_5 = 5;
const int K_BLEND_TILE_VERSION_6 = 6;
const int K_BLEND_TILE_VERSION_7 = 7;
const int K_OBJECTS_VERSION_2 = 2;
const int K_TRIGGERS_VERSION_2 = 2;
const int K_TRIGGERS_VERSION_3 = 3;
const int K_TRIGGERS_VERSION_4 = 4;

// Dict::DataType
enum { DICT_BOOL, DICT_INT, DICT_REAL, DICT_ASCIISTRING, DICT_UNICODESTRING };

// Little-endian reader that turns any out-of-range read into a sticky failure.
class Reader
{
public:
	Reader(const uint8_t *data, size_t size) : m_data(data), m_size(size) {}

	bool ok() const { return m_ok; }
	size_t pos() const { return m_pos; }
	size_t left() const { return m_ok ? m_size - m_pos : 0; }
	void fail() { m_ok = false; }

	const uint8_t *take(size_t n)
	{
		if (!m_ok || n > m_size - m_pos)
		{
			m_ok = false;
			return nullptr;
		}
		const uint8_t *p = m_data + m_pos;
		m_pos += n;
		return p;
	}
	void skip(size_t n) { take(n); }
	uint8_t u8()
	{
		const uint8_t *p = take(1);
		return p ? p[0] : 0;
	}
	uint16_t u16()
	{
		const uint8_t *p = take(2);
		return p ? uint16_t(p[0] | (p[1] << 8)) : 0;
	}
	uint32_t u32()
	{
		const uint8_t *p = take(4);
		return p ? uint32_t(p[0] | (p[1] << 8) | (p[2] << 16) | (uint32_t(p[3]) << 24)) : 0;
	}
	int32_t i32() { return int32_t(u32()); }
	float f32()
	{
		uint32_t bits = u32();
		float f;
		memcpy(&f, &bits, sizeof(f));
		return f;
	}
	std::string asciiString()
	{
		const uint16_t len = u16();
		const uint8_t *p = take(len);
		return p ? std::string(reinterpret_cast<const char *>(p), len) : std::string();
	}

private:
	const uint8_t *m_data;
	size_t m_size;
	size_t m_pos = 0;
	bool m_ok = true;
};

struct Chunk
{
	std::string label;
	int version = 0;
	Reader body{nullptr, 0};
};

typedef std::map<uint32_t, std::string> TableOfContents;

// Reads the next chunk header and splits its body off into its own reader.
bool nextChunk(Reader &r, const TableOfContents &toc, Chunk &chunk)
{
	if (r.left() < 10)
		return false;
	const uint32_t id = r.u32();
	chunk.version = r.u16();
	const int32_t size = r.i32();
	if (size < 0 || size_t(size) > r.left())
		return false;
	const uint8_t *body = r.take(size);
	TableOfContents::const_iterator it = toc.find(id);
	chunk.label = it != toc.end() ? it->second : std::string();
	chunk.body = Reader(body, size);
	return true;
}

bool readHeightMap(Chunk &c, MapData &map)
{
	// Later SAGE games (Battle for Middle-earth) use newer versions with 16-bit heights.
	if (c.version < K_HEIGHT_MAP_VERSION_1 || c.version > K_HEIGHT_MAP_VERSION_4)
		return false;
	Reader &r = c.body;
	int width = r.i32();
	int height = r.i32();
	map.border = c.version >= K_HEIGHT_MAP_VERSION_3 ? r.i32() : 0;
	if (c.version >= K_HEIGHT_MAP_VERSION_4)
	{
		const int32_t numBorders = r.i32();
		if (numBorders < 0 || size_t(numBorders) > r.left() / 8)
			return false;
		r.skip(size_t(numBorders) * 8);
	}
	const int32_t dataSize = r.i32();
	if (!r.ok() || width <= 0 || height <= 0 || width > MaxMapCells || height > MaxMapCells || dataSize != width * height)
		return false;
	const uint8_t *data = r.take(dataSize);
	if (!data)
		return false;

	map.heights.assign(data, data + dataSize);
	if (c.version == K_HEIGHT_MAP_VERSION_1)
	{
		// Cells used to be half as big; keep every other sample like WorldHeightMap::ParseSizeOnly.
		const int newWidth = (width + 1) / 2;
		const int newHeight = (height + 1) / 2;
		for (int y = 0; y < newHeight; ++y)
			for (int x = 0; x < newWidth; ++x)
				map.heights[y * newWidth + x] = map.heights[2 * y * width + 2 * x];
		width = newWidth;
		height = newHeight;
		map.heights.resize(size_t(width) * height);
	}

	if (map.border < 0 || 2 * map.border >= width || 2 * map.border >= height)
		map.border = 0;
	map.width = width;
	map.height = height;
	return true;
}

bool readBlendTiles(Chunk &c, MapData &map)
{
	Reader &r = c.body;
	if (map.heights.empty() || c.version == K_BLEND_TILE_VERSION_1)
		return false;
	const int32_t count = r.i32();
	if (count != map.width * map.height)
		return false;

	const uint8_t *tiles = r.take(size_t(count) * 2);
	r.skip(size_t(count) * 2); // blend tiles
	if (c.version >= K_BLEND_TILE_VERSION_6)
		r.skip(size_t(count) * 2); // extra blend tiles
	if (c.version >= K_BLEND_TILE_VERSION_5)
		r.skip(size_t(count) * 2); // cliff info
	if (c.version == K_BLEND_TILE_VERSION_7)
		r.skip(size_t(map.height) * ((map.width + 1) / 8)); // the old, too short, cliff flag rows
	else if (c.version > K_BLEND_TILE_VERSION_7)
		r.skip(size_t(map.height) * ((map.width + 7) / 8));
	r.i32(); // bitmap tiles
	r.i32(); // blended tiles
	if (c.version >= K_BLEND_TILE_VERSION_5)
		r.i32(); // cliff infos
	const int32_t numClasses = r.i32();
	if (!r.ok() || !tiles || numClasses < 0 || numClasses > 1024)
		return false;

	std::vector<TextureClass> classes(numClasses);
	for (TextureClass &tc : classes)
	{
		tc.firstTile = r.i32();
		tc.numTiles = r.i32();
		tc.width = r.i32();
		r.i32(); // legacy GDF flag
		tc.name = r.asciiString();
	}
	if (!r.ok())
		return false;

	map.tiles.resize(count);
	for (int32_t i = 0; i < count; ++i)
		map.tiles[i] = int16_t(tiles[2 * i] | (tiles[2 * i + 1] << 8));
	map.textureClasses.swap(classes);
	return true;
}

bool readDictWaypointName(Reader &r, const TableOfContents &toc, std::string &waypointName)
{
	const uint16_t pairs = r.u16();
	for (uint16_t i = 0; i < pairs && r.ok(); ++i)
	{
		const uint32_t keyAndType = r.u32();
		const int type = keyAndType & 0xff;
		switch (type)
		{
			case DICT_BOOL: r.u8(); break;
			case DICT_INT: r.i32(); break;
			case DICT_REAL: r.f32(); break;
			case DICT_ASCIISTRING:
			{
				std::string value = r.asciiString();
				TableOfContents::const_iterator it = toc.find(keyAndType >> 8);
				if (it != toc.end() && it->second == "waypointName")
					waypointName = value;
				break;
			}
			case DICT_UNICODESTRING: r.skip(size_t(r.u16()) * 2); break;
			default: r.fail(); break;
		}
	}
	return r.ok();
}

void readObjects(Chunk &list, const TableOfContents &toc, MapData &map)
{
	Chunk c;
	while (nextChunk(list.body, toc, c))
	{
		if (c.label != "Object")
			continue;
		Reader &r = c.body;
		const float x = r.f32();
		const float y = r.f32();
		r.f32(); // z
		r.f32(); // angle
		r.i32(); // flags
		r.asciiString(); // thing template
		std::string waypointName;
		if (c.version < K_OBJECTS_VERSION_2 || !readDictWaypointName(r, toc, waypointName))
			continue;

		int player = 0;
		char tail = 0;
		if (sscanf(waypointName.c_str(), "Player_%d_Star%c", &player, &tail) == 2 && tail == 't' && player >= 1 && player <= 8)
		{
			StartPosition start;
			start.player = player;
			start.x = x;
			start.y = y;
			map.startPositions.push_back(start);
		}
	}
}

void readPolygonTriggers(Chunk &c, MapData &map)
{
	Reader &r = c.body;
	int32_t count = r.i32();
	while (count-- > 0 && r.ok())
	{
		r.asciiString(); // name
		if (c.version >= K_TRIGGERS_VERSION_4)
			r.asciiString(); // layer
		r.i32(); // id
		bool isWater = false;
		if (c.version >= K_TRIGGERS_VERSION_2)
			isWater = r.u8() != 0;
		if (c.version >= K_TRIGGERS_VERSION_3)
		{
			r.u8(); // river
			r.i32(); // river start
		}
		const int32_t numPoints = r.i32();
		if (numPoints < 0 || size_t(numPoints) > r.left() / 12)
			return;

		WaterArea area;
		for (int32_t i = 0; i < numPoints; ++i)
		{
			const float x = float(r.i32());
			const float y = float(r.i32());
			const float z = float(r.i32());
			if (i == 0)
				area.z = z;
			area.x.push_back(x);
			area.y.push_back(y);
		}
		if (r.ok() && isWater && numPoints >= 3)
			map.water.push_back(area);
	}
}

} // namespace

bool readMap(const uint8_t *data, size_t size, MapData &out, std::string *error)
{
	auto failWith = [&](const char *message) {
		if (error)
			*error = message;
		return false;
	};

	Reader r(data, size);
	const uint8_t *tag = r.take(4);
	if (!tag || memcmp(tag, "CkMp", 4) != 0)
		return failWith("not a map file");

	TableOfContents toc;
	const int32_t count = r.i32();
	if (count < 0 || size_t(count) > r.left() / 5)
		return failWith("damaged map: bad table of contents");
	for (int32_t i = 0; i < count; ++i)
	{
		const uint8_t len = r.u8();
		const uint8_t *name = r.take(len);
		const uint32_t id = r.u32();
		if (!r.ok())
			return failWith("damaged map: bad table of contents");
		toc[id] = std::string(reinterpret_cast<const char *>(name), len);
	}

	MapData map;
	Chunk c;
	while (nextChunk(r, toc, c))
	{
		if (c.label == "HeightMapData")
		{
			if (!readHeightMap(c, map))
				return failWith(c.version > K_HEIGHT_MAP_VERSION_4 ? "not a Generals or Zero Hour map" : "damaged map: bad height map");
		}
		else if (c.label == "BlendTileData")
		{
			// Without tiles the terrain is still drawn, just in a neutral color.
			if (!readBlendTiles(c, map))
			{
				map.tiles.clear();
				map.textureClasses.clear();
			}
		}
		else if (c.label == "ObjectsList")
		{
			readObjects(c, toc, map);
		}
		else if (c.label == "PolygonTriggers")
		{
			readPolygonTriggers(c, map);
		}
	}

	if (map.heights.empty())
		return failWith("map has no height map");

	out = std::move(map);
	return true;
}

} // namespace MapThumbnail

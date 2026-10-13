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

// Reads the parts of a map file a thumbnail needs, following the readers in
// WorldHeightMap.cpp, PolygonTrigger.cpp and DataChunk.cpp.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace MapThumbnail
{

const float MapXYFactor = 10.0f;               ///< MAP_XY_FACTOR: world units per height map cell.
const float MapHeightScale = MapXYFactor / 16; ///< MAP_HEIGHT_SCALE: world units per height step.

struct TextureClass
{
	int firstTile = 0;
	int numTiles = 0;
	int width = 0;
	std::string name; ///< Terrain name from Terrain.ini, e.g. "TEDesert1".
};

struct WaterArea
{
	std::vector<float> x; ///< Polygon in world units.
	std::vector<float> y;
	float z = 0;          ///< Water surface height, the first point's z (TerrainLogic::getWaterHeight).
};

struct StartPosition
{
	int player = 0; ///< 1-based, from the Player_<n>_Start waypoint name.
	float x = 0;
	float y = 0;
};

struct MapData
{
	int width = 0;  ///< Height map size in cells, border included.
	int height = 0;
	int border = 0;
	std::vector<uint8_t> heights;
	std::vector<int16_t> tiles; ///< Per cell tile index; tile >> 2 is the source tile. Empty when absent.
	std::vector<TextureClass> textureClasses;
	std::vector<WaterArea> water;
	std::vector<StartPosition> startPositions;

	int playableWidth() const { return width - 2 * border; }
	int playableHeight() const { return height - 2 * border; }
};

/// Parses an uncompressed "CkMp" map. Only HeightMapData is required.
bool readMap(const uint8_t *data, size_t size, MapData &out, std::string *error);

} // namespace MapThumbnail

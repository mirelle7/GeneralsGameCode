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

// Terrain colors for the in-game render, taken from the game's own terrain textures
// (Terrain.ini plus Art/Terrain/*.tga, loose or inside .big archives) when a game folder is known.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace MapThumbnail
{

struct MapData;

struct Rgb
{
	float r = 0;
	float g = 0;
	float b = 0;
};

/// Average color of each source tile, indexed like WorldHeightMap::getSourceTile (tile index >> 2).
/// Tiles whose texture can't be found get a color guessed from the terrain name.
std::vector<Rgb> sourceTileColors(const MapData &map, const std::vector<std::string> &gameDirs);

/// Color guessed from a terrain or texture name, e.g. "GrassType1" or "TEDesert1".
Rgb colorFromTerrainName(const std::string &name);

/// Reads one file from the game folders: a loose file wins over one inside an INI or Terrain .big archive.
/// path uses backslashes, as in the archives, e.g. "Data\\INI\\Terrain.ini".
bool readGameFile(const std::vector<std::string> &gameDirs, const std::string &path, std::vector<uint8_t> &out);

} // namespace MapThumbnail

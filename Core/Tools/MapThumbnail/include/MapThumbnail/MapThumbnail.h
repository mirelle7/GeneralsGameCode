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

// Portable renderer for World Builder map (.map) thumbnails.
//
// It has no dependency on the game engine, so the same code backs the Windows Explorer
// thumbnail handler, the macOS Quick Look extension and the Linux thumbnailer.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace MapThumbnail
{

enum class Mode
{
	InGame, ///< Render the map from its terrain data, like the in-game radar and map select screen.
	Tga,    ///< Show the <map>.tga preview World Builder saves next to the map.
};

struct Image
{
	int width = 0;
	int height = 0;
	std::vector<uint8_t> rgba; ///< Top-down rows, 4 bytes per pixel, straight alpha.
};

struct Settings
{
	Mode mode = Mode::InGame;
	bool showStartPositions = true;
	/// Game install folders. Terrain textures are read from them for true in-game colors;
	/// without them, terrain is colored from the texture names.
	std::vector<std::string> gameDirs;
};

/// True when the data starts like a map file (plain "CkMp" chunk file or an "EAR" RefPack-compressed one).
bool isMapData(const uint8_t *data, size_t size);

/// Renders a thumbnail that fits in maxSize x maxSize, keeping the map's aspect ratio.
/// In Tga mode, falls back to the in-game render when the map has no preview TGA.
bool renderFile(const std::string &mapPath, int maxSize, const Settings &settings, Image &out, std::string *error = nullptr);

/// Same as renderFile for a map already in memory. There is no companion TGA to find, so the
/// in-game render is always used.
bool renderMemory(const uint8_t *data, size_t size, int maxSize, const Settings &settings, Image &out, std::string *error = nullptr);

/// Path of the per-user settings file:
/// Windows %APPDATA%\GeneralsGameCode\MapThumbnail.ini, elsewhere $XDG_CONFIG_HOME/generalsgamecode/MapThumbnail.ini
/// (~/.config when unset). GENERALS_MAP_THUMBNAIL_CONFIG overrides it.
std::string settingsPath();

/// Loads the settings file. A missing file yields the defaults (in-game mode).
/// Game folders found in the registry on Windows are appended when the file names none.
Settings loadSettings();
bool loadSettingsFile(const std::string &path, Settings &out);
bool saveSettingsFile(const std::string &path, const Settings &settings);

const char *modeName(Mode mode);
bool parseMode(const std::string &text, Mode &out);

} // namespace MapThumbnail

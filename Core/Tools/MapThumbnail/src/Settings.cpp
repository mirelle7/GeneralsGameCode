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

// The settings file is a small INI shared by every platform:
//
//   [MapThumbnail]
//   Mode = InGame            ; or Tga
//   ShowStartPositions = 1
//   GameDir = C:\Games\Command and Conquer Generals Zero Hour
//   GameDir = C:\Games\Command and Conquer Generals
//
// GameDir may repeat; earlier folders win.

#include "MapThumbnail/MapThumbnail.h"

#include "FileUtil.h"

#include <cstdlib>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <pwd.h>
#include <unistd.h>
#endif

namespace MapThumbnail
{

namespace
{

std::string trim(const std::string &text)
{
	const size_t first = text.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
		return std::string();
	const size_t last = text.find_last_not_of(" \t\r\n");
	return text.substr(first, last - first + 1);
}

#ifdef _WIN32
std::string wideToUtf8(const wchar_t *text)
{
	const int len = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
	if (len <= 1)
		return std::string();
	std::string out(len - 1, '\0');
	WideCharToMultiByte(CP_UTF8, 0, text, -1, &out[0], len, nullptr, nullptr);
	return out;
}

// The retail installers record their folder here (32-bit registry view).
std::vector<std::string> registryGameDirs()
{
	static const wchar_t *const keys[] = {
		L"SOFTWARE\\Electronic Arts\\EA Games\\Command and Conquer Generals Zero Hour",
		L"SOFTWARE\\Electronic Arts\\EA Games\\Generals",
	};
	std::vector<std::string> dirs;
	for (const wchar_t *key : keys)
	{
		wchar_t value[MAX_PATH];
		DWORD size = sizeof(value);
		if (RegGetValueW(HKEY_LOCAL_MACHINE, key, L"InstallPath", RRF_RT_REG_SZ | RRF_SUBKEY_WOW6432KEY, nullptr, value, &size) == ERROR_SUCCESS)
			dirs.push_back(wideToUtf8(value));
	}
	return dirs;
}
#endif

std::string homeDir()
{
#ifdef _WIN32
	return std::string();
#else
	// Inside the macOS sandbox $HOME is the container, but the settings file lives in the real home.
	if (const passwd *pw = getpwuid(getuid()))
		if (pw->pw_dir && *pw->pw_dir)
			return pw->pw_dir;
	const char *home = getenv("HOME");
	return home ? home : "";
#endif
}

} // namespace

const char *modeName(Mode mode)
{
	return mode == Mode::Tga ? "Tga" : "InGame";
}

bool parseMode(const std::string &text, Mode &out)
{
	const std::string lower = toLower(trim(text));
	if (lower == "ingame" || lower == "in-game" || lower == "game")
		out = Mode::InGame;
	else if (lower == "tga" || lower == "preview")
		out = Mode::Tga;
	else
		return false;
	return true;
}

std::string settingsPath()
{
	if (const char *overridePath = getenv("GENERALS_MAP_THUMBNAIL_CONFIG"))
		if (*overridePath)
			return overridePath;

#ifdef _WIN32
	PWSTR appData = nullptr;
	std::string dir;
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appData)))
		dir = wideToUtf8(appData);
	CoTaskMemFree(appData);
	return dir + "\\GeneralsGameCode\\MapThumbnail.ini";
#else
	const char *xdg = getenv("XDG_CONFIG_HOME");
	std::string dir = (xdg && *xdg == '/') ? xdg : homeDir() + "/.config";
#ifdef __APPLE__
	// A sandboxed extension sees its container in XDG_CONFIG_HOME too, so always use the real home.
	dir = homeDir() + "/.config";
#endif
	return dir + "/generalsgamecode/MapThumbnail.ini";
#endif
}

bool loadSettingsFile(const std::string &path, Settings &out)
{
	std::vector<uint8_t> data;
	if (!readWholeFile(pathFromUtf8(path), data, 1024 * 1024))
		return false;

	Settings settings;
	std::istringstream in(std::string(data.begin(), data.end()));
	std::string line;
	while (std::getline(in, line))
	{
		const size_t comment = line.find_first_of(";#");
		if (comment != std::string::npos)
			line.erase(comment);
		const size_t eq = line.find('=');
		if (eq == std::string::npos)
			continue;
		const std::string key = toLower(trim(line.substr(0, eq)));
		const std::string value = trim(line.substr(eq + 1));

		if (key == "mode")
			parseMode(value, settings.mode);
		else if (key == "showstartpositions")
			settings.showStartPositions = !(value == "0" || toLower(value) == "false" || toLower(value) == "no");
		else if (key == "gamedir" && !value.empty())
			settings.gameDirs.push_back(value);
	}
	out = settings;
	return true;
}

bool saveSettingsFile(const std::string &path, const Settings &settings)
{
	const fs::path file = pathFromUtf8(path);
	std::error_code ec;
	if (file.has_parent_path())
		fs::create_directories(file.parent_path(), ec);

	std::ofstream out(file, std::ios::binary | std::ios::trunc);
	out << "[MapThumbnail]\n";
	out << "; InGame renders the map's terrain like the game does; Tga shows the preview World Builder saved.\n";
	out << "Mode = " << modeName(settings.mode) << "\n";
	out << "ShowStartPositions = " << (settings.showStartPositions ? 1 : 0) << "\n";
	out << "; Game folders to read terrain textures from. Without them, terrain colors are guessed.\n";
	for (const std::string &dir : settings.gameDirs)
		out << "GameDir = " << dir << "\n";
	return bool(out);
}

Settings loadSettings()
{
	Settings settings;
	loadSettingsFile(settingsPath(), settings);
#ifdef _WIN32
	if (settings.gameDirs.empty())
		settings.gameDirs = registryGameDirs();
#endif
	return settings;
}

} // namespace MapThumbnail

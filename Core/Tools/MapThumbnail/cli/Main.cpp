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

// generals-map-thumbnailer: renders a map thumbnail to PNG. It is the freedesktop thumbnailer
// on Linux and a way to try the renderer or change the shared settings on any platform.

#include "MapThumbnail/MapThumbnail.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO
#include <stb_image_write.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

namespace
{

void usage()
{
	fprintf(stderr,
		"Usage:\n"
		"  generals-map-thumbnailer [options] <input.map> <output.png>\n"
		"  generals-map-thumbnailer --show-settings\n"
		"  generals-map-thumbnailer --save-settings [options]\n"
		"\n"
		"Options:\n"
		"  -s, --size N            fit the thumbnail in N x N pixels (default 256)\n"
		"  -m, --mode MODE         ingame (default) or tga\n"
		"  -g, --game-dir DIR      game folder to read terrain textures from (repeatable)\n"
		"      --start-positions   draw the numbered start positions (default)\n"
		"      --no-start-positions\n"
		"\n"
		"Options not given come from the settings file; --save-settings writes them to it.\n");
}

void writeToFile(void *context, void *data, int size)
{
	static_cast<std::ofstream *>(context)->write(static_cast<const char *>(data), size);
}

void printSettings(const MapThumbnail::Settings &settings)
{
	printf("Settings file: %s\n", MapThumbnail::settingsPath().c_str());
	printf("Mode = %s\n", MapThumbnail::modeName(settings.mode));
	printf("ShowStartPositions = %d\n", settings.showStartPositions ? 1 : 0);
	for (const std::string &dir : settings.gameDirs)
		printf("GameDir = %s\n", dir.c_str());
}

} // namespace

int main(int argc, char **argv)
{
	MapThumbnail::Settings settings = MapThumbnail::loadSettings();
	bool gameDirsGiven = false;
	bool showSettings = false;
	bool saveSettings = false;
	int size = 256;
	const char *files[2] = { nullptr, nullptr };
	int fileCount = 0;

	for (int i = 1; i < argc; ++i)
	{
		const std::string arg = argv[i];
		const bool hasValue = i + 1 < argc;
		if ((arg == "-s" || arg == "--size") && hasValue)
		{
			size = atoi(argv[++i]);
		}
		else if ((arg == "-m" || arg == "--mode") && hasValue)
		{
			if (!MapThumbnail::parseMode(argv[++i], settings.mode))
			{
				fprintf(stderr, "Unknown mode '%s'; use ingame or tga.\n", argv[i]);
				return 2;
			}
		}
		else if ((arg == "-g" || arg == "--game-dir") && hasValue)
		{
			if (!gameDirsGiven)
				settings.gameDirs.clear();
			gameDirsGiven = true;
			settings.gameDirs.push_back(argv[++i]);
		}
		else if (arg == "--start-positions")
			settings.showStartPositions = true;
		else if (arg == "--no-start-positions")
			settings.showStartPositions = false;
		else if (arg == "--show-settings")
			showSettings = true;
		else if (arg == "--save-settings")
			saveSettings = true;
		else if (arg == "-h" || arg == "--help")
		{
			usage();
			return 0;
		}
		else if (!arg.empty() && arg[0] != '-' && fileCount < 2)
			files[fileCount++] = argv[i];
		else
		{
			usage();
			return 2;
		}
	}

	if (saveSettings)
	{
		if (!MapThumbnail::saveSettingsFile(MapThumbnail::settingsPath(), settings))
		{
			fprintf(stderr, "Could not write %s\n", MapThumbnail::settingsPath().c_str());
			return 1;
		}
		printSettings(settings);
		return 0;
	}
	if (showSettings)
	{
		printSettings(settings);
		return 0;
	}
	if (fileCount != 2 || size <= 0)
	{
		usage();
		return 2;
	}

	MapThumbnail::Image image;
	std::string error;
	if (!MapThumbnail::renderFile(files[0], size, settings, image, &error))
	{
		fprintf(stderr, "%s: %s\n", files[0], error.c_str());
		return 1;
	}

	std::ofstream out(files[1], std::ios::binary | std::ios::trunc);
	if (!out || !stbi_write_png_to_func(writeToFile, &out, image.width, image.height, 4, image.rgba.data(), image.width * 4) || !out)
	{
		fprintf(stderr, "Could not write %s\n", files[1]);
		return 1;
	}
	return 0;
}

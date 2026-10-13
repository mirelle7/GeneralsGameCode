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

// Turns the map thumbnails on or off for the current user, like MapThumbnailToggle.exe on Windows.
//
// macOS: enables or disables the Quick Look extension with pluginkit.
// Linux: installs or removes the freedesktop thumbnailer entry (and its MIME type) in the
// user's data folder, pointing at the generals-map-thumbnailer next to this program.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__

static int run(const char *command)
{
	return system(command) == 0 ? 0 : 1;
}

int main(void)
{
	const char *id = MAPTHUMBNAIL_EXTENSION_ID;
	char command[512];
	char line[512];
	int found = 0;
	int enabled = 0;
	FILE *pipe;

	snprintf(command, sizeof(command), "pluginkit -m -i %s", id);
	pipe = popen(command, "r");
	if (pipe)
	{
		// Each line starts with '+' (on), '-' (off) or ' ' (default, which is on).
		while (fgets(line, sizeof(line), pipe))
		{
			if (strstr(line, id))
			{
				found = 1;
				enabled = line[0] != '-';
			}
		}
		pclose(pipe);
	}
	if (!found)
	{
		fprintf(stderr, "The Quick Look extension is not registered. Open GeneralsMapThumbnails.app once first.\n");
		return 1;
	}

	snprintf(command, sizeof(command), "pluginkit -e %s -i %s", enabled ? "ignore" : "use", id);
	if (run(command) != 0)
	{
		fprintf(stderr, "Could not change the map thumbnail setting.\n");
		return 1;
	}
	run("qlmanage -r cache >/dev/null 2>&1");
	printf("Map thumbnails are now %s.\n", enabled ? "OFF" : "ON");
	return 0;
}

#else

#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

// Keep in sync with linux/generals-map.xml.
static const char MimeXml[] =
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	"<mime-info xmlns=\"http://www.freedesktop.org/standards/shared-mime-info\">\n"
	"  <mime-type type=\"application/x-generals-map\">\n"
	"    <comment>Command &amp; Conquer Generals map</comment>\n"
	"    <generic-icon name=\"image-x-generic\"/>\n"
	"    <magic priority=\"80\">\n"
	"      <match type=\"string\" offset=\"0\" value=\"CkMp\"/>\n"
	"      <match type=\"big32\" offset=\"0\" value=\"0x45415200\"/>\n"
	"    </magic>\n"
	"  </mime-type>\n"
	"</mime-info>\n";

static char g_data[PATH_MAX];

// Builds g_data + suffix into out (PATH_MAX bytes); fails when it does not fit.
static int dataPath(char *out, const char *suffix)
{
	return snprintf(out, PATH_MAX, "%s%s", g_data, suffix) >= PATH_MAX;
}

static void makeDirs(char *path)
{
	char *p;
	for (p = path + 1; *p; ++p)
	{
		if (*p == '/')
		{
			*p = '\0';
			mkdir(path, 0755);
			*p = '/';
		}
	}
	mkdir(path, 0755);
}

static int writeFile(const char *dir, const char *name, const char *text)
{
	char path[PATH_MAX];
	FILE *file;
	if (dataPath(path, dir))
		return 1;
	makeDirs(path);
	if (dataPath(path, name))
		return 1;
	file = fopen(path, "w");
	if (!file)
		return 1;
	fputs(text, file);
	return fclose(file) != 0;
}

static void updateMime(void)
{
	char command[PATH_MAX + 64];
	if (snprintf(command, sizeof(command), "update-mime-database '%s/mime' >/dev/null 2>&1", g_data) < (int)sizeof(command) &&
		system(command) != 0)
		fprintf(stderr, "update-mime-database failed; the MIME type may not be known until it runs.\n");
}

static int fail(void)
{
	fprintf(stderr, "Could not change the map thumbnail setting.\n");
	return 1;
}

int main(void)
{
	char entry[PATH_MAX];
	char tool[PATH_MAX];
	char text[PATH_MAX * 2 + 128];
	const char *xdg = getenv("XDG_DATA_HOME");
	const char *home = getenv("HOME");
	ssize_t len;
	int failed;

	if (xdg && *xdg)
		failed = snprintf(g_data, sizeof(g_data), "%s", xdg) >= (int)sizeof(g_data);
	else if (home && *home)
		failed = snprintf(g_data, sizeof(g_data), "%s/.local/share", home) >= (int)sizeof(g_data);
	else
		failed = 1;
	if (failed || dataPath(entry, "/thumbnailers/generals-map.thumbnailer"))
		return fail();

	if (access(entry, F_OK) == 0)
	{
		failed = remove(entry) != 0;
		if (!dataPath(entry, "/mime/packages/generals-map.xml"))
			remove(entry);
		updateMime();
		if (failed)
			return fail();
		printf("Map thumbnails are now OFF.\n");
		if (access("/usr/share/thumbnailers/generals-map.thumbnailer", F_OK) == 0 ||
			access("/usr/local/share/thumbnailers/generals-map.thumbnailer", F_OK) == 0)
			printf("A system-wide install still shows them; remove it with your package manager.\n");
		return 0;
	}

	// Use the thumbnailer next to this program, or the one on PATH.
	len = readlink("/proc/self/exe", tool, sizeof(tool) - 1);
	if (len > 0)
	{
		tool[len] = '\0';
		*strrchr(tool, '/') = '\0';
		strncat(tool, "/generals-map-thumbnailer", sizeof(tool) - strlen(tool) - 1);
	}
	if (len <= 0 || access(tool, X_OK) != 0)
		strcpy(tool, "generals-map-thumbnailer");

	snprintf(text, sizeof(text),
		"[Thumbnailer Entry]\n"
		"TryExec=%s\n"
		"Exec=\"%s\" -s %%s %%i %%o\n"
		"MimeType=application/x-generals-map;\n",
		tool, tool);
	failed = writeFile("/mime/packages", "/mime/packages/generals-map.xml", MimeXml) ||
		writeFile("/thumbnailers", "/thumbnailers/generals-map.thumbnailer", text);
	updateMime();
	if (failed)
		return fail();
	printf("Map thumbnails are now ON. Restart your file manager to see them.\n");
	return 0;
}

#endif

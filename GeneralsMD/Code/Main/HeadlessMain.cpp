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

// TheSuperHackers @feature githubawn 21/10/2026 Entry point for platforms without WinMain.
// It does what WinMain does for a -headless run: no window, splash screen or crash dumps,
// and the game engine is a HeadlessGameEngine. Runs without -headless are refused.

#include <cstdio>
#include <string>

#include "Common/CommandLine.h"
#include "Common/CriticalSection.h"
#include "Common/GameEngine.h"
#include "Common/GameMemory.h"
#include "Common/GlobalData.h"
#include "Common/version.h"
#include "GameClient/ClientInstance.h"
#include "StdDevice/Common/HeadlessGameEngine.h"
#include "BuildVersion.h"
#include "GeneratedVersion.h"

// Win32 globals the engine reads. There is no window or instance off Windows.
HINSTANCE ApplicationHInstance = nullptr;
HWND ApplicationHWnd = nullptr;
DWORD TheMessageTime = 0;

const Char *g_strFile = "data\Generals.str";
const Char *g_csfFile = "data\%s\Generals.csf";
const char *gAppPrefix = "";

// The command-line parser reads __argc/__argv and GetCommandLineA, which the Windows CRT
// provides. The shim headers declare them; they are filled in from main's arguments.
int __argc = 0;
char **__argv = nullptr;
const char *g_compatCommandLine = nullptr;

static CriticalSection critSec1, critSec2, critSec3, critSec4;

GameEngine *CreateGameEngine()
{
	HeadlessGameEngine *engine = NEW HeadlessGameEngine;
	engine->setIsActive(true);
	return engine;
}

int main(int argc, char *argv[])
{
	__argc = argc;
	__argv = argv;
	std::string commandLine;
	for (int i = 0; i < argc; ++i)
	{
		if (i > 0)
			commandLine += ' ';
		commandLine += argv[i];
	}
	g_compatCommandLine = commandLine.c_str();

	Int exitcode = 1;

	TheAsciiStringCriticalSection = &critSec1;
	TheUnicodeStringCriticalSection = &critSec2;
	TheDmaCriticalSection = &critSec3;
	TheMemoryPoolCriticalSection = &critSec4;

	initMemoryManager();

	CommandLine::parseCommandLineForStartup();

	if (!TheGlobalData->m_headless)
	{
		fprintf(stderr, "This build has no renderer, audio or input; run it with -headless.\n");
		shutdownMemoryManager();
		return exitcode;
	}

	TheVersion = NEW Version;
	TheVersion->setVersion(VERSION_MAJOR, VERSION_MINOR, VERSION_BUILDNUM, VERSION_LOCALBUILDNUM,
		AsciiString(VERSION_BUILDUSER), AsciiString(VERSION_BUILDLOC),
		AsciiString(__TIME__), AsciiString(__DATE__));

	if (!rts::ClientInstance::initialize())
	{
		fprintf(stderr, "Another instance is already running.\n");
		delete TheVersion;
		TheVersion = nullptr;
		shutdownMemoryManager();
		return exitcode;
	}

	exitcode = GameMain();

	delete TheVersion;
	TheVersion = nullptr;

	shutdownMemoryManager();

	TheAsciiStringCriticalSection = nullptr;
	TheUnicodeStringCriticalSection = nullptr;
	TheDmaCriticalSection = nullptr;
	TheMemoryPoolCriticalSection = nullptr;

	return exitcode;
}

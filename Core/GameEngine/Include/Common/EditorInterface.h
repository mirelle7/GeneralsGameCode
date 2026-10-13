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

// EditorInterface.h
// TheSuperHackers @feature Seam between the game engine and the in-game WorldBuilder editor.
// The engine only talks to the editor through this interface, so the engine libraries do not
// depend on the editor library. TheEditor stays null when the editor is not built
// (RTS_BUILD_OPTION_EDITOR=OFF), and every hook then falls back to the original behavior.

#pragma once

#include "Lib/BaseType.h"

class EditorInterface
{
public:
	virtual ~EditorInterface() {}

	/// Ask the editor to take over the application. Called from the main menu. The switch happens
	/// over the next frames, once the shell map game has been cleared.
	virtual void requestEnter() = 0;

	/// True while the editor owns the frame (input, update and rendering).
	virtual Bool isActive() const = 0;

	/// Called at the start of GameEngine::update. Returns TRUE when the editor handled the frame,
	/// in which case the engine skips its regular client, network and logic update.
	virtual Bool updateFrame() = 0;

	/// Called by the display between Begin_Render and End_Render while the editor is active.
	virtual void drawFrame() = 0;

	/// Platform event hooks. Return TRUE when the editor consumed the event and the game must
	/// not process it.
	virtual Bool handleSDLEvent(const void *sdlEvent) = 0;
	virtual Bool handleWin32Message(void *hwnd, UnsignedInt message, UnsignedInt64 wParam, Int64 lParam) = 0;
};

extern EditorInterface *TheEditor;

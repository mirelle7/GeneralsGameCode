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

// EditorUIHooks.h
// TheSuperHackers @feature Replaces the MFC message boxes and dialogs that the WorldBuilder
// document code used to call directly. The in-game editor is immediate mode and cannot block
// inside a document operation, so messages are queued and shown by the editor UI next frame.

#pragma once

#include "Lib/BaseType.h"
#include "Common/AsciiString.h"

namespace EditorUI
{
	/// Queue a message for the user. Never blocks.
	void message(const char *text);

	/// Formatted variant of message().
	void messageFormat(const char *format, ...);

	/// Returns true if the named object layer is hidden in the layers panel.
	Bool isLayerHidden(const AsciiString &layerName);
}

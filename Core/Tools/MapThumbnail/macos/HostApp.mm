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

// macOS only loads Quick Look extensions that ship inside an app, so this app exists to carry
// MapThumbnailExtension.appex. Launching it once registers the extension.

#import <Cocoa/Cocoa.h>

#include "MapThumbnail/MapThumbnail.h"

int main(int, const char **)
{
	@autoreleasepool
	{
		[NSApplication sharedApplication];
		[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
		[NSApp activateIgnoringOtherApps:YES];

		NSString *settingsPath = [NSString stringWithUTF8String:MapThumbnail::settingsPath().c_str()];
		NSAlert *alert = [[NSAlert alloc] init];
		alert.messageText = @"Generals map thumbnails are installed";
		alert.informativeText = [NSString stringWithFormat:
			@"Finder now previews World Builder .map files, rendered like the game shows them.\n\n"
			@"To show the preview TGA World Builder saves instead, set \"Mode = Tga\" in %@ "
			@"(or run: generals-map-thumbnailer --mode tga --save-settings). "
			@"Add \"GameDir = <game folder>\" lines there for true terrain colors.", settingsPath];
		[alert addButtonWithTitle:@"OK"];
		[alert runModal];
	}
	return 0;
}

# Map Thumbnails

Shows World Builder maps (`.map`) as pictures in Windows Explorer, macOS Finder and Linux file managers.

Two modes, chosen in the settings file:

- **InGame** (default) renders the map from its terrain data the way the game's radar and map select screen draw it: terrain colors, height shading, water areas and the numbered player start positions.
- **Tga** shows the `<map name>.tga` preview World Builder saves next to the map. Maps without one fall back to InGame.

All three platforms share one renderer in `src/`, which has no dependency on the game engine. It reads plain and RefPack-compressed (`EAR`) maps, and decodes TGA files with stb_image.

## Building

The file browser plugins are loaded by the operating system, so they are built 64-bit, on their own rather than as part of the 32-bit game:

```
cmake -S Core/Tools/MapThumbnail -B build/mapthumbnail          # add -G Xcode on macOS
cmake --build build/mapthumbnail --config Release
ctest --test-dir build/mapthumbnail -C Release
```

The `Map Thumbnail` GitHub workflow builds all three and uploads them as artifacts.

## Installing

### Windows

```
regsvr32 MapThumbnailProvider.dll
```

This registers the handler for the current user only (no administrator rights needed). `regsvr32 /u MapThumbnailProvider.dll` removes it. The handler runs inside Explorer so that it can find the TGA next to each map.

### macOS

Copy `GeneralsMapThumbnails.app` to `/Applications` and open it once; that registers the Quick Look extension (System Settings > General > Login Items & Extensions > Quick Look lists it). The extension is sandboxed with read-only access to the file system, which it needs for the TGA next to each map, the settings file and the game folders.

### Linux

```
install -Dm755 generals-map-thumbnailer ~/.local/bin/generals-map-thumbnailer
install -Dm644 linux/generals-map.thumbnailer ~/.local/share/thumbnailers/generals-map.thumbnailer
install -Dm644 linux/generals-map.xml ~/.local/share/mime/packages/generals-map.xml
update-mime-database ~/.local/share/mime
```

or `cmake --install` for a system-wide install. This works with file managers that use freedesktop thumbnailers: GNOME Files, Nemo, Caja and Thunar (through tumbler). Maps are recognised by their content, so other `.map` files keep their usual type.

## Settings

The settings file is `%APPDATA%\GeneralsGameCode\MapThumbnail.ini` on Windows and `~/.config/generalsgamecode/MapThumbnail.ini` elsewhere:

```
[MapThumbnail]
Mode = InGame                ; or Tga
ShowStartPositions = 1
GameDir = C:\Games\Command and Conquer Generals Zero Hour
GameDir = C:\Games\Command and Conquer Generals
```

`GameDir` lines point at game installs. The InGame mode then takes each terrain's color from the game's own textures (`Terrain.ini` and `Art/Terrain`, loose or inside the INI and Terrain `.big` archives), like the game does. Without them it guesses colors from the terrain names. On Windows the retail install folders are found in the registry when no `GameDir` is set.

The command line tool can write the file for you:

```
generals-map-thumbnailer --mode tga --save-settings
generals-map-thumbnailer --show-settings
generals-map-thumbnailer -s 512 "My Map.map" preview.png     # try a render
```

File browsers cache thumbnails, so after changing the mode clear that cache to see it on maps already shown: Disk Cleanup > Thumbnails on Windows, `qlmanage -r cache` on macOS, `~/.cache/thumbnails` on Linux.

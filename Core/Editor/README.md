# In-game WorldBuilder

The classic WorldBuilder, rebuilt with Dear ImGui and running inside the game executable.
It replaces the main menu's "World Builder" button, which used to launch the separate MFC
`WorldBuilder.exe`. The editor does not interact with matches: it is a separate application mode
that takes over the frame while it is open.

## Using it

- Main menu > **World Builder**, or start the game with `-worldbuilder`.
- **File > Open** lists every map the game knows (official and user maps).
- **File > Save / Save As** writes a `.map` that the game and the classic WorldBuilder can load.
  Save into `<user data>\Maps\<Name>\<Name>.map` for the map to show up in skirmish.
- **File > Exit to Main Menu** restarts the shell map and brings the menus back.
- Camera: right drag pans, middle drag rotates and pitches, the wheel zooms, Home resets.
- Panels dock around the 3D view. The layout is saved to `WorldBuilderLayout.ini` in the user data
  folder.

## Build

`RTS_BUILD_OPTION_EDITOR` (default `ON`, forced `OFF` for VC6, which cannot compile Dear ImGui).
When it is off, `TheEditor` stays null and the menu button launches `WorldBuilder.exe` as before.

## How it fits into the engine

| Piece | Where |
|---|---|
| Engine seam (`EditorInterface`, `TheEditor`) | `Core/GameEngine/Include/Common/EditorInterface.h` |
| Frame takeover | `GameEngine::update` returns early while `TheEditor->updateFrame()` handles the frame |
| Rendering | `W3DDisplay::draw` calls `TheEditor->drawFrame()` between `Begin_Render` / `End_Render` |
| Input | `SDL3GameEngine::pollSDL3Events` and the Win32 `WndProc` offer events to the editor first |
| ImGui renderer | `Source/ImGuiRenderer.cpp`, draws through `IRenderBackend` like `Render2DClass`, so it runs on DX8 and bgfx; clip rectangles are applied on the CPU |
| Map document | `Source/EditorDocument.cpp`, port of `CWorldBuilderDoc` load/save |
| 3D view | `Source/EditorViewport.cpp`, port of `WbView3d` (own scene, camera, `WBHeightMap`) |
| Heightmap editing | `Source/WHeightMapEdit.cpp`, the classic `WorldHeightMapEdit` without MFC |

Message boxes and modal dialogs of the classic code are replaced by `EditorUI::message()`
(`Include/Editor/EditorUIHooks.h`), shown in the Messages panel.

## Status

Milestone 1: open, view and save maps.

| Milestone | Content | State |
|---|---|---|
| M1 | Editor mode, ImGui, open/view/save maps, camera | done |
| M2 | Height and texture tools, undo/redo | todo |
| M3 | Objects, waypoints, polygons, water, roads, lights, scorches, properties, layers | todo |
| M4 | Players, teams, build lists, map settings, new map | todo |
| M5 | Script editor | todo |
| M6 | Icons, shortcuts, parity polish | todo |

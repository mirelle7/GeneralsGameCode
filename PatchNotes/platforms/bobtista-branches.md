# bobtista topic branches into `pf/bob`

Worktree `<worktrees>/pf-bob`, branch `pf/bob` (started from `platforms` 8a06e5129e6). Nothing pushed.
Branch refs: `refs/scan/bobtista/b/<name>`. Method: `git merge-tree --write-tree platforms <ref>` per branch
(identical result tree = already present), then merge or hand-port the rest.
Stacked branches: `refactor/append-path-separator` < `bugfix/imagepacker-folder-path-overflow` (merged, the superset);
`bugfix/save-map-label-separators` is contained in both. `fix/missing-texture-mip-fill` < `refactor/d3dx-load-surface`.
The `route-*`, `widen-draw-arguments`, `remove-flat-shroud-shader` branches all share the same 8 route commits.

## Table (48 branches)

| branch | result | detail |
|---|---|---|
| bugfix/archive-listing-separator | MERGED | ArchiveFile listings use native separators (`FileSystem::appendPathSeparator`). Conflict in MapUtil.cpp: took branch's `const AsciiString`. |
| bugfix/assault-transport-new-member-xfer | already present | merge-tree result identical to platforms (trunk has the commits) |
| bugfix/failed-load-esc-menu | MERGED | clean |
| bugfix/firing-tracker-force-reload-xfer | already present | trunk `FiringTracker::xfer` already saves `m_frameToForceReload` (xfer version 2, 15/08/2026); only comment differs |
| bugfix/imagepacker-folder-path-overflow | MERGED | superset of append-path-separator (+1 commit, ImagePacker DirectorySelect overflow). Clean on top of it. |
| bugfix/lan-map-resolution | MERGED | host's map resolved through the map cache; `normalizePathSeparators` helper. 4 conflicts, see below. |
| bugfix/map-cache-separator | already present | trunk has `getPathInDirectory`, `getFileName`, listing-separator-aware `endingStr` in MapUtil.cpp; the rest came with archive-listing-separator |
| bugfix/map-display-name-separator | already present | `getFileName` (from append-path-separator) handles a missing separator |
| bugfix/map-transfer-paths | MERGED | FileTransfer.cpp/GameState.cpp portable-path handling with either separator. 2 files conflicted, took branch. |
| bugfix/pathfinder-move-allies-freed-path | MERGED | clean (already gated by the branch on retail compatibility) |
| bugfix/player-attacked-frame-xfer | already present | trunk `Player::xfer` version 9 saves `m_attackedFrame` |
| bugfix/renderer-init-failure-cleanup | already present | identical tree after merge (DX8Wrapper::Init failure cleanup is in trunk's DX8 backend) |
| bugfix/requeue-path-after-load | already present | trunk `AIUpdate::loadPostProcess` re-asks for the path (`m_waitingForPath && getPath()==nullptr`), same intent |
| bugfix/save-map-label-separators | already present (contained) | its 12 commits are in append-path-separator, merged |
| bugfix/script-engine-save-condition-bindings | MERGED (renumbered) | trunk already uses ScriptEngine xfer version 7 for `m_objectCounts`, so the condition team/object now take **version 8** (branch used 7). New save-format version vs. bobtista's branch: needs user's eye if saves must stay compatible with his tree. |
| bugfix/script-engine-save-object-counts | already present | trunk has the same `m_objectCounts` xfer (version 7) |
| bugfix/script-frame-to-evaluate | already present | identical tree |
| bugfix/scripted-camera-render-rate | MERGED | clean (camera moves advance by logic time) |
| bugfix/water-tracks-after-load | MERGED | clean |
| build/mingw-drop-d3dx8d | MERGED | clean (removes d3dx8 alias in cmake/mingw.cmake) |
| build/mingw-link-order | MERGED + FIXED | adds `ws2_32` to `corei_wwdownload` unconditionally; that breaks non-Windows links, so a follow-up commit guards it with `if(WIN32)`. Also adds `$<LINK_ONLY:g_/z_gameenginedevice>` on gameengine (unconditional in the branch, harmless on MSVC, targets exist everywhere). |
| feat/debug-auto-lan-startup | SKIPPED | trunk already has an earlier, different `NetworkAutoStart` (add/add conflict in NetworkAutoStart.h/.cpp, plus LANAPI.cpp, LANAPICallbacks.h, CommandLine.cpp, GameMain.cpp, MainMenu, NetworkDirectConnect, LanGameOptionsMenu): trunk drives a direct-connect flow, the branch rewrites it to drive through the LAN lobby (`StartLANGame` returns Bool, `shouldOpenLobby`, `hasFailed`). 11 files, whole-feature replacement, RTS_DEBUG only. Left alone because the wasm<->macOS match test depends on the current flow. **User decision**: replace trunk's with the branch's later. |
| feat/saveatframe-cli | MERGED | trunk had an ungated simpler version. Took the branch's hardened parsing (filename validation, reserved device names, `.sav` suffix, exit code, interactive-game only, no MP/replay) and kept the options available in Release (as trunk had them), i.e. functions outside `RTS_DEBUG`. Removed the duplicate gated definitions/table entries. |
| feature/load-external-save-replay-files | MERGED | `-ignoreReplaySyncErrors`: ignored mismatches no longer set `sawCRCMismatch` (took branch). |
| fix/missing-texture-mip-fill | PORTED | branch targets old `MissingTexture::_Init`; trunk moved it to `Init_Legacy_Missing_Texture` (texturecompat). Same fix (fill every mip level with the solid colour) applied there by hand. |
| fix/savegame-load-non-windows | MERGED | opens save maps through `TheFileSystem` (case-insensitive). Conflicts in GameStateMap.cpp: took branch over trunk's `NormalizeWin32PathForHost` fopen. |
| refactor/append-path-separator | MERGED | 14 files conflicted, ~19 hunks. Trunk had its own ad-hoc non-Windows `#ifndef _WIN32` separator code in the same spots (CRCDebug, INIMapCache, MapUtil, TerrainLogic, FileTransfer, Recorder, StatsCollector, GameState, GameInfoWindow, LanGameOptionsMenu, PopupSaveLoad, ReplayMenu, WOLGameSetupMenu): took the branch's `getFileName`/`isPathSeparator` helpers everywhere, deleted trunk's now-unused `getPathLeaf` (MapUtil) and `findPathLeaf` (ReplayMenu). Kept our Switch `avleopard` debug print in BIGFileSystem.cpp. FileTransfer: kept trunk's `GetFileInMapDirectory` use, then replaced by the branch's version in the map-transfer-paths merge. |
| refactor/bezier-no-d3dx | already present | trunk's BezierSegment.h already has its own `BezierMath` (and upstream #2670 owns the deterministic Bezier math) |
| refactor/d3dx-assemble-shader | SKIPPED | conflicts in W3DWater.cpp, W3DProfilerFrameCapture.cpp (trunk reworked the shader/backend creation paths). D3DX-removal on the DX8 path only; no effect on our non-Windows targets. |
| refactor/d3dx-create-texture | SKIPPED | 7 conflict hunks in dx8wrapper.cpp; trunk's texturecompat layer owns texture creation now |
| refactor/d3dx-error-string-fvf-size | SKIPPED | conflicts in HeightMap.cpp, dx8fvf.cpp, dx8wrapper.cpp |
| refactor/d3dx-filter-texture | SKIPPED | conflicts in TerrainTex.cpp, W3DTreeBuffer.cpp, dx8wrapper.cpp; trunk's `Copy_Legacy_Surface_Compat`/mip generation is the other form |
| refactor/d3dx-load-surface | SKIPPED | conflicts in dx8wrapper.cpp, missingtexture.cpp, surfaceclass.cpp (its mip-fill part ported separately, see above) |
| refactor/d3dx-matrix-math | SKIPPED | 8 files, 36 hunks (W3DShaderManager.cpp alone has 21): D3DX matrices to Matrix4x4 in shaders conflicts with the backend/bgfx matrix paths. Worth redoing by hand only if D3DX removal on Windows is wanted. |
| refactor/remove-flat-shroud-shader | PARTLY MERGED | only the last commit (unused flat shroud shader removal, 6a5ce47124c) cherry-picked (kept bobtista as author); its other 8 commits are the route-* stack. |
| refactor/route-buffers-draws | SKIPPED (by design) | reworks the render-backend object into the static `Renderer`; we keep the polymorphic `IRenderBackend` plus a thin `Renderer` facade. 48 files / ~2800 lines of conflict. |
| refactor/route-device-queries | SKIPPED (by design) | same |
| refactor/route-lights-fog | SKIPPED (by design) | same |
| refactor/route-render-targets | SKIPPED (by design) | same |
| refactor/route-shader-material | SKIPPED (by design) | same |
| refactor/route-texture-binding | SKIPPED (by design) | same |
| refactor/route-transforms | SKIPPED (by design) | same |
| refactor/widen-draw-arguments | SKIPPED (by design) | same (widens Renderer draw args to 32 bit; our backend interface differs) |
| test/nod3dx-integration | test/experiment | not merged |
| test/pr3332-skip-jam | test/experiment | not merged |
| test/pr3332-skip-poison | test/experiment | not merged |
| test/pr3332-skip-retail | test/experiment | not merged |
| test/pr3492-loopsound | test/experiment | not merged |

## Conflicts resolved (summary)
- append-path-separator: see table row. Rule: branch's helpers win over trunk's ad-hoc copies (same intent, shorter, handles both separators on all platforms; Windows behaviour unchanged since native separator stays `\`).
- lan-map-resolution: PathUtil.h kept both `normalizePathSeparators` (branch) and `getFirstPathSeparator` (trunk) (the diff hunk interleaved them); MapUtil.cpp `findMap` now uses `FileSystem::normalizePathSeparators` instead of trunk's inline `std::replace`; LanGameOptionsMenu (both games) kept trunk's `NetworkAutoStart` map override and only took the braces.
- map-transfer-paths: FileTransfer.cpp (`GetFileInMapDirectory`, `GetPreviewFromMap` via `removeExtension`) and GameState.cpp (`findLastPathSeparatorInRange`) took the branch.
- fix/savegame-load-non-windows: GameStateMap.cpp took branch.
- condition-bindings: hand-merged into xfer version 8 (see above).
- saveatframe-cli: duplicate parse functions/table entries between trunk's and the branch's version removed; GlobalData fields and ZH GameLogic kept trunk's ungated form; Generals GameLogic took the branch's body.
- load-external-save-replay-files: took branch in CommandLine.cpp and Recorder.cpp.
- Extra commits not from bobtista: `refactor(pathutil): Drop leaf helpers ...`, `fix(saveload): Remove stray brace ...` (my slip, in the condition-bindings follow-up), `build(cmake): Link ws2_32 for WWDownload only on Windows`, `fix(texture): Fill every MissingTexture mip level directly`.

## Cross-platform multiplayer / non-Windows data loading (matters for the wasm<->macOS test)
Merged and relevant: native-separator archive listings (ArchiveFile), map cache paths and display names, map transfer paths (FileTransfer), LAN host map resolution through the map cache, save-game map open through the file system and portable map path handling. Net effect: the same map name string resolves on `\`-listing and `/`-listing platforms. The running test is on `platforms`, which does not have any of this yet.
Behaviour to retest on non-Windows: ArchiveFile listing now returns native `/` separators from BIG archives on Linux/macOS/wasm (previously `\`); consumers in MapUtil already follow the actual separator, but any other code that string-matches `\` in archive listing results would break, only those two spots were checked.

## Needs your decision
1. `feat/debug-auto-lan-startup` skipped (replaces trunk's NetworkAutoStart wholesale).
2. ScriptEngine save version: condition bindings are version 8 here, 7 on bobtista's branch.
3. d3dx-* branches and route-*: skipped, see above; they only matter for Windows DX8 D3DX removal and the upstream Renderer refactor.
4. ArchiveFile native separators on non-Windows (see above) before taking this branch into `platforms` mid-test.

## Verification
- No conflict markers; no new/removed files (CMake source lists unchanged); added lines scanned for VC6 hazards (auto, lambdas, `= delete`, `using X =`, `<cstdint>`, enum class, unique_ptr): none.
- Compile check: MSVC `win32` preset (DX8, Debug), build dir <worktrees>/pf-bob/build/win32, targets z_generals and g_generals: 2046/2046 steps, 0 errors, generalszh.exe and generalsv.exe both link (script and logs: windows/bobmsvc.bat, windows/bob-win32-*.log in this folder). Not built: VC6, non-Windows targets, bgfx/SDL3 presets (the replaced non-Windows-only `#ifndef _WIN32` code is untested here).

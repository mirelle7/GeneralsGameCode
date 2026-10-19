# Log: group E_devices_main

All 21 files are resolved. No conflict markers remain. Two old-path files were deleted. Their content was already in trunk's new location, or has been ported there.

Rule numbers refer to RESOLVER-BRIEF.md.

| file | hunk | resolution | rule | NEEDS-REVIEW? |
|---|---|---|---|---|
| Core/GameEngine/Include/Precompiled/PreRTS.h | atlbase / vfw `_WIN32` gates (2) | ours. Theirs gated the same headers. Also removed theirs' stray `#endif` after vfw.h, which unbalanced trunk's larger `#ifdef _WIN32` block. The file now equals HEAD. | 2 | no |
| Generals/Code/GameEngine/Include/Precompiled/PreRTS.h (MD) | whole file | deleted. Trunk c95cf1c11fb unified PreRTS into Core/GameEngine/Include/Precompiled/PreRTS.h, which already has theirs' atlbase/vfw gating. | MD | no |
| Core/.../Bink/BinkVideoPlayer.h | bink.h gate | ours: trunk stubs HBINK. Removed theirs' trailing `#endif // _WIN32`, which would have been unbalanced. The file equals HEAD. | 2 | no |
| Core/.../StdBIGFileSystem.cpp | parse filename | combined: trunk's `fileInfo.` (value) plus theirs' `__SWITCH__` leopard debug log, changed from `fileInfo->` to `fileInfo.` | 1 | no |
| Core/.../StdLocalFileSystem.cpp | path iteration loop | ours (`auto p`, which is safe on libc++) | 2 | no |
| Core/.../BaseHeightMap.cpp | includes | combined: trunk's `WW3D2/` and `WWMath/` includes plus theirs' `__ANDROID__` android/log.h and time.h. Dropped `d3dx8core.h`, which trunk removed. | 1 | no |
| Core/.../W3DProgressBar.cpp | userdata casts (3) | ours | 2 | no |
| Core/.../W3DDisplay.cpp | draw() entry | combined: `ggcD("draw ENTER")` plus trunk's `W3D_UpdateScreenshotMessages()` | 1 | no |
| Core/.../W3DDisplay.cpp | ParticleSys update | ours. Trunk removed `TheParticleSystemManager->update()` here. Theirs' `ggcD` breadcrumb now follows `TheParticleSystemManager->DRAW()`. `ggcD` does nothing except on Switch. | 1 | no |
| Core/.../W3DDisplay.cpp | UI_DRAW profile block (2) | combined: trunk's `GGC_RPROFILE` scope plus theirs' `ggcD` breadcrumbs | 1 | no |
| Core/.../W3DDisplay.cpp | drawImage uv_rect | combined: trunk's `makeAtlasSafeUVRect` plus theirs' `__ANDROID__` log. The log now reads `uv_rect.Left/Top/Right/Bottom`, because the `uv` variable no longer exists in trunk. | 1 | no |
| Core/.../W3DSmudge.cpp | render() start | combined: trunk's early-return when there are no smudges, then theirs' `m_ggcRenderSkip & 16` check | 1 | no |
| Core/.../Water/W3DWaterTracks.cpp | render() lock, unlock, drawBatch (3) | ours. Trunk rewrote this on `RenderVertexBufferClass` and `AppendLockClass`, which capture per track. Theirs' bgfx optimisation captured once per batch: it re-locked the raw DX8 buffer in drawBatch. That code uses `Get_DX8_Vertex_Buffer` and `D3D_OK`, so it can't be ported to the new API without writing new code. **The ~190 updates/frame cost on GLES (Android) comes back.** | 2/7 | **YES** |
| Core/.../Water/W3DWaterTracks.cpp | End_Water_Overlay | combined: theirs' `__ANDROID__` track-count log plus trunk's `WW3D::Get_Render_Backend()` calls | 1 | no |
| GeneralsMD/.../W3DParticleSys.cpp (MD) | doParticles | **ported** to Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DParticleSys.cpp (trunk renamed it in a54d97dec3a): `m_ggcRenderSkip & 2` early-return. Old file deleted. | MD | no |
| GeneralsMD/.../Shadow/W3DProjectedShadow.cpp | includes | combined: trunk's includes plus theirs' VC6 (`_MSC_VER < 1300`) `namespace std { using ::fopen; ... }` alias. Trunk still calls `std::fopen`, `std::fprintf` and `std::fclose` here. | 1/4 | no |
| GeneralsMD/.../Shadow/W3DShadow.cpp | includes | ours. Trunk no longer uses `std::getenv` in this file (GgcRuntimeFlags replaced it), so theirs' alias isn't needed. | 2 | no |
| GeneralsMD/.../W3DAssetManager.cpp | lstrcpyn length | ours | 2 | no |
| GeneralsMD/.../W3DWebBrowser.cpp | closing `#endif` | ours. Also removed theirs' duplicate inner `#if defined(_WIN32)`, since trunk already wraps the whole file. The file equals HEAD. | 2 | no |
| GeneralsMD/.../W3DScene.cpp | visibility, cull, renderOne, mainLoop (4) | combined: trunk's `SceneDiagEnabled` counters plus theirs' `__ANDROID__` `g_ggc*` counters and the cull dump | 1 | no |
| SDL3Mouse.h | 3 hunks | ours (trunk's cursor members, `draw()`, `setVisibility`, `syncPositionToSystemCursor`). Added theirs' `addSDL3FingerEvent`. Theirs' UWP-only `draw()` declaration and its Apple/Emscripten and UWP members were dropped, because trunk's members replace them. | 1/2 | no |
| SDL3Mouse.cpp | 9 hunks | Rebuilt from trunk with three additions. (a) `__ANDROID__` android/log.h. (b) **UWP cursor:** on `GGC_PLATFORM_UWP`, `draw()` skips the SDL hardware colour cursor (Xbox shows no OS cursor), so it always draws the Mouse.ini 2D image. `drawFallbackCursor()` draws theirs' filled-span arrow on UWP; trunk's flag-gated line arrow stays on every other platform. (c) theirs' `addSDL3FingerEvent`, unchanged. Dropped theirs' Apple/Emscripten SDL system cursors: trunk now builds colour cursors from the game art on every platform. Dropped theirs' `capture`/`releaseCapture` platform split: trunk always uses grab and never relative mode, so the touch, Switch and UWP "absolute" fix is no longer needed. Dropped the `m_inputMovesAbsolute = TRUE` in `reset()`: trunk's `Mouse::reset()` no longer clears that flag. | 1/2 | **YES**: test the UWP cursor, and touch on Android/iOS/Switch |
| SDL3GameEngine.h | members | combined: trunk's resize and text-input members plus theirs' `SDL3Gamepad *m_gamepad`. Trunk removed `m_isInitialized`. | 1 | no |
| SDL3GameEngine.cpp | includes | combined: trunk's strings.h block plus theirs' `__ANDROID__` android/log.h | 1 | no |
| SDL3GameEngine.cpp | ctor init list | combined: trunk's members plus `m_gamepad(NULL)`, last to match declaration order | 1 | no |
| SDL3GameEngine.cpp | gamepad update | theirs' `m_gamepad->update(...)` now sits right after the SDL poll loop, before trunk's resize-settle code. It still runs at the end of the event poll, as in theirs. | 1 | no |
| SDL3GameEngine.cpp | createAudioManager | combined: trunk's `GGC_NO_AUDIO` handling plus `OpenALAudioManager`, then theirs' `#else return NEW DummyAudioManager;`. The trailing `return NULL` is dropped. | 1 | no |
| SDL3GameEngine.cpp | (auto-merged) | theirs' gamepad `handleEvent`, `FINGER_*` routing, Switch button log, `handleFingerEvent` and `GGC_GetDeviceResolution_SDL3` came through unchanged | 1 | `handleFingerEvent` lacks trunk's `m_headless` guard that the other mouse handlers have. Not added (rule 5). |
| SDL3Main.cpp | 11 hunks | **Rebuilt from HEAD** (trunk plus wasm-clean) with theirs' platform blocks added. See the next section. | 1/2 | **YES** (see the notes in the next section) |

## SDL3Main.cpp details

### Kept from HEAD
- Emscripten IDBFS (XDG_DATA_HOME), `ggc_ws_connect`, and the canvas-size windowing
- `parseCommandLineForStartup`, `DEBUG_INIT`, MiniDumper, headless handling, ClientInstance
- the macOS fullscreen Space code
- `ApplicationHWnd = (HWND)TheSDL3Window`, which trunk's BgfxBackend expects to be an `SDL_Window*`
- `g_strFile` with forward slashes
- `TheSDL3MetalLayer` defined under `__APPLE__`, which covers iOS too

### Ported from theirs
- **iOS:** TargetConditionals.h, the chdir to `$HOME/Documents`, `GENERALS_USER_DIR`, and the ggc_ios_boot.log boot log
- **Android:**
  - crash handler and its includes
  - `SDL_HINT_NO_SIGNAL_HANDLERS` and `TOUCH_MOUSE_EVENTS=0`
  - data-dir probe and chdir (`GGC_DirHasBigArchive`)
  - surface-settle event pump
- **Switch:** extern block, `ggcSwitchLog`, TLS probe, `socketInitialize`, `GENERALS_USER_DIR`, SD-card data-dir chdir, and `romfsUnmount`/`socketExit` at shutdown
- **UWP:**
  - `__wargv`/`__argv` setup
  - SDL pump (50x10ms) right after `SDL_Init`
  - LocalState/package data dir and `GENERALS_USER_DIR`
  - stdout to ggc_log.txt and stderr to ggc_err.txt
  - `ggcUnhandledExceptionFilter`, set after trunk's MiniDumper filter so it takes over on UWP
  - cmdline.txt args, which replace `__argv` before `parseCommandLineForStartup` reads `__argc`/`__argv`
- **macOS desktop-GL build:** `GGC_BGFX_RENDERER_GLSL` uses `SDL_WINDOW_OPENGL` and skips `SDL_Metal_CreateView`
- `SDL3/SDL_main.h`, now included only on Android, Switch, UWP and iOS. Theirs included it everywhere. It is excluded on desktop Windows, which uses trunk's `/ENTRY:mainCRTStartup` and must stay identical, and on wasm, Linux and macOS, where HEAD works without it. **NEEDS-REVIEW** if Switch or UWP turn out not to need it.

### Dropped from theirs
- `CriticalSection` statics plus `initMemoryManager()`: trunk's SDL3 entry deliberately uses the null memory manager. Rule 2. **NEEDS-REVIEW** if UWP crashes in early AsciiString use.
- `createGlobalData()`: trunk uses `parseCommandLineForStartup`.
- theirs' early Emscripten block (IDBFS at GENERALS_USER_DIR=/preferences): HEAD's wasm-clean replaces it. This dropped theirs' **default Skirmish.ini seed (FPS=61) for web**, and HEAD doesn't have it. **NEEDS-REVIEW** if you want it back; it would need trunk's XDG user-data path.
- theirs' real-HWND `ApplicationHWnd` on `_WIN32`: trunk's bgfx `GetNativeWindowHandle` casts it to `SDL_Window*`, so a real HWND would break desktop Windows. On UWP theirs fell back to the SDL window pointer anyway, which is what trunk does.
- `<io.h>` (unused)

## Notes for builders
- **GlobalData (other group)** must keep:
  - `m_ggcRenderSkip` in both Generals and GeneralsMD GlobalData, because Core W3DParticleSys, W3DSmudge, BaseHeightMap and W3DWaterTracks read it. Core W3DParticleSys.cpp now compiles for Generals too.
  - the `GENERALS_USER_DIR` environment override, which SDL3Main sets on Android, iOS, Switch and UWP.
- **BgfxBackend (other group, moving to `WW3D2/Backend/`)** must keep these, which my files reference:
  - `GGC_BgfxSetRenderScale(float)`, called from W3DDisplay.cpp under `GGC_RENDER_BACKEND_BGFX`
  - the `g_ggcVisForced/Hidden/Passed/Culled`, `g_ggcHlodSeen`, `g_ggcRenderOneObject` and `g_ggcReallyVisible` definitions (`__ANDROID__` externs in W3DScene.cpp)
- These files come from theirs and aren't in HEAD; they must be in the build:
  - New files: `Core/GameEngineDevice/Include/StdDevice/Common/DummyAudioManager.h`, used by SDL3GameEngine on non-OpenAL builds. Check whether it has a .cpp.
  - Already listed in GeneralsMD/Code/GameEngineDevice/CMakeLists.txt: `SDL3Gamepad.{h,cpp}`.
- SDL3 gamepad (Windows and UWP, created on every `_WIN32` SDL3 build, so desktop Windows SDL3 builds also get gamepad emulation, as in theirs):
  - It sends synthetic mouse events in window **pixel** coordinates. Trunk's `addSDL3MotionEvent`/`addSDL3ButtonEvent` now map window **logical** coordinates to display coordinates, including letterbox.
  - When window, pixel and display sizes match (Xbox fullscreen) nothing changes. Otherwise the gamepad cursor will be off. Needs a UWP test.
- The stb_truetype font (render2dsentence.cpp, UWPPlatformStubs.cpp) is not in this group.
- `GGC_SDL3_MAIN` is a CMake option (cmake/sdl3.cmake and GameEngineDevice CMakeLists in theirs) and is not in this group. Trunk already runs SDL3Main on Windows through `/ENTRY:mainCRTStartup`. Make sure the UWP build doesn't also get that `/ENTRY` link flag, since UWP uses `SDL_main.h`.
- SDL3Main.cpp includes trunk's `<windows.h>` compat shim before the new Android and Switch system headers (signal.h, unwind.h, dlfcn.h and so on). Watch for macro clashes on the first Android or Switch build.
- If `RTS_ENABLE_CRASHDUMP` is on for UWP, trunk's MiniDumper is set up first and then theirs' filter replaces it.

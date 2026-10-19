# build-wasm (pf/wasm, worktree <worktrees>/pf-wasm)

Result: configure OK, build OK (link OK), RAN: Zero Hour main menu with live shell map in headless Chrome (SwiftShader WebGL2), pthreads, crossOriginIsolated=true. No page errors. Not run: gameplay beyond menu, -mathCrcCheck / determinism comparison, real GPU, perf. Modern mode (RETAIL_COMPATIBLE_GAME=OFF). Nothing deployed.

## Commands that worked
Env (PowerShell): VS BuildTools devshell (cmake/ninja by full path), <tools>/emsdk (5.0.3) with EMSDK, EM_CONFIG=<tools>/emsdk\.emscripten, emsdk node/python/upstream\emscripten on PATH, C:\Program Files\Git\usr\bin on PATH (bgfx patch step needs `sh`).
FFmpeg: the script has CRLF in the worktree and no `make` exists on Windows, so it was built in WSL Ubuntu with a separate emsdk 5.0.3 (~/emsdk): `tr -d '\r' < scripts/build/web/build-ffmpeg-wasm.sh > ~/bf.sh; bash ~/bf.sh ~/ffmpeg-wasm`, copied to build/ffmpeg-wasm. (An older copied lib built without -pthread fails to link: "--shared-memory disallowed by refstruct.o".)
Configure: cmake --preset web-generalsmd-sdl3-bgfx -DCMAKE_MAKE_PROGRAM=<VS ninja> -DRTS_BUILD_OPTION_RETAIL_COMPATIBLE_GAME=OFF -DFFMPEG_INCLUDE_DIR=build/ffmpeg-wasm/include -DFFMPEG_{AVCODEC,AVFORMAT,AVUTIL,SWSCALE}_LIBRARY=build/ffmpeg-wasm/lib/lib*.a
Build: cmake --build build/web-generalsmd-sdl3-bgfx --config Release --target z_generals -j8  (host shaderc is built automatically)
Output: build/web-generalsmd-sdl3-bgfx/web/Release/index.{html,js,wasm} (wasm 23.8 MB).
Run: write serve.ini (untracked/ignored) with STEAM_ZH_DIR=<retail-zh-install-dir>, `python serve.py` in the worktree root, open http://localhost:8000/ and click "Use the local server" (then ~1 min to menu under SwiftShader). Headless harness: puppeteer-core in the scratchpad.

## Commits (git log platforms..pf/wasm); [S] = shared-code
- 38f725a0266 [S] wwlib: drop duplicate non-Windows RegistryClass stub (already done by every target)
- e697ba0dd95 [S] utility: WWVegas include dir for shim windows.h PCH (same as android/switch)
- ebd3dd874a0 [S] Lib/Profile.h includes; legacy profiler built only on WIN32 (core_debug is Windows-only); drop SDL3Main stack-dump stubs now inline in StackDump.h
- 21e1678aba9 [S] SDL3Main core_minidumper header/signature (cherry-pick from pf/xbox)
- 031b1531566 [S] W3DView Acos/Sqrt (xbox); f2a193209c9 [S] W3DTerrainParticles via backend (xbox); acbd2d1895f [S] DummyAudioManager return types (xbox); ab7f04763ed [S] bgfx maybe_unused (xbox); 6491e8b17e3 [S] AndroidWin32Stubs trim (android)
- afbe998258b [S] Lib/Debug.cpp: include time_compat.h for GetTickCount (non-Windows)
- c756988534e [S] render2dsentence: no DX8 caps on bgfx (max texture 2048)
- a88032e89bc [S, IMPORTANT] gamemath.cmake: rename GameMath's msun kernels (__ieee754_rem_pio2, __kernel_{cos,sin,tan,rem_pio2}) with gm_ prefix on non-Windows. SDL3's bundled libm defines the same names, and __kernel_rem_pio2 has a different signature; with static SDL3 (wasm, Switch, iOS) the linker picked SDL's copy and wasm-ld produced an INVALID module (binaryen: "popping from empty stack"). Also means GameMath's cos/sin used SDL's kernels, i.e. determinism hazard on any static-SDL target. Switch/iOS probably need this too.
- 5157fb9e472 bugfix(web): shell.html local-server path returned before removeRunDependency, so the engine never started (pre-existing in wasm-clean).

## Notes / not fixed
- GM_ENABLE_INTRINSICS is OFF in the wasm cache by default (no FORCE needed); FE_* defines are still in cmake/gamemath.cmake.
- FreeType + embedded DejaVu fonts, framepacer, GameMain loop, SDL3Main IDBFS all survived the merge (UI text renders).
- Console noise: WebGL getInternalformatParameter INVALID_ENUM warnings (bgfx), one "Missing texture task: trstrtholecvr.tga", relay ws://localhost:8090 refused (no relay running), "1 file clean install lacks Data/INI/INIZH.big" (Steam layout), serve.py needs serve.ini for this machine's Steam path.
- NEEDS-REVIEW from merge logs: Skirmish.ini FPS=61 web seed dropped; GENERALS_USER_DIR path change not exercised.
- Extra emsdk 6.0.5 installed at <tools>/emsdk-6 (unused in the end; safe to delete) and ~/emsdk, ~/ffmpeg-wasm in WSL.

## Update after platforms b06aa9529e0 (pf/wasm fast-forwarded)
- Incremental rebuild OK. New commit: restore Core/Libraries/Source/WWVegas/WWLib/registry_unix_stub.cpp. The per-target "drop duplicate registry stub" fixes each deleted a different one of the two stubs, so `platforms` has NEITHER file and configure fails ("Cannot find source file registry_unix_stub.cpp"). platforms needs that file back (take it from 38f725a0266).
- SimulationMathCrc = 76B53840, SimulationMathCrcDouble = 4E2B6D2C in the wasm build, modern mode, via `-mathCrcCheck`: identical to the Linux x86_64 reference. Method: headless Chrome, a setter on window.Module (evaluateOnNewDocument) that adds `arguments=['-mathCrcCheck']` before index.js reads it, "Use the local server" link, result read from console.log (printf) lines.
- Environment oddity: this Steam install's MapsZH.big/TerrainZH.big start with zero bytes, so BIGFileSystem logs "archive identifier mismatch"; menu/shell map still run.
- Leftovers: <tools>/emsdk-6 (unused), WSL ~/emsdk (only needed to rebuild FFmpeg), WSL ~/ffmpeg-wasm (copy already in build/ffmpeg-wasm), ~/bf.sh. All safe to delete.

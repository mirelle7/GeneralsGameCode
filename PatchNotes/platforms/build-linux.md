# build-linux (pf/linux)

Result: configure OK, build OK (target z_generals -> ~/build/pf-linux/GeneralsMD/Release/generalszh), modern mode (RETAIL_COMPATIBLE_GAME=OFF).
Ran: -headless -mathCrcCheck OK; -headless -quickstart ran 100s with no crash (timeout); -replay (2v2 HardAI) reported 0 errors but wall time 0:00:00, so probably not a real simulation (unverified).
Linux's default `all` target also tries z_wwaudio (needs Miles mss.h) which fails; build only `z_generals`.

## Math CRC (modern mode, Linux x86_64 gcc 13): SimulationMathCrc = 76B53840, SimulationMathCrcDouble = 4E2B6D2C
(must be run with cwd = game data dir: <test-exe-dir>, because INI loading happens before -mathCrcCheck is parsed)

## Environment (no sudo; user-space sysroot)
Missing system dev packages; instead apt-get download + dpkg -x into ~/pf-linux-sysroot (libfreetype-dev, libfontconfig-dev, libopenal-dev, ffmpeg libav*-dev + runtime libs, pkgconf, libvulkan-dev, libpng/zlib/brotli/expat/uuid dev), pkg-config .pc prefixes sed-fixed, dangling .so symlinks relinked to /usr/lib.
Equivalent for the user (one line): sudo apt install pkg-config libfreetype-dev libfontconfig-dev libopenal-dev libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev libvulkan-dev libxtst-dev
Commands: cmake --preset linux-generalsmd-sdl3-bgfx -B ~/build/pf-linux -DSDL_X11_XTEST=OFF -DRTS_BUILD_OPTION_RETAIL_COMPATIBLE_GAME=OFF (+PKG_CONFIG_PATH, CMAKE_PREFIX_PATH to sysroot; -DSDL_X11_XTEST=OFF only needed without libxtst-dev)
 ; ninja -f build-Release.ninja -j8 z_generals
Run: LD_LIBRARY_PATH=<build>/_deps/sdl3-build:<build>/_deps/openal_soft-build:<sysroot libdir> generalszh -headless ...

## Commits (git log --oneline platforms..pf/linux) [S = shared-code, other targets need it too]
e68b84bea13 build(wwlib): drop duplicate registry stub block [S]
345061775a9 build(utility): no windows.h shim PCH on Linux (Linux only)
e83ea3b4011 build(profile): debug headers via -iquote where core_debug absent (non-Win) [S]
56920efbebf build(lib): shim windows.h include dirs for Debug.cpp non-Win [S]
7de8e975831 build(lib): Debug.cpp mmsystem.h for GetTickCount; profile include fix [S]
a77babb291f bugfix(profile): Lib/Profile.h [S]
35eb6c0c5eb build(minidumper): SDL3Main/AndroidWin32Stubs follow core_minidumper [S]
d283a2c89e8 160aa01f521 ed87b67815c 1672e3eb16b: cherry-picked from pf/xbox (audio, w3dview, terrain particles, bgfx unused vars) [S]
8429a418dcd build(main): drop stack-dump no-ops [S]; d3e1523e530 (from pf/android) and 426edeaddd2 (from pf/xbox) trim duplicate stubs [S]
(Overlaps with android/xbox branches will conflict textually on merge-back but are equivalent.)

## Not done / notes
- Vulkan device not exercised (headless only; WSL). No windowed run.
- Static libfreetype/fontconfig .a in sysroot were initially picked up; fixed by relinking .so.

## Re-check after merge-back (platforms b06aa9529e0)
pf/linux fast-forwarded; merge-back left registry_unix_stub.cpp referenced but deleted: restored (commit "build(wwlib): Restore the non-Windows registry stub ..."). Rebuilt OK.
-headless -mathCrcCheck (cwd zhcooptest): SimulationMathCrc = 76B53840, SimulationMathCrcDouble = 4E2B6D2C (unchanged).
-loadmap "Maps\Alpine Assault\Alpine Assault.map" -headless -noFPSLimit ran 150s to timeout, no crash (no frame log in Release).
-replay: "-jobs N" uses WorkerProcess, which is a stub on non-Windows (always reports success, 0 s): do not use -jobs on Linux. In-process -replay works but the only replays on disk use a custom map (missing) and were recorded in retail mode, so CRC mismatch at frame 111 is not meaningful.

# Build report: xbox (UWP, Zero Hour) on pf/xbox

Configure OK, build OK (compile + link, 8.7 MB generalszh.exe, modern mode RETAIL_COMPATIBLE_GAME=OFF), package OK (1.0.50.0 msix, 6 MB, signed).
NOT run: console <xbox-ip> unreachable (ping "Destination host unreachable", Device Portal timeouts over ~3 minutes; console off/asleep). Nothing deployed, no shell-map/fps/screenshot, `generals\` BIG fallback not verified on device.

Commands (scratchpad wrappers: dev shell `VsDevCmd -arch=x64 -app_platform=UWP`, VS cmake/ninja by full path):
- configure (build dir build\uwp-x64): cmake -G Ninja -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=10.0 -DCMAKE_BUILD_TYPE=Release "-DCMAKE_CXX_FLAGS=/DWIN32 /D_WINDOWS /EHsc" "-DCMAKE_C_FLAGS=/DWIN32 /D_WINDOWS" -DRTS_FLAGS=/W3 -DRTS_BUILD_GENERALS=OFF -DRTS_BUILD_ZEROHOUR=ON -DRTS_BUILD_*_TOOLS/EXTRAS=OFF -DRTS_BUILD_OPTION_RETAIL_COMPATIBLE_GAME=OFF -DSAGE_USE_SDL3=ON -DSAGE_USE_OPENAL=ON -DRTS_BUILD_OPTION_FFMPEG=OFF -DGGC_RENDER_BACKEND=bgfx -DGGC_BGFX_RENDERER=dx11 -DGGC_SDL3_MAIN=ON -DCMAKE_PREFIX_PATH=<tools>/vcpkg/installed/x64-uwp -DFETCHCONTENT_SOURCE_DIR_SDL3=<tools>/xbox-deps/SDL3-uwp -DGGC_SHADERC_EXE=<worktrees>/xbox/build/win64-desk/_deps/bgfx_cmake-build/cmake/bgfx/Release/shaderc.exe
- build: ninja -C build\uwp-x64 -j8
- package: scripts\uwp\make-package.ps1 -BuildDir build\uwp-x64 -OutDir build\uwp-package-nodata -Version 1.0.50.0 -NoData (test cert copied from <worktrees>/xbox)
- deploy (pending): bash scripts/uwp/xbox-deploy.sh <xbox-ip> build/uwp-package-nodata/GeneralsZH_1.0.50.0_x64.msix --launch

Commits (all shared-code, none Xbox-specific except the stub removal):
- fd468459c2f build(uwp): Drop OSDisplaySetBusyState stub now defined by SDL3Main
- dc73934f589 Merge branch 'platforms' into pf/xbox
- fc0668dcf25 build(bgfx): Mark variables only used by compiled-out logging as maybe_unused
- ec18af5c7a1 build(main): Use the core_minidumper header and init signature in SDL3Main
- f0705114ebe build(w3dview): Use WWMath::Acos and Sqrt now that the Legacy variants are gone
- 157ac5dfdaa build(w3ddevice): Route terrain particle render state through the render backend
- f624ebff65d build(audio): Match DummyAudioManager track methods to AsciiString return type
- d5a033f04c8 build(profile): Include Lib/Profile.h since upstream moved rts/profile.h
- 62d3d265ed6 build(wwlib): Drop duplicate else branch left by the upstream merge

Fixes: duplicate else() in WWLib CMake; rts/profile.h -> Lib/Profile.h (3 files); DummyAudioManager next/prevMusicTrack return AsciiString; W3DTerrainParticles DX8Wrapper calls -> WW3D::Get_Render_Backend() (border addressing via Set_Texture_Address_Mode; DX8 backend border colour not set explicitly, default is transparent black); W3DView Acos_Legacy/Sqrt_Legacy -> Acos/Sqrt; SDL3Main MiniDumper header + initMiniDumper(path,'Z'); BgfxBackend 4 C4189 locals (BasePragmas now makes 4189 an error) -> [[maybe_unused]]; render2dsentence max texture size via backend Get_Texture_Limits (DX8Wrapper::CurrentCaps unresolved on bgfx); UWPPlatformStubs OSDisplaySetBusyState duplicate removed. The last two went into one commit (the render2dsentence change is in the "Drop OSDisplaySetBusyState" commit).
Not verified: BIGFileSystem generals\ fallback on device (code unchanged by me; needs console), SimulationMathCrc value, determinism.

---
## Update 2026-10-19: deployed to the console (now <xbox-ip>, DHCP; user confirmed)
Package GeneralsZH_1.0.69.0 (code-only, with FFmpeg DLLs: the exe now imports avcodec/avformat/avutil/z/swresample even with RTS_BUILD_OPTION_FFMPEG=OFF, so pass -FfmpegBinDir <tools>/vcpkg\installed\x64-uwp\bin to make-package.ps1; without them the app fails activation with 0x8027025B). Host shaderc: <worktrees>/pf-windows\build\win64\_deps\bgfx_cmake-build\cmake\bgfx\Release\shaderc.exe (same bgfx.cmake c480227 pin).
- Launch OK (user signed in). First launch failed in init: parseCommandLineForStartup() now calls WorkingDirectory::setExecutableWorkingDirectory() unless a setter ran, which discarded SDL3Main's chdir to LocalState\data, so BIGFileSystem found 0 archives ("Data\INI\Default\GameData.ini" missing -> INIException -> "Technical Difficulties"). Fix: SDL3Main now uses rts::WorkingDirectory::setCustomWorkingDirectory(dataDir). (Shared-code note: Android/Switch/iOS SDL3Main paths use chdir() the same way and probably need the same.)
- After the fix: data loads through the merged ArchiveFile/BIGFileSystem. Root ZH BIGs come from LocalState\data; the `generals\` fallback is reached (logging build showed generals\Patch.big etc. being mounted).
- Shell map renders (ZH logo, terrain, units, particles). Screenshots via /ext/screenshot. In-game fps overlay: 27[30] in light scenes, 14-23[30] in busy explosion scenes (before the merge: 25-29). Not a clean comparison; scene content differs.
- NOT reached: the main menu buttons/frame never appear (pre-merge screenshot <tools>/xbox-deps\xbox-shot8.png shows SOLO PLAY ... EXIT GAME). Only the logo draws, over 2 minutes. Cause not found. Suspects: merged GUI transition timing (upstream #2056/#2833 TransitionGroup::update uses FramePacer::getBaseOverUpdateFpsRatio) or the shell menu layout. No window-load errors appear in the (DEBUG_LOGGING) log; the log shows only benign `Targa: Failed to open trstrtholecvr.tga` and a CRCAfter0Maps.dat assertion.
- -mathCrcCheck (via LocalState\cmdline.txt, modern mode): SimulationMathCrc = 76B53840, SimulationMathCrcDouble = 4E2B6D2C. Matches the expected values.
- Debug-logging builds on UWP die silently at the first assert after shell-map load, and stdout/stderr files lose everything after the last 4096-byte block when the process dies abnormally; use non-logging builds + printf for diagnosis.
- Extra commit: bugfix(uwp) SDL3Main working directory (above).

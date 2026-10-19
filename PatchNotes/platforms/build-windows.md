# Windows build report (pf/windows, worktree <worktrees>/pf-windows)

## Targets
| preset | configure | build | notes |
|---|---|---|---|
| vc6 (z_generals + g_generals), modern mode (RETAIL_COMPATIBLE_GAME=OFF), build/vc6-modern | OK | OK, both link | replays fail as expected (modern mode, CRC mismatch ~frame 110) |
| vc6, retail mode (ON), build/vc6-retail, GeneralsMD | OK | OK | replay check: 6/10 pass at HEAD, 10/10 with commit 84ee6454fb7 area reverted (see below) |
| win32 (MSVC DX8, both games) | OK | OK | Debug exes in build/win32/{Generals,GeneralsMD}/Debug |
| win32-generalsmd-sdl3-bgfx | OK (needs x86 FFmpeg: built via vcpkg ffmpeg:x86-windows, -DFFMPEG_ROOT=<tools>/vcpkg/installed/x86-windows) | OK | not run |
| win64 (x64, SDL3+bgfx, ZH) | OK (FFmpeg from BtbN win64 gpl-shared into .deps-cache/ffmpeg-x64, per CI) | OK | runs but stops at start: "Be 1337! Go install Generals!" (64-bit registry view lacks the Generals InstallPath key), so no replay run |

## Commands
VC6: cmd batch that calls <tools>/vc6\VC6SP6\VC98\Bin\VCVARS32.BAT then
`...\CMake\bin\cmake.exe --preset vc6 -B build\vc6-modern -DRTS_BUILD_OPTION_RETAIL_COMPATIBLE_GAME=OFF -DCMAKE_MAKE_PROGRAM=...\Ninja\ninja.exe`
and `ninja -C build\vc6-modern -j8 -k 0 z_generals g_generals` (retail: `-B build\vc6-retail -D...=ON`).
MSVC: VsDevCmd.bat -arch=x86|x64, `cmake --preset win32|win32-generalsmd-sdl3-bgfx|win64`, ninja -C build\<preset>.
Scripts live in <notes-dir>/windows\ (vc6m.bat, vc6r.bat, msvc.bat ...).
IMPORTANT VC6 trap: ninja has no header dependency tracking with VC6 (no /showIncludes), so after a header or force-included header change
delete the cmake_pch.cxx.pch/.obj files (and the objects of dependants) or do a clean rebuild.

## Replay determinism (retail variant, VC6, zhcooptest data, 10 replays from GeneralsReplays/GeneralsZH/1.04, commit 0711bf9)
- Baseline: upstream/main 2886271c5cb built with VC6 on the same machine: 10/10 pass.
- pf/windows retail at HEAD: first run 6 errors: 3 were a crash (null WW3D::Get_Render_Backend() in W3DBridge::load while headless; fixed, commit 927d164a67a), 3-4 CRC mismatches (frames 8810, 10010, 18510, 9611).
- Bisect: reverting only GeneralsMD Weapon.h, Weapon.cpp, WeaponSet.cpp, Object.cpp, FiringTracker.cpp to upstream gives 10/10 pass.
  Culprit is bobtista's ungated trunk commit 84ee6454fb7 "bugfix(weapon): Fix use-after-free when firing a shot rebuilds the source's weapon set mid-fire"
  (not the merge). Gate it with RETAIL_COMPATIBLE_CRC if the retail path matters.
- Modern mode fails every retail replay at frame ~110 (expected).
- SimulationMathCrc (-mathCrcCheck, VC6): SimulationMathCrc = B7B83850, SimulationMathCrcDouble = E19B30FF in BOTH modern and retail builds
  (identical in the two modes on VC6). win64 / MSVC values not obtained (win64 stops at the registry check; MSVC exes not run).

## Commits (git log --oneline platforms..pf/windows). Shared-code ones marked [S]
- f069fe05b59 build(wwlib): Drop duplicate non-Windows registry stub branch [S]
- d8318c53471 build(vc6): trunk headers parse on VC6 (stdint, constexpr, deleted fns, DbgHelp64 types, std:: shims in CppTypes.h/stdint_subset.h) [S]
- e77ffd72240 build(vc6): Port the WW3D2 renderer decoupling to C++98 (NSDMI->ctors, auto, lambdas, using, rvalue refs, vector::data) [S]
- 5c73920fccb build(vc6): ZH device and game engine compile on VC6 [S]
- fcee1c6285c build(vc6): Build Generals against the decoupled WW3D2 renderer (ddsfile, W3DDynamicLight, asset manager, FVF constants, GGC_GetCurrentLogicFrame) [S]
- 41ff18964f4 build(vc6): Spell out the render state enum types in the logging helpers [S]
- 927d164a67a bugfix(bridge): Skip the shader pipeline query while loading bridges headless [S]
- cherry-picked from pf/xbox: 1be813d5171 (DummyAudioManager), 1cdcf914fe8 (terrain particles via backend), 490b8c45bac (SDL3Main minidumper), 527a32a875a (maybe_unused), eb8e5a83f44 (Lib/Profile.h) [S]
- 6c73bba8375 build(bgfx): Skip the DX8Caps font size query when the backend is bgfx (from pf/android) [S]
(Acos/Sqrt rename in W3DView.cpp and the registry stub were already done in the merge/my commits.)

## Other notes
- Generals (g_generals) was already broken on trunk against the ZH-only decoupling refactor; fixed by porting the few missing pieces (listed above), it links on VC6/MSVC.
- Things I created outside the worktree: <worktrees>/pf-windows-base (git worktree at upstream 2886271c5cb with build/vc6, baseline exe), vcpkg ffmpeg:x86-windows in <tools>/vcpkg, test exes in <test-exe-dir> (generalszh-pfwin-*.exe). Ask before removing.
- The game's MiniDumper deleted a few old files in the user's CrashDumps dir during my runs (keep-newest policy).
- No co-author trailers were added.

## Update after platforms b06aa9529e0 (math CRC across MSVC builds)
pf/windows fast-forwarded to b06aa9529e0 plus one commit (build(sdl3): Link the real stack dump on Windows SDL3 builds; SDL3Main stubs and StackDump WinMain ref).
SimulationMathCrc / Double (-mathCrcCheck, cwd <test-exe-dir>), all = 76B53840 / 4E2B6D2C, equal to Linux x86_64 modern:
win32 DX8 Debug and Release, win32-generalsmd-sdl3-bgfx Debug, win64 Debug and Release. VC6 (empty gamemath stub) = B7B83850 / E19B30FF.
No registry change needed: SDL3 builds take game data from the EXE directory, not the cwd (earlier hang/"uncaught exception" was an INI throw because the exe sat in an empty folder).
Workaround used: run dirs <notes-dir>/windows\w32run and w64run hold the exe + DLLs, with hardlinks/junctions to zhcooptest content.

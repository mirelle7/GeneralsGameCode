# `platforms` branch: progress report (2026-10-19)

One branch with every target except PS2/3DS: Win32 VC6, Win32 MSVC (DX8 and bgfx), Win64, Linux, macOS, iOS,
Android, Nintendo Switch, WebAssembly and Xbox One (UWP). It is bobtista's bgfx/SDL3/OpenAL trunk plus the
the author port branches plus upstream/main (including PR #2670, deterministic math) plus the fixes found while building.
Nothing has been pushed to GitHub. Everything is local (worktree `<worktrees>/platforms`, branch `platforms`).

## 1. What is in the branch

Built by merging with an explicit merge base each time, so no original commit was rewritten or lost:

| Step | What |
|---|---|
| base | bobtista `topic/trunk` ab40ada (2026-10-14) |
| 3 merges | `fix/gles-composite-vflip`, `fix/a1r5g5b5-atlas-expand`, `fix/wide-format-hs` |
| `ci/wasm-clean` | 35 commits, 8 conflicts, resolved by hand |
| `feature/xbox-uwp` | Android, iOS, macOS, Switch, Win64/OpenAL, Xbox. 119 conflicted files / 295 hunks, resolved by area (logs `logs/log-A..E*.md`) |
| `upstream/main` 2886271c5cb | PR #2670 and 126 more commits. 125 files / 271 hunks (logs `logs/log-U*.md`) |
| `pf/*` merge-backs | fixes found by the per-target builds |
| bobtista trunk again | his 3 new commits were the user's own `fix/*` commits with `@bugfix <author> <date>` tags |
| later cherry-picks | UWP working-directory fix, Release `-autoNetwork*` options, relay bridge for wasm, VC6 and Xcode build fixes, Xbox deploy-script fix |

Decisions baked in (all on purpose, all reversible):
- **Modern mode, no retail compatibility** (user decision). `RTS_BUILD_OPTION_RETAIL_COMPATIBLE_GAME` defaults to `OFF`
  (`cmake/config-retail.cmake`), which defines all eight `RETAIL_COMPATIBLE_*` guards as 0 and turns on upstream's deterministic
  WWMath (#2670). Use `-DRTS_BUILD_OPTION_RETAIL_COMPATIBLE_GAME=DEFAULT` (or `ON`) for upstream behaviour.
- **Upstream's #2670 wins over trunk's own earlier math attempt.** Trunk's competing functions (`Sqrt_Legacy`, `Acos_Legacy`, `Sqr` ...)
  are gone. Trunk's hand-written divide-by-zero guards were replaced by upstream's `WWMath::Div_Safe`, whose fallback is 0
  where trunk used 1 or 999999 in a few places. If gameplay differs from trunk, look there first.
- **Render backend:** upstream #3453 replaced the render-backend object with a static `Renderer` class. We kept our
  polymorphic backend (the bgfx code depends on it) with a thin `Renderer` facade (`WW3D2/Backend/RenderBackend.cpp`).
  When bobtista rebases onto upstream's version, drop the facade and follow his.
- **Save format:** ScriptEngine version 6 = upstream's letterbox flag, 7 = our object counts (see also pf/bob below).
- **gamemath:** our forced `GM_ENABLE_INTRINSICS OFF` was dropped (upstream default). The wasm/Android/Linux/MSVC CRCs still match.
- Android stays in whatever orientation SDL picks (portrait on the phone). The user prefers that, so the agent's force-landscape
  commit (`pf/android` 6b9530dc8cc) was deliberately not merged.

## 2. The headline result: cross-platform determinism works

`-mathCrcCheck` (modern mode) prints the same values on every platform that could run it:

| Platform | SimulationMathCrc / Double |
|---|---|
| MSVC x86 (DX8 Debug+Release, bgfx), MSVC x64 (Debug+Release) | 76B53840 / 4E2B6D2C |
| Linux x86_64 (gcc 13, WSL) | 76B53840 / 4E2B6D2C |
| Android arm64 (Android phone) | 76B53840 / 4E2B6D2C |
| WebAssembly (headless Chrome) | 76B53840 / 4E2B6D2C |
| Xbox One S (x64 UWP) | 76B53840 / 4E2B6D2C |
| VC6 | B7B83850 / E19B30FF (expected: VC6 only has the empty gamemath stub, so VC6 builds cannot play cross-platform) |
| macOS arm64, Switch | not measured yet |

This is only the math check. An actual two-platform multiplayer game has not been completed (section 4).

## 3. Status per target

| Target | Builds | Runs | Notes |
|---|---|---|---|
| Win32 VC6 | yes (ZH + Generals) | yes | retail replays pass 10/10 in a retail-ON VC6 build once bobtista's weapon-set commit 84ee6454fb7 is reverted (see findings) |
| Win32 MSVC DX8 / bgfx | yes | only `-mathCrcCheck` | exes never played interactively by an agent |
| Win64 MSVC | yes | `-mathCrcCheck` | data is read from the EXE's folder, not the cwd |
| Linux | yes (build only `z_generals`) | headless 150 s, no crash | Vulkan renderer never run (no windowed run) |
| Android | yes, APK via `scripts/android/build-apk.sh` | main menu + shell map, about 30 fps, on the phone | args via intent extra; the install replaced the old Box64 app (same package name) |
| WebAssembly | yes | main menu + shell map in Chrome | needs the ASYNCIFY build; relay bridge added |
| Switch | yes, `.nro` after `gcspatch.py` | Eden boots it and stays up | nobody has seen how far it gets |
| Xbox One | yes, msix packaged | boots, data loads, shell map renders, fps 27 light / 14-23 busy | **main menu buttons never appear** (regression vs the pre-merge build; suspect GUI transition timing) |
| macOS | yes (Release) | runs from `~/zh-game`, reaches the menu | the cross-platform match was not completed |
| iOS | not finished | no | Xcode generator work and PCH fix were in progress; device install needs the Xcode GUI |

## 4. Cross-platform match (wasm host <-> macOS join): where it stands

Done: relay bridge (`scripts/build/web/relay.py --bridge-ip <PC LAN IP>` binds real UDP on 8086/8088 and bridges the browser tab to the
LAN), `?ip=a.b.c.d` full-IP identity for the tab, and a real bug fix (bobtista's "bind INADDR_ANY on non-Windows" change had also hit wasm,
so the tab's `?ip=` identity was never sent). Windows firewall is disabled so inbound UDP is fine. Mac `~/zh-game` .big files are
identical to `<test-exe-dir>` (sha256). The wasm tab auto-hosted into the Game Options screen from the new Release
`-autoNetwork*` options (screenshot seen: host `WasmHost`, slot 2 Open). **The Mac join and the match itself were not completed**, so there
is no desync verdict yet.

## 5. Findings worth knowing

1. **gamemath vs SDL3 libm symbol clash** (`gm_` prefix fix in `cmake/gamemath.cmake`): with SDL3 linked statically, the linker picked SDL's
   `__kernel_*` instead of GameMath's, which gave an invalid wasm module and is a determinism hazard on any static-SDL target.
2. **Merge-back trap:** every target branch deleted a different one of two identical `registry_*stub*.cpp` files, so merging all of
   them left CMake naming a deleted file. Fixed (`registry_unix_stub.cpp` exists).
3. **bobtista trunk commit 84ee6454fb7** (weapon-set use-after-free fix) is not gated by `RETAIL_COMPATIBLE_CRC` and breaks 4 of 10 retail
   replays. Irrelevant in modern mode, but any retail-compatible build and upstreaming need it gated. Worth telling him.
4. **VC6:** trunk did not build on VC6 at all (stdint, `= delete`, `using`, DbgHelp64 types ...). Fixed in the branch. VC6 header
   dependency tracking under ninja is unreliable: delete the stale pch/obj after header changes.
5. **Non-Windows `-replay ... -jobs N`** reports success in 0 s because `WorkerProcess` is a stub. Do not trust it.
6. **UWP startup:** the new `CommandLine.cpp` startup code moved the working directory back to the read-only package folder. SDL3Main now
   marks the data directory as the working directory (cherry-picked).
7. **Xbox** moved to <xbox-ip> after a DHCP change (user confirmed). The deploy script launched stale package versions: fixed.
8. The Steam Zero Hour install on this PC has zeroed regions in `MapsZH/TerrainZH/MusicZH/TexturesZH.big`. Use `<test-exe-dir>`.
9. Release `-autoNetwork*` options (user request): `-autoNetworkMode direct`, `-autoNetworkHost <players>` or `-autoNetworkJoin <ip>`,
   `-autoNetworkLocalAddress`, `-autoNetworkName`, `-autoNetworkMap`, `-autoNetworkTimeout` (`ENABLE_NETWORK_AUTOSTART`, on by default).
   Compiled and checked on Windows (VC6 loop-variable and flush fixes in follow-up commits). Not yet seen running on the Mac.

## 6. Not merged yet / open decisions

- **`pf/bob` (tip 0f85849158e, 104 commits ahead of `platforms` counting bobtista history):** bobtista's 48 topic branches classified
  (table in `bobtista-branches.md`). Merged there: 16 (path separators, map transfer/cache, LAN map resolution, non-Windows save/load,
  pathfinder, scripted camera, water tracks, script-engine saves, CLI save options, MinGW). Skipped on purpose: 8 `route-*`/
  `widen-draw-arguments` (conflict with our polymorphic backend), 6 `d3dx-*` (conflict with trunk), `debug-auto-lan-startup`
  (replaces NetworkAutoStart), 5 `test/*`. Windows MSVC DX8 Debug builds with it. **Decisions for you:** merge it into `platforms`
  (recommended after the Mac match test); ScriptEngine save version is 8 there vs 7 here; are the skips OK; retest BIG archive listings
  on non-Windows (they now use native separators).
- `pf/android` 6b9530dc8cc (force landscape): intentionally unmerged.
- Per-target branches `pf/windows|switch|android|wasm|linux|apple|xbox|crossmatch` still exist locally; their useful commits are in
  `platforms`. They can be deleted once you are happy.

## 7. Left behind (nothing was deleted without you)

- PC: `<worktrees>/pf-*` worktrees (about ten), `<worktrees>/pf-windows-base`, `<tools>/emsdk-6`, WSL `~/emsdk`, `~/ffmpeg-wasm`, `~/pf-linux-sysroot`,
  `~/build/pf-linux`, extra test exes in `<test-exe-dir>` (`generalszh-pfwin-*.exe`), x86 FFmpeg in `<tools>/vcpkg`, run folders
  `<notes-dir>/windows\w32run` / `w64run` (contain junctions into zhcooptest: remove with `rmdir`, never recursively),
  the Eden junction `eden\user\sdmc\generalszh` and a copied `.nro` in `eden\user\sdmc\switch\`.
- Mac: `<repo>`, `<worktrees>/pf-apple` (has an uncommitted temporary "CROSSMATCH TEST ONLY" CMake hack unless reverted: `git checkout --
  Core/GameEngine/CMakeLists.txt GeneralsMD/Code/GameEngine/CMakeLists.txt`), `<worktrees>/pf-crossmatch`, `<worktrees>/trunk`, `~/generalszh-release-orig`.
  Mac disk was 21 GB free.
- Windows CrashDumps folder lost a few old dumps (the game's MiniDumper keeps only the newest).
- Session processes (relay, file server, test Chrome, Mac test game) were stopped at the end of this session.

## 8. How to test it yourself

Each target's exact commands are in the per-target notes next to this file (`build-<target>.md`). Short versions:

**Common:** game data = `<test-exe-dir>` (clean Zero Hour). Check determinism with `generalszh -mathCrcCheck` (the SDL3 Windows builds
read data from the folder that contains the EXE; run from the data folder or put the exe there). Expect `76B53840` / `4E2B6D2C`.

- **Windows (MSVC/VC6/Win64):** worktree `<worktrees>/pf-windows` (`pf/windows`; `platforms` is newer: `git merge --ff-only platforms` there first).
  Build scripts in `<notes-dir>/windows\`. Always use the VS BuildTools cmake/ninja by full path and source
  `VCVARS32.BAT` for VC6. Presets: `vc6`, `win32`, `win32-generalsmd-sdl3-bgfx`, `win64`. Build target `z_generals` (and `g_generals`).
- **Linux:** `wsl.exe -d Ubuntu -- bash -lc ...`, preset `linux-generalsmd-sdl3-bgfx`, build only `z_generals`; for a normal install:
  `sudo apt install pkg-config libfreetype-dev libfontconfig-dev libopenal-dev libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev libvulkan-dev libxtst-dev`.
- **Android (phone `<device-serial>`):** `ANDROID_HOME=%LOCALAPPDATA%\Android\Sdk bash scripts/android/build-apk.sh <build-dir> <out.apk>`, then
  `adb -s <device-serial> install -r ggc.apk`, `adb -s <device-serial> shell appops set com.generals.launcher MANAGE_EXTERNAL_STORAGE allow`,
  `MSYS_NO_PATHCONV=1 adb -s <device-serial> shell am start -n com.generals.launcher/.GameActivity --es args "-mathCrcCheck"` (screen must be on;
  result in `/sdcard/Games/Generals/SimulationMathCrc.txt`). Game data in `/sdcard/Games/Generals`.
- **WebAssembly:** in `<worktrees>/pf-wasm`: put `serve.ini` with `STEAM_ZH_DIR=<test-exe-dir>`, run `python serve.py`, open `http://localhost:8000/`
  and click "Use the local server". Use a real GPU Chrome with background throttling disabled (see `build-wasm.md`).
- **Switch:** run `gcspatch.py` on the ELF, then `nacptool` + `elf2nro --nacp` (see `build-switch.md`; the packaged `generalszh_pf.nro` is in the notes folder),
  load it in Eden (`<eden-dir>\eden.exe -g <nro>`). The data folder is the junction `eden\user\sdmc\generalszh`. Look at the Eden window to see how far it gets.
- **Xbox (<xbox-ip>, Dev Mode, a user must be signed in):** `bash scripts/uwp/xbox-deploy.sh <xbox-ip> <msix> --launch` (package
  `<worktrees>/pf-xbox\build\uwp-package-nodata\GeneralsZH_1.0.69.0_x64.msix`; needs the FFmpeg DLLs, see `build-xbox.md`). Logs in
  `LocalState\GGC\GeneralsZH\ggc_log.txt`, screenshot `GET /ext/screenshot?download=true`. Try `-noShellAnim` in `LocalState\cmdline.txt` to see if the main
  menu appears (that tests the transition-timing theory).
- **macOS:** `ssh <mac-user>@<mac-ip>`; `<worktrees>/pf-crossmatch` has the newest Release build
  (`build/macos-generalsmd-sdl3-bgfx/GeneralsMD/Release/generalszh`); run from `~/zh-game` with `-win -xres 1024 -yres 768`.
  Configure: `cmake --preset macos-generalsmd-sdl3-bgfx -DCMAKE_PREFIX_PATH=$HOME/code/GeneralsGameCode-2/build/ffmpeg-prebuilt/ffmpeg-macos`.
- **wasm <-> macOS match (manual):** on the PC, in `<worktrees>/pf-wasm`: `python serve.py` and
  `python -u scripts/build/web/relay.py --bridge-ip <PC LAN IP>` (find it with `ipconfig`; it was <pc-ip>), open
  `http://localhost:8000/?ip=<PC LAN IP>`, go to the LAN lobby and CREATE GAME. On the Mac:
  `cd ~/zh-game && ./generalszh -win -autoNetworkMode direct -autoNetworkJoin <PC LAN IP> -autoNetworkLocalAddress <mac-ip> -autoNetworkName MacJoiner`
  (or join through the LAN lobby by hand), start the match from the host, let it run a few minutes and watch for desync/mismatch messages
  on both sides. Then try the reverse. Please report what you see (screenshots/log lines).
- **Retail replay check (VC6 only):** build `vc6` with `-DRTS_BUILD_OPTION_RETAIL_COMPATIBLE_GAME=ON`; modern-mode builds are expected to fail retail replays.

## 9. Files next to this report

`build-<target>.md` (exact commands and results per target), `crossmatch.md`, `bobtista-branches.md`, `logs/` (every merge conflict decision
by area), and the briefs that were given to the agents. The same information is kept in the assistant's memory under `upstream-archive-project`
and `test-devices`.

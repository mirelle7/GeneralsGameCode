# build-apple (macOS + iOS)

## macOS: configure OK, build OK (z_generals, Release, modern mode), mathCrc OK
Tip: pf/apple 15f757ed544 (= platforms 8a06e5129e6 + one fix). Worktree on Mac: <worktrees>/pf-apple. No compile errors beyond the one CMake fix below.

Commands that worked (on the Mac, via ssh, PATH=~/bin:$PATH):
  cd <worktrees>/pf-apple
  cmake --preset macos-generalsmd-sdl3-bgfx -DCMAKE_PREFIX_PATH=$HOME/code/GeneralsGameCode-2/build/ffmpeg-prebuilt/ffmpeg-macos -DRTS_BUILD_OPTION_RETAIL_COMPATIBLE_GAME=OFF
  cmake --build build/macos-generalsmd-sdl3-bgfx --config Release --target z_generals -j8
(Preset is Ninja Multi-Config: without --config Release it builds Debug.) Full build ~12 min. Binary:
  <worktrees>/pf-apple/build/macos-generalsmd-sdl3-bgfx/GeneralsMD/Release/generalszh (arm64, 17.8 MB)

Commit (shared-code, any non-Windows target needs it): fd1b8d9627f build(wwlib): Drop duplicate non-Windows registry stub branch
(two else() branches in WWLib/CMakeLists.txt broke configure; kept trunk's registry_unix_stub.cpp, port's registry_stub_nonwindows.cpp no longer listed; no link problem seen.)

Math CRC (modern mode, macOS arm64): SimulationMathCrc = 76B53840, SimulationMathCrcDouble = 4E2B6D2C (matches all other platforms).
Trap: a single-instance flock in $TMPDIR (/tmp default) makes a 2nd copy exit 0 silently while another generalszh runs. For the CRC check I ran with TMPDIR=$HOME/tmp-mathcrc/ (must exist).

## For the crossmatch test
- Data folder: ~/zh-game (sha256 of all 21 .big files is IDENTICAL to <test-exe-dir>, verified; so no ~/zh-game-clean was needed/created).
- Binary: copy of the build at ~/zh-game/generalszh-platforms (the user's old ~/zh-game/generalszh is untouched).
- Run command (cwd must be the data folder):
    cd ~/zh-game && ./generalszh-platforms -win -xres 1024 -yres 768
  From ssh it does not show on screen; launch in the GUI session (e.g. `open`-launched .command, or whatever started the existing pid below).
- NOTE: at 10:01 a generalszh (pid 62251, parent launchd, cwd ~/zh-game, from the <worktrees>/pf-apple build dir, args `-win -xres 1024 -yres 768`) was already running that I did NOT start; I left it alone. It holds the instance lock, so kill it before launching another copy (or use a different TMPDIR).
- Not verified by me: reaching the main menu (screencapture over ssh fails: "could not create image from display"; the running process has all .big files open, so data loads).

## iOS
(see below, updated if attempted)

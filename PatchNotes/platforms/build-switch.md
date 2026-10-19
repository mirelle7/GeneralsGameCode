# build-switch (pf/switch, worktree <worktrees>/pf-switch)

Configure OK, build OK (z_generals links, 417 MB ELF), .nro packaged (32 MB). Boots in Eden to process start; game output not observable.

## Commands that worked (Git Bash)
export PATH="/c/Users/<you>/openinterpreter/Scripts:$PATH"; export DEVKITPRO=<tools>/devkitPro DEVKITA64=<tools>/devkitPro/devkitA64
P=<tools>/devkitPro/portlibs/switch
"/c/Program Files/CMake/bin/cmake.exe" --preset switch-generalsmd-sdl3-bgfx -DRTS_BUILD_OPTION_RETAIL_COMPATIBLE_GAME=OFF \
  -DCMAKE_MAKE_PROGRAM=C:/Users/<you>/openinterpreter/Scripts/ninja.exe -DFFMPEG_INCLUDE_DIR=$P/include \
  -DFFMPEG_AVCODEC_LIBRARY=$P/lib/libavcodec.a -DFFMPEG_AVFORMAT_LIBRARY=$P/lib/libavformat.a \
  -DFFMPEG_AVUTIL_LIBRARY=$P/lib/libavutil.a -DFFMPEG_SWSCALE_LIBRARY=$P/lib/libswscale.a
cmake --build build/switch-generalsmd-sdl3-bgfx --config Release --target z_generals -- -j8   (generator is Ninja Multi-Config: pass --config Release)
Output ELF: build/switch-generalsmd-sdl3-bgfx/GeneralsMD/Release/generalszh
Packaging: gcspatch.py (this folder; patches libgcc chkfeat/gcspopm/mrs gcspr_el0 in .text: 5/3/2 hits) then
  nacptool --create "Generals Zero Hour" GGC 1.0 gz.nacp ; elf2nro patched.elf out.nro --nacp=gz.nacp
  Result: <notes-dir>/generalszh_pf.nro
  (scripts/switch_package.py in the repo is the old embedded-romfs flow, not the working loose-files flow.)

## Merge-loss checklist
- SDL3 switch patch cloning: present in cmake/sdl3.cmake, "already patched" path worked.
- bgfx eglplatform NX patch: present and applied. GIT_SUBMODULES_RECURSE TRUE (trunk) worked fine on Switch.
- Host shaderc: automatic ExternalProject (host-shaderc) built with the default host compiler, no -DGGC_SHADERC_EXE needed.

## Commits (git log --oneline platforms..pf/switch); [S] = shared-code
d62f4ba2b28 build(gamemath): Build GameMath against Nintendo Switch newlib (patches fetched gmath.h __ISO_C_VISIBLE branch + signgam shim, Switch only)
c93b36e4a24 [S] build(wwlib): Drop duplicate non-Windows RegistryClass stub (cherry-pick from pf/wasm)
4a21b5199a9 [S] build(utility): WWVegas include dir for shim windows.h PCH (wasm)
90c1de2dd86 [S] build(platforms): include/link fallout of upstream merge (wasm)
fe43be7b1c4 [S] SDL3Main core_minidumper header/signature (wasm)
165ca2de743 [S] W3DView Acos/Sqrt (wasm)
e6d3f1c53c0 [S] W3DTerrainParticles via backend (wasm)
db865152b9b [S] DummyAudioManager return types (wasm)
adab8c0d188 [S] BgfxBackend maybe_unused (wasm)
ee81573cf50 [S] Lib/Debug.cpp time_compat.h (wasm)
38096f32e98 [S] AndroidWin32Stubs trim (wasm/android)
5fcb042d3d5 build(network): skip getifaddrs on Switch (Windows branches unchanged; #elif defined(_WIN32) keeps ws2ipdef.h)
79ab65c1aa2 [S-ish] build(cmake): treat NintendoSwitch like UNIX/ANDROID for win32_shims windows.h (WWVegas + Utility CMake)
a4d6b8ac2d7 [S] ww3d2: no DX8 caps query for font texture limit on bgfx (wasm)
b64c35abb73 build(client): no flock on Switch (ClientInstance.cpp)

## Eden test (<eden-dir>\eden.exe)
Made a directory junction <eden-dir>\user\sdmc\generalszh -> <test-exe-dir> (game data; no data existed on any emulator sdmc) and copied the nro to user\sdmc\switch\generalszh_pf.nro.
`eden.exe -g <nro>`: Eden loads it (reads NACP title "Generals Zero Hour", "Booting game" logged, window titled, process Responding for 100+ s, no crash dialog seen).
Eden's log stops at ~0.9 s after boot even with log_filter default=false/*:Debug, and the desktop screenshot (session has no visible Eden window), so I cannot say how far the game gets (menu / render / hang). No crash is logged. Needs someone to eyeball the window. Eden qt-config.ini restored.
Not done: no -mathCrcCheck value (no way to read output), no RETAIL_COMPATIBLE_CRC=0 comparison beyond building modern mode.

# Log: D_gameengine

All 27 files are resolved (64 conflict hunks). No conflict markers remain, and `#if`/`#endif` counts balance in every file.
Line endings stay CRLF (checked: 0 bare LF in every file).

"ours" = HEAD (trunk + wasm-clean); "theirs" = origin/feature/xbox-uwp.

## Trivial (summarised)
These hunks were the same change on both sides, or differed only in spelling (`#ifdef _WIN32` vs `#if defined(_WIN32)`, same rename under a different name, same comment), so I took ours:
- GameEngine.cpp hunks 1-3 (CComModule `_WIN32` guards)
- DataChunk.cpp hunk 1, GameText.cpp hunk 1
- InGamePopupMessage.cpp x3 (`s_pause` vs `s_popupPaused`; kept trunk's `s_pause`)
- ThingTemplate.cpp (comment only; the cast line was already identical)
- GadgetListBox.cpp (comment only; the cast was already identical)
- WindowVideoManager.h (trunk hashes the full uintptr_t)
- FramePacer.cpp (both have the same Emscripten uncapped-pacer fix; kept the wasm-clean version)

## Non-trivial hunks

| file | hunk | resolution | rule | NEEDS-REVIEW? |
|---|---|---|---|---|
| Core/.../GameClient/GameWindow.h | WindowMsgData include | combined: kept trunk's `<cstdint>` but put it behind `#if !defined(_MSC_VER) \|\| _MSC_VER >= 1300` (same guard as GameCommon.h). On VC6, uintptr_t comes from the PCH/stdint_adapter, as theirs relied on. Both comments kept | 1,4 | no |
| Core/.../GameClient/Smudge.h | includes + SmudgeIdToPtrMap typedef | ours. Trunk's `Utility/hash_map_adapter.h` already covers theirs' STLport/unordered_map split. I also put the auto-merged typedef back to trunk's plain `std::hash_map` (theirs' `#else unordered_map` branch would break VC6 without STLport). File now matches trunk | 2 | no |
| Core/.../GameLogic/Damage.h | getDeathTypeFlag/set/clear (4 hunks) | ours: trunk's `getDeathTypeFlagBit()` keeps bit 31 for DEATH_NORMAL on LP64, which is the same fix as theirs' `1U`. Retail-identical on x86 | 2,3 | no |
| Core/.../Common/CommandLine.cpp | parseCommandLine | ours. Trunk already reads `__argc/__argv` on every platform, so the Xbox UWP fix (don't use GetCommandLineA) is covered. UWP still depends on SDL3Main filling `__argv` from LocalState\cmdline.txt **before** `parseCommandLineForStartup()` runs | 2 | yes (see builder notes) |
| Core/.../System/LocalFile.cpp | writeChar x2 | ours (trunk returns the character, not its address; theirs only widened the old bug) | 2 | no |
| Core/.../System/UnicodeString.cpp | format_va | ours: trunk's `formatStringW()`/`translateWideFormat` covers the %s->%ls and %hs handling, including musl | 2 | no |
| Core/.../Common/WorkerProcess.cpp | trailing `#endif` + theirs' file-wide `#if defined(_WIN32)` (auto-merged at top) | ours/trunk: removed theirs' outer guard, so trunk's in-file non-Win stub is live. File now matches trunk | 2 | yes (AndroidWin32Stubs, see notes) |
| Core/.../GUI/IMEManager.cpp | top guard + trailing `#endif` | ours/trunk (in-file non-Win stubs for TheIMEManager/CreateIMEManagerInterface). File now matches trunk | 2 | yes (AndroidWin32Stubs, see notes) |
| Core/.../GameClient/GameText.cpp | CSF read loop + invert | ours: trunk's non-Win UTF-16 read and 16-bit invert are the same fix. Windows path untouched. Theirs' extra `kTBufferMax` bound dropped | 2,3 | no |
| Core/.../GameClient/LanguageFilter.cpp | readWord | ours (whole file = HEAD). Theirs changed the **Windows** path too (2-byte reads + max-length bound); trunk keeps Windows retail and has an equivalent non-Win branch | 2,3 | no |
| Core/.../GameClient/MapUtil.cpp | preview image creation | combined: `#if defined(_WIN32)` = trunk/retail (no image if copy fails); `#else` = theirs (fall back to the archived tgaName). Android log lines (auto-merged) kept | 1,3 | no |
| GeneralsMD/.../GameEngine.cpp | init: isMusicAlreadyLoaded quit | ours: trunk removed the quit check entirely, so theirs' Switch exemption is moot | 2 | no |
| GameEngine.cpp | init: shell map cache miss | ours: trunk only disables when the file really doesn't exist, which covers theirs' Android case. Theirs' `GGC_RENDER_BACKEND_BGFX` A/B diagnostic dropped | 2 | no |
| GameEngine.cpp | init: m_afterIntro block (auto-merged, theirs) | **ported**: trunk deleted `m_afterIntro` (new Intro state machine, Core/.../Intro.cpp). Kept `m_playIntro = FALSE` for `__ANDROID__` only and dropped the `m_afterIntro` line (would not compile). Dropped the `GGC_RENDER_BACKEND_BGFX` part (theirs' own comment calls it an A/B diagnostic; it would change trunk's maintained bgfx builds on every OS) | 1,3,7 | yes |
| GameEngine.cpp | init: afterIntro + resetSubsystems trace | ours + theirs' Switch trace before resetSubsystems() | 1 | no |
| GameEngine.cpp | update(): logic block (3 hunks) | trunk structure (`canUpdateGameLogic(IgnoreFrozenTime)`, logic then `step()` when not frozen) with theirs' Switch traces moved to the matching spots. "after Net" trace no longer prints canLogic (calling canUpdateGameLogic twice would have side effects) | 1 | no |
| GameEngine.cpp | execute(): deferred save load | ours (trunk removed it) + theirs' Switch "reached while" trace | 1 | no |
| GeneralsMD/.../GameClient.cpp | Intro done / legal page | ours: trunk removed the blocking LegalPage loop (Intro.cpp), so theirs' Android/Switch skip is moot. Also fixed the auto-merged Switch trace in update(), which read the removed `m_afterIntro` | 2 | no |
| GeneralsMD/.../GameLOD.cpp | shell-map low-end check | combined: `!__ANDROID__ && !__EMSCRIPTEN__ && !__SWITCH__`, all comments kept | 1 | no |
| GeneralsMD/.../GameMain.cpp | Emscripten loop (3 hunks) | ours (wasm-clean's `ggcEmscriptenLoopIteration`, plus trunk's `getExitCode()`). Theirs' duplicate loop dropped as instructed | 2 | no |
| GeneralsMD/.../GlobalData.cpp | includes | combined (trunk's `_UNIX` includes + `WW3D2/` paths + theirs' Android log include) | 1 | no |
| GlobalData.cpp | BuildUserDataPathFromRegistry (auto-merged) | **ported**: trunk's `_UNIX` branch (Android/iOS/Switch/wasm/Linux/mac) now honours `GENERALS_USER_DIR` first; theirs' launchers set it. Theirs' env branch inside the Windows section narrowed from `!_WIN32 \|\| UWP` to `GGC_PLATFORM_UWP` (the non-Win part is unreachable there) | 1 | yes (path change on mobile/wasm: now `<GENERALS_USER_DIR>/<leaf>/` with forward slashes) |
| GeneralsMD/.../DataChunk.cpp | readUnicodeString | ours (same 2-byte read fix) | 2 | no |
| GeneralsMD/.../StackDump.cpp | 13 hunks | ours: trunk's StackWalk64/CONTEXT rewrite already supports x64. Theirs' STACKDUMP_* macros, its file-wide `_WIN32` guard (CMake only builds this file on WIN32) and its duplicate x64 register dump (which had landed inside trunk's x86 branch) removed. One change kept: `StackDumpFromContext(DWORD_PTR...)` so it matches the merged StackDump.h declaration (identical type on x86) | 2 | no |
| GeneralsMD/.../registry.cpp | getStringFromRegistry | combined: trunk's NUL-terminate fix + theirs' `DWORD` types (same as `unsigned long` on Windows) | 1 | no |
| GeneralsMD/.../LanLobbyMenu.cpp | getUserName (3 hunks) | ours (whole file = HEAD): wasm-clean's `suffixNameForWebInstance` is the newer version of theirs' same feature. Theirs' uintptr_t cast was already in HEAD | 2 | no |
| GeneralsMD/.../MainMenu.cpp | MainMenuUpdate top | combined: theirs' Switch trace + trunk's RTS_DEBUG direct-connect block | 1 | no |
| MainMenu.cpp | auto-reveal menu (auto-merged) | narrowed `__ANDROID__ \|\| GGC_RENDER_BACKEND_BGFX` to `__ANDROID__` (bgfx part is an A/B diagnostic, and it changes trunk's bgfx behaviour) | 3 | yes |
| GeneralsMD/.../GameLogic.cpp | localSlot | combined: trunk's `_WIN32`/non-Win localSlot + theirs' `GGC_TRACE` (a no-op off Switch) | 1 | no |
| GameLogic.cpp | load barrier / ready wait / fade (3 hunks) | ours (wasm-clean emscripten_sleep + trunk FramePacer) | 2 | no |
| GameLogic.cpp | ship-underwater diagnostic in update() (auto-merged, **unguarded in theirs**) | wrapped in `#if defined(__ANDROID__)`. It wrote `<tools>/GeneralsGameCode-2/scratch/ship_win32.log` on Windows. Also removed theirs' duplicate `#include <emscripten.h>` | 3 | yes (probably delete entirely) |

## Builder notes
- **AndroidWin32Stubs.cpp** (GeneralsMD/Code/GameEngine/Source/Common, from theirs, built for `if(NOT WIN32)` in GeneralsMD/Code/GameEngine/CMakeLists.txt) defines symbols that trunk now defines too:
  - `WorkerProcess::*` and `TheIMEManager`/`CreateIMEManagerInterface`: also in trunk's in-file stubs in WorkerProcess.cpp/IMEManager.cpp, which I kept.
  - `g_LastErrorDump` and `OSDisplaySetBusyState`: also in trunk's SDL3Main.cpp.
  - Expect duplicate-symbol link errors on Linux/mac/Android/iOS/Switch/wasm. Trim that file down to what trunk lacks (likely the MiniDumper stubs and Android's `pthread_cancel`), or drop it from CMake. It is in no group; I did not touch it.
- **UWP command line:** CommandLine.cpp reads `__argc/__argv` (trunk). The SDL3Main owner must keep theirs' UWP code that adds the LocalState\cmdline.txt arguments to `__argv`, and it must run before `CommandLine::parseCommandLineForStartup()`. Trunk notes that startup parsing can run from static constructors, before main.
- **iOS `system()` guard** in GameEngine.cpp `updateTGAtoDDS()` is preserved (auto-merged, `TARGET_OS_IPHONE`).
- **Emscripten:** GameMain/GameLogic/FramePacer/LanLobbyMenu use wasm-clean's code only; theirs' duplicates dropped.
- **VC6 / GameWindow.h:** `<cstdint>` is now guarded. On VC6, `uintptr_t` must come from the PCH (`stdint_adapter.h` -> `stdint_subset.h`), the same as trunk's other uintptr_t uses.
- **StackDump:**
  - The cpp signature is `DWORD_PTR` to match the merged header.
  - Trunk's x64 call still casts to `(DWORD)` (it truncates, as trunk intended). It compiles.
- **Removed field:** `m_afterIntro` no longer exists in trunk. The two theirs references in my files are removed. Other groups should grep for it in their files (`Generals/` and GameEngineDevice).
- **Not in my group, used by my files:** GameEngine.cpp (Android/Switch) needs `GGC_GetDeviceResolution_SDL3` (defined in GameEngineDevice/SDL3GameEngine.cpp). GlobalData.cpp needs `OptionPreferences::getRenderResolutionScale/getGgcRenderSkip` and `m_renderResolutionScale/m_ggcRenderSkip` in GlobalData.h. All are present in the merged tree.

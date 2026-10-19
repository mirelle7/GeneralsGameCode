# log-U5_engine_libs (upstream merge)

| file | hunk | resolution | rule | NEEDS-REVIEW |
|---|---|---|---|---|
| Core/GameEngine/Include/Common/Debug.h | whole file | upstream (file reduced to GameDebugInit/ReleaseCrash; core moved to Lib/Debug.h) | 1 | no |
| Core/Libraries/Source/Lib/Include/Lib/Debug.h (new home, not conflicted) | HEAD-only additions | ported: DebugReopenLogInDirectory, DebugFlushLog, new DebugSetLogFlushEachLine(bool), `(1u << l)` DEBUG_LOG_LEVEL fix | 1 | no |
| Core/Libraries/Source/Lib/Debug.cpp (new home, not conflicted) | HEAD-only additions | ported: buffered log (no per-line fflush), flushLogFile + flush in DebugCrash stack path, buildLogFileNames (stores prefix/suffix), DebugReopenLogInDirectory, `dirbuf[0]=0` init for stub GetModuleFileName | 1 | no |
| Core/GameEngine/Source/Common/System/Debug.cpp | all 6 conflicts | took upstream file; re-added: Android logcat in ReleaseCrash/ReleaseCrashLocalized, GgcRuntimeFlags include, `DebugSetLogFlushEachLine(GgcFlags::Enabled(GgcFlag_LogFlush))` in GameDebugInit, DebugFlushLog() before fatal exit | 1,2 | yes (GgcFlag_LogFlush now wired through GameDebugInit, since Lib cannot see WWLib) |
| same | SimpleProfiler (+ __int64 -> long long fix) | dropped: upstream deleted it, no users anywhere | 1 | no |
| same | DebugReopenLogInDirectory/flush in old DebugInit | moved to Lib/Debug.cpp (above) | 1 | no |
| Core/GameEngine/Source/Common/INI/INI.cpp | scanType | combined: our floating-point strtod branch (NDK libc++) + upstream's tokenOffset and out_of_range strto* handling in the integral branch | 1,2 | no |
| Core/GameEngine/Source/Common/System/BIGFileSystem.cpp | Win32BIGFileSystem -> BIGFileSystem | already resolved by another agent at 21:04 (BIGLoadOrderLess priority sort present); not touched by me | - | check |
| Core/GameEngine/Source/Common/System/GameMemoryNull.cpp | include guard | combined: `#ifdef DISABLE_GAMEMEMORY` (upstream) around our `_WIN32 malloc.h / else cstdlib` | 1,2 | no |
| Core/GameEngine/Source/GameClient/MapUtil.cpp | preview copy | upstream (copyFromBigToDir returns Bool, no try/catch) + our Android logcat lines kept | 1,2 | no |
| Core/Libraries/Source/MiniDumper/MiniDumper.cpp | head | upstream's include list (no PreRTS.h) + our `#if defined(_WIN32)` wrapper (closing `#endif // _WIN32` already at EOF) | 1,2 | no |
| Core/Libraries/Source/debug/debug_debug.cpp | includes | combined: our `_ReturnAddress` intrin block + upstream's stdio_adapter/stringex (WWCommon.h dropped, as upstream) | 1,2,4 | no |
| Core/Libraries/Source/profile/profile_highlevel.cpp | includes | combined: `<cstring>` + upstream's stdio_adapter/stringex (WWCommon.h dropped) | 1,2 | no |
| GeneralsMD/.../BuildAssistant.cpp | radius | upstream (WWMath::Pow); our WWMath::Sqr was trunk's earlier deterministic attempt | upstream wins | no |
| GeneralsMD/.../InGameUI.cpp | recreateControlBar | upstream destroyControlBar() (fixes the same stacking bug via m_controlBarWindow); our ControlBarParent lookup dropped | 2 | no |

## Builder watch-outs
- Core/GameEngine/CMakeLists.txt still has conflict markers near line 1203-1210 (not my group) and an `if(WIN32) Source/Common/System/MiniDumper.cpp` entry: that file no longer exists; MiniDumper is the `core_minidumper` static lib (Core/Libraries/Source/MiniDumper), already linked at ~line 1257.
- GeneralsMD/Code/GameEngine/Source/Common/AndroidWin32Stubs.cpp and GeneralsMD/Code/Main/SDL3Main.cpp still `#include "Common/MiniDumper.h"` (now `MiniDumper/MiniDumper.h`, whose API is `initMiniDumper(const char*, Char product)`; the stubs use the old `initMiniDumper(const AsciiString&)`). MiniDumper.h is only declared under RTS_ENABLE_CRASHDUMP (default off non-Windows).
- Lib/Debug.cpp (core_libraries) is Windows API based (`<windows.h>`, CRITICAL_SECTION, GetModuleFileName, OutputDebugString, MessageBox); non-Windows targets need those from the compat layer, as before for Common/Debug.cpp. Check Android/Switch/wasm link.
- New public API DebugSetLogFlushEachLine(bool) in Lib/Debug.h (extern "C", uses `bool`).
- GgcRuntimeFlags.h is in WWLib; Common/Debug.cpp now includes it (it did before).

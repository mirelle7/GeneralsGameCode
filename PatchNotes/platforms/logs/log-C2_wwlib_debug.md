# Log: C2_wwlib_debug

Base 218fd403c8e, ours = HEAD (trunk ab40ada509a + wasm), theirs = origin/feature/xbox-uwp (06cf302294f).
42 conflict hunks resolved in 14 files, plus 2 modify/delete files. Line endings preserved (all CRLF). No conflict markers left.

| file | hunk (line or function) | resolution | rule # | NEEDS-REVIEW? |
|---|---|---|---|---|
| WWLib/DbgHelpLoader.cpp/.h (MD) | theirs wrapped the whole class/impl in `#ifdef _WIN32` | dropped, superseded: trunk moved it to `Dependencies/DbgHelp/`, and the root CMakeLists only builds that library `if (WIN32)` (non-Windows gets an empty INTERFACE `dbghelploader`, with no include dir). Old-path files deleted. Nothing to port. | 2 | no |
| WWDownload/registry.cpp | getStringFromRegistry locals | combined: trunk's NUL-terminate fix (`= {0}`, `sizeof(buffer) - 1`) + theirs' `DWORD` types (LP64: `unsigned long` is 8 bytes; on Windows DWORD == unsigned long, so the bytes are the same) | 1,3 | no |
| WWLib/Except.cpp | symptr->Address / SymGetSymFromAddr block in Dump_Exception_Info | theirs (full x64 implementation via `GGC_*` macros) instead of trunk's `#if _M_IX86 ... #else @todo stub` | 1 | **yes** |
| WWLib/Except.cpp | register/FP/EIP/stack dump (auto-merged region) | removed trunk's outer `#if defined(_M_IX86)` / `#else "not yet implemented on x64"` wrapper. Theirs' inner `#if _M_IX86 / #elif _M_X64` register dump stays. | 1 | **yes** (same item) |
| WWLib/Except.cpp | Stack_Walk `STACKFRAME` + trunk's `#else return 0` stub | theirs (`GGC_STACKFRAME`, RtlCaptureContext on x64, StackWalk64). Trunk's x64 stub removed. | 1 | **yes** (same item) |
| WWLib/Except.cpp | trunk-only: ThreadListAlive sentinel, `ULONG_PTR *fptr`, `WWDebug/` include paths | kept (auto-merged) | 1 | no |
| WWLib/FastAllocator.h | malloc.h include | ours (trunk: `<malloc.h>` on _WIN32, `<cstdlib>` otherwise, which also covers theirs' Apple `<malloc/malloc.h>` case) | 2 | no |
| WWLib/TARGA.h | TGA2Footer / extension fields (3 hunks) | ours (`int`, the same LP64 fix as theirs' `int32_t`). Theirs' bugfix comment kept. Removed theirs' now-unused `<cstdint>` include block. | 2 | no |
| WWLib/always.h | null W3DMPO class | ours: trunk removed the `W3DMPO` class and renamed the macro to `W3DMPO_CODE`, so theirs' virtual-dtor fix has nothing to apply to | 2 | no |
| WWLib/bittype.h | uint32 / sint32 / DWORD (3 hunks) | ours (`uint32_t`/`int32_t`, DWORD `#ifdef _WIN32`). Kept bobtista's DWORD comment from theirs (still accurate); dropped theirs' uint32 comment ("keep long on Windows" no longer true). | 2 | no |
| WWLib/win.h | non-Windows `#include <windows.h>` | ours (same include, without theirs' comment) | 2 | no |
| WWSaveLoad/persistfactory.h | Save objptr | ours (`uintptr_t`), the same fix as theirs | 2 | no |
| compat/win32_shims/windows.h | strupr | combined: trunk's `#if !defined(__EMSCRIPTEN__)` + `static inline`, plus theirs' `__attribute__((weak)) inline` (non-static), scoped to `__SWITCH__` only (theirs' commit "switch try") | 1,3 | minor: confirm Switch newlib still needs it |
| compat/win32_shims/windows.h | auto-merged theirs' `typedef long long __int64/_int64` | **reverted to trunk's `#define`**. Trunk's wwprofile.h no longer has the `typedef ... __int64` it worked around. Trunk code uses `unsigned __int64` (BgfxBackend.cpp) and `typedef int64_t _int64` under `#ifndef _int64` (old profile_funclevel.h pattern), and both break with a typedef. | 2,3 | no |
| compat/win32_shims/winsock.h | WSAStartup | ours (trunk's static inline fills wVersion plus more fields; the same fix as theirs' ggc_wsa_startup) | 2 | no |
| debug/debug_debug.cpp | SkipNext return-address capture (2 hunks) | ours (`unsigned help`, trunk's `_ReturnAddress` x64 branch); the asm line put back to `mov help,eax`. Added theirs' extra `#elif __GNUC__/__clang__` → `__builtin_return_address(0)` branch for non-x86 GCC/Clang. | 1,2 | no |
| debug/debug_debug.cpp | `<intrin.h>` include (auto-merged duplicates) | combined: one include, guarded with theirs' `defined(_MSC_VER) && !defined(_M_IX86)` + `#pragma intrinsic`. Trunk's unguarded `_MSC_VER` include would hit VC6, which has no intrin.h. Dropped theirs' `Utility/stdint_adapter.h` include (path gone, uintptr_t no longer used). | 4 | no |
| debug/debug_debug.cpp | AssertDone IDRETRY (2 hunks) | ours | 2 | no |
| debug/debug_except.cpp | 8 hunks: IP/register/FPU dump, DlgProc INT_PTR, bytes around IP | ours. Trunk's x64 version is more complete (FltSave dump; theirs printed "not implemented"). Removed theirs' now-unused `DEBUG_CONTEXT_IP` macro. The file is now identical to trunk. | 2 | no |
| profile/profile_funclevel.h | header types / GetCalls/GetTime/GetFunctionTime / GetId (5 hunks) | ours (`u64`, `uintptr_t GetId`). Dropped theirs' `Utility/stdint_adapter.h` include (it moved to `Precompiled/`, which is force-included). The file is identical to trunk. | 2 | see builder note |
| profile/profile_funclevel.cpp | 6 return-type hunks | ours (`u64`). The file is identical to trunk. | 2 | no |

## NEEDS-REVIEW
1. **Except.cpp x64 path**: chose theirs' full x64 crash dump (symbol lookup, register dump, StackWalk64) over trunk's deliberate "@todo, verify on x64" stubs. On x86 the logic is unchanged; only the address formatting moved from `%08X` to `%p`, which on MSVC x86 prints the same 8 uppercase hex digits. Theirs ran on Win64/Xbox. If trunk wants to keep its stubs, revert these three regions to trunk.
2. windows.h strupr: weak non-static `inline` only on `__SWITCH__`; static everywhere else, as on trunk.

## Builder notes
- `WWLib/DbgHelpLoader.{h,cpp}` are deleted. The library is `Dependencies/DbgHelp` (target `dbghelploader`, WIN32 only, so UWP included). Callers that still use the old API (`DbgHelpLoader_minidump.h`, `DbgHelpLoader::` statics) belong to other groups: Core MiniDumper.cpp, Generals/GeneralsMD StackDump.cpp.
- Trunk's `profile_funclevel.h` includes `<cstdint>` unguarded, and `bittype.h` includes `<stdint.h>`. That is trunk's own code, kept as is. If VC6 builds the profile lib and fails there, switch to the force-included `Precompiled/stdint_adapter.h`.
- `Utility/stdint_adapter.h` no longer exists (moved to `Dependencies/Precompiled/Precompiled/`). Any theirs' code elsewhere that includes `<Utility/stdint_adapter.h>` will fail.
- The `__int64`/`_int64` shim is back to `#define` (trunk). Theirs' xbox wwprofile.h `typedef signed long long __int64` must not come back (in the current tree it doesn't).
- Except.cpp uses `uintptr_t`, which comes from the force-included precompiled.h on VC6.

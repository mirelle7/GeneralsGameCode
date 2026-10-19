# log-U1_build (upstream merge, build system)

All 17 existing files in the group are conflict-marker free and still CRLF. `Core/Libraries/Include/CMakeLists.txt` no longer exists (moved to Core/Libraries/Source/Lib by upstream #3512; nothing to do).

| file | hunk | resolution | rule | NEEDS-REVIEW |
|---|---|---|---|---|
| CMakeLists.txt | gamemath include | upstream (`IS_VS6_BUILD` -> `add_library(gamemath INTERFACE)`, else include gamemath.cmake) | 1 | no |
| CMakeLists.txt | big middle hunk | combined: kept all of our SDL3/OpenAL/webfont/ffmpeg/render-backend blocks, added upstream's tracy block; dropped old root MaxSDK add_subdirectory (now in Dependencies/CMakeLists.txt, `add_subdirectory(Dependencies)` is already there) | 1,2 | no |
| cmake/gamemath.cmake | add/add | upstream's #2670 file; folded in only the Emscripten `FE_*` defines for gamemath (wasm needs them). DROPPED our `GM_ENABLE_INTRINSICS OFF FORCE` | 1 | YES: upstream leaves GM_ENABLE_INTRINSICS at the library default (ON, "where results should match software"). Trunk forced it OFF for cross-arch parity. Check gamemath builds/behaves on arm64/Android/Switch/wasm; if not, re-add the FORCE line |
| cmake/compilers.cmake | end of non-MSVC branch | ours (`-fdeclspec` for Clang, Emscripten link/compile options); our earlier `/arch:SSE2`, `/fp:precise` additions (non-conflicting) remain | 2 | maybe: `/arch:SSE2`+`/fp:precise` are trunk's deterministic-math flags, upstream has none; harmless duplicates of intent |
| cmake/config-build.cmake | options / feature_info | combined (SAGE_USE_SDL3/OPENAL/GGC_DIAGNOSTIC_TOOLS + upstream TESTS/BENCHMARKS) | 1 | no |
| cmake/config-build.cmake | `_UNIX` define | upstream moved it to `deps_config`; kept it there and extended condition to `UNIX OR NintendoSwitch`; removed our `core_config` copy. GGC_DIAGNOSTIC_TOOLS, SDL3 and OPENAL defs kept on core_config | 1,2 | no |
| Core/GameEngine/CMakeLists.txt | GameMemory/MiniDumper appends | upstream (GameMemory sources now unconditional in the list; MiniDumper.cpp/.h gone from list, now `core_minidumper` in Core/Libraries/Source/MiniDumper) | 1 | no |
| Core/GameEngine/CMakeLists.txt | public link libs | `core_minidumper` + `deps_dbghelp`; kept our move of `d3d8lib` into the non-bgfx conditional (removed from main list) | 1,2 | no |
| Core/GameEngineDevice/CMakeLists.txt | public link libs | upstream names: `deps_bink`/`deps_miles`, but kept our WIN32-only placement (replaced binkloader/milesloader in the WIN32 block); dropped stale Win32BIGFile* REMOVE_ITEM entries (files deleted by #3505) | 2 | no |
| Core/Libraries/Source/WWVegas/CMakeLists.txt | core_wwcommon links | `deps_dbghelp deps_usp10 stlport`; d3d8lib stays in our non-bgfx block; `milesloader` in our WIN32 block renamed `deps_miles` (Win-only kept); comment updated core_utility -> deps_utility | 2 | no |
| GeneralsMD/.../WWVegas/CMakeLists.txt | z_wwcommon | upstream (stlport removed, core_wwcommon provides it); WIN32 `milesloader` -> `deps_miles` | 1 | no |
| Core/.../WW3D2/CMakeLists.txt | Backend file list | ours: `Backend/RenderBackend.cpp/.h` in main list; `DX8Backend.cpp/.h` stay in our `GGC_RENDER_BACKEND dx8` block (upstream lists DX8Backend.cpp unconditionally) | 2 | YES: list also has a top-level `RenderBackend.h` (line ~160, from upstream's move) AND `Backend/RenderBackend.h`; both exist on disk. U3_ww3d2 owner should confirm only one RenderBackend.h survives and fix the list |
| Core/.../WWMath/CMakeLists.txt | gamemath link | upstream (unconditional; VS6 gets the INTERFACE stub) | 1 | no |
| Core/.../debug/CMakeLists.txt | 3 hunks | upstream contents (Include dir PUBLIC, `Precompiled/precompiled.h` + `Include/debug/debug.h` PCH, depsi_always/core_config/deps_utility/stlport links) kept inside our `if(WIN32)` guard | 1,2 | no |
| Core/.../profile/CMakeLists.txt | PCH + links | upstream header names inside our WIN32/else split PCH; links: upstream list plus `core_wwcommon` (needed on non-Win for win32_shims), `core_debug` left only in our WIN32 block (target does not exist elsewhere) | 1,2 | no |
| Dependencies/Utility/CMakeLists.txt | core_utility -> deps_utility | upstream rename; our non-Windows win32_shims include dirs + `<windows.h>` PCH block re-targeted onto `deps_utility` | 1,2 | no |
| Dependencies/Utility/Utility/endian_compat.h | include | combined: `#include "bit_adapter.h"` before the `#if`, our `__EMSCRIPTEN__` in the Linux branch kept | 1,2 | no |
| Dependencies/Utility/Utility/stringex.h | 2 hunks | combined: upstream `unichar` signatures, our `HAVE_WCSLCPY` guards (Switch newlib) kept | 1,2 | no |
| GeneralsMD/Code/GameEngine/CMakeLists.txt | GameMemory block | upstream (block removed); our WIN32 `StackDump.cpp` append kept | 1,2 | no |
| GeneralsMD/Code/Main/CMakeLists.txt | z_generals link list | ours `z_gameengine` kept; upstream's comctl32/imm32/vfw32/winmm are already in our WIN32 block; `binkloader`/`milesloader` there renamed `deps_bink`/`deps_miles` | 2 | no |

## File-list verification
- Every non-commented source/header listed in Core/GameEngine, Core/GameEngineDevice, GeneralsMD GameEngine, Core/Libraries/Source/{Lib,MiniDumper,debug,profile} CMake files exists on disk. Reverse check: BIGFileSystem.h/.cpp, GameMemory*, MiniDumper (moved), PathfindConstants etc. are listed; StdBIGFile*/Win32BIGFile* no longer listed and not on disk.
- Trig.cpp: listed only in Core/Libraries/Source/Lib/CMakeLists.txt (exists). No CMake lists the old Generals/GeneralsMD copies.
- No `visualc.h` listed anywhere except Core/Tools/WW3D/pluglib's own file (exists upstream).
- No leftover references to `dbghelploader`, `usp10loader`, `milesloader`, `binkloader`, `core_utility`, `core_precompiled` in any CMake/cmake file (comment fixed).

## Things for the builder / other groups
- Stale files still on disk, not listed in CMake, deleted upstream: `Generals/Code/GameEngine/Source/Common/System/Trig.cpp` and `GeneralsMD/Code/GameEngine/Source/Common/System/Trig.cpp` (owner of those paths should delete them).
- `GeneralsMD/Code/GameEngineDevice/CMakeLists.txt` (not my group) has REMOVE_ITEM entries for Win32GameEngine/Win32OSDisplay that are only harmless leftovers.
- Dependencies/Bink and Miles loaders are now `deps_bink`/`deps_miles` (built on every platform as before, linked only on WIN32 by us).
- `core_profile_legacy` and `core_debug` use `Precompiled/precompiled.h` (Dependencies/Precompiled). Core/Libraries/Source/Lib, WWLib, W3DView, Generals GameEngine still reference `BasePrecomp.h` which still exists upstream.
- gamemath intrinsics question above (NEEDS-REVIEW) is the main functional risk.

# log-U4_devices

Upstream merge (upstream/main 2886271c5cb into `platforms`). Group: device layer + BIG file code.

| file | hunk | resolution | rule | NEEDS-REVIEW |
|---|---|---|---|---|
| Core/GameEngine/Source/Common/System/BIGFileSystem.cpp | HEAD's Zero Hour BIG load-priority helpers (GetBigFilename/GetBIGLoadPriority) + ctor | ported onto merged BIGFileSystem; lambda in std::sort replaced by static `BIGLoadOrderLess` (VC6-safe) | 1,4 | no |
| BIGFileSystem.cpp | priority sort in loadArchiveFilesFromDirectory | ported (HEAD from Win32 + Std versions) | 1 | no |
| BIGFileSystem.cpp | init(): `__EMSCRIPTEN__` empty-archive warning, `_UNIX` `$CNC_ZH_INSTALLPATH` extra root (GetStringFromRegistry), `GGC_PLATFORM_UWP` "generals\\" fallback | ported from StdBIGFileSystem, platform guards kept | 2 | no |
| BIGFileSystem.cpp | openArchiveFile: 10x retry on open failure, header read check, identifier mismatch log, path buffer overrun guard, Switch svcOutputDebugString logs (extern decl, parseBig/parseFile/loadBigs/openBig), mount-failure log, trailing-separator normalization of `dir` | ported from StdBIGFileSystem. Retry uses std::thread/chrono, excluded on VC6 (`_MSC_VER < 1300`). Retry, stderr logs, overrun guard and dir normalization now apply on every platform incl. Windows (they used to be Std-only) | 1,3 | YES: Windows now takes these robustness paths; if byte-identical Windows is required, gate with `#if !defined(_WIN32) \|\| defined(GGC_PLATFORM_UWP)` |
| Core/GameEngine/{Include/Common/ArchiveFile.h, Source/Common/System/ArchiveFile.cpp} | HEAD's `CriticalSectionClass m_fileLock` + lock around `openFromArchive` (was in Win32BIGFile/StdBIGFile) | ported into merged ArchiveFile (`WWLib/mutex.h` include) | 1 | no |
| StdBIGFile.h/.cpp, Win32BIGFile.h/.cpp, StdBIGFileSystem.cpp | modify/delete | old-path files deleted (plain delete); Win32BIGFileSystem.* were already gone | 1 | no |
| GeneralsMD/.../SDL3GameEngine.cpp | not in group, but referenced deleted StdBIGFileSystem | 2 lines: include `Common/BIGFileSystem.h`, `NEW BIGFileSystem` (no conflict markers in that file) | 1 | no |
| StdLocalFileSystem.cpp | 3 hunks (#3530 case-insensitive first path component) | upstream's root_path/relative_path walk + pathSearch scan; kept trunk's `for (auto p : ...)` (Apple libc++ fix); trunk's duplicate leading-component scan dropped (same fix) | 2 | no |
| W3DDisplay.cpp | includes, Set_Gamma, Begin_Scene + new Free_All_FontChars_Glyph_Caches call | HEAD backend calls (`WW3D::Get_Render_Backend()->...`), no `Renderer.h`; upstream glyph-cache purge kept | 2 | no |
| W3DDisplayString.cpp | hotkey early-out | upstream's (identical logic to trunk's) | 2 | no |
| W3DMouse.cpp (2) | atan2 | upstream `WWMath::Atan2` (post-merge lines already use WWMATH_* constants) | upstream brief | no |
| W3DParticleSys.cpp | includes | both (BgfxRenderProfile, W3DWater, W3DTerrainParticles) | 1 | no |
| W3DProfilerFrameCapture.cpp | trunk's backend capture vs upstream Round->Roundf in removed code | HEAD | 2 | no |
| W3DScreenshot.cpp | surfaceCopy creation | HEAD (Create_Legacy_Surface_Wrapper + Peek_Legacy_Surface); upstream's `SurfaceClass(w,h,fmt)` + Peek_D3D_Surface not adopted | 2 | YES: if wanted, use `NEW_REF(SurfaceClass,(w,h,fmt))` with Peek_Legacy_Surface |
| W3DShaderManager.cpp, W3DView.cpp, W3DWater.cpp, Generals+ZH W3DScene.cpp | `Renderer::Clear` etc. vs `WW3D::Get_Render_Backend()->Clear` and includes | HEAD (ZH Scene: HEAD Set_Color_Write_Mask/Set_Z_Bias too) | 2 | see build notes (Renderer.h) |
| W3DSnow.cpp | trunk removed point-sprite path (renderSubBox, FtoDW); upstream only changed FtoDW->F2DW inside it | HEAD (path stays removed) | 2 | no |
| W3DWater.cpp | drawRiverWater indexCount guard (#upstream arcticdolphin) | upstream block; `BUFFER_TYPE_DYNAMIC` instead of `_DX8` alias; `m_drawingRiver = true` now set after the guard (as upstream) | 1 | no |
| Generals + ZH W3DShroud.h | m_shroudData | upstream: `UnsignedShort *m_shroudData`; m_pSrcTexture/m_srcTextureData/m_srcTexturePitch removed | upstream brief | no |
| Generals W3DShroud.cpp | sysmem copy storage, dtor, init, reset, fillBorder, copy | upstream (CPU `UnsignedShort` array, Lock-based copy). Added trunk's bgfx `Capture_Shroud_Texture` call on top, fed from m_shroudData, pitch = numCellsX*2, format R5G6B5 (A4R4G4B4 when RTS_DEBUG fogOfWar); includes RenderBackend.h/surfaceclass.h | 1,2 | no |
| ZH W3DShroud.cpp | same as Generals, plus trunk-only pieces | upstream storage/copy. Kept trunk's ShroudDiag block, `computeShroudPixel`, Is_Device_Lost, `shouldCaptureForBgfx`, but rewired them to m_shroudData (pitch/format computed). Dst texture: `#if GGC_RENDER_BACKEND_BGFX` trunk's `Create_Writable_Shroud_Texture`, else upstream's POOL_MANAGED TextureClass (so upstream's locked-copy works on DX8). HEAD's DX8 `Upload_Texture_Region` path in render() dropped (upstream's Lock copy replaces it); getShroudLevel guard now `m_shroudData != nullptr` | 1,2,7 | YES: DX8 (non-bgfx) uses managed pool + Lock; bgfx dst texture is not locked in `fillBorderShroudData`/copy (those are `#if !GGC_RENDER_BACKEND_BGFX`). Verify shroud on both backends |

## Watch-outs for the builder
- `Core/GameEngineDevice/CMakeLists.txt` (group U1) still lists `Win32BIGFile.h/.cpp`, `Win32BIGFileSystem.h/.cpp`; remove them (files are gone). No CMake entries exist for StdBIGFile*; `BIGFileSystem.cpp/.h` are already in Core/GameEngine/CMakeLists.txt.
- `Win32GameEngine.h` already uses `BIGFileSystem`; `SDL3GameEngine.cpp` now does too. Any other platform engine (Android/iOS/Switch/UWP/wasm) that referenced `StdBIGFileSystem` needs the same change (a repo grep found none left).
- `WW3D2/Renderer.h`: upstream added a small static `Renderer` class (Init/Set_Gamma/Clear/Set_Viewport...) with this name, whereas this tree's `Renderer.h` is trunk's large backend interface. I did not use upstream's `Renderer::` API in any device file (kept `WW3D::Get_Render_Backend()->`); the WW3D2 group must make sure there is no add/add clash and that nothing else calls `Renderer::X`.
- W3DDisplay.cpp no longer includes `WW3D2/Renderer.h`; if some symbol from it is needed, add back.
- ArchiveFile now holds a `CriticalSectionClass`; the class is not copied anywhere.
- VC6: BIGFileSystem.cpp now includes `<chrono>/<thread>` only when not VC6; `<cstdio>`, `<algorithm>`, `<vector>` are used by VC6 already (as HEAD's Win32 file did).

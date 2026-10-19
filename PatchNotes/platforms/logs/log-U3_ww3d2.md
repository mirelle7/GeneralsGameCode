# log-U3_ww3d2

## Key decision (NEEDS-REVIEW): upstream #3453 static `Renderer` vs HEAD's polymorphic IRenderBackend
Upstream replaced `IRenderBackend` (object, `WW3D::Get_Render_Backend()`) with a static `Renderer` class
(IRenderBackend.h -> Renderer.h, Backend/RenderBackend.h + Backend/DX8Backend.h deleted). HEAD's IRenderBackend is a
~1300-line virtual interface with the bgfx backend holding instance state, and hundreds of call sites in files
outside this group use `WW3D::Get_Render_Backend()->X`. Upstream's change was a pure rename (checked the full diff), so
nothing functional is lost by keeping HEAD's design. Resolution:
- `WW3D2/Renderer.h` = HEAD's full IRenderBackend.h content (class IRenderBackend, RenderBackendViewport, ...) plus an
  appended `typedef RenderBackendViewport RenderViewport;` and a small static `class Renderer` facade
  (Set_Gamma/Begin_Scene/End_Scene/Flip_To_Primary/Clear/Set_Viewport/Invalidate_Cached_Render_States/Set_Ambient/
  Set_Light_Environment) forwarding to `WW3D::Get_Render_Backend()`. Reason: git auto-merged upstream's
  `Renderer::Clear(...)`/`RenderViewport`/`#include "Renderer.h"` into non-conflicting files (W3DDisplay, W3DView,
  W3DWater, W3DShaderManager, W3DScene x2, Generals camera/render2d/scene, GeneralsMD camera, ...) so those compile unchanged.
  Facade bodies live in `Backend/RenderBackend.cpp` (always compiled, both backends).
- `WW3D2/IRenderBackend.h` recreated as a 1-line forwarding header to `Renderer.h` (many files still include it).
- `Backend/RenderBackend.h` restored from HEAD (the merge had deleted it; HEAD files include it).
- `Backend/DX8Backend.h` and `Backend/DX8Backend.cpp` = HEAD versions (upstream's only change to the .cpp was the rename).
- `ww3d.h` (not in my list, outside conflicts): re-added `class IRenderBackend;`, `Get_Render_Backend()` and
  `static IRenderBackend * RenderBackend;` that the clean merge had removed. `Get_Previous_Sync_Time` from upstream kept.
- `ww3d.cpp` = HEAD version entirely (upstream's diff there was rename-only + a SurfaceClass ctor change inside screenshot code HEAD replaced by `Capture_Back_Buffer_Image`).

## Table
| file | hunk | resolution | rule | NEEDS-REVIEW |
|---|---|---|---|---|
| WW3D2/Renderer.h | all 5 | ours (HEAD interface) + Renderer facade appended | 1,7 | YES (see above) |
| WW3D2/Backend/DX8Backend.cpp | 12 hunks | ours (whole file = HEAD) | 1 | YES |
| WW3D2/Backend/DX8Backend.h | no markers (modify/delete) | kept HEAD file, upstream deleted it only because of the Renderer rename | 1 | YES |
| WW3D2/ww3d.cpp | 9 hunks | ours (whole file = HEAD) | 1 | YES |
| WW3D2/ww3d.h | clean-merge fallout | restored Get_Render_Backend/RenderBackend | 1 | YES |
| WW3D2/indexbuffer.h | (new location of dx8indexbuffer.h content) | ported upstream's `enum { MAX_INDEX_COUNT = 0xffff }` into DynamicIBAccessClass (W3DWater.cpp already uses it) | modify/delete | no |
| WW3D2/dx8indexbuffer.h | 1 | ours (empty: HEAD moved classes to indexbuffer.h) | 1 | no |
| WW3D2/dx8wrapper.cpp | Log_DX8_ErrorCode | combined: HEAD `S_OK` + upstream `WWDEBUG_SAY(("%s", tmp))` | 1 | no |
| WW3D2/dx8wrapper.cpp | Shutdown texture release | ours: single `FixedFunctionState::Release_Raw_Textures()` (upstream's ARRAY_SIZE loop wrapped around HEAD's call removed; upstream's cached-texture release semantic is what Release_Raw_Textures does) | 1 | yes, minor |
| WW3D2/dx8wrapper.h | fog start/end | combined: HEAD `Commit_Fixed_Function_Render_Value` + upstream `std::bit_cast<DWORD>` (#3532); F2DW now `std::bit_cast` inline in dx8wrapper.h (#3516) | 1,2 | no |
| WW3D2/shattersystem.cpp | Sqrt | theirs `WWMath::Sqrt((float)...)` (HEAD's Sqrt_Legacy was trunk's own #2670 attempt) | upstream wins | no |
| WW3D2/texproject.cpp | hfov/vfov | theirs `WWMath::Atan` (upstream already uses `Fabsf_Legacy` on the tan lines, merged cleanly) | upstream wins | no |
| WW3D2/render2dsentence.h | CharData | combined: HEAD `Advance` + upstream `uint8 * Buffer`; added private `Setup_Glyph_Cache(int,int)` | 1 | no |
| WW3D2/render2dsentence.cpp | Store_GDI_Char dispatch | ported all HEAD non-GDI glyph paths (stb / CoreText / FreeType) onto upstream's new cache format: glyph buffers are `uint8` raw coverage, `Blit_Char` rebuilds the A4R4G4B4 texel. Glyph width clamped to `GlyphBitmapWidth`; `CurrPixelOffset += char_width*CharHeight`. GDI branch takes upstream's `GlyphBitmapWidth/Height` | 1,3 | YES (not compiled/run) |
| WW3D2/render2dsentence.cpp | Blit_Char | `#if !defined(_WIN32) && !defined(GGC_FONT_STB)` rebuilds premultiplied texel (RGB=alpha) to keep the CoreText/FreeType look; GDI and stb keep upstream's white-RGB + alpha texel (byte-identical on Windows) | 3 | yes |
| WW3D2/render2dsentence.cpp | Store_GDI_Char tail | combined: HEAD `Advance = char_size.cx` + upstream `Buffer = glyph_buffer_p` | 1 | no |
| WW3D2/render2dsentence.cpp | Create_GDI_Font (stb/Apple/FreeType) | new `FontCharsClass::Setup_Glyph_Cache(CharHeight*2+PixelOverlap+1, CharHeight)` before each `return true`, so `Update_Current_Buffer` (now GlyphBlockBytes/GlyphCellBytes based) works on non-GDI | 1 | yes |
| WW3D2/render2dsentence.cpp | include + Allocate_New_Surface | re-added `#include "dx8wrapper.h"` (HEAD had dropped it); upstream's `Get_DX8_Caps().MaxTexture*` replaced by `DX8Caps::Get_Max_Texture_Width/Height()` | 1 | yes (Apple include order) |
| GeneralsMD/.../mapper.cpp | local F2DW | theirs (removed; F2DW now in dx8wrapper.h) | upstream wins | no |
| GeneralsMD/.../meshmdl.cpp | includes | combined (HEAD backend includes + `Utility/bit_adapter.h`) | 1 | no |
| GeneralsMD/.../render2d.cpp | viewport | ours (backend Set_Viewport/Set_Texture) | 1 | no |
| GeneralsMD/.../scene.cpp | Clear | ours | 1 | no |
| GeneralsMD/.../shader.cpp | includes + patch segments | ours (backend `Set_Patch_Segments`); bit_adapter include not needed | 1 | no |
| Generals + GeneralsMD camera.cpp | (no markers present; already merged) | left: uses `RenderViewport`/`Renderer::Set_Viewport` via the facade, upstream `WWMath::Atan2` | - | no |

## Builder notes
- WW3D2/CMakeLists.txt still has an unresolved hunk (not in my group). It must list: `Backend/RenderBackend.cpp`,
  `Backend/RenderBackend.h`, `Backend/DX8Backend.cpp/.h` (DX8 only), `IRenderBackend.h` (shim), `Renderer.h` (new upstream name), `RenderBackend.h`.
- Anything still writing `uint16` into `FontCharsClassCharDataStruct::Buffer` elsewhere will not compile (it is `uint8*` now); I found none.
- `Renderer::Init/Shutdown` (upstream) are intentionally NOT provided; WW3D keeps `Create_Render_Backend`.
- Other groups: leave `Renderer::X`/`RenderViewport` lines produced by the clean merge alone (they work through the facade) or revert them to
  `WW3D::Get_Render_Backend()->X` / `RenderBackendViewport`; both compile.
- Facade methods deref `Get_Render_Backend()` without null check (HEAD's ww3d.cpp keeps its own null checks).

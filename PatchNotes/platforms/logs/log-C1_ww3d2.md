# Log: C1_ww3d2 (WW3D2 renderer + bgfx backend)

Theirs = origin/feature/xbox-uwp, base = 218fd403c8e, trunk = ab40ada509a. All edits keep each file's line endings (CRLF everywhere except Backend/BgfxBackendTextures.cpp, which is LF).

## Modify/delete (MD) files

| file | resolution | notes |
|---|---|---|
| WW3D2/BgfxBackend.cpp | ported to `WW3D2/Backend/BgfxBackend.cpp`; old file deleted | See the port table below |
| WW3D2/BgfxBackendState.h | ported to `Backend/BgfxBackendState.h`; old file deleted | Added `BgfxDevice::renderScale` and `BgfxStats::skipNoProgram/skipNoBuffer`. Trunk already had `deferredDestroyVB[Prev]` and `uint64_t next_id`. Dropped `currentFVF` (ggc-bisect, "strip when done") and `lastDrawFrame` (see NEEDS-REVIEW 4) |
| WW3D2/BgfxBackendTextures.cpp | ported to `Backend/BgfxBackendTextures.cpp`; old file deleted | See the textures table below |
| WW3D2/DX8Backend.cpp | old file deleted; nothing to port | Theirs only changed `unsigned __int64` to `uint64_t` in three casts. Trunk's `Backend/DX8Backend.cpp` no longer has those casts |
| WW3D2/StubD3D8Device.cpp | deleted; nothing to port | Trunk removed the standalone D3D shim (65389d123cf "Remove standalone D3D device shim"). bgfx no longer creates a D3D device, and dx8caps.cpp/dx8wrapper.cpp took over. Theirs' changes were the d3d8_iids include (no longer needed), Apple-only `/tmp/ggc_vblock.log` diagnostics marked "strip when done" (dropped), and a `DriverVersion` guard. Trunk already handles `DriverVersion` (dx8caps/DX8Backend) |
| GeneralsMD/.../sortingrenderer.cpp | ported to `Core/.../WW3D2/sortingrenderer.cpp` (unified by trunk a1358a97e1e); old file deleted | Added theirs' VC6 `namespace std { using ::memcmp; }` shim after the includes, guarded by `_MSC_VER < 1300`. Core's file uses `std::memcmp` |

### BgfxBackend.cpp port (theirs' 78 hunks, 5ad07d05a40..06cf302294f)

| theirs' change | resolution | rule | NEEDS-REVIEW |
|---|---|---|---|
| `GgcGetenv` getenv cache (d1e70a70e8b) | not ported: trunk already reads every switch through `GgcFlags` (GgcRuntimeFlags, resolved once and cached) | 2 | |
| `GGC_COALESCE_BUFFER_UPDATES` (06cf302294f, UWP only) | ported. Shadow/queue/flush now sit after `BgfxUseRenderThread()`. The six trunk `bgfx::update` sites (full, sub-range and the deferred range-flush paths for VB and IB) queue into the shadow under `#if GGC_COALESCE_BUFFER_UPDATES`. Flush runs before every `bgfx::frame()` (End_Scene and both shutdown loops), at Shutdown, and at Begin_Scene before deferred destroys. Shadows are forgotten when a handle is created or destroyed. Shadow size is the bgfx buffer's capacity, because trunk's dynamic buffers only grow | 1 | YES (1) |
| `BGFX_CONFIG_RENDERER_DIRECT3D11_USE_STAGING_BUFFER` | already in the merged cmake/bgfx.cmake:180 (another group) | | |
| GLSL desktop-GL renderer (macOS) | ported: `GGC_BGFX_RENDERER_GLSL` shader include block with trunk's full shader list, `GetConfiguredRendererType` returns OpenGL, `GetNativeWindowHandle` skips the CAMetalLayer for GL | 1 | |
| Android dual Vulkan/GLES | ported: spirv blobs embedded next to essl on Android, `GGC_SHADER_DATA/SIZE` macros (all 36 `CreateShaderProgram` call sites converted mechanically), Vulkan preferred with fallback to GLES at `bgfx::init` failure, `GGC_BGFX_FORCE_GLES` (raw `std::getenv`, read once at init; not in the GgcFlags registry), `g_ggcUseVulkanShaders` | 1 | |
| Switch: `nwindowGetDefault` as nwh, svcOutputDebugString logging (callback, init, caps, Begin_Scene, present, Create_Texture) | ported under `__SWITCH__` | 1 | |
| Switch: Create_Texture expands A1R5G5B5/A4R4G4B4 to RGBA8 | ported. Adapted to trunk's Create_Texture, which uploads every mip; the helper now takes a `MipSlice` | 1 | |
| UWP CoreWindow (`SDL.window.winrt.window`) as nwh | ported | 1 | |
| Android `SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER` as nwh | ported | 1 | |
| Emscripten `#canvas` nwh, skip `renderFrame()` on Emscripten | already in HEAD (wasm-clean) | 2 | |
| SDL window pixel size at Initialize/Begin_Scene | already covered by trunk's `GetBackendWindowSize` | 2 | |
| Render-resolution scale (`GGC_BgfxSetRenderScale/GetRenderScale`, called from W3DDisplay.cpp and the Android OptionsMenu) | combined with trunk's supersampling scale. `g_device.renderScale` (0.1-1.0) overrides trunk's [1,2] INI/env scale in `GetSceneRenderScale` when below 1. The setter requests trunk's safe-point rebuild (`g_requestSceneFramebufferRebuild`) rather than recreating the framebuffer immediately. Trunk already scales view rects, viewports and composite texel size from `sceneRenderWidth/Height` | 1 | |
| Android: direct-to-backbuffer at native scale (`noSceneFramebuffer = renderScale >= 0.999`) | ported under `__ANDROID__` | 1 | YES (2) |
| Composite V-flip (`originBottomLeft`) | already in HEAD (fix/gles-composite-vflip) | 2 | |
| `AdjustProjForBgfxDepth` (D3D [0,1] to GL [-1,1] projection remap) | ported with narrower guards. Android always remaps (theirs' exact behaviour). Apple and Switch remap only when `homogeneousDepth`. Windows/UWP/Linux/Emscripten keep trunk's unmodified projection, because theirs' version also remapped on D3D, which would change Windows behaviour | 3, 7 | YES (3) |
| Same-frame dynamic VB orphan on re-upload (`lastDrawFrame`, 27/06) | not ported: it changes behaviour on every platform, trunk has its own grow-only + deferred-destroy scheme, and bgfx Metal/D3D11 already buffer dynamic updates | 2, 3, 7 | YES (4) |
| Camera view transform pushed only at the first opaque draw (27/06 shatter fix) | not ported: trunk's version kept. Trunk re-pushes and re-applies at End_Scene, and has since added several camera fixes | 2, 7 | YES (4) |
| `ShouldHideMissingTextureForCurrentDraw`: ONE/ZERO blend counts as opaque | ported, but only under `__ANDROID__ \|\| __SWITCH__` so Windows stays identical | 3, 7 | YES (5) |
| `GGC_CULL_CW/CCW` macros | not ported: theirs left them as plain aliases ("swap disabled") | | |
| `skipNoProgram/skipNoBuffer` stats | ported (cheap; used by the Android/Switch triage logs) | 1 | |
| Android mesh/visibility counters `g_ggc*` (referenced from dx8polygonrenderer.h, dx8renderer.cpp, mesh.cpp, W3DScene.cpp under `__ANDROID__`) | ported: defined and reset under `__ANDROID__`, logged by the ggc-3d triage line | 1 | |
| Android bgfx trace limiter (250 lines) + logcat routing; Android ggc-perf logs around `bgfx::frame()` | ported under `__ANDROID__`. The limiter sits after trunk's WARN/ERROR filter | 1 | |
| `AllocPhase5Id` `unsigned __int64` to `uint64_t` | applied to trunk's `AllocResourceId` | 1 | |
| Apple ggc-vtx/ggc-alias/ggc-draw/ggc-terrain/ggc-alldraws `/tmp` bisection logs, `GGC_FORCE_SHOT`, `GGC_WIREFRAME`, ggc-bisect skip, `currentFVF` | dropped: all marked "Strip when done" scaffolding from the shatter hunt | | |
| Android one-shot dumps (ggc-vtx, ggc-idx, ggc-draw WORLD, ggc-water, ggc-cls, ggc-mvp, ggc-proj, ggc-fb) | dropped: bounded bring-up dumps that read structures trunk has since restructured | | |

### BgfxBackendTextures.cpp port

| theirs' change | resolution | rule | NEEDS-REVIEW |
|---|---|---|---|
| A1R5G5B5 expand (non-Windows / Switch RGBA8) | already in HEAD (fix/a1r5g5b5-atlas-expand with the Switch byte order) | 2 | |
| A4R4G4B4 uploaded as RGBA8 on Switch (R/B swapped in the expander) | ported | 1 | |
| A8R8G8B8/X8R8G8B8 uploaded as RGBA8 + `SwizzleBGRA8ToRGBA8` (Adreno samples BGRA8 as black) | ported, guarded by `__ANDROID__ \|\| __SWITCH__`. Theirs had `!_WIN32`; macOS/iOS Metal, Linux and the wasm build keep trunk's BGRA8. Swizzle added to the strict source-pitch check | 3, 7 | YES (6) |
| Switch DXT decoded to RGBA8 with bimg | ported. A `decodeDXTToRGBA8` layout flag lets trunk's pitch/row validation use block rows | 1 | |
| ForceOpaque / TeamColor key accept RGBA8 | ported | 1 | |
| Terrain-atlas mip upload with bgfxFmt / `ConvertBGR5A1ToBGRA8` | already covered by HEAD's expand-aware atlas path | 2 | |
| `namespace bgfx { void release(); }` forward declaration, Android `GGC_LOG_TEXUPDATE` | dropped: not needed by trunk's code / diagnostic only | | |

## Conflict hunks in group files

| file | hunk | resolution | rule | NEEDS-REVIEW |
|---|---|---|---|---|
| IRenderBackend.h | includes | combined: theirs' VC6 `#endif` + trunk's `<vector>` | 1, 4 | |
| dx8caps.cpp | 2x DriverVersion | ours: trunk reads `DriverVersionHighPart/LowPart` from the non-Windows shim, which beats theirs' zeros | 2 | |
| dx8wrapper.cpp | 2x DriverVersion | ours (same reason); file is now identical to trunk | 2 | |
| dx8webbrowser.h | 2 | ours: trunk's `_WIN32 && !GGC_RENDER_BACKEND_BGFX` stub also covers UWP; file identical to trunk | 2 | |
| framgrab.h | 1 | ours: trunk compiles the class on non-Windows with inline no-ops. Also removed theirs' auto-merged `#if defined(_WIN32)` wrapper at the top (it no longer had a matching `#else/#endif`). File identical to trunk | 2 | |
| dx8polygonrenderer.h | Render() | combined: Android `g_ggcMeshPolyRenders` counter + trunk's `Submit_Rigid_Packet` | 1 | |
| dx8renderer.cpp | overflow skip | combined: trunk + Android counter | 1 | |
| render2dsentence.cpp | Store_GDI_Char, Free_GDI_Font (+ restructuring) | combined; see the note below | 1, 7 | YES (7) |
| shaders/vs_scene_composite.sc | comment | ours (duplicate V-flip fix) | 2 | |
| shaders/vs_trees.sc | $output + UV writes | ours: trunk's outputs match trunk's fs_uber (it already writes `v_sceneDepth`) | 2 | |
| surfaceclass.cpp | 2x pointer arithmetic | ours: trunk's `static_cast` + offset is 64-bit clean, same as theirs' `uintptr_t` | 2 | |
| textureloader.cpp | includes; anon-namespace block; TGA open fail; TGA load fail; Update() sync load; Apply_Missing_Texture | ours, plus theirs' Android/Switch diagnostic logs and the `GGC_SwitchDebugLog` helper (other auto-merged hunks already call them). The sync-load hunk is ours (same Emscripten fix as theirs). The missing-texture logs use trunk's `Texture ? ... : ...` pattern because trunk allows a null Texture there | 1, 2 | |
| Generals + GeneralsMD ddsfile.h | Surface field | ours: trunk already uses `unsigned Surface;` with a 124-byte static_assert | 2 | |
| GeneralsMD meshgeometry.cpp | includes | ours + Android log include (used by auto-merged diagnostics) | 1 | |
| GeneralsMD assetmgr.cpp | includes; missing-file log; `lstrcpyn` length | ours + Android include/log; ours for the length (trunk's pointer difference is portable) | 1, 2 | |

render2dsentence.cpp note: trunk added CoreText (Apple) and FreeType+Fontconfig (other non-Windows, plus embedded DejaVu on Emscripten). Theirs added stb_truetype for all non-Windows plus UWP. Merged split, via a new `GGC_FONT_STB` (defined for `GGC_PLATFORM_UWP || __ANDROID__ || __SWITCH__`):
- stb_truetype (theirs) on UWP, Android and Switch.
- CoreText (trunk) on Apple.
- FreeType (trunk) on Linux and Emscripten.
- GDI (trunk) on Windows.

The stb `Create_GDI_Font` block now sits at the top of the function instead of inside the Windows GDI branch. Its `PixelOverlap` is derived from the same 96 dpi cell height, so it no longer calls the GetDC stub on UWP. The stb glyph code was adapted to trunk's `BufferList[i].Buffer` (value, not pointer) and sets `char_data->Advance`. In `Free_GDI_Font`, the FreeType cleanup is guarded so the stb platforms skip both the FreeType and the GDI cleanup.

Trivial: none beyond the above.

## NEEDS-REVIEW

1. UWP buffer coalescing on trunk's render thread. Trunk turns the bgfx render thread on by default everywhere (UWP included). Theirs' flush used `bgfx::makeRef` on the assumption that UWP was single-threaded. The port uses `bgfx::copy` when `BgfxUseRenderThread()` is true and `makeRef` otherwise. Measure Xbox perf both ways (`GGC_BGFX_NO_RENDER_THREAD=1` gives theirs' original setup).
2. Android native-scale direct path. On Android at native scale, `noSceneFramebuffer` is forced on (theirs' perf choice). That also disables trunk's newer post-processing (bloom, SSAO, soft particles) on Android at scale 1.0.
3. Projection remap scope. The remap now applies only on Android (always) and on Apple/Switch when `homogeneousDepth`. The wasm build (GLES/WebGL) keeps trunk's unmodified projection, because HEAD's wasm is verified working that way. Linux GL is unchanged too.
4. Not ported: theirs' two "shatter" fixes, the dynamic-VB orphan on re-upload and pushing the camera only once per frame. If the macOS/iOS shatter shows up on this branch, revisit both.
5. ONE/ZERO-blend-as-opaque missing-texture rule. Limited to Android/Switch; theirs had it on every platform.
6. RGBA8 upload + swizzle for 32-bit textures. Limited to Android/Switch (theirs had all non-Windows). Trunk's newer BGRA8-only paths are still BGRA8 everywhere: DXT5-with-alpha expansion, packed mesh atlas, sorted texture-array pages. Theirs reported that BGRA8 samples black on Adreno/switch-mesa, so these may need the same treatment there.
7. Font backend split (`GGC_FONT_STB`). Needs a text check on UWP, Android and Switch.

## Builder notes

- Old-path files deleted from the working tree (not `git rm`): `WW3D2/BgfxBackend.cpp`, `WW3D2/BgfxBackendState.h`, `WW3D2/BgfxBackendTextures.cpp`, `WW3D2/DX8Backend.cpp`, `WW3D2/StubD3D8Device.cpp`, `GeneralsMD/.../WW3D2/sortingrenderer.cpp`. Nothing new needs adding to CMake from this group.
- Android needs both essl and spirv shader headers (`*_essl.bin.h` and `*_spirv.bin.h`) for the full trunk shader set: uber_instanced/array/frameconst, bloom, ssao, copy, shadow_caster(+instanced), scene_depth_instanced and the rest. cmake/bgfx.cmake already adds the spirv variant for ANDROID. Check it covers every shader in trunk's list.
- macOS GLSL (`GGC_BGFX_RENDERER=glsl`) likewise needs `*_glsl.bin.h` for the full trunk shader set.
- **Core/Libraries/Source/WWVegas/WW3D2/CMakeLists.txt (not my group):** `elseif(UNIX AND NOT APPLE)` requires Freetype + Fontconfig. Android (and possibly Switch) now use stb_truetype and have no Fontconfig. Exclude `ANDROID` and `CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch"` there, or the configure will fail. The stb include-dir propagation at line ~432 (`NOT WIN32 OR WindowsStore`) already covers them.
- New public symbols in BgfxBackend.cpp: `void GGC_BgfxSetRenderScale(float)` and `float GGC_BgfxGetRenderScale()` (C++ linkage, matching the `extern` declarations in W3DDisplay.cpp and OptionsMenu.cpp). The `g_ggc*` counters exist only under `__ANDROID__`.
- `GGC_BGFX_FORCE_GLES` is read with raw `std::getenv` (once, at init); it is not registered in GgcRuntimeFlags.

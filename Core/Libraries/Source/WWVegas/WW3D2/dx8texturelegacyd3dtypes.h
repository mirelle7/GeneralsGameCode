/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#if defined(GGC_RENDER_BACKEND_BGFX)
#error dx8texturelegacyd3dtypes.h is only for the native DX8 texture compatibility path.
#endif

#include "d3d8.h"

typedef IDirect3DBaseTexture8 LegacyBaseTexture;
typedef IDirect3DSurface8 LegacySurface;
typedef IDirect3DSurface8 NativeCompatibilityTextureSurface;
typedef D3DSURFACE_DESC LegacySurfaceDesc;
typedef D3DVOLUME_DESC LegacyVolumeDesc;
typedef D3DLOCKED_RECT LegacyLockedRect;

typedef IDirect3DTexture8 LegacyLoaderTexture;
typedef IDirect3DSurface8 LegacyLoaderSurface;
typedef IDirect3DCubeTexture8 LegacyLoaderCubeTexture;
typedef IDirect3DVolumeTexture8 LegacyLoaderVolumeTexture;
typedef D3DLOCKED_RECT LegacyLoaderLockedRect;
typedef D3DLOCKED_BOX LegacyLoaderLockedBox;
typedef D3DCUBEMAP_FACES LegacyLoaderCubeFace;

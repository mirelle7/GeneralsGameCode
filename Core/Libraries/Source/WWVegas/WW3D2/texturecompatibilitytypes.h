/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 TheSuperHackers
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

#if !defined(GGC_RENDER_BACKEND_BGFX)
#error texturecompatibilitytypes.h is only for the standalone bgfx compatibility build.
#endif

#include "WWLib/win.h"

struct NativeCompatibilitySurfaceDesc
{
	NativeCompatibilitySurfaceDesc() : Format(0), Type(0), Usage(0), Pool(0), Size(0), MultiSampleType(0), Width(0), Height(0) {}
	unsigned int Format;
	unsigned int Type;
	unsigned int Usage;
	unsigned int Pool;
	unsigned int Size;
	unsigned int MultiSampleType;
	unsigned int Width;
	unsigned int Height;
};

struct NativeCompatibilityVolumeDesc
{
	NativeCompatibilityVolumeDesc() : Format(0), Type(0), Usage(0), Pool(0), Size(0), Width(0), Height(0), Depth(0) {}
	unsigned int Format;
	unsigned int Type;
	unsigned int Usage;
	unsigned int Pool;
	unsigned int Size;
	unsigned int Width;
	unsigned int Height;
	unsigned int Depth;
};

struct NativeCompatibilityLockedRect
{
	NativeCompatibilityLockedRect() : Pitch(0), pBits(nullptr) {}
	int Pitch;
	void *pBits;
};

struct NativeCompatibilityLockedBox
{
	NativeCompatibilityLockedBox() : RowPitch(0), SlicePitch(0), pBits(nullptr) {}
	int RowPitch;
	int SlicePitch;
	void *pBits;
};

enum NativeCompatibilityCubeFace
{
	NATIVE_COMPATIBILITY_CUBE_FACE_POSITIVE_X = 0,
	NATIVE_COMPATIBILITY_CUBE_FACE_NEGATIVE_X = 1,
	NATIVE_COMPATIBILITY_CUBE_FACE_POSITIVE_Y = 2,
	NATIVE_COMPATIBILITY_CUBE_FACE_NEGATIVE_Y = 3,
	NATIVE_COMPATIBILITY_CUBE_FACE_POSITIVE_Z = 4,
	NATIVE_COMPATIBILITY_CUBE_FACE_NEGATIVE_Z = 5,
};

struct NativeCompatibilityBaseTexture
{
	ULONG AddRef() { return 0; }
	ULONG Release() { return 0; }
	unsigned int GetLevelCount() { return 0; }
};

struct NativeCompatibilitySurface
{
	ULONG AddRef() { return 0; }
	ULONG Release() { return 0; }
	HRESULT GetDesc(NativeCompatibilitySurfaceDesc *desc)
	{
		if (desc != nullptr) {
			*desc = NativeCompatibilitySurfaceDesc();
		}
		return E_FAIL;
	}
	HRESULT LockRect(NativeCompatibilityLockedRect *locked_rect, const RECT *rect, DWORD flags)
	{
		(void)rect;
		(void)flags;
		if (locked_rect != nullptr) {
			*locked_rect = NativeCompatibilityLockedRect();
		}
		return E_FAIL;
	}
	HRESULT UnlockRect() { return E_FAIL; }
};

struct NativeCompatibilityTexture2D : NativeCompatibilityBaseTexture
{
	HRESULT GetSurfaceLevel(UINT level, NativeCompatibilitySurface **surface)
	{
		(void)level;
		if (surface != nullptr) {
			*surface = nullptr;
		}
		return E_FAIL;
	}
	HRESULT GetLevelDesc(UINT level, NativeCompatibilitySurfaceDesc *desc)
	{
		(void)level;
		if (desc != nullptr) {
			*desc = NativeCompatibilitySurfaceDesc();
		}
		return E_FAIL;
	}
	HRESULT LockRect(UINT level, NativeCompatibilityLockedRect *locked_rect, const RECT *rect, DWORD flags)
	{
		(void)level;
		(void)rect;
		(void)flags;
		if (locked_rect != nullptr) {
			*locked_rect = NativeCompatibilityLockedRect();
		}
		return E_FAIL;
	}
	HRESULT UnlockRect(UINT level)
	{
		(void)level;
		return E_FAIL;
	}
};

struct NativeCompatibilityCubeTexture : NativeCompatibilityBaseTexture
{
	HRESULT GetLevelDesc(UINT level, NativeCompatibilitySurfaceDesc *desc)
	{
		(void)level;
		if (desc != nullptr) {
			*desc = NativeCompatibilitySurfaceDesc();
		}
		return E_FAIL;
	}
	HRESULT LockRect(
		NativeCompatibilityCubeFace face,
		UINT level,
		NativeCompatibilityLockedRect *locked_rect,
		const RECT *rect,
		DWORD flags)
	{
		(void)face;
		(void)level;
		(void)rect;
		(void)flags;
		if (locked_rect != nullptr) {
			*locked_rect = NativeCompatibilityLockedRect();
		}
		return E_FAIL;
	}
	HRESULT UnlockRect(NativeCompatibilityCubeFace face, UINT level)
	{
		(void)face;
		(void)level;
		return E_FAIL;
	}
};

struct NativeCompatibilityVolumeTexture : NativeCompatibilityBaseTexture
{
	HRESULT GetLevelDesc(UINT level, NativeCompatibilityVolumeDesc *desc)
	{
		(void)level;
		if (desc != nullptr) {
			*desc = NativeCompatibilityVolumeDesc();
		}
		return E_FAIL;
	}
	HRESULT LockBox(UINT level, NativeCompatibilityLockedBox *locked_box, const void *box, DWORD flags)
	{
		(void)level;
		(void)box;
		(void)flags;
		if (locked_box != nullptr) {
			*locked_box = NativeCompatibilityLockedBox();
		}
		return E_FAIL;
	}
	HRESULT UnlockBox(UINT level)
	{
		(void)level;
		return E_FAIL;
	}
};

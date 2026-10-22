/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2026 TheSuperHackers
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

// TheSuperHackers @build bobtista 22/04/2026
// Minimal in-tree implementations of the D3DX8 helper functions the
// engine actually calls. There is no d3dx8.lib off Windows, so those builds
// compile this file instead.
//
// Not implemented here:
// - D3DXMatrixIdentity / D3DXMatrixIsIdentity — these are D3DXINLINE
//   in d3dx8math.inl, so every translation unit already has its own
//   copy; redefining them in a .cpp produces a type-modifier mismatch.
// - Non-critical functions (shader assemble from file/resource, mesh
//   loaders, font create, etc.) that aren't called at runtime.
// The texture, surface and shader helpers need a device and only report failure.

#if !defined(_WIN32)

#include <cmath>
#include <cstring>

#include <d3d8.h>
#include <d3dx8.h>

// -----------------------------------------------------------------------------
// Matrix helpers (the non-inline ones; D3DXMatrixIdentity is inlined in
// d3dx8math.inl).
// -----------------------------------------------------------------------------

extern "C" D3DXMATRIX * WINAPI D3DXMatrixTranspose(D3DXMATRIX * out, CONST D3DXMATRIX * in)
{
	if (out == nullptr || in == nullptr)
	{
		return out;
	}
	D3DXMATRIX tmp;
	for (int r = 0; r < 4; ++r)
	{
		for (int c = 0; c < 4; ++c)
		{
			tmp.m[c][r] = in->m[r][c];
		}
	}
	*out = tmp;
	return out;
}

extern "C" D3DXMATRIX * WINAPI D3DXMatrixMultiply(D3DXMATRIX * out, CONST D3DXMATRIX * a, CONST D3DXMATRIX * b)
{
	if (out == nullptr || a == nullptr || b == nullptr)
	{
		return out;
	}
	D3DXMATRIX tmp;
	for (int r = 0; r < 4; ++r)
	{
		for (int c = 0; c < 4; ++c)
		{
			tmp.m[r][c] =
				a->m[r][0] * b->m[0][c] +
				a->m[r][1] * b->m[1][c] +
				a->m[r][2] * b->m[2][c] +
				a->m[r][3] * b->m[3][c];
		}
	}
	*out = tmp;
	return out;
}

extern "C" D3DXMATRIX * WINAPI D3DXMatrixScaling(D3DXMATRIX * out, FLOAT sx, FLOAT sy, FLOAT sz)
{
	if (out == nullptr)
	{
		return nullptr;
	}
	std::memset(out, 0, sizeof(*out));
	out->_11 = sx;
	out->_22 = sy;
	out->_33 = sz;
	out->_44 = 1.0f;
	return out;
}

extern "C" D3DXMATRIX * WINAPI D3DXMatrixTranslation(D3DXMATRIX * out, FLOAT x, FLOAT y, FLOAT z)
{
	if (out == nullptr)
	{
		return nullptr;
	}
	std::memset(out, 0, sizeof(*out));
	out->_11 = out->_22 = out->_33 = out->_44 = 1.0f;
	out->_41 = x;
	out->_42 = y;
	out->_43 = z;
	return out;
}

extern "C" D3DXMATRIX * WINAPI D3DXMatrixRotationZ(D3DXMATRIX * out, FLOAT angle)
{
	if (out == nullptr)
	{
		return nullptr;
	}
	std::memset(out, 0, sizeof(*out));
	const FLOAT c = std::cos(angle);
	const FLOAT s = std::sin(angle);
	out->_11 =  c;  out->_12 =  s;
	out->_21 = -s;  out->_22 =  c;
	out->_33 = 1.0f;
	out->_44 = 1.0f;
	return out;
}

// 4x4 inverse via cofactor expansion. Returns out on success, nullptr
// when the matrix is singular. D3DX writes the determinant into
// pDeterminant when non-null.
extern "C" D3DXMATRIX * WINAPI D3DXMatrixInverse(D3DXMATRIX * out, FLOAT * det_out, CONST D3DXMATRIX * in)
{
	if (out == nullptr || in == nullptr)
	{
		return nullptr;
	}
	const FLOAT * m = &in->m[0][0];
	FLOAT inv[16];
	inv[ 0] =  m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15]
	         + m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
	inv[ 4] = -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15]
	         - m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
	inv[ 8] =  m[4]*m[ 9]*m[15] - m[4]*m[11]*m[13] - m[8]*m[5]*m[15]
	         + m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[ 9];
	inv[12] = -m[4]*m[ 9]*m[14] + m[4]*m[10]*m[13] + m[8]*m[5]*m[14]
	         - m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[ 9];
	inv[ 1] = -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15]
	         - m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
	inv[ 5] =  m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15]
	         + m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
	inv[ 9] = -m[0]*m[ 9]*m[15] + m[0]*m[11]*m[13] + m[8]*m[1]*m[15]
	         - m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[ 9];
	inv[13] =  m[0]*m[ 9]*m[14] - m[0]*m[10]*m[13] - m[8]*m[1]*m[14]
	         + m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[ 9];
	inv[ 2] =  m[1]*m[ 6]*m[15] - m[1]*m[ 7]*m[14] - m[5]*m[2]*m[15]
	         + m[5]*m[3]*m[14] + m[13]*m[2]*m[ 7] - m[13]*m[3]*m[ 6];
	inv[ 6] = -m[0]*m[ 6]*m[15] + m[0]*m[ 7]*m[14] + m[4]*m[2]*m[15]
	         - m[4]*m[3]*m[14] - m[12]*m[2]*m[ 7] + m[12]*m[3]*m[ 6];
	inv[10] =  m[0]*m[ 5]*m[15] - m[0]*m[ 7]*m[13] - m[4]*m[1]*m[15]
	         + m[4]*m[3]*m[13] + m[12]*m[1]*m[ 7] - m[12]*m[3]*m[ 5];
	inv[14] = -m[0]*m[ 5]*m[14] + m[0]*m[ 6]*m[13] + m[4]*m[1]*m[14]
	         - m[4]*m[2]*m[13] - m[12]*m[1]*m[ 6] + m[12]*m[2]*m[ 5];
	inv[ 3] = -m[1]*m[ 6]*m[11] + m[1]*m[ 7]*m[10] + m[5]*m[2]*m[11]
	         - m[5]*m[3]*m[10] - m[ 9]*m[2]*m[ 7] + m[ 9]*m[3]*m[ 6];
	inv[ 7] =  m[0]*m[ 6]*m[11] - m[0]*m[ 7]*m[10] - m[4]*m[2]*m[11]
	         + m[4]*m[3]*m[10] + m[ 8]*m[2]*m[ 7] - m[ 8]*m[3]*m[ 6];
	inv[11] = -m[0]*m[ 5]*m[11] + m[0]*m[ 7]*m[ 9] + m[4]*m[1]*m[11]
	         - m[4]*m[3]*m[ 9] - m[ 8]*m[1]*m[ 7] + m[ 8]*m[3]*m[ 5];
	inv[15] =  m[0]*m[ 5]*m[10] - m[0]*m[ 6]*m[ 9] - m[4]*m[1]*m[10]
	         + m[4]*m[2]*m[ 9] + m[ 8]*m[1]*m[ 6] - m[ 8]*m[2]*m[ 5];

	FLOAT d = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
	if (det_out != nullptr)
	{
		*det_out = d;
	}
	if (d == 0.0f)
	{
		return nullptr;
	}
	const FLOAT inv_d = 1.0f / d;
	FLOAT * out_m = &out->m[0][0];
	for (int i = 0; i < 16; ++i)
	{
		out_m[i] = inv[i] * inv_d;
	}
	return out;
}

// -----------------------------------------------------------------------------
// Vector transforms — full 4x4 multiply. D3DXVec3Transform expands v to
// (x,y,z,1) and writes a Vec4; D3DXVec4Transform is a straight Vec4
// times matrix. Both callers use these for picking/bezier math.
// -----------------------------------------------------------------------------

extern "C" D3DXVECTOR4 * WINAPI D3DXVec3Transform(D3DXVECTOR4 * out, CONST D3DXVECTOR3 * v, CONST D3DXMATRIX * m)
{
	if (out == nullptr || v == nullptr || m == nullptr)
	{
		return out;
	}
	D3DXVECTOR4 tmp;
	tmp.x = v->x * m->_11 + v->y * m->_21 + v->z * m->_31 + m->_41;
	tmp.y = v->x * m->_12 + v->y * m->_22 + v->z * m->_32 + m->_42;
	tmp.z = v->x * m->_13 + v->y * m->_23 + v->z * m->_33 + m->_43;
	tmp.w = v->x * m->_14 + v->y * m->_24 + v->z * m->_34 + m->_44;
	*out = tmp;
	return out;
}

extern "C" D3DXVECTOR4 * WINAPI D3DXVec4Transform(D3DXVECTOR4 * out, CONST D3DXVECTOR4 * v, CONST D3DXMATRIX * m)
{
	if (out == nullptr || v == nullptr || m == nullptr)
	{
		return out;
	}
	D3DXVECTOR4 tmp;
	tmp.x = v->x * m->_11 + v->y * m->_21 + v->z * m->_31 + v->w * m->_41;
	tmp.y = v->x * m->_12 + v->y * m->_22 + v->z * m->_32 + v->w * m->_42;
	tmp.z = v->x * m->_13 + v->y * m->_23 + v->z * m->_33 + v->w * m->_43;
	tmp.w = v->x * m->_14 + v->y * m->_24 + v->z * m->_34 + v->w * m->_44;
	*out = tmp;
	return out;
}

// -----------------------------------------------------------------------------
// FVF helpers
// -----------------------------------------------------------------------------

extern "C" UINT WINAPI D3DXGetFVFVertexSize(DWORD fvf)
{
	UINT size = 0;
	switch (fvf & D3DFVF_POSITION_MASK)
	{
	case D3DFVF_XYZ:    size += 3 * sizeof(float); break;
	case D3DFVF_XYZRHW: size += 4 * sizeof(float); break;
	case D3DFVF_XYZB1:  size += 3 * sizeof(float) + 1 * sizeof(float); break;
	case D3DFVF_XYZB2:  size += 3 * sizeof(float) + 2 * sizeof(float); break;
	case D3DFVF_XYZB3:  size += 3 * sizeof(float) + 3 * sizeof(float); break;
	case D3DFVF_XYZB4:  size += 3 * sizeof(float) + 4 * sizeof(float); break;
	case D3DFVF_XYZB5:  size += 3 * sizeof(float) + 5 * sizeof(float); break;
	default: break;
	}
	if (fvf & D3DFVF_NORMAL)
	{
		size += 3 * sizeof(float);
	}
	if (fvf & D3DFVF_PSIZE)
	{
		size += sizeof(float);
	}
	if (fvf & D3DFVF_DIFFUSE)
	{
		size += sizeof(DWORD);
	}
	if (fvf & D3DFVF_SPECULAR)
	{
		size += sizeof(DWORD);
	}
	const DWORD tex_count = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
	// Default 2 floats per set — D3DFVF_TEXCOORDSIZE1..4 overrides aren't
	// used anywhere in the engine.
	size += tex_count * 2 * sizeof(float);
	return size;
}

// -----------------------------------------------------------------------------
// Error strings — minimal static table. Real D3DX has a big message pool
// keyed by HRESULT; we just need enough for debug logs.
// -----------------------------------------------------------------------------

extern "C" HRESULT WINAPI D3DXGetErrorStringA(HRESULT hr, LPSTR pBuffer, UINT BufferLen)
{
	if (pBuffer == nullptr || BufferLen == 0)
	{
		return D3D_OK;
	}
	const char * msg;
	switch (static_cast<UINT>(hr))
	{
	case (UINT)D3D_OK:                   msg = "D3D_OK"; break;
	case (UINT)D3DERR_INVALIDCALL:       msg = "D3DERR_INVALIDCALL"; break;
	case (UINT)D3DERR_NOTAVAILABLE:      msg = "D3DERR_NOTAVAILABLE"; break;
	case (UINT)D3DERR_OUTOFVIDEOMEMORY:  msg = "D3DERR_OUTOFVIDEOMEMORY"; break;
	case (UINT)E_OUTOFMEMORY:            msg = "E_OUTOFMEMORY"; break;
	case (UINT)E_NOTIMPL:                msg = "E_NOTIMPL"; break;
	case (UINT)E_FAIL:                   msg = "E_FAIL"; break;
	default:                              msg = "D3DX unknown error"; break;
	}
	std::strncpy(pBuffer, msg, BufferLen - 1);
	pBuffer[BufferLen - 1] = '\0';
	return D3D_OK;
}

// -----------------------------------------------------------------------------
// Texture, surface and shader helpers. These need a Direct3D device, which only
// exists on Windows, so off Windows they report that they are not available.
// -----------------------------------------------------------------------------

extern "C" HRESULT WINAPI D3DXCreateTexture(LPDIRECT3DDEVICE8, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL,
	LPDIRECT3DTEXTURE8 * out_texture)
{
	if (out_texture) *out_texture = nullptr;
	return E_NOTIMPL;
}

extern "C" HRESULT WINAPI D3DXCreateCubeTexture(LPDIRECT3DDEVICE8, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL,
	LPDIRECT3DCUBETEXTURE8 * out_texture)
{
	if (out_texture) *out_texture = nullptr;
	return E_NOTIMPL;
}

extern "C" HRESULT WINAPI D3DXCreateVolumeTexture(LPDIRECT3DDEVICE8, UINT, UINT, UINT, UINT, DWORD, D3DFORMAT,
	D3DPOOL, LPDIRECT3DVOLUMETEXTURE8 * out_texture)
{
	if (out_texture) *out_texture = nullptr;
	return E_NOTIMPL;
}

extern "C" HRESULT WINAPI D3DXFilterTexture(LPDIRECT3DBASETEXTURE8, CONST PALETTEENTRY *, UINT, DWORD)
{
	return E_NOTIMPL;
}

extern "C" HRESULT WINAPI D3DXLoadSurfaceFromSurface(LPDIRECT3DSURFACE8, CONST PALETTEENTRY *, CONST RECT *,
	LPDIRECT3DSURFACE8, CONST PALETTEENTRY *, CONST RECT *, DWORD, D3DCOLOR)
{
	return E_NOTIMPL;
}

extern "C" HRESULT WINAPI D3DXCreateTextureFromFileExA(LPDIRECT3DDEVICE8, LPCSTR, UINT, UINT, UINT, DWORD,
	D3DFORMAT, D3DPOOL, DWORD, DWORD, D3DCOLOR, D3DXIMAGE_INFO *, PALETTEENTRY *, LPDIRECT3DTEXTURE8 * out_texture)
{
	if (out_texture) *out_texture = nullptr;
	return E_NOTIMPL;
}

extern "C" HRESULT WINAPI D3DXAssembleShader(LPCVOID, UINT, DWORD, LPD3DXBUFFER * constants,
	LPD3DXBUFFER * compiled_shader, LPD3DXBUFFER * errors)
{
	if (constants) *constants = nullptr;
	if (compiled_shader) *compiled_shader = nullptr;
	if (errors) *errors = nullptr;
	return E_NOTIMPL;
}

#endif // !_WIN32

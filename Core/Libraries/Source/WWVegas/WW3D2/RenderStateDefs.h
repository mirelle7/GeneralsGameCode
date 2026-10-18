/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 TheSuperHackers
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
*/

#pragma once

// Render state indices for FixedFunctionState::Cached/Set_Cached_Render_State.
// Values match legacy render state ordinals so the DX8 backend can use the same cache.
namespace RS {
const unsigned ZENABLE = 7;
const unsigned FILLMODE = 8;
const unsigned SHADEMODE = 9;
const unsigned ZWRITEENABLE = 14;
const unsigned ALPHATESTENABLE = 15;
const unsigned SRCBLEND = 19;
const unsigned DESTBLEND = 20;
const unsigned CULLMODE = 22;
const unsigned ZFUNC = 23;
const unsigned ALPHAREF = 24;
const unsigned ALPHAFUNC = 25;
const unsigned ALPHABLENDENABLE = 27;
const unsigned FOGENABLE = 28;
const unsigned SPECULARENABLE = 29;
const unsigned FOGCOLOR = 34;
const unsigned ZBIAS = 47;
const unsigned STENCILENABLE = 52;
const unsigned STENCILFAIL = 53;
const unsigned STENCILZFAIL = 54;
const unsigned STENCILPASS = 55;
const unsigned STENCILFUNC = 56;
const unsigned STENCILREF = 57;
const unsigned STENCILMASK = 58;
const unsigned STENCILWRITEMASK = 59;
const unsigned TEXTUREFACTOR = 60;
const unsigned LIGHTING = 137;
const unsigned AMBIENT = 139;
const unsigned NORMALIZENORMALS = 143;
const unsigned DIFFUSEMATERIALSOURCE = 145;
const unsigned AMBIENTMATERIALSOURCE = 147;
const unsigned EMISSIVEMATERIALSOURCE = 148;
const unsigned POINTSIZE = 154;
const unsigned POINTSIZEMIN = 155;
const unsigned POINTSPRITEENABLE = 156;
const unsigned POINTSCALEENABLE = 157;
const unsigned POINTSCALE_A = 158;
const unsigned POINTSCALE_B = 159;
const unsigned POINTSCALE_C = 160;
const unsigned PATCHSEGMENTS = 164;
const unsigned POINTSIZEMAX = 166;
const unsigned COLORWRITEENABLE = 168;
const unsigned BLENDOP = 171;
} // namespace RS

// Texture stage state indices for FixedFunctionState::Cached/Set_Cached_Texture_Stage_State.
// Values match legacy texture stage state ordinals so the DX8 backend can use the same cache.
namespace TSS {
const unsigned COLOROP = 1;
const unsigned COLORARG1 = 2;
const unsigned COLORARG2 = 3;
const unsigned ALPHAOP = 4;
const unsigned ALPHAARG1 = 5;
const unsigned ALPHAARG2 = 6;
const unsigned BUMPENVMAT00 = 7;
const unsigned BUMPENVMAT01 = 8;
const unsigned BUMPENVMAT10 = 9;
const unsigned BUMPENVMAT11 = 10;
const unsigned TEXCOORDINDEX = 11;
const unsigned ADDRESSU = 13;
const unsigned ADDRESSV = 14;
const unsigned MAGFILTER = 16;
const unsigned MINFILTER = 17;
const unsigned MIPFILTER = 18;
const unsigned MAXANISOTROPY = 21;
const unsigned BUMPENVLSCALE = 22;
const unsigned BUMPENVLOFFSET = 23;
const unsigned TEXTURETRANSFORMFLAGS = 24;
const unsigned ADDRESSW = 25;
const unsigned COLORARG0 = 26;
const unsigned ALPHAARG0 = 27;
} // namespace TSS

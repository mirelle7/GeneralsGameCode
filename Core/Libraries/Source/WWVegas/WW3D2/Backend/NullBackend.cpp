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

#include "Backend/NullBackend.h"
#include "BgfxRenderProfile.h"

// Globals and hooks that BgfxBackend.cpp defines and the shared non-DX8 code paths reference.
// -headless sets DX8Wrapper_IsWindowed to false, as with bgfx.
bool DX8Wrapper_IsWindowed = true;
int DX8Wrapper_PreserveFPU = 0;

extern "C" int GGC_GetBgfxSunShadowCullBox(float * /*center3*/, float * /*radius*/)
{
    return 0;
}

extern "C" void GGC_RequestBgfxFramebufferRebuild()
{
}

void GGC_BgfxSetRenderScale(float /*scale*/)
{
}

namespace GGCRenderProfile
{
    void Begin(Phase /*phase*/) {}
    void End(Phase /*phase*/) {}
    void EndFrame() {}
}

const RenderDeviceDescClass & NullBackend::Get_Render_Device_Desc(int /*deviceidx*/)
{
    return m_renderDeviceDesc;
}

const Vector3 & NullBackend::Get_Ambient() const
{
    return m_ambient;
}

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

// TheSuperHackers @feature githubawn 21/10/2026 NullBackend.
// IRenderBackend that draws nothing, for GGC_RENDER_BACKEND=null builds. Such
// builds link no graphics library and are meant for -headless runs, which never
// construct a backend. Every method keeps the IRenderBackend default.

#pragma once

#include "IRenderBackend.h"
#include "rddesc.h"
#include "WWMath/vector3.h"

class NullBackend : public IRenderBackend
{
public:
    NullBackend() : m_ambient(0.0f, 0.0f, 0.0f) {}

    virtual const RenderDeviceDescClass & Get_Render_Device_Desc(int deviceidx) override;
    virtual const Vector3 & Get_Ambient() const override;

private:
    RenderDeviceDescClass m_renderDeviceDesc;
    Vector3 m_ambient;
};

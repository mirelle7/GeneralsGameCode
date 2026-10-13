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

// ImGuiRenderer.h
// TheSuperHackers @feature Draws Dear ImGui through the engine's render backend, the same path
// Render2DClass uses, so it works on both the DX8 and the bgfx backend without a dedicated ImGui
// renderer per graphics API. Clip rectangles are applied on the CPU because neither backend
// exposes a per draw scissor rectangle.

#pragma once

struct ImDrawData;
class TextureClass;

class ImGuiRenderer
{
public:
	ImGuiRenderer();
	~ImGuiRenderer();

	/// Builds the font atlas texture. Requires an ImGui context and an initialized WW3D.
	bool init();
	void shutdown();

	/// Renders ImGui draw data. Must be called between WW3D::Begin_Render and End_Render.
	void render(ImDrawData *drawData);

private:
	TextureClass *m_fontTexture;
};

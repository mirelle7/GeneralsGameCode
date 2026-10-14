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

// ImGuiRenderer.cpp
// TheSuperHackers @feature Dear ImGui renderer on top of the engine render backend.

#include "Editor/ImGuiRenderer.h"

#include "imgui.h"

#include "Common/Debug.h"
#include "WW3D2/ww3d.h"
#include "WW3D2/texture.h"
#include "WW3D2/surfaceclass.h"
#include "WW3D2/vertexbuffer.h"
#include "WW3D2/indexbuffer.h"
#include "WW3D2/dx8fvf.h"
#include "WW3D2/render2d.h"
#include "WW3D2/vertmaterial.h"
#include "WW3D2/IRenderBackend.h"
#include "WW3D2/RenderBackend.h"
#include "WWMath/matrix4.h"

#include <algorithm>
#include <vector>

namespace
{
	// Vertices per backend draw. Kept well below the 16 bit index limit of the dynamic buffers.
	const int kMaxBatchVertices = 6144;

	struct ClipVertex
	{
		float x, y;
		float u, v;
		float r, g, b, a;
	};

	struct ClipRect
	{
		float minX, minY, maxX, maxY;
	};

	ClipVertex toClipVertex(const ImDrawVert &v)
	{
		ClipVertex out;
		out.x = v.pos.x;
		out.y = v.pos.y;
		out.u = v.uv.x;
		out.v = v.uv.y;
		out.r = (float)((v.col >> IM_COL32_R_SHIFT) & 0xFF);
		out.g = (float)((v.col >> IM_COL32_G_SHIFT) & 0xFF);
		out.b = (float)((v.col >> IM_COL32_B_SHIFT) & 0xFF);
		out.a = (float)((v.col >> IM_COL32_A_SHIFT) & 0xFF);
		return out;
	}

	ClipVertex lerp(const ClipVertex &a, const ClipVertex &b, float t)
	{
		ClipVertex out;
		out.x = a.x + (b.x - a.x) * t;
		out.y = a.y + (b.y - a.y) * t;
		out.u = a.u + (b.u - a.u) * t;
		out.v = a.v + (b.v - a.v) * t;
		out.r = a.r + (b.r - a.r) * t;
		out.g = a.g + (b.g - a.g) * t;
		out.b = a.b + (b.b - a.b) * t;
		out.a = a.a + (b.a - a.a) * t;
		return out;
	}

	// Sutherland-Hodgman against one axis aligned edge. axis 0 is x, 1 is y. keepGreater keeps the
	// side with coordinates >= limit.
	int clipPolygonEdge(const ClipVertex *in, int count, ClipVertex *out, int axis, float limit, bool keepGreater)
	{
		int outCount = 0;
		for (int i = 0; i < count; ++i)
		{
			const ClipVertex &cur = in[i];
			const ClipVertex &next = in[(i + 1) % count];
			const float curValue = axis == 0 ? cur.x : cur.y;
			const float nextValue = axis == 0 ? next.x : next.y;
			const bool curInside = keepGreater ? curValue >= limit : curValue <= limit;
			const bool nextInside = keepGreater ? nextValue >= limit : nextValue <= limit;

			if (curInside)
				out[outCount++] = cur;

			if (curInside != nextInside)
			{
				const float t = (limit - curValue) / (nextValue - curValue);
				out[outCount++] = lerp(cur, next, t);
			}
		}
		return outCount;
	}

	unsigned int toDiffuse(const ClipVertex &v)
	{
		// ImGui packs ABGR, the engine expects ARGB.
		const unsigned int r = (unsigned int)(v.r + 0.5f);
		const unsigned int g = (unsigned int)(v.g + 0.5f);
		const unsigned int b = (unsigned int)(v.b + 0.5f);
		const unsigned int a = (unsigned int)(v.a + 0.5f);
		return (a << 24) | (r << 16) | (g << 8) | b;
	}

	class Batch
	{
	public:
		Batch() : m_texture(nullptr), m_screenWidth(1.0f), m_screenHeight(1.0f), m_biasX(0.0f), m_biasY(0.0f)
		{
			m_vertices.reserve(kMaxBatchVertices);
		}

		void begin(float screenWidth, float screenHeight)
		{
			m_screenWidth = screenWidth;
			m_screenHeight = screenHeight;
			m_biasX = 0.0f;
			m_biasY = 0.0f;
			if (WW3D::Is_Screen_UV_Biased())
			{
				// Same half pixel offset Render2DClass applies on DX8.
				m_biasX = -0.5f;
				m_biasY = -0.5f;
			}
		}

		void setTexture(TextureClass *texture)
		{
			if (texture != m_texture)
			{
				flush();
				m_texture = texture;
			}
		}

		void addTriangle(const ClipVertex &a, const ClipVertex &b, const ClipVertex &c)
		{
			if ((int)m_vertices.size() + 3 > kMaxBatchVertices)
				flush();

			m_vertices.push_back(a);
			m_vertices.push_back(b);
			m_vertices.push_back(c);
		}

		void flush()
		{
			const int vertexCount = (int)m_vertices.size();
			if (vertexCount == 0)
				return;

			IRenderBackend *backend = WW3D::Get_Render_Backend();

			DynamicVBAccessClass vb(BUFFER_TYPE_DYNAMIC, dynamic_fvf_type, (unsigned short)vertexCount);
			{
				DynamicVBAccessClass::WriteLockClass lock(&vb);
				VertexFormatXYZNDUV2 *dst = lock.Get_Formatted_Vertex_Array();
				for (int i = 0; i < vertexCount; ++i)
				{
					const ClipVertex &src = m_vertices[i];
					dst[i].x = ((src.x + m_biasX) / m_screenWidth) * 2.0f - 1.0f;
					dst[i].y = 1.0f - ((src.y + m_biasY) / m_screenHeight) * 2.0f;
					dst[i].z = 0.0f;
					dst[i].nx = 0.0f;
					dst[i].ny = 0.0f;
					dst[i].nz = 1.0f;
					dst[i].diffuse = toDiffuse(src);
					dst[i].u1 = src.u;
					dst[i].v1 = src.v;
					dst[i].u2 = 0.0f;
					dst[i].v2 = 0.0f;
				}
			}

			DynamicIBAccessClass ib(BUFFER_TYPE_DYNAMIC, (unsigned short)vertexCount);
			{
				DynamicIBAccessClass::WriteLockClass lock(&ib);
				unsigned short *indices = lock.Get_Index_Array();
				for (int i = 0; i < vertexCount; ++i)
					indices[i] = (unsigned short)i;
			}

			backend->Set_Texture(0, m_texture);
			backend->Set_Vertex_Buffer(vb);
			backend->Set_Index_Buffer(ib, 0);
			// ImGui emits both windings and the conversion to clip space flips Y, so draw two sided.
			ShaderClass shader = Render2DClass::Get_Default_Shader();
			shader.Set_Cull_Mode(ShaderClass::CULL_MODE_DISABLE);
			backend->Set_Shader(shader);
			backend->Draw_Triangles(0, (unsigned short)(vertexCount / 3), 0, (unsigned short)vertexCount);

			m_vertices.clear();
		}

	private:
		std::vector<ClipVertex> m_vertices;
		TextureClass *m_texture;
		float m_screenWidth;
		float m_screenHeight;
		float m_biasX;
		float m_biasY;
	};

	void emitClippedTriangle(Batch &batch, const ClipVertex &a, const ClipVertex &b, const ClipVertex &c, const ClipRect &clip)
	{
		const float minX = std::min(a.x, std::min(b.x, c.x));
		const float maxX = std::max(a.x, std::max(b.x, c.x));
		const float minY = std::min(a.y, std::min(b.y, c.y));
		const float maxY = std::max(a.y, std::max(b.y, c.y));

		// Fully outside.
		if (maxX <= clip.minX || minX >= clip.maxX || maxY <= clip.minY || minY >= clip.maxY)
			return;

		// Fully inside, the common case.
		if (minX >= clip.minX && maxX <= clip.maxX && minY >= clip.minY && maxY <= clip.maxY)
		{
			batch.addTriangle(a, b, c);
			return;
		}

		// A triangle clipped by four edges has at most seven corners.
		ClipVertex polyA[8];
		ClipVertex polyB[8];
		polyA[0] = a;
		polyA[1] = b;
		polyA[2] = c;
		int count = 3;
		count = clipPolygonEdge(polyA, count, polyB, 0, clip.minX, true);
		count = clipPolygonEdge(polyB, count, polyA, 0, clip.maxX, false);
		count = clipPolygonEdge(polyA, count, polyB, 1, clip.minY, true);
		count = clipPolygonEdge(polyB, count, polyA, 1, clip.maxY, false);

		for (int i = 1; i + 1 < count; ++i)
			batch.addTriangle(polyA[0], polyA[i], polyA[i + 1]);
	}
}

ImGuiRenderer::ImGuiRenderer() :
	m_fontTexture(nullptr)
{
}

ImGuiRenderer::~ImGuiRenderer()
{
	shutdown();
}

bool ImGuiRenderer::init()
{
	ImGuiIO &io = ImGui::GetIO();
	io.BackendRendererName = "ww3d_render_backend";

	unsigned char *pixels = nullptr;
	int width = 0;
	int height = 0;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	if (pixels == nullptr || width <= 0 || height <= 0)
		return false;

	SurfaceClass *surface = NEW_REF(SurfaceClass, ((unsigned)width, (unsigned)height, WW3D_FORMAT_A8R8G8B8));
	int pitch = 0;
	unsigned char *bits = (unsigned char *)surface->Lock(&pitch);
	if (bits == nullptr)
	{
		REF_PTR_RELEASE(surface);
		return false;
	}

	for (int y = 0; y < height; ++y)
	{
		const unsigned char *src = pixels + y * width * 4;
		unsigned int *dst = (unsigned int *)(bits + y * pitch);
		for (int x = 0; x < width; ++x)
		{
			const unsigned int r = src[x * 4 + 0];
			const unsigned int g = src[x * 4 + 1];
			const unsigned int b = src[x * 4 + 2];
			const unsigned int a = src[x * 4 + 3];
			dst[x] = (a << 24) | (r << 16) | (g << 8) | b;
		}
	}
	surface->Unlock();

	m_fontTexture = MSGNEW("TextureClass") TextureClass(surface, MIP_LEVELS_1);
	REF_PTR_RELEASE(surface);

	io.Fonts->SetTexID((ImTextureID)(intptr_t)m_fontTexture);
	return m_fontTexture != nullptr;
}

void ImGuiRenderer::shutdown()
{
	if (ImGui::GetCurrentContext() != nullptr)
		ImGui::GetIO().Fonts->SetTexID((ImTextureID)0);

	REF_PTR_RELEASE(m_fontTexture);
}

void ImGuiRenderer::render(ImDrawData *drawData)
{
	if (drawData == nullptr || drawData->CmdListsCount == 0)
		return;

	const float width = drawData->DisplaySize.x * drawData->FramebufferScale.x;
	const float height = drawData->DisplaySize.y * drawData->FramebufferScale.y;
	if (width <= 0.0f || height <= 0.0f)
		return;

	IRenderBackend *backend = WW3D::Get_Render_Backend();
	if (backend == nullptr)
		return;

	// Save the transforms that the 2D pass overwrites, like Render2DClass::Render does.
	Matrix4x4 view, proj;
	backend->Get_Transform(RB_TRANSFORM_VIEW, view);
	backend->Get_Transform(RB_TRANSFORM_PROJECTION, proj);

	RenderBackendViewport vp;
	vp.x = 0;
	vp.y = 0;
	vp.width = (unsigned int)width;
	vp.height = (unsigned int)height;
	vp.min_z = 0.0f;
	vp.max_z = 1.0f;
	backend->Set_Viewport(vp);

	VertexMaterialClass *vm = VertexMaterialClass::Get_Preset(VertexMaterialClass::PRELIT_DIFFUSE);
	backend->Set_Material(vm);
	REF_PTR_RELEASE(vm);

	Matrix4x4 identity(true);
	backend->Set_World_Identity();
	backend->Set_View_Identity();
	backend->Set_Transform(RB_TRANSFORM_PROJECTION, identity);

	Batch batch;
	batch.begin(width, height);

	const ImVec2 clipOffset = drawData->DisplayPos;
	const ImVec2 clipScale = drawData->FramebufferScale;

	for (int n = 0; n < drawData->CmdListsCount; ++n)
	{
		const ImDrawList *cmdList = drawData->CmdLists[n];
		const ImDrawVert *vtx = cmdList->VtxBuffer.Data;
		const ImDrawIdx *idx = cmdList->IdxBuffer.Data;

		for (int c = 0; c < cmdList->CmdBuffer.Size; ++c)
		{
			const ImDrawCmd &cmd = cmdList->CmdBuffer[c];
			if (cmd.UserCallback != nullptr)
			{
				if (cmd.UserCallback != ImDrawCallback_ResetRenderState)
				{
					batch.flush();
					cmd.UserCallback(cmdList, &cmd);
				}
				continue;
			}

			ClipRect clip;
			clip.minX = (cmd.ClipRect.x - clipOffset.x) * clipScale.x;
			clip.minY = (cmd.ClipRect.y - clipOffset.y) * clipScale.y;
			clip.maxX = (cmd.ClipRect.z - clipOffset.x) * clipScale.x;
			clip.maxY = (cmd.ClipRect.w - clipOffset.y) * clipScale.y;
			if (clip.maxX <= clip.minX || clip.maxY <= clip.minY)
				continue;

			batch.setTexture((TextureClass *)(intptr_t)cmd.GetTexID());

			for (unsigned int i = 0; i + 2 < cmd.ElemCount; i += 3)
			{
				const ImDrawVert &v0 = vtx[cmd.VtxOffset + idx[cmd.IdxOffset + i + 0]];
				const ImDrawVert &v1 = vtx[cmd.VtxOffset + idx[cmd.IdxOffset + i + 1]];
				const ImDrawVert &v2 = vtx[cmd.VtxOffset + idx[cmd.IdxOffset + i + 2]];

				ClipVertex a = toClipVertex(v0);
				ClipVertex b = toClipVertex(v1);
				ClipVertex cc = toClipVertex(v2);
				a.x = (a.x - clipOffset.x) * clipScale.x; a.y = (a.y - clipOffset.y) * clipScale.y;
				b.x = (b.x - clipOffset.x) * clipScale.x; b.y = (b.y - clipOffset.y) * clipScale.y;
				cc.x = (cc.x - clipOffset.x) * clipScale.x; cc.y = (cc.y - clipOffset.y) * clipScale.y;

				emitClippedTriangle(batch, a, b, cc, clip);
			}
		}
	}

	batch.flush();

	backend->Set_Texture(0, nullptr);
	backend->Set_Transform(RB_TRANSFORM_VIEW, view);
	backend->Set_Transform(RB_TRANSFORM_PROJECTION, proj);
}

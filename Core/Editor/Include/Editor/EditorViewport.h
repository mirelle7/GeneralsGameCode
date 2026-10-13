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

// EditorViewport.h
// TheSuperHackers @feature The 3D view of the in-game WorldBuilder. Ported from the MFC
// WbView3d: its own scene, camera and terrain render object, and plain render objects for map
// objects. It never creates game Objects or Drawables.

#pragma once

#include "Lib/BaseType.h"
#include "Common/AsciiString.h"
#include "WW3D2/robjlist.h"
#include "WWMath/vector3.h"

class BaseHeightMapRenderObjClass;
struct Coord3D;
class CameraClass;
class EditorDocument;
class LightClass;
class MapObject;
class RTS3DScene;
class ThingTemplate;
class WBHeightMap;

enum { EDITOR_MAX_GLOBAL_LIGHTS = 3 };

class EditorViewport
{
public:
	EditorViewport();
	~EditorViewport();

	/// Creates the scene and the camera. Requires an initialized WW3D.
	void init();
	void shutdown();

	/// Builds the terrain and the map object render objects for a freshly loaded document.
	/// Pass null to clear the view.
	void setDocument(EditorDocument *doc);

	/// Sets the screen rectangle, in back buffer pixels, the 3D view renders into.
	void setViewRect(Int x, Int y, Int width, Int height);

	/// Renders the scene. Call between WW3D::Begin_Render and End_Render.
	void render();

	// Camera controls, mirroring the classic WorldBuilder: drag to pan, rotate, wheel to zoom.
	void pan(Real screenDX, Real screenDY);
	void rotate(Real radians);
	void pitch(Real delta);
	void zoom(Real wheelSteps);
	void resetCamera();
	void centerOn(Real worldX, Real worldY);

	/// Picks the terrain under a screen position. Returns FALSE when the cursor misses the map.
	Bool screenToWorld(Int screenX, Int screenY, Coord3D *world) const;

	Bool containsScreenPoint(Int screenX, Int screenY) const;

	// View options, see the View menu.
	Bool m_showObjects;
	Bool m_showWireframe;
	Bool m_showEntireMap;

private:
	void setupCamera();
	void updateLights();
	void updateTrees();
	void updateScorches();
	void createObjectRenderObjects();
	void clearScene();
	Real getGroundHeight(Real x, Real y) const;

	AsciiString getModelName(MapObject *mapObj, Real *scale) const;
	Int getPlayerColor(MapObject *mapObj) const;

	EditorDocument *m_doc;

	RTS3DScene *m_scene;
	CameraClass *m_camera;
	WBHeightMap *m_heightMapRenderObj;
	BaseHeightMapRenderObjClass *m_savedTerrainRenderObject;
	LightClass *m_globalLight[EDITOR_MAX_GLOBAL_LIGHTS];
	RefRenderObjListClass m_lightList;
	Bool m_needToLoadRoads;

	Int m_viewX;
	Int m_viewY;
	Int m_viewWidth;
	Int m_viewHeight;

	// Orbit camera around a target point on the ground.
	Vector3 m_cameraTarget;
	Real m_cameraYaw;
	Real m_cameraPitch;
	Real m_cameraDistance;
};

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

// EditorViewport.cpp
// TheSuperHackers @feature 3D view of the in-game WorldBuilder, ported from WbView3d.

#include "Editor/EditorViewport.h"

#include "Editor/EditorDocument.h"
#include "Editor/WBHeightMap.h"
#include "Editor/WHeightMapEdit.h"

#include "Common/Debug.h"
#include "Common/GlobalData.h"
#include "Common/MapObject.h"
#include "Common/NameKeyGenerator.h"
#include "Common/PlayerTemplate.h"
#include "Common/ThingTemplate.h"
#include "Common/WellKnownKeys.h"
#include "GameLogic/SidesList.h"
#include "GameLogic/TerrainLogic.h"
#include "W3DDevice/GameClient/W3DAssetManager.h"
#include "W3DDevice/GameClient/W3DScene.h"
#include "W3DDevice/GameClient/Module/W3DModelDraw.h"
#include "W3DDevice/GameClient/Module/W3DTreeDraw.h"
#include "WW3D2/camera.h"
#include "WW3D2/coltest.h"
#include "WW3D2/light.h"
#include "WW3D2/ww3d.h"
#include "WWMath/matrix3d.h"
#include "WWMath/lineseg.h"

#include <math.h>

namespace
{
	const Real kDefaultPitch = DEG_TO_RADF(37.5f);
	const Real kMinPitch = DEG_TO_RADF(10.0f);
	const Real kMaxPitch = DEG_TO_RADF(89.0f);
	const Real kMinDistance = 50.0f;
	const Real kMaxDistance = 5000.0f;
	const Real kFieldOfView = DEG_TO_RADF(50.0f);

	W3DAssetManager *getAssetManager()
	{
		return (W3DAssetManager *)WW3DAssetManager::Get_Instance();
	}

	void removeAllFromScene(RTS3DScene *scene)
	{
		SceneIterator *sceneIter = scene->Create_Iterator();
		sceneIter->First();
		while (!sceneIter->Is_Done())
		{
			RenderObjClass *robj = sceneIter->Current_Item();
			robj->Add_Ref();
			scene->Remove_Render_Object(robj);
			robj->Release_Ref();
			sceneIter->First();
		}
		scene->Destroy_Iterator(sceneIter);
	}
}

EditorViewport::EditorViewport() :
	m_showObjects(TRUE),
	m_showWireframe(FALSE),
	m_showEntireMap(TRUE),
	m_doc(nullptr),
	m_scene(nullptr),
	m_camera(nullptr),
	m_heightMapRenderObj(nullptr),
	m_savedTerrainRenderObject(nullptr),
	m_needToLoadRoads(FALSE),
	m_viewX(0),
	m_viewY(0),
	m_viewWidth(1),
	m_viewHeight(1),
	m_cameraTarget(0.0f, 0.0f, 0.0f),
	m_cameraYaw(0.0f),
	m_cameraPitch(kDefaultPitch),
	m_cameraDistance(600.0f)
{
	for (Int i = 0; i < EDITOR_MAX_GLOBAL_LIGHTS; ++i)
		m_globalLight[i] = nullptr;
}

EditorViewport::~EditorViewport()
{
	shutdown();
}

void EditorViewport::init()
{
	if (m_scene != nullptr)
		return;

	// The terrain render object constructor installs itself as TheTerrainRenderObject. Remember the
	// game's terrain so it can be restored when the editor hands the frame back.
	m_savedTerrainRenderObject = TheTerrainRenderObject;

	m_scene = NEW_REF(RTS3DScene, ());
	m_scene->Set_Ambient_Light(Vector3(0.5f, 0.5f, 0.5f));
	m_camera = NEW_REF(CameraClass, ());

	for (Int i = 0; i < EDITOR_MAX_GLOBAL_LIGHTS; ++i)
		m_globalLight[i] = NEW_REF(LightClass, (LightClass::DIRECTIONAL));
}

void EditorViewport::shutdown()
{
	if (m_scene == nullptr)
		return;

	setDocument(nullptr);

	for (Int i = 0; i < EDITOR_MAX_GLOBAL_LIGHTS; ++i)
		REF_PTR_RELEASE(m_globalLight[i]);

	REF_PTR_RELEASE(m_camera);
	REF_PTR_RELEASE(m_scene);

	TheTerrainRenderObject = m_savedTerrainRenderObject;
	m_savedTerrainRenderObject = nullptr;
}

void EditorViewport::clearScene()
{
	if (m_scene == nullptr)
		return;

	removeAllFromScene(m_scene);
	m_lightList.Reset_List();

	for (MapObject *pMapObj = MapObject::getFirstMapObject(); pMapObj; pMapObj = pMapObj->getNext())
		pMapObj->setRenderObj(nullptr);

	if (m_heightMapRenderObj != nullptr)
	{
		m_heightMapRenderObj->removeAllTrees();
		m_heightMapRenderObj->removeAllProps();
		m_heightMapRenderObj->clearAllScorches();
	}
}

void EditorViewport::setDocument(EditorDocument *doc)
{
	clearScene();
	REF_PTR_RELEASE(m_heightMapRenderObj);
	TheTerrainRenderObject = m_savedTerrainRenderObject;
	m_doc = doc;

	if (m_doc == nullptr || m_doc->getHeightMap() == nullptr || m_scene == nullptr)
		return;

	WorldHeightMapEdit *htMap = m_doc->getHeightMap();

	m_heightMapRenderObj = NEW_REF(WBHeightMap, ());
	m_scene->Add_Render_Object(m_heightMapRenderObj);

	updateLights();

	RefRenderObjListIterator lightListIt(&m_lightList);
	if (m_showEntireMap)
	{
		htMap->setDrawOrg(0, 0);
		htMap->setDrawWidth(htMap->getXExtent());
		htMap->setDrawHeight(htMap->getYExtent());
		m_heightMapRenderObj->initHeightData(htMap->getXExtent(), htMap->getYExtent(), htMap, &lightListIt);
	}
	else
	{
		htMap->setDrawWidth(std::min(128, htMap->getXExtent()));
		htMap->setDrawHeight(std::min(128, htMap->getYExtent()));
		m_heightMapRenderObj->initHeightData(htMap->getDrawWidth(), htMap->getDrawHeight(), htMap, &lightListIt);
	}
	m_heightMapRenderObj->updateViewImpassableAreas();
	m_heightMapRenderObj->setDrawEntireMap(m_showEntireMap);

	createObjectRenderObjects();
	updateTrees();
	updateScorches();
	m_needToLoadRoads = TRUE;

	resetCamera();
}

void EditorViewport::setViewRect(Int x, Int y, Int width, Int height)
{
	m_viewX = x;
	m_viewY = y;
	m_viewWidth = width > 1 ? width : 1;
	m_viewHeight = height > 1 ? height : 1;
}

Bool EditorViewport::containsScreenPoint(Int screenX, Int screenY) const
{
	return screenX >= m_viewX && screenX < m_viewX + m_viewWidth && screenY >= m_viewY && screenY < m_viewY + m_viewHeight;
}

Real EditorViewport::getGroundHeight(Real x, Real y) const
{
	if (m_heightMapRenderObj == nullptr)
		return 0.0f;
	return m_heightMapRenderObj->getHeightMapHeight(x, y, nullptr);
}

void EditorViewport::updateLights()
{
	m_lightList.Reset_List();

	TheWritableGlobalData->setTimeOfDay(TheGlobalData->m_timeOfDay);
	const GlobalData::TerrainLighting *ol = &TheGlobalData->m_terrainObjectsLighting[TheGlobalData->m_timeOfDay][0];
	m_scene->Set_Ambient_Light(Vector3(ol->ambient.red, ol->ambient.green, ol->ambient.blue));

	for (Int i = 0; i < EDITOR_MAX_GLOBAL_LIGHTS && i < MAX_GLOBAL_LIGHTS; ++i)
	{
		ol = &TheGlobalData->m_terrainObjectsLighting[TheGlobalData->m_timeOfDay][i];
		m_globalLight[i]->Set_Ambient(Vector3(0.0f, 0.0f, 0.0f));
		m_globalLight[i]->Set_Diffuse(Vector3(ol->diffuse.red, ol->diffuse.green, ol->diffuse.blue));
		m_globalLight[i]->Set_Specular(Vector3(0.0f, 0.0f, 0.0f));
		Matrix3D mtx;
		mtx.Set(Vector3(1, 0, 0), Vector3(0, 1, 0), Vector3(ol->lightPos.x, ol->lightPos.y, ol->lightPos.z), Vector3(0, 0, 0));
		m_globalLight[i]->Set_Transform(mtx);
		m_scene->setGlobalLight(m_globalLight[i], i);
	}

	if (m_heightMapRenderObj != nullptr)
		m_heightMapRenderObj->setTimeOfDay(TheGlobalData->m_timeOfDay);

	// Point lights placed in the map are handled at the device level.
	for (MapObject *pMapObj = MapObject::getFirstMapObject(); pMapObj; pMapObj = pMapObj->getNext())
	{
		if (!pMapObj->isLight())
			continue;

		Coord3D loc = *pMapObj->getLocation();
		loc.z += getGroundHeight(loc.x, loc.y);

		Dict *props = pMapObj->getProperties();
		RGBColor ambient, diffuse;
		ambient.setFromInt(props->getInt(TheKey_lightAmbientColor));
		diffuse.setFromInt(props->getInt(TheKey_lightDiffuseColor));

		LightClass *lightP = NEW_REF(LightClass, (LightClass::POINT));
		lightP->Set_Ambient(Vector3(ambient.red, ambient.green, ambient.blue));
		lightP->Set_Diffuse(Vector3(diffuse.red, diffuse.green, diffuse.blue));
		lightP->Set_Position(Vector3(loc.x, loc.y, loc.z + props->getReal(TheKey_lightHeightAboveTerrain)));
		lightP->Set_Far_Attenuation_Range(props->getReal(TheKey_lightInnerRadius), props->getReal(TheKey_lightOuterRadius));

		m_lightList.Add(lightP);
		m_scene->Add_Render_Object(lightP);
		pMapObj->setRenderObj(lightP);
		REF_PTR_RELEASE(lightP);
	}
}

void EditorViewport::updateTrees()
{
	if (m_heightMapRenderObj == nullptr)
		return;

	m_heightMapRenderObj->removeAllTrees();
	m_heightMapRenderObj->removeAllProps();
	for (MapObject *pMapObj = MapObject::getFirstMapObject(); pMapObj; pMapObj = pMapObj->getNext())
	{
		const ThingTemplate *tTemplate = pMapObj->getThingTemplate();
		if (tTemplate == nullptr || !tTemplate->isKindOf(KINDOF_OPTIMIZED_TREE))
			continue;

		const ModuleInfo &mi = tTemplate->getDrawModuleInfo();
		if (mi.getCount() <= 0)
			continue;

		const ModuleData *mdd = mi.getNthData(0);
		const W3DTreeDrawModuleData *md = mdd ? mdd->getAsW3DTreeDrawModuleData() : nullptr;
		if (md == nullptr)
			continue;

		Coord3D pos = *pMapObj->getLocation();
		pos.z += getGroundHeight(pos.x, pos.y);
		m_heightMapRenderObj->addTree((DrawableID)(intptr_t)pMapObj, pos, tTemplate->getAssetScale(), pMapObj->getAngle(), 0.0f, md);
	}
}

void EditorViewport::updateScorches()
{
	if (m_heightMapRenderObj == nullptr)
		return;

	m_heightMapRenderObj->clearAllScorches();
	for (MapObject *pMapObj = MapObject::getFirstMapObject(); pMapObj; pMapObj = pMapObj->getNext())
	{
		if (!pMapObj->isScorch())
			continue;

		const Coord3D *pos = pMapObj->getLocation();
		const Real radius = pMapObj->getProperties()->getReal(TheKey_objectRadius);
		const Scorches type = (Scorches)pMapObj->getProperties()->getInt(TheKey_scorchType);
		m_heightMapRenderObj->addStaticScorch(Vector3(pos->x, pos->y, pos->z), radius, type);
	}
}

AsciiString EditorViewport::getModelName(MapObject *mapObj, Real *scale) const
{
	*scale = 1.0f;
	const ThingTemplate *tTemplate = mapObj->getThingTemplate();
	if (tTemplate == nullptr || (mapObj->getFlags() & FLAG_DONT_RENDER))
		return AsciiString::TheEmptyString;

	ModelConditionFlags state;
	state.clear();
	if (TheGlobalData->m_weather == WEATHER_SNOWY)
		state.set(MODELCONDITION_SNOW);
	if (TheGlobalData->m_timeOfDay == TIME_OF_DAY_NIGHT)
		state.set(MODELCONDITION_NIGHT);

	*scale = tTemplate->getAssetScale();

	const ModuleInfo &mi = tTemplate->getDrawModuleInfo();
	if (mi.getCount() > 0)
	{
		const ModuleData *mdd = mi.getNthData(0);
		const W3DModelDrawModuleData *md = mdd ? mdd->getAsW3DModelDrawModuleData() : nullptr;
		if (md)
			return md->getBestModelNameForWB(state);
	}
	return AsciiString::TheEmptyString;
}

Int EditorViewport::getPlayerColor(MapObject *mapObj) const
{
	Bool exists;
	AsciiString objectTeamName = mapObj->getProperties()->getAsciiString(TheKey_originalOwner, &exists);
	if (!exists)
		return 0xFFFFFF;

	TeamsInfo *teamInfo = TheSidesList->findTeamInfo(objectTeamName);
	if (teamInfo == nullptr)
		return 0xFFFFFF;

	AsciiString teamOwner = teamInfo->getDict()->getAsciiString(TheKey_teamOwner);
	SidesInfo *pSide = TheSidesList->findSideInfo(teamOwner);
	if (pSide == nullptr)
		return 0xFFFFFF;

	Bool hasColor = FALSE;
	Int color = pSide->getDict()->getInt(TheKey_playerColor, &hasColor);
	if (hasColor)
		return color;

	AsciiString tmplname = pSide->getDict()->getAsciiString(TheKey_playerFaction);
	const PlayerTemplate *pt = ThePlayerTemplateStore->findPlayerTemplate(NAMEKEY(tmplname));
	if (pt)
		return pt->getPreferredColor()->getAsInt();

	return 0xFFFFFF;
}

void EditorViewport::createObjectRenderObjects()
{
	if (!m_showObjects)
		return;

	W3DAssetManager *assetManager = getAssetManager();
	if (assetManager == nullptr)
		return;

	for (MapObject *pMapObj = MapObject::getFirstMapObject(); pMapObj; pMapObj = pMapObj->getNext())
	{
		// Roads, bridges, lights and scorches have their own render paths.
		if (pMapObj->getFlags() & (FLAG_ROAD_FLAGS | FLAG_BRIDGE_FLAGS))
			continue;
		if (pMapObj->isLight() || pMapObj->isScorch() || pMapObj->isWaypoint())
			continue;

		const ThingTemplate *tTemplate = pMapObj->getThingTemplate();
		if (tTemplate && tTemplate->isKindOf(KINDOF_OPTIMIZED_TREE))
			continue;

		Real scale = 1.0f;
		AsciiString modelName = getModelName(pMapObj, &scale);
		if (modelName.isEmpty() || strncmp(modelName.str(), "No ", 3) == 0)
			continue;

		RenderObjClass *renderObj = assetManager->Create_Render_Obj(modelName.str(), scale, getPlayerColor(pMapObj));
		if (renderObj == nullptr)
			continue;

		Coord3D loc = *pMapObj->getLocation();
		loc.z += getGroundHeight(loc.x, loc.y);

		Matrix3D renderObjPos(true);
		renderObjPos.Translate(loc.x, loc.y, loc.z);
		renderObjPos.Rotate_Z(pMapObj->getAngle());
		renderObj->Set_Transform(renderObjPos);

		if (tTemplate && tTemplate->isKindOf(KINDOF_VEHICLE))
		{
			// Vehicles follow the slope of the terrain, like the classic WorldBuilder shows them.
			Coord3D terrainNormal;
			m_heightMapRenderObj->getHeightMapHeight(loc.x, loc.y, &terrainNormal);
			Matrix3D mtx;
			makeAlignToNormalMatrix(pMapObj->getAngle(), loc, terrainNormal, mtx);
			renderObj->Set_Transform(mtx);
		}

		pMapObj->setRenderObj(renderObj);
		m_scene->Add_Render_Object(renderObj);
		REF_PTR_RELEASE(renderObj);
	}
}

void EditorViewport::resetCamera()
{
	m_cameraYaw = 0.0f;
	m_cameraPitch = kDefaultPitch;
	m_cameraDistance = TheGlobalData->m_maxCameraHeight > 0.0f ? TheGlobalData->m_maxCameraHeight * 1.5f : 600.0f;

	Real x = 0.0f;
	Real y = 0.0f;
	WorldHeightMapEdit *htMap = m_doc ? m_doc->getHeightMap() : nullptr;
	if (htMap != nullptr)
	{
		x = MAP_XY_FACTOR * htMap->getXExtent() / 2 - htMap->getBorderSize() * MAP_XY_FACTOR;
		y = MAP_XY_FACTOR * htMap->getYExtent() / 2 - htMap->getBorderSize() * MAP_XY_FACTOR;
	}

	// Start where the game camera starts, like the classic WorldBuilder.
	AsciiString startingCamName = TheNameKeyGenerator->keyToName(TheKey_InitialCameraPosition);
	for (MapObject *pMapObj = MapObject::getFirstMapObject(); pMapObj; pMapObj = pMapObj->getNext())
	{
		if (pMapObj->isWaypoint() && startingCamName == pMapObj->getWaypointName())
		{
			x = pMapObj->getLocation()->x;
			y = pMapObj->getLocation()->y;
			break;
		}
	}

	centerOn(x, y);
}

void EditorViewport::centerOn(Real worldX, Real worldY)
{
	m_cameraTarget.Set(worldX, worldY, getGroundHeight(worldX, worldY));
}

void EditorViewport::pan(Real screenDX, Real screenDY)
{
	// Move the target so the ground under the cursor follows the drag.
	const Real worldPerPixel = (2.0f * m_cameraDistance * tanf(kFieldOfView * 0.5f)) / (Real)m_viewWidth;
	const Real c = cosf(m_cameraYaw);
	const Real s = sinf(m_cameraYaw);
	const Real dx = -screenDX * worldPerPixel;
	const Real dy = screenDY * worldPerPixel;
	centerOn(m_cameraTarget.X + dx * c - dy * s, m_cameraTarget.Y + dx * s + dy * c);
}

void EditorViewport::rotate(Real radians)
{
	m_cameraYaw += radians;
}

void EditorViewport::pitch(Real delta)
{
	m_cameraPitch += delta;
	if (m_cameraPitch < kMinPitch) m_cameraPitch = kMinPitch;
	if (m_cameraPitch > kMaxPitch) m_cameraPitch = kMaxPitch;
}

void EditorViewport::zoom(Real wheelSteps)
{
	m_cameraDistance *= powf(0.85f, wheelSteps);
	if (m_cameraDistance < kMinDistance) m_cameraDistance = kMinDistance;
	if (m_cameraDistance > kMaxDistance) m_cameraDistance = kMaxDistance;
}

void EditorViewport::setupCamera()
{
	int screenWidth, screenHeight, bits;
	bool windowed;
	WW3D::Get_Device_Resolution(screenWidth, screenHeight, bits, windowed);
	if (screenWidth <= 0 || screenHeight <= 0)
		return;

	Vector2 vMin((Real)m_viewX / (Real)screenWidth, (Real)m_viewY / (Real)screenHeight);
	Vector2 vMax((Real)(m_viewX + m_viewWidth) / (Real)screenWidth, (Real)(m_viewY + m_viewHeight) / (Real)screenHeight);
	m_camera->Set_Viewport(vMin, vMax);
	m_camera->Set_View_Plane(kFieldOfView, -1);
	m_camera->Set_Aspect_Ratio((Real)m_viewWidth / (Real)m_viewHeight);

	// The camera looks along -Y when the yaw is zero, matching the game's default camera.
	const Real horizontal = m_cameraDistance * cosf(m_cameraPitch);
	Vector3 source;
	source.X = m_cameraTarget.X + horizontal * sinf(m_cameraYaw);
	source.Y = m_cameraTarget.Y - horizontal * cosf(m_cameraYaw);
	source.Z = m_cameraTarget.Z + m_cameraDistance * sinf(m_cameraPitch);

	Matrix3D camtransform(1);
	camtransform.Look_At(source, m_cameraTarget, 0);
	m_camera->Set_Transform(camtransform);

	const Real lookDistance = std::max(300.0f, m_cameraDistance);
	m_camera->Set_Clip_Planes(lookDistance / 200.0f, lookDistance * 6.0f);
}

void EditorViewport::render()
{
	if (m_scene == nullptr || m_heightMapRenderObj == nullptr)
		return;

	setupCamera();

	if (m_needToLoadRoads)
	{
		m_heightMapRenderObj->loadRoadsAndBridges(nullptr, FALSE);
		m_heightMapRenderObj->worldBuilderUpdateBridgeTowers(getAssetManager(), m_scene);
		m_needToLoadRoads = FALSE;
	}

	RefRenderObjListIterator lightListIt(&m_lightList);
	m_heightMapRenderObj->updateCenter(m_camera, &m_cameraTarget, &lightListIt);
	m_heightMapRenderObj->On_Frame_Update();

	m_heightMapRenderObj->doTextures(true);
	m_scene->Set_Polygon_Mode(SceneClass::FILL);
	WW3D::Render(m_scene, m_camera);

	if (m_showWireframe)
	{
		m_heightMapRenderObj->doTextures(false);
		m_scene->Set_Polygon_Mode(SceneClass::LINE);
		WW3D::Render(m_scene, m_camera);
		m_heightMapRenderObj->doTextures(true);
		m_scene->Set_Polygon_Mode(SceneClass::FILL);
	}
}

Bool EditorViewport::screenToWorld(Int screenX, Int screenY, Coord3D *world) const
{
	if (m_camera == nullptr || !containsScreenPoint(screenX, screenY))
		return FALSE;

	const Real logX = (Real)(screenX - m_viewX) / (Real)m_viewWidth;
	const Real logY = (Real)(screenY - m_viewY) / (Real)m_viewHeight;

	// Same ray construction as WbView3d::viewToDocCoords, in view local coordinates.
	const Matrix3D &camera_matrix = m_camera->Get_Transform();
	const Vector3 rayLocation = m_camera->Get_Position();

	Vector2 min, max;
	m_camera->Get_View_Plane(min, max);
	const Real xscale = (max.X - min.X);
	const Real yscale = (max.Y - min.Y);

	const Real zmod = -1.0f;
	const Real xmod = (-logX + 0.5f) * zmod * xscale;
	const Real ymod = (logY - 0.5f) * zmod * yscale;

	Vector3 rayDirection(
		zmod * camera_matrix[0][2] + xmod * camera_matrix[0][0] + ymod * camera_matrix[0][1],
		zmod * camera_matrix[1][2] + xmod * camera_matrix[1][0] + ymod * camera_matrix[1][1],
		zmod * camera_matrix[2][2] + xmod * camera_matrix[2][0] + ymod * camera_matrix[2][1]);
	rayDirection.Normalize();

	const Real maxDistance = m_camera->Get_Depth() * MAP_XY_FACTOR;
	const Vector3 rayEnd = rayLocation + rayDirection * maxDistance;
	LineSegClass ray(rayLocation, rayEnd);

	Vector3 intersection(0, 0, 0);
	Bool hit = FALSE;
	if (m_heightMapRenderObj != nullptr)
	{
		CastResultStruct castResult;
		RayCollisionTestClass rayCollide(ray, &castResult);
		if (m_heightMapRenderObj->Cast_Ray(rayCollide))
		{
			intersection = castResult.ContactPoint;
			hit = TRUE;
		}
	}

	if (!hit)
	{
		// Fall back to the plane through the camera target, like the classic tracking Z.
		const Real z = m_cameraTarget.Z;
		intersection.X = Vector3::Find_X_At_Z(z, rayLocation, rayEnd);
		intersection.Y = Vector3::Find_Y_At_Z(z, rayLocation, rayEnd);
		intersection.Z = z;
	}

	world->x = intersection.X;
	world->y = intersection.Y;
	world->z = intersection.Z;
	return TRUE;
}

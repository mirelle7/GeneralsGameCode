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

// WorldBuilderEditor.cpp
// TheSuperHackers @feature The in-game WorldBuilder: switches the application between the main
// menu and the editor, owns the Dear ImGui context and lays out the classic WorldBuilder interface.

#include "Editor/WorldBuilderEditor.h"

#include "Editor/EditorDocument.h"
#include "Editor/EditorUIHooks.h"
#include "Editor/EditorViewport.h"
#include "Editor/ImGuiRenderer.h"
#include "Editor/WHeightMapEdit.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"

#include "Common/EditorInterface.h"
#include "Common/FileSystem.h"
#include "Common/FramePacer.h"
#include "Common/GameAudio.h"
#include "Common/GameEngine.h"
#include "Common/GlobalData.h"
#include "Common/MapObject.h"
#include "Common/WellKnownKeys.h"
#include "GameClient/Display.h"
#include "GameClient/MapUtil.h"
#include "GameClient/Shell.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/PolygonTrigger.h"
#include "GameLogic/SidesList.h"
#include "WW3D2/ww3d.h"

#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <vector>

#if defined(SAGE_USE_SDL3)
#include <SDL3/SDL.h>
#include "imgui_impl_sdl3.h"
extern SDL_Window *TheSDL3Window;
#elif defined(_WIN32)
#include "imgui_impl_win32.h"
extern HWND ApplicationHWnd;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

// ------------------------------------------------------------------------------------------------
// EditorUI hooks used by the ported WorldBuilder document code.
// ------------------------------------------------------------------------------------------------

namespace
{
	std::vector<std::string> s_pendingMessages;
}

void EditorUI::message(const char *text)
{
	DEBUG_LOG(("WorldBuilder: %s", text));
	s_pendingMessages.push_back(text);
}

void EditorUI::messageFormat(const char *format, ...)
{
	char buffer[2048];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	message(buffer);
}

Bool EditorUI::isLayerHidden(const AsciiString &)
{
	return FALSE;
}

// ------------------------------------------------------------------------------------------------

namespace
{

class WorldBuilderEditor : public EditorInterface
{
public:
	WorldBuilderEditor();
	virtual ~WorldBuilderEditor() override;

	virtual void requestEnter() override;
	virtual Bool isActive() const override { return m_state == STATE_ACTIVE; }
	virtual Bool updateFrame() override;
	virtual void drawFrame() override;
	virtual Bool handleSDLEvent(const void *sdlEvent) override;
	virtual Bool handleWin32Message(void *hwnd, UnsignedInt message, UnsignedInt64 wParam, Int64 lParam) override;

private:
	enum State
	{
		STATE_INACTIVE,
		STATE_ENTER_REQUESTED,	///< the main menu asked for the editor
		STATE_WAIT_FOR_SHELL,		///< waiting for the shell map game to be cleared
		STATE_ACTIVE,
		STATE_LEAVE_REQUESTED,
	};

	void activate();
	void deactivate();
	void requestLeave(Bool quitApplication);

	void beginImGuiFrame();
	void buildInterface();
	void buildMainMenuBar();
	void buildViewport();
	void buildMapInfoWindow();
	void buildOpenMapWindow();
	void buildSaveAsWindow();
	void buildMessagesWindow();
	void buildStatusBar();
	void handleViewportInput();

	void openMap(const AsciiString &path);
	void saveMap(const AsciiString &path);
	void closeMap();

	State m_state;
	Bool m_startupHandled;
	Bool m_quitAfterLeave;

	ImGuiContext *m_imguiContext;
	ImGuiRenderer m_renderer;
	EditorDocument m_document;
	EditorViewport m_viewport;

	AsciiString m_iniPath;
	Bool m_savedIsWorldBuilder;
	Bool m_savedShroudOn;

	// Interface state.
	Bool m_showOpenMap;
	Bool m_showSaveAs;
	Bool m_showMapInfo;
	Bool m_showMessages;
	Bool m_showDemo;
	std::string m_openFilter;
	std::string m_saveAsPath;
	std::vector<std::string> m_messageLog;
	Bool m_hasCursorWorld;
	Coord3D m_cursorWorld;
	ImVec2 m_framebufferScale;
};

WorldBuilderEditor::WorldBuilderEditor() :
	m_state(STATE_INACTIVE),
	m_startupHandled(FALSE),
	m_quitAfterLeave(FALSE),
	m_imguiContext(nullptr),
	m_savedIsWorldBuilder(FALSE),
	m_savedShroudOn(TRUE),
	m_showOpenMap(FALSE),
	m_showSaveAs(FALSE),
	m_showMapInfo(TRUE),
	m_showMessages(FALSE),
	m_showDemo(FALSE),
	m_hasCursorWorld(FALSE),
	m_framebufferScale(1.0f, 1.0f)
{
	m_cursorWorld.zero();
}

WorldBuilderEditor::~WorldBuilderEditor()
{
	// The engine is already gone here. Normal exits leave the editor first (see requestLeave), so
	// only the ImGui context can be left, and it holds no engine resources once deactivated.
	if (m_imguiContext != nullptr)
	{
		ImGui::DestroyContext(m_imguiContext);
		m_imguiContext = nullptr;
	}
}

void WorldBuilderEditor::requestEnter()
{
	if (m_state == STATE_INACTIVE)
		m_state = STATE_ENTER_REQUESTED;
}

void WorldBuilderEditor::requestLeave(Bool quitApplication)
{
	if (m_state == STATE_ACTIVE)
	{
		m_state = STATE_LEAVE_REQUESTED;
		m_quitAfterLeave = quitApplication;
	}
}

Bool WorldBuilderEditor::updateFrame()
{
	switch (m_state)
	{
		case STATE_INACTIVE:
			// -worldbuilder opens the editor as soon as the main menu is up.
			if (!m_startupHandled && TheGlobalData->m_startInWorldBuilder && TheShell != nullptr && TheShell->isShellActive())
			{
				m_startupHandled = TRUE;
				requestEnter();
			}
			return FALSE;

		case STATE_ENTER_REQUESTED:
			// Stop the shell map and hide the menus. Clearing the shell game is a deferred message, so
			// let the engine run its regular update until the game is gone.
			if (TheShell != nullptr)
			{
				TheShell->showShellMap(FALSE);
				TheShell->hide(TRUE);
			}
			m_state = STATE_WAIT_FOR_SHELL;
			return FALSE;

		case STATE_WAIT_FOR_SHELL:
			if (TheGameLogic != nullptr && TheGameLogic->isInGame())
				return FALSE;
			activate();
			m_state = STATE_ACTIVE;
			break;

		case STATE_LEAVE_REQUESTED:
			deactivate();
			m_state = STATE_INACTIVE;
			if (TheShell != nullptr)
			{
				TheShell->hide(FALSE);
				TheShell->showShellMap(TRUE);
			}
			if (m_quitAfterLeave)
			{
				m_quitAfterLeave = FALSE;
				TheGameEngine->setQuitting(TRUE);
			}
			return FALSE;

		case STATE_ACTIVE:
			break;
	}

	if (TheAudio != nullptr)
		TheAudio->UPDATE();

	beginImGuiFrame();
	buildInterface();
	ImGui::Render();

	WW3D::Update_Logic_Frame_Time(TheFramePacer->getLogicTimeStepMilliseconds());
	WW3D::Sync(WW3D::Get_Fractional_Sync_Milliseconds() >= WWSyncMilliseconds);

	if (TheDisplay != nullptr)
		TheDisplay->DRAW();

	return TRUE;
}

void WorldBuilderEditor::activate()
{
	m_savedIsWorldBuilder = TheGlobalData->m_isWorldBuilder;
	m_savedShroudOn = TheGlobalData->m_shroudOn;
	TheWritableGlobalData->m_isWorldBuilder = TRUE;
	TheWritableGlobalData->m_shroudOn = FALSE;

	m_imguiContext = ImGui::CreateContext();
	ImGui::SetCurrentContext(m_imguiContext);

	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
	// The game hides the OS cursor and draws its own, which is not drawn while the editor runs.
	io.MouseDrawCursor = true;

	m_iniPath = TheGlobalData->getPath_UserData();
	m_iniPath.concat("WorldBuilderLayout.ini");
	io.IniFilename = m_iniPath.str();

	ImGui::StyleColorsDark();

#if defined(SAGE_USE_SDL3)
	ImGui_ImplSDL3_InitForOther(TheSDL3Window);
#elif defined(_WIN32)
	ImGui_ImplWin32_Init(ApplicationHWnd);
#endif

	m_renderer.init();
	m_viewport.init();

	if (m_saveAsPath.empty() && TheMapCache != nullptr)
	{
		AsciiString dir = TheMapCache->getUserMapDir();
		m_saveAsPath = dir.str();
		m_saveAsPath += "\\NewMap\\NewMap.map";
	}

	m_showOpenMap = !m_document.isOpen();
}

void WorldBuilderEditor::deactivate()
{
	m_viewport.setDocument(nullptr);
	m_document.close();
	m_viewport.shutdown();
	m_renderer.shutdown();

#if defined(SAGE_USE_SDL3)
	ImGui_ImplSDL3_Shutdown();
	// Hand text input back in the state the game expects (see SDL3GameEngine::updateTextInputState).
	if (TheSDL3Window != nullptr)
		SDL_StopTextInput(TheSDL3Window);
#elif defined(_WIN32)
	ImGui_ImplWin32_Shutdown();
#endif

	ImGui::DestroyContext(m_imguiContext);
	m_imguiContext = nullptr;

	TheWritableGlobalData->m_isWorldBuilder = m_savedIsWorldBuilder;
	TheWritableGlobalData->m_shroudOn = m_savedShroudOn;
}

void WorldBuilderEditor::beginImGuiFrame()
{
#if defined(SAGE_USE_SDL3)
	ImGui_ImplSDL3_NewFrame();
#elif defined(_WIN32)
	ImGui_ImplWin32_NewFrame();
#endif

	// Lay out in window coordinates and render into the back buffer, which can have a different
	// size than the window (fixed game resolutions, HiDPI).
	ImGuiIO &io = ImGui::GetIO();
	int width, height, bits;
	bool windowed;
	WW3D::Get_Device_Resolution(width, height, bits, windowed);
	if (io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f)
		io.DisplaySize = ImVec2((float)width, (float)height);
	m_framebufferScale = ImVec2((float)width / io.DisplaySize.x, (float)height / io.DisplaySize.y);
	io.DisplayFramebufferScale = m_framebufferScale;

	ImGui::NewFrame();
}

void WorldBuilderEditor::drawFrame()
{
	m_viewport.render();
	m_renderer.render(ImGui::GetDrawData());
}

Bool WorldBuilderEditor::handleSDLEvent(const void *sdlEvent)
{
#if defined(SAGE_USE_SDL3)
	if (m_state != STATE_ACTIVE)
		return FALSE;

	const SDL_Event *event = (const SDL_Event *)sdlEvent;
	switch (event->type)
	{
		case SDL_EVENT_QUIT:
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			// The game routes quitting through its message stream, which does not run while the
			// editor owns the frame. Leave the editor first, then quit.
			requestLeave(TRUE);
			return TRUE;

		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP:
		case SDL_EVENT_TEXT_INPUT:
		case SDL_EVENT_TEXT_EDITING:
		case SDL_EVENT_MOUSE_MOTION:
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP:
		case SDL_EVENT_MOUSE_WHEEL:
			ImGui_ImplSDL3_ProcessEvent(event);
			return TRUE;

		default:
			// Window, focus and resize events go to both.
			ImGui_ImplSDL3_ProcessEvent(event);
			return FALSE;
	}
#else
	(void)sdlEvent;
	return FALSE;
#endif
}

Bool WorldBuilderEditor::handleWin32Message(void *hwnd, UnsignedInt message, UnsignedInt64 wParam, Int64 lParam)
{
#if !defined(SAGE_USE_SDL3) && defined(_WIN32)
	if (m_state != STATE_ACTIVE)
		return FALSE;

	if (message == WM_CLOSE)
	{
		requestLeave(TRUE);
		return TRUE;
	}

	ImGui_ImplWin32_WndProcHandler((HWND)hwnd, message, (WPARAM)wParam, (LPARAM)lParam);

	switch (message)
	{
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MBUTTONDBLCLK:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
		case WM_XBUTTONDBLCLK:
		case WM_MOUSEWHEEL:
		case WM_MOUSEHWHEEL:
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_CHAR:
			return TRUE;
		default:
			// System keys still reach DefWindowProc so Alt+F4 keeps working.
			return FALSE;
	}
#else
	(void)hwnd; (void)message; (void)wParam; (void)lParam;
	return FALSE;
#endif
}

// ------------------------------------------------------------------------------------------------
// Document commands
// ------------------------------------------------------------------------------------------------

void WorldBuilderEditor::openMap(const AsciiString &path)
{
	m_viewport.setDocument(nullptr);
	if (m_document.load(path))
	{
		m_viewport.setDocument(&m_document);
		m_saveAsPath = path.str();
		m_showOpenMap = FALSE;
	}
}

void WorldBuilderEditor::saveMap(const AsciiString &path)
{
	if (!m_document.isOpen())
		return;

	// Create the map folder, the game expects Maps\<name>\<name>.map.
	std::string dir = path.str();
	const size_t slash = dir.find_last_of("\\/");
	if (slash != std::string::npos)
	{
		dir.resize(slash);
		TheFileSystem->createDirectory(AsciiString(dir.c_str()));
	}

	if (m_document.save(path))
	{
		EditorUI::messageFormat("Saved '%s'.", path.str());
		if (TheMapCache != nullptr)
			TheMapCache->updateCache();
	}
}

void WorldBuilderEditor::closeMap()
{
	m_viewport.setDocument(nullptr);
	m_document.close();
}

// ------------------------------------------------------------------------------------------------
// Interface
// ------------------------------------------------------------------------------------------------

void WorldBuilderEditor::buildInterface()
{
	for (size_t i = 0; i < s_pendingMessages.size(); ++i)
	{
		m_messageLog.push_back(s_pendingMessages[i]);
		m_showMessages = TRUE;
	}
	s_pendingMessages.clear();

	buildMainMenuBar();
	buildViewport();
	buildStatusBar();

	if (m_showMapInfo)
		buildMapInfoWindow();
	if (m_showOpenMap)
		buildOpenMapWindow();
	if (m_showSaveAs)
		buildSaveAsWindow();
	if (m_showMessages)
		buildMessagesWindow();
	if (m_showDemo)
		ImGui::ShowDemoWindow(&m_showDemo);
}

void WorldBuilderEditor::buildMainMenuBar()
{
	if (!ImGui::BeginMainMenuBar())
		return;

	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("Open...", "Ctrl+O"))
			m_showOpenMap = TRUE;
		if (ImGui::MenuItem("Save", "Ctrl+S", false, m_document.isOpen()))
			saveMap(m_document.getPath());
		if (ImGui::MenuItem("Save As...", nullptr, false, m_document.isOpen()))
			m_showSaveAs = TRUE;
		if (ImGui::MenuItem("Close", nullptr, false, m_document.isOpen()))
			closeMap();
		ImGui::Separator();
		if (ImGui::MenuItem("Exit to Main Menu"))
			requestLeave(FALSE);
		if (ImGui::MenuItem("Quit Game"))
			requestLeave(TRUE);
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("View"))
	{
		bool showObjects = m_viewport.m_showObjects != FALSE;
		if (ImGui::MenuItem("Show Objects", nullptr, &showObjects))
		{
			m_viewport.m_showObjects = showObjects;
			if (m_document.isOpen())
				m_viewport.setDocument(&m_document);
		}
		bool wireframe = m_viewport.m_showWireframe != FALSE;
		if (ImGui::MenuItem("Show Wireframe", nullptr, &wireframe))
			m_viewport.m_showWireframe = wireframe;
		if (ImGui::MenuItem("Reset Camera", "Home"))
			m_viewport.resetCamera();
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Window"))
	{
		bool showInfo = m_showMapInfo != FALSE;
		if (ImGui::MenuItem("Map Info", nullptr, &showInfo))
			m_showMapInfo = showInfo;
		bool showMessages = m_showMessages != FALSE;
		if (ImGui::MenuItem("Messages", nullptr, &showMessages))
			m_showMessages = showMessages;
		bool showDemo = m_showDemo != FALSE;
		if (ImGui::MenuItem("ImGui Demo", nullptr, &showDemo))
			m_showDemo = showDemo;
		ImGui::EndMenu();
	}

	const ImGuiIO &io = ImGui::GetIO();
	if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O, false))
		m_showOpenMap = TRUE;
	if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false) && m_document.isOpen())
		saveMap(m_document.getPath());

	ImGui::EndMainMenuBar();
}

void WorldBuilderEditor::buildViewport()
{
	// The dock space covers the screen below the menu bar. Its empty central node is the 3D view:
	// panels dock around it like in the classic WorldBuilder.
	const ImGuiID dockId = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
	ImGuiDockNode *central = ImGui::DockBuilderGetCentralNode(dockId);

	ImVec2 pos = ImGui::GetMainViewport()->WorkPos;
	ImVec2 size = ImGui::GetMainViewport()->WorkSize;
	if (central != nullptr)
	{
		pos = central->Pos;
		size = central->Size;
	}

	m_viewport.setViewRect(
		(Int)(pos.x * m_framebufferScale.x),
		(Int)(pos.y * m_framebufferScale.y),
		(Int)(size.x * m_framebufferScale.x),
		(Int)(size.y * m_framebufferScale.y));

	if (!m_document.isOpen())
	{
		ImDrawList *drawList = ImGui::GetBackgroundDrawList();
		const char *hint = "No map open.  File > Open... (Ctrl+O)";
		const ImVec2 textSize = ImGui::CalcTextSize(hint);
		drawList->AddText(ImVec2(pos.x + (size.x - textSize.x) * 0.5f, pos.y + (size.y - textSize.y) * 0.5f), IM_COL32(200, 200, 200, 255), hint);
	}

	handleViewportInput();
}

void WorldBuilderEditor::handleViewportInput()
{
	ImGuiIO &io = ImGui::GetIO();
	m_hasCursorWorld = FALSE;

	if (!m_document.isOpen() || io.WantCaptureMouse)
		return;

	const Int mouseX = (Int)(io.MousePos.x * m_framebufferScale.x);
	const Int mouseY = (Int)(io.MousePos.y * m_framebufferScale.y);
	if (!m_viewport.containsScreenPoint(mouseX, mouseY))
		return;

	m_hasCursorWorld = m_viewport.screenToWorld(mouseX, mouseY, &m_cursorWorld);

	// Right drag pans, middle drag rotates and pitches, the wheel zooms.
	if (ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f))
		m_viewport.pan(io.MouseDelta.x * m_framebufferScale.x, io.MouseDelta.y * m_framebufferScale.y);
	if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
	{
		m_viewport.rotate(io.MouseDelta.x * 0.01f);
		m_viewport.pitch(io.MouseDelta.y * 0.005f);
	}
	if (io.MouseWheel != 0.0f)
		m_viewport.zoom(io.MouseWheel);

	if (!io.WantCaptureKeyboard)
	{
		const Real step = 20.0f;
		if (ImGui::IsKeyDown(ImGuiKey_LeftArrow)) m_viewport.pan(step, 0.0f);
		if (ImGui::IsKeyDown(ImGuiKey_RightArrow)) m_viewport.pan(-step, 0.0f);
		if (ImGui::IsKeyDown(ImGuiKey_UpArrow)) m_viewport.pan(0.0f, step);
		if (ImGui::IsKeyDown(ImGuiKey_DownArrow)) m_viewport.pan(0.0f, -step);
		if (ImGui::IsKeyPressed(ImGuiKey_Home, false)) m_viewport.resetCamera();
	}
}

void WorldBuilderEditor::buildMapInfoWindow()
{
	bool open = true;
	if (ImGui::Begin("Map Info", &open))
	{
		if (!m_document.isOpen())
		{
			ImGui::TextUnformatted("No map open.");
		}
		else
		{
			WorldHeightMapEdit *htMap = m_document.getHeightMap();
			ImGui::Text("File: %s", m_document.getPath().str());
			ImGui::Text("Size: %d x %d (border %d)", htMap->getXExtent(), htMap->getYExtent(), htMap->getBorderSize());

			Int objects = 0, waypoints = 0, lights = 0, roads = 0, scorches = 0;
			for (MapObject *obj = MapObject::getFirstMapObject(); obj; obj = obj->getNext())
			{
				if (obj->isWaypoint()) ++waypoints;
				else if (obj->isLight()) ++lights;
				else if (obj->isScorch()) ++scorches;
				else if (obj->getFlags() & (FLAG_ROAD_FLAGS | FLAG_BRIDGE_FLAGS)) ++roads;
				else ++objects;
			}
			Int triggers = 0;
			for (PolygonTrigger *trig = PolygonTrigger::getFirstPolygonTrigger(); trig; trig = trig->getNext())
				++triggers;

			ImGui::Text("Objects: %d", objects);
			ImGui::Text("Waypoints: %d (%d links)", waypoints, m_document.getNumWaypointLinks());
			ImGui::Text("Polygon triggers: %d", triggers);
			ImGui::Text("Road points: %d", roads);
			ImGui::Text("Lights: %d  Scorches: %d", lights, scorches);

			if (ImGui::CollapsingHeader("Players", ImGuiTreeNodeFlags_DefaultOpen) && TheSidesList != nullptr)
			{
				for (Int i = 0; i < TheSidesList->getNumSides(); ++i)
				{
					SidesInfo *side = TheSidesList->getSideInfo(i);
					AsciiString name = side->getDict()->getAsciiString(TheKey_playerName);
					AsciiString faction = side->getDict()->getAsciiString(TheKey_playerFaction);
					ImGui::BulletText("%s  %s", name.isEmpty() ? "(neutral)" : name.str(), faction.str());
				}
				ImGui::Text("Teams: %d", TheSidesList->getNumTeams());
			}
		}
	}
	ImGui::End();
	m_showMapInfo = open;
}

void WorldBuilderEditor::buildOpenMapWindow()
{
	ImGui::SetNextWindowSize(ImVec2(520.0f, 480.0f), ImGuiCond_FirstUseEver);
	bool open = true;
	if (ImGui::Begin("Open Map", &open))
	{
		ImGui::InputTextWithHint("##filter", "Filter", &m_openFilter);

		if (TheMapCache != nullptr && ImGui::BeginChild("maps", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders))
		{
			for (MapCache::const_iterator it = TheMapCache->begin(); it != TheMapCache->end(); ++it)
			{
				const AsciiString &path = it->first;
				if (!m_openFilter.empty())
				{
					AsciiString filter(m_openFilter.c_str());
					filter.toLower();
					if (strstr(path.str(), filter.str()) == nullptr)
						continue;
				}

				const char *label = path.str();
				if (ImGui::Selectable(label, false, ImGuiSelectableFlags_AllowDoubleClick) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					openMap(path);
					break;
				}
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("%s\n%s map", it->second.m_fileName.str(), it->second.m_isOfficial ? "Official" : "User");
			}
		}
		if (TheMapCache != nullptr)
			ImGui::EndChild();
	}
	ImGui::End();
	if (!open)
		m_showOpenMap = FALSE;
}

void WorldBuilderEditor::buildSaveAsWindow()
{
	ImGui::SetNextWindowSize(ImVec2(560.0f, 0.0f), ImGuiCond_Appearing);
	bool open = true;
	if (ImGui::Begin("Save Map As", &open, ImGuiWindowFlags_NoDocking))
	{
		ImGui::TextUnformatted("Path (the game looks for Maps\\<Name>\\<Name>.map):");
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputText("##path", &m_saveAsPath);
		if (ImGui::Button("Save") && !m_saveAsPath.empty())
		{
			saveMap(AsciiString(m_saveAsPath.c_str()));
			open = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
			open = false;
	}
	ImGui::End();
	if (!open)
		m_showSaveAs = FALSE;
}

void WorldBuilderEditor::buildMessagesWindow()
{
	bool open = true;
	if (ImGui::Begin("Messages", &open))
	{
		if (ImGui::Button("Clear"))
			m_messageLog.clear();
		ImGui::Separator();
		ImGui::BeginChild("log");
		for (size_t i = 0; i < m_messageLog.size(); ++i)
			ImGui::TextWrapped("%s", m_messageLog[i].c_str());
		ImGui::EndChild();
	}
	ImGui::End();
	m_showMessages = open;
}

void WorldBuilderEditor::buildStatusBar()
{
	ImGuiViewport *viewport = ImGui::GetMainViewport();
	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
	const float height = ImGui::GetFrameHeight();
	if (ImGui::BeginViewportSideBar("##StatusBar", viewport, ImGuiDir_Down, height, flags))
	{
		if (ImGui::BeginMenuBar())
		{
			if (m_hasCursorWorld)
				ImGui::Text("X %.1f  Y %.1f  Z %.1f", m_cursorWorld.x, m_cursorWorld.y, m_cursorWorld.z);
			else
				ImGui::TextUnformatted("Right drag: pan   Middle drag: rotate   Wheel: zoom   Home: reset camera");
			ImGui::EndMenuBar();
		}
	}
	ImGui::End();
}

} // namespace

EditorInterface *CreateWorldBuilderEditor()
{
	return new WorldBuilderEditor;
}

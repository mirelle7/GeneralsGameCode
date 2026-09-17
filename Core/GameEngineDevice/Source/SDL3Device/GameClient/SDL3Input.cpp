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

#include "Lib/BaseType.h"

#define _USE_MATH_DEFINES
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <SDL3/SDL.h>

#include "SDL3Device/GameClient/SDL3Input.h"
#include "Common/Debug.h"
#include "Common/file.h"
#include "Common/FileSystem.h"
#include "Common/GameAudio.h"
#include "Common/GameEngine.h"
#include "Common/MessageStream.h"
#include "Common/SeatManager.h"
#include "GameClient/Display.h"
#include "GameClient/InGameUI.h"
#include "GameClient/LookAtXlat.h"
#include "GameClient/GameWindowManager.h"
#include "GameLogic/GameLogic.h"
#include "SDL3Device/Common/SDL3GameEngine.h"
#include "SDL3Device/GameClient/SDL3Cursor.h"

SDL3InputManager* TheSDL3InputManager = nullptr;

static SDL_Scancode translateKeyValToScanCode(Int keyDef);

// SDL3Mouse implementation

SDL3Mouse::SDL3Mouse(SDL_Window* window)
	: Mouse()
	, m_Window(window)
	, m_IsCaptured(false)
	, m_IsVisible(true)
	, m_directionFrame(0)
	, m_accumulatedDeltaX(0.0f)
	, m_accumulatedDeltaY(0.0f)
	, m_activeSDLCursor(nullptr)
{
}

SDL3Mouse::~SDL3Mouse()
{
	releaseCapture();
}

void SDL3Mouse::init()
{
	Mouse::init();

	m_inputMovesAbsolute = true;

	// Show cursor by default
	setVisibility(true);
}

void SDL3Mouse::reset()
{
	Mouse::reset();

	releaseCapture();
	setVisibility(true);
}

void SDL3Mouse::update()
{
	Mouse::update();

	MouseCursor cursor = m_currentCursor;

	// Temporary probe for the "RMB scroll cursor stuck facing one way" report: this whole
	// direction mechanism is reported broken on the splitscreen branch but not the base SDL3
	// branch despite being byte-identical code here, so the divergence must be in one of these
	// INPUTS (isScrolling/getScrollAmount/accumulated deltas), not in the bucketing math itself.
	// Always on (no env gate) while this is being tracked down; seatLog itself already no-ops
	// unless splitscreen is enabled. Remove once the real cause is confirmed.
	const Bool cursorProbe = (cursor == SCROLL);

	if (cursor != NONE && cursor != INVALID_MOUSE_CURSOR && m_cursorInfo[cursor].numDirections > 1)
	{
		float dx = 0.0f;
		float dy = 0.0f;
		bool hasMovement = false;
		Bool usedScrollAmount = FALSE;

		if (cursor == SCROLL && TheInGameUI && TheInGameUI->isScrolling())
		{
			Coord2D scroll = TheInGameUI->getScrollAmount();
			if (scroll.x != 0.0f || scroll.y != 0.0f)
			{
				dx = scroll.x;
				dy = scroll.y;
				hasMovement = true;
				usedScrollAmount = TRUE;
			}
		}

		if (!hasMovement)
		{
			if (SDL_fabsf(m_accumulatedDeltaX) > 0.01f || SDL_fabsf(m_accumulatedDeltaY) > 0.01f)
			{
				dx = m_accumulatedDeltaX;
				dy = m_accumulatedDeltaY;
				hasMovement = true;

				m_accumulatedDeltaX = 0.0f;
				m_accumulatedDeltaY = 0.0f;
			}
		}

		if (hasMovement)
		{
			float angle = atan2f(dy, dx);
			if (angle < 0)
				angle += 2.0f * (float)M_PI;
			float segmentAngle = 2.0f * (float)M_PI / (float)m_cursorInfo[cursor].numDirections;
			m_directionFrame = (int)((angle + (segmentAngle / 2.0f)) / segmentAngle) % m_cursorInfo[cursor].numDirections;
		}

		if (cursorProbe)
		{
			seatLog("[GXCUR] isScrolling=%d viaScrollAmt=%d dx=%.3f dy=%.3f hasMovement=%d accumDX=%.3f accumDY=%.3f dirFrame=%d",
				(Int)(TheInGameUI && TheInGameUI->isScrolling()), (Int)usedScrollAmount,
				dx, dy, (Int)hasMovement, m_accumulatedDeltaX, m_accumulatedDeltaY, m_directionFrame);
		}
	}
	else
	{
		if (cursorProbe)
			seatLog("[GXCUR] direction branch NOT taken - numDirections=%d for cursor=%d", m_cursorInfo[cursor].numDirections, (Int)cursor);
		m_directionFrame = 0;
	}

	SDL_Cursor* requestedHandle = nullptr;
	bool bUseDefaultCursor = false;

	if (cursor == NONE || cursor == INVALID_MOUSE_CURSOR || !m_IsVisible)
	{
		bUseDefaultCursor = true;
	}
	else
	{
		requestedHandle = SDL3CursorManager::getCursor(cursor, m_directionFrame);
		if (!requestedHandle)
		{
			bUseDefaultCursor = true;
		}
	}

	if (bUseDefaultCursor)
	{
		requestedHandle = SDL3CursorManager::getCursor(NORMAL, 0);
		if (!requestedHandle)
		{
			requestedHandle = SDL_GetDefaultCursor();
		}
	}

	if (requestedHandle != m_activeSDLCursor)
	{
		SDL_SetCursor(requestedHandle);
		m_activeSDLCursor = requestedHandle;
	}
}

void SDL3Mouse::initCursorResources()
{
	SDL3CursorManager::initResources(this);
}

void SDL3Mouse::freeCursorResources()
{
	SDL3CursorManager::shutdown();
}

void SDL3Mouse::setCursor(MouseCursor cursor)
{
	if (m_currentCursor == cursor)
	{
		return;
	}

	Mouse::setCursor(cursor);
	m_currentCursor = cursor;
}

void SDL3Mouse::setVisibility(Bool visible)
{
	Mouse::setVisibility(visible);

	if (visible)
	{
		SDL_ShowCursor();
	}
	else
	{
		SDL_HideCursor();
	}
}

void SDL3Mouse::setPosition(Int x, Int y)
{
	// Update the engine cursor state.
	Mouse::setPosition(x, y);

	// Warp the OS cursor so the visible hardware cursor follows (used by the
	// splitscreen pad-driven seat, WP2). Convert game-internal coords to window
	// pixels; letterbox offset is ignored here (good enough for dev testing).
	if (!m_Window)
		return;

	int winW = 0, winH = 0;
	SDL_GetWindowSizeInPixels(m_Window, &winW, &winH);
	int intW = TheDisplay ? TheDisplay->getWidth()  : winW;
	int intH = TheDisplay ? TheDisplay->getHeight() : winH;

	float wx = (float)x;
	float wy = (float)y;
	if (intW > 0 && intH > 0 && (winW != intW || winH != intH))
	{
		wx = x * (float)winW / (float)intW;
		wy = y * (float)winH / (float)intH;
	}

	SDL_WarpMouseInWindow(m_Window, wx, wy);
}

void SDL3Mouse::confineToRegion(Int minX, Int minY, Int maxX, Int maxY)
{
	Mouse::confineToRegion(minX, minY, maxX, maxY);

	if (!m_Window)
		return;

	// Clip the OS cursor to the region (in window pixels). Scale from game-internal
	// coords, and treat a full-display region as "no confinement".
	int winW = 0, winH = 0;
	SDL_GetWindowSizeInPixels(m_Window, &winW, &winH);
	int intW = TheDisplay ? TheDisplay->getWidth()  : winW;
	int intH = TheDisplay ? TheDisplay->getHeight() : winH;

	SDL_Rect r;
	if (intW > 0 && intH > 0 && (winW != intW || winH != intH))
	{
		r.x = (int)(minX * (float)winW / (float)intW);
		r.y = (int)(minY * (float)winH / (float)intH);
		r.w = (int)((maxX - minX) * (float)winW / (float)intW);
		r.h = (int)((maxY - minY) * (float)winH / (float)intH);
	}
	else
	{
		r.x = minX; r.y = minY; r.w = maxX - minX; r.h = maxY - minY;
	}

	if (r.x <= 0 && r.y <= 0 && r.w >= winW && r.h >= winH)
		SDL_SetWindowMouseRect(m_Window, NULL); // full window => unconfined
	else
		SDL_SetWindowMouseRect(m_Window, &r);
}

void SDL3Mouse::loseFocus()
{
	Mouse::loseFocus();
	releaseCapture();
}

void SDL3Mouse::regainFocus()
{
	Mouse::regainFocus();
}

void SDL3Mouse::syncPositionToSystemCursor()
{
	if (!m_Window)
		return;

	float mx = 0.0f;
	float my = 0.0f;
	SDL_GetMouseState(&mx, &my);

	Uint32 windowID = SDL_GetWindowID(m_Window);
	int scaledX = (int)mx;
	int scaledY = (int)my;
	scaleMouseCoordinates((int)mx, (int)my, windowID, scaledX, scaledY);

	m_currMouse.pos.x = scaledX;
	m_currMouse.pos.y = scaledY;
	m_prevMouse.pos = m_currMouse.pos;
}

void SDL3Mouse::capture()
{
	if (!m_Window || m_isCursorCaptured)
	{
		return;
	}

	SDL_CaptureMouse(true);
	SDL_SetWindowMouseGrab(m_Window, true);
	onCursorCaptured(true);
}

void SDL3Mouse::releaseCapture()
{
	if (!m_isCursorCaptured)
	{
		return;
	}

	SDL_CaptureMouse(false);
	if (m_Window)
	{
		SDL_SetWindowMouseGrab(m_Window, false);
	}

	onCursorCaptured(false);
}

UnsignedByte SDL3Mouse::getMouseEvent(MouseIO* result, Bool flush)
{
	if (!TheSDL3InputManager)
	{
		return MOUSE_NONE;
	}

	SDL_Event nextEvent;
	if (!TheSDL3InputManager->getNextMouseEvent(nextEvent))
	{
		return MOUSE_NONE;
	}

	translateEvent(nextEvent, result);

	return MOUSE_OK;
}


// Unified event translation (Clean Slate Rewrite)
void SDL3Mouse::translateEvent(const SDL_Event& event, MouseIO* result)
{
	if (!result)
		return;

	// Reset state
	result->leftState = result->rightState = result->middleState = MBS_None;
	result->wheelPos = 0;
	result->deltaPos.x = result->deltaPos.y = 0;

	// Common timestamp (SDL3 uses nanoseconds, engine usually wants ms)
	result->time = (Uint32)(event.common.timestamp / 1000000);

	int rawX = 0;
	int rawY = 0;
	Uint32 windowID = 0;

	switch (event.type)
	{
		case SDL_EVENT_MOUSE_MOTION:
			rawX = (int)event.motion.x;
			rawY = (int)event.motion.y;
			windowID = event.motion.windowID;
			result->deltaPos.x = (Int)event.motion.xrel;
			result->deltaPos.y = (Int)event.motion.yrel;

			m_accumulatedDeltaX += event.motion.xrel;
			m_accumulatedDeltaY += event.motion.yrel;
			break;

		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP:
		{
			rawX = (int)event.button.x;
			rawY = (int)event.button.y;
			windowID = event.button.windowID;

			MouseButtonState state = event.button.down ? MBS_Down : MBS_Up;
			if (event.button.down && event.button.clicks >= 2)
				state = MBS_DoubleClick;

			if (event.button.button == SDL_BUTTON_LEFT)
				result->leftState = state;
			else if (event.button.button == SDL_BUTTON_RIGHT)
				result->rightState = state;
			else if (event.button.button == SDL_BUTTON_MIDDLE)
				result->middleState = state;
			break;
		}

		case SDL_EVENT_MOUSE_WHEEL:
		{
			rawX = (int)event.wheel.mouse_x;
			rawY = (int)event.wheel.mouse_y;
			windowID = event.wheel.windowID;
			result->wheelPos = (Int)(event.wheel.y * MOUSE_WHEEL_DELTA);
			break;
		}

		default:
			return;
	}

	// Dynamic Scaling Guard
	int scaledX, scaledY;
	scaleMouseCoordinates(rawX, rawY, windowID, scaledX, scaledY);
	result->pos.x = scaledX;
	result->pos.y = scaledY;
}

void SDL3Mouse::scaleMouseCoordinates(int rawX, int rawY, Uint32 windowID, int& scaledX, int& scaledY)
{
	SDL_Window* window = SDL_GetWindowFromID(windowID);
	if (!window || !TheDisplay)
	{
		scaledX = rawX;
		scaledY = rawY;
		return;
	}

	int winW = 0, winH = 0;
	// Mouse events are delivered in logical window points, so scale against logical window size
	SDL_GetWindowSize(window, &winW, &winH);

	int intW = TheDisplay->getWidth();
	int intH = TheDisplay->getHeight();

	// Guard: If we are at native resolution, bypass all math
	if (winW == intW && winH == intH)
	{
		scaledX = rawX;
		scaledY = rawY;
		return;
	}

	// Handle Viewport/Letterboxing if active
	int pbX, pbY, pbW, pbH;
	if (TheDisplay->getViewportRect(pbX, pbY, pbW, pbH))
	{
		int cx = std::max(0, std::min(pbW - 1, rawX - pbX));
		int cy = std::max(0, std::min(pbH - 1, rawY - pbY));
		scaledX = (int)(cx * (float)intW / pbW);
		scaledY = (int)(cy * (float)intH / pbH);
	}
	else
	{
		scaledX = (int)(rawX * (float)intW / winW);
		scaledY = (int)(rawY * (float)intH / winH);
	}
}

// SDL3Keyboard implementation

SDL3Keyboard::SDL3Keyboard()
	: Keyboard()
{}
SDL3Keyboard::~SDL3Keyboard() {}

void SDL3Keyboard::init() { Keyboard::init(); }
void SDL3Keyboard::reset() { Keyboard::reset(); }
void SDL3Keyboard::update() { Keyboard::update(); }

Bool SDL3Keyboard::getCapsState()
{
	return (SDL_GetModState() & SDL_KMOD_CAPS) != 0;
}

void SDL3Keyboard::getKey(KeyboardIO* key)
{
	if (!TheSDL3InputManager)
	{
		key->key = KEY_NONE;
		key->status = KeyboardIO::STATUS_UNUSED;
		return;
	}

	SDL_Event nextEvent;
	if (!TheSDL3InputManager->getNextKeyboardEvent(nextEvent))
	{
		key->key = KEY_NONE;
		key->status = KeyboardIO::STATUS_UNUSED;
		return;
	}

	const SDL_KeyboardEvent& keyEvent = nextEvent.key;
	KeyDefType keyDef = translateScanCodeToKeyVal(keyEvent.scancode);

	key->key = keyDef;
	key->status = KeyboardIO::STATUS_UNUSED;
	key->state = keyEvent.down ? KEY_STATE_DOWN : KEY_STATE_UP;
	key->keyDownTimeMsec = keyEvent.down ? (UnsignedInt)(keyEvent.timestamp / 1000000) : 0;

	SDL_Keymod mod = keyEvent.mod;
	if (mod & SDL_KMOD_LSHIFT)
		key->state |= KEY_STATE_LSHIFT;
	if (mod & SDL_KMOD_RSHIFT)
		key->state |= KEY_STATE_RSHIFT;
	if (mod & SDL_KMOD_LCTRL)
		key->state |= KEY_STATE_LCONTROL;
	if (mod & SDL_KMOD_RCTRL)
		key->state |= KEY_STATE_RCONTROL;
	if (mod & SDL_KMOD_LALT)
		key->state |= KEY_STATE_LALT;
	if (mod & SDL_KMOD_RALT)
		key->state |= KEY_STATE_RALT;
	if (mod & SDL_KMOD_CAPS)
		key->state |= KEY_STATE_CAPSLOCK;

	if (keyDef == KEY_LSHIFT)
		key->state &= ~KEY_STATE_LSHIFT;
	if (keyDef == KEY_RSHIFT)
		key->state &= ~KEY_STATE_RSHIFT;
	if (keyDef == KEY_LCTRL)
		key->state &= ~KEY_STATE_LCONTROL;
	if (keyDef == KEY_RCTRL)
		key->state &= ~KEY_STATE_RCONTROL;
	if (keyDef == KEY_LALT)
		key->state &= ~KEY_STATE_LALT;
	if (keyDef == KEY_RALT)
		key->state &= ~KEY_STATE_RALT;
}

KeyVal SDL3Keyboard::translateScanCodeToKeyVal(SDL_Scancode scan)
{
	switch (scan)
	{
		case SDL_SCANCODE_ESCAPE:
			return KEY_ESC;
		case SDL_SCANCODE_RETURN:
			return KEY_ENTER;
		case SDL_SCANCODE_KP_ENTER:
			return KEY_KPENTER;
		case SDL_SCANCODE_SPACE:
			return KEY_SPACE;
		case SDL_SCANCODE_TAB:
			return KEY_TAB;
		case SDL_SCANCODE_BACKSPACE:
			return KEY_BACKSPACE;
		case SDL_SCANCODE_DELETE:
			return KEY_DEL;
		case SDL_SCANCODE_HOME:
			return KEY_HOME;
		case SDL_SCANCODE_END:
			return KEY_END;
		case SDL_SCANCODE_PAGEUP:
			return KEY_PGUP;
		case SDL_SCANCODE_PAGEDOWN:
			return KEY_PGDN;
		case SDL_SCANCODE_INSERT:
			return KEY_INS;
		case SDL_SCANCODE_LSHIFT:
			return KEY_LSHIFT;
		case SDL_SCANCODE_RSHIFT:
			return KEY_RSHIFT;
		case SDL_SCANCODE_LCTRL:
			return KEY_LCTRL;
		case SDL_SCANCODE_RCTRL:
			return KEY_RCTRL;
		case SDL_SCANCODE_LALT:
			return KEY_LALT;
		case SDL_SCANCODE_RALT:
			return KEY_RALT;
		case SDL_SCANCODE_UP:
			return KEY_UP;
		case SDL_SCANCODE_DOWN:
			return KEY_DOWN;
		case SDL_SCANCODE_LEFT:
			return KEY_LEFT;
		case SDL_SCANCODE_RIGHT:
			return KEY_RIGHT;
		case SDL_SCANCODE_F1:
			return KEY_F1;
		case SDL_SCANCODE_F2:
			return KEY_F2;
		case SDL_SCANCODE_F3:
			return KEY_F3;
		case SDL_SCANCODE_F4:
			return KEY_F4;
		case SDL_SCANCODE_F5:
			return KEY_F5;
		case SDL_SCANCODE_F6:
			return KEY_F6;
		case SDL_SCANCODE_F7:
			return KEY_F7;
		case SDL_SCANCODE_F8:
			return KEY_F8;
		case SDL_SCANCODE_F9:
			return KEY_F9;
		case SDL_SCANCODE_F10:
			return KEY_F10;
		case SDL_SCANCODE_F11:
			return KEY_F11;
		case SDL_SCANCODE_F12:
			return KEY_F12;
		case SDL_SCANCODE_1:
			return KEY_1;
		case SDL_SCANCODE_2:
			return KEY_2;
		case SDL_SCANCODE_3:
			return KEY_3;
		case SDL_SCANCODE_4:
			return KEY_4;
		case SDL_SCANCODE_5:
			return KEY_5;
		case SDL_SCANCODE_6:
			return KEY_6;
		case SDL_SCANCODE_7:
			return KEY_7;
		case SDL_SCANCODE_8:
			return KEY_8;
		case SDL_SCANCODE_9:
			return KEY_9;
		case SDL_SCANCODE_0:
			return KEY_0;
		case SDL_SCANCODE_A:
			return KEY_A;
		case SDL_SCANCODE_B:
			return KEY_B;
		case SDL_SCANCODE_C:
			return KEY_C;
		case SDL_SCANCODE_D:
			return KEY_D;
		case SDL_SCANCODE_E:
			return KEY_E;
		case SDL_SCANCODE_F:
			return KEY_F;
		case SDL_SCANCODE_G:
			return KEY_G;
		case SDL_SCANCODE_H:
			return KEY_H;
		case SDL_SCANCODE_I:
			return KEY_I;
		case SDL_SCANCODE_J:
			return KEY_J;
		case SDL_SCANCODE_K:
			return KEY_K;
		case SDL_SCANCODE_L:
			return KEY_L;
		case SDL_SCANCODE_M:
			return KEY_M;
		case SDL_SCANCODE_N:
			return KEY_N;
		case SDL_SCANCODE_O:
			return KEY_O;
		case SDL_SCANCODE_P:
			return KEY_P;
		case SDL_SCANCODE_Q:
			return KEY_Q;
		case SDL_SCANCODE_R:
			return KEY_R;
		case SDL_SCANCODE_S:
			return KEY_S;
		case SDL_SCANCODE_T:
			return KEY_T;
		case SDL_SCANCODE_U:
			return KEY_U;
		case SDL_SCANCODE_V:
			return KEY_V;
		case SDL_SCANCODE_W:
			return KEY_W;
		case SDL_SCANCODE_X:
			return KEY_X;
		case SDL_SCANCODE_Y:
			return KEY_Y;
		case SDL_SCANCODE_Z:
			return KEY_Z;
		case SDL_SCANCODE_MINUS:
			return KEY_MINUS;
		case SDL_SCANCODE_EQUALS:
			return KEY_EQUAL;
		case SDL_SCANCODE_LEFTBRACKET:
			return KEY_LBRACKET;
		case SDL_SCANCODE_RIGHTBRACKET:
			return KEY_RBRACKET;
		case SDL_SCANCODE_SEMICOLON:
			return KEY_SEMICOLON;
		case SDL_SCANCODE_APOSTROPHE:
			return KEY_APOSTROPHE;
		case SDL_SCANCODE_GRAVE:
			return KEY_TICK;
		case SDL_SCANCODE_COMMA:
			return KEY_COMMA;
		case SDL_SCANCODE_PERIOD:
			return KEY_PERIOD;
		case SDL_SCANCODE_SLASH:
			return KEY_SLASH;
		case SDL_SCANCODE_BACKSLASH:
			return KEY_BACKSLASH;
		case SDL_SCANCODE_KP_1:
			return KEY_KP1;
		case SDL_SCANCODE_KP_2:
			return KEY_KP2;
		case SDL_SCANCODE_KP_3:
			return KEY_KP3;
		case SDL_SCANCODE_KP_4:
			return KEY_KP4;
		case SDL_SCANCODE_KP_5:
			return KEY_KP5;
		case SDL_SCANCODE_KP_6:
			return KEY_KP6;
		case SDL_SCANCODE_KP_7:
			return KEY_KP7;
		case SDL_SCANCODE_KP_8:
			return KEY_KP8;
		case SDL_SCANCODE_KP_9:
			return KEY_KP9;
		case SDL_SCANCODE_KP_0:
			return KEY_KP0;
		case SDL_SCANCODE_KP_PLUS:
			return KEY_KPPLUS;
		case SDL_SCANCODE_KP_MINUS:
			return KEY_KPMINUS;
		case SDL_SCANCODE_KP_MULTIPLY:
			return KEY_KPSTAR;
		case SDL_SCANCODE_KP_DIVIDE:
			return KEY_KPSLASH;
		case SDL_SCANCODE_KP_PERIOD:
			return KEY_KPDEL;
		case SDL_SCANCODE_CAPSLOCK:
			return KEY_CAPS;
		case SDL_SCANCODE_NUMLOCKCLEAR:
			return KEY_NUM;
		case SDL_SCANCODE_SCROLLLOCK:
			return KEY_SCROLL;
		case SDL_SCANCODE_PRINTSCREEN:
			return KEY_SYSREQ;
		default:
			return KEY_NONE;
	}
}

// SDL3InputManager implementation

SDL3InputManager::SDL3InputManager(SDL_Window* window)
	: m_window(window)
	, m_mouseNextFree(0)
	, m_mouseNextGet(0)
	, m_keyNextFree(0)
	, m_keyNextGet(0)
	, m_precisionMode(FALSE)
	, m_lastUpdateTime(0)
	, m_cursorSpeed(0.0f)
	, m_edgeAccelTimer(0.0f)
	, m_cursorRemainderX(0.0f)
	, m_cursorRemainderY(0.0f)
	, m_isQuitting(false)
{
	for (int i = 0; i < MAX_MOUSE_EVENTS; ++i)
		m_mouseEvents[i].type = SDL_EVENT_FIRST;
	for (int i = 0; i < MAX_KEY_EVENTS; ++i)
		m_keyEvents[i].type = SDL_EVENT_FIRST;
	TheSDL3InputManager = this;

	// TheSeatManager is a device-independent subsystem created later in
	// GameEngine::init; it may not exist yet. Opening pads here only populates
	// the local table - seat binding happens once joining is allowed.
	openAllGamepads();
	m_lastUpdateTime = SDL_GetTicks();
}

SDL3InputManager::~SDL3InputManager()
{
	closeAllGamepads();
	SDL3Mouse::freeCursorResources();
	TheSDL3InputManager = nullptr;
}

void SDL3InputManager::update()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_EVENT_QUIT:
			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
				if (TheMessageStream && TheMessageStream->isReadyForMessages())
				{
					TheMessageStream->appendMessage(GameMessage::MSG_META_DEMO_INSTANT_QUIT);
				}
				else
				{
					m_isQuitting = true;
				}
				break;

			case SDL_EVENT_GAMEPAD_ADDED:
				openGamepad(event.gdevice.which);
				break;

			case SDL_EVENT_GAMEPAD_REMOVED:
				closeGamepad(event.gdevice.which);
				break;

			case SDL_EVENT_WINDOW_FOCUS_GAINED:
				if (TheGameEngine)
					TheGameEngine->setIsActive(true);
				if (TheKeyboard)
					TheKeyboard->resetKeys();
				if (TheMouse)
				{
					TheMouse->regainFocus();
					TheMouse->refreshCursorCapture();
					TheMouse->syncPositionToSystemCursor();
				}
				if (TheAudio)
					TheAudio->unmuteAudio(AudioManager::MuteAudioReason_WindowFocus);
				break;

			case SDL_EVENT_WINDOW_FOCUS_LOST:
				if (TheGameEngine)
					TheGameEngine->setIsActive(false);
				if (TheKeyboard)
					TheKeyboard->resetKeys();
				if (TheMouse)
					TheMouse->loseFocus();
				if (TheAudio)
					TheAudio->muteAudio(AudioManager::MuteAudioReason_WindowFocus);
				break;

			case SDL_EVENT_WINDOW_MOUSE_ENTER:
				if (TheMouse)
					TheMouse->onCursorMovedInside();
				break;

			case SDL_EVENT_WINDOW_MOUSE_LEAVE:
				if (TheMouse)
					TheMouse->onCursorMovedOutside();
				break;

			case SDL_EVENT_MOUSE_MOTION:
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
			case SDL_EVENT_MOUSE_BUTTON_UP:
			case SDL_EVENT_MOUSE_WHEEL:
				addMouseSDLEvent(event);
				break;

			case SDL_EVENT_KEY_DOWN:
			case SDL_EVENT_KEY_UP:
				if (!event.key.repeat)
				{
					addKeyboardSDLEvent(event);
					if (event.key.down && (event.key.scancode == SDL_SCANCODE_RETURN || event.key.scancode == SDL_SCANCODE_KP_ENTER))
					{
						if (TheWindowManager)
						{
							GameWindow* focus = TheWindowManager->winGetFocus();
							if (focus)
							{
								const UnsignedInt style = focus->winGetStyle();
								if (BitIsSet(style, GWS_ENTRY_FIELD) || BitIsSet(style, GWS_COMBO_BOX))
								{
									TheWindowManager->winSendInputMsg(focus, GWM_IME_CHAR, 13, 0);
								}
							}
						}
					}
				}
				break;

			case SDL_EVENT_TEXT_INPUT:
				if (TheGameEngine)
				{
					SDL3GameEngine* engine = dynamic_cast<SDL3GameEngine*>(TheGameEngine);
					if (engine)
						engine->forwardTextInputEvent(event.text.text);
				}
				break;

			default:
				break;
		}
	}

	processGamepadInput();
}

Bool SDL3InputManager::getNextMouseEvent(SDL_Event& outEvent)
{
	if (m_mouseNextGet == m_mouseNextFree)
		return false;

	SDL_Event* event = &m_mouseEvents[m_mouseNextGet];
	m_mouseNextGet = (m_mouseNextGet + 1) % MAX_MOUSE_EVENTS;

	outEvent = *event;
	event->type = SDL_EVENT_FIRST;
	return true;
}

Bool SDL3InputManager::getNextKeyboardEvent(SDL_Event& outEvent)
{
	if (m_keyNextGet == m_keyNextFree)
		return false;

	SDL_Event* event = &m_keyEvents[m_keyNextGet];
	m_keyNextGet = (m_keyNextGet + 1) % MAX_KEY_EVENTS;

	outEvent = *event;
	event->type = SDL_EVENT_FIRST;
	return true;
}

void SDL3InputManager::addMouseSDLEvent(const SDL_Event& event)
{
	UnsignedInt nextFree = (m_mouseNextFree + 1) % MAX_MOUSE_EVENTS;
	if (nextFree == m_mouseNextGet)
		return;
	m_mouseEvents[m_mouseNextFree] = event;
	m_mouseNextFree = nextFree;
}

void SDL3InputManager::addKeyboardSDLEvent(const SDL_Event& event)
{
	UnsignedInt nextFree = (m_keyNextFree + 1) % MAX_KEY_EVENTS;
	if (nextFree == m_keyNextGet)
		return;
	m_keyEvents[m_keyNextFree] = event;
	m_keyNextFree = nextFree;
}

void SDL3InputManager::openGamepad(SDL_JoystickID id)
{
	if (id == 0 || m_pads.find(id) != m_pads.end())
		return;

	SDL_Gamepad* pad = SDL_OpenGamepad(id);
	if (!pad)
		return;

	PadEntry entry;
	entry.pad = pad;
	m_pads[id] = entry;
	DEBUG_LOG(("SDL3InputManager: Opened gamepad %u: %s", id, SDL_GetGamepadName(pad)));
	// Which seat this pad serves is the seat layer's call, made per frame in
	// processGamepadInput - there is no device role to assign here.
}

void SDL3InputManager::closeGamepad(SDL_JoystickID id)
{
	std::map<SDL_JoystickID, PadEntry>::iterator it = m_pads.find(id);
	if (it == m_pads.end())
		return;

	if (it->second.pad)
		SDL_CloseGamepad(it->second.pad);
	m_pads.erase(it);

	// Hand the unplug to the seat layer, which owns the device population: it releases
	// the seat-0 role if this pad held it, so a remaining pad can take over the mouse.
	if (TheSeatManager)
		TheSeatManager->onDeviceRemoved((Int)id);
}

void SDL3InputManager::openAllGamepads()
{
	int count = 0;
	SDL_JoystickID* joysticks = SDL_GetGamepads(&count);
	if (joysticks)
	{
		for (int i = 0; i < count; ++i)
			openGamepad(joysticks[i]);
		SDL_free(joysticks);
	}
}

void SDL3InputManager::releasePadVirtualInputs(PadEntry& entry)
{
	GamepadState& state = entry.injectState;

	if (state.rtDown)
		virtualPulseKey(SDL_SCANCODE_LCTRL, false);

	if (state.stickLeft) virtualPulseKey(SDL_SCANCODE_LEFT, false);
	if (state.stickRight) virtualPulseKey(SDL_SCANCODE_RIGHT, false);
	if (state.stickUp) virtualPulseKey(SDL_SCANCODE_UP, false);
	if (state.stickDown) virtualPulseKey(SDL_SCANCODE_DOWN, false);

	// Release any logical button this pad was holding through the shared binding table
	// (see injectLegacyMouseKeyboard) - GamepadState no longer tracks per-button state
	// itself now that edges come from the seat layer's SeatInputState.
	for (Int b = 0; b < SEAT_BUTTON_COUNT; ++b)
	{
		if (!entry.prevLogical[b])
			continue;

		const SeatButtonBinding& bind = getSeatButtonBinding((SeatButton)b);
		switch (bind.m_action)
		{
			case SEAT_ACT_CLICK_LEFT:
				virtualPulseMouse(SDL_BUTTON_LEFT, false);
				break;
			case SEAT_ACT_CLICK_RIGHT:
				virtualPulseMouse(SDL_BUTTON_RIGHT, false);
				break;
			case SEAT_ACT_KEY:
			case SEAT_ACT_SHIFT_KEY:
			{
				const SDL_Scancode sc = translateKeyValToScanCode(bind.m_key);
				if (sc != SDL_SCANCODE_UNKNOWN)
					virtualPulseKey(sc, false);
				break;
			}
			case SEAT_ACT_META:
			case SEAT_ACT_NONE:
			default:
				break;
		}
	}

	entry.injectState = GamepadState();
	memset(entry.prevLogical, 0, sizeof(entry.prevLogical));
}

void SDL3InputManager::closeAllGamepads()
{
	// No persistent "primary" pad anymore (the seat layer assigns seat 0 to a pad
	// per frame), so release virtual input for every pad's inject state; entries that
	// never drove the OS mouse/keyboard have all-false state and the guards no-op.
	for (std::map<SDL_JoystickID, PadEntry>::iterator it = m_pads.begin(); it != m_pads.end(); ++it)
		releasePadVirtualInputs(it->second);

	for (std::map<SDL_JoystickID, PadEntry>::iterator it = m_pads.begin(); it != m_pads.end(); ++it)
	{
		if (it->second.pad)
			SDL_CloseGamepad(it->second.pad);
	}
	m_pads.clear();
	m_lastUpdateTime = 0;

	m_cursorSpeed = 0.0f;
	m_edgeAccelTimer = 0.0f;
	m_cursorRemainderX = 0.0f;
	m_cursorRemainderY = 0.0f;

	if (TheLookAtTranslator)
		TheLookAtTranslator->setControllerInputActive(false);
}

void SDL3InputManager::virtualPulseKey(SDL_Scancode scancode, bool down)
{
	SDL_Event keyEvent;
	memset(&keyEvent, 0, sizeof(keyEvent));
	keyEvent.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
	keyEvent.common.timestamp = SDL_GetTicksNS();
	keyEvent.key.scancode = scancode;
	keyEvent.key.down = down;

	if (scancode == SDL_SCANCODE_LCTRL)
		keyEvent.key.mod = SDL_KMOD_LCTRL;
	else if (scancode == SDL_SCANCODE_LSHIFT)
		keyEvent.key.mod = SDL_KMOD_LSHIFT;
	else if (scancode == SDL_SCANCODE_LALT)
		keyEvent.key.mod = SDL_KMOD_LALT;

	addKeyboardSDLEvent(keyEvent);
}

void SDL3InputManager::virtualPulseMouse(Uint8 button, bool down)
{
	static Uint64 lastClickTime[3] = {0, 0, 0};
	Uint64 now = SDL_GetTicks();

	SDL_Event clickEvent;
	memset(&clickEvent, 0, sizeof(clickEvent));
	clickEvent.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
	clickEvent.common.timestamp = SDL_GetTicksNS();
	clickEvent.button.button = button;

	int buttonIdx = (button == SDL_BUTTON_LEFT) ? 0 : ((button == SDL_BUTTON_RIGHT) ? 1 : 2);
	if (down)
	{
		if (now - lastClickTime[buttonIdx] < 350)
			clickEvent.button.clicks = 2;
		else
			clickEvent.button.clicks = 1;
		lastClickTime[buttonIdx] = now;
	}
	else
	{
		clickEvent.button.clicks = 1;
	}

	clickEvent.button.down = down;

	float mx, my;
	SDL_GetMouseState(&mx, &my);
	clickEvent.button.x = mx;
	clickEvent.button.y = my;

	if (m_window)
	{
		clickEvent.button.windowID = SDL_GetWindowID(m_window);
	}

	addMouseSDLEvent(clickEvent);
}

void SDL3InputManager::handleGamepadButton(SDL_GamepadButton button, bool& currentState, bool isDown, std::function<void(bool)> action)
{
	if (isDown != currentState)
	{
		action(isDown);
		currentState = isDown;
	}
}

void SDL3InputManager::processGamepadInput()
{
	if (m_window && !(SDL_GetWindowFlags(m_window) & SDL_WINDOW_INPUT_FOCUS))
	{
		for (std::map<SDL_JoystickID, PadEntry>::iterator it = m_pads.begin(); it != m_pads.end(); ++it)
			releasePadVirtualInputs(it->second);
		m_lastUpdateTime = 0;
		m_cursorSpeed = 0.0f;
		m_edgeAccelTimer = 0.0f;
		m_cursorRemainderX = 0.0f;
		m_cursorRemainderY = 0.0f;
		if (TheLookAtTranslator)
			TheLookAtTranslator->setControllerInputActive(false);
		return;
	}

	Uint64 now = SDL_GetTicks();
	float deltaTime = (now - m_lastUpdateTime) / 1000.0f;
	m_lastUpdateTime = now;
	if (deltaTime > 0.1f)
		deltaTime = 0.1f;

	if (TheSeatManager)
		TheSeatManager->setConnectedDeviceCount((Int)m_pads.size());

	const float DEADZONE = DEFAULT_DEADZONE;
	bool hasStickInput = false;

	for (std::map<SDL_JoystickID, PadEntry>::iterator it = m_pads.begin(); it != m_pads.end(); ++it)
	{
		PadEntry& entry = it->second;
		if (!entry.pad)
			continue;

		SeatInputState state;
		readGamepadState(entry.pad, entry, state);

		// The seat layer owns every pad and decides which seat this one belongs to; the
		// answer is the only thing this backend branches on. There is no longer a separate
		// "legacy" pad path running its own admission rules in parallel - injecting the
		// mouse/keyboard is simply what seat 0 does with a pad.
		const Int seat = (TheSeatManager != nullptr)
			? TheSeatManager->routeDeviceInput((Int)it->first, state)
			: 0;	// no seat layer (VC6/non-SDL3 configurations): behave as seat 0

		if (seat == 0)
		{
			// Seat 0 == the keyboard/mouse seat. Its pad drives the OS pointer with the
			// full legacy button mapping, so single-player and menu navigation are
			// unchanged whether or not splitscreen is enabled.
			injectLegacyMouseKeyboard(entry, state, deltaTime);

			float rx = SDL_GetGamepadAxis(entry.pad, SDL_GAMEPAD_AXIS_RIGHTX) / AXIS_MAX;
			float ry = SDL_GetGamepadAxis(entry.pad, SDL_GAMEPAD_AXIS_RIGHTY) / AXIS_MAX;
			float lx_axis = SDL_GetGamepadAxis(entry.pad, SDL_GAMEPAD_AXIS_LEFTX) / AXIS_MAX;
			float ly_axis = SDL_GetGamepadAxis(entry.pad, SDL_GAMEPAD_AXIS_LEFTY) / AXIS_MAX;
			float stickMag_check = sqrtf(lx_axis * lx_axis + ly_axis * ly_axis);
			hasStickInput = (stickMag_check > DEADZONE) || (SDL_fabsf(rx) > DEADZONE) || (SDL_fabsf(ry) > DEADZONE);
		}
		// seat > 0: driven entirely by SeatManager::createStreamMessages (own cursor,
		// seat-tagged messages) - the backend must not inject anything for it.
		// seat < 0: another pad is already acting for seat 0; this one idles until it joins.
	}

	if (TheLookAtTranslator)
		TheLookAtTranslator->setControllerInputActive(hasStickInput);
}

void SDL3InputManager::readGamepadState(SDL_Gamepad* pad, PadEntry& entry, SeatInputState& out) const
{
	out.clear();
	if (!pad)
		return;

	const float DEADZONE = DEFAULT_DEADZONE;

	float lx = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTX)  / AXIS_MAX;
	float ly = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTY)  / AXIS_MAX;
	float rx = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_RIGHTX) / AXIS_MAX;
	float ry = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_RIGHTY) / AXIS_MAX;
	out.leftX  = (SDL_fabsf(lx) > DEADZONE) ? lx : 0.0f;
	out.leftY  = (SDL_fabsf(ly) > DEADZONE) ? ly : 0.0f;
	out.rightX = (SDL_fabsf(rx) > DEADZONE) ? rx : 0.0f;
	out.rightY = (SDL_fabsf(ry) > DEADZONE) ? ry : 0.0f;
	out.leftTrigger  = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER)  / AXIS_MAX;
	out.rightTrigger = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) / AXIS_MAX;

	// Map physical gamepad buttons to engine-side logical buttons (SeatInput.h).
	static const SDL_GamepadButton s_logicalMap[SEAT_BUTTON_COUNT] =
	{
		SDL_GAMEPAD_BUTTON_SOUTH,           // SEAT_BUTTON_CONFIRM
		SDL_GAMEPAD_BUTTON_EAST,            // SEAT_BUTTON_CANCEL
		SDL_GAMEPAD_BUTTON_WEST,            // SEAT_BUTTON_ACTION
		SDL_GAMEPAD_BUTTON_NORTH,           // SEAT_BUTTON_ALT_ACTION
		SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,   // SEAT_BUTTON_MODIFIER
		SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,  // SEAT_BUTTON_COMMAND_BAR
		SDL_GAMEPAD_BUTTON_START,           // SEAT_BUTTON_JOIN
		SDL_GAMEPAD_BUTTON_BACK,            // SEAT_BUTTON_LEAVE
		SDL_GAMEPAD_BUTTON_LEFT_STICK,      // SEAT_BUTTON_CURSOR_CLICK
		SDL_GAMEPAD_BUTTON_RIGHT_STICK,     // SEAT_BUTTON_CAMERA_RESET
		SDL_GAMEPAD_BUTTON_DPAD_UP,         // SEAT_BUTTON_DPAD_UP
		SDL_GAMEPAD_BUTTON_DPAD_DOWN,       // SEAT_BUTTON_DPAD_DOWN
		SDL_GAMEPAD_BUTTON_DPAD_LEFT,       // SEAT_BUTTON_DPAD_LEFT
		SDL_GAMEPAD_BUTTON_DPAD_RIGHT,      // SEAT_BUTTON_DPAD_RIGHT
	};

	for (Int i = 0; i < SEAT_BUTTON_COUNT; ++i)
	{
		bool down = SDL_GetGamepadButton(pad, s_logicalMap[i]);
		out.buttonDown[i]     = down ? TRUE : FALSE;
		out.buttonPressed[i]  = (down && !entry.prevLogical[i]) ? TRUE : FALSE;
		out.buttonReleased[i] = (!down && entry.prevLogical[i]) ? TRUE : FALSE;
		entry.prevLogical[i]  = down;
	}
}

// The engine speaks KeyDefs (DIK_*) and this backend speaks SDL scancodes, so delivering a
// binding's keystroke as a real OS event needs the inverse of translateScanCodeToKeyVal. Only the
// keys the binding table actually uses are here; anything else is reported rather than silently
// dropped, because a keystroke that goes nowhere is invisible - it is exactly how three pad buttons
// managed to look wired while doing nothing at all.
static SDL_Scancode translateKeyValToScanCode(Int keyDef)
{
	switch (keyDef)
	{
		case KEY_A:      return SDL_SCANCODE_A;
		case KEY_Q:      return SDL_SCANCODE_Q;
		case KEY_LSHIFT: return SDL_SCANCODE_LSHIFT;
		case KEY_LCTRL:  return SDL_SCANCODE_LCTRL;
		case KEY_ESC:    return SDL_SCANCODE_ESCAPE;
		case KEY_SPACE:  return SDL_SCANCODE_SPACE;
		case KEY_1:      return SDL_SCANCODE_1;
		case KEY_2:      return SDL_SCANCODE_2;
		case KEY_3:      return SDL_SCANCODE_3;
		case KEY_4:      return SDL_SCANCODE_4;
		default:
			DEBUG_CRASH(("translateKeyValToScanCode: pad binding uses key %d, which has no scancode "
				"here - seat 0's pad will do nothing for that button", keyDef));
			return SDL_SCANCODE_UNKNOWN;
	}
}

void SDL3InputManager::injectLegacyMouseKeyboard(PadEntry& entry, const SeatInputState& state,
	float deltaTime)
{
	SDL_Gamepad* pad = entry.pad;
	const float DEADZONE = DEFAULT_DEADZONE;

	float resolutionScale = 1.0f;
	int windowWidth = 0;
	int windowHeight = 0;
	if (m_window && SDL_GetWindowSize(m_window, &windowWidth, &windowHeight) && windowHeight > 0)
		resolutionScale = (float)windowHeight / DESIGNED_WINDOW_HEIGHT;

	const float CURSOR_SPEED = DEFAULT_CURSOR_SPEED * resolutionScale;
	const float CURSOR_ACCELERATION = DEFAULT_CURSOR_ACCELERATION * resolutionScale;
	const float CURSOR_DECELERATION = DEFAULT_CURSOR_DECELERATION * resolutionScale;

	// 1. TRIGGERS (Modifiers: RT = Force Attack / LCtrl, LT = Precision)
	bool ltPressed = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > TRIGGER_THRESHOLD;
	if (ltPressed != entry.injectState.ltDown)
	{
		entry.injectState.ltDown = ltPressed;
		m_precisionMode = entry.injectState.ltDown;
	}

	bool rtPressed = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > TRIGGER_THRESHOLD;
	if (rtPressed != entry.injectState.rtDown)
	{
		entry.injectState.rtDown = rtPressed;
		virtualPulseKey(SDL_SCANCODE_LCTRL, entry.injectState.rtDown);
	}

	// 2. STICKS (Movement & Panning)
	float lx = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTX) / AXIS_MAX;
	float ly = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTY) / AXIS_MAX;
	float stickMagnitude = sqrtf(lx * lx + ly * ly);
	if (stickMagnitude > 1.0f)
		stickMagnitude = 1.0f;

	if (stickMagnitude > DEADZONE)
	{
		// Instant polar direction matching exact stick angle (0 rotational lag, 0 loops)
		float dirX = lx / stickMagnitude;
		float dirY = ly / stickMagnitude;

		// Radially remove the deadzone, then use a smoothstep response curve.
		float response = (stickMagnitude - DEADZONE) / (1.0f - DEADZONE);
		response = response * response * (3.0f - 2.0f * response);

		float targetSpeed = CURSOR_SPEED * response;

		// Edge acceleration boost (1.75x speed after holding outer edge for > 0.25s)
		const float EDGE_ACCEL_THRESHOLD = 0.85f;
		const float EDGE_ACCEL_DELAY = 0.25f;
		const float EDGE_ACCEL_RAMP_TIME = 0.35f;
		const float MAX_BOOST_MULTIPLIER = 1.75f;

		if (stickMagnitude > EDGE_ACCEL_THRESHOLD)
		{
			m_edgeAccelTimer += deltaTime;
			if (m_edgeAccelTimer > EDGE_ACCEL_DELAY)
			{
				float rampProgress = (m_edgeAccelTimer - EDGE_ACCEL_DELAY) / EDGE_ACCEL_RAMP_TIME;
				if (rampProgress > 1.0f)
					rampProgress = 1.0f;
				targetSpeed *= (1.0f + (MAX_BOOST_MULTIPLIER - 1.0f) * rampProgress);
			}
		}
		else
		{
			m_edgeAccelTimer = 0.0f;
		}

		// Smooth scalar speed acceleration
		if (m_cursorSpeed < targetSpeed)
		{
			m_cursorSpeed += CURSOR_ACCELERATION * deltaTime;
			if (m_cursorSpeed > targetSpeed)
				m_cursorSpeed = targetSpeed;
		}
		else if (m_cursorSpeed > targetSpeed)
		{
			m_cursorSpeed -= CURSOR_DECELERATION * deltaTime;
			if (m_cursorSpeed < targetSpeed)
				m_cursorSpeed = targetSpeed;
		}

		// Instant direction * smooth accelerated speed
		float velocityX = dirX * m_cursorSpeed;
		float velocityY = dirY * m_cursorSpeed;

		m_cursorRemainderX += velocityX * deltaTime;
		m_cursorRemainderY += velocityY * deltaTime;
		int cursorDeltaX = (int)m_cursorRemainderX;
		int cursorDeltaY = (int)m_cursorRemainderY;
		m_cursorRemainderX -= cursorDeltaX;
		m_cursorRemainderY -= cursorDeltaY;

		if (cursorDeltaX != 0 || cursorDeltaY != 0)
		{
			float mx, my;
			SDL_GetMouseState(&mx, &my);
			SDL_WarpMouseInWindow(m_window, mx + cursorDeltaX, my + cursorDeltaY);
		}
	}
	else
	{
		// Instant stop on release (0 speed, 0 timer, 0 remainder)
		m_cursorSpeed = 0.0f;
		m_edgeAccelTimer = 0.0f;
		m_cursorRemainderX = 0.0f;
		m_cursorRemainderY = 0.0f;
	}

	float rx = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_RIGHTX) / AXIS_MAX;
	float ry = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_RIGHTY) / AXIS_MAX;

	handleGamepadButton(
		SDL_GAMEPAD_BUTTON_INVALID,
		entry.injectState.stickLeft,
		rx < -DEADZONE,
		[this](bool d) { virtualPulseKey(SDL_SCANCODE_LEFT, d); }
	);
	handleGamepadButton(
		SDL_GAMEPAD_BUTTON_INVALID,
		entry.injectState.stickRight,
		rx > DEADZONE,
		[this](bool d) { virtualPulseKey(SDL_SCANCODE_RIGHT, d); }
	);
	handleGamepadButton(
		SDL_GAMEPAD_BUTTON_INVALID,
		entry.injectState.stickUp,
		ry < -DEADZONE,
		[this](bool d) { virtualPulseKey(SDL_SCANCODE_UP, d); }
	);
	handleGamepadButton(
		SDL_GAMEPAD_BUTTON_INVALID,
		entry.injectState.stickDown,
		ry > DEADZONE,
		[this](bool d) { virtualPulseKey(SDL_SCANCODE_DOWN, d); }
	);

	// 3. BUTTONS & D-PAD (Actions & Hotkeys)
	//
	// Driven entirely by the shared binding table. This used to be fourteen hand-written
	// handleGamepadButton blocks, and a second copy of the same decisions lived in SeatManager for
	// seats 1..7 - which is how the shoulder buttons ended up swapped between the two and three
	// buttons ended up bound to keystrokes that do nothing. What each button MEANS is now decided
	// in one place; this function only decides how to DELIVER it, which for seat 0 means real OS
	// events, because seat 0 is the one that owns the pointer.
	//
	// Edges come from the logical state the caller already read for this pad, rather than from a
	// second edge-detector - one fewer thing that can disagree.
	for (Int b = 0; b < SEAT_BUTTON_COUNT; ++b)
	{
		if (!state.buttonPressed[b] && !state.buttonReleased[b])
			continue;

		const SeatButtonBinding& bind = getSeatButtonBinding((SeatButton)b);
		const bool down = state.buttonPressed[b] ? true : false;

		switch (bind.m_action)
		{
			case SEAT_ACT_CLICK_LEFT:
				virtualPulseMouse(SDL_BUTTON_LEFT, down);
				break;

			case SEAT_ACT_CLICK_RIGHT:
				virtualPulseMouse(SDL_BUTTON_RIGHT, down);
				break;

			case SEAT_ACT_KEY:
			case SEAT_ACT_SHIFT_KEY:
			{
				const SDL_Scancode sc = translateKeyValToScanCode(bind.m_key);
				if (sc != SDL_SCANCODE_UNKNOWN)
					virtualPulseKey(sc, down);
				break;
			}

			case SEAT_ACT_META:
				if (down && TheMessageStream)
					TheMessageStream->appendMessage((GameMessage::Type)bind.m_meta);
				break;

			case SEAT_ACT_NONE:
			default:
				break;
		}
	}
}

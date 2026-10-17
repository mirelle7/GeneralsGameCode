/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2026 TheSuperHackers
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
*/

// TheSuperHackers @feature githubawn 16/10/2026 Gamepad -> emulated mouse/keyboard
// for the SDL3 backend. Mapping mirrors the PC SDL3 input layer's seat-0 bindings:
//   left stick  cursor (LT held = precision)     right stick  camera scroll (arrows)
//   A / B       left / right click               X / Y        attack-move (A) / stop (S)
//   LB          Q (select matching)              RB held      Shift (queue / add)
//   RT held     Ctrl (force fire)                Start / Back Esc / Space
//   R3          H (home base)                    D-pad        control groups 1-4

#if defined(SAGE_USE_SDL3)

#include "SDL3Device/GameClient/SDL3Gamepad.h"
#include "SDL3Device/GameClient/SDL3Keyboard.h"
#include "SDL3Device/GameClient/SDL3Mouse.h"

#include <cmath>
#include <cstdio>
#include <cstring>

extern SDL_Window *TheSDL3Window;

namespace
{
	enum ActionType
	{
		ACT_NONE,
		ACT_LEFT_CLICK,
		ACT_RIGHT_CLICK,
		ACT_KEY,
	};

	struct ButtonBinding
	{
		SDL_GamepadButton button;
		ActionType action;
		SDL_Scancode key;
	};

	const ButtonBinding kBindings[] =
	{
		{ SDL_GAMEPAD_BUTTON_SOUTH,          ACT_LEFT_CLICK,  SDL_SCANCODE_UNKNOWN },
		{ SDL_GAMEPAD_BUTTON_EAST,           ACT_RIGHT_CLICK, SDL_SCANCODE_UNKNOWN },
		{ SDL_GAMEPAD_BUTTON_WEST,           ACT_KEY,         SDL_SCANCODE_A },
		{ SDL_GAMEPAD_BUTTON_NORTH,          ACT_KEY,         SDL_SCANCODE_S },
		{ SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,  ACT_KEY,         SDL_SCANCODE_Q },
		{ SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, ACT_KEY,         SDL_SCANCODE_LSHIFT },
		{ SDL_GAMEPAD_BUTTON_START,          ACT_KEY,         SDL_SCANCODE_ESCAPE },
		{ SDL_GAMEPAD_BUTTON_BACK,           ACT_KEY,         SDL_SCANCODE_SPACE },
		{ SDL_GAMEPAD_BUTTON_RIGHT_STICK,    ACT_KEY,         SDL_SCANCODE_H },
		{ SDL_GAMEPAD_BUTTON_DPAD_LEFT,      ACT_KEY,         SDL_SCANCODE_1 },
		{ SDL_GAMEPAD_BUTTON_DPAD_UP,        ACT_KEY,         SDL_SCANCODE_2 },
		{ SDL_GAMEPAD_BUTTON_DPAD_RIGHT,     ACT_KEY,         SDL_SCANCODE_3 },
		{ SDL_GAMEPAD_BUTTON_DPAD_DOWN,      ACT_KEY,         SDL_SCANCODE_4 },
	};
	const int kNumBindings = sizeof(kBindings) / sizeof(kBindings[0]);

	const SDL_Scancode kScrollKeys[4] = { SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN };

	const float kAxisMax = 32767.0f;
	const float kStickDeadzone = 0.18f;
	const float kScrollThreshold = 0.5f;
	const float kTriggerThreshold = 0.5f;
	// Full-deflection cursor speed in pixels per second at 1080 lines; scaled with
	// the window height so it feels the same at any resolution.
	const float kCursorSpeed = 1100.0f;
	const float kPrecisionScale = 0.35f;
}

SDL3Gamepad::SDL3Gamepad()
	: m_pad(NULL)
	, m_initialized(false)
	, m_ctrlDown(false)
	, m_cursorX(-1.0f)
	, m_cursorY(-1.0f)
	, m_speed(0.0f)
	, m_lastTicks(0)
{
	memset(m_buttonDown, 0, sizeof(m_buttonDown));
	memset(m_scrollKeyDown, 0, sizeof(m_scrollKeyDown));
}

SDL3Gamepad::~SDL3Gamepad()
{
	if (m_pad != NULL)
	{
		SDL_CloseGamepad(m_pad);
		m_pad = NULL;
	}
}

void SDL3Gamepad::openAll()
{
	int count = 0;
	SDL_JoystickID *ids = SDL_GetGamepads(&count);
	if (ids != NULL)
	{
		for (int i = 0; i < count && m_pad == NULL; ++i)
		{
			open(ids[i]);
		}
		SDL_free(ids);
	}
}

void SDL3Gamepad::open(SDL_JoystickID id)
{
	if (m_pad != NULL)
	{
		return;
	}
	m_pad = SDL_OpenGamepad(id);
	if (m_pad != NULL)
	{
		printf("[ggc] gamepad opened: %s\n", SDL_GetGamepadName(m_pad));
		fflush(stdout);
	}
}

void SDL3Gamepad::close(SDL_JoystickID id)
{
	if (m_pad != NULL && SDL_GetGamepadID(m_pad) == id)
	{
		SDL_CloseGamepad(m_pad);
		m_pad = NULL;
		openAll();
	}
}

void SDL3Gamepad::handleEvent(const SDL_Event &event)
{
	if (event.type == SDL_EVENT_GAMEPAD_ADDED)
	{
		open(event.gdevice.which);
	}
	else if (event.type == SDL_EVENT_GAMEPAD_REMOVED)
	{
		close(event.gdevice.which);
	}
}

void SDL3Gamepad::sendKey(SDL3Keyboard *keyboard, SDL_Scancode scancode, bool down)
{
	if (keyboard == NULL)
	{
		return;
	}
	SDL_KeyboardEvent ev;
	memset(&ev, 0, sizeof(ev));
	ev.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
	ev.timestamp = SDL_GetTicksNS();
	ev.scancode = scancode;
	ev.down = down;
	keyboard->addSDL3KeyEvent(ev);
}

void SDL3Gamepad::sendButton(SDL3Mouse *mouse, Uint8 button, bool down)
{
	if (mouse == NULL)
	{
		return;
	}
	SDL_MouseButtonEvent ev;
	memset(&ev, 0, sizeof(ev));
	ev.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
	ev.timestamp = SDL_GetTicksNS();
	ev.button = button;
	ev.down = down;
	ev.clicks = 1;
	ev.x = m_cursorX;
	ev.y = m_cursorY;
	mouse->addSDL3ButtonEvent(ev);
}

void SDL3Gamepad::releaseAll(SDL3Mouse *mouse, SDL3Keyboard *keyboard)
{
	for (int i = 0; i < kNumBindings; ++i)
	{
		if (!m_buttonDown[i])
		{
			continue;
		}
		m_buttonDown[i] = false;
		if (kBindings[i].action == ACT_LEFT_CLICK)
			sendButton(mouse, SDL_BUTTON_LEFT, false);
		else if (kBindings[i].action == ACT_RIGHT_CLICK)
			sendButton(mouse, SDL_BUTTON_RIGHT, false);
		else if (kBindings[i].action == ACT_KEY)
			sendKey(keyboard, kBindings[i].key, false);
	}
	for (int i = 0; i < 4; ++i)
	{
		if (m_scrollKeyDown[i])
		{
			m_scrollKeyDown[i] = false;
			sendKey(keyboard, kScrollKeys[i], false);
		}
	}
	if (m_ctrlDown)
	{
		m_ctrlDown = false;
		sendKey(keyboard, SDL_SCANCODE_LCTRL, false);
	}
}

void SDL3Gamepad::update(SDL3Mouse *mouse, SDL3Keyboard *keyboard)
{
	if (!m_initialized)
	{
		m_initialized = true;
#if defined(GGC_PLATFORM_UWP)
		// The UWP build of SDL only has the Windows.Gaming.Input joystick driver, and
		// SDL leaves it disabled unless this hint is set, so no pad would ever appear.
		SDL_SetHint(SDL_HINT_JOYSTICK_WGI, "1");
#endif
		if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD))
		{
			printf("[ggc] gamepad: SDL_InitSubSystem failed: %s\n", SDL_GetError());
		}
		openAll();
		printf("[ggc] gamepad: %s\n", m_pad ? SDL_GetGamepadName(m_pad) : "none connected yet");
		fflush(stdout);
		m_lastTicks = SDL_GetTicks();
	}

	if (m_pad == NULL || mouse == NULL)
	{
		return;
	}

	const Uint64 now = SDL_GetTicks();
	float dt = (float)(now - m_lastTicks) / 1000.0f;
	m_lastTicks = now;
	if (dt > 0.1f)
		dt = 0.1f;

	int winW = 0, winH = 0;
	if (TheSDL3Window != NULL)
	{
		SDL_GetWindowSizeInPixels(TheSDL3Window, &winW, &winH);
	}
	if (winW <= 0 || winH <= 0)
	{
		return;
	}

	// Follow the real cursor when it moved by other means (a mouse, the game warping it).
	const MouseIO *status = mouse->getMouseStatus();
	if (m_cursorX < 0.0f && status->pos.x == 0 && status->pos.y == 0)
	{
		// Nothing has positioned the cursor yet (no mouse attached): start mid-screen.
		m_cursorX = (float)(winW / 2);
		m_cursorY = (float)(winH / 2);
		SDL_MouseMotionEvent ev;
		memset(&ev, 0, sizeof(ev));
		ev.type = SDL_EVENT_MOUSE_MOTION;
		ev.timestamp = SDL_GetTicksNS();
		ev.x = m_cursorX;
		ev.y = m_cursorY;
		mouse->addSDL3MotionEvent(ev);
	}
	else if (m_cursorX < 0.0f || (Int)m_cursorX != status->pos.x || (Int)m_cursorY != status->pos.y)
	{
		m_cursorX = (float)status->pos.x;
		m_cursorY = (float)status->pos.y;
	}

	// --- cursor ----------------------------------------------------------------
	float lx = SDL_GetGamepadAxis(m_pad, SDL_GAMEPAD_AXIS_LEFTX) / kAxisMax;
	float ly = SDL_GetGamepadAxis(m_pad, SDL_GAMEPAD_AXIS_LEFTY) / kAxisMax;
	const float mag = sqrtf(lx * lx + ly * ly);
	if (mag > kStickDeadzone)
	{
		// Rescale past the deadzone and apply a quadratic curve for fine control.
		const float norm = (mag > 1.0f ? 1.0f : mag);
		const float curved = ((norm - kStickDeadzone) / (1.0f - kStickDeadzone));
		float speed = kCursorSpeed * ((float)winH / 1080.0f) * curved * curved;
		const float lt = SDL_GetGamepadAxis(m_pad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) / kAxisMax;
		if (lt > kTriggerThreshold)
			speed *= kPrecisionScale;

		const float oldX = m_cursorX;
		const float oldY = m_cursorY;
		m_cursorX += (lx / mag) * speed * dt;
		m_cursorY += (ly / mag) * speed * dt;
		if (m_cursorX < 0.0f) m_cursorX = 0.0f;
		if (m_cursorY < 0.0f) m_cursorY = 0.0f;
		if (m_cursorX > (float)(winW - 1)) m_cursorX = (float)(winW - 1);
		if (m_cursorY > (float)(winH - 1)) m_cursorY = (float)(winH - 1);

		if ((Int)m_cursorX != (Int)oldX || (Int)m_cursorY != (Int)oldY)
		{
			SDL_MouseMotionEvent ev;
			memset(&ev, 0, sizeof(ev));
			ev.type = SDL_EVENT_MOUSE_MOTION;
			ev.timestamp = SDL_GetTicksNS();
			ev.x = m_cursorX;
			ev.y = m_cursorY;
			ev.xrel = m_cursorX - oldX;
			ev.yrel = m_cursorY - oldY;
			mouse->addSDL3MotionEvent(ev);
		}
	}

	// --- buttons -----------------------------------------------------------------
	for (int i = 0; i < kNumBindings; ++i)
	{
		const bool down = SDL_GetGamepadButton(m_pad, kBindings[i].button);
		if (down == m_buttonDown[i])
		{
			continue;
		}
		m_buttonDown[i] = down;
		switch (kBindings[i].action)
		{
			case ACT_LEFT_CLICK:  sendButton(mouse, SDL_BUTTON_LEFT, down); break;
			case ACT_RIGHT_CLICK: sendButton(mouse, SDL_BUTTON_RIGHT, down); break;
			case ACT_KEY:         sendKey(keyboard, kBindings[i].key, down); break;
			default: break;
		}
	}

	const float rt = SDL_GetGamepadAxis(m_pad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) / kAxisMax;
	const bool ctrl = rt > kTriggerThreshold;
	if (ctrl != m_ctrlDown)
	{
		m_ctrlDown = ctrl;
		sendKey(keyboard, SDL_SCANCODE_LCTRL, ctrl);
	}

	// --- camera scroll (right stick -> arrow keys) ------------------------------
	const float rx = SDL_GetGamepadAxis(m_pad, SDL_GAMEPAD_AXIS_RIGHTX) / kAxisMax;
	const float ry = SDL_GetGamepadAxis(m_pad, SDL_GAMEPAD_AXIS_RIGHTY) / kAxisMax;
	const bool want[4] = { rx < -kScrollThreshold, rx > kScrollThreshold, ry < -kScrollThreshold, ry > kScrollThreshold };
	for (int i = 0; i < 4; ++i)
	{
		if (want[i] != m_scrollKeyDown[i])
		{
			m_scrollKeyDown[i] = want[i];
			sendKey(keyboard, kScrollKeys[i], want[i]);
		}
	}

	// Drop everything held if the window loses focus so nothing sticks.
	if (TheSDL3Window != NULL && !(SDL_GetWindowFlags(TheSDL3Window) & SDL_WINDOW_INPUT_FOCUS))
	{
		releaseAll(mouse, keyboard);
	}
}

#endif // SAGE_USE_SDL3

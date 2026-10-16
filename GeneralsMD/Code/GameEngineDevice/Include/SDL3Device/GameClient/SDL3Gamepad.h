/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2026 TheSuperHackers
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
*/

// TheSuperHackers @feature githubawn 16/10/2026 Gamepad -> mouse/keyboard for the
// SDL3 backend (Xbox has no mouse in most setups). The left stick drives an emulated
// cursor and the buttons inject the same mouse clicks / hotkeys the PC build's SDL3
// input layer uses (see Core SDL3Input + SeatManager bindings), so menus and the
// in-game command bar work unchanged.

#pragma once

#if defined(SAGE_USE_SDL3)

#include <SDL3/SDL.h>

class SDL3Mouse;
class SDL3Keyboard;

class SDL3Gamepad
{
public:
	SDL3Gamepad();
	~SDL3Gamepad();

	void handleEvent(const SDL_Event &event);
	void update(SDL3Mouse *mouse, SDL3Keyboard *keyboard);

private:
	enum { NUM_BUTTONS = 16 };

	void openAll();
	void open(SDL_JoystickID id);
	void close(SDL_JoystickID id);
	void releaseAll(SDL3Mouse *mouse, SDL3Keyboard *keyboard);
	void sendKey(SDL3Keyboard *keyboard, SDL_Scancode scancode, bool down);
	void sendButton(SDL3Mouse *mouse, Uint8 button, bool down);

	SDL_Gamepad *m_pad;
	bool m_initialized;
	bool m_buttonDown[NUM_BUTTONS];
	bool m_ctrlDown;
	bool m_scrollKeyDown[4];
	float m_cursorX;
	float m_cursorY;
	float m_speed;
	Uint64 m_lastTicks;
};

#endif

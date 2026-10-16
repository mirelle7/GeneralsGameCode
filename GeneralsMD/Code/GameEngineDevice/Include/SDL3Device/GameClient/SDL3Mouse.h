/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
*/

#pragma once

#if defined(SAGE_USE_SDL3)

#include <SDL3/SDL.h>

#include "GameClient/Mouse.h"

class Image;

class SDL3Mouse : public Mouse
{
public:
	SDL3Mouse();
	virtual ~SDL3Mouse() override;

	virtual void init() override;
	virtual void reset() override;
	virtual void initCursorResources() override;
	virtual void setCursor(MouseCursor cursor) override;
	virtual void setPosition(Int x, Int y) override;
#if defined(GGC_PLATFORM_UWP)
	// TheSuperHackers @feature githubawn 16/10/2026 The Xbox shell shows no OS cursor
	// for UWP games, so draw the game's own 2D cursor images (Mouse.ini) each frame.
	virtual void draw() override;
#endif

	void addSDL3MotionEvent(const SDL_MouseMotionEvent &event);
	void addSDL3ButtonEvent(const SDL_MouseButtonEvent &event);
	void addSDL3WheelEvent(const SDL_MouseWheelEvent &event);
	// TheSuperHackers @feature bobtista 15/06/2026 Touch input -> left mouse.
	// A finger touch maps directly to a left click at that pixel so the menu is
	// navigable. phase: 0=down, 1=up, 2=motion.
	void addSDL3FingerEvent(const SDL_TouchFingerEvent &event, int phase);

protected:
	virtual void capture() override;
	virtual void releaseCapture() override;
	virtual UnsignedByte getMouseEvent(MouseIO *result, Bool flush) override;

private:
	void pushEvent(const MouseIO &event);

	MouseIO m_buffer[NUM_MOUSE_EVENTS];
	UnsignedInt m_nextGetIndex;
	UnsignedInt m_nextFreeIndex;
#if defined(__APPLE__) || defined(__EMSCRIPTEN__)
	SDL_Cursor *m_sdlCursors[Mouse::NUM_MOUSE_CURSORS];
#endif
#if defined(GGC_PLATFORM_UWP)
	const Image *m_cursorImages[Mouse::NUM_MOUSE_CURSORS];
	Bool m_cursorImagesLoaded;
#endif
};

#endif

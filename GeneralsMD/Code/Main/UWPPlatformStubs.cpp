/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2026 TheSuperHackers
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
*/

// TheSuperHackers @build githubawn 16/10/2026 UWP (Xbox) link stubs. The UWP
// API surface (WindowsApp.lib) has no IME (imm32), AVI capture (vfw32) or GDI, but
// the engine still references them from Windows-only code paths: the IME manager,
// the frame-grab movie writer and the GDI font fallback (UWP rasterizes fonts with
// stb_truetype instead). These no-ops report failure so callers back off.

#if defined(GGC_PLATFORM_UWP)

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOIME
#include <windows.h>
#include <objbase.h>
#include <SDL3/SDL.h>
#include <cstdio>

typedef bool Bool;

void OSDisplaySetBusyState(Bool, Bool) {}

extern "C" {

// --- imm32 -----------------------------------------------------------------
void *ImmGetContext(void *) { return nullptr; }
BOOL ImmReleaseContext(void *, void *) { return FALSE; }
void *ImmCreateContext(void) { return nullptr; }
BOOL ImmDestroyContext(void *) { return FALSE; }
void *ImmAssociateContext(void *, void *) { return nullptr; }
DWORD ImmGetProperty(void *, DWORD) { return 0; }
LONG ImmGetCompositionStringA(void *, DWORD, void *, DWORD) { return 0; }
LONG ImmGetCompositionStringW(void *, DWORD, void *, DWORD) { return 0; }
DWORD ImmGetCandidateListCountA(void *, DWORD *count) { if (count) *count = 0; return 0; }
DWORD ImmGetCandidateListCountW(void *, DWORD *count) { if (count) *count = 0; return 0; }
DWORD ImmGetCandidateListA(void *, DWORD, void *, DWORD) { return 0; }
DWORD ImmGetCandidateListW(void *, DWORD, void *, DWORD) { return 0; }

// --- vfw32 (AVI movie capture) ----------------------------------------------
void AVIFileInit(void) {}
void AVIFileExit(void) {}
HRESULT AVIFileOpenA(void **file, const char *, UINT, void *) { if (file) *file = nullptr; return E_FAIL; }
HRESULT AVIFileCreateStreamA(void *, void **stream, void *) { if (stream) *stream = nullptr; return E_FAIL; }
ULONG AVIFileRelease(void *) { return 0; }
ULONG AVIStreamRelease(void *) { return 0; }
HRESULT AVIStreamSetFormat(void *, LONG, void *, LONG) { return E_FAIL; }
HRESULT AVIStreamWrite(void *, LONG, LONG, void *, LONG, DWORD, LONG *, LONG *) { return E_FAIL; }

} // extern "C"

// --- gdi32 (declared dllimport, so the linker wants the __imp_ pointers) -----
namespace
{
	INT_PTR WINAPI ggcGdiFail() { return 0; }
}

#define GGC_UWP_GDI_STUB(name) extern "C" void *__imp_##name = (void *)&ggcGdiFail;
GGC_UWP_GDI_STUB(AddFontResourceA)
GGC_UWP_GDI_STUB(RemoveFontResourceA)
GGC_UWP_GDI_STUB(CreateCompatibleDC)
GGC_UWP_GDI_STUB(CreateDIBSection)
GGC_UWP_GDI_STUB(CreateFontA)
GGC_UWP_GDI_STUB(DeleteDC)
GGC_UWP_GDI_STUB(DeleteObject)
GGC_UWP_GDI_STUB(ExtTextOutW)
GGC_UWP_GDI_STUB(GetTextExtentPoint32W)
GGC_UWP_GDI_STUB(GetTextMetricsA)
GGC_UWP_GDI_STUB(SelectObject)
GGC_UWP_GDI_STUB(SetBkColor)
GGC_UWP_GDI_STUB(SetTextColor)
GGC_UWP_GDI_STUB(SetDeviceGammaRamp)
#undef GGC_UWP_GDI_STUB

// --- user32 / ole32 / shell32 / kernel32 desktop-only imports ----------------
// These resolve to the desktop import libraries (pulled in by #pragma comment or
// dependency targets) but do not exist in the UWP API set, so the app would fail to
// load on Xbox. Defining the __imp_ pointers here keeps them out of the import table.
// Window geometry is answered from the SDL window; the rest report benign defaults.
extern SDL_Window *TheSDL3Window;

namespace
{
	BOOL WINAPI ggcGetClientRect(HWND, LPRECT rect)
	{
		if (rect == nullptr) return FALSE;
		int w = 0, h = 0;
		if (TheSDL3Window != nullptr) SDL_GetWindowSizeInPixels(TheSDL3Window, &w, &h);
		rect->left = 0; rect->top = 0; rect->right = w; rect->bottom = h;
		return TRUE;
	}
	BOOL WINAPI ggcGetCursorPos(LPPOINT pt)
	{
		if (pt == nullptr) return FALSE;
		float x = 0.0f, y = 0.0f;
		SDL_GetMouseState(&x, &y);
		pt->x = (LONG)x; pt->y = (LONG)y;
		return TRUE;
	}
	BOOL WINAPI ggcTrue() { return TRUE; }
	INT_PTR WINAPI ggcZero() { return 0; }
	int WINAPI ggcMessageBoxA(HWND, LPCSTR text, LPCSTR caption, UINT)
	{
		// stdout is redirected to LocalState\...\ggc_log.txt on UWP.
		printf("[ggc] MessageBox: %s: %s\n", caption ? caption : "", text ? text : "");
		fflush(stdout);
		return IDOK;
	}
	int WINAPI ggcMessageBoxW(HWND, LPCWSTR text, LPCWSTR caption, UINT)
	{
		printf("[ggc] MessageBoxW: %ls: %ls\n", caption ? caption : L"", text ? text : L"");
		fflush(stdout);
		return IDOK;
	}
	UINT WINAPI ggcGetDoubleClickTime() { return 500; }
	HKL WINAPI ggcGetKeyboardLayout(DWORD) { return (HKL)(ULONG_PTR)0x04090409; }
	BOOL WINAPI ggcSetRect(LPRECT r, int l, int t, int rr, int b)
	{
		if (r == nullptr) return FALSE;
		r->left = l; r->top = t; r->right = rr; r->bottom = b;
		return TRUE;
	}
	HRESULT WINAPI ggcCoInit(LPVOID) { return CoInitializeEx(nullptr, COINIT_MULTITHREADED); }
	void WINAPI ggcOleUninitialize() { CoUninitialize(); }
	HRESULT WINAPI ggcFail() { return E_FAIL; }
	LPSTR WINAPI ggcLstrcpynA(LPSTR dst, LPCSTR src, int n)
	{
		if (dst == nullptr || n <= 0) return dst;
		int i = 0;
		for (; i < n - 1 && src != nullptr && src[i] != '\0'; ++i) dst[i] = src[i];
		dst[i] = '\0';
		return dst;
	}
}

#define GGC_UWP_IMP(name, fn) extern "C" void *__imp_##name = (void *)&fn;
GGC_UWP_IMP(GetClientRect, ggcGetClientRect)
GGC_UWP_IMP(GetWindowRect, ggcGetClientRect)
GGC_UWP_IMP(GetCursorPos, ggcGetCursorPos)
GGC_UWP_IMP(ScreenToClient, ggcTrue)
GGC_UWP_IMP(AdjustWindowRect, ggcTrue)
GGC_UWP_IMP(GetWindowLongA, ggcZero)
GGC_UWP_IMP(GetDesktopWindow, ggcZero)
GGC_UWP_IMP(MonitorFromWindow, ggcZero)
GGC_UWP_IMP(GetMonitorInfoA, ggcZero)
GGC_UWP_IMP(GetDoubleClickTime, ggcGetDoubleClickTime)
GGC_UWP_IMP(SetWindowTextA, ggcTrue)
GGC_UWP_IMP(SetWindowTextW, ggcTrue)
GGC_UWP_IMP(GetDC, ggcZero)
GGC_UWP_IMP(ReleaseDC, ggcZero)
GGC_UWP_IMP(ShowWindow, ggcZero)
GGC_UWP_IMP(MessageBoxA, ggcMessageBoxA)
GGC_UWP_IMP(MessageBoxW, ggcMessageBoxW)
GGC_UWP_IMP(GetKeyboardLayout, ggcGetKeyboardLayout)
GGC_UWP_IMP(SetFocus, ggcZero)
GGC_UWP_IMP(SetWindowPos, ggcTrue)
GGC_UWP_IMP(IsIconic, ggcZero)
GGC_UWP_IMP(SetRect, ggcSetRect)
GGC_UWP_IMP(FindWindowW, ggcZero)
GGC_UWP_IMP(SetForegroundWindow, ggcTrue)
GGC_UWP_IMP(GetAsyncKeyState, ggcZero)
GGC_UWP_IMP(OleInitialize, ggcCoInit)
GGC_UWP_IMP(CoInitialize, ggcCoInit)
GGC_UWP_IMP(OleUninitialize, ggcOleUninitialize)
GGC_UWP_IMP(OleRun, ggcFail)
GGC_UWP_IMP(GlobalHandle, ggcZero)
GGC_UWP_IMP(lstrcpynA, ggcLstrcpynA)
#undef GGC_UWP_IMP

#endif // GGC_PLATFORM_UWP

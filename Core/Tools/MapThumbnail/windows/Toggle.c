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

// Turns the Explorer map thumbnails on or off by registering or unregistering the
// MapThumbnailProvider.dll that sits next to this exe. Built without the C runtime to stay tiny.

#include <windows.h>

static const wchar_t ShellExKey[] = L"Software\\Classes\\SystemFileAssociations\\.map\\ShellEx\\{E357FCCD-A995-4576-B01F-234630154E96}";
static const wchar_t ClsidText[] = L"{26E5AA40-AEAC-4886-B7EA-64406AA591FF}";
static const wchar_t DllName[] = L"MapThumbnailProvider.dll";

static BOOL isEnabled(void)
{
	wchar_t current[64];
	DWORD size = sizeof(current);
	return RegGetValueW(HKEY_CURRENT_USER, ShellExKey, NULL, RRF_RT_REG_SZ, NULL, current, &size) == ERROR_SUCCESS &&
		lstrcmpiW(current, ClsidText) == 0;
}

void WINAPI ToggleMain(void)
{
	typedef HRESULT(STDAPICALLTYPE * ServerFunc)(void);
	const BOOL enable = !isEnabled();
	const wchar_t *message = L"MapThumbnailProvider.dll was not found next to this program.";
	wchar_t path[MAX_PATH];
	DWORD len = GetModuleFileNameW(NULL, path, MAX_PATH);
	HMODULE dll = NULL;
	HRESULT hr = E_FAIL;

	while (len > 0 && path[len - 1] != L'\\')
		--len;
	if (len > 0 && len + ARRAYSIZE(DllName) <= MAX_PATH)
	{
		lstrcpyW(path + len, DllName);
		dll = LoadLibraryW(path);
	}
	if (dll)
	{
		ServerFunc func = (ServerFunc)GetProcAddress(dll, enable ? "DllRegisterServer" : "DllUnregisterServer");
		if (func)
			hr = func();
		message = FAILED(hr) ? L"Could not change the map thumbnail setting." :
			enable ? L"Map thumbnails are now ON." : L"Map thumbnails are now OFF.";
	}

	MessageBoxW(NULL, message, L"Map Thumbnails", FAILED(hr) ? MB_ICONERROR : MB_ICONINFORMATION);
	ExitProcess(FAILED(hr) ? 1 : 0);
}

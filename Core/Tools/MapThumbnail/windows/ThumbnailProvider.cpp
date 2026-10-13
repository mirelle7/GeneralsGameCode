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

// Windows Explorer thumbnail handler (IThumbnailProvider) for World Builder maps.
//
// Install for the current user with "regsvr32 MapThumbnailProvider.dll", remove with "regsvr32 /u".
// The DLL must match Explorer's architecture, so build it 64-bit.

#include "MapThumbnail/MapThumbnail.h"

#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <thumbcache.h>

#include <new>
#include <string>
#include <vector>

namespace
{

// {26E5AA40-AEAC-4886-B7EA-64406AA591FF}
const CLSID CLSID_MapThumbnailProvider = { 0x26e5aa40, 0xaeac, 0x4886, { 0xb7, 0xea, 0x64, 0x40, 0x6a, 0xa5, 0x91, 0xff } };
const wchar_t ClsidText[] = L"{26E5AA40-AEAC-4886-B7EA-64406AA591FF}";
const wchar_t ThumbnailHandlerShellEx[] = L"{E357FCCD-A995-4576-B01F-234630154E96}";
const wchar_t HandlerName[] = L"Generals Map Thumbnail Provider";
const ULONG MaxStreamSize = 64 * 1024 * 1024;

HINSTANCE g_module = nullptr;
LONG g_objects = 0;
LONG g_locks = 0;

std::string toUtf8(const wchar_t *text)
{
	const int len = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
	if (len <= 1)
		return std::string();
	std::string out(len - 1, '\0');
	WideCharToMultiByte(CP_UTF8, 0, text, -1, &out[0], len, nullptr, nullptr);
	return out;
}

class MapThumbnailProvider : public IThumbnailProvider, public IInitializeWithFile, public IInitializeWithStream
{
public:
	MapThumbnailProvider() { InterlockedIncrement(&g_objects); }
	virtual ~MapThumbnailProvider() { InterlockedDecrement(&g_objects); }

	IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv) override
	{
		static const QITAB table[] = {
			QITABENT(MapThumbnailProvider, IThumbnailProvider),
			QITABENT(MapThumbnailProvider, IInitializeWithFile),
			QITABENT(MapThumbnailProvider, IInitializeWithStream),
			{ nullptr, 0 },
		};
		return QISearch(this, table, riid, ppv);
	}
	IFACEMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&m_refs); }
	IFACEMETHODIMP_(ULONG) Release() override
	{
		const ULONG refs = InterlockedDecrement(&m_refs);
		if (refs == 0)
			delete this;
		return refs;
	}

	// Explorer passes the path when process isolation is disabled for the handler (see registration),
	// which is what lets the TGA mode find the preview saved next to the map.
	IFACEMETHODIMP Initialize(LPCWSTR path, DWORD) override
	{
		if (!m_path.empty() || !m_data.empty())
			return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
		m_path = toUtf8(path);
		return m_path.empty() ? E_INVALIDARG : S_OK;
	}

	IFACEMETHODIMP Initialize(IStream *stream, DWORD) override
	{
		if (!m_path.empty() || !m_data.empty())
			return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
		STATSTG stat = {};
		if (FAILED(stream->Stat(&stat, STATFLAG_NONAME)) || stat.cbSize.QuadPart > MaxStreamSize)
			return E_FAIL;
		m_data.resize(size_t(stat.cbSize.QuadPart));
		ULONG read = 0;
		if (!m_data.empty() && (FAILED(stream->Read(m_data.data(), ULONG(m_data.size()), &read)) || read != m_data.size()))
		{
			m_data.clear();
			return E_FAIL;
		}
		return S_OK;
	}

	IFACEMETHODIMP GetThumbnail(UINT cx, HBITMAP *bitmap, WTS_ALPHATYPE *alpha) override
	{
		*bitmap = nullptr;
		*alpha = WTSAT_UNKNOWN;

		const MapThumbnail::Settings settings = MapThumbnail::loadSettings();
		MapThumbnail::Image image;
		bool ok;
		if (!m_path.empty())
			ok = MapThumbnail::renderFile(m_path, int(cx), settings, image);
		else
			ok = MapThumbnail::renderMemory(m_data.data(), m_data.size(), int(cx), settings, image);
		if (!ok)
			return E_FAIL;

		BITMAPINFO info = {};
		info.bmiHeader.biSize = sizeof(info.bmiHeader);
		info.bmiHeader.biWidth = image.width;
		info.bmiHeader.biHeight = -image.height; // top-down
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biBitCount = 32;
		info.bmiHeader.biCompression = BI_RGB;

		void *bits = nullptr;
		HBITMAP dib = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
		if (!dib)
			return E_OUTOFMEMORY;

		uint8_t *out = static_cast<uint8_t *>(bits);
		const size_t pixels = size_t(image.width) * image.height;
		for (size_t i = 0; i < pixels; ++i)
		{
			const uint8_t *in = &image.rgba[i * 4];
			out[i * 4 + 0] = in[2];
			out[i * 4 + 1] = in[1];
			out[i * 4 + 2] = in[0];
			out[i * 4 + 3] = 255;
		}

		*bitmap = dib;
		*alpha = WTSAT_RGB;
		return S_OK;
	}

private:
	LONG m_refs = 1;
	std::string m_path;
	std::vector<uint8_t> m_data;
};

class ClassFactory : public IClassFactory
{
public:
	IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv) override
	{
		static const QITAB table[] = {
			QITABENT(ClassFactory, IClassFactory),
			{ nullptr, 0 },
		};
		return QISearch(this, table, riid, ppv);
	}
	IFACEMETHODIMP_(ULONG) AddRef() override { return 2; }
	IFACEMETHODIMP_(ULONG) Release() override { return 1; }

	IFACEMETHODIMP CreateInstance(IUnknown *outer, REFIID riid, void **ppv) override
	{
		*ppv = nullptr;
		if (outer)
			return CLASS_E_NOAGGREGATION;
		MapThumbnailProvider *provider = new (std::nothrow) MapThumbnailProvider();
		if (!provider)
			return E_OUTOFMEMORY;
		const HRESULT hr = provider->QueryInterface(riid, ppv);
		provider->Release();
		return hr;
	}

	IFACEMETHODIMP LockServer(BOOL lock) override
	{
		if (lock)
			InterlockedIncrement(&g_locks);
		else
			InterlockedDecrement(&g_locks);
		return S_OK;
	}
};

ClassFactory g_factory;

HRESULT setValue(HKEY root, const std::wstring &key, const wchar_t *name, const std::wstring &value)
{
	return HRESULT_FROM_WIN32(RegSetKeyValueW(root, key.c_str(), name, REG_SZ, value.c_str(), DWORD((value.size() + 1) * sizeof(wchar_t))));
}

HRESULT setValue(HKEY root, const std::wstring &key, const wchar_t *name, DWORD value)
{
	return HRESULT_FROM_WIN32(RegSetKeyValueW(root, key.c_str(), name, REG_DWORD, &value, sizeof(value)));
}

const std::wstring ClassesKey = L"Software\\Classes\\";
const std::wstring ClsidKey = ClassesKey + L"CLSID\\" + ClsidText;
// SystemFileAssociations applies whatever program the .map extension is associated with.
const std::wstring ShellExKey = ClassesKey + L"SystemFileAssociations\\.map\\ShellEx\\" + ThumbnailHandlerShellEx;

} // namespace

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		g_module = instance;
		DisableThreadLibraryCalls(instance);
	}
	return TRUE;
}

STDAPI DllCanUnloadNow()
{
	return (g_objects == 0 && g_locks == 0) ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **ppv)
{
	*ppv = nullptr;
	if (!IsEqualCLSID(clsid, CLSID_MapThumbnailProvider))
		return CLASS_E_CLASSNOTAVAILABLE;
	return g_factory.QueryInterface(riid, ppv);
}

// Registers for the current user only, so no administrator rights are needed.
STDAPI DllRegisterServer()
{
	wchar_t modulePath[MAX_PATH];
	const DWORD len = GetModuleFileNameW(g_module, modulePath, ARRAYSIZE(modulePath));
	if (len == 0 || len == ARRAYSIZE(modulePath))
		return HRESULT_FROM_WIN32(GetLastError());

	HRESULT hr = setValue(HKEY_CURRENT_USER, ClsidKey, nullptr, HandlerName);
	if (SUCCEEDED(hr))
		hr = setValue(HKEY_CURRENT_USER, ClsidKey, L"DisableProcessIsolation", DWORD(1));
	if (SUCCEEDED(hr))
		hr = setValue(HKEY_CURRENT_USER, ClsidKey + L"\\InprocServer32", nullptr, modulePath);
	if (SUCCEEDED(hr))
		hr = setValue(HKEY_CURRENT_USER, ClsidKey + L"\\InprocServer32", L"ThreadingModel", L"Apartment");
	if (SUCCEEDED(hr))
		hr = setValue(HKEY_CURRENT_USER, ShellExKey, nullptr, ClsidText);

	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
	return hr;
}

STDAPI DllUnregisterServer()
{
	// Leave the .map entry alone if another handler has taken it over since.
	wchar_t current[64] = {};
	DWORD size = sizeof(current);
	if (RegGetValueW(HKEY_CURRENT_USER, ShellExKey.c_str(), nullptr, RRF_RT_REG_SZ, nullptr, current, &size) == ERROR_SUCCESS &&
		_wcsicmp(current, ClsidText) == 0)
		RegDeleteTreeW(HKEY_CURRENT_USER, ShellExKey.c_str());
	RegDeleteTreeW(HKEY_CURRENT_USER, ClsidKey.c_str());

	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
	return S_OK;
}

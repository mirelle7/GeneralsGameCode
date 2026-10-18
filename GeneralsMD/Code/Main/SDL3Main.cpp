/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
*/

#if defined(SAGE_USE_SDL3)

// TheSuperHackers @build bobtista 29/04/2026 Pull in the Win compat shim
// before any engine header so AsciiString.h's inline _stricmp resolves
// (compat/win32_shims/windows.h maps it to strcasecmp on non-Win).
#include <windows.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#include <emscripten/html5.h>
// Defined in Core/GameEngine/Source/GameNetwork/udp.cpp: opens the WebSocket the LAN
// transport tunnels through. UDP::Bind opens it too, but the handshake is asynchronous, so
// connecting at startup gives it time to be ready before LAN discovery starts announcing.
extern "C" void ggc_ws_connect(void);
#endif

#include <SDL3/SDL.h>
#if defined(__APPLE__)
#include <SDL3/SDL_metal.h>
#endif
// TheSuperHackers @build githubawn 18/06/2026 iOS asset-dir redirect needs these.
#if defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#include <unistd.h>   // chdir
#endif
#endif
// TheSuperHackers @build bobtista 13/06/2026 On Android SDLActivity invokes the
// "SDL_main" symbol via JNI; SDL_main.h remaps main->SDL_main so the entry point
// is exported from libz_generals.so. iOS, Switch and UWP need SDL's entry glue too.
#if defined(__ANDROID__) || defined(__SWITCH__) || defined(GGC_PLATFORM_UWP) || (defined(__APPLE__) && TARGET_OS_IPHONE)
#include <SDL3/SDL_main.h>
#endif
#include <cstdio>
#include <cstdlib>
#include <string>
#if defined(__ANDROID__)
#include <SDL3/SDL_system.h>  // SDL_GetAndroidExternalStoragePath
#include <unistd.h>            // chdir
#include <cstdlib>            // setenv
#include <signal.h>
#include <unwind.h>
#include <dlfcn.h>
#include <cxxabi.h>
#include <android/log.h>
#include <dirent.h>           // opendir/readdir (locate the game data dir)
#include <strings.h>          // strcasecmp
#include <cstring>

// TheSuperHackers @build bobtista 13/06/2026 Crash backtrace handler. Android's
// debuggerd does not produce a tombstone for stack-overflow SIGSEGVs, so install
// our own handler on an alternate signal stack that unwinds and symbolizes via
// dladdr -> logcat (tag "ggc-crash").
namespace
{
	struct GgcBtState { void **current; void **end; };

	_Unwind_Reason_Code ggc_unwind_cb(_Unwind_Context *ctx, void *arg)
	{
		GgcBtState *st = (GgcBtState *)arg;
		uintptr_t pc = _Unwind_GetIP(ctx);
		if (pc != 0 && st->current != st->end) {
			*st->current++ = (void *)pc;
		}
		return _URC_NO_REASON;
	}

	void ggc_crash_handler(int sig)
	{
		void *frames[64];
		GgcBtState st = { frames, frames + 64 };
		_Unwind_Backtrace(ggc_unwind_cb, &st);
		int n = (int)(st.current - frames);
		__android_log_print(6, "ggc-crash", "*** signal %d, %d frames ***", sig, n);
		for (int i = 0; i < n; i++) {
			Dl_info info;
			const char *sym = "?";
			char demangled[512];
			uintptr_t off = 0;
			if (dladdr(frames[i], &info) && info.dli_sname) {
				int status = 0;
				size_t len = sizeof(demangled);
				char *d = abi::__cxa_demangle(info.dli_sname, demangled, &len, &status);
				sym = (status == 0 && d) ? d : info.dli_sname;
				off = (uintptr_t)frames[i] - (uintptr_t)info.dli_saddr;
			}
			__android_log_print(6, "ggc-crash", "  #%02d %s+%zu", i, sym, (size_t)off);
		}
		_exit(1);
	}

	void ggc_install_crash_handler()
	{
		static char alt_stack[SIGSTKSZ < 65536 ? 65536 : SIGSTKSZ];
		stack_t ss;
		ss.ss_sp = alt_stack;
		ss.ss_size = sizeof(alt_stack);
		ss.ss_flags = 0;
		sigaltstack(&ss, nullptr);
		struct sigaction sa;
		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = ggc_crash_handler;
		sa.sa_flags = SA_ONSTACK;
		sigaction(SIGSEGV, &sa, nullptr);
		sigaction(SIGABRT, &sa, nullptr);
	}
}
#endif

#if defined(__SWITCH__)
#include <unistd.h>
#include <stdint.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
extern "C" {
	uint32_t romfsMountSelf(const char* name);
	uint32_t romfsUnmount(const char* name);
	const void* socketGetDefaultInitConfig(void);
	uint32_t socketInitialize(const void* config);
	void socketExit(void);
	// libnx SVC wrapper: routes a debug string to the emulator/host log (visible in
	// Ryujinx "Guest" log and yuzu Debug log). Our out-of-band boot trace channel.
	uint32_t svcOutputDebugString(const char* str, uint64_t size);
	// TheSuperHackers @bugfix githubawn 01/07/2026 The no-romfs NRO reads its .big
	// archives from the SD card, but the libnx sdmc: device is not guaranteed to be
	// mounted for a bare NRO. Mount it explicitly before we try to reach the data.
	uint32_t fsdevMountSdmc(void);
}

// Boot trace helper: emit a line to the emulator/host debug log.
static void ggcSwitchLog(const char* msg)
{
	svcOutputDebugString(msg, strlen(msg));
}
#endif

// TheSuperHackers @info bobtista 30/04/2026 Release builds strip
// WWDEBUG_SAY, so during macOS bring-up we emit a small set of stderr
// breadcrumbs so we can see where init reaches even when the window
// never comes up. Set GGC_TRACE=1 to enable.
#define GGC_TRACE(fmt, ...) do { \
    if (GgcFlags::Enabled(GgcFlag_Trace)) { \
        std::fprintf(stderr, "[ggc] " fmt "\n", ##__VA_ARGS__); \
        std::fflush(stderr); \
    } \
} while (0)

#include "Common/CommandLine.h"
#include "Common/Debug.h"
#include "Common/GameEngine.h"
#include "Common/GameMemory.h"
#include "Common/GlobalData.h"
#include "Common/version.h"
#include "GameClient/ClientInstance.h"
#include "GgcRuntimeFlags.h"
#include "SDL3GameEngine.h"
#if defined(_WIN32) && defined(RTS_ENABLE_CRASHDUMP)
#include "MiniDumper/MiniDumper.h"
#endif
// Version constants generated by cmake at configure time.
#include "BuildVersion.h"
#include "GeneratedVersion.h"

namespace
{
	const char * const kWindowTitle = "Command & Conquer Generals Zero Hour";
	const int kDefaultWindowWidth = 800;
	const int kDefaultWindowHeight = 600;
	const int kMinWindowWidth = kDefaultWindowWidth;
	const int kMinWindowHeight = kDefaultWindowHeight;
}

#if !defined(_WIN32)
// TheSuperHackers @build bobtista 07/06/2026 On Windows the CRT already declares and populates
// __argc/__argv (stdlib.h) before main() runs (see /ENTRY:mainCRTStartup); only POSIX needs these.
int __argc = 0;
char **__argv = NULL;
#endif

// TheSuperHackers @bugfix bobtista 30/04/2026 Backing storage for the
// compat shim's GetCommandLineA(). Populated in main() before any
// CommandLine::parse* call so the engine sees the real arg vector
// joined into a single Win32-style command-line string.
const char *g_compatCommandLine = "";
static std::string s_compatCommandLineStorage;

// TheSuperHackers @build bobtista 29/04/2026 Globals normally provided by the
// Win-side WinMain.cpp.
const char *g_strFile = "data/Generals.str";
const char *g_csfFile = "data/%s/Generals.csf";
// TheSuperHackers @build bobtista 09/06/2026 Debug.cpp references gAppPrefix from inside
// DEBUG_LOGGING-gated code; WinMain.cpp defines it on Win, so provide it here so a logging
// build of the SDL3 entry point links.
const char *gAppPrefix = "";

// Stack-dump shims. The Win build provides these in core_debug; the engine
// references them from RTS_DEBUG / IG_DEBUG_STACKTRACE-gated code so just
// give them weak no-ops on non-Win.
#include "Common/StackDump.h"
AsciiString g_LastErrorDump;
void StackDumpFromContext(unsigned long /*eip*/, unsigned long /*esp*/, unsigned long /*ebp*/, void (* /*cb*/)(const char *)) {}

// OS busy-state UI is Win-only.
void OSDisplaySetBusyState(Bool /*busyDisplay*/, Bool /*busySystem*/) {}

SDL_Window *TheSDL3Window = NULL;
// TheSuperHackers @build bobtista 07/06/2026 Define with the same HWND type every consumer
// extern-declares it as. On macOS HWND is a void* shim so this is unchanged; on Windows HWND is
// the real type, and defining it as void* here left the symbol unresolved at link.
HWND ApplicationHWnd = NULL;
#if defined(_WIN32)
// TheSuperHackers @build bobtista 13/06/2026 ATL's _Module.Init (GameEngine ctor, Win-only) needs
// the real module handle. WinMain.cpp supplies this on the non-SDL3 Win build; the SDL3 entry point
// must define and populate it too (set in main() via GetModuleHandle), else the Win SDL3 link fails.
HINSTANCE ApplicationHInstance = NULL;
#endif
// TheSuperHackers @bugfix bobtista 30/04/2026 macOS-only: keep the
// SDL_Metal view alive for the lifetime of the bgfx renderer, and
// publish its CAMetalLayer pointer here so BgfxBackend's
// GetNativeWindowHandle can hand it to bgfx as platformData.nwh
// instead of an NSWindow. Passing the NSWindow lets bgfx try to
// install its own CAMetalLayer on the contentView, which fights with
// the layer SDL3 already created and trips the Apple AGX driver
// during pipeline-state compile.
#if defined(__APPLE__)
SDL_MetalView TheSDL3MetalView = NULL;
void *TheSDL3MetalLayer = NULL;
#endif

extern Int GameMain();

#if defined(__ANDROID__)
// TheSuperHackers @bugfix githubawn 29/06/2026 Returns true if the directory
// contains at least one ".big" archive, used to pick the real game data dir from
// a list of candidates on Android (shared storage vs the app's private dir).
static bool GGC_DirHasBigArchive(const char *dir)
{
	DIR *d = opendir(dir);
	if (d == NULL)
		return false;

	bool found = false;
	struct dirent *ent;
	while ((ent = readdir(d)) != NULL)
	{
		const char *name = ent->d_name;
		const size_t len = strlen(name);
		if (len > 4 && strcasecmp(name + len - 4, ".big") == 0)
		{
			found = true;
			break;
		}
	}

	closedir(d);
	return found;
}
#endif

#if defined(GGC_PLATFORM_UWP)
// TheSuperHackers @build githubawn 16/10/2026 Xbox has no debugger attached in Dev
// Mode sideloading, so log unhandled exceptions (code, faulting RVA and a stack of
// module RVAs) to ggc_log.txt; resolve them against generalszh.pdb on the PC.
static void ggcLogAddress(const char *tag, void *addr)
{
	HMODULE mod = NULL;
	char name[MAX_PATH] = "?";
	if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)addr, &mod) && mod != NULL)
	{
		GetModuleFileNameA(mod, name, sizeof(name));
		const char *leaf = strrchr(name, '\\');
		printf("[ggc]   %s %p %s+0x%llx\n", tag, addr, leaf ? leaf + 1 : name, (unsigned long long)((char *)addr - (char *)mod));
	}
	else
	{
		printf("[ggc]   %s %p\n", tag, addr);
	}
}

static LONG WINAPI ggcUnhandledExceptionFilter(EXCEPTION_POINTERS *info)
{
	printf("[ggc] CRASH: exception 0x%08lx at %p\n", info->ExceptionRecord->ExceptionCode, info->ExceptionRecord->ExceptionAddress);
	if (info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && info->ExceptionRecord->NumberParameters >= 2)
	{
		printf("[ggc]   access violation %s address %p\n", info->ExceptionRecord->ExceptionInformation[0] ? "writing" : "reading", (void *)info->ExceptionRecord->ExceptionInformation[1]);
	}
	ggcLogAddress("fault", info->ExceptionRecord->ExceptionAddress);
	// Walk the faulting context with RtlVirtualUnwind (x64 unwind tables).
	CONTEXT ctx = *info->ContextRecord;
	for (int frame = 0; frame < 48 && ctx.Rip != 0; ++frame)
	{
		ggcLogAddress("frame", (void *)ctx.Rip);
		DWORD64 imageBase = 0;
		PRUNTIME_FUNCTION fn = RtlLookupFunctionEntry(ctx.Rip, &imageBase, NULL);
		if (fn == NULL)
		{
			// Leaf function: return address is at [rsp].
			ctx.Rip = *(DWORD64 *)ctx.Rsp;
			ctx.Rsp += 8;
			continue;
		}
		PVOID handlerData = NULL;
		DWORD64 establisher = 0;
		RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, ctx.Rip, fn, &ctx, &handlerData, &establisher, NULL);
	}
	fflush(stdout);
	return EXCEPTION_CONTINUE_SEARCH;
}
#endif

#if defined(_WIN32) && defined(RTS_ENABLE_CRASHDUMP)
// TheSuperHackers @bugfix bobtista 15/08/2026 The SDL3 entry point replaces WinMain on Windows
// (see the /ENTRY:mainCRTStartup link option), so the exception filter and minidump setup that
// only ever lived in WinMain never ran and a crash produced no dump at all.
static LONG WINAPI SDL3UnhandledExceptionFilter( struct _EXCEPTION_POINTERS *e_info )
{
	if (TheMiniDumper && TheMiniDumper->IsInitialized())
	{
		TheMiniDumper->TriggerMiniDumpForException(e_info, DumpType_Minimal);
		TheMiniDumper->TriggerMiniDumpForException(e_info, DumpType_Full);
	}

	MiniDumper::shutdownMiniDumper();
	return EXCEPTION_EXECUTE_HANDLER;
}
#endif

int main(int argc, char **argv)
{
#if !defined(_WIN32)
	__argc = argc;
	__argv = argv;
#elif defined(GGC_PLATFORM_UWP)
	// TheSuperHackers @bugfix githubawn 16/10/2026 Under the UWP entry the CRT never
	// builds __argv/__wargv, and OpenAL Soft reads __wargv[0] to find its config next
	// to the exe (a null dereference on Xbox). Point them at this process.
	{
		static wchar_t exePathW[MAX_PATH];
		static wchar_t *wargv[2] = { exePathW, NULL };
		GetModuleFileNameW(NULL, exePathW, MAX_PATH);
		if (__wargv == NULL)
		{
			__wargv = wargv;
		}
		if (__argv == NULL)
		{
			__argc = argc;
			__argv = argv;
		}
	}
#endif

#if defined(__SWITCH__)
	ggcSwitchLog("[ggc] main: entered\n");
	// TheSuperHackers @build githubawn 03/07/2026 TLS probe: OpenAL and bgfx both crash
	// (va=0x0) reading a C++ thread_local on the main thread. Log the thread pointer
	// registers and a real thread_local access to confirm the main-thread TLS setup.
	{
		unsigned long tp_rw = 0, tp_ro = 0;
		__asm__ volatile("mrs %0, tpidr_el0" : "=r"(tp_rw));
		__asm__ volatile("mrs %0, tpidrro_el0" : "=r"(tp_ro));
		static thread_local volatile int ggcTlsTest = 0x5a5a;
		char b[160];
		snprintf(b, sizeof(b), "[ggc] tpidr_el0=0x%lx tpidrro_el0=0x%lx &tls=%p\n",
			tp_rw, tp_ro, (void*)&ggcTlsTest);
		ggcSwitchLog(b);
		snprintf(b, sizeof(b), "[ggc] tls read = 0x%x\n", ggcTlsTest);
		ggcSwitchLog(b);
	}
	socketInitialize(socketGetDefaultInitConfig());
	// TheSuperHackers @build githubawn 03/07/2026 The Switch build ships as a homebrew
	// NRO (with a NACP) that reads its .big archives loose from the SD card
	// (sdmc:/generalszh), the same model RetroArch uses. Do NOT call romfsMountSelf
	// (in NRO mode it parses the self-NRO's embedded romfs asset, which this NRO does
	// not have, and that read crashes the emulator FS service). Do NOT call
	// fsdevMountSdmc either: libnx already auto-mounts sdmc: in homebrew __appInit, and
	// re-mounting collides and returns a misleading LibnxError_OutOfMemory (0x559).
	// The data dir is located and chdir'd below (after SDL_Init).
	ggcSwitchLog("[ggc] socket init done; sdmc: auto-mounted by libnx\n");
	setenv("GENERALS_USER_DIR", "sdmc:/switch/GeneralsZH", 1);
#endif

	GgcFlags::DumpTableIfRequested();

	GGC_TRACE("main entered argc=%d", argc);

#if defined(_WIN32) && defined(RTS_ENABLE_CRASHDUMP)
	SetUnhandledExceptionFilter( SDL3UnhandledExceptionFilter );
#endif

#if defined(_WIN32)
	ApplicationHInstance = GetModuleHandle(NULL);
#endif

	// TheSuperHackers @bugfix bobtista 30/04/2026 Build a Win32-style
	// command-line string from argv so GetCommandLineA() in the compat
	// shim returns the real arguments. The legacy parser expects token 0
	// to be the executable name and starts parsing at token 1, so argv[0]
	// must be preserved here.
	//
	// The engine's parseCommandLine tokenises with nextParam(buf, "\" "),
	// i.e. ' ' and '"' are the only separators and there is no backslash
	// escape - so we just wrap any arg that contains a space in double
	// quotes. Args that contain BOTH a space and a literal '"' are
	// unsupported by the engine parser itself, so we don't try to encode
	// them either.
	for (int i = 0; i < argc; ++i)
	{
		if (i > 0)
		{
			s_compatCommandLineStorage += ' ';
		}
		const char *a = argv[i];
		bool needsQuote = false;
		for (const char *p = a; *p != '\0'; ++p)
		{
			if (*p == ' ')
			{
				needsQuote = true;
				break;
			}
		}
		if (needsQuote)
		{
			s_compatCommandLineStorage += '"';
		}
		s_compatCommandLineStorage += a;
		if (needsQuote)
		{
			s_compatCommandLineStorage += '"';
		}
	}
	g_compatCommandLine = s_compatCommandLineStorage.c_str();

	// TheSuperHackers @bugfix bobtista 09/06/2026 WinMain initializes the debug log via
	// initMemoryManager(); the SDL3 entry point with the null memory manager never does, so a
	// logging build produced no output. Initialize it here so DEBUG_LOG reaches the log file and
	// console. Expands to nothing when debug logging is compiled out.
	DEBUG_INIT(DEBUG_FLAGS_DEFAULT);

	// TheSuperHackers @bugfix bobtista 02/09/2026 A headless run never initializes the renderer, so
	// it must not take over the display: it runs as a background app with a hidden window and
	// never enters fullscreen. Before this every headless launch blanked the screen and stole
	// focus from whatever the user was doing.
	bool wantHeadless = false;
	for (int headlessArg = 1; headlessArg < argc; ++headlessArg)
	{
		if (strcmp(argv[headlessArg], "-headless") == 0)
		{
			wantHeadless = true;
		}
	}
	if (wantHeadless)
	{
		SDL_SetHint(SDL_HINT_MAC_BACKGROUND_APP, "1");
		// Headless runs are harness runs; let several of them share the machine with each other
		// and with an open game instead of bailing on the single instance lock.
		rts::ClientInstance::setMultiInstance(TRUE);
		rts::ClientInstance::skipPrimaryInstance();
	}
	else
	{
		// TheSuperHackers @bugfix bobtista 24/09/2026 Say explicitly that a playable run is a
		// foreground app. SDL defaults this hint to true on macOS 14 and newer, which skips the
		// activation it performs in applicationDidFinishLaunching, so the window opened behind
		// whatever was already frontmost: clicks were swallowed as activation, Escape could not
		// skip the intro, and macOS drew its own arrow instead of the game cursor until the user
		// switched away and back. SDL_RaiseWindow alone cannot recover it because the
		// NSApp activateIgnoringOtherApps it relies on is ignored for a process that is not
		// already frontmost. Set GGC_NO_FOCUS for automated runs that must not steal focus.
		SDL_SetHint(SDL_HINT_MAC_BACKGROUND_APP, GgcFlags::Enabled(GgcFlag_NoFocus) ? "1" : "0");
	}

#if defined(__ANDROID__)
	SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
	// TheSuperHackers @feature bobtista 15/06/2026 We translate touch events to
	// left-mouse ourselves (SDL3Mouse::addSDL3FingerEvent); disable SDL's own
	// touch->mouse synthesis so a single tap does not produce duplicate clicks.
	SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
#endif

	GGC_TRACE("calling SDL_Init");
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
	{
		SDL_Log("SDL_Init failed: %s", SDL_GetError());
		return 1;
	}
	GGC_TRACE("SDL_Init OK");

#if defined(GGC_PLATFORM_UWP)
	// TheSuperHackers @build githubawn 16/10/2026 UWP activates the CoreWindow from its
	// Activated event, which is only dispatched while events are pumped. Engine init
	// takes many seconds without pumping, so the shell would kill the app for not
	// activating in time; pump briefly first so the splash screen is dismissed.
	for (int i = 0; i < 50; ++i)
	{
		SDL_PumpEvents();
		SDL_Delay(10);
	}

	// TheSuperHackers @build githubawn 16/10/2026 UWP (Xbox): the game data ships in
	// the package (read-only install dir, the engine's relative paths resolve from
	// there) and user data (Options.ini, saves, replays) goes to LocalState.
	{
		// The data can instead be uploaded once into LocalState\data (Device Portal file
		// explorer) so code-only package updates stay small; prefer it when present.
		const char *basePath = SDL_GetBasePath();
		char *prefPath = SDL_GetPrefPath("GGC", "GeneralsZH");
		char dataDir[1024];
		dataDir[0] = '\0';
		if (prefPath != NULL)
		{
			char probe[1024];
			snprintf(probe, sizeof(probe), "%s..\\..\\data\\INIZH.big", prefPath);
			if (GetFileAttributesA(probe) != INVALID_FILE_ATTRIBUTES)
			{
				snprintf(dataDir, sizeof(dataDir), "%s..\\..\\data\\", prefPath);
			}
		}
		if (dataDir[0] == '\0' && basePath != NULL)
		{
			snprintf(dataDir, sizeof(dataDir), "%s", basePath);
		}
		if (dataDir[0] != '\0')
		{
			SetCurrentDirectoryA(dataDir);
		}
		if (prefPath != NULL)
		{
			_putenv_s("GENERALS_USER_DIR", prefPath);
			// No console on Xbox: keep stdout/stderr (bgfx + engine logging) in a file
			// that Device Portal's file explorer can download.
			char logPath[1024];
			snprintf(logPath, sizeof(logPath), "%sggc_log.txt", prefPath);
			freopen(logPath, "w", stdout);
			setvbuf(stdout, NULL, _IONBF, 0);
			// A UWP process starts without a valid stderr descriptor, so _dup2 onto it
			// fails silently; give stderr its own file instead.
			snprintf(logPath, sizeof(logPath), "%sggc_err.txt", prefPath);
			freopen(logPath, "w", stderr);
			setvbuf(stderr, NULL, _IONBF, 0);
			SetUnhandledExceptionFilter(ggcUnhandledExceptionFilter);
			SDL_free(prefPath);
		}
		printf("[ggc] UWP base=%s data=%s user=%s\n", basePath ? basePath : "(null)", dataDir, getenv("GENERALS_USER_DIR"));

		// TheSuperHackers @feature githubawn 16/10/2026 A UWP app gets no command line, so
		// read extra arguments from LocalState\cmdline.txt (uploadable through Device
		// Portal). Whitespace-separated; "ENV:NAME=VALUE" sets an environment variable.
		{
			static char cmdBuf[4096];
			static char *uwpArgv[64];
			int uwpArgc = 0;
			uwpArgv[uwpArgc++] = (char *)"generalszh.exe";
			char cmdPath[1024];
			const char *user = getenv("GENERALS_USER_DIR");
			snprintf(cmdPath, sizeof(cmdPath), "%s..\\..\\cmdline.txt", user ? user : "");
			FILE *cf = fopen(cmdPath, "rb");
			if (cf != NULL)
			{
				const size_t n = fread(cmdBuf, 1, sizeof(cmdBuf) - 1, cf);
				fclose(cf);
				cmdBuf[n] = '\0';
				char *ctx = NULL;
				for (char *tok = strtok_s(cmdBuf, " \t\r\n", &ctx); tok != NULL && uwpArgc < 63; tok = strtok_s(NULL, " \t\r\n", &ctx))
				{
					if (strncmp(tok, "ENV:", 4) == 0)
					{
						_putenv(tok + 4);
					}
					else
					{
						uwpArgv[uwpArgc++] = tok;
					}
					printf("[ggc] cmdline.txt: %s\n", tok);
				}
			}
			uwpArgv[uwpArgc] = NULL;
			__argc = uwpArgc;
			__argv = uwpArgv;
		}

	}
#endif

#if defined(__SWITCH__)
	// TheSuperHackers @bugfix githubawn 03/07/2026 Locate the loose .big archives on
	// the SD card and chdir there so the engine's relative paths (Data\INI\...)
	// resolve. libnx already auto-mounts sdmc: in homebrew __appInit, so we only need
	// to find the data dir. CRITICAL: probe with ABSOLUTE paths only. A relative
	// fopen() before any successful chdir crashes inside libnx fsdev_getfspath (it
	// indexes fsdev_fsdevices[cwd] with cwd == -1 when no default device is set).
	{
		const char* basePath = SDL_GetBasePath();
		char argvDir[256];
		argvDir[0] = 0;
		if (argc > 0 && argv && argv[0])
		{
			size_t len = 0;
			while (argv[0][len] && len < sizeof(argvDir) - 1) { argvDir[len] = argv[0][len]; ++len; }
			argvDir[len] = 0;
			for (size_t i = len; i-- > 0; )
			{
				if (argvDir[i] == '/' || argvDir[i] == '\\') { argvDir[i] = 0; break; }
			}
		}

		const char* candidates[] = {
			"sdmc:/generalszh",
			"sdmc:/switch/generalszh",
			basePath,
			argvDir[0] ? argvDir : nullptr,
			"sdmc:/",
		};

		for (unsigned i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i)
		{
			const char* c = candidates[i];
			if (c == nullptr || c[0] == '\0') continue;
			// Absolute probe (crash-safe: device resolved by name, not cwd).
			char probePath[512];
			snprintf(probePath, sizeof(probePath), "%s/GeneralsZH.big", c);
			FILE* probe = fopen(probePath, "rb");
			char b[600];
			snprintf(b, sizeof(b), "[ggc] probe %s = %s\n", probePath, probe ? "OPEN" : "no");
			ggcSwitchLog(b);
			if (probe)
			{
				fclose(probe);
				const int cd = chdir(c);
				snprintf(b, sizeof(b), "[ggc] chdir %s = %d\n", c, cd);
				ggcSwitchLog(b);
				break;
			}
		}
	}
#endif

#if defined(__ANDROID__)
	// Install after SDL_Init so SDL's own handlers don't replace ours.
	ggc_install_crash_handler();
#endif

#if defined(__ANDROID__)
	// TheSuperHackers @build bobtista 13/06/2026 The engine loads its .big
	// archives and Data/ tree relative to the working directory. On Android the
	// process starts with cwd "/", so switch to the directory that actually holds
	// the game data.
	// TheSuperHackers @bugfix githubawn 29/06/2026 The app's private external
	// storage dir (SDL_GetAndroidExternalStoragePath) is empty on most installs;
	// the game data lives in shared storage (e.g. /sdcard/Games/Generals). Probe a
	// list of candidate directories and chdir to the first one that actually
	// contains .big archives, instead of unconditionally using the (empty) private
	// dir, which made the very first INI load (GameData) throw INI_CANT_OPEN_FILE
	// and abort init. Reading shared storage needs the MANAGE_EXTERNAL_STORAGE
	// ("All files access") grant declared in the manifest. Keep the user-data root
	// (saves / prefs / crash logs) in the always-writable private dir.
	{
		const char *userDir = SDL_GetAndroidExternalStoragePath();
		if (userDir != NULL && userDir[0] != '\0')
		{
			setenv("GENERALS_USER_DIR", userDir, 1);
		}

		const char *candidates[] = {
			getenv("GENERALS_DATA_DIR"),          // explicit override wins
			"/sdcard/Games/Generals",
			"/storage/emulated/0/Games/Generals",
			userDir,                              // last resort (data pushed into app dir)
		};

		const char *dataDir = NULL;
		for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i)
		{
			const char *c = candidates[i];
			if (c == NULL || c[0] == '\0')
				continue;
			if (GGC_DirHasBigArchive(c))
			{
				dataDir = c;
				break;
			}
		}

		if (dataDir == NULL)
			dataDir = userDir;  // nothing matched; fall back so chdir/logging still runs

		if (dataDir != NULL && dataDir[0] != '\0')
		{
			if (chdir(dataDir) == 0)
			{
				SDL_Log("[ggc] data dir (cwd): %s", dataDir);
			}
			else
			{
				SDL_Log("[ggc] chdir to data dir failed: %s", dataDir);
			}
		}
		else
		{
			SDL_Log("[ggc] could not resolve a game data directory");
		}
	}
#endif

#if defined(__APPLE__) && TARGET_OS_IPHONE
	// TheSuperHackers @build githubawn 18/06/2026 On iOS the app is sandboxed and
	// starts with an arbitrary cwd; the engine loads its .big archives and Data/
	// tree relative to the working directory. Point cwd (and the user-data root)
	// at the app's Documents container, where the game assets are placed (and
	// which UIFileSharingEnabled exposes in the Files app).
	{
		const char *home = getenv("HOME");
		if (home != NULL && home[0] != '\0')
		{
			char docs[1024];
			snprintf(docs, sizeof(docs), "%s/Documents", home);
			const int chdirRc = chdir(docs);
			if (chdirRc == 0)
			{
				SDL_Log("[ggc] asset dir (cwd): %s", docs);
			}
			else
			{
				SDL_Log("[ggc] chdir to asset dir failed: %s", docs);
			}
			setenv("GENERALS_USER_DIR", docs, 1);

			// TheSuperHackers @build githubawn 18/06/2026 Boot diagnostic: write a
			// file (readable over USB via devicectl) recording cwd and whether the
			// game data is reachable, so a crash-on-launch can be diagnosed without
			// the iOS console.
			{
				char logp[1200];
				snprintf(logp, sizeof(logp), "%s/ggc_ios_boot.log", docs);
				FILE *bl = fopen(logp, "w");
				if (bl != NULL)
				{
					char cwd[1024]; cwd[0] = '\0';
					getcwd(cwd, sizeof(cwd));
					fprintf(bl, "chdirRc=%d cwd=%s\n", chdirRc, cwd);
					FILE *big = fopen("GeneralsZH.big", "rb");
					fprintf(bl, "GeneralsZH.big=%s\n", big ? "OPEN_OK" : "OPEN_FAIL");
					if (big) fclose(big);
					FILE *ini = fopen("INIZH.big", "rb");
					fprintf(bl, "INIZH.big=%s\n", ini ? "OPEN_OK" : "OPEN_FAIL");
					if (ini) fclose(ini);
					fprintf(bl, "reached: pre-GameMain\n");
					fclose(bl);
				}
			}
		}
		else
		{
			SDL_Log("[ggc] HOME not set; cannot locate Documents asset dir");
		}
	}
#endif

#if defined(__EMSCRIPTEN__)
	// TheSuperHackers @feature githubawn 29/07/2026 The browser's in-memory filesystem
	// is thrown away on reload, so options, saves and replays would never survive a
	// refresh. Mount IndexedDB (IDBFS) at /preferences and point the Unix user-data
	// lookup (GlobalData::BuildUserDataPathFromRegistry, XDG_DATA_HOME branch) at it,
	// which keeps the engine side unchanged. The initial syncfs(true) loads whatever
	// IndexedDB already holds; the interval flushes writes back out.
	setenv("XDG_DATA_HOME", "/preferences", 1);
	EM_ASM({
		FS.mkdir('/preferences');
		FS.mount(IDBFS, {}, '/preferences');
		FS.syncfs(true, function(err) {
			if (err) { console.error('IDBFS load failed:', err); }
		});
		setInterval(function() { FS.syncfs(false, function() {}); }, 5000);
	});

	// TheSuperHackers @feature githubawn 30/07/2026 Open the LAN relay socket now rather
	// than on the first UDP::Bind: the WebSocket handshake is asynchronous, so a bind
	// followed straight away by a LAN announce would send it before the socket was up and
	// lose it. The ?ip=N identity override is applied later, in
	// IPEnumeration::getAddresses, because a GlobalData re-init would clobber it here.
	ggc_ws_connect();
#endif

	int windowW = kDefaultWindowWidth;
	int windowH = kDefaultWindowHeight;
	bool wantWindowed = wantHeadless;
	int requestedW = 0;
	int requestedH = 0;
	for (int argi = 1; argi < argc; ++argi)
	{
		if (strcmp(argv[argi], "-win") == 0)
		{
			wantWindowed = true;
		}
		else if (strcmp(argv[argi], "-xres") == 0 && argi + 1 < argc)
		{
			requestedW = atoi(argv[argi + 1]);
		}
		else if (strcmp(argv[argi], "-yres") == 0 && argi + 1 < argc)
		{
			requestedH = atoi(argv[argi + 1]);
		}
	}
#if defined(__EMSCRIPTEN__)
	// TheSuperHackers @bugfix 31/07/2026 Never ask for fullscreen on the web. The canvas is the
	// window here, and a browser only grants real fullscreen from a user gesture, so the request
	// did not produce a fullscreen page - it just sized the drawing buffer to the reported
	// desktop mode while the page kept presenting the canvas at its CSS box. The two disagreed,
	// so the cursor landed somewhere other than where it was drawn. Run windowed at the canvas'
	// own CSS size instead, which keeps the drawing buffer and the presented size in step.
	wantWindowed = true;
	if (requestedW <= 0 && requestedH <= 0)
	{
		double cssWidth = 0.0;
		double cssHeight = 0.0;
		if (emscripten_get_element_css_size("#canvas", &cssWidth, &cssHeight) == EMSCRIPTEN_RESULT_SUCCESS
			&& cssWidth >= 1.0 && cssHeight >= 1.0)
		{
			requestedW = (int)cssWidth;
			requestedH = (int)cssHeight;
		}
	}
#endif

	{
		const SDL_DisplayID primaryDisplay = SDL_GetPrimaryDisplay();
		const SDL_DisplayMode *desktopMode = SDL_GetDesktopDisplayMode(primaryDisplay);
		if (!wantWindowed && desktopMode != nullptr && desktopMode->w > 0 && desktopMode->h > 0)
		{
			windowW = desktopMode->w;
			windowH = desktopMode->h;
		}
	}

	// TheSuperHackers @bugfix bobtista 28/05/2026 Honor -xres/-yres when -win is set so '-win -xres 1600 -yres 1200' produces a 1600x1200 window instead of 800x600.
	if (wantWindowed)
	{
		if (requestedW > 0)
		{
			windowW = requestedW;
		}
		if (requestedH > 0)
		{
			windowH = requestedH;
		}
	}
	Uint32 windowFlags = SDL_WINDOW_RESIZABLE;
#if defined(__APPLE__)
	// TheSuperHackers @feature githubawn 28/06/2026 The desktop-GL build needs a
	// GL-capable window (bgfx attaches its own NSOpenGLContext to the contentView);
	// the Metal/MoltenVK builds need a Metal-backed view. Pick the matching flag.
#if defined(GGC_BGFX_RENDERER_GLSL)
	windowFlags |= SDL_WINDOW_OPENGL;
#else
	windowFlags |= SDL_WINDOW_METAL;
#endif
	// TheSuperHackers @bugfix bobtista 08/06/2026 Do NOT create the window hidden on macOS for
	// fullscreen. A real fullscreen Space (which hides the menu bar + Dock) is entered via
	// -[NSWindow toggleFullScreen:], and AppKit only animates a VISIBLE, active window into a Space;
	// toggling a hidden window degrades to a borderless window confined to the usable area. The
	// fullscreen transition below happens fast enough that there is no meaningful windowed flash.
#else
	// TheSuperHackers @bugfix bobtista 28/05/2026 Hide the window during fullscreen bring-up so it doesn't briefly appear at the requested resolution before SDL_SetWindowFullscreen takes effect; windowed runs show the window immediately.
	if (!wantWindowed)
	{
		windowFlags |= SDL_WINDOW_HIDDEN;
	}
#endif
	if (wantHeadless)
	{
		windowFlags |= SDL_WINDOW_HIDDEN;
	}
	GGC_TRACE("calling SDL_CreateWindow");
	TheSDL3Window = SDL_CreateWindow(kWindowTitle, windowW, windowH, windowFlags);
	if (TheSDL3Window == NULL)
	{
		SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
		SDL_Quit();
		return 1;
	}
#if !defined(__APPLE__)
	if (!wantWindowed)
	{
		SDL_SetWindowFullscreenMode(TheSDL3Window, nullptr);
		SDL_SetWindowFullscreen(TheSDL3Window, true);
		SDL_SyncWindow(TheSDL3Window);
	}
#endif
	GGC_TRACE("SDL_CreateWindow OK window=%p", (void*)TheSDL3Window);

	// TheSuperHackers @feature bobtista 08/06/2026 Enforce a minimum content size so a drag
	// cannot shrink the window below what the fixed-resolution UI can lay out. SDL_SetWindowAspectRatio
	// is avoided: on macOS its windowWillResize: delegate traps AppKit's live-resize loop.
	SDL_SetWindowMinimumSize(TheSDL3Window, kMinWindowWidth, kMinWindowHeight);

	ApplicationHWnd = (HWND)TheSDL3Window;

#if defined(__APPLE__)
	// TheSuperHackers @bugfix bobtista 30/04/2026 Use SDL3's official
	// Metal-view helper so we own a CAMetalLayer-backed NSView and can
	// hand bgfx the CAMetalLayer directly. Without this, bgfx receives
	// the NSWindow and races with SDL3 for control of the contentView's
	// layer, which manifests as intermittent AGX driver compilation
	// crashes in AGCDeserializedReply on macOS Tahoe / Apple Silicon.
	// TheSuperHackers @build githubawn 17/06/2026 Skipped for the desktop-GL build: bgfx's
	// GL backend installs an NSOpenGLContext on the contentView instead, and a CAMetalLayer
	// there would leave GL with no drawable.
#if !defined(GGC_BGFX_RENDERER_GLSL)
	GGC_TRACE("calling SDL_Metal_CreateView");
	TheSDL3MetalView = SDL_Metal_CreateView(TheSDL3Window);
	if (TheSDL3MetalView != NULL)
	{
		TheSDL3MetalLayer = SDL_Metal_GetLayer(TheSDL3MetalView);
		GGC_TRACE("SDL_Metal_CreateView OK view=%p layer=%p",
		          (void*)TheSDL3MetalView, TheSDL3MetalLayer);
	}
	else
	{
		SDL_Log("SDL_Metal_CreateView failed: %s", SDL_GetError());
	}
#endif

	// TheSuperHackers @bugfix bobtista 08/06/2026 Enter a REAL macOS fullscreen Space (hides the menu
	// bar + Dock, covers the whole display). Per SDL3's Cocoa backend this needs a resizable window
	// (set above), desktop/NULL fullscreen mode (so it is not "exclusive"), and the window VISIBLE and
	// raised when toggled - a window that is still hidden makes SDL_SetWindowFullscreen a no-op and
	// falls back to a borderless window confined to the usable area (menu bar + Dock left on top).
	if (!wantWindowed)
	{
		SDL_ShowWindow(TheSDL3Window);
		SDL_RaiseWindow(TheSDL3Window);
		SDL_SetWindowFullscreenMode(TheSDL3Window, nullptr);
		SDL_SetWindowFullscreen(TheSDL3Window, true);
		SDL_SyncWindow(TheSDL3Window);
	}
	else if (!wantHeadless)
	{
		// TheSuperHackers @bugfix bobtista 15/08/2026 Raise the window so it becomes the key window.
		// A window created by a process that is not already frontmost does not reliably become key on
		// macOS. When it loses that race no SDL_EVENT_WINDOW_FOCUS_GAINED ever arrives, so the engine
		// stays inactive, no keyboard events are delivered (Escape cannot skip the intro), and macOS
		// swallows most clicks as activation attempts, which reads as menu buttons needing to be
		// clicked repeatedly. The fullscreen path above already raises the window and is unaffected.
		SDL_ShowWindow(TheSDL3Window);
		SDL_RaiseWindow(TheSDL3Window);
	}
#endif

#if defined(__ANDROID__)
	// TheSuperHackers @bugfix bobtista 15/06/2026 On Android the SurfaceView is
	// created asynchronously and the fullscreen/orientation transition destroys
	// and recreates it. Engine init (which calls bgfx::init ~several seconds
	// later) does not pump SDL events, so if we proceed before the surface has
	// settled, bgfx binds a stale/destroyed ANativeWindow and crashes inside
	// ANativeWindow_setBuffersGeometry (or renders nothing). Pump events here
	// until the native window handle is non-null and stable for a short window.
	{
		void *prevWin = nullptr;
		int stableFrames = 0;
		for (int i = 0; i < 300 && stableFrames < 12; ++i)
		{
			SDL_PumpEvents();
			SDL_Event ev;
			while (SDL_PollEvent(&ev)) { /* drain pending events */ }
			SDL_PropertiesID props = SDL_GetWindowProperties(TheSDL3Window);
			void *nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER, NULL);
			if (nwh != nullptr && nwh == prevWin)
			{
				++stableFrames;
			}
			else
			{
				stableFrames = 0;
			}
			prevWin = nwh;
			SDL_Delay(16);
		}
		SDL_Log("[ggc] Android surface settled: nwh=%p stableFrames=%d", prevWin, stableFrames);
	}
#endif

	// TheSuperHackers @build bobtista 30/04/2026 Mirror the early-init the
	// Win path does in WinMain.cpp: build TheVersion, then run command-line
	// parsing so TheWritableGlobalData exists before GameEngine::init runs.
	TheVersion = NEW Version;
	TheVersion->setVersion(VERSION_MAJOR, VERSION_MINOR, VERSION_BUILDNUM, VERSION_LOCALBUILDNUM,
		AsciiString(VERSION_BUILDUSER), AsciiString(VERSION_BUILDLOC),
		AsciiString(__TIME__), AsciiString(__DATE__));

	GGC_TRACE("calling parseCommandLineForStartup cmdline='%s'", g_compatCommandLine);
	CommandLine::parseCommandLineForStartup();
	GGC_TRACE("parseCommandLineForStartup OK headless=%d",
	          (TheGlobalData != NULL && TheGlobalData->m_headless) ? 1 : 0);

#ifdef DEBUG_LOGGING
	// TheSuperHackers @bugfix bobtista 09/08/2026 The log is created beside the executable, which
	// a read-only install directory rejects. Move it to the user data directory when that failed.
	if (TheGlobalData != NULL)
	{
		DebugReopenLogInDirectory(TheGlobalData->getPath_UserData().str());
	}
#endif

#if defined(_WIN32) && defined(RTS_ENABLE_CRASHDUMP)
	// Mirror WinMain: the dump directory hangs off the user data path, so this has to follow
	// command-line parsing. It also follows the log relocation above so that a read-only install
	// still records where the dumps went, or why they were disabled.
	if (TheGlobalData != NULL)
	{
		MiniDumper::initMiniDumper(TheGlobalData->getPath_UserData().str(), 'Z');
	}
#endif

	// TheSuperHackers @bugfix bobtista 30/04/2026 -headless asks for
	// engine-only execution (no rendering, no audio); on Apple Silicon
	// macOS Tahoe even initialising bgfx Metal trips the AGX driver
	// bug, so explicitly null out ApplicationHWnd here so the renderer
	// chain (W3DDisplay::init -> WW3D::Init -> DX8Wrapper::Init ->
	// BgfxBackend::Initialize) bails on null hwnd before any Metal
	// pipeline state is constructed.
	if (TheGlobalData != NULL && TheGlobalData->m_headless)
	{
		ApplicationHWnd = NULL;
	}

	// TheSuperHackers @fix bobtista 08/07/2026 Mirror WinMain: refuse to start a
	// second client instance. Multi-instance builds and command-line flags claim
	// a free instance slot inside initialize() instead of failing.
	Int result = 0;
	if (rts::ClientInstance::initialize())
	{
		GGC_TRACE("calling GameMain");
		result = GameMain();
		GGC_TRACE("GameMain returned result=%d", result);
	}
	else
	{
		DEBUG_LOG(("Generals is already running...Bail!"));
	}

#if defined(__APPLE__)
	if (TheSDL3MetalView != NULL)
	{
		SDL_Metal_DestroyView(TheSDL3MetalView);
		TheSDL3MetalView = NULL;
		TheSDL3MetalLayer = NULL;
	}
#endif

	SDL_DestroyWindow(TheSDL3Window);
	TheSDL3Window = NULL;
	ApplicationHWnd = NULL;
	SDL_Quit();

#if defined(__SWITCH__)
#if !defined(GGC_SWITCH_SD_DATA)
	romfsUnmount("romfs");
#endif
	socketExit();
#endif

#if defined(_WIN32) && defined(RTS_ENABLE_CRASHDUMP)
	MiniDumper::shutdownMiniDumper();
#endif

	return result;
}

#endif

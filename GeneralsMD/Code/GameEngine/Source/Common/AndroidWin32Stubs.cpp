/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 TheSuperHackers
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
*/

// TheSuperHackers @build bobtista 13/06/2026 Link-time stubs for symbols that
// are missing on non-Windows platforms. WorkerProcess, the IME manager,
// g_LastErrorDump and OSDisplaySetBusyState now have non-Windows definitions
// elsewhere (WorkerProcess.cpp, IMEManager.cpp, SDL3Main.cpp), and MiniDumper is
// compiled out with RTS_ENABLE_CRASHDUMP, so only pthread_cancel remains.

// --- pthread_cancel ---------------------------------------------------------
// Android's bionic libc does not implement thread cancellation. The GamespySDK
// Linux thread backend calls pthread_cancel(); provide a no-op so it links.
// (Online play is stubbed on Android, so the cancellation never matters.)
#if defined(__ANDROID__)
#include <pthread.h>
extern "C" int pthread_cancel(pthread_t) { return 0; }
#endif

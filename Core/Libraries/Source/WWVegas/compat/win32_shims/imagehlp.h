#pragma once

// TheSuperHackers @build bobtista 13/06/2026 — imagehlp.h shim for non-Windows
// platforms (Android, macOS, Linux). The crash-dump / stack-walk APIs are
// Windows-only; DbgHelpLoader gates all real usage on WIN32 at runtime via
// dynamic DLL loading. On non-Windows we just need the type stubs so the
// header compiles; none of the APIs are actually called.

#ifndef _WIN32

#include "windows.h"

// IMAGEHLP_SYMBOL stub
#ifndef MAX_SYM_NAME
#define MAX_SYM_NAME 2000
#endif

#endif // !_WIN32

# TheSuperHackers @build bobtista 13/06/2026 On Android the game is loaded by the
# Java SDLActivity via System.loadLibrary("z_generals"), which then calls the
# SDL_main entry point. So it must be a shared library (libz_generals.so), not an
# executable (there is no crt main()).
if(ANDROID)
    add_library(z_generals SHARED)
elseif(WIN32)
    add_executable(z_generals WIN32)
else()
    add_executable(z_generals)
endif()

# Use a binary name that doesn't conflict with original game.
if(ANDROID)
    # SDLActivity.getLibraries() returns "z_generals" -> libz_generals.so
    set_target_properties(z_generals PROPERTIES OUTPUT_NAME "z_generals")
elseif(EMSCRIPTEN)
    # TheSuperHackers @build githubawn 29/07/2026 Emit index.html/.js/.wasm so the
    # root URL of the directory the web build is served from boots the game.
    set_target_properties(z_generals PROPERTIES OUTPUT_NAME "index")
elseif("${CMAKE_SYSTEM}" MATCHES "Windows")
    set_target_properties(z_generals PROPERTIES OUTPUT_NAME "generalszh${RTS_BUILD_OUTPUT_SUFFIX}")
else()
    set_target_properties(z_generals PROPERTIES OUTPUT_NAME "generalszh${RTS_BUILD_OUTPUT_SUFFIX}")
endif()

# TheSuperHackers @build githubawn 18/06/2026 iOS app bundle + code signing.
# iOS executables must be .app bundles with a valid Info.plist; the Xcode
# generator drives automatic signing from these attributes. Pass the signing
# team via -DGGC_IOS_DEVELOPMENT_TEAM=XXXXXXXXXX at configure time.
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set_target_properties(z_generals PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/iOSInfo.plist.in"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.generalszh"
        MACOSX_BUNDLE_BUNDLE_NAME "Zero Hour"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.example.generalszh"
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${GGC_IOS_DEVELOPMENT_TEAM}"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1"
        XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
        # TheSuperHackers @build githubawn 18/06/2026 Generate a runnable Xcode
        # scheme for the app target so it can be selected and Run/installed onto
        # a device from the GUI (the default project only exposes the aggregate
        # "genzh"/ALL_BUILD scheme, which is build-only and cannot be run).
        XCODE_GENERATE_SCHEME ON
    )
endif()

target_link_libraries(z_generals PRIVATE
    z_gameengine
    z_gameenginedevice
    zi_always
)

if(SAGE_USE_SDL3)
    target_link_libraries(z_generals PRIVATE sdl3lib)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    set(_dkp "$ENV{DEVKITPRO}")
    target_link_libraries(z_generals PRIVATE
        "${_dkp}/portlibs/switch/lib/libz.a"
        "${_dkp}/portlibs/switch/lib/libbz2.a"
        "${_dkp}/portlibs/switch/lib/libEGL.a"
        "${_dkp}/portlibs/switch/lib/libGLESv2.a"
        "${_dkp}/portlibs/switch/lib/libglapi.a"
        "${_dkp}/portlibs/switch/lib/libdrm_nouveau.a"
        "${_dkp}/portlibs/switch/lib/libdav1d.a"
        "${_dkp}/portlibs/switch/lib/libswresample.a"
        "${_dkp}/libnx/lib/libnx.a"
    )
    unset(_dkp)
endif()

# TheSuperHackers @build githubawn 16/10/2026 Desktop Windows links these for both
# entry points; UWP (WindowsStore) has none of them.
if(WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
    target_link_libraries(z_generals PRIVATE
        deps_bink
        comctl32
        dinput8
        dxguid
        imm32
        deps_miles
        vfw32
        winmm
    )
endif()

# TheSuperHackers @build bobtista 13/06/2026 The bgfx GLES renderer references
# GL/EGL entry points directly; link the Android system GLES/EGL libraries
# (and libandroid for ANativeWindow).
if(ANDROID)
    target_link_libraries(z_generals PRIVATE GLESv2 EGL android log)
endif()

if(SAGE_USE_SDL3)
    # TheSuperHackers @build bobtista 07/06/2026 SDL3Main.cpp uses a cross-platform int main(),
    # but on Windows the exe is built with the GUI subsystem whose default entry is WinMainCRTStartup
    # (expects WinMain). Point the entry at mainCRTStartup so the CRT calls main() directly while
    # staying in the WINDOWS subsystem (no console window), avoiding an SDL_main/WinMain shim.
    # UWP (WindowsStore) is entered through SDL's own app entry instead.
    if(WIN32 AND MSVC AND NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
        target_link_options(z_generals PRIVATE "/ENTRY:mainCRTStartup")
    endif()
endif()

# TheSuperHackers @build bobtista 22/04/2026
# Standalone bgfx owns the device through bgfx and supplies the remaining
# D3DX helper symbols in-tree, so neither d3d8.lib nor d3dx8.lib is needed.
# Ref-popup keeps both for the real DX8 reference path.
if(NOT GGC_RENDER_BACKEND STREQUAL "bgfx")
    target_link_libraries(z_generals PRIVATE d3d8 d3dx8)
endif()

# TODO Originally referred to build host and user, replace with git info perhaps?
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/GeneratedVersion.h
"#pragma once

#define VERSION_LOCALBUILDNUM 0
#define VERSION_BUILDUSER \"\"
#define VERSION_BUILDLOC \"\"
"
)

# Based on original binary values for these variables.
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/BuildVersion.h
"#pragma once

#define VERSION_MAJOR 1
#define VERSION_MINOR 4
#define VERSION_BUILDNUM 601
"
)

if(MSVC)
    target_link_options(z_generals PRIVATE "/NODEFAULTLIB:libci.lib")
endif()

# TheSuperHackers @build bobtista 09/08/2026 The staged Linux zip keeps its bundled
# SDL3/OpenAL/FFmpeg in libs/ beside the executable, but the shipped binary's RUNPATH holds
# only build-tree paths, so it found them solely through the LD_LIBRARY_PATH that run.sh
# exports and launching ./generalszh directly failed. BUILD_RPATH appends rather than
# replaces, so deploy-linux-generalsmd.sh keeps reading those build-tree entries to walk the
# transitive closure.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_target_properties(z_generals PROPERTIES BUILD_RPATH "$ORIGIN/libs")
endif()

target_include_directories(z_generals PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# TheSuperHackers @build bobtista 07/06/2026 SDL3 is the entry point whenever SAGE_USE_SDL3 is
# set, including on Windows (win32-generalsmd-sdl3-bgfx). The Win32 WinMain entry is only used for
# the non-SDL3 Windows build. Previously SDL3Main was gated on NOT WIN32, so the Windows SDL3 build
# linked WinMain and left TheSDL3Window (defined in SDL3Main.cpp) unresolved.
target_sources(z_generals PRIVATE
    $<$<AND:$<BOOL:${WIN32}>,$<NOT:$<BOOL:${SAGE_USE_SDL3}>>>:WinMain.cpp>
    $<$<AND:$<BOOL:${WIN32}>,$<NOT:$<BOOL:${SAGE_USE_SDL3}>>>:WinMain.h>
    $<$<BOOL:${SAGE_USE_SDL3}>:SDL3Main.cpp>
    $<$<STREQUAL:${CMAKE_SYSTEM_NAME},NintendoSwitch>:SwitchPlatformStubs.cpp>
    $<$<STREQUAL:${CMAKE_SYSTEM_NAME},WindowsStore>:UWPPlatformStubs.cpp>
)

# The stubs declare imm32/vfw32 entry points with their own signatures, so keep
# windows.h (with imm.h) from the precompiled header out of that file.
set_source_files_properties(UWPPlatformStubs.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

# RC files optional for MinGW builds
# Icon: LoadIcon() handles missing resource gracefully (uses default system icon)
# Manifest: DPI awareness metadata (nice to have but not essential)
if(MSVC)
    # VS2005 and later adds default manifest, we need to turn it off to prevent conflict with custom manifest
    if(NOT IS_VS6_BUILD)
        target_link_options(z_generals PRIVATE
            "/MANIFEST:NO"
        )
    endif()

    target_sources(z_generals PRIVATE
        RTS.RC
    )
endif()

# Strip debug symbols to separate file for MinGW Release builds
# This creates generalszh${RTS_BUILD_OUTPUT_SUFFIX}.exe.debug (similar to MSVC .pdb files)
if(MINGW AND COMMAND add_debug_strip_target)
    add_debug_strip_target(z_generals)
endif()

# TheSuperHackers @feature githubawn 29/07/2026 Emscripten (WebAssembly) link wiring.
# Produces an .html shell plus the .js/.wasm next to it, in one predictable directory
# a web server can be pointed at. Memory growth, threads and exceptions are set
# globally in cmake/compilers.cmake because the dependencies need them too; only the
# executable-specific settings live here.
if(EMSCRIPTEN)
    set_target_properties(z_generals PROPERTIES
        SUFFIX ".html"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/web")

    target_link_options(z_generals PRIVATE
        # Drop DWARF from the shipped module. The dependencies compile with debug
        # info even in Release, which is most of the module's size, and it is dead
        # weight for anything but local debugging.
        $<$<NOT:$<CONFIG:Debug>>:-g0>
        # Asyncify lets the engine's blocking wait loops (e.g. the "all players
        # loaded" barrier in GameLogic::startNewGame) yield to the browser event
        # loop through emscripten_sleep() instead of deadlocking, since network
        # messages are only delivered when control returns to the event loop.
        "-sASYNCIFY"
        # Pre-spawn the Web Worker pool the engine's threads run on. Pre-spawning
        # avoids a deadlock when a thread is created while the main thread waits;
        # POOL_SIZE_STRICT=0 lets the pool grow instead of erroring.
        "-sPTHREAD_POOL_SIZE=8"
        "-sPTHREAD_POOL_SIZE_STRICT=0"
        "-sINITIAL_MEMORY=536870912"
        "-sSTACK_SIZE=4MB"
        "-sFORCE_FILESYSTEM=1"
        # bgfx renders through WebGL 2 only.
        "-sMIN_WEBGL_VERSION=2"
        "-sMAX_WEBGL_VERSION=2"
        "-sFULL_ES3=1"
        "-sEXIT_RUNTIME=0"
        "-sEXPORTED_RUNTIME_METHODS=['FS','IDBFS','ccall','cwrap','callMain','stringToUTF8']"
        # IDBFS backs the options/save directory with IndexedDB so settings survive a
        # reload; the game mounts it in SDL3Main.
        "-lidbfs.js"
        # TheSuperHackers @build githubawn 30/07/2026 LAN multiplayer over WebSockets:
        # udp.cpp reimplements the UDP class on the emscripten_websocket_* API to tunnel the
        # game's datagrams to relay.py, and that API lives in the websocket.js JS library.
        # stringToUTF8 (exported above) is used from EM_ASM to build the relay URL from
        # location.hostname.
        "-lwebsocket.js"
        # The browser has no system fonts and no Fontconfig, so the font the text
        # renderer uses is embedded in the module (see cmake/webfont.cmake).
        # SHELL: keeps each flag with its argument: CMake de-duplicates repeated link
        # options, which would drop the second --embed-file and leave its path parsed
        # as a stray input file.
        "SHELL:--embed-file ${GGC_WEB_FONT_REGULAR}@/fonts/DejaVuSans.ttf"
        "SHELL:--embed-file ${GGC_WEB_FONT_BOLD}@/fonts/DejaVuSans-Bold.ttf"
        "SHELL:--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html"
    )

    # Relink when the shell changes.
    set_property(TARGET z_generals APPEND PROPERTY LINK_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html")
endif()

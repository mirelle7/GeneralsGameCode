# Interface libraries to set common defines and includes, avoid duplication later
add_library(core_wwcommon INTERFACE)

target_link_libraries(core_wwcommon INTERFACE
    deps_dbghelp
    deps_usp10
    stlport
)

if(NOT GGC_RENDER_BACKEND STREQUAL "bgfx")
    target_link_libraries(core_wwcommon INTERFACE
        d3d8lib
    )
endif()

# TheSuperHackers @build bobtista 29/04/2026 milesloader is Win-only (the
# Miles Sound System runtime loader). On non-Win we use OpenAL instead.
if(WIN32)
    target_link_libraries(core_wwcommon INTERFACE
        deps_miles
    )
endif()

target_include_directories(core_wwcommon INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
if(UNIX OR ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    target_include_directories(core_wwcommon INTERFACE compat/win32_shims)
    # TheSuperHackers @build bobtista 13/06/2026 — Force-include the windows.h
    # shim so that DX8 SDK headers (d3d8types.h etc.) which use LONG/INT/HWND
    # without including windows.h themselves compile cleanly on NDK Clang.
    # TheSuperHackers @build githubawn 18/06/2026 Skip under the Xcode generator:
    # Xcode emits this -include before its precompiled prefix-header include,
    # which makes clang ignore the PCH. On Xcode, windows.h is instead supplied
    # via the deps_utility PCH (see Dependencies/Utility/CMakeLists.txt).
    if(NOT XCODE OR CMAKE_DISABLE_PRECOMPILE_HEADERS)
        target_compile_options(core_wwcommon INTERFACE
            -include ${CMAKE_CURRENT_SOURCE_DIR}/compat/win32_shims/windows.h
        )
    endif()
endif()

if(NOT WIN32)
    # TheSuperHackers @build bobtista 29/04/2026 BEFORE so the compat shims
    # (e.g. dinput.h) take priority over headers that come along for the ride
    # via FetchContent (the dx8 SDK ships a Win-flavored dinput.h that fails
    # to parse on macOS/Linux).
    target_include_directories(core_wwcommon BEFORE INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/compat/win32_shims
    )
endif()

add_subdirectory(WW3D2)
add_subdirectory(WWAudio)
add_subdirectory(WWDebug)
add_subdirectory(WWDownload)
add_subdirectory(WWLib)
add_subdirectory(WWMath)
add_subdirectory(WWSaveLoad)
add_subdirectory(WWStub)

# Helpful interface to bundle the ww modules together.
add_library(core_wwvegas INTERFACE)

target_include_directories(core_wwvegas INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
if(UNIX OR ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
    target_include_directories(core_wwvegas INTERFACE compat/win32_shims)
    if(NOT XCODE OR CMAKE_DISABLE_PRECOMPILE_HEADERS)
        target_compile_options(core_wwvegas INTERFACE
            -include ${CMAKE_CURRENT_SOURCE_DIR}/compat/win32_shims/windows.h
        )
    endif()
endif()

# TheSuperHackers @build bobtista 29/04/2026 Propagate the win32 shim include
# path through core_wwvegas so consumers (z_gameengine, etc.) can resolve
# <windows.h> and friends to the compat layer without each linking core_wwcommon
# transitively. core_wwlib only links core_wwcommon PRIVATE.
if(NOT WIN32)
    target_include_directories(core_wwvegas BEFORE INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/compat/win32_shims
    )
endif()

target_link_libraries(core_wwvegas INTERFACE
    # core_ww3d2
    # core_wwaudio
    core_wwdebug
    # core_wwdownload
    core_wwlib
    core_wwmath
    core_wwsaveload
)

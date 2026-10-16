# cmake/openal.cmake
#
# TheSuperHackers @build bobtista 28/04/2026 OpenAL Soft dependency for the
# GeneralsMD macOS-native audio branch.

if(SAGE_USE_OPENAL)
    if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
        # TheSuperHackers @build 27/06/2026 Emscripten ships its own OpenAL
        # implementation (AL/*.h headers in the sysroot, mapped onto Web Audio).
        # Link the built-in -lopenal instead of compiling OpenAL Soft for wasm.
        message(STATUS "Using Emscripten's built-in OpenAL for GeneralsMD audio")
        if(NOT TARGET OpenAL::OpenAL)
            add_library(OpenAL::OpenAL INTERFACE IMPORTED GLOBAL)
            set_target_properties(OpenAL::OpenAL PROPERTIES
                INTERFACE_LINK_LIBRARIES "openal"
            )
        endif()
    else()
        message(STATUS "Configuring OpenAL Soft for GeneralsMD audio")

        include(FetchContent)

        FetchContent_Declare(
            openal_soft
            URL "https://github.com/kcat/openal-soft/archive/refs/tags/1.24.2.tar.gz"
            URL_HASH "SHA256=7efd383d70508587fbc146e4c508771a2235a5fc8ae05bf6fe721c20a348bd7c"
        )

        set(ALSOFT_INSTALL OFF CACHE BOOL "" FORCE)
        set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(ALSOFT_TESTS OFF CACHE BOOL "" FORCE)
        set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)
        set(ALSOFT_NO_CONFIG_UTIL ON CACHE BOOL "" FORCE)

        # TheSuperHackers @build 29/06/2026 Force OpenAL Soft static on iOS only.
        # The default shared libopenal.dylib links via @rpath but the iOS .app has
        # no Frameworks/ to embed it into, so dyld aborts before main() at launch
        # (same class of failure as the static SDL3/miles fix). Other platforms keep
        # the default shared build (Android packages the .so; desktop resolves it).
        if(CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
            set(LIBTYPE "STATIC" CACHE STRING "" FORCE)
        endif()

        # TheSuperHackers @build githubawn 29/06/2026 Switch uses -mtp=soft which
        # routes thread_local through __aarch64_read_tp (libgcc soft-TLS).  That
        # symbol is unreachable in a fully-static link against libnx.  Disabling
        # TLS makes OpenAL fall back to a plain global context pointer, which is
        # sufficient for Switch (single audio thread, no concurrent AL contexts).
        if(CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
            set(ALSOFT_NO_TLS ON CACHE BOOL "" FORCE)
        endif()

        # TheSuperHackers @build githubawn 16/10/2026 OpenAL Soft's UWP backend needs
        # C++/WinRT and otherwise downloads it with nuget. The Windows SDK already ships
        # the cppwinrt headers (on the dev shell INCLUDE path), so hand it a target.
        if(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore" AND NOT TARGET Microsoft::CppWinRT)
            add_library(Microsoft::CppWinRT INTERFACE IMPORTED GLOBAL)
        endif()

        FetchContent_MakeAvailable(openal_soft)

        # OpenAL Soft 1.24's UWP code still uses the deprecated /await coroutines,
        # which current MSVC STL rejects unless silenced.
        if(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
            foreach(_t OpenAL alsoft.common)
                if(TARGET ${_t})
                    target_compile_definitions(${_t} PRIVATE _SILENCE_EXPERIMENTAL_COROUTINE_DEPRECATION_WARNINGS)
                endif()
            endforeach()
            # The WASAPI backend's MMCSS calls live in avrt.dll, which is not in the UWP
            # API set; link no-op stubs instead so OpenAL32.dll loads on Xbox.
            add_library(ggc_avrt_stub STATIC "${CMAKE_CURRENT_LIST_DIR}/uwp/avrt_stub.c")
            target_link_libraries(OpenAL PRIVATE ggc_avrt_stub)
        endif()
    endif()
endif()

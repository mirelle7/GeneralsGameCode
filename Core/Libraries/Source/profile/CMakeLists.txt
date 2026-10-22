set(PROFILE_SRC
    "Include/profile/profile_doc.h"
    "Include/profile/profile_funclevel.h"
    "Include/profile/profile_highlevel.h"
    "Include/profile/profile_result.h"
    "Include/profile/profile.h"
    "internal_cmd.h"
    "internal_funclevel.h"
    "internal_highlevel.h"
    "internal_result.h"
    "internal.h"
    "profile_cmd.cpp"
    "profile_funclevel.cpp"
    "profile_highlevel.cpp"
    "profile_result.cpp"
    "profile.cpp"
)

add_library(core_profile_legacy STATIC)

target_sources(core_profile_legacy PRIVATE ${PROFILE_SRC})

target_include_directories(core_profile_legacy PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
)

target_precompile_headers(core_profile_legacy PRIVATE
    [["Precompiled/precompiled.h"]] # Must be first
    "Include/profile/profile.h"
    "internal.h"
    <windows.h>
)

target_link_libraries(core_profile_legacy PRIVATE
    depsi_always # Gives force included headers. Link early.
    core_config
    deps_utility
    stlport
)

# core_debug is Windows-only (see Core/Libraries/Source/debug/CMakeLists.txt).
if(WIN32)
    target_link_libraries(core_profile_legacy PRIVATE
        core_debug
    )
else()
    # The debug headers that internal.h reads are still needed. -iquote, not -I: an
    # angle-bracket <debug/...> must keep resolving to libstdc++'s own headers.
    target_compile_options(core_profile_legacy PRIVATE
        -iquote ${CMAKE_CURRENT_SOURCE_DIR}/../debug/Include
    )
endif()

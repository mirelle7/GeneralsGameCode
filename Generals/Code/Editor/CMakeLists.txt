# TheSuperHackers @feature In-game WorldBuilder editor, see Core/Editor.

add_library(g_editor STATIC)

target_link_libraries(g_editor PRIVATE
    corei_editor
    gi_always
    gi_main
)

target_link_libraries(g_editor PUBLIC
    g_gameenginedevice
    imgui
)

target_include_directories(g_editor PUBLIC
    ${CMAKE_SOURCE_DIR}/Core/Editor/Include
)

if(NOT WIN32)
    target_include_directories(g_editor BEFORE PRIVATE
        ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/compat/win32_shims
    )
endif()

target_precompile_headers(g_editor PRIVATE
    [["Precompiled/BasePrecomp.h"]] # Must be first
    <windows.h>
    [["Common/STLTypedefs.h"]]
    [["Common/SubsystemInterface.h"]]
    [["WWLib/INI.h"]]
    [["WWLib/WWCommon.h"]]
)

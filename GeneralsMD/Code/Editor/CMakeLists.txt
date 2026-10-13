# TheSuperHackers @feature In-game WorldBuilder editor, see Core/Editor.

add_library(z_editor STATIC)

target_link_libraries(z_editor PRIVATE
    corei_editor
    zi_always
    zi_main
)

target_link_libraries(z_editor PUBLIC
    z_gameenginedevice
    imgui
)

target_include_directories(z_editor PUBLIC
    ${CMAKE_SOURCE_DIR}/Core/Editor/Include
)

if(NOT WIN32)
    target_include_directories(z_editor BEFORE PRIVATE
        ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/compat/win32_shims
    )
endif()

target_precompile_headers(z_editor PRIVATE
    [["Precompiled/BasePrecomp.h"]] # Must be first
    <windows.h>
    [["Common/STLTypedefs.h"]]
    [["Common/SubsystemInterface.h"]]
    [["WWLib/INI.h"]]
    [["WWLib/WWCommon.h"]]
)

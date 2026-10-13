# cmake/imgui.cmake
#
# TheSuperHackers @feature Dear ImGui dependency for the in-game WorldBuilder
# (RTS_BUILD_OPTION_EDITOR). The docking branch is used so the editor can
# recreate the classic WorldBuilder panel layout.
#
# Only the core library and the platform backends are built here. Rendering
# goes through the engine's own render backend (DX8 or bgfx), see
# Core/Editor/Source/ImGuiRenderer.cpp, so no ImGui renderer backend is used.

if(RTS_BUILD_OPTION_EDITOR)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.91.9b-docking
        GIT_SHALLOW    TRUE
    )

    FetchContent_MakeAvailable(imgui)

    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    )

    target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${imgui_SOURCE_DIR}/misc/cpp
    )

    target_compile_definitions(imgui PUBLIC
        IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    )

    if(SAGE_USE_SDL3)
        target_sources(imgui PRIVATE ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp)
        target_link_libraries(imgui PUBLIC sdl3lib)
    elseif(WIN32 OR "${CMAKE_SYSTEM}" MATCHES "Windows")
        target_sources(imgui PRIVATE ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp)
        target_link_libraries(imgui PUBLIC dwmapi gdi32)
    endif()

    set_target_properties(imgui PROPERTIES FOLDER "Dependencies/imgui")
endif()

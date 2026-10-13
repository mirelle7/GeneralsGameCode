# TheSuperHackers @feature In-game WorldBuilder editor (Dear ImGui). The sources are shared by both
# games and compiled per game into z_editor / g_editor, like the other Core interface libraries.

set(EDITOR_SRC
    Include/Editor/EditorDocument.h
    Include/Editor/EditorUIHooks.h
    Include/Editor/EditorViewport.h
    Include/Editor/ImGuiRenderer.h
    Include/Editor/WBHeightMap.h
    Include/Editor/WHeightMapEdit.h
    Include/Editor/WorldBuilderEditor.h
    Source/EditorDocument.cpp
    Source/EditorViewport.cpp
    Source/ImGuiRenderer.cpp
    Source/WBHeightMap.cpp
    Source/WHeightMapEdit.cpp
    Source/WorldBuilderEditor.cpp
)

add_library(corei_editor INTERFACE)

target_sources(corei_editor INTERFACE ${EDITOR_SRC})

target_include_directories(corei_editor INTERFACE Include)

target_link_libraries(corei_editor INTERFACE imgui)

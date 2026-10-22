FetchContent_Declare(
    dx8
    GIT_REPOSITORY https://github.com/TheSuperHackers/min-dx8-sdk.git
    GIT_TAG        7bddff8c01f5fb931c3cb73d4aa8e66d303d97bc
)

FetchContent_MakeAvailable(dx8)

# Off Windows there are no DX8 libraries, only the headers. The D3DX8 helpers the engine
# calls come from Core/Libraries/Source/WWVegas/WW3D2/D3DXStubs.cpp.
if(NOT WIN32)
    set_property(TARGET d3d8lib PROPERTY INTERFACE_LINK_LIBRARIES "")
    set_property(TARGET d3d8lib PROPERTY INTERFACE_LINK_DIRECTORIES "")
    set_property(TARGET d3d8lib PROPERTY INTERFACE_LINK_OPTIONS "")
endif()

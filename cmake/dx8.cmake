FetchContent_Declare(
    dx8
    GIT_REPOSITORY https://github.com/TheSuperHackers/min-dx8-sdk.git
    GIT_TAG        7bddff8c01f5fb931c3cb73d4aa8e66d303d97bc
)

FetchContent_MakeAvailable(dx8)

# TheSuperHackers @build bobtista 22/04/2026
# There are no 64-bit d3d8/d3dx8 import libraries. On 64-bit builds keep
# the DX8 headers (include dir is still on d3d8lib) but strip the bundled
# 32-bit libraries off the shared d3d8lib INTERFACE target. The D3DX8
# helpers come from Core/Libraries/Source/WWVegas/WW3D2/D3DXStubs.cpp,
# and dinput8/dxguid resolve to the Windows SDK libraries.
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    set_property(TARGET d3d8lib PROPERTY INTERFACE_LINK_LIBRARIES "")
    set_property(TARGET d3d8lib PROPERTY INTERFACE_LINK_DIRECTORIES "")
    set_property(TARGET d3d8lib PROPERTY INTERFACE_LINK_OPTIONS "")
    target_link_libraries(d3d8lib INTERFACE dinput8 dxguid)
endif()

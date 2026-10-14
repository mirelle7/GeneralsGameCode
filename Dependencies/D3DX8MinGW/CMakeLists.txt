# TheSuperHackers @build Link D3DX8 statically for MinGW-w64.
#
# MinGW-w64 only ships an import library for the debug d3dx8d.dll, which players do not have.
# Instead, link the static d3dx8.lib of the DirectX 8 SDK (min-dx8-sdk), like the MSVC builds do.
#
# Two adjustments make the MSVC-built library link with MinGW-w64:
# - Its init.obj defines DirectX GUIDs that MinGW-w64's libdxguid.a defines too. A patched copy of
#   the library keeps only the GUIDs libdxguid.a lacks global; the duplicates become local.
# - It references a few MSVC runtime helpers that msvcrt.dll does not export under those names.
#   D3DX8MsvcRuntime.cpp supplies them.

set(D3DX8_SDK_LIB "${dx8_SOURCE_DIR}/d3dx8.lib")
set(D3DX8_WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}/d3dx8")
set(D3DX8_MINGW_LIB "${D3DX8_WORK_DIR}/d3dx8_mingw.lib")
set(D3DX8_INIT_OBJ "obj/i386/init.obj")

if(NOT EXISTS "${D3DX8_MINGW_LIB}" OR "${D3DX8_SDK_LIB}" IS_NEWER_THAN "${D3DX8_MINGW_LIB}")
    file(MAKE_DIRECTORY "${D3DX8_WORK_DIR}/obj/i386")
    file(COPY_FILE "${D3DX8_SDK_LIB}" "${D3DX8_WORK_DIR}/d3dx8_patch.lib")

    # The archive stores members with directories, so match them by full path (P).
    execute_process(
        COMMAND "${CMAKE_AR}" xP d3dx8_patch.lib "${D3DX8_INIT_OBJ}"
        COMMAND_ERROR_IS_FATAL ANY
        WORKING_DIRECTORY "${D3DX8_WORK_DIR}")
    execute_process(
        COMMAND "${CMAKE_OBJCOPY}"
            -G _IID_ID3DXFont
            -G _IID_ID3DXRenderToEnvMap
            -G _IID_ID3DXRenderToSurface
            -G _IID_ID3DXSprite
            "${D3DX8_INIT_OBJ}" init_mingw.obj
        COMMAND_ERROR_IS_FATAL ANY
        WORKING_DIRECTORY "${D3DX8_WORK_DIR}")
    execute_process(
        COMMAND "${CMAKE_AR}" dP d3dx8_patch.lib "${D3DX8_INIT_OBJ}"
        COMMAND_ERROR_IS_FATAL ANY
        WORKING_DIRECTORY "${D3DX8_WORK_DIR}")
    execute_process(
        COMMAND "${CMAKE_AR}" rs d3dx8_patch.lib init_mingw.obj
        COMMAND_ERROR_IS_FATAL ANY
        WORKING_DIRECTORY "${D3DX8_WORK_DIR}")

    file(RENAME "${D3DX8_WORK_DIR}/d3dx8_patch.lib" "${D3DX8_MINGW_LIB}")
endif()

add_library(d3dx8_msvc_runtime STATIC D3DX8MsvcRuntime.cpp)

# The min-dx8-sdk links "d3dx8d" for MinGW. A target with that name takes precedence over the
# MinGW import library of the same name.
add_library(d3dx8d INTERFACE IMPORTED GLOBAL)
target_link_libraries(d3dx8d INTERFACE
    "${D3DX8_MINGW_LIB}"
    d3dx8_msvc_runtime
    dxguid
    msvcrt
    uuid
)

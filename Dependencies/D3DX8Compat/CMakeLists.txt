# Links the MSVC compiled d3dx8.lib from min-dx8-sdk into MinGW-w64 Clang/LLD builds.
#
# The MinGW-w64 dx8 setup links against libd3dx8d.a, the import library of the debug D3DX8 runtime
# (d3dx8d.dll), which only came with the DirectX 8 SDK and is not available on end user systems.
# LLD can link the MSVC static library directly, given a few MSVC runtime helpers (D3DX8MsvcCompat.cpp).
#
# Defining a target named d3dx8d makes every existing reference to d3dx8d (d3d8lib, d3dx8 alias) use it.

add_library(deps_d3dx8_msvc_compat STATIC)

target_sources(deps_d3dx8_msvc_compat PRIVATE
    D3DX8MsvcCompat.cpp
)

# Strip the VC6 CodeView debug info from d3dx8.lib, which LLD does not understand and warns about.
set(D3DX8_LIB "${dx8_SOURCE_DIR}/d3dx8.lib")
find_program(LLVM_OBJCOPY_EXECUTABLE NAMES llvm-objcopy)
if(LLVM_OBJCOPY_EXECUTABLE)
    set(D3DX8_STRIPPED_LIB "${CMAKE_CURRENT_BINARY_DIR}/d3dx8.lib")
    execute_process(
        COMMAND ${LLVM_OBJCOPY_EXECUTABLE} --strip-debug "${D3DX8_LIB}" "${D3DX8_STRIPPED_LIB}"
        RESULT_VARIABLE D3DX8_STRIP_RESULT
    )
    if(D3DX8_STRIP_RESULT EQUAL 0)
        set(D3DX8_LIB "${D3DX8_STRIPPED_LIB}")
    endif()
endif()

add_library(d3dx8d INTERFACE)

target_link_libraries(d3dx8d INTERFACE
    "${D3DX8_LIB}"
    deps_d3dx8_msvc_compat
)

target_link_options(d3dx8d INTERFACE
    # d3dx8.lib requests the VC6 iostream library, which it does not actually need.
    "LINKER:-Xlink=-nodefaultlib:libci.lib"
)

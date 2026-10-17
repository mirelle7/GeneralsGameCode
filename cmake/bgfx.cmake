# cmake/bgfx.cmake
#
# TheSuperHackers @refactor bobtista 10/04/2026 bgfx dependency for the
# GGC_RENDER_BACKEND=bgfx build. Included from cmake/render-backend.cmake.
#
# Pulls in bgfx via the community bgfx.cmake wrapper, which internally
# fetches bgfx/bx/bimg as git submodules. We pin a specific bgfx.cmake
# SHA for reproducibility.
#
# This file is NOT included when GGC_RENDER_BACKEND is dx8.

# Disable bgfx features we don't need. These must be set BEFORE
# FetchContent_MakeAvailable so bgfx.cmake picks them up at configure time.
# TheSuperHackers @build githubawn 16/10/2026 HLSL profile for the dx11 shaders.
# s_5_0 needs D3D feature level 11_0; UWP games on Xbox One only get D3D11 at
# feature level 10_1, so the WindowsStore build defaults to s_4_0.
if(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
    set(GGC_BGFX_DX11_SHADER_PROFILE "s_4_0" CACHE STRING "bgfx shaderc profile for dx11 shaders")
else()
    set(GGC_BGFX_DX11_SHADER_PROFILE "s_5_0" CACHE STRING "bgfx shaderc profile for dx11 shaders")
endif()

set(BGFX_BUILD_EXAMPLES       OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TESTS          OFF CACHE BOOL "" FORCE)
# TheSuperHackers @build githubawn 29/07/2026 A cross build (Emscripten) cannot run
# the shaderc it would produce, because that binary targets the game's platform, not
# the build host. Skip the tools in the cross tree and compile a host shaderc
# separately below.
# TheSuperHackers @build githubawn 16/10/2026 UWP (WindowsStore) tools would be
# app-container executables that cannot run on the build host; like Android, iOS and
# Nintendo Switch, use a host-built shaderc instead.
if(CMAKE_CROSSCOMPILING OR ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch" OR CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
    set(GGC_BGFX_HOST_SHADERC TRUE CACHE INTERNAL "")
else()
    set(GGC_BGFX_HOST_SHADERC FALSE CACHE INTERNAL "")
endif()
if(GGC_BGFX_HOST_SHADERC)
    set(BGFX_BUILD_TOOLS          OFF CACHE BOOL "" FORCE)
    set(BGFX_BUILD_TOOLS_BIN2C    OFF CACHE BOOL "" FORCE)
    set(BGFX_BUILD_TOOLS_SHADER   OFF CACHE BOOL "" FORCE)
else()
    set(BGFX_BUILD_TOOLS          ON  CACHE BOOL "" FORCE)  # shaderc is mandatory
    set(BGFX_BUILD_TOOLS_BIN2C    ON  CACHE BOOL "" FORCE)
    set(BGFX_BUILD_TOOLS_SHADER   ON  CACHE BOOL "" FORCE)
endif()
set(BGFX_BUILD_TOOLS_GEOMETRY OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_TEXTURE  OFF CACHE BOOL "" FORCE)
set(BGFX_INSTALL              OFF CACHE BOOL "" FORCE)
set(BGFX_CUSTOM_TARGETS       OFF CACHE BOOL "" FORCE)
# TheSuperHackers @build githubawn 29/07/2026 bgfx's WebGPU backend needs the Dawn/
# emdawnwebgpu port, which we do not ship. WebGL 2 through the GLES backend is what
# the web build uses, so keep WebGPU out of the compile.
set(BGFX_CONFIG_RENDERER_WEBGPU OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    bgfx_cmake
    GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake.git
    # TheSuperHackers @bugfix bobtista 30/04/2026 Bumped from 668550d
    # to current HEAD to pick up bgfx Metal fixes — most importantly
    # #3683 (depth/stencil store action on the main swap chain w/ MSAA)
    # and #3685 (dynamic buffer alignment on Metal). Older bgfx pin
    # produced a malformed Metal pipeline descriptor which Apple's AGX
    # driver crashed on while compiling the BlitVertexFastClear and
    # EndOfTile helper shaders.
    GIT_TAG        c480227693fccc749c36994d175bace20ba2fce2
    # Nested submodules (bgfx, bx, bimg) are cloned recursively by FetchContent.
    GIT_SUBMODULES_RECURSE TRUE
    # TheSuperHackers @bugfix bobtista Bounds-check the write into the fixed Metal
    # per-frame uniform buffer. bgfx advances the offset by the program's full CB size
    # every submit, so a heavy scene (China Nuke general challenge, ~5000+ draws/frame)
    # overran the 8MB arena and wrote past its end, crashing or hanging the render thread.
    # Idempotent so reconfigure on an already-patched tree is a no-op.
    # TheSuperHackers @build bobtista 13/07/2026 --ignore-whitespace so the patch applies
    # when core.autocrlf gives the patch file CRLF endings on Windows checkouts.
    PATCH_COMMAND sh -c "git -C bgfx apply --reverse --check --ignore-whitespace '${CMAKE_CURRENT_LIST_DIR}/patches/bgfx-metal-uniform-buffer.patch' 2>/dev/null || git -C bgfx apply --ignore-whitespace '${CMAKE_CURRENT_LIST_DIR}/patches/bgfx-metal-uniform-buffer.patch'"
)

FetchContent_GetProperties(bgfx_cmake)
if(NOT bgfx_cmake_POPULATED)
    FetchContent_Populate(bgfx_cmake)

    if(CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
        # Patch bx/include/bx/platform.h for GCC 15 compatibility
        set(PLATFORM_H "${bgfx_cmake_SOURCE_DIR}/bx/include/bx/platform.h")
        if(EXISTS "${PLATFORM_H}")
            message(STATUS "Patching bx/include/bx/platform.h for GCC 15 compatibility...")
            file(READ "${PLATFORM_H}" PLATFORM_CONTENT)
            string(REPLACE "#elif defined(__has_builtin) && __has_builtin(__is_target_os) && __is_target_os(xros)" "#elif defined(__clang__) && defined(__has_builtin)\n#\tif __has_builtin(__is_target_os)\n#\t\tif __is_target_os(xros)\n#\t\t\tundef  BX_PLATFORM_VISIONOS\n#\t\t\tdefine BX_PLATFORM_VISIONOS 1\n#\t\tendif\n#\tendif" PLATFORM_CONTENT "${PLATFORM_CONTENT}")
            file(WRITE "${PLATFORM_H}" "${PLATFORM_CONTENT}")
        endif()

        # Patch bx/src/os.cpp to not include dlfcn.h on Nintendo Switch
        set(OS_CPP "${bgfx_cmake_SOURCE_DIR}/bx/src/os.cpp")
        if(EXISTS "${OS_CPP}")
            message(STATUS "Patching bx/src/os.cpp for Nintendo Switch compatibility...")
            file(READ "${OS_CPP}" OS_CONTENT)
            string(REPLACE "!BX_PLATFORM_PS4\r\n#\t\tinclude <dlfcn.h>" "!BX_PLATFORM_PS4 && !BX_PLATFORM_NX\r\n#\t\tinclude <dlfcn.h>" OS_CONTENT "${OS_CONTENT}")
            string(REPLACE "!BX_PLATFORM_PS4\n#\t\tinclude <dlfcn.h>" "!BX_PLATFORM_PS4 && !BX_PLATFORM_NX\n#\t\tinclude <dlfcn.h>" OS_CONTENT "${OS_CONTENT}")
            file(WRITE "${OS_CPP}" "${OS_CONTENT}")
        endif()

        # TheSuperHackers @build githubawn 03/07/2026 bgfx bundles its own khronos EGL
        # headers whose eglplatform.h has no Nintendo Switch case (-> #error "Platform
        # not recognized", EGLNativeWindowType undefined). switch-mesa uses opaque void*
        # native types; add an NX case so bgfx's OpenGLES/EGL backend compiles. We pass
        # libnx's NWindow* (as void*) via GetNativeWindowHandle.
        set(EGLPLAT_H "${bgfx_cmake_SOURCE_DIR}/bgfx/3rdparty/khronos/EGL/eglplatform.h")
        if(EXISTS "${EGLPLAT_H}")
            file(READ "${EGLPLAT_H}" EGLPLAT_CONTENT)
            # Idempotent: only patch if the NX case is not already present. The
            # replacement below re-emits the "#else / #error" search pattern, so
            # without this guard every reconfigure would splice in another block.
            string(FIND "${EGLPLAT_CONTENT}" "__SWITCH__" _nx_already)
            if(_nx_already EQUAL -1)
                message(STATUS "Patching bgfx khronos eglplatform.h for Nintendo Switch...")
                # NOTE: use plain ';' here, not '\;'. Inside a quoted set() the
                # semicolon is already literal; escaping it leaves a stray '\' in
                # the generated header ("stray '\' in program" compile error).
                set(_nx_egl "#elif defined(__SWITCH__) || defined(__NX__)\n\ntypedef void *EGLNativeDisplayType;\ntypedef void *EGLNativePixmapType;\ntypedef void *EGLNativeWindowType;\n\n#else\n#error \"Platform not recognized\"")
                string(REPLACE "#else\r\n#error \"Platform not recognized\"" "${_nx_egl}" EGLPLAT_CONTENT "${EGLPLAT_CONTENT}")
                string(REPLACE "#else\n#error \"Platform not recognized\"" "${_nx_egl}" EGLPLAT_CONTENT "${EGLPLAT_CONTENT}")
                file(WRITE "${EGLPLAT_H}" "${EGLPLAT_CONTENT}")
            endif()
        endif()
    endif()

    add_subdirectory(${bgfx_cmake_SOURCE_DIR} ${bgfx_cmake_BINARY_DIR})
endif()

# TheSuperHackers @build githubawn 28/06/2026 bgfx defaults BGFX_CONFIG_RENDERER_VULKAN
# to OFF on Apple (it targets Android/Linux/Windows). When we ask for the Vulkan
# renderer on macOS (via MoltenVK), the backend is compiled but never registered, so
# bgfx::init silently falls back to Metal and then chokes feeding it SPIR-V. Force the
# config on so renderer_vk registers and Vulkan can actually be selected. config.h uses
# #ifndef, so this define is respected without redefinition conflicts.
if(GGC_RENDER_BACKEND STREQUAL "bgfx" AND GGC_BGFX_RENDERER STREQUAL "vulkan" AND TARGET bgfx)
    target_compile_definitions(bgfx PRIVATE BGFX_CONFIG_RENDERER_VULKAN=1)
    message(STATUS "Forced BGFX_CONFIG_RENDERER_VULKAN=1 (Apple/MoltenVK)")
endif()

# TheSuperHackers @build githubawn 28/06/2026 Same story for desktop OpenGL: bgfx
# defaults BGFX_CONFIG_RENDERER_OPENGL to 0 on Apple (Metal is preferred there), so
# renderer_gl is compiled but never registered and bgfx::init falls back to Metal.
# Force GL 4.1 (the macOS core-profile ceiling) so the GL backend is selectable.
if(GGC_RENDER_BACKEND STREQUAL "bgfx" AND GGC_BGFX_RENDERER STREQUAL "glsl" AND TARGET bgfx)
    target_compile_definitions(bgfx PRIVATE BGFX_CONFIG_RENDERER_OPENGL=41)
    message(STATUS "Forced BGFX_CONFIG_RENDERER_OPENGL=41 (Apple desktop GL)")
endif()

# TheSuperHackers @feature githubawn 03/07/2026 Nintendo Switch renders through the
# OpenGL ES backend on switch-mesa (nouveau), NOT Vulkan (there is no Vulkan ICD in the
# Switch homebrew environment; bgfx's native NX path is NVN/licensed which we do not
# have). bgfx defaults BGFX_CONFIG_RENDERER_OPENGLES off for BX_PLATFORM_NX, so force
# the GLES backend on and force the NX glcontext to use EGL (like Android) so renderer_gl
# registers and can create a context on the SDL Switch driver's default NWindow.
if(GGC_RENDER_BACKEND STREQUAL "bgfx" AND CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch" AND TARGET bgfx)
    target_compile_definitions(bgfx PRIVATE
        BGFX_CONFIG_RENDERER_OPENGLES=30
        BGFX_CONFIG_RENDERER_VULKAN=0
        BGFX_USE_EGL=1)
    # switch-mesa GLES/EGL stack (nouveau). PUBLIC so it propagates into the game link.
    target_link_libraries(bgfx PUBLIC EGL GLESv2 glapi drm_nouveau)
    message(STATUS "Forced BGFX_CONFIG_RENDERER_OPENGLES=30 + EGL (Nintendo Switch / switch-mesa)")
endif()

# TheSuperHackers @build githubawn 16/10/2026 bx detects UWP (BX_PLATFORM_WINRT, which
# makes bgfx create a CoreWindow swap chain) from WINAPI_FAMILY. Set it on the bgfx
# libraries only; the game code keeps the desktop API family for its Win32 headers.
if(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
    foreach(_t bgfx bx bimg bimg_decode bimg_encode)
        if(TARGET ${_t})
            target_compile_definitions(${_t} PRIVATE WINAPI_FAMILY=WINAPI_FAMILY_APP)
        endif()
    endforeach()
    # TheSuperHackers @performance githubawn 17/10/2026 bgfx updates part of a dynamic
    # vertex/index buffer by creating a throwaway staging buffer per call. The engine does
    # ~160 such updates per frame, and on the Xbox One UWP driver each CreateBuffer costs
    # ~0.3 ms (50 ms/frame). The staging-buffer config keeps one persistent staging buffer
    # per dynamic buffer instead.
    target_compile_definitions(bgfx PRIVATE BGFX_CONFIG_RENDERER_DIRECT3D11_USE_STAGING_BUFFER=1)
endif()

# IDE organization.
foreach(_t bgfx bx bimg shaderc bimg_decode bimg_encode)
    if(TARGET ${_t})
        set_target_properties(${_t} PROPERTIES FOLDER "Dependencies/bgfx")
    endif()
endforeach()

# TheSuperHackers @refactor bobtista 11/04/2026 bgfx shader compilation.
# bgfx shaders are authored in ".sc" files (GLSL-ish with bgfx pragmas) and
# compiled by the shaderc tool (built as part of bgfx.cmake) into per-platform
# bytecode. The --bin2c option emits a C header with the compiled bytecode as
# a uint8_t array, which we then #include from BgfxBackend.cpp and hand to
# bgfx::createShader via bgfx::makeRef.
#
# Shader output follows GGC_BGFX_RENDERER. DX11 remains the Windows default;
# Metal is the macOS default.
#
# Usage in callers:
#   ggc_compile_bgfx_shader(<source.sc>)   # once per shader file
# Then link the ggc_bgfx_shaders target into whatever library consumes the
# generated headers (from either the corei_ww3d2 INTERFACE chain or directly).
#
# The generated header ends up at ${CMAKE_BINARY_DIR}/ggc_bgfx_shaders/
# with the C array named from the basename, e.g. vs_passthrough_dx11 on
# Windows or vs_passthrough_metal / vs_passthrough_spirv elsewhere.
#
# The varying.def.sc file is resolved relative to the .sc file's directory
# and must exist there.
#
# The ggc_bgfx_shaders target is a STATIC library with a single dummy .cpp so
# the generated header files have a concrete library to live on. Making it a
# real library (not INTERFACE) is deliberate: CMake's INCLUDE_DIRECTORIES
# propagation through INTERFACE libraries was flaky for our use case and the
# dependency from consumers to the custom_command outputs wasn't firing
# reliably. STATIC + PUBLIC include dirs + link chain is the robust path.

# Shaderc include path (where bgfx_shader.sh lives in the fetched bgfx tree).
set(GGC_BGFX_SHADER_INCLUDE_DIR "${bgfx_cmake_SOURCE_DIR}/bgfx/src" CACHE INTERNAL "")

# TheSuperHackers @build githubawn 29/07/2026 Host shaderc for cross builds.
# Shader bytecode is produced at build time, so shaderc has to run on the build
# host. In a native build that is the shaderc target from bgfx.cmake. In a cross
# build (Emscripten, Android, iOS, Switch, UWP) it cannot be, so build the
# already-fetched bgfx.cmake tree a second time with the host's default toolchain
# and use the shaderc it produces.
# Pass -DGGC_SHADERC_EXE=/path/to/shaderc to skip that build and reuse an existing
# binary (CI cache, a previous native build tree, a distro package).
if(GGC_BGFX_HOST_SHADERC)
    if(DEFINED GGC_SHADERC_EXE)
        if(NOT EXISTS "${GGC_SHADERC_EXE}")
            message(FATAL_ERROR "GGC_SHADERC_EXE does not exist: ${GGC_SHADERC_EXE}")
        endif()
        message(STATUS "Using prebuilt host shaderc: ${GGC_SHADERC_EXE}")
        add_custom_target(ggc_host_shaderc)
    else()
        include(ExternalProject)

        set(_ggc_host_shaderc_prefix "${CMAKE_BINARY_DIR}/host-shaderc")
        if(CMAKE_HOST_WIN32)
            set(_ggc_host_shaderc_name "shaderc.exe")
        else()
            set(_ggc_host_shaderc_name "shaderc")
        endif()
        # Pin the output directory (for every configuration a multi-config host
        # generator might pick) so the binary lands where the custom commands look.
        set(_ggc_host_shaderc_bin "${_ggc_host_shaderc_prefix}/bin")
        set(GGC_SHADERC_EXE "${_ggc_host_shaderc_bin}/${_ggc_host_shaderc_name}")

        ExternalProject_Add(ggc_host_shaderc
            SOURCE_DIR        "${bgfx_cmake_SOURCE_DIR}"
            PREFIX            "${_ggc_host_shaderc_prefix}"
            DOWNLOAD_COMMAND  ""
            UPDATE_COMMAND    ""
            PATCH_COMMAND     ""
            INSTALL_COMMAND   ""
            BUILD_BYPRODUCTS  "${GGC_SHADERC_EXE}"
            CMAKE_ARGS
                -DCMAKE_BUILD_TYPE=Release
                -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${_ggc_host_shaderc_bin}
                -DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE=${_ggc_host_shaderc_bin}
                -DCMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG=${_ggc_host_shaderc_bin}
                -DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO=${_ggc_host_shaderc_bin}
                -DCMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL=${_ggc_host_shaderc_bin}
                -DBGFX_BUILD_EXAMPLES=OFF
                -DBGFX_BUILD_TESTS=OFF
                -DBGFX_BUILD_TOOLS=ON
                -DBGFX_BUILD_TOOLS_BIN2C=ON
                -DBGFX_BUILD_TOOLS_SHADER=ON
                -DBGFX_BUILD_TOOLS_GEOMETRY=OFF
                -DBGFX_BUILD_TOOLS_TEXTURE=OFF
                -DBGFX_INSTALL=OFF
                -DBGFX_CUSTOM_TARGETS=OFF
                -DBGFX_LIBRARY_TYPE=STATIC
            BUILD_COMMAND     ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release --target shaderc
        )
        message(STATUS "Building a host shaderc for the cross build: ${GGC_SHADERC_EXE}")
    endif()
endif()

# TheSuperHackers @build bobtista 14/06/2026 bgfx vendors stb_truetype.h here.
# WW3D2 uses it to rasterize fonts on non-Windows builds (no GDI).
set(GGC_BGFX_STB_INCLUDE_DIR "${bgfx_cmake_SOURCE_DIR}/bgfx/3rdparty" CACHE INTERNAL "")

# Shared output directory for every compiled shader header.
set(GGC_BGFX_SHADERS_OUT_DIR "${CMAKE_BINARY_DIR}/ggc_bgfx_shaders" CACHE INTERNAL "")

# One-time setup: create the ggc_bgfx_shaders target with a dummy source so
# it compiles as a real STATIC library.
function(ggc_bgfx_shaders_init)
    if(TARGET ggc_bgfx_shaders)
        return()
    endif()

    file(MAKE_DIRECTORY "${GGC_BGFX_SHADERS_OUT_DIR}")
    set(_dummy "${GGC_BGFX_SHADERS_OUT_DIR}/_ggc_bgfx_shaders_dummy.cpp")
    if(NOT EXISTS "${_dummy}")
        file(WRITE "${_dummy}"
             "// Auto-generated. Exists only so ggc_bgfx_shaders has a compilable source.\n"
             "namespace { char ggc_bgfx_shaders_anchor = 0; }\n")
    endif()

    add_library(ggc_bgfx_shaders STATIC "${_dummy}")
    set_target_properties(ggc_bgfx_shaders PROPERTIES FOLDER "Dependencies/bgfx")
    target_include_directories(ggc_bgfx_shaders PUBLIC "${GGC_BGFX_SHADERS_OUT_DIR}")
endfunction()

function(ggc_compile_bgfx_shader source_sc)
    # TheSuperHackers @build githubawn 29/07/2026 Cross builds compile shaders with
    # the host shaderc set up above; native builds keep using the in-tree target.
    # TheSuperHackers @build githubawn 17/06/2026 Cross builds (Android, iOS)
    # cannot execute the target-built shaderc on the host (an iOS/arm64 shaderc
    # is killed by macOS, an Android one cannot run on the build host).
    if(GGC_BGFX_HOST_SHADERC)
        set(_shaderc_command "${GGC_SHADERC_EXE}")
        set(_shaderc_dependency ggc_host_shaderc)
    else()
        if(NOT TARGET shaderc)
            message(FATAL_ERROR "ggc_compile_bgfx_shader: shaderc target not available. "
                                "Ensure BGFX_BUILD_TOOLS_SHADER=ON and bgfx.cmake is included.")
        endif()
        set(_shaderc_command "$<TARGET_FILE:shaderc>")
        set(_shaderc_dependency shaderc)
    endif()

    cmake_parse_arguments(_ggc_sc "" "NAME;DEFINES" "" ${ARGN})

    ggc_bgfx_shaders_init()

    get_filename_component(_sc_abs "${source_sc}" ABSOLUTE)
    get_filename_component(_sc_dir "${_sc_abs}" DIRECTORY)
    get_filename_component(_sc_name "${_sc_abs}" NAME_WE)

    if(_sc_name MATCHES "^vs_")
        set(_shader_type "vertex")
    elseif(_sc_name MATCHES "^fs_")
        set(_shader_type "fragment")
    else()
        message(FATAL_ERROR "ggc_compile_bgfx_shader: '${_sc_name}' must start with vs_ or fs_.")
    endif()

    # NAME compiles the same source under a different output/symbol name
    # (variant builds), DEFINES passes preprocessor definitions to shaderc.
    if(_ggc_sc_NAME)
        set(_sc_name "${_ggc_sc_NAME}")
    endif()
    set(_define_args "")
    if(_ggc_sc_DEFINES)
        set(_define_args --define "${_ggc_sc_DEFINES}")
    endif()

    # TheSuperHackers @feature githubawn 22/06/2026 Build a list of shader variants to
    # compile. Normally one per GGC_BGFX_RENDERER, but on Android we additionally build the
    # Vulkan (spirv) variant alongside GLES (essl) so BgfxBackend can pick at runtime
    # (Vulkan preferred, GLES fallback). Each entry is "suffix|platform|profile".
    set(_shader_variants "")
    if(GGC_BGFX_RENDERER STREQUAL "metal")
        # TheSuperHackers @bugfix bobtista 30/04/2026 The bare "metal"
        # profile compiles to MSL 1.0, which Apple Silicon's AGX driver
        # has effectively deprecated on macOS Tahoe (pipeline-state
        # compiles fault inside MTLCompiler). Target metal30-14
        # (MSL 3.0 / macOS 14) which the M1/M2/M3/M4 family all support
        # and the runtime accepts cleanly. metal22-11 also works on
        # older systems if M-family compatibility ever matters.
        list(APPEND _shader_variants "metal|osx|metal30-14")
    elseif(GGC_BGFX_RENDERER STREQUAL "vulkan")
        list(APPEND _shader_variants "spirv|linux|spirv")
    elseif(GGC_BGFX_RENDERER STREQUAL "glsl")
        # Desktop OpenGL. macOS core profile caps at GL 4.1 -> glsl 410.
        list(APPEND _shader_variants "glsl|osx|410")
    elseif(GGC_BGFX_RENDERER STREQUAL "essl")
        # TheSuperHackers @build githubawn 29/07/2026 OpenGL ES 3.0 / WebGL 2 output
        # for the Emscripten build. The "android" shaderc platform is the generic
        # GLES one; the 300_es profile matches the WebGL 2 feature level bgfx asks
        # for (GLES 3.0 shading language).
        list(APPEND _shader_variants "essl|android|300_es")
        if(ANDROID)
            list(APPEND _shader_variants "spirv|linux|spirv")
        endif()
    else()
        list(APPEND _shader_variants "dx11|windows|${GGC_BGFX_DX11_SHADER_PROFILE}")
    endif()

    set(_varying_def "${_sc_dir}/varying.def.sc")

    foreach(_variant IN LISTS _shader_variants)
        string(REPLACE "|" ";" _variant_parts "${_variant}")
        list(GET _variant_parts 0 _shader_suffix)
        list(GET _variant_parts 1 _shader_platform)
        list(GET _variant_parts 2 _shader_profile)

        set(_out_header "${GGC_BGFX_SHADERS_OUT_DIR}/${_sc_name}_${_shader_suffix}.bin.h")
        set(_varname "${_sc_name}_${_shader_suffix}")

        add_custom_command(
            OUTPUT "${_out_header}"
            COMMAND "${_shaderc_command}"
                -f "${_sc_abs}"
                -o "${_out_header}"
                --bin2c "${_varname}"
                -i "${GGC_BGFX_SHADER_INCLUDE_DIR}"
                --platform "${_shader_platform}"
                --profile "${_shader_profile}"
                --type "${_shader_type}"
                --varyingdef "${_varying_def}"
                ${_define_args}
                -O 3
            DEPENDS "${_sc_abs}" "${_varying_def}" ${_shaderc_dependency}
            COMMENT "Compiling bgfx shader ${_sc_name} (${_shader_suffix})"
            VERBATIM
        )

        target_sources(ggc_bgfx_shaders PRIVATE "${_out_header}")
        set_source_files_properties("${_out_header}" PROPERTIES
            GENERATED TRUE
            HEADER_FILE_ONLY TRUE
        )
        set_property(GLOBAL APPEND PROPERTY GGC_BGFX_SHADER_HEADERS "${_out_header}")
    endforeach()
endfunction()

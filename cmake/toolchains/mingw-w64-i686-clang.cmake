# MinGW-w64 32-bit (i686) Toolchain File using Clang/LLD
# Use with: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64-i686-clang.cmake
#
# Requires clang, lld and a MinGW-w64 i686 sysroot (headers, CRT and libstdc++),
# e.g. on Debian/Ubuntu: apt install clang lld llvm mingw-w64

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)

set(MINGW_TRIPLE i686-w64-mingw32)

# Specify the cross compiler
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_C_COMPILER_TARGET ${MINGW_TRIPLE})
set(CMAKE_CXX_COMPILER_TARGET ${MINGW_TRIPLE})

# Link with LLD
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-fuse-ld=lld")

# Binutils: prefer LLVM tools, fall back to the MinGW-w64 binutils
find_program(CMAKE_RC_COMPILER NAMES llvm-windres ${MINGW_TRIPLE}-windres)
find_program(CMAKE_AR NAMES llvm-ar ${MINGW_TRIPLE}-ar)
find_program(CMAKE_RANLIB NAMES llvm-ranlib ${MINGW_TRIPLE}-ranlib)
find_program(CMAKE_DLLTOOL NAMES llvm-dlltool ${MINGW_TRIPLE}-dlltool)

# Target environment
set(CMAKE_FIND_ROOT_PATH /usr/${MINGW_TRIPLE})

# Adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# search headers and libraries in the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Force 32-bit pointer size
set(CMAKE_SIZEOF_VOID_P 4)

# Disable MFC-dependent tools (not compatible with MinGW-w64)
set(RTS_BUILD_CORE_TOOLS OFF CACHE BOOL "Disable MFC-dependent core tools for MinGW" FORCE)
set(RTS_BUILD_GENERALS_TOOLS OFF CACHE BOOL "Disable MFC-dependent Generals tools for MinGW" FORCE)
set(RTS_BUILD_ZEROHOUR_TOOLS OFF CACHE BOOL "Disable MFC-dependent Zero Hour tools for MinGW" FORCE)

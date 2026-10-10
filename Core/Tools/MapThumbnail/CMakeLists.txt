# Map thumbnails for Windows Explorer, macOS Finder and Linux file managers.
#
# The thumbnail handlers are loaded by the operating system, so they must match its architecture
# (64-bit on current Windows and macOS) rather than the 32-bit game. Build this folder on its own:
#
#   cmake -S Core/Tools/MapThumbnail -B build/mapthumbnail
#   cmake --build build/mapthumbnail
#
# On macOS use -G Xcode, which is what builds the Quick Look extension.

cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(MapThumbnail VERSION 1.0 LANGUAGES C CXX)
    if(APPLE)
        enable_language(OBJCXX)
    endif()
    include(${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake/stb.cmake)
    set_target_properties(stb PROPERTIES SYSTEM ON)
    option(MAPTHUMBNAIL_BUILD_TESTS "Build the map thumbnail tests" ON)
else()
    option(MAPTHUMBNAIL_BUILD_TESTS "Build the map thumbnail tests" OFF)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# Static runtime, so the handler DLL loads into Explorer without a VC++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_library(mapthumbnail STATIC
    include/MapThumbnail/MapThumbnail.h
    src/FileUtil.cpp
    src/FileUtil.h
    src/GameData.cpp
    src/GameData.h
    src/MapReader.cpp
    src/MapReader.h
    src/MapThumbnail.cpp
    src/RefPack.cpp
    src/RefPack.h
    src/Settings.cpp
    src/StbImpl.cpp
)
target_include_directories(mapthumbnail PUBLIC include PRIVATE src)
target_link_libraries(mapthumbnail PRIVATE stb)
if(WIN32)
    target_compile_definitions(mapthumbnail PUBLIC UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(mapthumbnail PUBLIC advapi32 shell32 ole32)
endif()

add_executable(generals-map-thumbnailer cli/Main.cpp)
target_link_libraries(generals-map-thumbnailer PRIVATE mapthumbnail stb)
install(TARGETS generals-map-thumbnailer RUNTIME DESTINATION bin)

if(WIN32)
    add_subdirectory(windows)
elseif(APPLE)
    add_subdirectory(macos)
else()
    add_subdirectory(linux)
endif()

if(MAPTHUMBNAIL_BUILD_TESTS)
    enable_testing()
    add_executable(mapthumbnail_tests tests/MapThumbnailTest.cpp)
    target_link_libraries(mapthumbnail_tests PRIVATE mapthumbnail)
    target_include_directories(mapthumbnail_tests PRIVATE src)
    add_test(NAME mapthumbnail_tests COMMAND mapthumbnail_tests)
endif()

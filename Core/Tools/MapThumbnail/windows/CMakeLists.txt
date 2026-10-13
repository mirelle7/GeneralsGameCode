# Windows Explorer thumbnail handler. Explorer only loads handlers of its own architecture,
# which is 64-bit on every current Windows, so a 32-bit build of this DLL is of little use.

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(WARNING "MapThumbnailProvider is being built 32-bit; 64-bit Windows Explorer will not load it.")
endif()

add_library(MapThumbnailProvider SHARED
    ThumbnailProvider.cpp
    MapThumbnailProvider.def
)
target_link_libraries(MapThumbnailProvider PRIVATE mapthumbnail shlwapi)
set_target_properties(MapThumbnailProvider PROPERTIES PREFIX "")
if(MINGW)
    target_link_options(MapThumbnailProvider PRIVATE -static)
endif()

# Double-click switch that registers or unregisters the DLL next to it.
add_executable(MapThumbnailToggle WIN32 Toggle.c)
target_link_libraries(MapThumbnailToggle PRIVATE advapi32 user32)
if(MSVC)
    # No C runtime keeps the exe at a few KB. Debug keeps it for the /RTC checks.
    target_compile_options(MapThumbnailToggle PRIVATE /GS-)
    target_link_options(MapThumbnailToggle PRIVATE /ENTRY:ToggleMain $<$<NOT:$<CONFIG:Debug>>:/NODEFAULTLIB>)
    target_link_libraries(MapThumbnailToggle PRIVATE kernel32)
endif()

install(TARGETS MapThumbnailProvider MapThumbnailToggle RUNTIME DESTINATION bin)

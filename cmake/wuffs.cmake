# Fetch the Wuffs library for decoding Targa files. Wuffs is memory-safe by construction,
# which matters because Targa files can come from other players with transferred maps.
# Wuffs needs a C99 compiler, so VC6 builds decode with stb_image instead.

set(WUFFS_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/wuffs)

file(DOWNLOAD
    https://raw.githubusercontent.com/google/wuffs/ba25980637db55730c62b466f188fae33b9289f6/release/c/wuffs-v0.4.c
    ${WUFFS_DIR}/wuffs-v0.4.c
    EXPECTED_HASH SHA256=1f8039ef82911604c063f6ac2ed57254bdb17d742aebdeae06356530d4a0fde7
)

file(CONFIGURE OUTPUT ${WUFFS_DIR}/wuffs.c CONTENT "#define WUFFS_IMPLEMENTATION\n#include \"wuffs-v0.4.c\"\n")

add_library(wuffs STATIC)

target_sources(wuffs PRIVATE ${WUFFS_DIR}/wuffs.c)

# Users include wuffs-v0.4.c without WUFFS_IMPLEMENTATION to get the declarations.
target_include_directories(wuffs PUBLIC ${WUFFS_DIR})

target_compile_definitions(wuffs PUBLIC
    WUFFS_CONFIG__MODULES
    WUFFS_CONFIG__MODULE__BASE
    WUFFS_CONFIG__MODULE__TARGA
)

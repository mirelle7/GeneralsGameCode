set(GM_ENABLE_TESTS OFF CACHE BOOL "Disable GameMath tests" FORCE)

FetchContent_Declare(
    gamemath
    GIT_REPOSITORY https://github.com/TheSuperHackers/GameMath.git
    GIT_TAG        59f7ccd494f7e7c916a784ac26ef266f9f09d78d
)

FetchContent_MakeAvailable(gamemath)

# TheSuperHackers @build githubawn 29/07/2026 WebAssembly has no floating-point
# exception flags, so Emscripten's <fenv.h> defines FE_ALL_EXCEPT as 0 and omits the
# individual FE_* flags entirely. GameMath's musl-derived rounding helpers name those
# flags when they raise inexact/invalid, so define them as the empty set: feraiseexcept
# and fetestexcept are already no-ops there. No effect on the computed results, which
# is what determinism depends on.
if(EMSCRIPTEN AND TARGET gamemath)
    target_compile_definitions(gamemath PRIVATE
        FE_INVALID=0
        FE_DIVBYZERO=0
        FE_OVERFLOW=0
        FE_UNDERFLOW=0
        FE_INEXACT=0
    )
endif()

# TheSuperHackers @build Nintendo Switch's newlib defines __ISO_C_VISIBLE (glibc and bionic
# leave it unset), which makes gmath.h take its C99 branch: it needs __double_t/__float_t
# that newlib lacks and defines gm_isfinite()/gm_isnormal() as macros that then clash with
# their own declarations. gmath.h is included by the game code too, so skip that branch in
# the fetched header. Idempotent.
if(CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch" AND TARGET gamemath)
    set(_gmath_h "${gamemath_SOURCE_DIR}/include/gmath.h")
    file(READ "${_gmath_h}" _gmath_content)
    string(REPLACE "#if __ISO_C_VISIBLE >= 1999" "#if __ISO_C_VISIBLE >= 1999 && !defined(__NX__)" _gmath_new "${_gmath_content}")
    if(NOT _gmath_new STREQUAL _gmath_content)
        file(WRITE "${_gmath_h}" "${_gmath_new}")
    endif()
    # newlib's math.h defines signgam as a macro; GameMath defines the variable itself.
    target_compile_options(gamemath PRIVATE -include ${CMAKE_CURRENT_LIST_DIR}/switch-gamemath-shim.h)
endif()

# Ensure GameMath includes are available to ALL targets
# to prevent one-definition-rule violations and ensure USE_DETERMINISTIC_MATH activates consistently.
include_directories(${gamemath_SOURCE_DIR}/include)

set(GS_OPENSSL FALSE)
set(GAMESPY_SERVER_NAME "server.cnc-online.net")

FetchContent_Declare(
    gamespy
    GIT_REPOSITORY https://github.com/TheSuperHackers/GamespySDK.git
    GIT_TAG        07e3d15c500415abc281efb74322ab6d9c857eb8
)

FetchContent_MakeAvailable(gamespy)

# Android's C library has no pthread_cancel. GameSpy only uses it when it destroys a
# thread, so detach the thread there instead: it finishes on its own and is cleaned up.
if(ANDROID AND TARGET gscommon)
    target_compile_definitions(gscommon PRIVATE pthread_cancel=pthread_detach)
endif()

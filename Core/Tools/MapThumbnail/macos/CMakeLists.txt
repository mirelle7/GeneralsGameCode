# Quick Look thumbnail extension. macOS only loads extensions embedded in an app, so this builds
# GeneralsMapThumbnails.app with MapThumbnailExtension.appex inside. App extensions need the
# Xcode generator: cmake -S Core/Tools/MapThumbnail -B build/mapthumbnail -G Xcode

if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
    message(STATUS "Skipping the Quick Look extension: it needs the Xcode generator (-G Xcode).")
    return()
endif()

set(MAPTHUMBNAIL_BUNDLE_ID "com.thesuperhackers.GeneralsMapThumbnails" CACHE STRING "Bundle identifier of the map thumbnail app")
set(MAPTHUMBNAIL_SIGN_IDENTITY "-" CACHE STRING "Code signing identity; '-' signs ad hoc for local use")

set(MAPTHUMBNAIL_XCODE_COMMON
    XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET 10.15
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${MAPTHUMBNAIL_SIGN_IDENTITY}"
    XCODE_ATTRIBUTE_CODE_SIGN_STYLE Manual
    XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
)

add_executable(MapThumbnailExtension MACOSX_BUNDLE ThumbnailProvider.mm)
target_link_libraries(MapThumbnailExtension PRIVATE
    mapthumbnail
    "-framework Foundation"
    "-framework CoreGraphics"
    "-framework QuickLookThumbnailing"
)
target_link_options(MapThumbnailExtension PRIVATE -e _NSExtensionMain)
set_target_properties(MapThumbnailExtension PROPERTIES
    BUNDLE_EXTENSION appex
    XCODE_PRODUCT_TYPE com.apple.product-type.app-extension
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Extension-Info.plist
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER ${MAPTHUMBNAIL_BUNDLE_ID}.ThumbnailExtension
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS ${CMAKE_CURRENT_SOURCE_DIR}/Extension.entitlements
    XCODE_ATTRIBUTE_APPLICATION_EXTENSION_API_ONLY YES
    ${MAPTHUMBNAIL_XCODE_COMMON}
)

add_executable(GeneralsMapThumbnails MACOSX_BUNDLE HostApp.mm)
target_link_libraries(GeneralsMapThumbnails PRIVATE mapthumbnail "-framework Cocoa")
set_target_properties(GeneralsMapThumbnails PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/HostApp-Info.plist
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER ${MAPTHUMBNAIL_BUNDLE_ID}
    XCODE_EMBED_APP_EXTENSIONS MapThumbnailExtension
    XCODE_EMBED_APP_EXTENSIONS_CODE_SIGN_ON_COPY ON
    XCODE_EMBED_APP_EXTENSIONS_REMOVE_HEADERS_ON_COPY ON
    ${MAPTHUMBNAIL_XCODE_COMMON}
)
add_dependencies(GeneralsMapThumbnails MapThumbnailExtension)

install(TARGETS GeneralsMapThumbnails BUNDLE DESTINATION Applications)

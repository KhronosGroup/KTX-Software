
add_executable( es3loadtests
    ${EXE_FLAG}
    $<TARGET_OBJECTS:appfwSDL>
    $<TARGET_OBJECTS:GLAppSDL>
    $<TARGET_OBJECTS:objUtil>
    glloadtests/gles1/ES1LoadTests.cpp
    glloadtests/gles1/DrawTexture.cpp
    glloadtests/gles1/DrawTexture.h
    glloadtests/gles1/TexturedCube.cpp
    glloadtests/gles1/TexturedCube.h
    ${LOAD_TEST_COMMON_RESOURCE_FILES}
)

target_include_directories(
    es3loadtests
PRIVATE
    $<TARGET_PROPERTY:appfwSDL,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:GLAppSDL,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:objUtil,INTERFACE_INCLUDE_DIRECTORIES>
)

if(OPENGL_FOUND)
    target_include_directories(
        es3loadtests
    PRIVATE
        ${OPENGL_INCLUDE_DIR}
    )
endif()

target_link_libraries(
    es3loadtests
    ktx
)

if(OPENGL_FOUND AND NOT EMSCRIPTEN)
    target_link_libraries(
        es3loadtests
        ${OPENGL_LIBRARIES}
    )
endif()

if(SDL2_FOUND)
    target_link_libraries(
        es3loadtests
        ${SDL2_LIBRARIES}
    )
endif()

if(ZLIB_FOUND)
    target_link_libraries( es3loadtests ZLIB::ZLIB )
endif()

if(APPLE)
    if(IOS)
        set( INFO_PLIST "${PROJECT_SOURCE_DIR}/tests/loadtests/glloadtests/resources/ios/Info.plist" )
        target_sources(
            es3loadtests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/icons/ios/CommonIcons.xcassets
            glloadtests/resources/ios/LaunchImages.xcassets
            glloadtests/resources/ios/LaunchScreen.storyboard
        )

        target_link_libraries(
            es3loadtests
            ${AudioToolbox_LIBRARY}
            ${AVFoundation_LIBRARY}
            ${CoreAudio_LIBRARY}
            ${CoreBluetooth_LIBRARY}
            ${CoreGraphics_LIBRARY}
            ${CoreMotion_LIBRARY}
            ${Foundation_LIBRARY}
            ${GameController_LIBRARY}
            ${Metal_LIBRARY}
            ${OpenGLES_LIBRARY}
            ${QuartzCore_LIBRARY}
            ${UIKit_LIBRARY}
        )
    else()
        set( INFO_PLIST "${PROJECT_SOURCE_DIR}/tests/loadtests/glloadtests/resources/mac/Info.plist" )
    endif()
elseif(EMSCRIPTEN)
    set_target_properties(
        es3loadtests
    PROPERTIES
        COMPILE_FLAGS "-Wpedantic -s DISABLE_EXCEPTION_CATCHING=0 -s USE_SDL=2 -s USE_WEBGL2=1 -O0 -g"
        # LINK_FLAGS "--source-map-base ./ --preload-file testimages --exclude-file testimages/genref --exclude-file testimages/*.pgm --exclude-file testimages/*.ppm --exclude-file testimages/*.pam --exclude-file testimages/*.pspimage -s ALLOW_MEMORY_GROWTH=1 -s DISABLE_EXCEPTION_CATCHING=0 -s USE_SDL=2 -s USE_WEBGL2=1 -g4"
        LINK_FLAGS "-s ALLOW_MEMORY_GROWTH=1 -s DISABLE_EXCEPTION_CATCHING=0 -s USE_SDL=2 -s USE_WEBGL2=1 -g"
    )
elseif(WIN32)
    target_sources(
        es3loadtests
    PRIVATE
        glloadtests/resources/win/glloadtests.rc
        glloadtests/resources/win/resource.h
    )
    target_link_libraries(
        es3loadtests
        "${CMAKE_SOURCE_DIR}/other_lib/win/Release-x64/glew32.lib"
    )
    ensure_runtime_dependencies_windows(es3loadtests)
endif()

target_link_libraries( es3loadtests ${LOAD_TEST_COMMON_LIBS} )

target_compile_definitions(
    es3loadtests
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
    "GL_CONTEXT_PROFILE=SDL_GL_CONTEXT_PROFILE_ES"
    GL_CONTEXT_MAJOR_VERSION=3
    GL_CONTEXT_MINOR_VERSION=0
)

if(APPLE)
    set(PRODUCT_NAME "es3loadtests")
    set(EXECUTABLE_NAME ${PRODUCT_NAME})
    set(PRODUCT_BUNDLE_IDENTIFIER "org.khronos.ktx.${PRODUCT_NAME}")
    configure_file( ${INFO_PLIST} Info.plist )
    set_target_properties( es3loadtests PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
        MACOSX_BUNDLE_ICON_FILE "ktx_app.icns"
        # Because libassimp is built with bitcode disabled. It's not important unless
        # submitting to the App Store and currently bitcode is optional.
        XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
        XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES"
    )
endif()
if(KTX_ICON)
    set_target_properties( es3loadtests PROPERTIES RESOURCE ${KTX_ICON} )
endif()


if(EMSCRIPTEN)
    set_target_properties(es3loadtests PROPERTIES SUFFIX ".html")
endif()

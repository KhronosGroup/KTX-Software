# Copyright 2020 Andreas Atteneder
# SPDX-License-Identifier: Apache-2.0

set(OPENGL_ES_EMULATOR "" CACHE PATH "Path to OpenGL ES emulation libraries")

function( create_gl_target target sources KTX_GL_CONTEXT_PROFILE KTX_GL_CONTEXT_MAJOR_VERSION KTX_GL_CONTEXT_MINOR_VERSION EMULATE_GLES )

    add_executable( ${target}
        ${EXE_FLAG}
        ${sources}
        ${LOAD_TEST_COMMON_RESOURCE_FILES}
    )

    target_include_directories(
        ${target}
    PRIVATE
        $<TARGET_PROPERTY:appfwSDL,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:GLAppSDL,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:ktx,INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:objUtil,INTERFACE_INCLUDE_DIRECTORIES>
    )

    if(OPENGL_FOUND)
        target_include_directories(
            ${target}
        PRIVATE
            ${OPENGL_INCLUDE_DIR}
        )
    endif()

    target_link_libraries(
        ${target}
        objUtil
        GLAppSDL
        appfwSDL
        ktx
        ${KTX_ZLIB_LIBRARIES}
    )

    if(OPENGL_FOUND AND NOT EMSCRIPTEN AND NOT EMULATE_GLES)
        target_link_libraries(
            ${target}
            ${OPENGL_LIBRARIES}
        )
    endif()

    if(SDL2_FOUND)
        target_link_libraries(
            ${target}
            ${SDL2_LIBRARIES}
        )
    endif()

    if(APPLE)
        if(IOS)
            set( INFO_PLIST "${PROJECT_SOURCE_DIR}/tests/loadtests/glloadtests/resources/ios/Info.plist" )
            set( KTX_RESOURCES
                ${PROJECT_SOURCE_DIR}/icons/ios/CommonIcons.xcassets
                glloadtests/resources/ios/LaunchImages.xcassets
                glloadtests/resources/ios/LaunchScreen.storyboard
            )
            target_sources( ${target} PRIVATE ${KTX_RESOURCES} )
            target_link_libraries(
                ${target}
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
            set( KTX_RESOURCES ${KTX_ICON} )
            set( INFO_PLIST "${PROJECT_SOURCE_DIR}/tests/loadtests/glloadtests/resources/mac/Info.plist" )
        endif()
    elseif(EMSCRIPTEN)
        target_link_options(
            ${target}
        PRIVATE
            "SHELL:--source-map-base ./"
            "SHELL:--preload-file '${PROJECT_SOURCE_DIR}/tests/testimages@testimages'"
            "SHELL:--exclude-file '${PROJECT_SOURCE_DIR}/tests/testimages/genref'"
            "SHELL:--exclude-file '${PROJECT_SOURCE_DIR}/tests/testimages/genktx2'"
            "SHELL:--exclude-file '${PROJECT_SOURCE_DIR}/tests/testimages/cubemap*'"
            "SHELL:-s ALLOW_MEMORY_GROWTH=1"
            "SHELL:-s DISABLE_EXCEPTION_CATCHING=0"
            "SHELL:-s USE_SDL=2"
            "SHELL:-s USE_WEBGL2=1"
        )
    elseif(WIN32)
        target_sources(
            ${target}
        PRIVATE
            glloadtests/resources/win/glloadtests.rc
            glloadtests/resources/win/resource.h
        )
        if(EMULATE_GLES)
            if (KTX_GL_CONTEXT_MAJOR_VERSION EQUAL 1)
                target_link_libraries(
                    ${target}
                    "${OPENGL_ES_EMULATOR}/libGLES_CM.lib"
                    "${OPENGL_ES_EMULATOR}/libEGL.lib"
                )
             else()
                target_link_libraries(
                    ${target}
                    "${OPENGL_ES_EMULATOR}/libGLESv2.lib"
                    "${OPENGL_ES_EMULATOR}/libEGL.lib"
                )
             endif()
        else()
            target_link_libraries(
                ${target}
                "${CMAKE_SOURCE_DIR}/other_lib/win/Release-x64/glew32.lib"
            )
        endif()
        ensure_runtime_dependencies_windows(${target})
    endif()

    target_link_libraries( ${target} ${LOAD_TEST_COMMON_LIBS} )

    if(NOT EMULATE_GLES OR KTX_GL_CONTEXT_MAJOR_VERSION GREATER 1)
        target_compile_definitions(
            ${target}
        PRIVATE
            $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
            GL_CONTEXT_PROFILE=${KTX_GL_CONTEXT_PROFILE}
            GL_CONTEXT_MAJOR_VERSION=${KTX_GL_CONTEXT_MAJOR_VERSION}
            GL_CONTEXT_MINOR_VERSION=${KTX_GL_CONTEXT_MINOR_VERSION}
        )
     else()
        target_compile_definitions(
            ${target}
        PRIVATE
            $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
        )
     endif()

    if(APPLE)
        set(PRODUCT_NAME "${target}")
        set(EXECUTABLE_NAME ${PRODUCT_NAME})
        # How amazingly irritating. We have to set both of these to the same value.
        # The first must be set otherwise the app cannot be installed on iOS. The second
        # has to be set to avoid an Xcode warning.
        set(PRODUCT_BUNDLE_IDENTIFIER "org.khronos.ktx.${PRODUCT_NAME}")
        set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.khronos.ktx.${PRODUCT_NAME}")
        configure_file( ${INFO_PLIST} ${target}/Info.plist )
        set_target_properties( ${target} PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/${target}/Info.plist"
            MACOSX_BUNDLE_ICON_FILE "ktx_app.icns"
            # Because libassimp is built with bitcode disabled. It's not important unless
            # submitting to the App Store and currently bitcode is optional.
            XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
            XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES"
            XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "ktx_app"
            XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" # iPhone and iPad
        )
        set_xcode_code_sign(${target})
        unset(PRODUCT_NAME)
        unset(EXECUTABLE_NAME)
        unset(PRODUCT_BUNDLE_IDENTIFIER)
        if(KTX_RESOURCES)
            set_target_properties( ${target} PROPERTIES RESOURCE "${KTX_RESOURCES}" )
        endif()

        if(NOT IOS)
            set_target_properties( ${target} PROPERTIES
                INSTALL_RPATH "@executable_path/../Frameworks"
            )
            add_custom_command( TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:ktx> "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Frameworks/$<TARGET_FILE_NAME:ktx>"
                COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/other_lib/mac/$<CONFIG>/libSDL2.dylib" "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Frameworks/libSDL2.dylib"
                COMMENT "Copy libraries/frameworks to build destination"
            )
            install(TARGETS ktx
                LIBRARY
                    DESTINATION "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Frameworks"
                    COMPONENT GlLoadTestApps
                PUBLIC_HEADER
                    DESTINATION "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Headers"
            )
            install(TARGETS ${target}
                BUNDLE
                    DESTINATION .
                    COMPONENT GlLoadTestApps
                RESOURCE
                    DESTINATION Resources
                    COMPONENT GlLoadTestApps
            )
        endif()

    elseif(EMSCRIPTEN)
        set_target_properties(${target} PROPERTIES SUFFIX ".html")
    endif()
endfunction( create_gl_target target )


set( ES1_SOURCES
    glloadtests/gles1/ES1LoadTests.cpp
    glloadtests/gles1/DrawTexture.cpp
    glloadtests/gles1/DrawTexture.h
    glloadtests/gles1/TexturedCube.cpp
    glloadtests/gles1/TexturedCube.h
)

set( GL3_SOURCES
    glloadtests/shader-based/BasisuTest.cpp
    glloadtests/shader-based/BasisuTest.h
    glloadtests/shader-based/DrawTexture.cpp
    glloadtests/shader-based/DrawTexture.h
    glloadtests/shader-based/GL3LoadTests.cpp
    glloadtests/shader-based/GL3LoadTestSample.cpp
    glloadtests/shader-based/GL3LoadTestSample.h
    glloadtests/shader-based/mygl.h
    glloadtests/shader-based/shaders.cpp
    glloadtests/shader-based/TextureArray.cpp
    glloadtests/shader-based/TextureArray.h
    glloadtests/shader-based/TextureCubemap.cpp
    glloadtests/shader-based/TextureCubemap.h
    glloadtests/shader-based/TexturedCube.cpp
    glloadtests/shader-based/TexturedCube.h
    glloadtests/utils/GLMeshLoader.hpp
    glloadtests/utils/GLTextureTranscoder.hpp
)


if(WIN32)
    if(NOT OPENGL_ES_EMULATOR)
        message("OPENGL_ES_EMULATOR not set. Will not build OpenGL ES load tests applications.")
    else()
        set(EMULATE_GLES ON)
    endif()
endif()

if(IOS OR EMULATE_GLES)
    # OpenGL ES 1.0
    create_gl_target( es1loadtests "${ES1_SOURCES}" SDL_GL_CONTEXT_PROFILE_ES 1 0 ON )
    if(IOS)
        set_xcode_code_sign(es1loadtests)
    endif()
endif()

if(IOS OR EMSCRIPTEN OR EMULATE_GLES)
    # OpenGL ES 3.0
    create_gl_target( es3loadtests "${GL3_SOURCES}" SDL_GL_CONTEXT_PROFILE_ES 3 0 ON )
    if(IOS)
        set_xcode_code_sign(es3loadtests)
    endif()
endif()

if( (APPLE AND NOT IOS) OR LINUX OR WIN32 )
    # OpenGL 3.3
    create_gl_target( gl3loadtests "${GL3_SOURCES}" SDL_GL_CONTEXT_PROFILE_CORE 3 3 OFF )
endif()

# Copyright 2020 Andreas Atteneder
# SPDX-License-Identifier: Apache-2.0

set(OPENGL_ES_EMULATOR "" CACHE PATH "Path to OpenGL ES emulation libraries")

function( create_gl_target target sources resources KTX_GL_CONTEXT_PROFILE KTX_GL_CONTEXT_MAJOR_VERSION KTX_GL_CONTEXT_MINOR_VERSION EMULATE_GLES)

    add_executable( ${target}
        ${EXE_FLAG}
        ${sources}
        ${resources}
        glloadtests.cmake
    )

    set_code_sign(${target})

    target_include_directories(
        ${target}
    PRIVATE
        $<TARGET_PROPERTY:appfwSDL,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:GLAppSDL,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:ktx,INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:objUtil,INTERFACE_INCLUDE_DIRECTORIES>
    )

    set_target_properties(${target} PROPERTIES
        CXX_VISIBILITY_PRESET ${STATIC_APP_LIB_SYMBOL_VISIBILITY}
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
            # Don't add these to ${resources}. If they're flagged as resources
            # the resource installer in `install(TARGETS` will be confused by
            # xcassets being directories.
            target_sources( ${target}
                PRIVATE
                    ${PROJECT_SOURCE_DIR}/icons/ios/CommonIcons.xcassets
                    glloadtests/resources/ios/LaunchImages.xcassets
                    glloadtests/resources/ios/LaunchScreen.storyboard
            )
            target_link_libraries(
                ${target}
                ${AudioToolbox_LIBRARY}
                ${AVFoundation_LIBRARY}
                ${CoreAudio_LIBRARY}
                ${CoreBluetooth_LIBRARY}
                ${CoreGraphics_LIBRARY}
                ${CoreMotion_LIBRARY}
                ${CoreHaptics_LIBRARY}
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

    set_target_properties( ${target} PROPERTIES RESOURCE "${resources}" )
      
     if(APPLE)
        set(PRODUCT_NAME "${target}")
        set(EXECUTABLE_NAME ${PRODUCT_NAME})
        # How amazingly irritating. We have to set both of these to the same
        # value. The first must be set otherwise the app cannot be installed
        # on iOS. The second has to be set to avoid an Xcode warning.
        set(PRODUCT_BUNDLE_IDENTIFIER "org.khronos.ktx.${PRODUCT_NAME}")
        set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.khronos.ktx.${PRODUCT_NAME}")
        configure_file( ${INFO_PLIST} ${target}/Info.plist )
        set_target_properties( ${target} PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/${target}/Info.plist"
            MACOSX_BUNDLE_ICON_FILE "ktx_app.icns"
            # Because libassimp is built with bitcode disabled. It's not
            # important unless submitting to the App Store and currently
            # bitcode is optional.
            XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
            XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES"
            XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "ktx_app"
            XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" # iPhone and iPad
        )
        unset(PRODUCT_NAME)
        unset(EXECUTABLE_NAME)
        unset(PRODUCT_BUNDLE_IDENTIFIER)

#        install(DIRECTORY
#            ${CMAKE_CURRENT_SOURCE_DIR}/common/models
#            DESTINATION "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Resource"
#            COMPONENT GlLoadTestApps
#        )
#        install(FILES
#            ${TEST_IMAGES}
#            DESTINATION "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Resource/testimages"
#            COMPONENT GlLoadTestApps
#        )
#        install(TARGETS loadtest_models
#            RESOURCE
#                DESTINATION "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Resources/models"
#                COMPONENT GlloadTestApps
#        )

        # The generated project code for building an Apple bundle automatically
        # copies the executable and all files with the RESOURCE property to the
        # bundle adjusting for the difference in bundle layout between iOS &
        # macOS.

        if(NOT IOS)
            set_target_properties( ${target} PROPERTIES
                INSTALL_RPATH "@executable_path/../Frameworks"
            )
            
            add_custom_command( TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:ktx> "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Frameworks/$<TARGET_FILE_NAME:ktx>"
                COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE_NAME:ktx> "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Frameworks/$<TARGET_SONAME_FILE_NAME:ktx>"
                COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/other_lib/mac/$<CONFIG>/libSDL2.dylib" "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Frameworks/libSDL2.dylib"
                COMMENT "Copy libraries & frameworks to build destination"
            )

            # Specify destination for cmake --install.
            install(TARGETS ${target}
                BUNDLE
                    DESTINATION /Applications
                    COMPONENT GlLoadTestApps
            )

        endif()

    else()
        if(EMSCRIPTEN)
            set_target_properties(${target} PROPERTIES SUFFIX ".html")
        endif()

        # This is for other platforms.
        install(TARGETS ${target}
            RESOURCE
                DESTINATION Resources
                COMPONENT GlLoadTestApps
        )

        if(NOT ${target} STREQUAL "es1loadtests")
            add_custom_command( TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                  ${CMAKE_CURRENT_SOURCE_DIR}/common/models
                  $<TARGET_FILE_DIR:${target}>/models
            )
        endif()
        add_custom_command( TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
              $<TARGET_FILE_DIR:${target}>/testimages
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
              ${test_images}
              $<TARGET_FILE_DIR:${target}>/testimages
        )
        # TODO: Figure out where to install when we include
        # these test commands in an installer.
        #install(FILES ${MODELS}
        #    DESTINATION $<TARGET_FILE_DIR:${target}>/models
        #)
        #install(FILES ${TEST_IMAGES}
        #    DESTINATION $<TARGET_FILE_DIR:${target}>testimages
        #)
    endif()
endfunction( create_gl_target target )


set( ES1_TEST_IMAGES
    hi_mark.ktx
    luminance-reference-metadata.ktx
    orient-up-metadata.ktx
    orient-down-metadata.ktx
    etc1.ktx
    etc2-rgb.ktx
    etc2-rgba1.ktx
    etc2-rgba8.ktx
    rgba-reference.ktx
    rgb-reference.ktx
    rgb-amg-reference.ktx
    rgb-mipmap-reference.ktx
    hi_mark_sq.ktx
)
list( TRANSFORM ES1_TEST_IMAGES
    PREPEND "${PROJECT_SOURCE_DIR}/tests/testimages/"
)
set( ES1_RESOURCE_FILES ${KTX_ICON} ${ES1_TEST_IMAGES} )

set( ES1_SOURCES
    glloadtests/gles1/ES1LoadTests.cpp
    glloadtests/gles1/DrawTexture.cpp
    glloadtests/gles1/DrawTexture.h
    glloadtests/gles1/TexturedCube.cpp
    glloadtests/gles1/TexturedCube.h
)

set( GL3_TEST_IMAGES
    etc1s_Iron_Bars_001_normal.ktx2
    uastc_Iron_Bars_001_normal.ktx2
    color_grid_uastc_zstd.ktx2
    color_grid_zstd.ktx2
    color_grid_uastc.ktx2
    color_grid_basis.ktx2
    kodim17_basis.ktx2
    kodim17_basis.ktx2
    FlightHelmet_baseColor_basis.ktx2
    rgba-reference-u.ktx2
    rgba-reference-u.ktx2
    rgba-reference-u.ktx2
    cubemap_goldengate_uastc_rdo4_zstd5_rd.ktx2
    cubemap_yokohama_basis_rd.ktx2
    orient-down-metadata-u.ktx2
    orient-down-metadata-u.ktx2
    texturearray_bc3_unorm.ktx2
    texturearray_astc_8x8_unorm.ktx2
    texturearray_etc2_unorm.ktx2
    3dtex_7_reference_u.ktx2
    rgb-mipmap-reference-u.ktx2
    hi_mark.ktx
    orient-up-metadata.ktx
    orient-down-metadata.ktx
    not4_rgb888_srgb.ktx
    etc1.ktx
    etc2-rgb.ktx
    etc2-rgba1.ktx
    etc2-rgba8.ktx
    etc2-sRGB.ktx
    etc2-sRGBa1.ktx
    etc2-sRGBa8.ktx
    rgba-reference.ktx
    rgb-reference.ktx
    conftestimage_R11_EAC.ktx
    conftestimage_SIGNED_R11_EAC.ktx
    conftestimage_RG11_EAC.ktx
    conftestimage_SIGNED_RG11_EAC.ktx
    texturearray_bc3_unorm.ktx
    texturearray_astc_8x8_unorm.ktx
    texturearray_etc2_unorm.ktx
    rgb-amg-reference.ktx
    rgb-mipmap-reference.ktx
    hi_mark_sq.ktx
)
list( TRANSFORM GL3_TEST_IMAGES
    PREPEND "${PROJECT_SOURCE_DIR}/tests/testimages/"
)
set( GL3_RESOURCE_FILES ${LOAD_TEST_COMMON_RESOURCE_FILES} ${GL3_TEST_IMAGES} )

set( GL3_SOURCES
    common/TranscodeTargetStrToFmt.cpp
    common/TranscodeTargetStrToFmt.h
    common/disable_glm_warnings.h
    common/reenable_warnings.h
    glloadtests/shader-based/DrawTexture.cpp
    glloadtests/shader-based/DrawTexture.h
    glloadtests/shader-based/EncodeTexture.cpp
    glloadtests/shader-based/EncodeTexture.h
    glloadtests/shader-based/GL3LoadTests.cpp
    glloadtests/shader-based/GL3LoadTestSample.cpp
    glloadtests/shader-based/GL3LoadTestSample.h
    glloadtests/shader-based/InstancedSampleBase.cpp
    glloadtests/shader-based/InstancedSampleBase.h
    glloadtests/shader-based/mygl.h
    glloadtests/shader-based/shaders.cpp
    glloadtests/shader-based/Texture3d.cpp
    glloadtests/shader-based/Texture3d.h
    glloadtests/shader-based/TextureArray.cpp
    glloadtests/shader-based/TextureArray.h
    glloadtests/shader-based/TextureMipmap.cpp
    glloadtests/shader-based/TextureMipmap.h
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
    create_gl_target( es1loadtests "${ES1_SOURCES}" "${ES1_RESOURCE_FILES}" SDL_GL_CONTEXT_PROFILE_ES 1 0 ON)
endif()

if(IOS OR EMSCRIPTEN OR EMULATE_GLES)
    # OpenGL ES 3.0
    create_gl_target( es3loadtests "${GL3_SOURCES}" "${GL3_RESOURCE_FILES}" SDL_GL_CONTEXT_PROFILE_ES 3 0 ON YES)
endif()

if( (APPLE AND NOT IOS) OR LINUX OR WIN32 )
    # OpenGL 3.3
    create_gl_target( gl3loadtests "${GL3_SOURCES}" "${GL3_RESOURCE_FILES}" SDL_GL_CONTEXT_PROFILE_CORE 3 3 OFF YES)
endif()

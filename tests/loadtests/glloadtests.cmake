# Copyright 2020-2026 Andreas Atteneder, Mark Callow
# SPDX-License-Identifier: Apache-2.0

set(OPENGL_ES_EMULATOR "" CACHE PATH "Path to OpenGL ES emulation libraries")

if(NOT APPLE_LOCKED_OS)
    find_package(OpenGL REQUIRED)
endif()
if(WIN32)
    find_package(GLEW REQUIRED)
endif()

function( create_gl_target target version sources common_resources ktx_image_sources
          KTX_GL_CONTEXT_PROFILE
          KTX_GL_CONTEXT_MAJOR_VERSION KTX_GL_CONTEXT_MINOR_VERSION
          EMULATE_GLES)

    set( resources
       ${common_resources};${ktx_image_sources};
    )

    add_executable( ${target}
        ${EXE_FLAG}
        glloadtests.cmake
        ${sources}
        ${resources}
    )

    set_code_sign(${target})

    # Nota Bene.
    #
    # 1. With the Visual Studio generator, at least, The SDL and GLEW
    #    includes coming from GLAppSDL are being converted to system
    #    includes. To see them in VS, view the whole command line in
    #    the compile section of the project properties and look at the
    #    Additional Options pane at the bottom.
    # 2. GL_APP_SDL's INTERFACE_INCLUDE_DIRECTORIES includes the SYSTEM
    #    include from appfwSDL.
    #
    # I do not understand the reasons for either of these.
    target_include_directories(
        ${target}
    PRIVATE
        $<TARGET_PROPERTY:GLAppSDL,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:objUtil,INTERFACE_INCLUDE_DIRECTORIES>
    )

    set_target_properties(${target} PROPERTIES
        CXX_VISIBILITY_PRESET ${STATIC_APP_LIB_SYMBOL_VISIBILITY}
    )


    target_link_libraries(
        ${target}
        objUtil
        GLAppSDL
        appfwSDL
        ktx
    )

    if(NOT EMSCRIPTEN AND NOT EMULATE_GLES)
        target_link_libraries(
            ${target}
            ${OPENGL_LIBRARIES}
        )
    endif()

    if(APPLE)
        if(IOS)
            # This is location where CMake puts the configured Info.plist.
            # I have not found a CMake variable for this.
            set( launch_screen ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${target}.dir/LaunchScreen.storyboard )
            configure_file( glloadtests/resources/ios/LaunchScreen.storyboard.in
                 ${launch_screen}
            )
            set( INFO_PLIST_IN "${PROJECT_SOURCE_DIR}/tests/loadtests/glloadtests/resources/ios/Info.plist.in" )
            set( icon_launch_assets
                ${PROJECT_SOURCE_DIR}/icons/ios/CommonIcons.xcassets
                glloadtests/resources/ios/LaunchImages.xcassets
                ${launch_screen}
            )
            target_sources( ${target}
                PRIVATE
                ${PROJECT_SOURCE_DIR}/icons/ios/CommonIcons.xcassets
                glloadtests/resources/ios/LaunchImages.xcassets
                glloadtests/resources/ios/LaunchScreen.storyboard.in
            )
            # Add to resources so they'll be copied to the bundle.
            list( APPEND resources ${icon_launch_assets} )
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
            set( INFO_PLIST_IN "${PROJECT_SOURCE_DIR}/tests/loadtests/glloadtests/resources/mac/Info.plist.in" )
        endif()
    elseif(EMSCRIPTEN)
        # Beware of de-duplication in list expansion for commands and options.
        # SHELL: prevents it but if they are separate items in the list they
        # be de-duplicated.
        list( TRANSFORM ktx_image_sources REPLACE
            "(${PROJECT_SOURCE_DIR}/tests/resources/(ktx|ktx2)/([a-zA-Z0-9_].*$))"
            "SHELL:--preload-file \\1@\\3"
            OUTPUT_VARIABLE preloads
        )
        target_link_options(
            ${target}
        PRIVATE
            "SHELL:--source-map-base ./"
            ${preloads}
            "SHELL:--exclude-file '${PROJECT_SOURCE_DIR}/tests/resources/ktx2/cubemap*'"
            "SHELL:--use-port=sdl3"
            "SHELL:-s ALLOW_MEMORY_GROWTH=1"
            "SHELL:-s DISABLE_EXCEPTION_CATCHING=0"
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
                ${GLEW_LIBRARIES}
            )
        endif()
    elseif(LINUX)
        # The output file is configured at CMake config time.
        configure_file(glloadtests/resources/linux/glloadtests.desktop.in
                  ${CMAKE_CURRENT_BINARY_DIR}/${target}.desktop
        )
        target_sources(
              ${target}
          PRIVATE
              # Put the input file in sources as that is what must be edited.
              glloadtests/resources/linux/glloadtests.desktop.in
             #${CMAKE_CURRENT_BINARY_DIR}/${target}.desktop
        )
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
            $<$<PLATFORM_ID:Windows>:NOMINMAX>
        )
    else()
        target_compile_definitions(
            ${target}
        PRIVATE
            $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
            $<$<PLATFORM_ID:Windows>:NOMINMAX>
        )
    endif()

    set_target_properties( ${target} PROPERTIES RESOURCE "${resources}" )

    if(APPLE)
        set(product_name "${target}")
        set( bundle_identifier org.khronos.ktx.${product_name} )
        # This property must be set to avoid an Xcode warning.
        set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.khronos.ktx.${product_name}")
        # See important comment about MACOSX_BUNDLE_INFO_PLIST and these
        # properties in ./vkloadtests.cmake.
        set_target_properties( ${target} PROPERTIES
            MACOSX_BUNDLE_BUNDLE_NAME ${product_name}
            MACOSX_BUNDLE_EXECUTABLE_NAME ${product_name}
            MACOSX_BUNDLE_COPYRIGHT "© 2024 Khronos Group, Inc."
            MACOSX_BUNDLE_GUI_IDENTIFIER ${bundle_identifier}
            MACOSX_BUNDLE_INFO_PLIST ${INFO_PLIST_IN}
            MACOSX_BUNDLE_INFO_STRING "View KTX textures; display via OpenGL."
            MACOSX_BUNDLE_ICON_FILE ${KTX_APP_ICON}
            MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
            MACOSX_BUNDLE_SHORT_VERSION_STRING  ${PROJECT_VERSION}
            # Because libassimp is built with bitcode disabled. It's not
            # important unless submitting to the App Store and currently
            # bitcode is optional.
            XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
            XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES"
            XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME ${KTX_APP_ICON_BASENAME}
            XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" # iPhone and iPad
        )
        unset(product_name)
        unset(bundle_identifier)

        # The generated project code for building an Apple bundle automatically
        # copies the executable and all files with the RESOURCE property to the
        # bundle adjusting for the difference in bundle layout between iOS &
        # macOS.

        if(NOT IOS)
            set_target_properties( ${target} PROPERTIES
                INSTALL_RPATH "@executable_path/../Frameworks"
            )

            if(BUILD_SHARED_LIBS)
              add_custom_command( TARGET ${target} POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:ktx> "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Frameworks/$<TARGET_FILE_NAME:ktx>"
                  COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE_NAME:ktx> "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Frameworks/$<TARGET_SONAME_FILE_NAME:ktx>"
                  COMMENT "Copy KTX library to build destination"
              )
            endif()
            # Re. SDL3 & assimp: no copy required.: vcpkg libs are static or else
            # vcpkg arranges copy. Brew libs cannot be bundled.

            # Specify destination for cmake --install.
            install(TARGETS ${target}
                BUNDLE
                    DESTINATION /Applications
                    COMPONENT GlLoadTestApps
            )

        endif()

    else()
        if(WINDOWS OR LINUX)
            # These custom targets, custom commands and dependencies copy
            # the resources next to the executable for ease of use during
            # debugging and testing. They have no effect on the install target.

            # Copy the KTX images specific to the current target.
            list(TRANSFORM ktx_image_sources
                REPLACE "^[a-zA-Z0-9:/<>].*/ktx(2?)" ${BUILDTIME_RESOURCES_DIR}
                OUTPUT_VARIABLE ktx_image_copies
            )
            add_custom_command(
                OUTPUT ${ktx_image_copies}
                COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILDTIME_RESOURCES_DIR}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ktx_image_sources} ${BUILDTIME_RESOURCES_DIR}
                COMMENT "Copy ${target}'s ktx images to build destination"
                DEPENDS ${ktx_image_sources}
                VERBATIM
            )
            add_custom_target(
                copied_${target}_ktx_images
                DEPENDS ${ktx_image_copies}
            )
            add_dependencies(
                copied_${target}_ktx_images
                copied_common_ktx_images
            )
            add_dependencies( ${target}
                copied_ktx_icons
                copied_models
                copied_${target}_ktx_images
            )
        endif()

        if(EMSCRIPTEN)
            set_target_properties(${target} PROPERTIES SUFFIX ".html")
        endif()

        # See important comment and TODO:s starting at line 365
        # in ./vkloadtests.cmake regarding installation of these
        # targets. Search for "keep the resources".

        set_target_properties( ${target} PROPERTIES
            INSTALL_RPATH "\$ORIGIN;${CMAKE_INSTALL_FULL_LIBDIR}"
        )

        ######### IMPORTANT ######
        # When installing via `cmake --install` ALSO install the
        # library component. There seems no way to make a dependency.
        ##########################
#        set( destroot "${LOAD_TEST_DESTROOT}/$<TARGET_FILE_NAME:${target}>")
        # NOTE: WHEN RUNNING MANUAL INSTALLS INSTALL library COMPONENT TOO.
#        install(TARGETS ${target}
#            RUNTIME
#                DESTINATION ${destroot}/bin
#                COMPONENT GlLoadTestApps
#            LIBRARY
#                DESTINATION ${CMAKE_INSTALL_LIBDIR}
#                COMPONENT GlLoadTestApps
#            RESOURCE
#                DESTINATION ${destroot}/resources
#                COMPONENT GlLoadTestApps
#        )
#        install(TARGETS ktx
#            RUNTIME
#                DESTINATION ${destroot}/bin
#                COMPONENT GlLoadTestApps
#            LIBRARY
#                DESTINATION ${destroot}/lib
#                COMPONENT GlLoadTestApps
#        )
#        if(LINUX)
#            # Add a link from the regular bin directory to put command
#            # on PATH.
#            install(CODE "
#               EXECUTE_PROCESS(COMMAND ln -s ${destroot}/bin/$<TARGET_FILE_NAME:${target}> ${CMAKE_INSTALL_FULL_BINDIR}
#               )"
#               COMPONENT GlLoadTestApps
#            )
#            install(FILES
#                ${CMAKE_CURRENT_BINARY_DIR}/${target}.desktop
#                DESTINATION /usr/share/applications
#                COMPONENT GlLoadTestApps
#            )
#        endif(LINUX)
    endif()
endfunction( create_gl_target target )

set( es1_ktx_image_sources
    no_npot.ktx
    hi_mark.ktx
    l8_unorm_metadata.ktx
)
list( TRANSFORM es1_ktx_image_sources
    PREPEND "${PROJECT_SOURCE_DIR}/tests/resources/ktx/"
)

set( es1_sources
    glloadtests/gles1/ES1LoadTests.cpp
    glloadtests/gles1/DrawTexture.cpp
    glloadtests/gles1/DrawTexture.h
    glloadtests/gles1/TexturedCube.cpp
    glloadtests/gles1/TexturedCube.h
)

set( gl3_ktx2_image_sources
    FlightHelmet_baseColor_blze.ktx2
    r8g8b8_srgb_mip.ktx2
    r8g8b8a8_srgb.ktx2
    r8g8b8a8_srgb_3d_7.ktx2
)

set( gl3_ktx_image_sources
    conftestimage_R11_EAC.ktx
    conftestimage_SIGNED_R11_EAC.ktx
    conftestimage_RG11_EAC.ktx
    conftestimage_SIGNED_RG11_EAC.ktx
    hi_mark.ktx
)
list( TRANSFORM gl3_ktx2_image_sources
    PREPEND "${PROJECT_SOURCE_DIR}/tests/resources/ktx2/"
)
list( TRANSFORM gl3_ktx_image_sources
    PREPEND "${PROJECT_SOURCE_DIR}/tests/resources/ktx/"
)

set( gl3_ktx12_image_sources ${gl3_ktx2_image_sources} ${gl3_ktx_image_sources} )
source_group("Resources/KTX Images" FILES ${gl3_ktx12_image_sources})

set( gl3_sources
    common/TranscodeTargetStrToFmt.cpp
    common/TranscodeTargetStrToFmt.h
    common/disable_glm_warnings.h
    common/ltexceptions.h
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
    create_gl_target( es1loadtests "ES1" "${es1_sources}"
        "${KTX_ICON_SOURCES}"                                   # common_resources
        "${es1_ktx_image_sources}"                              # ktx_image_sources
        SDL_GL_CONTEXT_PROFILE_ES 1 0 ON
    )
endif()

if(IOS OR EMSCRIPTEN OR EMULATE_GLES)
    # OpenGL ES 3.0
    create_gl_target( es3loadtests "ES3" "${gl3_sources}"
        "${KTX_ICON_SOURCES};${LOAD_TEST_COMMON_MODEL_SOURCES}"             # common_resources
        "${gl3_ktx12_image_sources};${LOAD_TEST_COMMON_KTX12_IMAGE_SOURCES}" # ktx_image_sources
        SDL_GL_CONTEXT_PROFILE_ES 3 0 ON YES
    )
endif()

if( (APPLE AND NOT IOS) OR LINUX OR WIN32 )
    # OpenGL 3.3
    create_gl_target( gl3loadtests "GL3" "${gl3_sources}"
        "${KTX_ICON_SOURCES};${LOAD_TEST_COMMON_MODEL_SOURCES}"              # common_resources
        "${gl3_ktx12_image_sources};${LOAD_TEST_COMMON_KTX12_IMAGE_SOURCES}" # ktx_image_sources
        SDL_GL_CONTEXT_PROFILE_CORE 3 3 OFF YES
    )
endif()

unset( es1_sources )
unset( gl3_sources )
unset( es1_ktx_image_sources )
unset( gl3_ktx2_image_sources )
unset( gl3_ktx_image_sources )
unset( gl3_ktx12_image_sources )

# Copyright 2020 Andreas Atteneder
# SPDX-License-Identifier: Apache-2.0

# Find Vulkan package
if(IOS)
    # On iOS we link against MoltenVK.framework manually
    set( MOLTENVK_FRAMEWORK ${MOLTENVK_SDK}/iOS/framework/MoltenVK.framework )
    if( NOT IS_DIRECTORY ${MOLTENVK_FRAMEWORK})
        # Fallback: Older Vulkan SDKs have MoltenVK.framework at a
        # different sub path
        message("Could not find MoltenVK framework (at '${MOLTENVK_FRAMEWORK}')")
        set(MOLTENVK_FRAMEWORK ${MOLTENVK_SDK}/iOS/MoltenVK.framework)
    endif()
    if( NOT IS_DIRECTORY ${MOLTENVK_FRAMEWORK})
        message("Could not find MoltenVK framework (at '${MOLTENVK_FRAMEWORK}')")
        # Fallback: On newer Vulkan SDKs it's a .xcframework at the root
        # level. CMake does not support linking those directly (see
        # https://gitlab.kitware.com/cmake/cmake/-/issues/21752), so we
        # manually pick the static library file for iOS arm64 from a
        # subfolder here
        if( IS_DIRECTORY ${MOLTENVK_SDK}/MoltenVK.xcframework )
            set( MOLTENVK_FRAMEWORK ${MOLTENVK_SDK}/MoltenVK.xcframework/ios-arm64/libMoltenVK.a )
        endif()
    endif()
    if( NOT EXISTS ${MOLTENVK_FRAMEWORK})
        message(SEND_ERROR "Could not find MoltenVK framework (at MOLTENVK_SDK dir '${MOLTENVK_SDK}')")
    else()
        message(STATUS "Found MoltenVK framework at ${MOLTENVK_FRAMEWORK}")
    endif()
else()
    find_package(Vulkan REQUIRED)
    if(Vulkan_FOUND)
        # Once we've moved on to CMake 3.20
        #cmake_path(REMOVE_FILENAME ${Vulkan_LIBRARY} OUTPUT VARIABLE Vulkan_LIBRARY_DIR)
        # Until then
        string(REGEX REPLACE lib/.*$ lib/ Vulkan_LIBRARY_DIR ${Vulkan_LIBRARY})
        message(STATUS "Found Vulkan at ${Vulkan_INCLUDE_DIR} & ${Vulkan_LIBRARY}")
    endif()
    if(APPLE)
        # Vulkan_LIBRARY points to "libvulkan.dylib".
        # Find the name of the actual dylib which includes the version no.
        # Keep this for when CI has macOS 12.3 support!
        #execute_process(COMMAND readlink -f ${Vulkan_LIBRARY}
        execute_process(COMMAND ${PROJECT_SOURCE_DIR}/ci_scripts/readlink.sh ${Vulkan_LIBRARY}
                        OUTPUT_VARIABLE Vulkan_LIBRARY_REAL_PATH_NAME
                        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        string(REGEX REPLACE ^.*/libvulkan libvulkan
               Vulkan_LIBRARY_REAL_FILE_NAME ${Vulkan_LIBRARY_REAL_PATH_NAME}
        )
        # Find the name that includes only the major version number.
        execute_process(COMMAND readlink ${Vulkan_LIBRARY}
                        OUTPUT_VARIABLE Vulkan_LIBRARY_SONAME_FILE_NAME
                        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif()
endif()

include(compile_shader.cmake)

set(SHADER_SOURCES "")

compile_shader(shader_textoverlay textoverlay appfwSDL/VulkanAppSDL/shaders shaders )
compile_shader(shader_cube cube vkloadtests/shaders/cube shaders )
compile_shader(shader_cubemap_reflect reflect vkloadtests/shaders/cubemap shaders )
compile_shader(shader_cubemap_skybox skybox vkloadtests/shaders/cubemap shaders )
compile_shader_list(shader_texture vkloadtests/shaders/texture shaders texture.vert texture1d.frag texture2d.frag)
compile_shader(shader_texture3d instancing3d vkloadtests/shaders/texture3d shaders )
compile_shader(shader_texturearray instancing vkloadtests/shaders/texturearray shaders )
compile_shader(shader_texturemipmap instancinglod vkloadtests/shaders/texturemipmap shaders )

add_custom_target(
    spirv_shaders
    DEPENDS
    shader_textoverlay
    shader_cube
    shader_cubemap_reflect
    shader_cubemap_skybox
    shader_texture
    shader_texture3d
    shader_texturearray
    shader_texturemipmap
)

set( VK_TEST_IMAGES
    etc1s_Iron_Bars_001_normal.ktx2
    uastc_Iron_Bars_001_normal.ktx2
    ktx_document_uastc_rdo4_zstd5.ktx2
    color_grid_uastc_zstd.ktx2
    color_grid_zstd.ktx2
    color_grid_uastc.ktx2
    color_grid_basis.ktx2
    kodim17_basis.ktx2
    pattern_02_bc2.ktx2
    ktx_document_basis.ktx2
    rgba-mipmap-reference-basis.ktx2
    3dtex_7_reference_u.ktx2
    arraytex_7_mipmap_reference_u.ktx2
    cubemap_goldengate_uastc_rdo4_zstd5_rd.ktx2
    cubemap_yokohama_basis_rd.ktx2
    skybox_zstd.ktx2
    orient-down-metadata.ktx
    orient-up-metadata.ktx
    rgba-reference.ktx
    etc2-rgb.ktx
    etc2-rgba8.ktx
    etc2-sRGB.ktx
    etc2-sRGBa8.ktx
    pattern_02_bc2.ktx
    rgb-amg-reference.ktx
    metalplate-amg-rgba8.ktx
    not4_rgb888_srgb.ktx
    texturearray_bc3_unorm.ktx
    texturearray_astc_8x8_unorm.ktx
    texturearray_etc2_unorm.ktx
)
list( TRANSFORM VK_TEST_IMAGES
    PREPEND "${PROJECT_SOURCE_DIR}/tests/testimages/"
)

set( KTX_RESOURCES ${LOAD_TEST_COMMON_RESOURCE_FILES} ${VK_TEST_IMAGES} )

add_executable( vkloadtests
    ${EXE_FLAG}
    appfwSDL/VulkanAppSDL/VulkanAppSDL.cpp
    appfwSDL/VulkanAppSDL/VulkanAppSDL.h
    appfwSDL/VulkanAppSDL/vulkancheckres.h
    appfwSDL/VulkanAppSDL/VulkanContext.cpp
    appfwSDL/VulkanAppSDL/VulkanContext.h
    appfwSDL/VulkanAppSDL/vulkandebug.cpp
    appfwSDL/VulkanAppSDL/vulkandebug.h
    appfwSDL/VulkanAppSDL/VulkanSwapchain.cpp
    appfwSDL/VulkanAppSDL/VulkanSwapchain.h
    appfwSDL/VulkanAppSDL/vulkantextoverlay.hpp
    appfwSDL/VulkanAppSDL/vulkantools.cpp
    appfwSDL/VulkanAppSDL/vulkantools.h
    common/disable_glm_warnings.h
    common/reenable_warnings.h
    vkloadtests/InstancedSampleBase.cpp
    vkloadtests/InstancedSampleBase.h
    vkloadtests/Texture.cpp
    vkloadtests/Texture.h
    vkloadtests/Texture3d.cpp
    vkloadtests/Texture3d.h
    vkloadtests/TextureArray.cpp
    vkloadtests/TextureArray.h
    vkloadtests/TextureCubemap.cpp
    vkloadtests/TextureCubemap.h
    vkloadtests/TexturedCube.cpp
    vkloadtests/TexturedCube.h
    vkloadtests/TextureMipmap.cpp
    vkloadtests/TextureMipmap.h
    vkloadtests/utils/VulkanMeshLoader.hpp
    vkloadtests/utils/VulkanTextureTranscoder.hpp
    vkloadtests/VulkanLoadTests.cpp
    vkloadtests/VulkanLoadTests.h
    vkloadtests/VulkanLoadTestSample.cpp
    vkloadtests/VulkanLoadTestSample.h
    ${LOAD_TEST_COMMON_RESOURCE_FILES}
    ${SHADER_SOURCES}
    ${VK_TEST_IMAGES}
    vkloadtests.cmake
)

set_code_sign(vkloadtests)

target_include_directories(vkloadtests
PRIVATE
    ${SDL2_INCLUDE_DIRS}
    $<TARGET_PROPERTY:appfwSDL,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:ktx,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:objUtil,INTERFACE_INCLUDE_DIRECTORIES>
    appfwSDL/VulkanAppSDL
    vkloadtests
    vkloadtests/utils
)

target_include_directories(vkloadtests
  SYSTEM PRIVATE
      ${PROJECT_SOURCE_DIR}/other_include
)

target_link_libraries(vkloadtests
    ktx
    ${KTX_ZLIB_LIBRARIES}
    objUtil
    appfwSDL
)

set_target_properties(vkloadtests PROPERTIES
    CXX_VISIBILITY_PRESET ${STATIC_APP_LIB_SYMBOL_VISIBILITY}
)

if(IOS)
    target_include_directories(
        vkloadtests
    PRIVATE
        ${MOLTENVK_SDK}/include
    )
elseif(Vulkan_FOUND)
    target_include_directories(
        vkloadtests
    PRIVATE
        ${Vulkan_INCLUDE_DIR}
    )
    target_link_libraries(
        vkloadtests
        ${Vulkan_LIBRARY}
    )
endif()

if(SDL2_FOUND)
    target_link_libraries(
        vkloadtests
        ${SDL2_LIBRARIES}
    )
endif()

set( MOLTENVK_ICD
    ${PROJECT_SOURCE_DIR}/other_lib/mac/resources/MoltenVK_icd.json
)
set( VK_LAYER
    ${PROJECT_SOURCE_DIR}/other_lib/mac/resources/VkLayer_khronos_validation.json
    ${PROJECT_SOURCE_DIR}/other_lib/mac/resources/VkLayer_api_dump.json
)
target_sources(vkloadtests PUBLIC ${MOLTENVK_ICD} ${VK_LAYER})

if(APPLE)
    if(IOS)
        set( INFO_PLIST "${PROJECT_SOURCE_DIR}/tests/loadtests/vkloadtests/resources/ios/Info.plist" )
        set( icon_launch_assets
            ${PROJECT_SOURCE_DIR}/icons/ios/CommonIcons.xcassets
            vkloadtests/resources/ios/LaunchImages.xcassets
            vkloadtests/resources/ios/LaunchScreen.storyboard
        )
        target_sources( vkloadtests
            PRIVATE
                ${icon_launch_assets}
        )
        # Add to resources so they'll be copied to the bundle.
        list( APPEND KTX_RESOURCES ${icon_launch_assets} )
        target_link_libraries(
            vkloadtests
            ${AudioToolbox_LIBRARY}
            ${AVFoundation_LIBRARY}
            ${CoreAudio_LIBRARY}
            ${CoreBluetooth_LIBRARY}
            ${CoreGraphics_LIBRARY}
            ${CoreMotion_LIBRARY}
            ${CoreHaptics_LIBRARY}
            ${Foundation_LIBRARY}
            ${GameController_LIBRARY}
            ${IOSurface_LIBRARY}
            ${Metal_LIBRARY}
            ${MOLTENVK_FRAMEWORK}
            ${OpenGLES_LIBRARY}
            ${QuartzCore_LIBRARY}
            ${UIKit_LIBRARY}
        )
    else()
        set( INFO_PLIST "${PROJECT_SOURCE_DIR}/tests/loadtests/vkloadtests/resources/mac/Info.plist" )
    endif()

    set_source_files_properties(${MOLTENVK_ICD} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/vulkan/icd.d")
    set_source_files_properties(${VK_LAYER} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/vulkan/explicit_layer.d")
elseif(WIN32)
    ensure_runtime_dependencies_windows(vkloadtests)
elseif(LINUX)
        target_sources(
            vkloadtests
        PRIVATE
            vkloadtests/resources/linux/vkloadtests.desktop
        )

endif()

target_link_libraries( vkloadtests ${LOAD_TEST_COMMON_LIBS} )

target_compile_definitions(
    vkloadtests
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
)

set_target_properties( vkloadtests PROPERTIES RESOURCE "${KTX_RESOURCES};${SHADER_SOURCES}" )

if(APPLE)
    set(PRODUCT_NAME "vkloadtests")
    set(EXECUTABLE_NAME ${PRODUCT_NAME})
    # How amazingly irritating. We have to set both of these to the same value.
    # The first must be set otherwise the app cannot be installed on iOS. The
    # second has to be set to avoid an Xcode warning.
    set(PRODUCT_BUNDLE_IDENTIFIER "org.khronos.ktx.${PRODUCT_NAME}")
    set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.khronos.ktx.${PRODUCT_NAME}")
    configure_file( ${INFO_PLIST} vkloadtests/Info.plist )
    set_target_properties( vkloadtests PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/vkloadtests/Info.plist"
        MACOSX_BUNDLE_ICON_FILE "ktx_app.icns"
        # Because libassimp is built with bitcode disabled. It's not important unless
        # submitting to the App Store and currently bitcode is optional.
        XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
        XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES"
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "ktx_app"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" # iPhone and iPad
    )
    unset(PRODUCT_NAME)
    unset(EXECUTABLE_NAME)
    unset(PRODUCT_BUNDLE_IDENTIFIER)

    # The generated project code for building an Apple bundle automatically
    # copies the executable and all files with the RESOURCE property to the
    # bundle adjusting for the difference in bundle layout between iOS &
    # macOS.

    if(NOT IOS)
        # Set RPATH to find libktx dylib
        set_target_properties( vkloadtests PROPERTIES
            INSTALL_RPATH "@executable_path/../Frameworks"
        )

        if(NOT KTX_FEATURE_STATIC_LIBRARY)
          add_custom_command( TARGET vkloadtests POST_BUILD
              COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:ktx> "$<TARGET_BUNDLE_CONTENT_DIR:vkloadtests>/Frameworks/$<TARGET_FILE_NAME:ktx>"
              COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE_NAME:ktx> "$<TARGET_BUNDLE_CONTENT_DIR:vkloadtests>/Frameworks/$<TARGET_SONAME_FILE_NAME:ktx>"
              COMMENT "Copy KTX library to build destination"
          )
        endif()
        add_custom_command( TARGET vkloadtests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy "${Vulkan_LIBRARY_DIR}/libMoltenVK.dylib" "$<TARGET_BUNDLE_CONTENT_DIR:vkloadtests>/Frameworks/libMoltenVK.dylib"
            COMMAND ${CMAKE_COMMAND} -E copy "${Vulkan_LIBRARY_DIR}/libVkLayer*.dylib" "$<TARGET_BUNDLE_CONTENT_DIR:vkloadtests>/Frameworks/"
            COMMAND ${CMAKE_COMMAND} -E copy "${Vulkan_LIBRARY_REAL_PATH_NAME}" "$<TARGET_BUNDLE_CONTENT_DIR:vkloadtests>/Frameworks/"
            COMMAND ${CMAKE_COMMAND} -E create_symlink "${Vulkan_LIBRARY_REAL_FILE_NAME}" "$<TARGET_BUNDLE_CONTENT_DIR:vkloadtests>/Frameworks/${Vulkan_LIBRARY_SONAME_FILE_NAME}"
            COMMENT "Copy libraries & frameworks to build destination"
        )
        # No need to copy when there is a TARGET. The BREW SDL
        # library has no LC_RPATH setting so the binary will
        # only search for it where it was during linking.
        # The vcpkg SDL target copies the library.
        if(NOT TARGET SDL2::SDL2)
            add_custom_command( TARGET vkloadtests POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/other_lib/mac/$<CONFIG>/libSDL2.dylib" "$<TARGET_BUNDLE_CONTENT_DIR:vkloadtests>/Frameworks/libSDL2.dylib"
                COMMENT "Copy repo's SDL2 library to build destination"
            )
        endif()

        # Specify destination for cmake --install.
        install(TARGETS vkloadtests
            BUNDLE
                DESTINATION /Applications
                COMPONENT VkLoadTestApp
        )

        ## Uncomment for Bundle analysis
        # install( CODE "
        #     include(BundleUtilities)
        #     verify_app($<TARGET_BUNDLE_DIR:vkloadtests>)
        #     #fixup_bundle($<TARGET_BUNDLE_DIR:vkloadtests> \"\" \"\")"
        # )
    endif()
else()
    # This is for other platforms.
    # This copies the resources next to the executable for ease
    # of use during debugging and testing.
    add_custom_command( TARGET vkloadtests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
          $<TARGET_FILE_DIR:vkloadtests>/../resources
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${KTX_RESOURCES} ${SHADER_SOURCES}
          $<TARGET_FILE_DIR:vkloadtests>/../resources
    )

    # To keep the resources (test images and models) close to the
    # executable and to be compliant with the Filesystem Hierarchy
    # Standard https://refspecs.linuxfoundation.org/FHS_3.0/fhs/index.html
    # we have chosen to install the apps and data in /opt/<target>.
    # Each target has a `bin` directory with the executable and a
    # `resources` directory with the resources. We install a symbolic
    # link to the executable in ${CMAKE_INSTALL_LIBDIR}, usually
    # /usr/local/bin.

    set_target_properties( vkloadtests PROPERTIES
        INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}"
    )

    ######### IMPORTANT ######
    # When installing via `cmake --install` ALSO install the
    # library component. There seems no way to make a dependency.
    ##########################

#    set( destroot "${LOAD_TEST_DESTROOT}/$<TARGET_FILE_NAME:vkloadtests>")
#    # NOTE: WHEN RUNNING MANUAL INSTALLS INSTALL library COMPONENT TOO.
#    install(TARGETS vkloadtests
#        RUNTIME
#            DESTINATION ${destroot}/bin
#            COMPONENT VkLoadTestApp
#        RESOURCE
#            DESTINATION ${destroot}/resources
#            COMPONENT VkLoadTestApp
#    )
#    if(LINUX)
#        # Add a link from the regular bin directory to put command
#        # on PATH.
#        install(CODE "
#           EXECUTE_PROCESS(COMMAND ln -s ${destroot}/bin/$<TARGET_FILE_NAME:vkloadtests> ${CMAKE_INSTALL_FULL_BINDIR}
#           )"
#           COMPONENT VkLoadTestApp
#        )
#        install(FILES
#            vkloadtests/resources/linux/vkloadtests.desktop
#            DESTINATION /usr/share/applications
#            COMPONENT VkLoadTestApp
#        )
#    endif()
endif()

add_dependencies(
    vkloadtests
    spirv_shaders
)


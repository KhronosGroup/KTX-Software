# Copyright 2020 Andreas Atteneder
# SPDX-License-Identifier: Apache-2.0

# Find Vulkan package
if(IOS)
    # On iOS we link against MoltenVK.framework manually (see below)
else()
    find_package(Vulkan)
    if(Vulkan_FOUND)
        if(NOT Vulkan_INCLUDE_DIRS AND Vulkan_INCLUDE_DIR)
            # Fallback for inconsistent variable name on Windows
            set(Vulkan_INCLUDE_DIRS ${Vulkan_INCLUDE_DIR})
        endif()
    endif()
endif()


if(APPLE)
    # Try to locate Vulkan SDK install directory
    if(NOT VULKAN_INSTALL_DIR)
        message(SEND_ERROR "Please provide a valid path to your Vulkan SDK installation in CMake variable 'VULKAN_INSTALL_DIR'!")
    endif()

    if(IOS)
        set( VULKAN_SDK "${VULKAN_INSTALL_DIR}/MoltenVK" )
        set( MOLTEN_VK_FRAMEWORK ${VULKAN_SDK}/iOS/framework/MoltenVK.framework )
        if( NOT IS_DIRECTORY ${MOLTEN_VK_FRAMEWORK})
            # Fallback: Older Vulkan SDKs have MoltenVK.framework at a different sub path
            message("Could not find MoltenVK.framework (at VULKAN_SDK dir '${VULKAN_SDK}')")
            set(MOLTEN_VK_FRAMEWORK ${VULKAN_SDK}/iOS/MoltenVK.framework)
        endif()
        if( NOT IS_DIRECTORY ${MOLTEN_VK_FRAMEWORK})
            message(SEND_ERROR "Could not find MoltenVK.framework (at VULKAN_SDK dir '${VULKAN_SDK}')")
        else()
            message("Found MoltenVK.framwork at ${MOLTEN_VK_FRAMEWORK}")
        endif()
    else()
        set( VULKAN_SDK "${VULKAN_INSTALL_DIR}/macOS")
    endif()
endif()

include(compile_shader.cmake)

set(SHADER_SOURCES "")

compile_shader(shader_textoverlay textoverlay appfwSDL/VulkanAppSDL/shaders shaders )
compile_shader(shader_cube cube vkloadtests/shaders/cube shaders )
compile_shader(shader_cubemap_reflect reflect vkloadtests/shaders/cubemap shaders )
compile_shader(shader_cubemap_skybox skybox vkloadtests/shaders/cubemap shaders )
compile_shader_list(shader_texture vkloadtests/shaders/texture shaders texture.vert texture1d.frag texture2d.frag)
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
    shader_texturearray
    shader_texturemipmap
)

add_executable( vkloadtests
    ${EXE_FLAG}
    $<TARGET_OBJECTS:appfwSDL>
    $<TARGET_OBJECTS:objUtil>
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
    vkloadtests/InstancedSampleBase.cpp
    vkloadtests/InstancedSampleBase.h
    vkloadtests/Texture.cpp
    vkloadtests/Texture.h
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
)
if(IOS)
    set_xcode_code_sign(vkloadtests)
endif()

target_include_directories(
    vkloadtests
PRIVATE
    ${SDL2_INCLUDE_DIRS}
    $<TARGET_PROPERTY:appfwSDL,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:ktx,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:objUtil,INTERFACE_INCLUDE_DIRECTORIES>
    appfwSDL/VulkanAppSDL
    vkloadtests
    vkloadtests/utils
)

target_link_libraries(
    vkloadtests
    ktx
    ${KTX_ZLIB_LIBRARIES}
)

if(IOS)
    target_include_directories(
        vkloadtests
    PRIVATE
        ${VULKAN_SDK}/include
    )
elseif(Vulkan_FOUND)
    target_include_directories(
        vkloadtests
    PRIVATE
        ${Vulkan_INCLUDE_DIRS}
    )
    target_link_libraries(
        vkloadtests
        ${Vulkan_LIBRARIES}
    )
endif()

if(SDL2_FOUND)
    target_link_libraries(
        vkloadtests
        ${SDL2_LIBRARIES}
    )
endif()

set( MOLTEN_VK_ICD
    ${PROJECT_SOURCE_DIR}/other_lib/mac/resources/MoltenVK_icd.json
)
set( VK_LAYER
    ${PROJECT_SOURCE_DIR}/other_lib/mac/resources/VkLayer_khronos_validation.json
    ${PROJECT_SOURCE_DIR}/other_lib/mac/resources/VkLayer_api_dump.json
)
target_sources(vkloadtests PUBLIC ${MOLTEN_VK_ICD} ${VK_LAYER})

if(APPLE)
    if(IOS)
        set( INFO_PLIST "${PROJECT_SOURCE_DIR}/tests/loadtests/vkloadtests/resources/ios/Info.plist" )
        set( KTX_RESOURCES
            ${PROJECT_SOURCE_DIR}/icons/ios/CommonIcons.xcassets
            vkloadtests/resources/ios/LaunchImages.xcassets
            vkloadtests/resources/ios/LaunchScreen.storyboard
        )
        target_sources( vkloadtests PRIVATE ${KTX_RESOURCES} )
        target_link_libraries(
            vkloadtests
            ${AudioToolbox_LIBRARY}
            ${AVFoundation_LIBRARY}
            ${CoreAudio_LIBRARY}
            ${CoreBluetooth_LIBRARY}
            ${CoreGraphics_LIBRARY}
            ${CoreMotion_LIBRARY}
            ${Foundation_LIBRARY}
            ${GameController_LIBRARY}
            ${Metal_LIBRARY}
            ${MOLTEN_VK_FRAMEWORK}
            ${OpenGLES_LIBRARY}
            ${QuartzCore_LIBRARY}
            ${UIKit_LIBRARY}
        )
    else()
        set( KTX_RESOURCES ${KTX_ICON} )
        set( INFO_PLIST "${PROJECT_SOURCE_DIR}/tests/loadtests/vkloadtests/resources/mac/Info.plist" )
        set( VK_ICD "${VULKAN_SDK}/share/vulkan/icd.d/MoltenVK_icd.json" )
    endif()

    set_source_files_properties(${MOLTEN_VK_ICD} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/vulkan/icd.d")
    set_source_files_properties(${VK_LAYER} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/vulkan/explicit_layer.d")
elseif(WIN32)
    ensure_runtime_dependencies_windows(vkloadtests)
endif()

target_link_libraries( vkloadtests ${LOAD_TEST_COMMON_LIBS} )

target_compile_definitions(
    vkloadtests
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
)

if(APPLE)
    set_source_files_properties(${SHADER_SOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/shaders")

    set(PRODUCT_NAME "vkloadtests")
    set(EXECUTABLE_NAME ${PRODUCT_NAME})
    # How amazingly irritating. We have to set both of these to the same value.
    # The first must be set otherwise the app cannot be installed on iOS. The second
    # has to be set to avoid an Xcode warning.
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
    set_xcode_code_sign(vkloadtests)
    unset(PRODUCT_NAME)
    unset(EXECUTABLE_NAME)
    unset(PRODUCT_BUNDLE_IDENTIFIER)
    if(KTX_RESOURCES)
        set_target_properties( vkloadtests PROPERTIES RESOURCE "${KTX_RESOURCES}" )
    endif()

    if(NOT IOS)
        set_target_properties( vkloadtests PROPERTIES
            INSTALL_RPATH "@executable_path/../Frameworks"
        )
        add_custom_command( TARGET vkloadtests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy "${VULKAN_SDK}/lib/libMoltenVK.dylib" "$<TARGET_BUNDLE_CONTENT_DIR:vkloadtests>/Frameworks/libMoltenVK.dylib"
            COMMAND ${CMAKE_COMMAND} -E copy "${VULKAN_SDK}/lib/libVkLayer*.dylib" "$<TARGET_BUNDLE_CONTENT_DIR:vkloadtests>/Frameworks/"
            COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/other_lib/mac/$<CONFIG>/libSDL2.dylib" "$<TARGET_BUNDLE_CONTENT_DIR:vkloadtests>/Frameworks/libSDL2.dylib"
            COMMENT "Copy libraries/frameworks to build destination"
        )

        install(TARGETS ktx
            LIBRARY
                DESTINATION "$<TARGET_BUNDLE_CONTENT_DIR:vkloadtests>/Frameworks"
                COMPONENT VkLoadTestApp
            PUBLIC_HEADER
                DESTINATION "$<TARGET_BUNDLE_CONTENT_DIR:vkloadtests>/Headers"
        )
        install(DIRECTORY "${VULKAN_SDK}/Frameworks/vulkan.framework"
            DESTINATION "$<TARGET_BUNDLE_CONTENT_DIR:vkloadtests>/Frameworks"
            COMPONENT VkLoadTestApp
        )
        install(TARGETS vkloadtests
            BUNDLE
                DESTINATION .
                COMPONENT VkLoadTestApp
            RESOURCE
                COMPONENT VkLoadTestApp
                ## Providing a destination shuts down a warning, but produces an orphaned file in installs
                # DESTINATION Resources
        )

        ## Uncomment for Bundle analyzation
        # install( CODE "
        #     include(BundleUtilities)
        #     verify_app($<TARGET_BUNDLE_DIR:vkloadtests>)
        #     #fixup_bundle($<TARGET_BUNDLE_DIR:vkloadtests> \"\" \"\")"
        # )
    endif()
endif()

add_dependencies(
    vkloadtests
    spirv_shaders
)

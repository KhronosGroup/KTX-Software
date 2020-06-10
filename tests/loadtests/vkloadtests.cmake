
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
)

target_sources(
    vkloadtests
PUBLIC
    ${SHADER_SOURCES}
)

target_include_directories(
    vkloadtests
PRIVATE
    ${SDL2_INCLUDE_DIRS}
    $<TARGET_PROPERTY:appfwSDL,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES>
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

if(Vulkan_FOUND)
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
    ${PROJECT_SOURCE_DIR}/other_lib/mac/resources/VkLayer_core_validation.json
    ${PROJECT_SOURCE_DIR}/other_lib/mac/resources/VkLayer_object_tracker.json
    ${PROJECT_SOURCE_DIR}/other_lib/mac/resources/VkLayer_parameter_validation.json
    ${PROJECT_SOURCE_DIR}/other_lib/mac/resources/VkLayer_standard_validation.json
    ${PROJECT_SOURCE_DIR}/other_lib/mac/resources/VkLayer_threading.json
    ${PROJECT_SOURCE_DIR}/other_lib/mac/resources/VkLayer_unique_objects.json
)
target_sources(vkloadtests PUBLIC ${MOLTEN_VK_ICD} ${VK_LAYER})

if(APPLE)
    if(IOS)

        # Try to locate MoltenVK.framework
        set(MOLTEN_VK_FRAMEWORK ${VULKAN_SDK}/iOS/framework/MoltenVK.framework)
        if(NOT IS_DIRECTORY ${MOLTEN_VK_FRAMEWORK})
            # Fallback: Older Vulkan SDKs have MoltenVK.framework at a different sub path
            message("Could not find MoltenVK.framework (at VULKAN_SDK dir ${VULKAN_SDK})")
            set(MOLTEN_VK_FRAMEWORK ${VULKAN_SDK}/iOS/MoltenVK.framework)
        endif()
        if(NOT IS_DIRECTORY ${MOLTEN_VK_FRAMEWORK})
            message(SEND_ERROR "Could not find MoltenVK.framework (at VULKAN_SDK dir ${VULKAN_SDK})")
        else()
            message("Found MoltenVK.framwork at ${MOLTEN_VK_FRAMEWORK}")
        endif()


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
    set(PRODUCT_NAME "vkloadtests")
    set(EXECUTABLE_NAME ${PRODUCT_NAME})
    set(PRODUCT_BUNDLE_IDENTIFIER "org.khronos.ktx.${PRODUCT_NAME}")
    configure_file( ${INFO_PLIST} vkloadtests/Info.plist )
    set_target_properties( vkloadtests PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/vkloadtests/Info.plist"
        MACOSX_BUNDLE_ICON_FILE "ktx_app.icns"
        # Because libassimp is built with bitcode disabled. It's not important unless
        # submitting to the App Store and currently bitcode is optional.
        XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
        XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES"
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "ktx_app"
    )
    unset(PRODUCT_NAME)
    unset(EXECUTABLE_NAME)
    unset(PRODUCT_BUNDLE_IDENTIFIER)
    if(KTX_RESOURCES)
        set_target_properties( vkloadtests PROPERTIES RESOURCE "${KTX_RESOURCES}" )
    endif()

    if(NOT IOS)
        add_custom_command( TARGET vkloadtests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:ktx> "$<TARGET_BUNDLE_CONTENT_DIR:vkloadtests>/Frameworks/$<TARGET_FILE_NAME:ktx>"
            COMMAND ${CMAKE_COMMAND} -E copy "${VULKAN_SDK}/lib/libMoltenVK.dylib" "$<TARGET_BUNDLE_CONTENT_DIR:vkloadtests>/Frameworks/libMoltenVK.dylib"
            COMMAND ${CMAKE_COMMAND} -E copy "${VULKAN_SDK}/lib/libVkLayer*.dylib" "$<TARGET_BUNDLE_CONTENT_DIR:vkloadtests>/Frameworks/"
            COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/other_lib/mac/$<CONFIG>/libSDL2.dylib" "$<TARGET_BUNDLE_CONTENT_DIR:vkloadtests>/Frameworks/libSDL2.dylib"
            COMMENT "Copy libraries/frameworks to build destination"
        )

        install(DIRECTORY "${VULKAN_SDK}/Frameworks/vulkan.framework" DESTINATION "$<TARGET_BUNDLE_CONTENT_DIR:vkloadtests>/Frameworks/vulkan.framework" )
        install(TARGETS vkloadtests BUNDLE DESTINATION .)
    endif()
endif()

add_dependencies(
    vkloadtests
    spirv_shaders
)

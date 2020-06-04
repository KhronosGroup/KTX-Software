
add_executable( vkloadtests
    ${EXE_FLAG}
    $<TARGET_OBJECTS:appfwSDL>
    $<TARGET_OBJECTS:objUtil>
    appfwSDL/VulkanAppSDL/VulkanAppSDL.cpp
    appfwSDL/VulkanAppSDL/VulkanContext.cpp
    appfwSDL/VulkanAppSDL/vulkandebug.cpp
    appfwSDL/VulkanAppSDL/VulkanSwapchain.cpp
    appfwSDL/VulkanAppSDL/vulkantools.cpp
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
)

if(ZLIB_FOUND)
    target_link_libraries( vkloadtests ZLIB::ZLIB )
endif()

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

if(APPLE)
    if(IOS)
        set( INFO_PLIST "${PROJECT_SOURCE_DIR}/tests/loadtests/vkloadtests/resources/ios/Info.plist" )
        target_sources(
            vkloadtests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/icons/ios/CommonIcons.xcassets
            vkloadtests/resources/ios/LaunchImages.xcassets
            vkloadtests/resources/ios/LaunchScreen.storyboard
        )
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
            ${OpenGLES_LIBRARY}
            ${QuartzCore_LIBRARY}
            ${UIKit_LIBRARY}
        )
    else()
        set( INFO_PLIST "${PROJECT_SOURCE_DIR}/tests/loadtests/vkloadtests/resources/mac/Info.plist" )
    endif()
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
    )
endif()
if(KTX_ICON)
    set_target_properties( vkloadtests PROPERTIES RESOURCE ${KTX_ICON} )
endif()

add_dependencies(
    vkloadtests
    spirv_shaders
)


add_executable( vkloadtests
${EXE_FLAG}
$<TARGET_OBJECTS:appfwSDL>
$<TARGET_OBJECTS:objUtil>
appfwSDL/VulkanAppSDL/VulkanAppSDL.cpp
appfwSDL/VulkanAppSDL/VulkanContext.cpp
appfwSDL/VulkanAppSDL/vulkandebug.cpp
appfwSDL/VulkanAppSDL/VulkanSwapchain.cpp
appfwSDL/VulkanAppSDL/vulkantools.cpp
common/LoadTestSample.cpp
common/SwipeDetector.cpp
vkloadtests/InstancedSampleBase.cpp
vkloadtests/Texture.cpp
vkloadtests/TextureArray.cpp
vkloadtests/TextureCubemap.cpp
vkloadtests/TexturedCube.cpp
vkloadtests/TextureMipmap.cpp
vkloadtests/VulkanLoadTests.cpp
vkloadtests/VulkanLoadTestSample.cpp
${GL_LOAD_TEST_RESOURCE_FILES}
)

target_sources(
    vkloadtests
PUBLIC
    ${SHADER_SOURCES}
)

target_include_directories(
vkloadtests
PRIVATE
$<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES>
${PROJECT_SOURCE_DIR}/lib
${PROJECT_SOURCE_DIR}/other_include
${PROJECT_SOURCE_DIR}/utils
${SDL2_INCLUDE_DIRS}
appfwSDL
appfwSDL/VulkanAppSDL
common
geom
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

if(APPLE)
target_link_libraries(
    vkloadtests
    ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libassimp.a
    ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libIrrXML.a
    ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libminizip.a
    ${SDL2_LIBRARIES}
)
elseif(EMSCRIPTEN)
elseif(LINUX)

set( LINUX_LIB_DIR Release )
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set( LINUX_LIB_DIR Debug )
endif()

target_link_libraries(
    vkloadtests
    ${assimp_LIBRARIES}
    ${CMAKE_SOURCE_DIR}/other_lib/linux/${LINUX_LIB_DIR}-x64/libSDL2.a
)
elseif(WIN32)
target_link_libraries(
    vkloadtests
    "${CMAKE_SOURCE_DIR}/other_lib/win/$<CONFIG>-x64/SDL2.lib"
    "${CMAKE_SOURCE_DIR}/other_lib/win/$<CONFIG>-x64/SDL2main.lib"
    "${CMAKE_SOURCE_DIR}/other_lib/win/Release-x64/assimp.lib"
    "${CMAKE_SOURCE_DIR}/other_lib/win/Release-x64/glew32.lib"
)
ensure_runtime_dependencies_windows(vkloadtests)
endif()

target_compile_definitions(
vkloadtests
PRIVATE
$<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
)

set_target_properties( vkloadtests PROPERTIES
    RESOURCE "${PROJECT_SOURCE_DIR}/icons/mac/ktx_app.icns"
    MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/tests/loadtests/vkloadtests/resources/mac/Info.plist"
    MACOSX_BUNDLE_ICON_FILE "ktx_app.icns"
    # Because libassimp is built with bitcode disabled. It's not important unless
    # submitting to the App Store and currently bitcode is optional.
    XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
    XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES"
)

add_dependencies(
    vkloadtests
    spirv_shaders
)


if(NOT WIN32)
    find_package(SDL2 REQUIRED)
endif()
if(NOT APPLE)
    find_package(assimp REQUIRED)
endif()

find_package( ZLIB REQUIRED )

set(builddir "${CMAKE_BINARY_DIR}")

# By wrapping in generator expression we force multi configuration generators (like Visual Studio)
# to take the exact path and not change it.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${builddir}/$<CONFIG>>)


function( ensure_runtime_dependencies_windows target )
    # Custom copy commands to ensure all dependencies (DLLs, testimages, shaders, models) are in correct location
    # relative to executable.
    add_custom_command( TARGET gl3loadtests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/other_lib/win/$<CONFIG>-x64/SDL2.dll" "$<TARGET_FILE_DIR:${target}>"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/other_lib/win/Release-x64/assimp.dll" "$<TARGET_FILE_DIR:${target}>"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/other_lib/win/Release-x64/glew32.dll" "$<TARGET_FILE_DIR:${target}>"
        COMMENT "Copy DLLs to build destination"
    )
    add_custom_command( TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory  "${builddir}/shaders" "$<TARGET_FILE_DIR:${target}>/shaders"
        COMMENT "Copy shaders to build destination"
    )
    add_custom_command( TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory  "${PROJECT_SOURCE_DIR}/tests/testimages" "$<TARGET_FILE_DIR:${target}>/testimages"
        COMMENT "Copy testimages to build destination"
    )
    add_custom_command( TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory  "${PROJECT_SOURCE_DIR}/tests/loadtests/common/models" "$<TARGET_FILE_DIR:${target}>/models"
        COMMENT "Copy models to build destination"
    )
endfunction()

add_library( appfwSDL OBJECT
    appfwSDL/AppBaseSDL.cpp
    appfwSDL/main.cpp
)

target_include_directories(
    appfwSDL
PUBLIC
    ${SDL2_INCLUDE_DIRS}
    appfwSDL
    $<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/other_include
    ${PROJECT_SOURCE_DIR}/utils
    common
    geom
    glloadtests
    glloadtests/utils
)

if(OPENGL_FOUND)
    target_include_directories(
        appfwSDL
    PUBLIC
        ${OPENGL_INCLUDE_DIR}
    )
endif()

#Get a list of all of the files in the testimages
file(GLOB_RECURSE TEST_IMAGES "${PROJECT_SOURCE_DIR}/tests/testimages/*")
#Get a list of all of the files in the models
file(GLOB_RECURSE MODELS "common/models/*")

set( GL_LOAD_TEST_RESOURCE_FILES
    ${TEST_IMAGES}
    ${MODELS}
    "${PROJECT_SOURCE_DIR}/icons/mac/ktx_app.icns"
)

if(APPLE)
    set( EXE_FLAG MACOSX_BUNDLE )
elseif(WIN32)
    set( EXE_FLAG WIN32 )
endif()

add_executable( gl3loadtests
    ${EXE_FLAG}
    $<TARGET_OBJECTS:appfwSDL>
    $<TARGET_OBJECTS:objUtil>
    appfwSDL/GLAppSDL.cpp
    common/LoadTestSample.cpp
    common/SwipeDetector.cpp
    glloadtests/GLLoadTests.cpp
    glloadtests/shader-based/BasisuTest.cpp
    glloadtests/shader-based/DrawTexture.cpp
    glloadtests/shader-based/GL3LoadTests.cpp
    glloadtests/shader-based/GL3LoadTestSample.cpp
    glloadtests/shader-based/shaders.cpp
    glloadtests/shader-based/TextureArray.cpp
    glloadtests/shader-based/TextureCubemap.cpp
    glloadtests/shader-based/TexturedCube.cpp
    ${GL_LOAD_TEST_RESOURCE_FILES}
    )

target_include_directories(
    gl3loadtests
    PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/other_include
    ${PROJECT_SOURCE_DIR}/utils
    appfwSDL
    common
    geom
    glloadtests
    glloadtests/utils
    )

if(OPENGL_FOUND)
    target_include_directories(
        gl3loadtests
        PRIVATE
        ${OPENGL_INCLUDE_DIR}
    )
endif()

target_link_libraries(
    gl3loadtests
    ${OPENGL_LIBRARIES}
    ${SDL2_LIBRARIES}
    ktx
    ZLIB::ZLIB
)

if(APPLE)
    target_link_libraries(
        gl3loadtests
        ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libassimp.a
        ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libIrrXML.a
        ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libminizip.a
    )
elseif(LINUX)
    target_link_libraries(
        gl3loadtests
        ${assimp_LIBRARIES}
    )
elseif(WIN32)
    target_link_libraries(
        gl3loadtests
        "${CMAKE_SOURCE_DIR}/other_lib/win/$<CONFIG>-x64/SDL2.lib"
        "${CMAKE_SOURCE_DIR}/other_lib/win/$<CONFIG>-x64/SDL2main.lib"
        "${CMAKE_SOURCE_DIR}/other_lib/win/Release-x64/assimp.lib"
        "${CMAKE_SOURCE_DIR}/other_lib/win/Release-x64/glew32.lib"
    )
    ensure_runtime_dependencies_windows(gl3loadtests)
endif()

target_compile_definitions(
    gl3loadtests
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
    "GL_CONTEXT_PROFILE=SDL_GL_CONTEXT_PROFILE_${KTX_GL_CONTEXT_PROFILE}"
    GL_CONTEXT_MAJOR_VERSION=${KTX_GL_CONTEXT_MAJOR_VERSION}
    GL_CONTEXT_MINOR_VERSION=${KTX_GL_CONTEXT_MINOR_VERSION}
)

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
    ZLIB::ZLIB
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

if(APPLE)
    target_link_libraries(
        vkloadtests
        ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libassimp.a
        ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libIrrXML.a
        ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libminizip.a
        ${SDL2_LIBRARIES}
    )
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

function(compileshader shader_target shader_name shader_src_path shader_path)

    set(vert_name "${shader_name}.vert")
    set(vert2spirv_in "${shader_src_path}/${vert_name}")
    set(vert2spirv_out "${builddir}/${shader_path}/${vert_name}.spv")

    add_custom_command(OUTPUT
    ${vert2spirv_out}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${builddir}/${shader_path}
    COMMAND glslc "-fshader-stage=vertex" -o "${vert2spirv_out}" "${vert2spirv_in}"
    DEPENDS ${vert2spirv_in}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMENT "Compiling ${vert_name}."
    VERBATIM
    )

    set(frag_name "${shader_name}.frag")
    set(frag2spirv_in "${shader_src_path}/${frag_name}")
    set(frag2spirv_out "${builddir}/${shader_path}/${frag_name}.spv")

    add_custom_command(OUTPUT
    ${frag2spirv_out}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${builddir}/${shader_path}
    COMMAND glslc "-fshader-stage=fragment" -o "${frag2spirv_out}" "${frag2spirv_in}"
    DEPENDS ${frag2spirv_in}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMENT "Compiling ${frag_name}."
    VERBATIM
    )

    add_custom_target(
        ${shader_target}
        DEPENDS
        ${vert2spirv_out}
        ${frag2spirv_out}
        SOURCES
        ${vert2spirv_in}
        ${frag2spirv_in}
    )

    set_target_properties(${shader_target} PROPERTIES EXCLUDE_FROM_ALL "FALSE")

    target_sources( gl3loadtests
    PUBLIC
        ${frag2spirv_out}
        ${vert2spirv_out}
    )

    target_sources( vkloadtests
    PUBLIC
        ${frag2spirv_out}
        ${vert2spirv_out}
    )

    set_source_files_properties(${frag2spirv_out} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/shaders")
    set_source_files_properties(${vert2spirv_out} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/shaders")

endfunction(compileshader)

compileshader(shader_textoverlay textoverlay appfwSDL/VulkanAppSDL/shaders shaders )
compileshader(shader_cube cube vkloadtests/shaders/cube shaders )
compileshader(shader_cubemap_reflect reflect vkloadtests/shaders/cubemap shaders )
compileshader(shader_cubemap_skybox skybox vkloadtests/shaders/cubemap shaders )
compileshader(shader_texture texture vkloadtests/shaders/texture shaders )
compileshader(shader_texturearray instancing vkloadtests/shaders/texturearray shaders )
compileshader(shader_texturemipmap instancinglod vkloadtests/shaders/texturemipmap shaders )

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

set_target_properties(spirv_shaders PROPERTIES EXCLUDE_FROM_ALL "FALSE")

set_source_files_properties(${TEST_IMAGES} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/testimages")
set_source_files_properties(${MODELS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/models")
# Hack to prevent *.obj 3D files to be mistaken as linkable obj files
set_source_files_properties(${MODELS} PROPERTIES HEADER_FILE_ONLY TRUE )

set_target_properties( gl3loadtests PROPERTIES
    RESOURCE "${PROJECT_SOURCE_DIR}/icons/mac/ktx_app.icns"
    MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/tests/loadtests/glloadtests/resources/mac/Info.plist"
    MACOSX_BUNDLE_ICON_FILE "ktx_app.icns"
)

set_target_properties( vkloadtests PROPERTIES
    RESOURCE "${PROJECT_SOURCE_DIR}/icons/mac/ktx_app.icns"
    MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/tests/loadtests/vkloadtests/resources/mac/Info.plist"
    MACOSX_BUNDLE_ICON_FILE "ktx_app.icns"
)

# file(COPY "${PROJECT_SOURCE_DIR}/tests/testimages" DESTINATION "${builddir}")
# file(COPY common/models DESTINATION "${builddir}")

add_dependencies(
    vkloadtests
    spirv_shaders
)

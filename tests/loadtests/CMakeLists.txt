
find_package(SDL2)

add_library( appfwSDL OBJECT
    appfwSDL/AppBaseSDL.cpp
    appfwSDL/main.cpp
)

target_include_directories(
    appfwSDL
    PUBLIC
    ${SDL2_INCLUDE_DIRS}
    appfwSDL
    ${objKtxRead_include_dirs}
    ${OPENGL_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/other_include
    ${PROJECT_SOURCE_DIR}/utils
    common
    geom
    glloadtests
    glloadtests/utils
)

target_link_libraries(
    appfwSDL
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES}
)

add_executable( gl3loadtests
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
    )

target_include_directories(
    gl3loadtests
    PRIVATE
    ${objKtxRead_include_dirs}
    ${OPENGL_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/other_include
    ${PROJECT_SOURCE_DIR}/utils
    appfwSDL
    common
    geom
    glloadtests
    glloadtests/utils
    )

target_link_libraries(
    gl3loadtests
    ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libassimp.a
    ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libIrrXML.a
    ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libminizip.a
    ${OPENGL_LIBRARIES}
    ${SDL2_LIBRARIES}
    appfwSDL
    gtest
    ktx
    z
    )

target_compile_definitions(
    gl3loadtests
    PRIVATE
    ${KTX_DEFS}
    "GL_CONTEXT_PROFILE=SDL_GL_CONTEXT_PROFILE_${KTX_GL_CONTEXT_PROFILE}"
    GL_CONTEXT_MAJOR_VERSION=${KTX_GL_CONTEXT_MAJOR_VERSION}
    GL_CONTEXT_MINOR_VERSION=${KTX_GL_CONTEXT_MINOR_VERSION}
)

add_executable( vkloadtests
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
    )

target_include_directories(
    vkloadtests
    PRIVATE
    ${objKtxRead_include_dirs}
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/other_include
    ${PROJECT_SOURCE_DIR}/utils
    ${Vulkan_INCLUDE_DIRS}
    appfwSDL
    appfwSDL/VulkanAppSDL
    common
    geom
    vkloadtests
    vkloadtests/utils
    )

target_link_libraries(
    vkloadtests
    ${Vulkan_LIBRARIES}
    ${SDL2_LIBRARIES}
    gtest
    ktx
)

target_link_libraries(
    vkloadtests
    z
    )

if(APPLE)
target_link_libraries(
    vkloadtests
    ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libassimp.a
    ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libIrrXML.a
    ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libminizip.a
    )
elseif(LINUX)
target_link_libraries(
    vkloadtests
    assimp
    )
endif()

target_compile_definitions(
    vkloadtests
    PRIVATE
    ${KTX_DEFS}
)

set(builddir "${CMAKE_CURRENT_BINARY_DIR}")

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

file(COPY "${PROJECT_SOURCE_DIR}/tests/testimages" DESTINATION "${builddir}")
file(COPY common/models DESTINATION "${builddir}")

add_dependencies(
    vkloadtests
    spirv_shaders
)

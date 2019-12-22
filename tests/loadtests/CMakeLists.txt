
find_package(SDL2)

set( SDL_SRC
    appfwSDL/AppBaseSDL.cpp
    appfwSDL/main.cpp
)

add_executable( glloadtest
    $<TARGET_OBJECTS:objUtil>
    glloadtests/GLLoadTests.cpp
    common/LoadTestSample.cpp
    common/SwipeDetector.cpp
    glloadtests/shader-based/BasisuTest.cpp
    glloadtests/shader-based/DrawTexture.cpp
    glloadtests/shader-based/GL3LoadTestSample.cpp
    glloadtests/shader-based/GL3LoadTests.cpp
    glloadtests/shader-based/TextureArray.cpp
    glloadtests/shader-based/TexturedCube.cpp
    glloadtests/shader-based/shaders.cpp
    ${SDL_SRC}
    appfwSDL/GLAppSDL.cpp
    )

target_include_directories(
    glloadtest
    PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${objKtxRead_include_dirs}
    ${PROJECT_SOURCE_DIR}/other_include
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/utils
    common
    geom
    appfwSDL
    glloadtests
    )

target_link_libraries(
    glloadtest
    ${OPENGL_LIBRARIES}
    ${SDL2_LIBRARIES}
    gtest
    ktx
    )

target_compile_definitions(
    glloadtest
    PRIVATE
    ${KTX_DEFS}
)

add_executable( vkloadtest
    $<TARGET_OBJECTS:objUtil>
    common/LoadTestSample.cpp
    common/SwipeDetector.cpp
    vkloadtests/VulkanLoadTests.cpp
    vkloadtests/VulkanLoadTestSample.cpp
    vkloadtests/InstancedSampleBase.cpp
    vkloadtests/Texture.cpp
    vkloadtests/TextureArray.cpp
    vkloadtests/TextureCubemap.cpp
    vkloadtests/TexturedCube.cpp
    vkloadtests/TextureMipmap.cpp
    ${SDL_SRC}
    appfwSDL/VulkanAppSDL/VulkanAppSDL.cpp
    appfwSDL/VulkanAppSDL/VulkanContext.cpp
    appfwSDL/VulkanAppSDL/VulkanSwapchain.cpp
    appfwSDL/VulkanAppSDL/vulkandebug.cpp
    appfwSDL/VulkanAppSDL/vulkantools.cpp
    )

target_include_directories(
    vkloadtest
    PRIVATE
    ${Vulkan_INCLUDE_DIRS}
    ${objKtxRead_include_dirs}
    ${PROJECT_SOURCE_DIR}/other_include
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/utils
    common
    geom
    appfwSDL
    appfwSDL/VulkanAppSDL
    vkloadtests
    )

target_link_libraries(
    vkloadtest
    ${Vulkan_LIBRARIES}
    ${SDL2_LIBRARIES}
    gtest
    ktx
)

if(APPLE)
target_link_libraries(
    vkloadtest
    ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libassimp.a
    ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libIrrXML.a
    ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libminizip.a
    z
    )
elseif(LINUX)
target_link_libraries(
    vkloadtest
    assimp
    z
    )
endif()

target_compile_definitions(
    vkloadtest
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
compileshader(shader_cube cube vkloadtests/shaders/cube shaders/cube )
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
file(COPY vkloadtests/models DESTINATION "${builddir}")

add_dependencies(
    vkloadtest
    spirv_shaders
)

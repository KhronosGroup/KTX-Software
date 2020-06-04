if(NOT WIN32 AND NOT IOS AND NOT EMSCRIPTEN)
    find_package(SDL2 REQUIRED)
endif()
if(NOT APPLE AND NOT EMSCRIPTEN)
    find_package(assimp REQUIRED)
endif()

if(EMSCRIPTEN)
    find_package( ZLIB )
else()
    find_package( ZLIB REQUIRED )
endif()

set(builddir "${CMAKE_BINARY_DIR}")

# By wrapping in generator expression we force multi configuration generators (like Visual Studio)
# to take the exact path and not change it.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${builddir}/$<CONFIG>>)


function( ensure_runtime_dependencies_windows target )
    # Custom copy commands to ensure all dependencies (DLLs, testimages, shaders, models) are in correct location
    # relative to executable.
    add_custom_command( TARGET ${target} POST_BUILD
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
    appfwSDL/AppBaseSDL.h
    appfwSDL/main.cpp
    common/LoadTestSample.cpp
    common/LoadTestSample.h
    common/ltexceptions.h
    common/SwipeDetector.cpp
    common/SwipeDetector.h
    common/vecmath.hpp
    geom/cube_data.h
    geom/cube.h
    geom/frame.h
    geom/quad.h

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
)

add_library( GLAppSDL OBJECT
    appfwSDL/GLAppSDL.cpp
    appfwSDL/GLAppSDL.h
)

target_include_directories(
    GLAppSDL
PUBLIC
    ${PROJECT_SOURCE_DIR}/other_include
    glloadtests
    glloadtests/utils
)

if(OPENGL_FOUND)
    target_include_directories(
        GLAppSDL
    PUBLIC
        ${OPENGL_INCLUDE_DIR}
    )
endif()


set( LOAD_TEST_COMMON_LIBS )

if(APPLE)
    set( EXE_FLAG MACOSX_BUNDLE )
    set( KTX_ICON ${PROJECT_SOURCE_DIR}/icons/mac/ktx_app.icns )
    if(IOS)
        set( LOAD_TEST_COMMON_LIBS
            ${CMAKE_SOURCE_DIR}/other_lib/ios/$<CONFIG>-iphoneos/libSDL2.a
            ${CMAKE_SOURCE_DIR}/other_lib/ios/Release-iphoneos/libassimp.a
        )
    else()
        set( LOAD_TEST_COMMON_LIBS
            ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libassimp.a
            ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libIrrXML.a
            ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libminizip.a
        )
    endif()
elseif(LINUX)
    set( LINUX_LIB_DIR Release )
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set( LINUX_LIB_DIR Debug )
    endif()
    set( LOAD_TEST_COMMON_LIBS
        ${assimp_LIBRARIES}
        ${CMAKE_SOURCE_DIR}/other_lib/linux/${LINUX_LIB_DIR}-x64/libSDL2.a
    )
elseif(WIN32)
    set( EXE_FLAG WIN32 )
    set( KTX_ICON ${PROJECT_SOURCE_DIR}/icons/win/ktx_app.ico )
    set( LOAD_TEST_COMMON_LIBS
        "${CMAKE_SOURCE_DIR}/other_lib/win/$<CONFIG>-x64/SDL2.lib"
        "${CMAKE_SOURCE_DIR}/other_lib/win/$<CONFIG>-x64/SDL2main.lib"
        "${CMAKE_SOURCE_DIR}/other_lib/win/Release-x64/assimp.lib"
    )
endif()


#Get a list of all of the files in the testimages
file(GLOB TEST_IMAGES "${PROJECT_SOURCE_DIR}/tests/testimages/*.ktx" "${PROJECT_SOURCE_DIR}/tests/testimages/*.ktx2")
#Get a list of all of the files in the models
file(GLOB_RECURSE MODELS "common/models/*")

set( LOAD_TEST_COMMON_RESOURCE_FILES
    ${KTX_ICON}
    ${TEST_IMAGES}
    ${MODELS}
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

    set(SHADER_SOURCES ${SHADER_SOURCES} ${frag2spirv_out} ${vert2spirv_out} PARENT_SCOPE)

endfunction(compileshader)

set(SHADER_SOURCES "")

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

include(glloadtests.cmake)

if(NOT EMSCRIPTEN)
    include(vkloadtests.cmake)
endif()

set_source_files_properties(${TEST_IMAGES} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/testimages")
set_source_files_properties(${SHADER_SOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/shaders")
set_source_files_properties(${MODELS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/models")
# Hack to prevent *.obj 3D files to be mistaken as linkable obj files
set_source_files_properties(${MODELS} PROPERTIES HEADER_FILE_ONLY TRUE )

# file(COPY "${PROJECT_SOURCE_DIR}/tests/testimages" DESTINATION "${builddir}")
# file(COPY common/models DESTINATION "${builddir}")

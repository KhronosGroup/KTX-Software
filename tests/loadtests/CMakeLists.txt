# Copyright 2017-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

find_package(OpenGL)

if(NOT WIN32 AND NOT IOS AND NOT EMSCRIPTEN)
    find_package(SDL2 REQUIRED)
endif()
if(LINUX)
    find_package(assimp REQUIRED)
endif()

if(NOT EMSCRIPTEN AND NOT WIN32)
    # libassimp needs zlib. On Windows we use a dll which has zlib embedded.
    # There is no offcial libassimp for Emscripten yet and we've had no
    # time to experiment so tests that use assimp are no included in the
    # Emscripten build.
    find_package( ZLIB REQUIRED )
    set( KTX_ZLIB_LIBRARIES ZLIB::ZLIB )
endif()

if(APPLE AND IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
    # Find Frameworks
    find_library(AudioToolbox_LIBRARY AudioToolbox)
    find_library(AVFoundation_LIBRARY AVFoundation)
    find_library(CoreAudio_LIBRARY CoreAudio)
    find_library(CoreBluetooth_LIBRARY CoreBluetooth)
    find_library(CoreGraphics_LIBRARY CoreGraphics)
    find_library(CoreMotion_LIBRARY CoreMotion)
    find_library(Foundation_LIBRARY Foundation)
    find_library(GameController_LIBRARY GameController)
    find_library(Metal_LIBRARY Metal)
    find_library(OpenGLES_LIBRARY OpenGLES)
    find_library(QuartzCore_LIBRARY QuartzCore)
    find_library(UIKit_LIBRARY UIKit)
endif()

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
        COMMAND ${CMAKE_COMMAND} -E copy_directory  "${CMAKE_CURRENT_BINARY_DIR}/shaders" "$<TARGET_FILE_DIR:${target}>/shaders"
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

target_compile_features(appfwSDL PUBLIC c_std_99 cxx_std_11)
if(EMSCRIPTEN)
    target_compile_options( appfwSDL PUBLIC
         "SHELL:-s USE_SDL=2"
    )
endif()

target_include_directories(
    appfwSDL
PUBLIC
    ${SDL2_INCLUDE_DIRS}
    appfwSDL
    $<TARGET_PROPERTY:ktx,INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/other_include
    ${PROJECT_SOURCE_DIR}/utils
    common
    geom
)

add_library( GLAppSDL OBJECT
    appfwSDL/GLAppSDL.cpp
    appfwSDL/GLAppSDL.h
    glloadtests/GLLoadTests.cpp
    glloadtests/GLLoadTests.h
)

if(EMSCRIPTEN)
    target_compile_options( GLAppSDL PUBLIC
        "SHELL:-s DISABLE_EXCEPTION_CATCHING=0"
        "SHELL:-s USE_SDL=2"
        "SHELL:-s USE_WEBGL2=1"
    )
endif()

target_link_libraries(GLAppSDL appfwSDL)
target_include_directories(
    GLAppSDL
PUBLIC
    ${PROJECT_SOURCE_DIR}/other_include
    appfwSDL
    common
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

include(glloadtests.cmake)

if(NOT EMSCRIPTEN)
    include(vkloadtests.cmake)
endif()

# Hack to prevent *.obj 3D files to be mistaken as linkable obj files
set_source_files_properties(${MODELS} PROPERTIES HEADER_FILE_ONLY TRUE )

if(APPLE)
    set_source_files_properties(${TEST_IMAGES} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/testimages")
    set_source_files_properties(${MODELS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/models")
elseif(LINUX)
    file(COPY "${PROJECT_SOURCE_DIR}/tests/testimages" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    file(COPY common/models DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
endif()


# Copyright 2017-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

#if(WIN32)
#    cmake_print_variables(
#        CMAKE_SYSTEM_VERSION
#        CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION
#    )
#endif()

# N.B. Do not set CMAKE_XCODE_GENERATE_SCHEME. If set, CMake will
# destroy the scheme, losing any user-made settings, when generating
# the project after the cache has been deleted. Cache deletion is
# needed whenever Xcode is updated with new SDK versions due to CMake
# hard-wiring the version in many of its variables.
#
# XCODE_SCHEME settings can be found at
#  https://cmake.org/cmake/help/latest/prop_tgt/XCODE_GENERATE_SCHEME.html
# These settings are recommended by MoltenVK Runtime User Guide.
set( CMAKE_XCODE_SCHEME_ENABLE_GPU_API_VALIDATION FALSE )
set( CMAKE_XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE DISABLED )

#set(CMAKE_FIND_DEBUG_MODE TRUE)
find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)
#set(CMAKE_FIND_DEBUG_MODE FALSE)
if(NOT EMSCRIPTEN)
    # There is no official assimp port for Emscripten, and we've had
    # no time to experiment, so tests that use assimp are omitted from
    # loadtests when building for the web.
    find_package(assimp REQUIRED CONFIG REQUIRED)
endif()

# We use our own local copy of GL headers to ensure we have glcorearb.h.
set( OPENGL_INCLUDE_DIR  ${CMAKE_SOURCE_DIR}/other_include )

if(APPLE AND IOS)
    if( CMAKE_OSX_ARCHITECTURES )
        list(LENGTH CMAKE_OSX_ARCHITECTURES archs_len)
        list(GET CMAKE_OSX_ARCHITECTURES 0 arch0)
        if( ${archs_len} GREATER 1 OR (NOT ${arch0} STREQUAL "arm64") )
            message(FATAL_ERROR "iOS loadtests only supported on arm64."
                    " Please disable KTX_FEATURE_LOADTEST_APPS"
                    " or change CMAKE_OSX_ARCHITECTURES.")
        endif()
    endif()
    # Find Frameworks
    find_library(AudioToolbox_LIBRARY AudioToolbox)
    find_library(AVFoundation_LIBRARY AVFoundation)
    find_library(CoreAudio_LIBRARY CoreAudio)
    find_library(CoreBluetooth_LIBRARY CoreBluetooth)
    find_library(CoreGraphics_LIBRARY CoreGraphics)
    find_library(CoreHaptics_LIBRARY CoreHaptics)
    find_library(CoreMotion_LIBRARY CoreMotion)
    find_library(Foundation_LIBRARY Foundation)
    find_library(GameController_LIBRARY GameController)
    find_library(IOSurface_LIBRARY IOSurface)
    if(${KTX_FEATURE_LOADTEST_APPS} MATCHES "Vulkan")
      find_library(Metal_LIBRARY Metal)
    endif()
    if(${KTX_FEATURE_LOADTEST_APPS} MATCHES "OpenGL")
      find_library(OpenGLES_LIBRARY OpenGLES)
    endif()
    find_library(QuartzCore_LIBRARY QuartzCore)
    find_library(UIKit_LIBRARY UIKit)
endif()

function( ensure_runtime_dependencies_windows target )
    # Custom copy commands to ensure all dependencies (testimages,
    # shaders, models) are in correct location relative to executable.

    if(${target} MATCHES "vkloadtests")
        add_custom_command( TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory  "${CMAKE_CURRENT_BINARY_DIR}/shaders" "$<TARGET_FILE_DIR:${target}>/resources"
            COMMENT "Copy shaders to build destination"
        )
    endif()
    add_custom_command( TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory  "${PROJECT_SOURCE_DIR}/tests/testimages" "$<TARGET_FILE_DIR:${target}>/resources"
        COMMENT "Copy testimages to build destination"
    )
    add_custom_command( TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory  "${PROJECT_SOURCE_DIR}/tests/loadtests/common/models" "$<TARGET_FILE_DIR:${target}>/resources"
        COMMENT "Copy models to build destination"
    )
endfunction()

add_library( appfwSDL STATIC
    appfwSDL/AppBaseSDL.cpp
    appfwSDL/AppBaseSDL.h
    appfwSDL/main.cpp
    common/LoadTestSample.cpp
    common/LoadTestSample.h
    common/ltexceptions.h
    common/SwipeDetector.cpp
    common/SwipeDetector.h
    common/disable_glm_warnings.h
    common/reenable_warnings.h
    common/vecmath.hpp
    geom/cube_data.h
    geom/cube.h
    geom/frame.h
    geom/quad.h
    ${PROJECT_SOURCE_DIR}/external/SDL_gesture/SDL_gesture.h
)

target_compile_features(appfwSDL PUBLIC c_std_99 cxx_std_17)
if(EMSCRIPTEN)
    target_compile_options( appfwSDL PUBLIC
         "SHELL:--use-port=sdl3"
    )
endif()

set_target_properties(appfwSDL PROPERTIES
    CXX_VISIBILITY_PRESET ${STATIC_APP_LIB_SYMBOL_VISIBILITY}
)

target_include_directories(
    appfwSDL
PUBLIC
    appfwSDL
    $<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/utils
    ${PROJECT_SOURCE_DIR}/external/SDL_gesture
    common
    geom
)

target_include_directories(
    appfwSDL
SYSTEM AFTER PUBLIC
    SDL3::Headers
    ${PROJECT_SOURCE_DIR}/other_include # For glm
)

target_link_libraries(
    appfwSDL
PUBLIC
    SDL3::SDL3
)

if(${KTX_FEATURE_LOADTEST_APPS} MATCHES "OpenGL")
  add_library( GLAppSDL STATIC
      appfwSDL/GLAppSDL.cpp
      appfwSDL/GLAppSDL.h
      glloadtests/GLLoadTests.cpp
      glloadtests/GLLoadTests.h
  )

  if(EMSCRIPTEN)
      target_compile_options( GLAppSDL PUBLIC
          "SHELL:-s DISABLE_EXCEPTION_CATCHING=0"
          "SHELL:--use-port=sdl3"
      )
  endif()

  target_link_libraries(GLAppSDL appfwSDL)

  target_include_directories(
      GLAppSDL
  PUBLIC
      $<TARGET_PROPERTY:appfwSDL,INCLUDE_DIRECTORIES>
      glloadtests
      glloadtests/utils
  )

  # The above appfwSDL include brings with it the
  # INTERFACE_SYSTEM_INCLUDE_DIRECTORIES.
  target_include_directories(
      GLAppSDL
  SYSTEM BEFORE PUBLIC
      $<IF:$<BOOL:${WIN32}>,${GLEW_INCLUDE_DIR},${OPENGL_INCLUDE_DIR}>
      #$<TARGET_PROPERTY:appfwSDL,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
  )

  set_target_properties(GLAppSDL PROPERTIES
      CXX_VISIBILITY_PRESET ${STATIC_APP_LIB_SYMBOL_VISIBILITY}
  )
endif()

set( LOAD_TEST_COMMON_LIBS )

if(APPLE)
    set( EXE_FLAG MACOSX_BUNDLE )
    set( KTX_APP_ICON_BASENAME ktx_app)
    set( KTX_DOC_ICON_BASENAME ktx_document)
    set( KTX_APP_ICON ${KTX_APP_ICON_BASENAME}.icns)
    set( KTX_DOC_ICON ${KTX_DOC_ICON_BASENAME}.icns)
    # On iOS an icon is a directory of images not a single file.
    # BASENAME is the name of the directory in the common .xcassets directory.
    # Assets are copied to the bundle in the build target setup.
    set( KTX_APP_ICON_PATH ${PROJECT_SOURCE_DIR}/icons/mac/${KTX_APP_ICON} )
    set( KTX_DOC_ICON_PATH ${PROJECT_SOURCE_DIR}/icons/mac/${KTX_DOC_ICON} )
    cmake_print_variables(assimp_LIBRARIES)
    if(IOS)
        set( LOAD_TEST_COMMON_LIBS
            assimp::assimp
        )
    else()
        set( LOAD_TEST_COMMON_LIBS
            assimp::assimp
        )
    endif()
elseif(LINUX)
    set( KTX_APP_ICON_PATH ${PROJECT_SOURCE_DIR}/icons/linux/ktx_app.svg )
    set( LOAD_TEST_COMMON_LIBS
        assimp::assimp
        #${SDL3_LIBRARIES}
        SDL3::SDL3
    )
elseif(WIN32)
    set( EXE_FLAG WIN32 )
    set( KTX_APP_ICON_PATH ${PROJECT_SOURCE_DIR}/icons/win/ktx_app.ico )
    set( LOAD_TEST_COMMON_LIBS
        assimp::assimp
    )
endif()

set( LOAD_TEST_COLLADA_MODELS
     "teapot.dae"
)
list(TRANSFORM LOAD_TEST_COLLADA_MODELS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/common/models/")
# An Xcode (or possibly CMake) update in late 2024 or early 2025 started
# compressing .dae files when copying them to bundle resources. This piece
# of undocumented magic makes it just copy. The Xcode GUI presents an option
# menu on the file with "Copy" and "Compress" in the project settings browser.
# This attribute sets it to "Copy."
set_source_files_properties( ${LOAD_TEST_COLLADA_MODELS}
    PROPERTIES
        XCODE_FILE_ATTRIBUTES "--decompress"
)

set( LOAD_TEST_OBJ_MODELS
     "cube.obj"
     "sphere.obj"
     "torusknot.obj"
)
list(TRANSFORM LOAD_TEST_OBJ_MODELS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/common/models/")
# Hack to prevent *.obj 3D files being mistaken as linkable obj files
set_source_files_properties( ${LOAD_TEST_OBJ_MODELS}
    PROPERTIES HEADER_FILE_ONLY TRUE
)
set( LOAD_TEST_COMMON_MODELS
     ${LOAD_TEST_COLLADA_MODELS}
     ${LOAD_TEST_OBJ_MODELS}
)

# A custom loadtest_models target does not work as configure fails at
# an install_target that uses this with an error "the target is not
# an executable, library or module."
#add_custom_target( loadtest_models
#    SOURCES ${LOAD_TEST_COMMON_MODELS}
#)

set( LOAD_TEST_COMMON_RESOURCE_FILES
    ${KTX_APP_ICON_PATH}
    ${KTX_DOC_ICON_PATH}
    ${LOAD_TEST_COMMON_MODELS}
)

if(LINUX)
    set( LOAD_TEST_DESTROOT "/opt" )
endif()

if(${KTX_FEATURE_LOADTEST_APPS} MATCHES "OpenGL")
    include(glloadtests.cmake)
endif()

if(${KTX_FEATURE_LOADTEST_APPS} MATCHES "Vulkan")
    include(vkloadtests.cmake)
endif()

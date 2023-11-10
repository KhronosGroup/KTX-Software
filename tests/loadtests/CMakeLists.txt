# Copyright 2017-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

include(CMakePrintHelpers)

set( fp_pref ${CMAKE_FIND_PACKAGE_PREFER_CONFIG} )
# Prefer CONFIG for vcpkg
set( CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE )
find_package(SDL2 QUIET)
find_package(assimp QUIET)
if(${KTX_FEATURE_LOADTEST_APPS} MATCHES "OpenGL")
    find_package(OpenGL)
    if(WIN32)
        find_package(GLEW QUIET)
    endif()
endif()
set( CMAKE_FIND_PACKAGE_PREFER_CONFIG ${fp_pref} )

cmake_print_variables(OPENGL_FOUND OPENGL_INCLUDE_DIR)
# We use our own local copy of GL headers to ensure we have glcorearb.h.
set( OPENGL_INCLUDE_DIR  ${CMAKE_SOURCE_DIR}/other_include )
cmake_print_variables(OPENGL_INCLUDE_DIR)
cmake_print_variables(SDL2_FOUND SDL2_INCLUDE_DIRS)
cmake_print_variables(assimp_FOUND ASSIMP_FOUND ASSIMP_INCLUDE_DIRS)


message("GLEW_FOUND = ${GLEW_FOUND}")
message("assimp_FOUND = ${assimp_FOUND}")

cmake_print_variables(CPU_ARCHITECTURE CMAKE_HOST_SYSTEM_PROCESSOR)
if(NOT (SDL2_FOUND)
    message("In SDL NOT FOUND block")
    # Use binary from repo.
    if(APPLE)
        if(IOS)
            # There is a check elsewhere that errors when not building
            # for iOS/arm64.
            set( SDL2_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/other_include )
            set( SDL2_LIBRARIES
                ${CMAKE_SOURCE_DIR}/other_lib/ios/$<CONFIG>-iphoneos/libSDL2.a
            )
        else()
            # These are universal binaries
            set( SDL2_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/other_include )
            set( SDL2_LIBRARIES
                ${CMAKE_SOURCE_DIR}/other_lib/mac/$<CONFIG>/libSDL2.dylib
            )
        endif()
    elseif(WIN32 AND ${CPU_ARCHITECTURE} STREQUAL "x86_64")
        set( SDL2_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/other_include )
        set( SDL2_LIBRARIES
            ${CMAKE_SOURCE_DIR}/other_lib/win/$<CONFIG>-x64/SDL2main.lib
            ${CMAKE_SOURCE_DIR}/other_lib/win/$<CONFIG>-x64/SDL2.lib
        )
    else()
        message( FATAL_ERROR "SDL2 not found. Can't build loadtest apps." )
    endif()
endif()
message("SDL2_INCLUDE_DIRS = ${SDL2_INCLUDE_DIRS}")
if(NOT assimp_FOUND)
    if(APPLE)
        if(IOS)
            set( ASSIMP_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/other_include )
            set( ASSIMP_LIBRARIES
                ${CMAKE_SOURCE_DIR}/other_lib/ios/Release-iphoneos/libassimp.a
            )
        else()
            # These are universal binaries
            set( ASSIMP_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/other_include )
            set( ASSIMP_LIBRARIES
                ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libassimp.a
            )
        endif()
    elseif(WIN32 AND ${CPU_ARCHITECTURE} STREQUAL "x86_64" )
        set( ASSIMP_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/other_include )
        set( ASSIMP_LIBRARIES
            ${CMAKE_SOURCE_DIR}/other_lib/win/Release-x64/assimp.lib
        )
    elseif(NOT EMSCRIPTEN)
        # There is no official libassimp for Emscripten yet and we've had
        # no time to experiment so tests that use assimp are not included
        # in the Emscripten build.
        message( FATAL_ERROR "Assimp not found. Can't build loadtest apps." )
    endif()
endif()
if(NOT EMSCRIPTEN AND NOT WIN32)
    # libassimp needs zlib. On Windows we use a dll which has zlib embedded.
    find_package( ZLIB REQUIRED )
    set( KTX_ZLIB_LIBRARIES ZLIB::ZLIB )
endif()

if(${KTX_FEATURE_LOADTEST_APPS} MATCHES "OpenGL" AND WIN32)
    if(NOT GLEW_FOUND)
message("Inside NOT GLEW_FOUND")
        if(${CPU_ARCHITECTURE} STREQUAL "x86_64")
message("Inside x86_64 NOT SDL2_FOUND")
            set( GLEW_INCLUDE_DIR  ${CMAKE_SOURCE_DIR}/other_include )
            set( GLEW_LIBRARIES
                ${CMAKE_SOURCE_DIR}/other_lib/win/Release-x64/glew32.lib
            )
        else()
            message( FATAL_ERROR "GLEW not found. Can't build gl3loadtests app." )
        endif()
    endif()
endif()
message("GLEW_INCLUDE_DIR = ${GLEW_INCLUDE_DIR}")

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

    # Custom copy commands to ensure all dependencies (DLLs, testimages,
    # shaders, models) are in correct location relative to executable.

    # When these packages are imported via vcpkg, something there is
    # adding a post-build to copy their DLLs. These are for the repo
    # included DLLs.
    #
    # It does not appear possible to use a genex to decide presence
    # of an entire command so we use the long-winded way.
    if (NOT TARGET SDL2::SDL2)
        add_custom_command( TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/other_lib/win/$<CONFIG>-x64/SDL2.dll" "$<TARGET_FILE_DIR:${target}>"
            COMMENT "Copy SDL2 DLLs to build destination"
        )
    endif()
    if (NOT TARGET assimp::assimp)
        add_custom_command( TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/other_lib/win/Release-x64/assimp.dll" "$<TARGET_FILE_DIR:${target}>"
            COMMENT "Copy assimp DLL to build destination"
        )
   endif()
    if (NOT TARGET GLEW::GLEW)
        add_custom_command( TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E;copy "${CMAKE_SOURCE_DIR}/other_lib/win/Release-x64/glew32.dll" "$<TARGET_FILE_DIR:${target}>"
            COMMENT "Copy GLEW DLL to build destination"
        )
    endif()
    if(${KTX_FEATURE_LOADTEST_APPS} MATCHES "Vulkan")
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
)

target_compile_features(appfwSDL PUBLIC c_std_99 cxx_std_11)
if(EMSCRIPTEN)
    target_compile_options( appfwSDL PUBLIC
         "SHELL:-s USE_SDL=2"
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
    common
    geom
)

target_include_directories(
    appfwSDL
SYSTEM AFTER PUBLIC
    ${SDL2_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/other_include # For glm
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
          "SHELL:-s USE_SDL=2"
      )
  endif()

  target_link_libraries(GLAppSDL appfwSDL)

#  if(OPENGL_FOUND)
#      target_include_directories(
#          GLAppSDL
#      PUBLIC
#          $<IF:$<BOOL:${WIN32}>,${GLEW_INCLUDE_DIRS},${OPENGL_INCLUDE_DIR}>
#          #${GLEW_INCLUDE_DIRS}
#      )
#  endif()

message("GLEW_INCLUDE_DIR = ${GLEW_INCLUDE_DIR}")
  target_include_directories(
      GLAppSDL
  PUBLIC
      $<TARGET_PROPERTY:appfwSDL,INCLUDE_DIRECTORIES>
      glloadtests
      glloadtests/utils
  )

# XXX The above appfwSDL include is bringing with it the
# INTERFACE_SYSTEM_INCLUDE_DIRECTORIES, maybe because they're public.
# Keep this in case needed with other generators.
  target_include_directories(
      GLAppSDL
  SYSTEM BEFORE PUBLIC
      ${GLEW_INCLUDE_DIR}
      # For glm, SDL2
 #     $<TARGET_PROPERTY:appfwSDL,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
 )

  get_target_property(afssiincludes appfwSDL INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)
  get_target_property(afsiincludes appfwSDL INTERFACE_INCLUDE_DIRECTORIES)
  get_target_property(afsincludes appfwSDL INCLUDE_DIRECTORIES)
  get_target_property(gfasiincludes GLAppSDL INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)
  get_target_property(gfaiincludes GLAppSDL INTERFACE_INCLUDE_DIRECTORIES)
  get_target_property(gfaincludes GLAppSDL INCLUDE_DIRECTORIES)

  message("afssiincludes is ${afssiincludes}")
  message("afsiincludes is ${afsiincludes}")
  message("afsincludes is ${afsincludes}")
  message("gfasiincludes is ${gfasiincludes}")
  message("gfaiincludes is ${gfaiincludes}")
  message("gfaincludes is ${gfaincludes}")
  set_target_properties(GLAppSDL PROPERTIES
      CXX_VISIBILITY_PRESET ${STATIC_APP_LIB_SYMBOL_VISIBILITY}
  )
endif()

set( LOAD_TEST_COMMON_LIBS )

if(APPLE)
    set( EXE_FLAG MACOSX_BUNDLE )
    if(IOS)
        # Do not set KTX_ICON here. On iOS the icon is not a single file.
        # Icon setup must be handled when creating a target.
        set( LOAD_TEST_COMMON_LIBS
            ${ASSIMP_LIBRARIES}
            ${SDL2_LIBRARIES}
        )
    else()
        set( KTX_ICON ${PROJECT_SOURCE_DIR}/icons/mac/ktx_app.icns )
        set( LOAD_TEST_COMMON_LIBS
            ${ASSIMP_LIBRARIES}
            ${SDL2_LIBRARIES}
            ${CMAKE_SOURCE_DIR}/other_lib/mac/Release/libminizip.a
        )
    endif()
elseif(LINUX)
    set( KTX_ICON ${PROJECT_SOURCE_DIR}/icons/linux/ktx_app.svg )
    set( LOAD_TEST_COMMON_LIBS
        ${ASSIMP_LIBRARIES}
        ${SDL2_LIBRARIES}
    )
elseif(WIN32)
    set( EXE_FLAG WIN32 )
    set( KTX_ICON ${PROJECT_SOURCE_DIR}/icons/win/ktx_app.ico )
    set( LOAD_TEST_COMMON_LIBS
        ${ASSIMP_LIBRARIES}
        ${SDL2_LIBRARIES}
    )
endif()

set( LOAD_TEST_COMMON_MODELS
     "cube.obj"
     "sphere.obj"
     "teapot.dae"
     "torusknot.obj"
)
list(TRANSFORM LOAD_TEST_COMMON_MODELS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/common/models/")
# Hack to prevent *.obj 3D files being mistaken as linkable obj files
set_source_files_properties( ${LOAD_TEST_COMMON_MODELS}
    PROPERTIES HEADER_FILE_ONLY TRUE
)

# A custom loadtest_models target does not work as configure fails at
# an install_targets that uses this with an error "the target is not
# an executable, library or module."
#add_custom_target( loadtest_models
#    SOURCES ${LOAD_TEST_COMMON_MODELS}
#)

set( LOAD_TEST_COMMON_RESOURCE_FILES
    ${KTX_ICON}
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

# Copyright 2017-2026 The Khronos Group Inc.
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
         "SHELL:-Wno-experimental"
    )

    target_link_options(
        appfwSDL
    PUBLIC
        -Wno-experimental
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
    ${CMAKE_SOURCE_DIR}/external/dfdutils
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
    set( KTX_APP_ICON_SOURCE ${PROJECT_SOURCE_DIR}/icons/mac/${KTX_APP_ICON} )
    set( KTX_DOC_ICON_SOURCE ${PROJECT_SOURCE_DIR}/icons/mac/${KTX_DOC_ICON} )
    #cmake_print_variables(assimp_LIBRARIES)
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
    set( KTX_APP_ICON_SOURCE ${PROJECT_SOURCE_DIR}/icons/linux/ktx_app.svg )
    set( LOAD_TEST_COMMON_LIBS
        assimp::assimp
        SDL3::SDL3
    )
elseif(WIN32)
    set( EXE_FLAG WIN32 )
    set( KTX_APP_ICON_SOURCE ${PROJECT_SOURCE_DIR}/icons/win/ktx_app.ico )
    set( LOAD_TEST_COMMON_LIBS
        assimp::assimp
    )
endif()

set( LOAD_TEST_COLLADA_MODELS
     "teapot.dae"
)

list(TRANSFORM LOAD_TEST_COLLADA_MODELS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/common/models/"
    OUTPUT_VARIABLE LOAD_TEST_COLLADA_MODEL_SOURCES
)
# An Xcode (or possibly CMake) update in late 2024 or early 2025 started
# compressing .dae files when copying them to bundle resources. This piece
# of undocumented magic makes it just copy. The Xcode GUI presents an option
# menu on the file with "Copy" and "Compress" in the project settings browser.
# This attribute sets it to "Copy."
set_source_files_properties( ${LOAD_TEST_COLLADA_MODEL_SOURCES}
    PROPERTIES
        XCODE_FILE_ATTRIBUTES "--decompress"
)

set( LOAD_TEST_OBJ_MODELS
     "cube.obj"
     "sphere.obj"
     "torusknot.obj"
)
list( TRANSFORM LOAD_TEST_OBJ_MODELS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/common/models/"
    OUTPUT_VARIABLE LOAD_TEST_OBJ_MODEL_SOURCES
)
# Hack to prevent *.obj 3D files being mistaken as linkable obj files
set_source_files_properties( ${LOAD_TEST_OBJ_MODEL_SOURCES}
    PROPERTIES HEADER_FILE_ONLY TRUE
)
set( LOAD_TEST_COMMON_MODEL_SOURCES
     ${LOAD_TEST_COLLADA_MODEL_SOURCES}
     ${LOAD_TEST_OBJ_MODEL_SOURCES}
)
source_group("Resources/Models" FILES ${LOAD_TEST_COMMON_MODEL_SOURCES})

set( LOAD_TEST_COMMON_RESOURCE_FILE_SOURCES
    ${KTX_APP_ICON_SOURCE}
    ${KTX_DOC_ICON_SOURCE}
    ${LOAD_TEST_COMMON_MODEL_SOURCES}
)
source_group("Resources" FILES
    ${KTX_APP_ICON_SOURCE} ${KTX_DOC_ICON_SOURCE}
)
source_group("Resources/Models" ${LOAD_TEST_COMMON_MODEL_SOURCES})

set( load_test_common_ktx2_image_sources
    astc_8x8_unorm_array_7.ktx2
    bc3_unorm_array_7.ktx2
    color_grid_uastc_zstd_5.ktx2
    color_grid_zstd_5.ktx2
    color_grid_blze.ktx2
    cubemap_goldengate_uastc_rdo_4_zstd_5.ktx2
    cubemap_yokohama_blze.ktx2
    etc2_unorm_array_7.ktx2
    Iron_Bars_001_normal_blze.ktx2
    Iron_Bars_001_normal_uastc_zstd_10.ktx2
    kodim17_blze.ktx2
    orient_down_metadata.ktx2
    orient_up_metadata.ktx2
    pattern_02_bc2.ktx2
    skybox_zstd_22.ktx2
)
list( TRANSFORM load_test_common_ktx2_image_sources
    PREPEND "${PROJECT_SOURCE_DIR}/tests/resources/ktx2/"
)

set( load_test_common_ktx_image_sources
    astc_8x8_unorm_array_7.ktx
    bc3_unorm_array_7.ktx
    etc1.ktx                     # For {es1,gl3,es3}loadtests
    etc2_rgb.ktx
    etc2_rgba1.ktx               # For {es1,gl3,es3}loadtests
    etc2_rgba8.ktx
    etc2_srgb.ktx
    etc2_srgba1.ktx              # For {gl3,es3}loadtests
    etc2_srgba8.ktx
    etc2_unorm_array_7.ktx
    hi_mark_sq.ktx               # For {es1,gl3,es3}loadtests
    metalplate_amg.ktx
    not4_r8g8b8_srgb.ktx
    orient_down_metadata.ktx
    orient_up_metadata.ktx
    pattern_02_bc2.ktx
    r8g8b8_srgb.ktx
    r8g8b8_srgb_mip.ktx         # For {es1,gl3,es3}loadtests
    r8g8b8_unorm_amg.ktx
    r8g8b8a8_srgb.ktx
)
list( TRANSFORM load_test_common_ktx_image_sources
    PREPEND "${PROJECT_SOURCE_DIR}/tests/resources/ktx/"
)
set( LOAD_TEST_COMMON_KTX12_IMAGE_SOURCES
    ${load_test_common_ktx2_image_sources}
    ${load_test_common_ktx_image_sources}
)
source_group("Resources/KTX Images" FILES ${LOAD_TEST_COMMON_KTX12_IMAGE_SOURCES})

unset( load_test_common_ktx2_image_sources )
unset( load_test_common_ktx_image_sources )

# A custom loadtest_models target does not work as configure fails at
# an install_target that uses this with an error "the target is not
# an executable, library or module."
#add_custom_target( loadtest_models
#    SOURCES ${LOAD_TEST_COMMON_MODELS}
#)

if(NOT APPLE)
    # Arrange to copy resources next to the executable for ease of use
    # during debugging and testing. Not needed on Apple because resources
    # are copied into the built bundles.
    #
    # We cannot use just custom commands because the resources directory
    # being created, the models and some of the KTX files are shared by
    # both gl3loadtests and vkloadtests so when building in parallel the
    # instances of the rule may conflict giving rise to races. To avoid
    # this use custom targets that become dependencies of the apps which
    # imposes a strict runtime order on the builds, avoiding races.
    #
    # IMPORTANT NOTE
    # These targets and copies play no part in eventual installation of the
    # application and are only for convenience during development. Therefore 
    # the above mentioned error does not occur.

    set(load_test_common_resources_copied
        ${KTX_APP_ICON}  # TODO: Is this needed on Windows?
        ${KTX_DOC_ICON}
        ${LOAD_TEST_COLLADA_MODELS}
        ${LOAD_TEST_OBJ_MODELS}
    )
    # The resources directory is created in the build tree next to the
    # executable's directory. This sibling relationship will be maintained
    # when the app and its resources are installed on a system.
    set(BUILDTIME_RESOURCES_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../resources)
    list(TRANSFORM load_test_common_resources_copied PREPEND ${BUILDTIME_RESOURCES_DIR}/)

    add_custom_command(
        OUTPUT ${BUILDTIME_RESOURCES_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILDTIME_RESOURCES_DIR}"
        COMMENT "Make resources directory"
        VERBATIM
    )
    add_custom_target(
        buildtime_resources_dir
        DEPENDS ${BUILDTIME_RESOURCES_DIR}
    )
    add_custom_command(
        OUTPUT ${load_test_common_resources_copied}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILDTIME_RESOURCES_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LOAD_TEST_COMMON_RESOURCE_FILE_SOURCES} ${BUILDTIME_RESOURCES_DIR}
        COMMENT "Copy common resources (icons and models to build destination"
        DEPENDS ${LOAD_TEST_COMMON_RESOURCE_FILE_SOURCES}
        VERBATIM
    )
    add_custom_target(
        copied_models
        DEPENDS ${load_test_common_resources_copied}
    )
    unset(load_test_common_resources_copied)
    list(TRANSFORM LOAD_TEST_COMMON_KTX12_IMAGE_SOURCES REPLACE "^[a-zA-Z0-9:/<>].*/ktx(2?)" ${BUILDTIME_RESOURCES_DIR}
        OUTPUT_VARIABLE common_ktx_image_copies
    )
    add_custom_command(
        OUTPUT ${common_ktx_image_copies}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LOAD_TEST_COMMON_KTX12_IMAGE_SOURCES} ${BUILDTIME_RESOURCES_DIR}
        COMMENT "Copy common KTX and KTX2 images to build destination"
        DEPENDS ${LOAD_TEST_COMMON_KTX12_IMAGE_SOURCES}
        VERBATIM
    )
    add_custom_target(
        copied_common_ktx_images
        DEPENDS ${common_ktx_image_copies}  
    )
    # To ensure the resources directory is created before copying KTX images.
    add_dependencies( copied_common_ktx_images copied_models )
    unset( common_ktx_image_copies )
endif()

if(LINUX)
    set( LOAD_TEST_DESTROOT "/opt" )
endif()

if(${KTX_FEATURE_LOADTEST_APPS} MATCHES "OpenGL")
    include(glloadtests.cmake)
endif()

if(${KTX_FEATURE_LOADTEST_APPS} MATCHES "Vulkan")
    include(vkloadtests.cmake)
endif()

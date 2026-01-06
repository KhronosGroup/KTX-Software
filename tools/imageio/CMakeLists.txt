# Copyright 2022 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

include("../../external/lodepng.cmake")
include("../../external/basisu.cmake")

set( PLUGIN_SOURCES
    exr.imageio/exrinput.cc
    jpg.imageio/jpginput.cc
    npbm.imageio/npbminput.cc
    png.imageio/pnginput.cc
    png.imageio/pngoutput.cc
)
set( PLUGIN_HEADERS
)

set( EXTERNAL
    "${PROJECT_SOURCE_DIR}/external"
)

set( EXR_PLUGIN_EXTERNAL_SOURCES
)
set( EXR_PLUGIN_EXTERNAL_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/external/tinyexr.h
)
set( JPG_PLUGIN_EXTERNAL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/external/jpgd.cpp
)
set( JPG_PLUGIN_EXTERNAL_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/external/jpgd.h
)

add_library( imageio STATIC
    formatdesc.h
    image.hpp
    imagecodec.hpp
    imagespan.hpp
    imageinput.cc
    imageio.cc
    imageio.h
    imageio_utility.h
    imageoutput.cc
    ${PLUGIN_HEADERS}
    ${PLUGIN_SOURCES}
    ${EXR_PLUGIN_EXTERNAL_HEADERS}
    ${EXR_PLUGIN_EXTERNAL_SOURCES}
    ${JPG_PLUGIN_EXTERNAL_HEADERS}
    ${JPG_PLUGIN_EXTERNAL_SOURCES}
)

source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX "Source Files" FILES ${PLUGIN_SOURCES})
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX "Header Files" FILES ${PLUGIN_HEADERS})
source_group("Header Files/exr.imageio" FILES
    ${EXR_PLUGIN_EXTERNAL_SOURCES}
)
source_group("Header Files/exr.imageio" FILES
    ${EXR_PLUGIN_EXTERNAL_HEADERS}
)
source_group("Source Files/jpg.imageio" FILES
    ${JPG_PLUGIN_EXTERNAL_SOURCES}
)
source_group("Header Files/jpg.imageio" FILES
    ${JPG_PLUGIN_EXTERNAL_HEADERS}
)
source_group("Source Files/png.imageio" FILES
    ${PNG_PLUGIN_EXTERNAL_SOURCES}
)
source_group("Header Files/png.imageio" FILES
    ${PNG_PLUGIN_EXTERNAL_HEADERS}
)

target_include_directories(
    imageio
PUBLIC
    .
PRIVATE
    $<TARGET_PROPERTY:objUtil,INTERFACE_INCLUDE_DIRECTORIES>
)

target_include_directories(
    imageio
    SYSTEM
PUBLIC
    ${PROJECT_SOURCE_DIR}/other_include
PRIVATE
    "${PROJECT_SOURCE_DIR}/external/dfdutils"
    "${CMAKE_CURRENT_SOURCE_DIR}/external"
)

target_compile_definitions(
    imageio
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
)

target_compile_features(imageio PUBLIC cxx_std_17)

target_link_libraries(imageio fmt::fmt basisu::basisu_encoder lodepng::lodepng)

set_target_properties(imageio PROPERTIES
    CXX_VISIBILITY_PRESET ${STATIC_APP_LIB_SYMBOL_VISIBILITY}
)


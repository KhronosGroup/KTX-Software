# Copyright 2022 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

set( PLUGIN_SOURCES
    exr.imageio/exrinput.cc
    jpg.imageio/jpginput.cc
    npbm.imageio/npbminput.cc
    png.imageio/lodepng.cc
    png.imageio/pnginput.cc
    png.imageio/pngoutput.cc
)
set( PLUGIN_HEADERS
    png.imageio/lodepng.h
)

add_library( imageio STATIC
    formatdesc.h
    image.hpp
    imageinput.cc
    imageio.cc
    imageio.h
    imageio_utility.h
    imageoutput.cc
    ${PROJECT_SOURCE_DIR}/lib/astc-encoder/Source/tinyexr.h
    ${PROJECT_SOURCE_DIR}/lib/basisu/encoder/jpgd.cpp
    ${PROJECT_SOURCE_DIR}/lib/basisu/encoder/jpgd.h
    ${PLUGIN_HEADERS}
    ${PLUGIN_SOURCES}
)

source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX "Source Files" FILES ${PLUGIN_SOURCES})
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" PREFIX "Header Files" FILES ${PLUGIN_HEADERS})
source_group("Source Files/jpg.imageio" FILES
    ${PROJECT_SOURCE_DIR}/lib/basisu/encoder/jpgd.cpp
)
source_group("Header Files/jpg.imageio" FILES
    ${PROJECT_SOURCE_DIR}/lib/basisu/encoder/jpgd.h
)
source_group("Header Files/exr.imageio" FILES
    ${PROJECT_SOURCE_DIR}/lib/astc-encoder/Source/tinyexr.h
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
    ${PROJECT_SOURCE_DIR}/lib/astc-encoder/Source
    ${PROJECT_SOURCE_DIR}/lib/basisu
    ${PROJECT_SOURCE_DIR}/lib/dfdutils
)

target_compile_definitions(
    imageio
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
)

set_target_properties(imageio PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_VISIBILITY_PRESET ${STATIC_APP_LIB_SYMBOL_VISIBILITY}
)

target_link_libraries(imageio fmt::fmt)

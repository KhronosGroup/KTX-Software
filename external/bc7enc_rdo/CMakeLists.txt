# Copyright 2019-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)
project(bc7enc_rdo)

add_library(rgbcx
    ${CMAKE_CURRENT_LIST_DIR}/rgbcx.cpp     # BC1, BC3, BC4 and BC5 encoders/decoders
)

add_library(ert
    ${CMAKE_CURRENT_LIST_DIR}/ert.cpp       # RDO post processing step and some utils
)

target_compile_features(rgbcx PUBLIC cxx_std_17)
target_compile_features(ert PUBLIC cxx_std_17)

# original bc7enc_rdo disables strict aliasing (see https://github.com/richgel999/bc7enc_rdo/blob/master/CMakeLists.txt)
# basisu also emphasises on compiling with '-fno-strict-aliasing'
if (NOT MSVC)
  target_compile_options(rgbcx PRIVATE
    -fno-strict-aliasing
    # Without this, GCC 15.2.0 outputs "error: writing 1 byte into a region of size 0" This is a known GCC false positive.
    # See GCC bug report here: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=106757
    $<$<CXX_COMPILER_ID:GNU>:-Wno-stringop-overflow>
  )
  target_compile_options(ert PRIVATE -fno-strict-aliasing)
endif()

target_include_directories(rgbcx
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/
)

target_include_directories(ert
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/
)

add_library(bc7enc_rdo::rgbcx ALIAS rgbcx)
add_library(bc7enc_rdo::ert ALIAS ert)

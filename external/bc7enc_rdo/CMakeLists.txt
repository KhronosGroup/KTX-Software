# Copyright 2019-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

add_library(bc7enc_rdo)
target_compile_features(bc7enc_rdo PUBLIC cxx_std_17)

# original bc7enc_rdo disables strict aliasing (see https://github.com/richgel999/bc7enc_rdo/blob/master/CMakeLists.txt)
# basisu also emphasises on compiling with '-fno-strict-aliasing'
if (NOT MSVC)
  target_compile_options(bc7enc_rdo PRIVATE -fno-strict-aliasing)
endif()

target_sources(bc7enc_rdo
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/rgbcx.cpp     # BC1, BC3, BC4 and BC5 encoders/decoders
    ${CMAKE_CURRENT_LIST_DIR}/ert.cpp       # RDO post processing step and some utils
)

target_include_directories(bc7enc_rdo
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/
)

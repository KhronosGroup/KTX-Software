# Copyright 2017-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

add_library(obj_basisu_cbind OBJECT
    inc/basisu_c_binding.h
    src/basisu_c_binding.cpp
)

target_compile_features(obj_basisu_cbind PUBLIC c_std_99 cxx_std_11)

target_include_directories(
    obj_basisu_cbind
PUBLIC
    inc
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/utils
)

target_compile_definitions(
    obj_basisu_cbind
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
)

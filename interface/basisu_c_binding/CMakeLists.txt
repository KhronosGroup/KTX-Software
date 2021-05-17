# Copyright 2017-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

add_library(obj_basisu_cbind STATIC
    inc/basisu_c_binding.h
    src/basisu_c_binding.cpp
)

target_compile_features(obj_basisu_cbind PUBLIC c_std_99 cxx_std_11)

target_include_directories(
    obj_basisu_cbind
PUBLIC
    inc
PRIVATE
    $<TARGET_PROPERTY:ktx,INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/utils
)

target_compile_definitions(
    obj_basisu_cbind
PUBLIC
    KTX_BASISU_C_BINDINGS
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
    # We're only using this to read .basis files.
    BASISD_SUPPORT_KTX2_ZSTD=0
    BASISD_SUPPORT_KTX2=0
)

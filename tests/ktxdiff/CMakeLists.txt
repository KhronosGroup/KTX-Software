# Copyright 2022-2023 The Khronos Group Inc.
# Copyright 2022-2023 RasterGrid Kft.
# SPDX-License-Identifier: Apache-2.0


add_executable(ktxdiff
        ktxdiff_main.cpp
)

target_compile_features(
    ktxdiff
PUBLIC
    cxx_std_17
)

target_include_directories(
    ktxdiff
PRIVATE
    .
    $<TARGET_PROPERTY:ktx,INCLUDE_DIRECTORIES>
)

target_include_directories(
    ktxdiff
    SYSTEM
PRIVATE
    $<TARGET_PROPERTY:objUtil,INTERFACE_INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/lib/src
    ${PROJECT_SOURCE_DIR}/other_include
)

target_link_libraries(
    ktxdiff
PRIVATE
    ktx
    ${ASTCENC_LIB_TARGET}
    fmt::fmt
    objUtil
)

target_compile_definitions(
    ktxdiff
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
)

set_test_properties(ktxdiff)

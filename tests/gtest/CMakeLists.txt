# Copyright 2017-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

add_library( gtest STATIC
    src/gtest-all.cc
    src/gtest-internal-inl.h
    src/gtest_main.cc
)

target_include_directories(
    gtest
PUBLIC
    include
    .
)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    set_source_files_properties(
        src/gtest-all.cc
        PROPERTIES COMPILE_OPTIONS "-Wno-missing-field-initializers"
    )
endif()


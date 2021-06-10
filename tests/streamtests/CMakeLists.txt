# Copyright 2017-2021 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

add_executable( streamtests
    streamtests.cc
)

target_include_directories(
    streamtests
PRIVATE
    $<TARGET_PROPERTY:ktx,INCLUDE_DIRECTORIES>
    ${CMAKE_CURRENT_SOURCE_DIR}/../../other_include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib  # Just for gl_format.h
)

target_link_libraries(
    streamtests
    gtest
    ktx
    ${CMAKE_THREAD_LIBS_INIT}
)

target_compile_definitions(
    streamtests
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
)

gtest_discover_tests( streamtests
    TEST_PREFIX streamtest
    EXTRA_ARGS "${PROJECT_SOURCE_DIR}/tests/testimages/"
)
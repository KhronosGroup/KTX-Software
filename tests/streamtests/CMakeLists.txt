# Copyright 2017-2021 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

add_executable( streamtests
    streamtests.cc
)
set_test_properties(streamtests)
set_code_sign(streamtests)

target_include_directories(
    streamtests
PRIVATE
    $<TARGET_PROPERTY:ktx,INCLUDE_DIRECTORIES>
    ${CMAKE_CURRENT_SOURCE_DIR}/../../other_include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib  # Just for gl_format.h
)

set_target_properties(
    streamtests PROPERTIES
    # Force C++14 (for std::make_unique on AppleClang and Linux GCC)
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED YES
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
    TEST_PREFIX streamtest.
    # With the 5s default we get periodic timeouts on Travis & GitHub CI.
    DISCOVERY_TIMEOUT 20
    EXTRA_ARGS "${PROJECT_SOURCE_DIR}/tests/testimages/"
)

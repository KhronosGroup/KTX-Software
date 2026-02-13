# Copyright 2017-2021 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.22)

set( KTX_THREADTESTS_RUNS
    1 # Even running the test 100 times rarely reveals a race.
    CACHE STRING
    "Number of times to run the threadtests; higher improves chances of catching race conditions"
)
mark_as_advanced(FORCE KTX_THREADTESTS_RUNS)

add_executable( threadtests
    threadtests.cc
    ../tests.cmake
)
set_test_properties(threadtests)
set_code_sign(threadtests)

target_compile_features( threadtests PUBLIC cxx_std_20 )

target_include_directories(
    threadtests
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:objUtil,INTERFACE_INCLUDE_DIRECTORIES>
    ${CMAKE_SOURCE_DIR}/external/dfdutils
)

target_link_libraries(
    threadtests
    gtest
    ktx
    "$<${is_stdformat_unsupported}:fmt::fmt>"
    ${CMAKE_THREAD_LIBS_INIT}
)

target_compile_definitions(
    threadtests
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
)

# Use a script to launch the test multiple times as running it once
# is highly unlikely to reveal a race. The thread unsafe parts are the
# one time initialization of global tables. Since initialization is a
# one time thing protected by global variables, relaunching the test
# program is the only way to repeat the test.
add_test(
    NAME threadtests
    COMMAND "${CMAKE_COMMAND}" -P ${CMAKE_SOURCE_DIR}/cmake/launch.cmake ${KTX_THREADTESTS_RUNS} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/threadtests ${CMAKE_SOURCE_DIR}/tests/resources
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

#gtest_discover_tests( threadtests
#    TEST_PREFIX threadtest.
#    # With the 5s default we get periodic timeouts on Travis & GitHub CI.
#    DISCOVERY_TIMEOUT 20
#    EXTRA_ARGS "${PROJECT_SOURCE_DIR}/tests/resources/"
#)

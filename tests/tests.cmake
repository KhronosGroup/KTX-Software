# Copyright 2020-2021 Andreas Atteneder
# SPDX-License-Identifier: Apache-2.0

# gtest based unit-tests

include(GoogleTest)

add_subdirectory(gtest)
find_package(Threads)

# This setting is critical when using the Xcode generator on
# an Apple Silicon Mac. On Apple Silicon all executables must
# be signed. The Xcode generator sets up signing as a post
# build operation. Default test discovery mode, POST_BUILD,
# is also a post build operation. It runs before the signing
# post build so the test executable won't run on Apple
# Silicon when instantiated to discover the tests. This setting
# delays test discovery until a test is run by which time the
# test executable will be signed.
if (CMAKE_GENERATOR STREQUAL Xcode)
  set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)
endif()

enable_testing()

add_subdirectory(transcodetests)
add_subdirectory(streamtests)

add_executable( unittests
    unittests/unittests.cc
    unittests/image_unittests.cc
    unittests/wthelper.h
    tests.cmake
)
set_test_properties(unittests)
set_code_sign(unittests)

target_include_directories(
    unittests
PRIVATE
    $<TARGET_PROPERTY:ktx,INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/tools
    loadtests/common
)

target_link_libraries(
    unittests
    gtest
    ktx
    ${CMAKE_THREAD_LIBS_INIT}
)

add_executable( texturetests
    texturetests/texturetests.cc
    unittests/wthelper.h
)
set_test_properties(texturetests)
set_code_sign(texturetests)

target_include_directories(
    texturetests
PRIVATE
    $<TARGET_PROPERTY:ktx,INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/other_include
    ${PROJECT_SOURCE_DIR}/lib
    unittests
)

target_link_libraries(
    texturetests
    gtest
    ktx
    ${CMAKE_THREAD_LIBS_INIT}
)

gtest_discover_tests(unittests
    TEST_PREFIX unittest
    # With the 5s default we get periodic timeouts on Travis & GitHub CI.
    DISCOVERY_TIMEOUT 20
)
gtest_discover_tests(texturetests
    TEST_PREFIX texturetest
    DISCOVERY_TIMEOUT 20
)

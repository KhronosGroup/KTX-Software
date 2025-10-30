# Copyright 2020-2021 Andreas Atteneder
# SPDX-License-Identifier: Apache-2.0

# gtest based unit-tests

include(GoogleTest)

add_subdirectory(gtest)
find_package(Threads)

# This setting is critical when cross compiling and on Apple
# Silicon Macs. By default (MODE POST_BUILD) test discovery
# is done as a post build operation which runs the test
# executable to discover the list of tests as soon as it is
# built. This unsurprisingly fails when cross compiling as
# tests built for the target won't run on the host. It fails
# on Apple Silicon as all executables must be signed. Because
# most generators (Xcode certainly) set up signing as a post
# build operation which runs after the test discovery post
# build the test executable will not be signed.
#
# This setting delays test discovery until a test is run by
# which time the test executable will be signed and will most
# likely be on the intended target. For simplicity use this
# setting on all platforms.
set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)

enable_testing()

add_subdirectory(transcodetests)
add_subdirectory(streamtests)

add_executable( unittests
    "${PROJECT_SOURCE_DIR}/external/dfdutils/dfd2vk.c"
    unittests/image_unittests.cc
    unittests/test_fragment_uri.cc
    unittests/test_string_to_vkformat.cc
    unittests/unittests.cc
    unittests/vkformat_list.inl
    unittests/wthelper.h
    tests.cmake
)
set_test_properties(unittests)
set_code_sign(unittests)

target_compile_features(
    unittests
PUBLIC
    cxx_std_17
)

target_include_directories(
    unittests
PRIVATE
    $<TARGET_PROPERTY:ktx,INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/lib/src
    ${PROJECT_SOURCE_DIR}/tools
    ${PROJECT_SOURCE_DIR}/tools/imageio
    loadtests/common
)

target_include_directories(
    unittests
    SYSTEM
PRIVATE
    ${PROJECT_SOURCE_DIR}/other_include
)

target_link_libraries(
    unittests
    gtest
    ktx
    fmt::fmt
    ${CMAKE_THREAD_LIBS_INIT}
)

if(NOT DEFINED KTX_DIFF_PATH)
    message(FATAL_ERROR "KTX_DIFF_PATH not defined. Needed by texturetests.")
endif()

add_executable( texturetests
    texturetests/texturetests.cc
    unittests/wthelper.h
    tests.cmake
)
set_test_properties(texturetests)
set_code_sign(texturetests)

target_compile_features(
    texturetests
PUBLIC
    cxx_std_17
)

target_include_directories(
    texturetests
PRIVATE
    $<TARGET_PROPERTY:ktx,INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/other_include
    ${PROJECT_SOURCE_DIR}/lib/src
    unittests
)

target_link_libraries(
    texturetests
    gtest
    ktx
    ${CMAKE_THREAD_LIBS_INIT}
)

gtest_discover_tests(unittests
    TEST_PREFIX unittest.
    # With the 5s default we get periodic timeouts on Travis & GitHub CI.
    DISCOVERY_TIMEOUT 20
)

gtest_discover_tests(texturetests
    TEST_PREFIX texturetest.
    DISCOVERY_TIMEOUT 20
    EXTRA_ARGS "${PROJECT_SOURCE_DIR}/tests/testimages/" ${KTX_DIFF_PATH}
)

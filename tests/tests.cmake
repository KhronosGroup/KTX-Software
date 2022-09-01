# Copyright 2020-2021 Andreas Atteneder
# SPDX-License-Identifier: Apache-2.0

# gtest based unit-tests

include(GoogleTest)

add_subdirectory(gtest)
find_package(Threads)

enable_testing()

add_subdirectory(transcodetests)
add_subdirectory(streamtests)

add_executable( unittests
    unittests/unittests.cc
    unittests/image_unittests.cc
    unittests/wthelper.h
    tests.cmake
)

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
    DISCOVERY_TIMEOUT 15
)
gtest_discover_tests(texturetests
    TEST_PREFIX texturetest
    DISCOVERY_TIMEOUT 15
)

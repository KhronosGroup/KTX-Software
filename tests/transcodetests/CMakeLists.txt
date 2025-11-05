# Copyright 2017-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

add_executable( transcodetests
    transcodetests.cc
    ../tests.cmake
)
set_test_properties(transcodetests)
set_code_sign(transcodetests)

target_include_directories(
    transcodetests
PRIVATE
    ${PROJECT_SOURCE_DIR}/lib/src
)

target_link_libraries(
    transcodetests
    gtest
    ktx
    obj_basisu_cbind
    basisu_encoder
    ${CMAKE_THREAD_LIBS_INIT}
)

target_compile_definitions(
    transcodetests
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
)

target_compile_features(transcodetests PUBLIC cxx_std_11)

gtest_discover_tests( transcodetests
    TEST_PREFIX transcodetest.
    # With the 5s default we get periodic timeouts on Travis & GitHub CI.
    DISCOVERY_TIMEOUT 15
    EXTRA_ARGS "${PROJECT_SOURCE_DIR}/tests/testimages/"
)

if(WIN32)
    target_compile_definitions(
        transcodetests
    PRIVATE
        $<$<STREQUAL:"${BASISU_CBIND_LIBTYPE}","STATIC">:KHRONOS_STATIC>
    )
endif()

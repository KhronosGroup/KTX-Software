# Copyright 2017-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

add_executable( transcodetests
    level_processor_tests.cc
    streaming_construction_tests.cc
    transcodetests.cc
    ../tests.cmake
)
set_test_properties(transcodetests)
set_code_sign(transcodetests)

target_include_directories(
    transcodetests
PRIVATE
    ${PROJECT_SOURCE_DIR}/lib/src
    $<TARGET_PROPERTY:objUtil,INTERFACE_INCLUDE_DIRECTORIES>
    ${CMAKE_SOURCE_DIR}/external/dfdutils
)

target_link_libraries(
    transcodetests
    GTest::gtest
    ktx
    "$<${is_stdformat_unsupported}:fmt::fmt>"
    obj_basisu_cbind
    basisu_encoder
    ${CMAKE_THREAD_LIBS_INIT}
)

target_compile_definitions(
    transcodetests
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
)

target_compile_features( transcodetests PUBLIC cxx_std_20 )

gtest_discover_tests( transcodetests
    TEST_PREFIX transcodetest.
    # With the 5s default we get periodic timeouts on Travis & GitHub CI.
    DISCOVERY_TIMEOUT 15
    EXTRA_ARGS "${PROJECT_SOURCE_DIR}/tests/resources/"
)

if(WIN32)
    # The BasisU C binding is always built as a static library so its
    # declarations must not be marked dllimport. Do that with KTX_BASISU_API,
    # not KHRONOS_STATIC: the latter also empties KTX_API in ktx.h, leaving
    # imported data such as KTX_ETC1S_DEFAULT_COMPRESSION_LEVEL unresolvable
    # when linking the shared libktx. When libktx itself is static it defines
    # KHRONOS_STATIC publicly and ktx.h is correct without our help.
    target_compile_definitions(
        transcodetests
    PRIVATE
        $<$<STREQUAL:"${BASISU_CBIND_LIBTYPE}","STATIC">:KTX_BASISU_API=>
    )
endif()

# Copyright 2017-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

add_executable( transcodetests
    transcodetests.cc
)
set_test_properties(transcodetests)
set_code_sign(transcodetests)

target_include_directories(
    transcodetests
PRIVATE
    $<TARGET_PROPERTY:ktx,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:obj_basisu_cbind,INTERFACE_INCLUDE_DIRECTORIES>
    ${CMAKE_CURRENT_SOURCE_DIR}/../../other_include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib
)

target_link_libraries(
    transcodetests
    gtest
    ktx
    obj_basisu_cbind
    ${CMAKE_THREAD_LIBS_INIT}
)

target_compile_definitions(
    transcodetests
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
    $<TARGET_PROPERTY:obj_basisu_cbind,INTERFACE_COMPILE_DEFINITIONS>
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

    # Retrieve and set warning settings for the 2 files from the ktx target.
    # See comment in ../../CMakeLists.txt around line 625.
    if (transcoder_options)
        set_source_files_properties(
            ${PROJECT_SOURCE_DIR}/lib/basisu/transcoder/basisu_transcoder.cpp
            PROPERTIES COMPILE_OPTIONS "${transcoder_options}"
        )
    endif()
    if (zstd_options)
        set_source_files_properties(
            ${PROJECT_SOURCE_DIR}/lib/basisu/zstd/zstd.c
            PROPERTIES COMPILE_OPTIONS "${zstd_options}"
        )
    endif()

    # The Windows ktx.dll does not export needed symbols. This is a workaround to get it linking.
    add_library(obj_tmp_basisu OBJECT
        $<$<NOT:$<BOOL:${KTX_FEATURE_STATIC_LIBRARY}>>:${PROJECT_SOURCE_DIR}/lib/basisu/transcoder/basisu_transcoder.cpp>
        ${PROJECT_SOURCE_DIR}/lib/basisu/zstd/zstd.c
        )
    target_compile_definitions(
        obj_tmp_basisu
        PUBLIC # only for basisu_c_binding.
            BASISU_NO_ITERATOR_DEBUG_LEVEL
        )
    target_link_libraries(transcodetests obj_tmp_basisu)
endif()

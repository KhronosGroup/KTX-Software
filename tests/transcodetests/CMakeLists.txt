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

    # Retrieve and set warning settings for the 2 files from the ktx_read
    # target.
    # TODO: After BasisU update check if this is still necessary to compile
    # without warnings
    get_source_file_property(
        transcoder_options
        "${PROJECT_SOURCE_DIR}/external/basisu/transcoder/basisu_transcoder.cpp"
        TARGET_DIRECTORY ktx_read
        COMPILE_OPTIONS
    )
    if (transcoder_options)
        set_source_files_properties(
           "${PROJECT_SOURCE_DIR}/external/basisu/transcoder/basisu_transcoder.cpp"
           PROPERTIES COMPILE_OPTIONS "${transcoder_options}"
        )
    endif()
    if(KTX_FEATURE_EMBEDDED_ZSTD)
        get_source_file_property(
            zstd_options
            "${PROJECT_SOURCE_DIR}/external/basisu/zstd/zstd.c"
            TARGET_DIRECTORY ktx_read
            COMPILE_OPTIONS
        )
        if (zstd_options)
            set_source_files_properties(
                ${PROJECT_SOURCE_DIR}/external/basisu/zstd/zstd.c
                PROPERTIES COMPILE_OPTIONS "${zstd_options}"
            )
        endif()
    endif()

    # The Windows ktx.dll does not export needed symbols. This is a workaround to get it linking.
    add_library(obj_tmp_basisu OBJECT
        $<$<BOOL:${BUILD_SHARED_LIBS}>:${PROJECT_SOURCE_DIR}/external/basisu/transcoder/basisu_transcoder.cpp>
    )

    if(KTX_FEATURE_EMBEDDED_ZSTD)
        target_sources(obj_tmp_basisu PRIVATE "${PROJECT_SOURCE_DIR}/external/basisu/zstd/zstd.c")
    endif()
        
    target_compile_definitions(
        obj_tmp_basisu
        PUBLIC # only for basisu_c_binding.
            BASISU_NO_ITERATOR_DEBUG_LEVEL
            # We're only using this to read .basis files.
            BASISD_SUPPORT_KTX2_ZSTD=0
            BASISD_SUPPORT_KTX2=0
        )
    target_link_libraries(transcodetests obj_tmp_basisu)
endif()

# Copyright 2017-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

function(set_test_properties test_target)
    # See comments in set_tools_properties() in ../tools/CMakeLists.txt.
    if(APPLE)
        set_target_properties(${test_target} PROPERTIES
            INSTALL_RPATH "@executable_path;@executable_path/../${CMAKE_INSTALL_LIBDIR}"
        )
    elseif(LINUX)
        set_target_properties(${test_target} PROPERTIES
            INSTALL_RPATH "\$ORIGIN;\$ORIGIN/../${CMAKE_INSTALL_LIBDIR}"
        )
    endif()
endfunction()

if(KTX_FEATURE_LOADTEST_APPS)
    # Test apps that load and display various KTX textures
    add_subdirectory(loadtests)
endif()

# gtest based unit-tests
if(KTX_FEATURE_TESTS)
    # Used by texturetests as well as CTS.
    add_subdirectory(ktxdiff)
    set(KTX_DIFF_PATH $<TARGET_FILE:ktxdiff>)
    include(tests.cmake)
endif()

# tools tests
if(KTX_FEATURE_TOOLS)
    include( ktx2check-tests.cmake )
    include( ktx2ktx2-tests.cmake )
    include( ktxsc-tests.cmake )
    include( toktx-tests.cmake )

    # ktx cli tool tests
    if(KTX_FEATURE_TOOLS_CTS)
        set(KTX_TOOLS_PATH $<TARGET_FILE:ktxtools>)
        if(NOT KTX_DIFF_PATH)
            set(KTX_DIFF_PATH $<TARGET_FILE:ktxdiff>)
        endif()
        add_subdirectory(cts/clitests)
    endif()
endif()

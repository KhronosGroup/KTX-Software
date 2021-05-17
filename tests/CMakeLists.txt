# Copyright 2017-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

if(KTX_FEATURE_LOADTEST_APPS)
    # Test apps that load and display various KTX textures
    if(APPLE AND CPU_ARCHITECTURE STREQUAL arm64)
        message(WARNING "Building load test apps is not supported on Apple Silicon as host platform, so they are skipped")
    else()
        add_subdirectory(loadtests)
    endif()
endif()

# gtest based unit-tests
if(KTX_FEATURE_TESTS AND NOT IOS AND NOT ANDROID AND NOT EMSCRIPTEN)
    include(tests.cmake)
endif()

# tools tests
if(KTX_FEATURE_TOOLS)
    include( ktx2check-tests.cmake )
    include( toktx-tests.cmake )
endif()

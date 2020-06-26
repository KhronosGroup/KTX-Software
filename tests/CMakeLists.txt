if(KTX_FEATURE_LOADTEST_APPS)
    # Test apps that load and display various KTX textures
    add_subdirectory(loadtests)
endif()

# gtest based unit-tests
if(NOT IOS AND NOT ANDROID AND NOT EMSCRIPTEN)
    include(tests.cmake)
endif()

# tools tests
if(KTX_FEATURE_TOOLS)
    include( ktx2check-tests.cmake )
    include( toktx-tests.cmake )
endif()

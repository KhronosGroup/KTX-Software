if(EMSCRIPTEN)
    message(STATUS "Tests don't work with emscripten -> skipping tests")
    return()
endif()

if(KTX_FEATURE_LOADTEST_APPS)
    # Test apps that load and display various KTX textures
    add_subdirectory(loadtests)
endif()

# gtest based unit-tests
if(NOT IOS)
    include(tests.cmake)
endif()

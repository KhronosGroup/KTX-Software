# gtest based unit-tests

add_subdirectory(gtest)
find_package(Threads)

enable_testing()

add_subdirectory(transcodetests)

add_executable( unittests
    unittests/unittests.cc
    unittests/wthelper.h
)

target_include_directories(
    unittests
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/lib
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
    $<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES>
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

add_test( NAME unittests COMMAND unittests )
add_test( NAME texturetests COMMAND texturetests )

if(WIN32)
    set_tests_properties(
        unittests
        texturetests
    PROPERTIES
        # Make sure ktx DLL is found by adding its directory to PATH
        ENVIRONMENT "PATH=$<TARGET_FILE_DIR:ktx>\;$ENV{PATH}"
    )
endif()

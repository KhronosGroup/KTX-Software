# gtest based unit-tests

add_subdirectory(gtest)
find_package(Threads)

enable_testing()

add_subdirectory(transcodetests)

add_executable( unittests
    unittests/unittests.cc
)

if(OPENGL_FOUND)
    target_include_directories(
        unittests
    PUBLIC
        ${OPENGL_INCLUDE_DIR}
    )
endif()

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

if(OPENGL_FOUND)
    add_executable( texturetests
        texturetests/texturetests.cc
        )

    target_include_directories(
        texturetests
        # PUBLIC
        PRIVATE
        ${OPENGL_INCLUDE_DIR}
        $<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES>
        ${PROJECT_SOURCE_DIR}/other_include
        ${PROJECT_SOURCE_DIR}/lib
        unittests
        )

    target_link_libraries(
        texturetests
        ${OPENGL_LIBRARIES}
        gtest
        ktx
        ${CMAKE_THREAD_LIBS_INIT}
        )
endif()

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

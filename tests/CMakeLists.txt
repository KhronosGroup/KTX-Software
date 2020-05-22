if(EMSCRIPTEN)
    message(STATUS "Tests don't work with emscripten -> skipping tests")
    return()
endif()

find_package (Threads)

add_subdirectory(gtest)
enable_testing()

add_subdirectory(transcodetests)

if(NOT KTX_FEATURE_KTX1)
    message(WARNING "KTX_FEATURE_KTX1 is not set -> disabling tests")
    return()
endif()

if(NOT KTX_FEATURE_KTX2)
    message(WARNING "KTX_FEATURE_KTX2 is not set -> disabling tests")
    return()
endif()

if(NOT KTX_FEATURE_WRITE)
    message(WARNING "KTX_FEATURE_WRITE is not set -> disabling tests")
    return()
endif()

# Adding Tests

add_executable( unittests
    unittests/unittests.cc
)

target_include_directories(
    unittests
PUBLIC
    ${OPENGL_INCLUDE_DIR}
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

add_subdirectory(loadtests)

add_test( NAME unittests COMMAND unittests )
add_test( NAME texturetests COMMAND texturetests )

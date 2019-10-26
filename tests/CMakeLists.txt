if(EMSCRIPTEN)
    message(STATUS "Tests don't work with emscripten -> skipping tests")
    return()
endif()

find_package (Threads)

add_subdirectory(gtest)
enable_testing()

get_property(objKtxRead_include_dirs TARGET objKtxRead PROPERTY INCLUDE_DIRECTORIES)

add_subdirectory(transcodetests)

if(NOT KTX_FEATURE_KTX1)
    message(WARNING "KTX_FEATURE_KTX1 is not set -> disabling tests")
    return()
endif()

if(NOT KTX_FEATURE_KTX2)
    message(WARNING "KTX_FEATURE_KTX2 is not set -> disabling tests")
    return()
endif()

if(NOT KTX_FEATURE_UPLOAD)
    message(WARNING "KTX_FEATURE_UPLOAD is not set -> disabling tests")
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
    PRIVATE
    ${objKtxRead_include_dirs}
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
    )

target_include_directories(
    texturetests
    # PUBLIC
    PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${objKtxRead_include_dirs}
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

add_subdirectory(loadtests)

add_test( NAME unittests COMMAND unittests )
add_test( NAME texturetests COMMAND texturetests )

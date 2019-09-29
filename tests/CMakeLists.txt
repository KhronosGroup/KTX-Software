add_subdirectory(gtest)
enable_testing()

get_property(objKtxRead_include_dirs TARGET objKtxRead PROPERTY INCLUDE_DIRECTORIES)

add_executable( unittests
    unittests/unittests.cc
)

target_include_directories(
    unittests
    PRIVATE
    ${objKtxRead_include_dirs}
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib
    )

target_link_libraries(
    unittests
    gtest
    ktx
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
    ${CMAKE_CURRENT_SOURCE_DIR}/../other_include
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib
    unittests
    )

target_link_libraries(
    texturetests
    ${OPENGL_LIBRARIES}
    gtest
    ktx
    )

get_property(texturetests_includes TARGET texturetests PROPERTY INCLUDE_DIRECTORIES)

add_test( NAME unittests COMMAND unittests )
add_test( NAME texturetests COMMAND texturetests )

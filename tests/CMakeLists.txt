add_subdirectory(gtest)
enable_testing()

find_package(OpenGL REQUIRED)

add_executable( unittests
    unittests/unittests.cc
)

get_property(objKtxRead_include_dirs TARGET objKtxRead PROPERTY INCLUDE_DIRECTORIES)

target_include_directories(
    unittests
    PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${objKtxRead_include_dirs}
    ../lib
    )

target_link_libraries(
    unittests
    ${OPENGL_LIBRARIES}
    gtest
    ktx
    )

add_test( NAME unittests COMMAND unittests )

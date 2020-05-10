
add_executable( transcodetests
    transcodetests.cc
    $<TARGET_OBJECTS:obj_basisu_cbind>
    )

get_property(ktx_include_dirs TARGET ktx PROPERTY INCLUDE_DIRECTORIES)
get_property(cbind_include_dirs TARGET obj_basisu_cbind PROPERTY INCLUDE_DIRECTORIES)

target_include_directories(
    transcodetests
    PRIVATE
    ${ktx_include_dirs}
    ${cbind_include_dirs}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../other_include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib
    )

target_link_libraries(
    transcodetests
    gtest
    ktx
    ${CMAKE_THREAD_LIBS_INIT}
    )

add_test( NAME transcodetests COMMAND transcodetests "${PROJECT_SOURCE_DIR}/tests/testimages/" )

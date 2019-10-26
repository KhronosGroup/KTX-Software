
add_executable( transcodetests
    transcodetests.cc
    $<TARGET_OBJECTS:obj_basisu_cbind>
    )

get_property(objKtxRead_include_dirs TARGET objKtxRead PROPERTY INCLUDE_DIRECTORIES)
get_property(basisu_include_dirs TARGET objBasisu PROPERTY INCLUDE_DIRECTORIES)
get_property(cbind_include_dirs TARGET obj_basisu_cbind PROPERTY INCLUDE_DIRECTORIES)

target_include_directories(
    transcodetests
    PRIVATE
    ${objKtxRead_include_dirs}
    ${basisu_include_dirs}
    ${cbind_include_dirs}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../other_include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib
    )

target_link_libraries(
    transcodetests
    gtest
    ktx
    )

add_test( NAME transcodetests COMMAND transcodetests "${PROJECT_SOURCE_DIR}/tests/testimages/" )


add_executable( transcodetests
    transcodetests.cc
    $<TARGET_OBJECTS:obj_basisu_cbind>
    )

target_include_directories(
    transcodetests
    PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:obj_basisu_cbind,INTERFACE_INCLUDE_DIRECTORIES>
    ${CMAKE_CURRENT_SOURCE_DIR}/../../other_include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib
    )

target_link_libraries(
    transcodetests
    gtest
    ktx
    ${CMAKE_THREAD_LIBS_INIT}
    )

target_compile_definitions(
    transcodetests
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
)

add_test( NAME transcodetests COMMAND transcodetests "${PROJECT_SOURCE_DIR}/tests/testimages/" )

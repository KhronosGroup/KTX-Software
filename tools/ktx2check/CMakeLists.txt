add_executable( ktx2check
    ktx2check.cpp
    $<TARGET_OBJECTS:objUtil>
    )

get_property(objKtxRead_include_dirs TARGET objKtxRead PROPERTY INCLUDE_DIRECTORIES)
get_property(objUtil_include_dirs TARGET objUtil PROPERTY INCLUDE_DIRECTORIES)


target_include_directories(
    ktx2check
    PRIVATE
    ${objKtxRead_include_dirs}
    ${objUtil_include_dirs}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../other_include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib
    )

target_link_libraries(
    ktx2check
    ktx
    )
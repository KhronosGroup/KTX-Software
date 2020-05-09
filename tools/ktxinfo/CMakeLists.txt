add_executable( ktxinfo
    ktxinfo.cpp
    $<TARGET_OBJECTS:objUtil>
    )

get_property(objKtxRead_include_dirs TARGET objKtxRead PROPERTY INCLUDE_DIRECTORIES)
get_property(objUtil_include_dirs TARGET objUtil PROPERTY INCLUDE_DIRECTORIES)


target_include_directories(
    ktxinfo
    PRIVATE
    ${objKtxRead_include_dirs}
    ${objUtil_include_dirs}
    ${PROJECT_SOURCE_DIR}/lib
    )

target_link_libraries(
    ktxinfo
    ktx
    )

install(TARGETS ktxinfo)

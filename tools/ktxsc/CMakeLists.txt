add_executable( ktxsc
    ktxsc.cpp
    $<TARGET_OBJECTS:objUtil>
    )

get_property(ktx_include_dirs TARGET ktx PROPERTY INCLUDE_DIRECTORIES)
get_property(objUtil_include_dirs TARGET objUtil PROPERTY INCLUDE_DIRECTORIES)


target_include_directories(
    ktxsc
    PRIVATE
    ${ktx_include_dirs}
    ${objUtil_include_dirs}
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/other_include
    )

target_link_libraries(
    ktxsc
    ktx
    )

install(TARGETS ktxsc)

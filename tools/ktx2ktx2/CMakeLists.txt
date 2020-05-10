add_executable( ktx2ktx2
    ktx2ktx2.cpp
    $<TARGET_OBJECTS:objUtil>
    )

get_property(ktx_include_dirs TARGET ktx PROPERTY INCLUDE_DIRECTORIES)
get_property(objUtil_include_dirs TARGET objUtil PROPERTY INCLUDE_DIRECTORIES)


target_include_directories(
    ktx2ktx2
    PRIVATE
    ${ktx_include_dirs}
    ${objUtil_include_dirs}
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/other_include
    )

target_link_libraries(
    ktx2ktx2
    ktx
    )

install(TARGETS ktx2ktx2)

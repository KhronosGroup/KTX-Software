add_executable( ktxinfo
    $<TARGET_OBJECTS:objUtil>
    ktxinfo.cpp
    stdafx.h
    )

get_property(ktx_include_dirs TARGET ktx PROPERTY INCLUDE_DIRECTORIES)
get_property(objUtil_include_dirs TARGET objUtil PROPERTY INCLUDE_DIRECTORIES)


target_include_directories(
    ktxinfo
    PRIVATE
    ${ktx_include_dirs}
    ${objUtil_include_dirs}
    ${PROJECT_SOURCE_DIR}/lib
    )

target_link_libraries(
    ktxinfo
    ktx
    )

install(TARGETS ktxinfo)

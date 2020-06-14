add_executable( ktxinfo
    $<TARGET_OBJECTS:objUtil>
    ktxinfo.cpp
    stdafx.h
)
create_version_header( tools/ktxinfo ktxinfo )

target_include_directories(
    ktxinfo
PRIVATE
    .
    $<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:objUtil,INTERFACE_INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/lib
)

target_link_libraries(
    ktxinfo
    ktx
)

install(TARGETS ktxinfo
    RUNTIME
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT tools
)

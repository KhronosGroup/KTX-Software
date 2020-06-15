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

set_target_properties(ktxinfo PROPERTIES
    XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
    INSTALL_RPATH "@executable_path;/usr/local/lib"
)
set_xcode_code_sign(ktxinfo)

install(TARGETS ktxinfo
    RUNTIME
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT tools
)

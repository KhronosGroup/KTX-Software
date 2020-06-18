add_executable( ktxsc
    $<TARGET_OBJECTS:objUtil>
    ktxsc.cpp
    stdafx.h
)
create_version_header( tools/ktxsc ktxsc )

target_include_directories(
    ktxsc
PRIVATE
    .
    $<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:objUtil,INTERFACE_INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/other_include
)

target_link_libraries(
    ktxsc
    ktx
)

set_target_properties(ktxsc PROPERTIES
    XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
    INSTALL_RPATH "@executable_path;/usr/local/lib"
)
set_xcode_code_sign(ktxsc)

add_executable( ktx2check
    $<TARGET_OBJECTS:objUtil>
    ktx2check.cpp
    stdafx.h
)
create_version_header( tools/ktx2check ktx2check )

target_include_directories(
    ktx2check
PRIVATE
    .
    $<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:objUtil,INTERFACE_INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/other_include
)

target_link_libraries(
    ktx2check
    ktx
)

set_target_properties(ktx2check PROPERTIES
    XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
    INSTALL_RPATH "@executable_path;/usr/local/lib"
)
set_xcode_code_sign(ktx2check)

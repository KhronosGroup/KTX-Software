# Copyright 2017-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

add_executable( ktxinfo
    ktxinfo.cpp
)
create_version_header( tools/ktxinfo ktxinfo )

target_include_directories(
    ktxinfo
PRIVATE
    .
    $<TARGET_PROPERTY:ktx,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:objUtil,INTERFACE_INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/lib
)

target_link_libraries(
    ktxinfo
    ktx
    objUtil
)

set_tool_properties(ktxinfo)
set_code_sign(ktxinfo)

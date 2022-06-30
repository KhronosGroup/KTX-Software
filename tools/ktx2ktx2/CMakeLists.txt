# Copyright 2017-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

add_executable( ktx2ktx2
    ktx2ktx2.cpp
)
create_version_header( tools/ktx2ktx2 ktx2ktx2 )

target_include_directories(
    ktx2ktx2
PRIVATE
    .
    $<TARGET_PROPERTY:ktx,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:objUtil,INTERFACE_INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/other_include
)

target_link_libraries(
    ktx2ktx2
    ktx
    objUtil
)

set_tool_properties(ktx2ktx2)
set_code_sign(ktx2ktx2)

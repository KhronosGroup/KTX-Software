# Copyright 2017-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

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
    $<TARGET_PROPERTY:ktx,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:objUtil,INTERFACE_INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/other_include
)

target_link_libraries(
    ktx2check
    ktx
)
set_tool_properties(ktx2check)
set_xcode_code_sign(ktx2check)

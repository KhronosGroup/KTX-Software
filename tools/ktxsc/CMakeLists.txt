# Copyright 2019-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

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
    $<TARGET_PROPERTY:ktx,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:objUtil,INTERFACE_INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/other_include
)

target_link_libraries(
    ktxsc
    ktx
)

set_tool_properties(ktxsc)
set_xcode_code_sign(ktxsc)

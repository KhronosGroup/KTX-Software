# Copyright 2019-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

add_executable( ktxsc
    ktxsc.cpp
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
    objUtil
)

set_target_properties(
    ktxsc
    PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
)

set_tool_properties(ktxsc)
set_code_sign(ktxsc)

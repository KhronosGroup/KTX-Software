# Copyright 2017-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

add_executable( ktx2check
    ktx2check.cpp
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

set_target_properties(
    ktx2check PROPERTIES
    CXX_VISIBILITY_PRESET ${STATIC_APP_LIB_SYMBOL_VISIBILITY}
    # Force C++14 (for std::make_unique on AppleClang and Linux GCC)
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(
    ktx2check
    ktx
    objUtil
)

set_tool_properties(ktx2check)
set_code_sign(ktx2check)

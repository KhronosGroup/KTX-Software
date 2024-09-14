# Copyright 2010-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

add_executable( toktx
    toktx.cc
)

create_version_header( tools/toktx toktx )

target_include_directories(
    toktx
PRIVATE
    .
    $<TARGET_PROPERTY:imageio,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:ktx,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:objUtil,INTERFACE_INCLUDE_DIRECTORIES>
)

target_include_directories(
    toktx
    SYSTEM
PRIVATE
    $<TARGET_PROPERTY:imageio,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
    "${PROJECT_SOURCE_DIR}/lib"
    "${PROJECT_SOURCE_DIR}/external/dfdutils"
)

target_link_libraries(
    toktx
    imageio
    ktx
    objUtil
)

target_compile_definitions(
    toktx
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
)

target_compile_features(toktx PUBLIC cxx_std_11)

set_tool_properties(toktx)
set_code_sign(toktx)

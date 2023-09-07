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
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/lib/dfdutils
)

target_link_libraries(
    toktx
    imageio
    ktx
    objUtil
)

set_target_properties(
    toktx
    PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
)

target_compile_definitions(
    toktx
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
)

# The tools, except ktx2check which explicitly chooses c++14, are somehow
# being set to use the c++11 dialect. Libraries and other targets are not.
# Despite searching every CMakeLists.txt and .cmake file in the project,
# I've been unable to find where this is being set. As toktx needs c++11
# (at least), explicitly set it here, a precaution against the mysterious
# setting disappearing.
target_compile_features(toktx PUBLIC cxx_std_11)

set_tool_properties(toktx)
set_code_sign(toktx)

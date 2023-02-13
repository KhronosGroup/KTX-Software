# Copyright 2022-2023 The Khronos Group Inc.
# Copyright 2022-2023 RasterGrid Kft.
# SPDX-License-Identifier: Apache-2.0


add_executable(ktxtools
        command.cpp
        command.h
        command_create.cpp
        command_extract.cpp
        command_info.cpp
        command_validate.cpp
        formats.h
        ktx_main.cpp
        utility.h
        validate.cpp
        validate.h
        validation_messages.h
)

create_version_header(tools/ktx ktxtools)

set_target_properties(
    ktxtools
    PROPERTIES
        OUTPUT_NAME ktx
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
)

target_include_directories(
    ktxtools
PRIVATE
    .
    $<TARGET_PROPERTY:imageio,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:${KTX_TARGET_NAME_STATIC_LIB},INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/lib
)

target_link_libraries(
    ktxtools
PRIVATE
    imageio
    ${KTX_TARGET_NAME_STATIC_LIB}
    objUtil
    fmt::fmt
    cxxopts::cxxopts
)

target_compile_definitions(
    ktxtools
PRIVATE
    $<TARGET_PROPERTY:${KTX_TARGET_NAME_STATIC_LIB},INTERFACE_COMPILE_DEFINITIONS>
)

set_tool_properties(ktxtools)
set_code_sign(ktxtools)

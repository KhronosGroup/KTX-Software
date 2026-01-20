# Copyright 2025 Danylo Sivachenko (@Daniil-SV).
# SPDX-License-Identifier: Apache-2.0

####################################################
# fmt
####################################################
include(FetchContent)

if (TARGET fmt::fmt)
    message(STATUS "(${PROJECT_NAME}): Using configured fmt target")
    return()
endif()

set(FMT_VERSION 11.2.0)
message(STATUS "FMT VERSION: ${FMT_VERSION}")

# Options
# Always build fmt static
set(BUILD_SHARED_LIBS OFF)
set(FMT_INSTALL OFF)
set(FMT_SYSTEM_HEADERS ON)

# Declare package
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG ${FMT_VERSION}
    FIND_PACKAGE_ARGS
)

# Populate fmt
FetchContent_MakeAvailable(fmt)

# Copyright 2025 Danylo Sivachenko (@Daniil-SV).
# SPDX-License-Identifier: Apache-2.0

####################################################
# cxxopts
####################################################
include(FetchContent)

if (TARGET cxxopts::cxxopts)
    message(STATUS "(${PROJECT_NAME}): Using configured cxxopts target")
    return()
endif()

set(CXXOPTS_VERSION 3.3.1)
message(STATUS "CXXOPTS VERSION: ${CXXOPTS_VERSION}")

# Options
# Always build fmt static
set(BUILD_SHARED_LIBS OFF)
set(FMT_INSTALL OFF)
set(FMT_SYSTEM_HEADERS ON)

# Declare package
FetchContent_Declare(
    cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    GIT_TAG "v${CXXOPTS_VERSION}"
    FIND_PACKAGE_ARGS
)

# Populate fmt
FetchContent_MakeAvailable(cxxopts)
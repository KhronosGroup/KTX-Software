####################################################
# fmt
####################################################
include(FetchContent)

if (TARGET fmt::fmt)
    message(STATUS "Using prebuilt fmt")
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
    GIT_TAG "v${FMT_VERSION}"
    FIND_PACKAGE_ARGS
)

# Populate fmt
FetchContent_MakeAvailable(fmt)

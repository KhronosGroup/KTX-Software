####################################################
# Zstandart
####################################################
include(FetchContent)

if (TARGET zstd::libzstd)
    message(STATUS "Using prebuilt zstd")
    return()
endif()

set(ZSTD_VERSION 1.5.7)
message(STATUS "ZSTD VERSION: ${ZSTD_VERSION}")

# Options
# Build static by default
if(NOT DEFINED ZSTD_BUILD_SHARED)
	set(ZSTD_BUILD_STATIC ON)
	set(ZSTD_BUILD_SHARED OFF)
endif()
set(ZSTD_LEGACY_SUPPORT OFF)
set(ZSTD_BUILD_DICTBUILDER OFF)
set(ZSTD_BUILD_DEPRECATED OFF)
set(ZSTD_BUILD_PROGRAMS OFF)
set(ZSTD_BUILD_TESTS OFF)
set(ZSTD_BUILD_CONTRIB OFF)

# Declare package
FetchContent_Declare(
    zstd
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR build/cmake
    URL "https://github.com/facebook/zstd/releases/download/v${ZSTD_VERSION}/zstd-${ZSTD_VERSION}.tar.gz"
    FIND_PACKAGE_ARGS NAMES ZSTD zstd
)

# Populate zstd
FetchContent_MakeAvailable(zstd)

if (NOT TARGET zstd::libzstd)
    # Create one common target name
    if (TARGET libzstd_static)
        add_library(zstd::libzstd ALIAS libzstd_static)
    elseif(TARGET libzstd_shared)
        add_library(zstd::libzstd ALIAS libzstd_shared)
    endif()
endif()
####################################################
# Zstandart
####################################################
include(FetchContent)

if (TARGET zstd::libzstd)
    message(STATUS "(${PROJECT_NAME}): Using configured zstd target")
    return()
endif()

set(ZSTD_VERSION 1.5.7)

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
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# On most platforms, static libraries require compilation with -fPIC for shared builds, but zstd is usually supplied without it.
# So we need to build it manually in such cases
# On Windows, this behavior is simplified, so we can try to use system installed library (shared/static) without any fear.
# Or if this is a static library build or was requested to use shared zstd, we can also search for already installed files.
if (NOT ${BUILD_SHARED_LIBS} OR CMAKE_SYSTEM_NAME STREQUAL "Windows" OR ZSTD_BUILD_SHARED)
    find_package(zstd CONFIG)
endif()

if (zstd_FOUND)
    message(STATUS "Using system ZSTD ${zstd_VERSION}")
else()
    message(STATUS "ZSTD VERSION: ${ZSTD_VERSION}")

    # Declare package
    FetchContent_Declare(
        zstd
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        SOURCE_SUBDIR build/cmake
        URL "https://github.com/facebook/zstd/releases/download/v${ZSTD_VERSION}/zstd-${ZSTD_VERSION}.tar.gz"
    )

    # Populate zstd
    FetchContent_MakeAvailable(zstd)
endif()

if (NOT TARGET zstd::libzstd)
    # Normalize different zstd target names to one common alias
    set(ZSTD_LOOKUP_NAMES
        ZSTD::ZSTD
    )

    if (${ZSTD_BUILD_SHARED})
        list(APPEND ZSTD_LOOKUP_NAMES
            zstd::libzstd_shared
            libzstd_shared
        )
    endif()

    if (${ZSTD_BUILD_STATIC})
        list(APPEND ZSTD_LOOKUP_NAMES
            zstd::libzstd_static
            libzstd_static
        )
    endif()

    foreach(LOOKUP_NAME IN LISTS ZSTD_LOOKUP_NAMES)
        if (TARGET ${LOOKUP_NAME})
            add_library(zstd::libzstd ALIAS ${LOOKUP_NAME})
            break()
        endif()
    endforeach()
endif()
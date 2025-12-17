# Copyright 2025 Danylo Sivachenko (@Daniil-SV).
# SPDX-License-Identifier: Apache-2.0

####################################################
# Basis Universal Encoder
####################################################
include(FetchContent)

if (TARGET basisu::basisu_encoder)
    message(STATUS "(${PROJECT_NAME}): Using configured Basis Universal Encoder target")
    return()
endif()

# TODO: Restore after new basisu release
#set(BASISU_VERSION 1_60)
#message(STATUS "Basisu VERSION: ${BASISU_VERSION}")

# Options
set(TOOL FALSE)
set(EXAMPLES FALSE)
if(NOT ${CPU_ARCHITECTURE} STREQUAL "x86_64")
    # Basisu sets this TRUE if MSVC is TRUE.
    set(BASISU_SSE FALSE)
endif()
set(SSE ${BASISU_SSE})
set(OPENCL ${BASISU_OPENCL})

# Declare package
FetchContent_Declare(
    basisu
    GIT_REPOSITORY https://github.com/Daniil-SV/basis_universal.git
    GIT_TAG master #"v${BASISU_VERSION}"
    FIND_PACKAGE_ARGS
)

# Populate basisu
# We need to explicitly populate basisu and add it as a subdirectory with EXCLUDE_FROM_ALL 
# to avoid problems with KTX_WERROR in ci
FetchContent_GetProperties(basisu)
if (NOT basisu_POPULATED)
    FetchContent_Populate(basisu)
    add_subdirectory(
        ${basisu_SOURCE_DIR}
        ${basisu_BINARY_DIR}
        EXCLUDE_FROM_ALL
    )
endif()

if (NOT TARGET basisu::basisu_encoder)
    add_library(basisu::basisu_encoder ALIAS basisu_encoder)
    
    # In some package managers, like vcpkg, all headers are stored in "basisu" subdirectory, 
    # while in the official repository they are stored in the root folder, which creates some conflicts.
    # Objectively, headers in a subdirectory look safer and better.
    # So the solution is to copy the headers to the subdirectory by hands, if necessary.

    # Temporary folder with our headers
    set(BASISU_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include/basisu)

    # Folders with headers in repo root
    set(BASISU_SUBDIRS encoder transcoder)
    foreach(subdir ${BASISU_SUBDIRS})
        # Collecting just headers paths
        file(GLOB_RECURSE headers
            RELATIVE ${basisu_SOURCE_DIR}/${subdir}
            ${basisu_SOURCE_DIR}/${subdir}/*.h
            ${basisu_SOURCE_DIR}/${subdir}/*.inc
        )

        # Iterating over collected files
        foreach(filename ${headers})
            get_filename_component(directory ${filename} DIRECTORY)

            # Ensure destination directory exists
            file(
                MAKE_DIRECTORY
                ${BASISU_INCLUDE_DIR} ${BASISU_INCLUDE_DIR}/${subdir}/${directory}
            )

            # And finally copy to temporary folder
            file(COPY ${basisu_SOURCE_DIR}/${subdir}/${filename}
                DESTINATION ${BASISU_INCLUDE_DIR}/${subdir}/${directory})
        endforeach()
    endforeach()

    # Add temporary include path to target
    target_include_directories(basisu_encoder PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    )
endif()

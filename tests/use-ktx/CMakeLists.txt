# Copyright 2025 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Project, that is not KTX-Software, to test using libktx as a sub-project.

cmake_minimum_required(VERSION 3.22)

# MUST be set before project() else it is ignored.
# N.B IOS and, in the Darwin case, CMAKE_SYSTEM_NAME are not set
# until after the project() command. The latter is most strange.
if(CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CMAKE_SYSTEM_NAME STREQUAL "tvOS")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "iOS/tvOS Deployment Target")
    set(CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH NO)
elseif(CMAKE_SYSTEM_NAME STREQUAL "visionOS")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "1.0" CACHE STRING "visionOS Deployment Target")
else()
    set(CMAKE_OSX_DEPLOYMENT_TARGET "13.3" CACHE STRING "macOS Deployment Target")
endif()

project(use-ktx
    VERSION 1.0
    DESCRIPTION "Test using libktx project independently of KTX-Software"
)

option(USE_STATIC_LIBRARY "Build and use static library" OFF)

set(path_to_ktx ../../lib)

if(WIN32)
    # Set this to make the ktx build put ktx.dll into the same
    # directory as use-ktx.exe, so use-ktx can be run in its
    # build location.
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

if(USE_STATIC_LIBRARY)
    message("Building and using static library")
    set(BUILD_SHARED_LIBS OFF)
else()
    message("Building and using shared library")
endif()
add_subdirectory(${path_to_ktx} ktx)

add_executable(use-ktx
    use-ktx.cc
)

# libktx adds KHRONOS_STATIC as a public define so no need to set it here.
target_compile_features(use-ktx PRIVATE cxx_std_20)
target_link_libraries(use-ktx PRIVATE ktx)

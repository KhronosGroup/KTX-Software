#  SPDX-License-Identifier: Apache-2.0
#  ----------------------------------------------------------------------------
#  Copyright 2020-2021 Arm Limited
#
#  Licensed under the Apache License, Version 2.0 (the "License"); you may not
#  use this file except in compliance with the License. You may obtain a copy
#  of the License at:
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
#  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
#  License for the specific language governing permissions and limitations
#  under the License.
#  ----------------------------------------------------------------------------

# CMake configuration
cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0069 NEW)  # LTO support
cmake_policy(SET CMP0091 NEW)  # MSVC runtime support

project(astcenc VERSION 3.0.0)

# Command line configuration
function(printopt optName optVal optArch tgtArch)
    if(${optVal})
        if(${optArch} MATCHES ${tgtArch})
            message("  -- ${optName} backend - ON")
        else()
            message("  -- ${optName} backend - SKIPPED (${optArch} only)")
        endif()
    else()
        message("  -- ${optName} backend - OFF")
    endif()
endfunction()

set(VALID_ARCH aarch64 aarch32 aarch32hf x64)
set(ARCH x64 CACHE STRING "Target architecture")
set_property(CACHE ARCH PROPERTY STRINGS ${VALID_ARCH})

message("-- Selecting target astcenc backends:")
list(FIND VALID_ARCH ${ARCH} index)
if(index EQUAL -1)
    message(FATAL_ERROR "ARCH must be one of ${VALID_ARCH}")
endif()

set(ANY_ISA 0)

option(ISA_AVX2 "Enable builds for AVX2 SIMD")
printopt("AVX2" ${ISA_AVX2} "x64" ${ARCH})
if(${ISA_AVX2} AND ${ARCH} MATCHES "x64")
    set(ANY_ISA 1)
endif()

option(ISA_SSE41 "Enable builds for SSE4.1 SIMD")
printopt("SSE4.1" ${ISA_SSE41} "x64" ${ARCH})
if(${ISA_SSE41} AND ${ARCH} MATCHES "x64")
    set(ANY_ISA 1)
endif()

option(ISA_SSE2 "Enable builds for SSE2 SIMD")
printopt("SSE2" ${ISA_SSE2} "x64" ${ARCH})
if(${ISA_SSE2} AND ${ARCH} MATCHES "x64")
    set(ANY_ISA 1)
endif()

option(ISA_NEON "Enable builds for NEON SIMD")
printopt("NEON" ${ISA_NEON} "aarch32|aarch32hf|aarch64" ${ARCH})
if(${ISA_NEON} AND ${ARCH} MATCHES "aarch32|aarch32hf|aarch64")
    set(ANY_ISA 1)
endif()

option(ISA_NONE "Enable builds for no SIMD")
if(${ISA_NONE})
    set(ANY_ISA 1)
    message("  -- No SIMD backend - ON")
else()
    message("  -- No SIMD backend - OFF")
endif()

option(DECOMPRESSOR "Enable builds for decompression only")
if(${DECOMPRESSOR})
    message("  -- Decompress-only backend - ON")
else()
    message("  -- Decompress-only backend - OFF")
endif()

option(DIAGNOSTICS "Enable builds for diagnostic trace")
if(${DIAGNOSTICS})
    message("  -- Diagnostic tracing - ON")
else()
    message("  -- Diagnostic tracing - OFF")
endif()

option(UNITTEST "Enable builds for unit tests")
if(${UNITTEST})
    message("  -- Unit tests - ON")
else()
    message("  -- Unit tests - OFF")
endif()

if(NOT ${ANY_ISA})
    message(FATAL_ERROR "At least one backend ISA must be enabled")
endif()

# Project configuration

# Must be done after project() but before compiler settings
# or it gets overriden (this contradicts the official docs)
if(${ARCH} MATCHES "aarch64")
    set(CMAKE_OSX_ARCHITECTURES "arm64")

elseif(${ARCH} MATCHES "aarch32|aarch32hf")
    # Deliberately use Armv7 for Clang - its most compatible with the current
    # code; we provide some fallbacks for a few missing v8 intrinsics.
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

        set(CMAKE_OSX_ARCHITECTURES "armv7")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv7-a")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=neon-vfpv4")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument")
        set(CMAKE_CXX_FLAGS_RELEASE "-O3")

    # For GCC we directly target Armv8-A A32, and provide some emulated
    # fallback intrinsics for functionality that exists for A64 but not A32.
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=neon-fp-armv8")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfp16-format=ieee")

        if(${ARCH} STREQUAL "aarch32")  # arm-linux-gnueabi-g++
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfloat-abi=softfp")
        elseif(${ARCH} STREQUAL "aarch32hf")  # arm-linux-gnueabihf-g++
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfloat-abi=hard")
        endif()

        # Workaround for Source/tinyexr.h:11025
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-type-limits")
    endif()
else()
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(PACKAGE_ROOT astcenc)
include(CTest)

# Subcomponents
add_subdirectory(Source)

# Configure package archive
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    string(TOLOWER "macOS" PKG_OS)
else()
    string(TOLOWER ${CMAKE_SYSTEM_NAME} PKG_OS)
endif()

set(PKG_VER ${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR})

set(CPACK_PACKAGE_FILE_NAME "astcenc-${PKG_VER}-${PKG_OS}-${ARCH}")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)
set(CPACK_PACKAGE_CHECKSUM SHA256)
set(CPACK_GENERATOR ZIP)

include(CPack) # Must be included after CPack configuration.

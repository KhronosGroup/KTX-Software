#  SPDX-License-Identifier: Apache-2.0
#  ----------------------------------------------------------------------------
#  Copyright 2020-2023 Arm Limited
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

if(MSVC)
    add_compile_options("/wd4324") # Disable structure was padded due to alignment specifier
endif()

project(astcencoder VERSION 4.4.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include(CTest)

option(ASTCENC_ISA_AVX2 "Enable astcenc builds for AVX2 SIMD")
option(ASTCENC_ISA_SSE41 "Enable astcenc builds for SSE4.1 SIMD")
option(ASTCENC_ISA_SSE2 "Enable astcenc builds for SSE2 SIMD")
option(ASTCENC_ISA_NEON "Enable astcenc builds for NEON SIMD")
option(ASTCENC_ISA_NONE "Enable astcenc builds for no SIMD")
option(ASTCENC_ISA_NATIVE "Enable astcenc builds for native SIMD")
option(ASTCENC_DECOMPRESSOR "Enable astcenc builds for decompression only")
option(ASTCENC_SHAREDLIB "Enable astcenc builds with core library shared objects")
option(ASTCENC_DIAGNOSTICS "Enable astcenc builds with diagnostic trace")
option(ASTCENC_ASAN "Enable astcenc builds with address sanitizer")
option(ASTCENC_UNITTEST "Enable astcenc builds with unit tests")
option(ASTCENC_INVARIANCE "Enable astcenc floating point invariance" ON)
option(ASTCENC_CLI "Enable build of astcenc command line tools" ON)

set(ASTCENC_UNIVERSAL_BUILD OFF)
set(ASTCENC_MACOS_BUILD OFF)
set(ASTCENC_MACOS_ARCH_LEN 0)

# Preflight for some macOS-specific build options
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    set(ASTCENC_MACOS_BUILD ON)
    list(LENGTH CMAKE_OSX_ARCHITECTURES ASTCENC_MACOS_ARCH_LEN)
endif()

# Count options which MUST be x64
set(ASTCENC_X64_ISA_COUNT 0)
set(ASTCENC_CONFIGS ${ASTCENC_ISA_AVX2} ${ASTCENC_ISA_SSE41} ${ASTCENC_ISA_SSE2})
foreach(ASTCENC_CONFIG ${ASTCENC_CONFIGS})
    if(${ASTCENC_CONFIG})
        math(EXPR ASTCENC_X64_ISA_COUNT "${ASTCENC_X64_ISA_COUNT} + 1")
    endif()
endforeach()

# Count options which MUST be arm64
set(ASTCENC_ARM64_ISA_COUNT 0)
set(ASTCENC_CONFIGS ${ASTCENC_ISA_NEON})
foreach(ASTCENC_CONFIG ${ASTCENC_CONFIGS})
    if(${ASTCENC_CONFIG})
        math(EXPR ASTCENC_ARM64_ISA_COUNT "${ASTCENC_ARM64_ISA_COUNT} + 1")
    endif()
endforeach()

# macOS builds
if("${ASTCENC_MACOS_BUILD}")
    list(FIND CMAKE_OSX_ARCHITECTURES "x86_64" ASTCENC_IS_X64)
    list(FIND CMAKE_OSX_ARCHITECTURES "arm64" ASTCENC_IS_ARM64)
    list(FIND CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD)" ASTCENC_IS_AUTO)

    # Turn list index into boolean
    if(${ASTCENC_IS_X64} EQUAL -1)
        set(ASTCENC_IS_X64 OFF)
    else()
        set(ASTCENC_IS_X64 ON)
    endif()

    if(${ASTCENC_IS_ARM64} EQUAL -1)
        set(ASTCENC_IS_ARM64 OFF)
    else()
        set(ASTCENC_IS_ARM64 ON)
    endif()

    if(${ASTCENC_IS_AUTO} EQUAL -1)
        set(ASTCENC_IS_AUTO OFF)
    else()
        set(ASTCENC_IS_AUTO ON)
    endif()

    # Set up defaults if no more specific ISA set - use XCode's own defaults
    if((ASTCENC_IS_ARM64 OR ASTCENC_IS_AUTO) AND ("${ASTCENC_ARM64_ISA_COUNT}" EQUAL 0) AND (NOT "${ASTCENC_ISA_NONE}"))
        set(ASTCENC_ARM64_ISA_COUNT 1)
        set(ASTCENC_ISA_NEON ON)
    endif()

    if((ASTCENC_IS_X64 OR ASTCENC_IS_AUTO) AND ("${ASTCENC_X64_ISA_COUNT}" EQUAL 0) AND (NOT "${ASTCENC_ISA_NONE}"))
        set(ASTCENC_X64_ISA_COUNT 1)
        set(ASTCENC_ISA_SSE41 ON)
    endif()

    # User might be doing multi-architecture - XCode sets this at runtime
    if("${ASTCENC_IS_AUTO}")
        if(("${ASTCENC_ARM64_ISA_COUNT}" GREATER 1) OR ("${ASTCENC_X64_ISA_COUNT}" GREATER 1))
            message(FATAL_ERROR "For macOS universal binaries only one backend per architecture is allowed.")
        endif()

        set(ASTCENC_UNIVERSAL_BUILD ON)

    # User requested explicit multi-architecture universal build
    elseif("${ASTCENC_MACOS_ARCH_LEN}" GREATER 2)
        message(FATAL_ERROR "For macOS universal binaries only x86_64 and arm64 builds are allowed.")

    elseif("${ASTCENC_MACOS_ARCH_LEN}" EQUAL 2)
        if(NOT (${ASTCENC_IS_X64} AND ${ASTCENC_IS_ARM64}))
            message(FATAL_ERROR "For macOS universal binaries only x86_64 and arm64 builds are allowed.")
        endif()

        if(("${ASTCENC_ARM64_ISA_COUNT}" GREATER 1) OR ("${ASTCENC_X64_ISA_COUNT}" GREATER 1))
            message(FATAL_ERROR "For macOS universal binaries only one backend per architecture is allowed.")
        endif()

        set(ASTCENC_UNIVERSAL_BUILD ON)

    # User requested explicit single architecture build
    elseif("${ASTCENC_MACOS_ARCH_LEN}" EQUAL 1)
        if("${ASTCENC_IS_X64}" AND "${ASTCENC_ARM64_ISA_COUNT}")
            message(FATAL_ERROR "For macOS x86_64 builds an arm64 backend cannot be specified.")
        endif()

        if("${ASTCENC_IS_ARM64}" AND "${ASTCENC_X64_ISA_COUNT}")
            message(FATAL_ERROR "For macOS arm64 builds an x86_64 backend cannot be specified.")
        endif()

    # Else is this a implicit multi-architecture universal build?
    elseif(("${ASTCENC_ARM64_ISA_COUNT}" EQUAL 1) AND ("${ASTCENC_X64_ISA_COUNT}" GREATER 1))
        string(CONCAT MSG "For macOS setting multiple architecture backends builds a universal binary. "
                          "For universal binaries only one backend per architecture is allowed.")
        message(FATAL_ERROR "${MSG}")

    elseif(("${ASTCENC_X64_ISA_COUNT}" EQUAL 1) AND ("${ASTCENC_ARM64_ISA_COUNT}" GREATER 1))
        string(CONCAT MSG "For macOS setting multiple architecture backends builds a universal binary. "
                          "For universal binaries only one backend per architecture is allowed.")
        message(FATAL_ERROR "${MSG}")

    elseif(("${ASTCENC_ARM64_ISA_COUNT}" EQUAL 1) AND ("${ASTCENC_X64_ISA_COUNT}" EQUAL 1))
        set(ASTCENC_UNIVERSAL_BUILD ON)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")

    # Else is this an implicit single architecture build?
    elseif("${ASTCENC_ARM64_ISA_COUNT}" EQUAL 1)
        set(CMAKE_OSX_ARCHITECTURES "arm64")

    elseif("${ASTCENC_X64_ISA_COUNT}" EQUAL 1)
        set(CMAKE_OSX_ARCHITECTURES "x86_64")

    else()
        # Do nothing here - assume it defaults to host?

    endif()

# Non-macOS builds
else()
    if(("${ASTCENC_ARM64_ISA_COUNT}" GREATER 0) AND ("${ASTCENC_X64_ISA_COUNT}" GREATER 0))
        message(FATAL_ERROR "Builds can only support a single architecture per configure.")
    endif()
endif()

# If nothing more specific is set then fall back on the compiler's defaults
if(("${ASTCENC_ARM64_ISA_COUNT}" EQUAL 0) AND ("${ASTCENC_X64_ISA_COUNT}" EQUAL 0) AND (NOT "${ASTCENC_ISA_NONE}"))
    set(ASTCENC_ISA_NATIVE ON)
endif()

function(printopt optName optVal)
    if(${optVal})
        message(STATUS "  ${optName}  - ON")
    else()
        message(STATUS "  ${optName}  - OFF")
    endif()
endfunction()

if("${ASTCENC_BLOCK_MAX_TEXELS}")
     message(STATUS "  Max block texels - ${ASTCENC_BLOCK_MAX_TEXELS}")
endif()

printopt("AVX2 backend   " ${ASTCENC_ISA_AVX2})
printopt("SSE4.1 backend " ${ASTCENC_ISA_SSE41})
printopt("SSE2 backend   " ${ASTCENC_ISA_SSE2})
printopt("NEON backend   " ${ASTCENC_ISA_NEON})
printopt("NONE backend   " ${ASTCENC_ISA_NONE})
printopt("NATIVE backend " ${ASTCENC_ISA_NATIVE})
if("${ASTCENC_MACOS_BUILD}")
    printopt("Universal bin  " ${ASTCENC_UNIVERSAL_BUILD})
endif()
printopt("Invariance     " ${ASTCENC_INVARIANCE})
printopt("Shared libs    " ${ASTCENC_SHAREDLIB})
printopt("Decompressor   " ${ASTCENC_DECOMPRESSOR})
printopt("Diagnostics    " ${ASTCENC_DIAGNOSTICS})
printopt("ASAN           " ${ASTCENC_ASAN})
printopt("Unit tests     " ${ASTCENC_UNITTEST})

# Subcomponents
add_subdirectory(Source)

# Configure package archive
if(ASTCENC_PACKAGE)
    if("${ASTCENC_MACOS_BUILD}")
        string(TOLOWER "macOS" ASTCENC_PKG_OS)
    else()
        string(TOLOWER ${CMAKE_SYSTEM_NAME} ASTCENC_PKG_OS)
    endif()

    set(CPACK_PACKAGE_FILE_NAME "astcenc-${CMAKE_PROJECT_VERSION}-${ASTCENC_PKG_OS}-${ASTCENC_PACKAGE}")
    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)
    set(CPACK_PACKAGE_CHECKSUM SHA256)
    set(CPACK_GENERATOR ZIP)

    include(CPack) # Must be included after CPack configuration.
endif()

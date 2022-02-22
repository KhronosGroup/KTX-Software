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

if(MSVC)
    add_compile_options("/wd4324") # Disable structure was padded due to alignment specifier
endif()

project(astcencoder VERSION 3.3.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(PACKAGE_ROOT astcenc)

include(CTest)

option(ISA_AVX2 "Enable builds for AVX2 SIMD")
option(ISA_SSE41 "Enable builds for SSE4.1 SIMD")
option(ISA_SSE2 "Enable builds for SSE2 SIMD")
option(ISA_NEON "Enable builds for NEON SIMD")
option(ISA_NONE "Enable builds for no SIMD")
option(ISA_NATIVE "Enable builds for native SIMD")
option(DECOMPRESSOR "Enable builds for decompression only")
option(DIAGNOSTICS "Enable builds for diagnostic trace")
option(UNITTEST "Enable builds for unit tests")
option(CLI "Enable build of CLI" ON)

set(UNIVERSAL_BUILD OFF)
set(MACOS_BUILD OFF)
set(MACOS_ARCH_LEN 0)

# Preflight for some macOS-specific build options
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    set(MACOS_BUILD ON)
    list(LENGTH CMAKE_OSX_ARCHITECTURES MACOS_ARCH_LEN)
endif()

# Count options which MUST be x64
set(X64_ISA_COUNT 0)
set(CONFIGS ${ISA_AVX2} ${ISA_SSE41} ${ISA_SSE2})
foreach(CONFIG ${CONFIGS})
    if(${CONFIG})
        math(EXPR X64_ISA_COUNT "${X64_ISA_COUNT} + 1")
    endif()
endforeach()

# Count options which MUST be arm64
set(ARM64_ISA_COUNT 0)
set(CONFIGS ${ISA_NEON})
foreach(CONFIG ${CONFIGS})
    if(${CONFIG})
        math(EXPR ARM64_ISA_COUNT "${ARM64_ISA_COUNT} + 1")
    endif()
endforeach()

# macOS builds
if("${MACOS_BUILD}")
    list(FIND CMAKE_OSX_ARCHITECTURES "x86_64" IS_X64)
    list(FIND CMAKE_OSX_ARCHITECTURES "arm64" IS_ARM64)
    list(FIND CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD)" IS_AUTO)

    # Turn list index into boolean
    if(${IS_X64} EQUAL -1)
        set(IS_X64 OFF)
    else()
        set(IS_X64 ON)
    endif()

    if(${IS_ARM64} EQUAL -1)
        set(IS_ARM64 OFF)
    else()
        set(IS_ARM64 ON)
    endif()

    if(${IS_AUTO} EQUAL -1)
        set(IS_AUTO OFF)
    else()
        set(IS_AUTO ON)
    endif()

    # Set up defaults if no more specific ISA set - use XCode's own defaults
    if((IS_ARM64 OR IS_AUTO) AND ("${ARM64_ISA_COUNT}" EQUAL 0) AND (NOT "${ISA_NONE}"))
        set(ARM64_ISA_COUNT 1)
        set(ISA_NEON ON)
    endif()

    if((IS_X64 OR IS_AUTO) AND ("${X64_ISA_COUNT}" EQUAL 0) AND (NOT "${ISA_NONE}"))
        set(X64_ISA_COUNT 1)
        set(ISA_SSE41 ON)
    endif()

    # User might be doing multi-architecture - XCode sets this at runtime
    if("${IS_AUTO}")
        if(("${ARM64_ISA_COUNT}" GREATER 1) OR ("${X64_ISA_COUNT}" GREATER 1))
            message(FATAL_ERROR "For macOS universal binaries only one backend per architecture is allowed.")
        endif()

        set(UNIVERSAL_BUILD ON)

    # User requested explicit multi-architecture universal build
    elseif("${MACOS_ARCH_LEN}" GREATER 2)
        message(FATAL_ERROR "For macOS universal binaries only x86_64 and arm64 builds are allowed.")

    elseif("${MACOS_ARCH_LEN}" EQUAL 2)
        if(NOT (${IS_X64} AND ${IS_ARM64}))
            message(FATAL_ERROR "For macOS universal binaries only x86_64 and arm64 builds are allowed.")
        endif()

        if(("${ARM64_ISA_COUNT}" GREATER 1) OR ("${X64_ISA_COUNT}" GREATER 1))
            message(FATAL_ERROR "For macOS universal binaries only one backend per architecture is allowed.")
        endif()

        set(UNIVERSAL_BUILD ON)

    # User requested explicit single architecture build
    elseif("${MACOS_ARCH_LEN}" EQUAL 1)
        if("${IS_X64}" AND "${ARM64_ISA_COUNT}")
            message(FATAL_ERROR "For macOS x86_64 builds an arm64 backend cannot be specified.")
        endif()

        if("${IS_ARM64}" AND "${X64_ISA_COUNT}")
            message(FATAL_ERROR "For macOS arm64 builds an x86_64 backend cannot be specified.")
        endif()

    # Else is this a implicit multi-architecture universal build?
    elseif(("${ARM64_ISA_COUNT}" EQUAL 1) AND ("${X64_ISA_COUNT}" GREATER 1))
        string(CONCAT MSG "For macOS setting multiple architecture backends builds a universal binary. "
                          "For universal binaries only one backend per architecture is allowed.")
        message(FATAL_ERROR "${MSG}")

    elseif(("${X64_ISA_COUNT}" EQUAL 1) AND ("${ARM64_ISA_COUNT}" GREATER 1))
        string(CONCAT MSG "For macOS setting multiple architecture backends builds a universal binary. "
                          "For universal binaries only one backend per architecture is allowed.")
        message(FATAL_ERROR "${MSG}")

    elseif(("${ARM64_ISA_COUNT}" EQUAL 1) AND ("${X64_ISA_COUNT}" EQUAL 1))
        set(UNIVERSAL_BUILD ON)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")

    # Else is this an implicit single architecture build?
    elseif("${ARM64_ISA_COUNT}" EQUAL 1)
        set(CMAKE_OSX_ARCHITECTURES "arm64")

    elseif("${X64_ISA_COUNT}" EQUAL 1)
        set(CMAKE_OSX_ARCHITECTURES "x86_64")

    else()
        # Do nothing here - assume it defaults to host?

    endif()

# Non-macOS builds
else()
    if(("${ARM64_ISA_COUNT}" GREATER 0) AND ("${X64_ISA_COUNT}" GREATER 0))
        message(FATAL_ERROR "Builds can only support a single architecture per configure.")
    endif()
endif()

# If nothing more specific is set then fall back on the compiler's defaults
if(("${ARM64_ISA_COUNT}" EQUAL 0) AND ("${X64_ISA_COUNT}" EQUAL 0) AND (NOT "${ISA_NONE}"))
    set(ISA_NATIVE ON)
endif()

function(printopt optName optVal)
    if(${optVal})
        message(STATUS "  ${optName} -  ON")
    else()
        message(STATUS "  ${optName} - OFF")
    endif()
endfunction()

printopt("AVX2 backend   " ${ISA_AVX2})
printopt("SSE4.1 backend " ${ISA_SSE41})
printopt("SSE2 backend   " ${ISA_SSE2})
printopt("NEON backend   " ${ISA_NEON})
printopt("NONE backend   " ${ISA_NONE})
printopt("NATIVE backend " ${ISA_NATIVE})
if("${MACOS_BUILD}")
    printopt("Universal bin  " ${UNIVERSAL_BUILD})
endif()
printopt("Decompressor   " ${DECOMPRESSOR})
printopt("Diagnostics    " ${DIAGNOSTICS})
printopt("Unit tests     " ${UNITTEST})

# Subcomponents
add_subdirectory(Source)

# Configure package archive
if(PACKAGE)
    if("${MACOS_BUILD}")
        string(TOLOWER "macOS" PKG_OS)
    else()
        string(TOLOWER ${CMAKE_SYSTEM_NAME} PKG_OS)
    endif()

    set(PKG_VER ${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR})

    set(CPACK_PACKAGE_FILE_NAME "astcenc-${PKG_VER}-${PKG_OS}-${PACKAGE}")
    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)
    set(CPACK_PACKAGE_CHECKSUM SHA256)
    set(CPACK_GENERATOR ZIP)

    include(CPack) # Must be included after CPack configuration.
endif()

# Copyright 2026 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# libFuzzer targets for libktx, built as a project of their own on top of
# libktx as a sub-project, like tests/use-ktx. See README.md.

cmake_minimum_required(VERSION 3.22)

project(ktx-fuzz
    VERSION 1.0
    DESCRIPTION "libFuzzer targets for libktx"
    LANGUAGES C CXX
)

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "The fuzz targets need Clang: libFuzzer is part of Clang.")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

set(KTX_FUZZ_SANITIZERS "address,undefined" CACHE STRING
    "Sanitizers to build libktx and the fuzz targets with, as for -fsanitize")

# -fsanitize=function reports every call through libktx's C vtable, whose
# entries take ktxTexture* while the implementations take ktxTexture2* or
# ktxTexture1*.
set(fuzz_sanitize_flags
    -fsanitize=${KTX_FUZZ_SANITIZERS}
    -fno-sanitize=function
    -fno-sanitize-recover=all
    -fno-omit-frame-pointer
)
# Instrument libktx for coverage feedback as well as the sanitizers. This must
# be set before libktx is added.
add_compile_options(-fsanitize=fuzzer-no-link ${fuzz_sanitize_flags})
add_link_options(${fuzz_sanitize_flags})

set(BUILD_SHARED_LIBS OFF)
set(LIBKTX_VERSION_READ_ONLY ON CACHE BOOL "" FORCE)
set(LIBKTX_FEATURE_VK_UPLOAD OFF CACHE BOOL "" FORCE)
set(LIBKTX_FEATURE_GL_UPLOAD OFF CACHE BOOL "" FORCE)
add_subdirectory(../../lib ktx)

add_executable(ktx_read_fuzzer
    ktx_read_fuzzer.cc
)
target_compile_features(ktx_read_fuzzer PRIVATE cxx_std_17)
target_link_libraries(ktx_read_fuzzer PRIVATE ktx_read)
# -fsanitize=fuzzer links Clang's libFuzzer runtime, which provides main() and
# calls LLVMFuzzerTestOneInput, so there is no library to add for it.
target_link_options(ktx_read_fuzzer PRIVATE -fsanitize=fuzzer)

add_executable(ktx_info_fuzzer
    ktx_info_fuzzer.cc
)
target_compile_features(ktx_info_fuzzer PRIVATE cxx_std_17)
target_link_libraries(ktx_info_fuzzer PRIVATE ktx_read)
target_link_options(ktx_info_fuzzer PRIVATE -fsanitize=fuzzer)

# The validator of the ktx tool. Only validate.cpp is built, not the tool:
# the validator needs fmt and the headers of the tool, but none of the
# encoders and image libraries the tool links.
set(ktx_root ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(BUILD_SHARED_LIBS OFF)
set(FMT_INSTALL OFF)
set(FMT_SYSTEM_HEADERS ON)
add_subdirectory(${ktx_root}/external/fmt fmt)
add_subdirectory(${ktx_root}/external/cxxopts cxxopts)

add_executable(ktx_validate_fuzzer
    ktx_validate_fuzzer.cc
    ${ktx_root}/tools/ktx/validate.cpp
)
target_compile_features(ktx_validate_fuzzer PRIVATE cxx_std_17)
target_include_directories(ktx_validate_fuzzer PRIVATE
    ${ktx_root}/tools/ktx
    ${ktx_root}/tools/imageio
    ${ktx_root}/utils
    $<TARGET_PROPERTY:ktx_read,INCLUDE_DIRECTORIES>
)
target_include_directories(ktx_validate_fuzzer SYSTEM PRIVATE
    ${ktx_root}/lib/src
    ${ktx_root}/other_include
)
target_compile_definitions(ktx_validate_fuzzer PRIVATE
    $<TARGET_PROPERTY:ktx_read,INTERFACE_COMPILE_DEFINITIONS>
)
# The validator converts header fields such as supercompressionScheme and
# vkFormat to the C enums of ktx.h before checking them. Values out of the
# range of such an enum are undefined in C++, but every 32-bit value is a
# possible field value and is reported as invalid. The tool is not built
# with -fstrict-enums, so the conversions are not checked here: they would
# end every run on the first file with an unknown scheme.
target_compile_options(ktx_validate_fuzzer PRIVATE -fno-sanitize=enum)
target_link_libraries(ktx_validate_fuzzer PRIVATE ktx_read fmt::fmt cxxopts::cxxopts)
target_link_options(ktx_validate_fuzzer PRIVATE -fsanitize=fuzzer)

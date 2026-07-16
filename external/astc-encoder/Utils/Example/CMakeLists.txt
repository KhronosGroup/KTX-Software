#  SPDX-License-Identifier: Apache-2.0
#  ----------------------------------------------------------------------------
#  Copyright 2021-2026 Arm Limited
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

include(ExternalProject)

project(astcencoder_example VERSION 1.1.0)

set(ASTCENC_VERSION "main" CACHE STRING "ASTCEncoder git version to import")

# Add the external project and pull out the project directories we need

# The default build is a native build which supports the highest level of SIMD
# exposed by the compiler when using default compiler flags. Add a single
# SIMD enable to the CMAKE_CACHE_ARGS option to force something specific, but
# remember to change the link library in target_link_libraries() to match.
#
#  *  Add "-DASTCENC_ISA_SSE2:String=ON" and link against "astcenc-sse2-static"
#  *  Add "-DASTCENC_ISA_SSE41:String=ON" and link against "astcenc-sse4.1-static"
#  *  Add "-DASTCENC_ISA_AVX2:String=ON" and link against "astcenc-avx2-static"
#  *  Add "-DASTCENC_ISA_NEON:String=ON" and link against "astcenc-neon-static"
#
# The DOWNLOAD_COMMAND here is slightly convoluted so that we can point the
# example build at GitHub PRs in our Actions pipelines. Normal users can remove
# the DOWNLOAD_COMMAND and UPDATE_COMMAND and set GIT_TAG instead.
ExternalProject_Add(astcencoder
    GIT_REPOSITORY https://github.com/ARM-software/astc-encoder
    CMAKE_CACHE_ARGS -DASTCENC_CLI:STRING=OFF -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
    DOWNLOAD_COMMAND
      git clone https://github.com/ARM-software/astc-encoder <SOURCE_DIR> &&
      cd <SOURCE_DIR> &&
      git fetch origin ${ASTCENC_VERSION} &&
      git checkout --detach FETCH_HEAD
    UPDATE_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_property(astcencoder
    SOURCE_DIR)

ExternalProject_Get_property(astcencoder
    BINARY_DIR)

# Build the command line
add_executable(astcenc_example astc_api_example.cpp)

# ... with astcencoder as a dependency
add_dependencies(astcenc_example astcencoder)

# ... with astcencoder Source dir on the include path
target_include_directories(astcenc_example
    PRIVATE
        ${SOURCE_DIR}/Source)

# ... with astcencoder Binary dir on the library path and as a library dep
target_link_directories(astcenc_example
    PRIVATE
        ${BINARY_DIR}/Source)

target_link_libraries(astcenc_example
    PRIVATE
        astcenc-native-static)

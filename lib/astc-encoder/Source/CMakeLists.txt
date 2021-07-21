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

# Overwrite the LTO flags to force fat LTO; worth 3-4% performance
# See https://gitlab.kitware.com/cmake/cmake/-/issues/16808
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND ${CLI})
    set(CMAKE_CXX_COMPILE_OPTIONS_IPO "-flto")
endif()

if(${DECOMPRESSOR})
    set(CODEC dec)
else()
    set(CODEC enc)
endif()

if(${UNIVERSAL_BUILD})
    if(${ISA_AVX2})
        set(ISA_SIMD "avx2")
    elseif(${ISA_SSE41})
        set(ISA_SIMD "sse4.1")
    elseif(${ISA_SSE2})
        set(ISA_SIMD "sse2")
    endif()
    include(cmake_core.cmake)
else()
    set(ARTEFACTS native none neon avx2 sse4.1 sse2)
    set(CONFIGS ${ISA_NATIVE} ${ISA_NONE} ${ISA_NEON} ${ISA_AVX2} ${ISA_SSE41} ${ISA_SSE2})
    list(LENGTH ARTEFACTS ARTEFACTS_LEN)
    math(EXPR ARTEFACTS_LEN "${ARTEFACTS_LEN} - 1")

    foreach(INDEX RANGE ${ARTEFACTS_LEN})
        list(GET ARTEFACTS ${INDEX} ARTEFACT)
        list(GET CONFIGS ${INDEX} CONFIG)
        if(${CONFIG})
            set(ISA_SIMD ${ARTEFACT})
            include(cmake_core.cmake)
        endif()
    endforeach()
endif()

# - - - - - - - - - - - - - - - - - -
# Unit testing
if(${UNITTEST})
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    add_subdirectory(GoogleTest)
    enable_testing()
    add_subdirectory(UnitTest)
endif()

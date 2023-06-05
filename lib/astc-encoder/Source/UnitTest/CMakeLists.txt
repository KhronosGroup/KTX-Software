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

if(${ASTCENC_UNIVERSAL_BUILD})
    if(${ASTCENC_ISA_AVX2})
        set(ASTCENC_ISA_SIMD "avx2")
    elseif(${ASTCENC_ISA_SSE41})
        set(ASTCENC_ISA_SIMD "sse4.1")
    elseif(${ASTCENC_ISA_SSE2})
        set(ASTCENC_ISA_SIMD "sse2")
    endif()
    include(cmake_core.cmake)
else()
    set(ASTCENC_ARTIFACTS native none neon avx2 sse4.1 sse2)
    set(ASTCENC_CONFIGS ${ASTCENC_ISA_NATIVE} ${ASTCENC_ISA_NONE} ${ASTCENC_ISA_NEON} ${ASTCENC_ISA_AVX2} ${ASTCENC_ISA_SSE41} ${ASTCENC_ISA_SSE2})
    list(LENGTH ASTCENC_ARTIFACTS ASTCENC_ARTIFACTS_LEN)
    math(EXPR ASTCENC_ARTIFACTS_LEN "${ASTCENC_ARTIFACTS_LEN} - 1")

    foreach(INDEX RANGE ${ASTCENC_ARTIFACTS_LEN})
        list(GET ASTCENC_ARTIFACTS ${INDEX} ASTCENC_ARTIFACT)
        list(GET ASTCENC_CONFIGS ${INDEX} ASTCENC_CONFIG)
        if(${ASTCENC_CONFIG})
            set(ASTCENC_ISA_SIMD ${ASTCENC_ARTIFACT})
            include(cmake_core.cmake)
        endif()
    endforeach()
endif()

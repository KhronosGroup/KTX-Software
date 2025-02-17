#  SPDX-License-Identifier: Apache-2.0
#  ----------------------------------------------------------------------------
#  Copyright 2020-2025 Arm Limited
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

set(ASTCENC_TEST test-unit-${ASTCENC_ISA_SIMD})

add_executable(${ASTCENC_TEST})

# Enable LTO under the conditions where the codec library will use LTO.
# The library link will fail if the settings don't match
if(${ASTCENC_CLI})
    set_property(TARGET ${ASTCENC_TEST}
        PROPERTY
            INTERPROCEDURAL_OPTIMIZATION_RELEASE True)
endif()

# Use a static runtime on MSVC builds (ignored on non-MSVC compilers)
set_property(TARGET ${ASTCENC_TEST}
    PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_sources(${ASTCENC_TEST}
    PRIVATE
        test_simd.cpp
        test_softfloat.cpp
        test_decode.cpp)

target_include_directories(${ASTCENC_TEST}
    PRIVATE
        ${gtest_SOURCE_DIR}/include)

target_link_libraries(${ASTCENC_TEST}
    PRIVATE
        astcenc-${ASTCENC_ISA_SIMD}-static)

target_compile_options(${ASTCENC_TEST}
    PRIVATE
        # Use pthreads on Linux/macOS
        $<$<PLATFORM_ID:Linux,Darwin>:-pthread>

        # MSVC compiler defines
        $<$<CXX_COMPILER_ID:MSVC>:/EHsc>

        # G++ and Clang++ compiler defines
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wextra>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wpedantic>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wshadow>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wno-c++98-compat-pedantic>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wno-c++98-c++11-compat-pedantic>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wno-float-equal>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wno-overriding-option>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wno-unsafe-buffer-usage>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wno-switch-default>

        # Ignore things that the googletest build triggers
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wno-unknown-warning-option>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wno-double-promotion>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wno-undef>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wno-reserved-identifier>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wno-global-constructors>)

# Set up configuration for SIMD ISA builds
if(${ASTCENC_ISA_SIMD} MATCHES "none")
    target_compile_definitions(${ASTCENC_TEST}
        PRIVATE
            ASTCENC_NEON=0
            ASTCENC_SVE=0
            ASTCENC_SSE=0
            ASTCENC_AVX=0
            ASTCENC_POPCNT=0
            ASTCENC_F16C=0)

elseif(${ASTCENC_ISA_SIMD} MATCHES "neon")
    target_compile_definitions(${ASTCENC_TEST}
        PRIVATE
            ASTCENC_NEON=1
            ASTCENC_SVE=0
            ASTCENC_SSE=0
            ASTCENC_AVX=0
            ASTCENC_POPCNT=0
            ASTCENC_F16C=0)

elseif(${ASTCENC_ISA_SIMD} MATCHES "sve_256")
    target_compile_definitions(${ASTCENC_TEST}
        PRIVATE
            ASTCENC_NEON=1
            ASTCENC_SVE=8
            ASTCENC_SSE=0
            ASTCENC_AVX=0
            ASTCENC_POPCNT=0
            ASTCENC_F16C=0)

    # Enable SVE
    target_compile_options(${ASTCENC_TEST}
        PRIVATE
            -march=armv8-a+sve -msve-vector-bits=256)

elseif(${ASTCENC_ISA_SIMD} MATCHES "sve_128")
    target_compile_definitions(${ASTCENC_TEST}
        PRIVATE
            ASTCENC_NEON=1
            ASTCENC_SVE=4
            ASTCENC_SSE=0
            ASTCENC_AVX=0
            ASTCENC_POPCNT=0
            ASTCENC_F16C=0)

    # Enable SVE
    target_compile_options(${ASTCENC_TEST}
        PRIVATE
            -march=armv8-a+sve)

elseif(${ASTCENC_ISA_SIMD} MATCHES "sse2")
    target_compile_definitions(${ASTCENC_TEST}
        PRIVATE
            ASTCENC_NEON=0
            ASTCENC_SVE=0
            ASTCENC_SSE=20
            ASTCENC_AVX=0
            ASTCENC_POPCNT=0
            ASTCENC_F16C=0)

    target_compile_options(${ASTCENC_TEST}
        PRIVATE
        $<$<CXX_COMPILER_ID:${GNU_LIKE}>:-msse2>)

elseif(${ASTCENC_ISA_SIMD} MATCHES "sse4.1")
    target_compile_definitions(${ASTCENC_TEST}
        PRIVATE
            ASTCENC_NEON=0
            ASTCENC_SVE=0
            ASTCENC_SSE=41
            ASTCENC_AVX=0
            ASTCENC_POPCNT=1
            ASTCENC_F16C=0)

    target_compile_options(${ASTCENC_TEST}
        PRIVATE
            $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-msse4.1 -mpopcnt>)

elseif(${ASTCENC_ISA_SIMD} MATCHES "avx2")
    target_compile_definitions(${ASTCENC_TEST}
        PRIVATE
            ASTCENC_NEON=0
            ASTCENC_SVE=0
            ASTCENC_SSE=41
            ASTCENC_AVX=2
            ASTCENC_POPCNT=1
            ASTCENC_F16C=1)

    target_compile_options(${ASTCENC_TEST}
        PRIVATE
            $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-mavx2 -mpopcnt -mf16c>
            $<$<CXX_COMPILER_ID:MSVC>:/arch:AVX2>)

endif()

target_link_libraries(${ASTCENC_TEST}
    PRIVATE
        gtest_main)

add_test(NAME ${ASTCENC_TEST}
         COMMAND ${ASTCENC_TEST})

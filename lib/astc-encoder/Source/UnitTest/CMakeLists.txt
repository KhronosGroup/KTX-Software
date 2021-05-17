#  SPDX-License-Identifier: Apache-2.0
#  ----------------------------------------------------------------------------
#  Copyright 2020 Arm Limited
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

# - - - - - - - - - - - - - - - - - -
# No architecture-specific SIMD

if (${ISA_NONE})
    set(ISA_SIMD none)
    include(cmake_core.cmake)
endif()

# - - - - - - - - - - - - - - - - - -
# Armv8-A architecture-specific SIMD

if (${ISA_NEON})
    set(ISA_SIMD neon)
    include(cmake_core.cmake)
endif()

# - - - - - - - - - - - - - - - - - -
# x86-64 architecture-specific SIMD

if (${ISA_AVX2})
    set(ISA_SIMD avx2)
    include(cmake_core.cmake)
endif()

if (${ISA_SSE41})
    set(ISA_SIMD sse4.1)
    include(cmake_core.cmake)
endif()

if (${ISA_SSE2})
    set(ISA_SIMD sse2)
    include(cmake_core.cmake)
endif()

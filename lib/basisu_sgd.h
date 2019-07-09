/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab textwidth=70: */

/*
 * ©2019 Khronos Group, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @internal
 * @file basisu_sgd.h
 * @~English
 *
 * @brief Declare global data for Basis Universal supercompression.
 *
 * These functions are private and should not be used outside the library.
 */

#ifndef _BASISU_SGD_H_
#define _BASISU_SGD_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum sliceflags { KTX_BU_SLICE_HAS_ALPHA, KTX_BU_SLICE_ETC1S };
typedef struct ktxBasisGlobalHeader {
    uint32_t globalFlags;
    uint16_t endpointCount;
    uint16_t selectorCount;
    uint32_t endpointsByteOffset;  // Offset from start of sgd
    uint32_t endpointsByteLength;
    uint32_t selectorsByteOffset;  // Offset from start of sgd
    uint32_t selectorsByteLength;
    uint32_t tablesByteOffset;     // Offset from start of sgd
    uint32_t tablesByteLength;
    uint32_t extendedByteOffset;   // Offset from start of sgd
    uint32_t extendedByteLength;
} ktxBasisGlobalHeader;

// 1, or 2, slices per layer, face & slice.
// These offsets are relative to start of a mip level as given by the
// main levelIndex. So there is one of these indices per level.
#define SLICE_DESC_BASE_DEFN \
      uint32_t sliceFlags; \
      uint32_t sliceByteOffset; \
      uint32_t sliceByteLength;

typedef struct ktxBasisBaseSliceDesc {
    SLICE_DESC_BASE_DEFN
} ktxBasisBaseSliceDesc;

// This description is used when globalFlags & alpha != 0.
typedef struct ktxBasisSliceDesc {
    SLICE_DESC_BASE_DEFN
    uint32_t alphaSliceByteOffset;
    uint32_t alphaSliceByteLength;
} ktxBasisSliceDesc;

#define BGD_SLICE_DESCS(sgd) \
                    (ktxBasisSliceDesc*)(sgd + sizeof(ktxBasisGlobalHeader))

// The header is followed in the global data by an array of slice descriptions,
// then these.
//    uint8_t[endpointsByteLength] endpointsData;
//    uint8_t[selectorsByteLength] selectorsData;
//    uint8_t[tablesByteLength] tablesData;

#ifdef __cplusplus
}
#endif

#endif /* _BASISU_SGD_H_ */

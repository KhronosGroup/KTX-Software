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
    uint32_t endpointsByteLength;
    uint32_t selectorsByteLength;
    uint32_t tablesByteLength;
    uint32_t extendedByteLength;
} ktxBasisGlobalHeader;

// This header is followed by imageCount "slice" descriptions.

// 1, or 2 slices per image (i.e. layer, face & slice).
// These offsets are relative to start of a mip level as given by the
// main levelIndex.
typedef struct ktxBasisSliceDesc {
    uint32_t sliceFlags;
    uint32_t sliceByteOffset;
    uint32_t sliceByteLength;
    uint32_t alphaSliceByteOffset;
    uint32_t alphaSliceByteLength;
} ktxBasisSliceDesc;

#define BGD_SLICE_DESCS(bgd) \
        reinterpret_cast<ktxBasisSliceDesc*>(bgd + sizeof(ktxBasisGlobalHeader))

// The are followed in the global data by these ...
//    uint8_t[endpointsByteLength] endpointsData;
//    uint8_t[selectorsByteLength] selectorsData;
//    uint8_t[tablesByteLength] tablesData;

#define BGD_ENDPOINTS_ADDR(bgd, imageCount) \
    (bgd + sizeof(ktxBasisGlobalHeader) + sizeof(ktxBasisSliceDesc) * imageCount)

#define BGD_SELECTORS_ADDR(bgd, bgdh, imageCount) (BGD_ENDPOINTS_ADDR(bgd, imageCount) + bgdh.endpointsByteLength)

#define BGD_TABLES_ADDR(bgd, bgdh, imageCount) (BGD_SELECTORS_ADDR(bgd, bgdh, imageCount) + bgdh.selectorsByteLength)

#define BGD_EXTENDED_ADDR(bgd, bgdh, imageCount) (BGD_TABLES_ADDR(bgd, bgdh, imageCount) + bgdh.tablesByteLength)

#ifdef __cplusplus
}
#endif

#endif /* _BASISU_SGD_H_ */

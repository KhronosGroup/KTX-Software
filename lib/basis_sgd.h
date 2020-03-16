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

#ifndef _BASIS_SGD_H_
#define _BASIS_SGD_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// These must be the same values as cBASISHeaderFlagETC1S and
// cBASISHeaderFlagHasAlphaSlices respectively. As they are within a C
// namespace they can't easily be accessed from a c header.
enum bu_global_flag_bits_e { eBuIsETC1S = 0x01, eBUHasAlphaSlices = 0x04 };
// And this must be the same value as cSliceDescFlagsFrameIsIFrame.
enum bu_image_flags__bits_e { eBUImageIsIframe = 0x02 };

typedef uint32_t buFlags;

typedef struct ktxBasisGlobalHeader {
    buFlags globalFlags;
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
typedef struct ktxBasisImageDesc {
    buFlags imageFlags;
    uint32_t rgbSliceByteOffset;
    uint32_t rgbSliceByteLength;
    uint32_t alphaSliceByteOffset;
    uint32_t alphaSliceByteLength;
} ktxBasisImageDesc;

#define BGD_IMAGE_DESCS(bgd) \
        reinterpret_cast<ktxBasisImageDesc*>(bgd + sizeof(ktxBasisGlobalHeader))

// The are followed in the global data by these ...
//    uint8_t[endpointsByteLength] endpointsData;
//    uint8_t[selectorsByteLength] selectorsData;
//    uint8_t[tablesByteLength] tablesData;

#define BGD_ENDPOINTS_ADDR(bgd, imageCount) \
    (bgd + sizeof(ktxBasisGlobalHeader) + sizeof(ktxBasisImageDesc) * imageCount)

#define BGD_SELECTORS_ADDR(bgd, bgdh, imageCount) (BGD_ENDPOINTS_ADDR(bgd, imageCount) + bgdh.endpointsByteLength)

#define BGD_TABLES_ADDR(bgd, bgdh, imageCount) (BGD_SELECTORS_ADDR(bgd, bgdh, imageCount) + bgdh.selectorsByteLength)

#define BGD_EXTENDED_ADDR(bgd, bgdh, imageCount) (BGD_TABLES_ADDR(bgd, bgdh, imageCount) + bgdh.tablesByteLength)

#ifdef __cplusplus
}
#endif

#endif /* _BASIS_SGD_H_ */

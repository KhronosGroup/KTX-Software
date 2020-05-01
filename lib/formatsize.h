/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright (c) 2019 The Khronos Group Inc.
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
 * @file
 * @~English
 *
 * @brief Struct for returning size information about an image format.
 *
 * @author Mark Callow, www.edgewise-consulting.com
 */

#ifndef _FORMATSIZE_H_
#define _FORMATSIZE_H_

#include "ktx.h"

typedef enum ktxFormatSizeFlagBits {
    KTX_FORMAT_SIZE_PACKED_BIT                = 0x00000001,
    KTX_FORMAT_SIZE_COMPRESSED_BIT            = 0x00000002,
    KTX_FORMAT_SIZE_PALETTIZED_BIT            = 0x00000004,
    KTX_FORMAT_SIZE_DEPTH_BIT                 = 0x00000008,
    KTX_FORMAT_SIZE_STENCIL_BIT               = 0x00000010,
} ktxFormatSizeFlagBits;

typedef ktx_uint32_t ktxFormatSizeFlags;

/**
 * @brief Structure for holding size information for a texture format.
 */
typedef struct ktxFormatSize {
    ktxFormatSizeFlags  flags;
    unsigned int        paletteSizeInBits;  // For KTX1.
    unsigned int        blockSizeInBits;
    unsigned int        blockWidth;         // in texels
    unsigned int        blockHeight;        // in texels
    unsigned int        blockDepth;         // in texels
} ktxFormatSize;

#ifdef __cplusplus
extern "C" {
#endif

bool ktxFormatSize_initFromDfd(ktxFormatSize* This, ktx_uint32_t* pDfd);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _FORMATSIZE_H_ */

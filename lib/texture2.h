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
 * @file texture2.h
 * @~English
 *
 * @brief Declare internal ktxTexture2 functions for sharing between
 *        compilation units.
 *
 * These functions are private and should not be used outside the library.
 */

#ifndef _TEXTURE2_H_
#define _TEXTURE2_H_

#include "texture.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CLASS ktxTexture2
#include "texture_funcs.inl"
#undef CLASS

typedef struct ktxTexture2_private {
    ktx_uint8_t* _supercompressionGlobalData;
    ktx_uint32_t _requiredLevelAlignment;
    ktx_uint64_t _sgdByteLength;
    ktx_uint64_t _firstLevelFileOffset; /*!< Always 0, unless the texture was
                                         created from a stream and the image
                                         data is not yet loaded. */
    // Must be last so it can grow.
    ktxLevelIndexEntry _levelIndex[1]; /*!< Offsets in this index are from the
                                        start of the image data. Use
                                        ktxTexture_levelStreamOffset() and
                                        ktxTexture_levelDataOffset(). The former
                                        will add the above file offset to the
                                        index offset. */
} ktxTexture2_private;

KTX_error_code
ktxTexture2_LoadImageData(ktxTexture2* This,
                          ktx_uint8_t* pBuffer, ktx_size_t bufSize);

KTX_error_code
ktxTexture2_constructFromStreamAndHeader(ktxTexture2* This, ktxStream* pStream,
                                         KTX_header2* pHeader,
                                         ktxTextureCreateFlags createFlags);

ktx_uint64_t ktxTexture2_calcDataSizeTexture(ktxTexture2* This);
ktx_size_t ktxTexture2_calcLevelOffset(ktxTexture2* This, ktx_uint32_t level);
ktx_uint32_t ktxTexture2_calcRequiredLevelAlignment(ktxTexture2* This);
ktx_uint64_t ktxTexture2_levelFileOffset(ktxTexture2* This, ktx_uint32_t level);
ktx_uint64_t ktxTexture2_levelDataOffset(ktxTexture2* This, ktx_uint32_t level);

#ifdef __cplusplus
}
#endif

#endif /* _TEXTURE2_H_ */

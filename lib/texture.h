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
 * @file texture.h
 * @~English
 *
 * @brief Declare internal ktxTexture functions for sharing between
 *        compilation units.
 *
 * These functions are private and should not be used outside the library.
 */

#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include "ktx.h"
#include "formatsize.h"
#include "stream.h"

#define DECLARE_SUPER(baseClass) baseClass* super = (baseClass *)This
#define DECLARE_PRIVATE(class) struct class ## _private* private = This->_private
#define DECLARE_PROTECTED(class) struct class ## _protected* prtctd = This->_protected;

/**
 * @memberof ktxTexture
 * @~English
 *
 * @brief private members of ktxTexture.
 */
struct ktxTexture_protected {
    ktxFormatSize _formatSize;
    ktxStream _stream;
};

/*
 ======================================
     Internal ktxTexture functions
 ======================================
*/

#define ktxTexture_getStream(t) ((ktxStream*)(&(t)->_protected->_stream))
#define ktxTexture1_getStream(t1) ktxTexture_getStream((ktxTexture*)t1)
#define ktxTexture2_getStream(t2) ktxTexture_getStream((ktxTexture*)t2)

typedef enum {
    KTX_FORMAT_VERSION_ONE = 1,
    KTX_FORMAT_VERSION_TWO = 2
} ktxFormatVersionEnum;

KTX_error_code
ktxTexture_iterateLoadedImages(ktxTexture* This, PFNKTXITERCB iterCb,
                               void* userdata);
KTX_error_code
ktxTexture_iterateSourceImages(ktxTexture* This, PFNKTXITERCB iterCb,
                               void* userdata);

ktx_size_t ktxTexture_calcDataSizeTexture(ktxTexture* This,
                                          ktxFormatVersionEnum fv);
ktx_size_t ktxTexture_calcImageSize(ktxTexture* This, ktx_uint32_t level,
                                    ktxFormatVersionEnum fv);
ktx_bool_t ktxTexture_isActiveStream(ktxTexture* This);
ktx_size_t ktxTexture_calcLevelOffset(ktxTexture* This, ktx_uint32_t level,
                                  ktxFormatVersionEnum fv);
ktx_size_t ktxTexture_calcLevelSize(ktxTexture* This, ktx_uint32_t level,
                                    ktxFormatVersionEnum fv);
ktx_size_t ktxTexture_calcFaceLodSize(ktxTexture* This, ktx_uint32_t level,
                                      ktxFormatVersionEnum fv);
void ktxTexture_rowInfo(ktxTexture* This, ktx_uint32_t level,
                        ktx_uint32_t* numRows, ktx_uint32_t* rowBytes,
                        ktx_uint32_t* rowPadding);
KTX_error_code
ktxTexture_construct(ktxTexture* This, ktxTextureCreateInfo* createInfo,
                     ktxFormatSize* formatSize,
                     ktxTextureCreateStorageEnum storageAllocation);

KTX_error_code
ktxTexture_constructFromStream(ktxTexture* This, ktxStream* pStream,
                               ktxTextureCreateFlags createFlags);

void
ktxTexture_destruct(ktxTexture* This);

/*
 ======================================
     Virtual ktxTexture1 functions
 ======================================
*/

/* FIXME Should these be callable by applications? */

void ktxTexture1_Destroy(ktxTexture1* This);
KTX_error_code ktxTexture1_GLUpload(ktxTexture1* This, GLuint* pTexture,
                                    GLenum* pTarget, GLenum* pGlerror);
KTX_error_code ktxTexture1_IterateLevelFaces(ktxTexture1* This,
                                             PFNKTXITERCB iterCb,
                                             void* userdata);
KTX_error_code ktxTexture1_IterateLoadLevelFaces(ktxTexture1* This,
                                                 PFNKTXITERCB iterCb,
                                                 void* userdata);


/*
 ======================================
     Internal ktxTexture1 functions
 ======================================
*/

KTX_error_code
ktxTexture1_constructFromStreamAndHeader(ktxTexture1* This, ktxStream* pStream,
                                         KTX_header* pHeader,
                                         ktxTextureCreateFlags createFlags);

void ktxTexture1_destruct(ktxTexture1* This);

ktx_uint32_t ktxTexture1_glTypeSize(ktxTexture1* This);

/*
 ======================================
     Internal ktxTexture2 functions
 ======================================
*/

KTX_error_code
ktxTexture2_constructFromStreamAndHeader(ktxTexture2* This, ktxStream* pStream,
                                         KTX_header2* pHeader,
                                         ktxTextureCreateFlags createFlags);

#endif /* _TEXTURE_H_ */

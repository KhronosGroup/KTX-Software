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
 * @file texture_funcs.h
 * @~English
 *
 * @brief Templates for functions common to base & derived ktxTexture classes.
 *
 * Define CLASS before including this file.
 */

#define CAT(c, n) PRIMITIVE_CAT(c, n)
#define PRIMITIVE_CAT(c, n) c ## _ ## n

#define CLASS_FUNC(name) CAT(CLASS, name)

/*
 ======================================
     Virtual ktxTexture functions
 ======================================
*/


void CLASS_FUNC(Destroy)(CLASS* This);
KTX_error_code CLASS_FUNC(GetImageOffset)(CLASS* This, ktx_uint32_t level,
                                          ktx_uint32_t layer,
                                          ktx_uint32_t faceSlice,
                                          ktx_size_t* pOffset);
ktx_size_t CLASS_FUNC(GetImageSize)(CLASS* This, ktx_uint32_t level);
KTX_error_code CLASS_FUNC(GLUpload)(CLASS* This, GLuint* pTexture,
                                    GLenum* pTarget, GLenum* pGlerror);
KTX_error_code CLASS_FUNC(IterateLevelFaces)(CLASS* This,
                                             PFNKTXITERCB iterCb,
                                             void* userdata);
KTX_error_code CLASS_FUNC(IterateLoadLevelFaces)(CLASS* This,
                                                 PFNKTXITERCB iterCb,
                                                 void* userdata);
KTX_error_code CLASS_FUNC(SetImageFromStdioStream)(CLASS* This,
                                    ktx_uint32_t level,ktx_uint32_t layer,
                                    ktx_uint32_t faceSlice,
                                    FILE* src, ktx_size_t srcSize);
KTX_error_code CLASS_FUNC(SetImageFromMemory)(CLASS* This,
                               ktx_uint32_t level, ktx_uint32_t layer,
                               ktx_uint32_t faceSlice,
                               const ktx_uint8_t* src, ktx_size_t srcSize);

KTX_error_code CLASS_FUNC(WriteToStdioStream)(CLASS* This, FILE* dstsstr);
KTX_error_code CLASS_FUNC(WriteToNamedFile)(CLASS* This,
                                             const char* const dstname);
KTX_error_code CLASS_FUNC(WriteToMemory)(CLASS* This,
                          ktx_uint8_t** ppDstBytes, ktx_size_t* pSize);

/*
 ======================================
     Internal ktxTexture functions
 ======================================
*/


void CLASS_FUNC(destruct)(CLASS* This);


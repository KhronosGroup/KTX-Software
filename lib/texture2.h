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

KTX_error_code
ktxTexture2_constructFromStreamAndHeader(ktxTexture2* This, ktxStream* pStream,
                                         KTX_header2* pHeader,
                                         ktxTextureCreateFlags createFlags);

#ifdef __cplusplus
}
#endif

#endif /* _TEXTURE2_H_ */

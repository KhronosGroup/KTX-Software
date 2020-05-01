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
 * @file basisu_transcoder_config.h
 * @~English
 *
 * @brief Set up which Basis transcoders are included.
 *
 * @warning These values need to match the same definitions in
 *          basisu/transcoder/basisu_transcoder.cpp to truly omit the code from a build.
 */

#ifndef _BASIS_TRANSCODER_CONFIG_H_
#define _BASIS_TRANSCODER_CONFIG_H_

#ifndef BASISD_SUPPORT_DXT1
#define BASISD_SUPPORT_DXT1 1
#endif

#ifndef BASISD_SUPPORT_DXT5A
#define BASISD_SUPPORT_DXT5A 1
#endif

// Disable all BC7 transcoders if necessary (useful when cross compiling
// to WebAsm)
#if defined(BASISD_SUPPORT_BC7) && !BASISD_SUPPORT_BC7
  #ifndef BASISD_SUPPORT_BC7_MODE5
  #define BASISD_SUPPORT_BC7_MODE5 0
  #endif
#endif // !BASISD_SUPPORT_BC7

// BC7 mode 5 supports both opaque and opaque+alpha textures, and uses
// substantially less memory than BC1.
#ifndef BASISD_SUPPORT_BC7_MODE5
#define BASISD_SUPPORT_BC7_MODE5 1
#endif

#ifndef BASISD_SUPPORT_PVRTC1
#define BASISD_SUPPORT_PVRTC1 1
#endif

#ifndef BASISD_SUPPORT_ETC2_EAC_A8
#define BASISD_SUPPORT_ETC2_EAC_A8 1
#endif

// Set BASISD_SUPPORT_UASTC to 0 to completely disable support for transcoding
// UASTC files.
#ifndef BASISD_SUPPORT_UASTC
#define BASISD_SUPPORT_UASTC 1
#endif

#ifndef BASISD_SUPPORT_ASTC
#define BASISD_SUPPORT_ASTC 1
#endif

// Note that if BASISD_SUPPORT_ATC is enabled, BASISD_SUPPORT_DXT5A should also
// be enabled for alpha support.
#ifndef BASISD_SUPPORT_ATC
#define BASISD_SUPPORT_ATC 1
#endif

// Support for ETC2 EAC R11 and ETC2 EAC RG11
#ifndef BASISD_SUPPORT_ETC2_EAC_RG11
#define BASISD_SUPPORT_ETC2_EAC_RG11 1
#endif

// If BASISD_SUPPORT_ASTC_HIGHER_OPAQUE_QUALITY is 1, opaque blocks will be
// transcoded to ASTC at slightly higher quality (higher than BC1), but the
// transcoder tables will be 2x as large. This impacts grayscale and
// grayscale+alpha textures the most.
#ifndef BASISD_SUPPORT_ASTC_HIGHER_OPAQUE_QUALITY
  #ifdef __EMSCRIPTEN__
    // Let's assume size matters more than quality when compiling with
    // to WebAsm with emscripten.
    #define BASISD_SUPPORT_ASTC_HIGHER_OPAQUE_QUALITY 0
  #else
    // Compiling native, so an extra 64K lookup table is probably acceptable.
    #define BASISD_SUPPORT_ASTC_HIGHER_OPAQUE_QUALITY 1
  #endif
#endif

#ifndef BASISD_SUPPORT_FXT1
#define BASISD_SUPPORT_FXT1 1
#endif

#ifndef BASISD_SUPPORT_PVRTC2
#define BASISD_SUPPORT_PVRTC2 1
#endif

#if BASISD_SUPPORT_PVRTC2
#if !BASISD_SUPPORT_ATC
#error BASISD_SUPPORT_ATC must be 1 if BASISD_SUPPORT_PVRTC2 is 1
#endif
#endif

#if BASISD_SUPPORT_ATC
#if !BASISD_SUPPORT_DXT5A
#error BASISD_SUPPORT_DXT5A must be 1 if BASISD_SUPPORT_ATC is 1
#endif
#endif


#endif /* _BASIS_TRANSCODER_CONFIG_H_ */

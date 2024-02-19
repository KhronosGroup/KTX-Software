/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab textwidth=70: */

/*
 * Copyright 2024 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file vk2gl.h
 * @~English
 *
 * @brief Get GL format information matching a VkFormat
 *
 */

#ifndef _VK2GL_H_
#define _VK2GL_H_

#include "vkformat_enum.h"
#include "GL/glcorearb.h"

// These are only in glext.h.
#if !defined( GL_COMPRESSED_SRGB_S3TC_DXT1_EXT )
#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT                0x8C4C
#endif
#if !defined( GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT )
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT          0x8C4D
#endif
#if !defined( GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT )
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT          0x8C4E
#endif
#if !defined( GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT )
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT          0x8C4F
#endif

// These are only in GLES headers not in glcorearb.h or glext.h.

//
// ETC
//

#if !defined( GL_ETC1_RGB8_OES )
#define GL_ETC1_RGB8_OES                                0x8D64
#endif

//
// PVRTC
//

#if !defined( GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG )
#define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG              0x8C01
#define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG              0x8C00
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG             0x8C03
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG             0x8C02
#endif
#if !defined( GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG )
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG             0x9137
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG             0x9138
#endif
#if !defined( GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT )
#define GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT             0x8A54
#define GL_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT             0x8A55
#define GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT       0x8A56
#define GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT       0x8A57
#endif
#if !defined( GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV2_IMG )
#define GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV2_IMG       0x93F0
#define GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV2_IMG       0x93F1
#endif

//
// ASTC
//

#if !defined( GL_COMPRESSED_RGBA_ASTC_3x3x3_OES )
#define GL_COMPRESSED_RGBA_ASTC_3x3x3_OES               0x93C0
#define GL_COMPRESSED_RGBA_ASTC_4x3x3_OES               0x93C1
#define GL_COMPRESSED_RGBA_ASTC_4x4x3_OES               0x93C2
#define GL_COMPRESSED_RGBA_ASTC_4x4x4_OES               0x93C3
#define GL_COMPRESSED_RGBA_ASTC_5x4x4_OES               0x93C4
#define GL_COMPRESSED_RGBA_ASTC_5x5x4_OES               0x93C5
#define GL_COMPRESSED_RGBA_ASTC_5x5x5_OES               0x93C6
#define GL_COMPRESSED_RGBA_ASTC_6x5x5_OES               0x93C7
#define GL_COMPRESSED_RGBA_ASTC_6x6x5_OES               0x93C8
#define GL_COMPRESSED_RGBA_ASTC_6x6x6_OES               0x93C9
#endif

#if !defined( GL_COMPRESSED_SRGB8_ALPHA8_ASTC_3x3x3_OES )
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_3x3x3_OES       0x93E0
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x3x3_OES       0x93E1
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x3_OES       0x93E2
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x4_OES       0x93E3
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4x4_OES       0x93E4
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x4_OES       0x93E5
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x5_OES       0x93E6
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5x5_OES       0x93E7
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x5_OES       0x93E8
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x6_OES       0x93E9
#endif

static inline GLenum vkFormat2glInternalFormat( VkFormat vkFormat )
{
    switch ( vkFormat )
    {
#include "vkFormat2glInternalFormat.inl"
        default: return GL_INVALID_VALUE;
    }
}


static inline GLenum vkFormat2glFormat( VkFormat vkFormat )
{
    switch ( vkFormat )
    {
#include "vkFormat2glFormat.inl"
        default: return GL_INVALID_VALUE;
    }
}

static inline GLenum vkFormat2glType( VkFormat vkFormat )
{
    switch ( vkFormat )
    {
#include "vkFormat2glType.inl"
        default: return GL_INVALID_VALUE;
    }
}

#endif /* _VK2GL_H_ */


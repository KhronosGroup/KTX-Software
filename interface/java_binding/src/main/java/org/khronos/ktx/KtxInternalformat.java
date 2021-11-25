/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

public class KtxInternalformat {
    public static final int GL_R3_G3_B2                       = 0x2A10;
    public static final int GL_RGB4                           = 0x804F;
    public static final int GL_RGB5                           = 0x8050;
    public static final int GL_RGB8                           = 0x8051;
    public static final int GL_RGB10                          = 0x8052;
    public static final int GL_RGB12                          = 0x8053;
    public static final int GL_RGB16                          = 0x8054;
    public static final int GL_RGBA2                          = 0x8055;
    public static final int GL_RGBA4                          = 0x8056;
    public static final int GL_RGB5_A1                        = 0x8057;
    public static final int GL_RGBA8                          = 0x8058;
    public static final int GL_RGB10_A2                       = 0x8059;
    public static final int GL_RGBA12                         = 0x805A;
    public static final int GL_RGBA16                         = 0x805B;

    public static final int GL_COMPRESSED_RGB_S3TC_DXT1_EXT   = 0x83F0;
    public static final int GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  = 0x83F1;
    public static final int GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  = 0x83F2;
    public static final int GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  = 0x83F3;

    public static final int GL_SRGB_EXT                       = 0x8C40;
    public static final int GL_SRGB8_EXT                      = 0x8C41;
    public static final int GL_SRGB_ALPHA_EXT                 = 0x8C42;
    public static final int GL_SRGB8_ALPHA8_EXT               = 0x8C43;
    public static final int GL_SLUMINANCE_ALPHA_EXT           = 0x8C44;
    public static final int GL_SLUMINANCE8_ALPHA8_EXT         = 0x8C45;
    public static final int GL_SLUMINANCE_EXT                 = 0x8C46;
    public static final int GL_SLUMINANCE8_EXT                = 0x8C47;
    public static final int GL_COMPRESSED_SRGB_EXT            = 0x8C48;
    public static final int GL_COMPRESSED_SRGB_ALPHA_EXT      = 0x8C49;
    public static final int GL_COMPRESSED_SLUMINANCE_EXT      = 0x8C4A;
    public static final int GL_COMPRESSED_SLUMINANCE_ALPHA_EXT = 0x8C4B;
    public static final int GL_COMPRESSED_SRGB_S3TC_DXT1_EXT = 0x8C4C;
    public static final int GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT = 0x8C4D;
    public static final int GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT = 0x8C4E;
    public static final int GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT = 0x8C4F;

    public static final int GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG  = 0x8C00;
    public static final int GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG  = 0x8C01;
    public static final int GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG = 0x8C02;
    public static final int GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG = 0x8C03;

    public static final int GL_ATC_RGB_AMD = 0x8C92;
    public static final int GL_ATC_RGBA_EXPLICIT_ALPHA_AMD = 0x8C93;
    public static final int GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD = 0x87EE;

    public static final int GL_COMPRESSED_LUMINANCE_LATC1_EXT = 0x8C70;
    public static final int GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT = 0x8C72;
    public static final int GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT = 0x8C71;
    public static final int GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT = 0x8C73;

    public static final int GL_ETC1_RGB8_OES = 0x8D64;

    public static final int GL_COMPRESSED_RGBA_BPTC_UNORM_EXT           = 0x8E8C;
    public static final int GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB     = 0x8E8D;
    public static final int GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB     = 0x8E8E;
    public static final int GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB   = 0x8E8F;

    public static final int GL_COMPRESSED_RGBA8_ETC2_EAC        = 0x9278;
    public static final int GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC = 0x9279;
    public static final int GL_COMPRESSED_R11_EAC               = 0x9270;
    public static final int GL_COMPRESSED_SIGNED_R11_EAC        = 0x9271;
    public static final int GL_COMPRESSED_RG11_EAC              = 0x9272;
    public static final int GL_COMPRESSED_SIGNED_RG11_EAC       = 0x9273;

    public static final int GL_COMPRESSED_RGBA_ASTC_4x4_KHR = 0x93B0;
    public static final int GL_COMPRESSED_RGBA_ASTC_5x4_KHR   = 0x93B1;
    public static final int GL_COMPRESSED_RGBA_ASTC_5x5_KHR   = 0x93B2;
    public static final int GL_COMPRESSED_RGBA_ASTC_6x5_KHR   = 0x93B3;
    public static final int GL_COMPRESSED_RGBA_ASTC_6x6_KHR   = 0x93B4;
    public static final int GL_COMPRESSED_RGBA_ASTC_8x5_KHR   = 0x93B5;
    public static final int GL_COMPRESSED_RGBA_ASTC_8x6_KHR   = 0x93B6;
    public static final int GL_COMPRESSED_RGBA_ASTC_8x8_KHR   = 0x93B7;
    public static final int GL_COMPRESSED_RGBA_ASTC_10x5_KHR  = 0x93B8;
    public static final int GL_COMPRESSED_RGBA_ASTC_10x6_KHR  = 0x93B9;
    public static final int GL_COMPRESSED_RGBA_ASTC_10x8_KHR  = 0x93BA;
    public static final int GL_COMPRESSED_RGBA_ASTC_10x10_KHR = 0x93BB;
    public static final int GL_COMPRESSED_RGBA_ASTC_12x10_KHR = 0x93BC;
    public static final int GL_COMPRESSED_RGBA_ASTC_12x12_KHR = 0x93BD;
    public static final int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR = 0x93D0;
    public static final int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR = 0x93D1;
    public static final int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR = 0x93D2;
    public static final int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR = 0x93D3;
    public static final int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR = 0x93D4;
    public static final int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR = 0x93D5;
    public static final int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR = 0x93D6;
    public static final int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR = 0x93D7;
    public static final int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR = 0x93D8;
    public static final int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR = 0x93D9;
    public static final int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR = 0x93DA;
    public static final int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR = 0x93DB;
    public static final int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR = 0x93DC;
    public static final int GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR = 0x93DD;
}

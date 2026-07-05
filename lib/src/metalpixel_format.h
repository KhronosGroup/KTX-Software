/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab textwidth=70: */

/*
 * Copyright 2026 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

 /**
  * @internal
  * @file
  * @~English
  *
  * @brief Definitions of Metal pixel formats.
  *
  * Obviously this is very similar to the Apple original. It uses the same enumerator
  * and emumeration names to avoid confusion for users of the Metal API.
  */

/*
 * Note: <Metal/MTLPixelFormat.h> does not have an include guard. As there is no intention
 * to expose this alternative .h to libktx users, that will not be an issue.
 */
#ifndef METALPIXEL_FORMAT_H_
#define METALPIXEL_FORMAT_H_

/*
 * Original Obj-C declaration is `typedef NS_ENUM(NSUInteger, MTLPixelFormat);`
 * so use unsigned to match.
 */
typedef enum MTLPixelFormat
{
    MTLPixelFormatInvalid = 0U,

    /* Normal 8 bit formats */
    MTLPixelFormatA8Unorm      = 1U,

    MTLPixelFormatR8Unorm      = 10U,
    MTLPixelFormatR8Unorm_sRGB = 11U,
    MTLPixelFormatR8Snorm      = 12U,
    MTLPixelFormatR8Uint       = 13U,
    MTLPixelFormatR8Sint       = 14U,

    /* Normal 16 bit formats */
    MTLPixelFormatR16Unorm     = 20U,
    MTLPixelFormatR16Snorm     = 22U,
    MTLPixelFormatR16Uint      = 23U,
    MTLPixelFormatR16Sint      = 24U,
    MTLPixelFormatR16Float     = 25U,

    MTLPixelFormatRG8Unorm      = 30U,
    MTLPixelFormatRG8Unorm_sRGB = 31U,
    MTLPixelFormatRG8Snorm      = 32U,
    MTLPixelFormatRG8Uint       = 33U,
    MTLPixelFormatRG8Sint       = 34U,

    /* Packed 16 bit formats */
    MTLPixelFormatB5G6R5Unorm = 40U,
    MTLPixelFormatA1BGR5Unorm = 41U,
    MTLPixelFormatABGR4Unorm  = 42U,
    MTLPixelFormatBGR5A1Unorm = 43U,

    /* Normal 32 bit formats */
    MTLPixelFormatR32Uint  = 53U,
    MTLPixelFormatR32Sint  = 54U,
    MTLPixelFormatR32Float = 55U,

    MTLPixelFormatRG16Unorm  = 60U,
    MTLPixelFormatRG16Snorm  = 62U,
    MTLPixelFormatRG16Uint   = 63U,
    MTLPixelFormatRG16Sint   = 64U,
    MTLPixelFormatRG16Float  = 65U,

    MTLPixelFormatRGBA8Unorm      = 70U,
    MTLPixelFormatRGBA8Unorm_sRGB = 71U,
    MTLPixelFormatRGBA8Snorm      = 72U,
    MTLPixelFormatRGBA8Uint       = 73U,
    MTLPixelFormatRGBA8Sint       = 74U,
    
    MTLPixelFormatBGRA8Unorm      = 80U,
    MTLPixelFormatBGRA8Unorm_sRGB = 81U,

    /* Packed 32 bit formats */
    MTLPixelFormatRGB10A2Unorm  = 90U,
    MTLPixelFormatRGB10A2Uint   = 91U,

    MTLPixelFormatRG11B10Float  = 92U,
    MTLPixelFormatRGB9E5Float   = 93U,

    MTLPixelFormatBGR10A2Unorm  = 94U,

    MTLPixelFormatBGR10_XR      = 554U,
    MTLPixelFormatBGR10_XR_sRGB = 555U,

    /* Normal 64 bit formats */
    MTLPixelFormatRG32Uint       = 103U,
    MTLPixelFormatRG32Sint       = 104U,
    MTLPixelFormatRG32Float      = 105U,

    MTLPixelFormatRGBA16Unorm    = 110U,
    MTLPixelFormatRGBA16Snorm    = 112U,
    MTLPixelFormatRGBA16Uint     = 113U,
    MTLPixelFormatRGBA16Sint     = 114U,
    MTLPixelFormatRGBA16Float    = 115U,

    MTLPixelFormatBGRA10_XR      = 552U,
    MTLPixelFormatBGRA10_XR_sRGB = 553U,

    /* Normal 128 bit formats */
    MTLPixelFormatRGBA32Uint  = 123U,
    MTLPixelFormatRGBA32Sint  = 124U,
    MTLPixelFormatRGBA32Float = 125U,

    /* Compressed formats. */

    /* S3TC/DXT */
    MTLPixelFormatBC1_RGBA              = 130U,
    MTLPixelFormatBC1_RGBA_sRGB         = 131U,
    MTLPixelFormatBC2_RGBA              = 132U,
    MTLPixelFormatBC2_RGBA_sRGB         = 133U,
    MTLPixelFormatBC3_RGBA              = 134U,
    MTLPixelFormatBC3_RGBA_sRGB         = 135U,

    /* RGTC */
    MTLPixelFormatBC4_RUnorm            = 140U,
    MTLPixelFormatBC4_RSnorm            = 141U,
    MTLPixelFormatBC5_RGUnorm           = 142U,
    MTLPixelFormatBC5_RGSnorm           = 143U,

    /* BPTC */
    MTLPixelFormatBC6H_RGBFloat         = 150U,
    MTLPixelFormatBC6H_RGBUfloat        = 151U,
    MTLPixelFormatBC7_RGBAUnorm         = 152U,
    MTLPixelFormatBC7_RGBAUnorm_sRGB    = 153U,

    /* PVRTC */
    MTLPixelFormatPVRTC_RGB_2BPP        = 160U,
    MTLPixelFormatPVRTC_RGB_2BPP_sRGB   = 161U,
    MTLPixelFormatPVRTC_RGB_4BPP        = 162U,
    MTLPixelFormatPVRTC_RGB_4BPP_sRGB   = 163U,
    MTLPixelFormatPVRTC_RGBA_2BPP       = 164U,
    MTLPixelFormatPVRTC_RGBA_2BPP_sRGB  = 165U,
    MTLPixelFormatPVRTC_RGBA_4BPP       = 166U,
    MTLPixelFormatPVRTC_RGBA_4BPP_sRGB  = 167U,

    /* ETC2 */
    MTLPixelFormatEAC_R11Unorm          = 170U,
    MTLPixelFormatEAC_R11Snorm          = 172U,
    MTLPixelFormatEAC_RG11Unorm         = 174U,
    MTLPixelFormatEAC_RG11Snorm         = 176U,
    MTLPixelFormatEAC_RGBA8             = 178U,
    MTLPixelFormatEAC_RGBA8_sRGB        = 179U,

    MTLPixelFormatETC2_RGB8             = 180U,
    MTLPixelFormatETC2_RGB8_sRGB        = 181U,
    MTLPixelFormatETC2_RGB8A1           = 182U,
    MTLPixelFormatETC2_RGB8A1_sRGB      = 183U,

    /* ASTC */
    MTLPixelFormatASTC_4x4_sRGB         = 186U,
    MTLPixelFormatASTC_5x4_sRGB         = 187U,
    MTLPixelFormatASTC_5x5_sRGB         = 188U,
    MTLPixelFormatASTC_6x5_sRGB         = 189U,
    MTLPixelFormatASTC_6x6_sRGB         = 190U,
    MTLPixelFormatASTC_8x5_sRGB         = 192U,
    MTLPixelFormatASTC_8x6_sRGB         = 193U,
    MTLPixelFormatASTC_8x8_sRGB         = 194U,
    MTLPixelFormatASTC_10x5_sRGB        = 195U,
    MTLPixelFormatASTC_10x6_sRGB        = 196U,
    MTLPixelFormatASTC_10x8_sRGB        = 197U,
    MTLPixelFormatASTC_10x10_sRGB       = 198U,
    MTLPixelFormatASTC_12x10_sRGB       = 199U,
    MTLPixelFormatASTC_12x12_sRGB       = 200U,

    MTLPixelFormatASTC_4x4_LDR          = 204U,
    MTLPixelFormatASTC_5x4_LDR          = 205U,
    MTLPixelFormatASTC_5x5_LDR          = 206U,
    MTLPixelFormatASTC_6x5_LDR          = 207U,
    MTLPixelFormatASTC_6x6_LDR          = 208U,
    MTLPixelFormatASTC_8x5_LDR          = 210U,
    MTLPixelFormatASTC_8x6_LDR          = 211U,
    MTLPixelFormatASTC_8x8_LDR          = 212U,
    MTLPixelFormatASTC_10x5_LDR         = 213U,
    MTLPixelFormatASTC_10x6_LDR         = 214U,
    MTLPixelFormatASTC_10x8_LDR         = 215U,
    MTLPixelFormatASTC_10x10_LDR        = 216U,
    MTLPixelFormatASTC_12x10_LDR        = 217U,
    MTLPixelFormatASTC_12x12_LDR        = 218U,


    /* ASTC HDR (High Dynamic Range) Formats */
    MTLPixelFormatASTC_4x4_HDR          = 222U,
    MTLPixelFormatASTC_5x4_HDR          = 223U,
    MTLPixelFormatASTC_5x5_HDR          = 224U,
    MTLPixelFormatASTC_6x5_HDR          = 225U,
    MTLPixelFormatASTC_6x6_HDR          = 226U,
    MTLPixelFormatASTC_8x5_HDR          = 228U,
    MTLPixelFormatASTC_8x6_HDR          = 229U,
    MTLPixelFormatASTC_8x8_HDR          = 230U,
    MTLPixelFormatASTC_10x5_HDR         = 231U,
    MTLPixelFormatASTC_10x6_HDR         = 232U,
    MTLPixelFormatASTC_10x8_HDR         = 233U,
    MTLPixelFormatASTC_10x10_HDR        = 234U,
    MTLPixelFormatASTC_12x10_HDR        = 235U,
    MTLPixelFormatASTC_12x12_HDR        = 236U,

    /* YUV formats */
    MTLPixelFormatGBGR422 = 240U,
    MTLPixelFormatBGRG422 = 241U,

    /* Depth */

    MTLPixelFormatDepth16Unorm  = 250U,
    MTLPixelFormatDepth32Float  = 252U,

    /* Stencil */

    MTLPixelFormatStencil8 = 253U,

    /* Depth Stencil */

    MTLPixelFormatDepth24Unorm_Stencil8 = 255U,
    MTLPixelFormatDepth32Float_Stencil8 = 260U,

    MTLPixelFormatX32_Stencil8 = 261U,
    MTLPixelFormatX24_Stencil8 = 262U,

    MTLPixelFormatUnspecialized = 263U,
} MTLPixelFormat;

#endif

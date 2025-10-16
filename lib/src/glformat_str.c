/*
** Copyright 2025 The Khronos Group Inc.
**
** SPDX-License-Identifier: Apache-2.0
*/

#include <stdint.h>
#include <ctype.h>
#include <stdio.h>

#include "GL/glcorearb.h"
#include "GL/glext.h"
#define GL_APIENTRY APIENTRY // Keep following included file happy.
#include "GLES2/gl2ext.h"  // For the ASTC 3D values

// The unsuffixed names are deprecated so not in glcorearb.h and it defines macros
// which hide their definitions in glext.h. Rather than include gl.h define these in
// terms of the equivalent extension names which are also in glext.h.
#define GL_SR8                            GL_SR8_EXT
#define GL_SRG8                           GL_SRG8_EXT
// These are from GLES2/gl2.h.
#define GL_LUMINANCE                      0x1909
#define GL_LUMINANCE_ALPHA                0x190A

// s/\(^[A-Z]\)/GL_\1/
// s/\(GL_[A-Z0-9_]*\) .*$/      case \1: return "\1";/

const char* glFormatString(GLenum format)
{
    static char retstr[11];

    switch (format) {
      case 0: return "0 (Compressed)";
      case GL_DEPTH_COMPONENT: return "GL_DEPTH_COMPONENT";
      case GL_DEPTH_STENCIL: return "GL_DEPTH_STENCIL";
      case GL_STENCIL_INDEX: return "GL_STENCIL_INDEX";
      case GL_RED: return "GL_RED";
      case GL_RG: return "GL_RG";
      case GL_RGB: return "GL_RGB";
      case GL_RGBA: return "GL_RGBA";
      case GL_GREEN: return "GL_GREEN";
      case GL_BLUE: return "GL_BLUE";
      case GL_BGR: return "GL_BGR";
      case GL_BGRA: return "GL_BGRA";
      case GL_RED_INTEGER: return "GL_RED_INTEGER";
      case GL_GREEN_INTEGER: return "GL_GREEN_INTEGER";
      case GL_BLUE_INTEGER: return "GL_BLUE_INTEGER";
      case GL_RG_INTEGER: return "GL_RG_INTEGER";
      case GL_RGB_INTEGER: return "GL_RGB_INTEGER";
      case GL_RGBA_INTEGER: return "GL_RGBA_INTEGER";
      case GL_BGR_INTEGER: return "GL_BGR_INTEGER";
      case GL_BGRA_INTEGER: return "GL_BGRA_INTEGER";
      case GL_SRGB: return "GL_SRGB";
      case GL_SRGB_ALPHA: return "GL_SRGB_ALPHA";
      // Deprecated values.
      case GL_ALPHA: return "GL_ALPHA";
      case GL_LUMINANCE: return "GL_LUMINANCE";
      case GL_LUMINANCE_ALPHA: return "GL_LUMINANCE_ALPHA";
      default:
         (void)snprintf(retstr, sizeof(retstr), "%#x", format);
         return retstr;
    }
}

const char* glInternalformatString(GLenum format)
{
    static char retstr[11];

    switch (format) {
      case GL_R8: return "GL_R8";
      case GL_R8_SNORM: return "GL_R8_SNORM";
      case GL_R16: return "GL_R16";
      case GL_R16_SNORM: return "GL_R16_SNORM";
      case GL_RG8: return "GL_RG8";
      case GL_RG8_SNORM: return "GL_RG8_SNORM";
      case GL_RG16: return "GL_RG16";
      case GL_RG16_SNORM: return "GL_RG16_SNORM";
      case GL_R3_G3_B2: return "GL_R3_G3_B2";
      case GL_RGB4: return "GL_RGB4";
      case GL_RGB5: return "GL_RGB5";
      case GL_RGB565: return "GL_RGB565";
      case GL_RGB8: return "GL_RGB8";
      case GL_RGB8_SNORM: return "GL_RGB8_SNORM";
      case GL_RGB10: return "GL_RGB10";
      case GL_RGB12: return "GL_RGB12";
      case GL_RGB16: return "GL_RGB16";
      case GL_RGB16_SNORM: return "GL_RGB16_SNORM";
      case GL_RGBA2: return "GL_RGBA2";
      case GL_RGBA4: return "GL_RGBA4";
      case GL_RGB5_A1: return "GL_RGB5_A1";
      case GL_RGBA8: return "GL_RGBA8";
      case GL_RGBA8_SNORM: return "GL_RGBA8_SNORM";
      case GL_RGB10_A2: return "GL_RGB10_A2";
      case GL_RGB10_A2UI: return "GL_RGB10_A2UI";
      case GL_RGBA12: return "GL_RGBA12";
      case GL_RGBA16: return "GL_RGBA16";
      case GL_RGBA16_SNORM: return "GL_RGBA16_SNORM";
      case GL_SR8: return "GL_SR8";
      case GL_SRG8: return "GL_SRG8";
      case GL_SRGB8: return "GL_SRGB8";
      case GL_SRGB8_ALPHA8: return "GL_SRGB8_ALPHA8";
      case GL_R16F: return "GL_R16F";
      case GL_RG16F: return "GL_RG16F";
      case GL_RGB16F: return "GL_RGB16F";
      case GL_RGBA16F: return "GL_RGBA16F";
      case GL_R32F: return "GL_R32F";
      case GL_RG32F: return "GL_RG32F";
      case GL_RGB32F: return "GL_RGB32F";
      case GL_RGBA32F: return "GL_RGBA32F";
      case GL_R11F_G11F_B10F: return "GL_R11F_G11F_B10F";
      case GL_RGB9_E5: return "GL_RGB9_E5";
      case GL_R8I: return "GL_R8I";
      case GL_R8UI: return "GL_R8UI";
      case GL_R16I: return "GL_R16I";
      case GL_R16UI: return "GL_R16UI";
      case GL_R32I: return "GL_R32I";
      case GL_R32UI: return "GL_R32UI";
      case GL_RG8I: return "GL_RG8I";
      case GL_RG8UI: return "GL_RG8UI";
      case GL_RG16I: return "GL_RG16I";
      case GL_RG16UI: return "GL_RG16UI";
      case GL_RG32I: return "GL_RG32I";
      case GL_RG32UI: return "GL_RG32UI";
      case GL_RGB8I: return "GL_RGB8I";
      case GL_RGB8UI: return "GL_RGB8UI";
      case GL_RGB16I: return "GL_RGB16I";
      case GL_RGB16UI: return "GL_RGB16UI";
      case GL_RGB32I: return "GL_RGB32I";
      case GL_RGB32UI: return "GL_RGB32UI";
      case GL_RGBA8I: return "GL_RGBA8I";
      case GL_RGBA8UI: return "GL_RGBA8UI";
      case GL_RGBA16I: return "GL_RGBA16I";
      case GL_RGBA16UI: return "GL_RGBA16UI";
      case GL_RGBA32I: return "GL_RGBA32I";
      case GL_RGBA32UI: return "GL_RGBA32UI";
      case GL_DEPTH_COMPONENT16: return "GL_DEPTH_COMPONENT16";
      case GL_DEPTH_COMPONENT24: return "GL_DEPTH_COMPONENT24";
      case GL_DEPTH_COMPONENT32: return "GL_DEPTH_COMPONENT32";
      case GL_DEPTH_COMPONENT32F: return "GL_DEPTH_COMPONENT32F";
      case GL_DEPTH24_STENCIL8: return "GL_DEPTH24_STENCIL8";
      case GL_DEPTH32F_STENCIL8: return "GL_DEPTH32F_STENCIL8";
      case GL_STENCIL_INDEX1: return "GL_STENCIL_INDEX1";
      case GL_STENCIL_INDEX4: return "GL_STENCIL_INDEX4";
      case GL_STENCIL_INDEX8: return "GL_STENCIL_INDEX8";
      case GL_STENCIL_INDEX16: return "GL_STENCIL_INDEX16";
      case GL_COMPRESSED_RED: return "GL_COMPRESSED_RED";
      case GL_COMPRESSED_RG: return "GL_COMPRESSED_RG";
      case GL_COMPRESSED_RGB: return "GL_COMPRESSED_RGB";
      case GL_COMPRESSED_RGBA: return "GL_COMPRESSED_RGBA";
      case GL_COMPRESSED_SRGB: return "GL_COMPRESSED_SRGB";
      case GL_COMPRESSED_SRGB_ALPHA: return "GL_COMPRESSED_SRGB_ALPHA";
      case GL_COMPRESSED_RED_RGTC1: return "GL_COMPRESSED_RED_RGTC1";
      case GL_COMPRESSED_SIGNED_RED_RGTC1: return "GL_COMPRESSED_SIGNED_RED_RGTC1";
      case GL_COMPRESSED_RG_RGTC2: return "GL_COMPRESSED_RG_RGTC2";
      case GL_COMPRESSED_SIGNED_RG_RGTC2: return "GL_COMPRESSED_SIGNED_RG_RGTC2";
      case GL_COMPRESSED_RGBA_BPTC_UNORM: return "GL_COMPRESSED_RGBA_BPTC_UNORM";
      case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM: return "GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM";
      case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT: return "GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT";
      case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT: return "GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT";
      case GL_COMPRESSED_RGB8_ETC2: return "GL_COMPRESSED_RGB8_ETC2";
      case GL_COMPRESSED_SRGB8_ETC2: return "GL_COMPRESSED_SRGB8_ETC2";
      case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2: return "GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2";
      case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2: return "GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2";
      case GL_COMPRESSED_RGBA8_ETC2_EAC: return "GL_COMPRESSED_RGBA8_ETC2_EAC";
      case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC: return "GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC";
      case GL_COMPRESSED_R11_EAC: return "GL_COMPRESSED_R11_EAC";
      case GL_COMPRESSED_SIGNED_R11_EAC: return "GL_COMPRESSED_SIGNED_R11_EAC";
      case GL_COMPRESSED_RG11_EAC: return "GL_COMPRESSED_RG11_EAC";
      case GL_COMPRESSED_SIGNED_RG11_EAC: return "GL_COMPRESSED_SIGNED_RG11_EAC";
      case GL_COMPRESSED_RGBA_ASTC_4x4_KHR: return "GL_COMPRESSED_RGBA_ASTC_4x4_KHR";
      case GL_COMPRESSED_RGBA_ASTC_5x4_KHR: return "GL_COMPRESSED_RGBA_ASTC_5x4_KHR";
      case GL_COMPRESSED_RGBA_ASTC_5x5_KHR: return "GL_COMPRESSED_RGBA_ASTC_5x5_KHR";
      case GL_COMPRESSED_RGBA_ASTC_6x5_KHR: return "GL_COMPRESSED_RGBA_ASTC_6x5_KHR";
      case GL_COMPRESSED_RGBA_ASTC_6x6_KHR: return "GL_COMPRESSED_RGBA_ASTC_6x6_KHR";
      case GL_COMPRESSED_RGBA_ASTC_8x5_KHR: return "GL_COMPRESSED_RGBA_ASTC_8x5_KHR";
      case GL_COMPRESSED_RGBA_ASTC_8x6_KHR: return "GL_COMPRESSED_RGBA_ASTC_8x6_KHR";
      case GL_COMPRESSED_RGBA_ASTC_8x8_KHR: return "GL_COMPRESSED_RGBA_ASTC_8x8_KHR";
      case GL_COMPRESSED_RGBA_ASTC_10x5_KHR: return "GL_COMPRESSED_RGBA_ASTC_10x5_KHR";
      case GL_COMPRESSED_RGBA_ASTC_10x6_KHR: return "GL_COMPRESSED_RGBA_ASTC_10x6_KHR";
      case GL_COMPRESSED_RGBA_ASTC_10x8_KHR: return "GL_COMPRESSED_RGBA_ASTC_10x8_KHR";
      case GL_COMPRESSED_RGBA_ASTC_10x10_KHR: return "GL_COMPRESSED_RGBA_ASTC_10x10_KHR";
      case GL_COMPRESSED_RGBA_ASTC_12x10_KHR: return "GL_COMPRESSED_RGBA_ASTC_12x10_KHR";
      case GL_COMPRESSED_RGBA_ASTC_12x12_KHR: return "GL_COMPRESSED_RGBA_ASTC_12x12_KHR";
      case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR";
      case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR";
      case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR";
      case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR";
      case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR";
      case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR";
      case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR";
      case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR";
      case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR";
      case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR";
      case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR";
      case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR";
      case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR";
      case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR";
      case GL_COMPRESSED_RGB_S3TC_DXT1_EXT: return "GL_COMPRESSED_RGB_S3TC_DXT1_EXT";
      case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT: return "GL_COMPRESSED_RGBA_S3TC_DXT1_EXT";
      case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT: return "GL_COMPRESSED_RGBA_S3TC_DXT3_EXT";
      case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT: return "GL_COMPRESSED_RGBA_S3TC_DXT5_EXT";
      case GL_COMPRESSED_RGB_FXT1_3DFX: return "GL_COMPRESSED_RGB_FXT1_3DFX";
      case GL_COMPRESSED_RGBA_FXT1_3DFX: return "GL_COMPRESSED_RGBA_FXT1_3DFX";
      case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT: return "GL_COMPRESSED_SRGB_S3TC_DXT1_EXT";
      case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT: return "GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT";
      case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT: return "GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT";
      case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT: return "GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT";
      case GL_COMPRESSED_RGBA_ASTC_3x3x3_OES: return "GL_COMPRESSED_RGBA_ASTC_3x3x3_OES";
      case GL_COMPRESSED_RGBA_ASTC_4x3x3_OES: return "GL_COMPRESSED_RGBA_ASTC_4x3x3_OES";
      case GL_COMPRESSED_RGBA_ASTC_4x4x3_OES: return "GL_COMPRESSED_RGBA_ASTC_4x4x3_OES";
      case GL_COMPRESSED_RGBA_ASTC_4x4x4_OES: return "GL_COMPRESSED_RGBA_ASTC_4x4x4_OES";
      case GL_COMPRESSED_RGBA_ASTC_5x4x4_OES: return "GL_COMPRESSED_RGBA_ASTC_5x4x4_OES";
      case GL_COMPRESSED_RGBA_ASTC_5x5x4_OES: return "GL_COMPRESSED_RGBA_ASTC_5x5x4_OES";
      case GL_COMPRESSED_RGBA_ASTC_5x5x5_OES: return "GL_COMPRESSED_RGBA_ASTC_5x5x5_OES";
      case GL_COMPRESSED_RGBA_ASTC_6x5x5_OES: return "GL_COMPRESSED_RGBA_ASTC_6x5x5_OES";
      case GL_COMPRESSED_RGBA_ASTC_6x6x5_OES: return "GL_COMPRESSED_RGBA_ASTC_6x6x5_OES";
      case GL_COMPRESSED_RGBA_ASTC_6x6x6_OES: return "GL_COMPRESSED_RGBA_ASTC_6x6x6_OES";
      case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_3x3x3_OES: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_3x3x3_OES";
      case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x3x3_OES: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x3x3_OES";
      case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x3_OES: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x3_OES";
      case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x4_OES: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x4_OES";
      case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4x4_OES: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4x4_OES";
      case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x4_OES: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x4_OES";
      case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x5_OES: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x5_OES";
      case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5x5_OES: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5x5_OES";
      case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x5_OES: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x5_OES";
      case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x6_OES: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x6_OES";
      case GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG: return "GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG";
      case GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG: return "GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG";
      case GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG: return "GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG";
      case GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG: return "GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG";
      case GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT: return "GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT";
      case GL_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT: return "GL_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT";
      case GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT: return "GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT";
      case GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT: return "GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT";
      case GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV2_IMG: return "GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV2_IMG";
      case GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV2_IMG: return "GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV2_IMG";
      case GL_ETC1_RGB8_OES: return "GL_ETC1_RGB8_OES";
      // Deprecated values.
      case GL_ALPHA8_EXT: return "GL_ALPHA8_EXT";
      case GL_LUMINANCE4_ALPHA4_EXT: return "GL_LUMINANCE4_ALPHA4_EXT";
      case GL_LUMINANCE8_ALPHA8_EXT: return "GL_LUMINANCE8_ALPHA8_EXT";
      case GL_LUMINANCE8_EXT: return "GL_LUMINANCE8_EXT";
      default:
         (void)snprintf(retstr, sizeof(retstr), "%#x", format);
         return retstr;
    }
}

const char* glTypeString(GLenum type)
{
    static char retstr[11];

    switch (type) {
      case 0: return "0 (Compressed)";
      case GL_UNSIGNED_BYTE: return "GL_UNSIGNED_BYTE";
      case GL_BYTE: return "GL_BYTE";
      case GL_UNSIGNED_SHORT: return "GL_UNSIGNED_SHORT";
      case GL_SHORT: return "GL_SHORT";
      case GL_UNSIGNED_INT: return "GL_UNSIGNED_INT";
      case GL_INT: return "GL_INT";
      case GL_HALF_FLOAT: return "GL_HALF_FLOAT";
      case GL_FLOAT: return "GL_FLOAT";
      case GL_UNSIGNED_BYTE_3_3_2: return "GL_UNSIGNED_BYTE_3_3_2";
      case GL_UNSIGNED_BYTE_2_3_3_REV: return "GL_UNSIGNED_BYTE_2_3_3_REV";
      case GL_UNSIGNED_SHORT_5_6_5: return "GL_UNSIGNED_SHORT_5_6_5";
      case GL_UNSIGNED_SHORT_5_6_5_REV: return "GL_UNSIGNED_SHORT_5_6_5_REV";
      case GL_UNSIGNED_SHORT_4_4_4_4: return "GL_UNSIGNED_SHORT_4_4_4_4";
      case GL_UNSIGNED_SHORT_4_4_4_4_REV: return "GL_UNSIGNED_SHORT_4_4_4_4_REV";
      case GL_UNSIGNED_SHORT_5_5_5_1: return "GL_UNSIGNED_SHORT_5_5_5_1";
      case GL_UNSIGNED_SHORT_1_5_5_5_REV: return "GL_UNSIGNED_SHORT_1_5_5_5_REV";
      case GL_UNSIGNED_INT_8_8_8_8: return "GL_UNSIGNED_INT_8_8_8_8";
      case GL_UNSIGNED_INT_8_8_8_8_REV: return "GL_UNSIGNED_INT_8_8_8_8_REV";
      case GL_UNSIGNED_INT_10_10_10_2: return "GL_UNSIGNED_INT_10_10_10_2";
      case GL_UNSIGNED_INT_2_10_10_10_REV: return "GL_UNSIGNED_INT_2_10_10_10_REV";
      case GL_UNSIGNED_INT_24_8: return "GL_UNSIGNED_INT_24_8";
      case GL_UNSIGNED_INT_10F_11F_11F_REV: return "GL_UNSIGNED_INT_10F_11F_11F_REV";
      case GL_UNSIGNED_INT_5_9_9_9_REV: return "GL_UNSIGNED_INT_5_9_9_9_REV";
      case GL_FLOAT_32_UNSIGNED_INT_24_8_REV: return "GL_FLOAT_32_UNSIGNED_INT_24_8_REV";
      default:
         (void)snprintf(retstr, sizeof(retstr), "%#x", type);
         return retstr;
    }
}


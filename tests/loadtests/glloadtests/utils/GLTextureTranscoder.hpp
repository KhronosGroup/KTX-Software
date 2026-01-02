/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <ktx.h>

#if !defined(GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT)
#define GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT 0x8A54
#define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG  0x8C01
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG 0x9137
#endif
#if !defined(GL_COMPRESSED_RG_RGTC2)
#define GL_COMPRESSED_RG_RGTC2              0x8DBD
#endif
#if !defined(GL_COMPRESSED_RGBA_BPTC_UNORM)
#define GL_COMPRESSED_RGBA_BPTC_UNORM       0x8E8C
#endif
#if !defined(GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT)
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT 0x8E8E
#endif

class TextureTranscoder {
  public:
    TextureTranscoder() {
        determineCompressedTexFeatures(deviceFeatures);
        if (deviceFeatures.astc_ldr)
            defaultTf = KTX_TTF_ASTC_4x4_RGBA;
        else if (deviceFeatures.bc3)
            defaultTf = KTX_TTF_BC1_OR_3;
        else if (deviceFeatures.etc2)
            defaultTf = KTX_TTF_ETC; // Let transcoder decide RGB or RGBA
        else if (deviceFeatures.pvrtc1)
            defaultTf = KTX_TTF_PVRTC1_4_RGBA;
        else if (deviceFeatures.etc1)
            defaultTf = KTX_TTF_ETC1_RGB;
        else {
            std::stringstream message;

            message << "OpenGL implementation does not support any available transcode target.";
            throw std::runtime_error(message.str());
        }
    }

    void transcode(ktxTexture2* kTexture,
                   ktx_transcode_fmt_e otf = KTX_TTF_NOSELECTION) {
        KTX_error_code ktxresult;
        ktx_transcode_fmt_e tf;
        if (otf != KTX_TTF_NOSELECTION) {
            tf = otf;
        } else {
            khr_df_model_e colorModel = ktxTexture2_GetColorModel_e(kTexture);
            if (colorModel == KHR_DF_MODEL_UASTC && deviceFeatures.astc_ldr) {
                tf = KTX_TTF_ASTC_4x4_RGBA;
            } else if (colorModel == KHR_DF_MODEL_ETC1S && deviceFeatures.etc2) {
                tf = KTX_TTF_ETC;
            } else {
                tf = defaultTf;
            }
        }
        ktxresult = ktxTexture2_TranscodeBasis(kTexture, tf, 0);
        if (KTX_SUCCESS != ktxresult) {
            std::stringstream message;

            message << "Transcoding of ktxTexture2 to "
                    << ktxTranscodeFormatString(tf) << " failed: "
                    << ktxErrorString(ktxresult);
            throw std::runtime_error(message.str());
        }
    }

  protected:
    ktx_transcode_fmt_e defaultTf;

    struct compressedTexFeatures {
        bool astc_ldr;
        bool astc_hdr;
        bool bc6h;
        bool bc7;
        bool etc1;
        bool etc2;
        bool bc3;
        bool pvrtc1;
        bool pvrtc_srgb;
        bool pvrtc2;
        bool rgtc;
    } deviceFeatures;

    void determineCompressedTexFeatures(compressedTexFeatures& features) {
        ktx_int32_t numCompressedFormats;

        memset(&features, false, sizeof(features));

        glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &numCompressedFormats);
        GLint* formats = new GLint[numCompressedFormats];
        glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, formats);

        for (ktx_int32_t i = 0; i < numCompressedFormats; i++) {
            if (formats[i] == GL_COMPRESSED_RGBA8_ETC2_EAC)
                features.etc2 = true;
            if (formats[i] == GL_ETC1_RGB8_OES)
                features.etc1 = true;
            if (formats[i] == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
                features.bc3 = true;
            if (formats[i] == GL_COMPRESSED_RG_RGTC2)
                features.rgtc = true;
            if (formats[i] == GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT)
                features.pvrtc_srgb = true;
            if (formats[i] == GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG)
                features.pvrtc1 = true;
            if (formats[i] == GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG)
                features.pvrtc2 = true;
            if (formats[i] == GL_COMPRESSED_RGBA_ASTC_4x4_KHR)
                features.astc_ldr = true;
            if (formats[i] == GL_COMPRESSED_RGBA_BPTC_UNORM)
                features.bc7 = true;
            if (formats[i] == GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT)
                features.bc6h = true;
        }
        delete[] formats;

        // Just in case COMPRESSED_TEXTURE_FORMATS didn't return anything.
        // There is no ETC2 extension. It went into core in OpenGL ES 2.0.
        // ARB_es_compatibility is not a good indicator. ETC2 could be supported
        // by software decompression. Better to report unsupported.
        if (!features.etc1 && SDL_GL_ExtensionSupported("GL_OES_compressed_ETC1_RGB8_texture"))
            features.etc1 = true;;
        if (!features.bc3 && SDL_GL_ExtensionSupported("GL_EXT_texture_compression_s3tc"))
            features.bc3 = true;
        if (!features.rgtc && SDL_GL_ExtensionSupported("GL_ARB_texture_compression_rgtc"))
            features.rgtc = true;
        if (!features.pvrtc1 && SDL_GL_ExtensionSupported("GL_IMG_texture_compression_pvrtc"))
            features.pvrtc1 = true;
        if (!features.pvrtc2 && SDL_GL_ExtensionSupported("GL_IMG_texture_compression_pvrtc2"))
            features.pvrtc2 = true;
        if (!features.pvrtc_srgb && SDL_GL_ExtensionSupported("GL_EXT_pvrtc_sRGB"))
            features.pvrtc_srgb = true;
        if (!(features.bc7 && features.bc6h) && SDL_GL_ExtensionSupported("GL_ARB_texture_compression_bptc"))
            features.bc6h = features.bc7 = true;
        if (!features.astc_ldr && SDL_GL_ExtensionSupported("GL_KHR_texture_compression_astc_ldr"))
            features.astc_ldr = true;
        // The only way to identify this support is the extension string.
        // The format name is the same.
        if (SDL_GL_ExtensionSupported("GL_KHR_texture_compression_astc_hdr"))
            features.astc_hdr = true;
    }

};

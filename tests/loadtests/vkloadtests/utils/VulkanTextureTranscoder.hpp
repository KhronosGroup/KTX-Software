/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <ktx.h>
#include <vulkan/vulkan.hpp>

#include "VulkanContext.h"

class TextureTranscoder {
  public:
    TextureTranscoder(VulkanContext& vkctx) : vkctx(vkctx) {
        if (vkctx.gpuFeatures.textureCompressionASTC_LDR)
            defaultLDRTf = KTX_TTF_ASTC_4x4_RGBA;
        else if (vkctx.gpuFeatures.textureCompressionETC2)
            defaultLDRTf = KTX_TTF_ETC;
        else if (vkctx.gpuFeatures.textureCompressionBC)
            defaultLDRTf = KTX_TTF_BC1_OR_3;
        else if (vkctx.enabledDeviceExtensions.pvrtc) {
            defaultLDRTf = KTX_TTF_PVRTC2_4_RGBA;
        } else {
            std::stringstream message;

            message << "Vulkan implementation does not support any available SDR transcode target.";
            throw std::runtime_error(message.str());
        }
        if (vkctx.gpuFeatureAstcHdr)
            defaultHDRTf = KTX_TTF_ASTC_HDR_4x4_RGBA;
        else if (vkctx.gpuFeatures.textureCompressionBC)
            defaultHDRTf = KTX_TTF_BC6HU;
    }

    void transcode(ktxTexture2* kTexture) {
        KTX_error_code ktxresult;
        ktx_transcode_fmt_e tf;
        khr_df_model_e colorModel = ktxTexture2_GetColorModel_e(kTexture);

        if (colorModel == KHR_DF_MODEL_UASTC
            && vkctx.gpuFeatures.textureCompressionASTC_LDR) {
            tf = KTX_TTF_ASTC_4x4_RGBA;
        } else if (colorModel == KHR_DF_MODEL_ETC1S
                   && vkctx.gpuFeatures.textureCompressionETC2) {
            tf = KTX_TTF_ETC;
        } else if (colorModel == KHR_DF_MODEL_UASTC_HDR_4X4
                   && vkctx.gpuFeatureAstcHdr) {
            tf = KTX_TTF_ASTC_HDR_4x4_RGBA;
        } else if (colorModel == KHR_DF_MODEL_UASTC_HDR_6X6
                   && vkctx.gpuFeatureAstcHdr) {
            tf = KTX_TTF_ASTC_HDR_6x6_RGBA;
        } else if (colorModel == KHR_DF_MODEL_UASTC_HDR_4X4
                || colorModel == KHR_DF_MODEL_UASTC_HDR_6X6) {
                tf = defaultHDRTf;
        } else {
                tf = defaultLDRTf;
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
    ktx_transcode_fmt_e defaultLDRTf;
    ktx_transcode_fmt_e defaultHDRTf;
    VulkanContext& vkctx;
};

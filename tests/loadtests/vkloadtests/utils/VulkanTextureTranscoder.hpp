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
    TextureTranscoder(VulkanContext& vkctx) {
        vk::PhysicalDeviceFeatures deviceFeatures;
        vkctx.gpu.getFeatures(&deviceFeatures);

        if (deviceFeatures.textureCompressionASTC_LDR)
            tf = KTX_TTF_ASTC_4x4_RGBA;
        else if (deviceFeatures.textureCompressionETC2)
            tf = KTX_TTF_ETC;
        else if (deviceFeatures.textureCompressionBC)
            tf = KTX_TTF_BC1_OR_3;
        else if (vkctx.enabledDeviceExtensions.pvrtc) {
            tf = KTX_TTF_PVRTC2_4_RGBA;
        } else {
            std::stringstream message;

            message << "Vulkan implementation does not support any available transcode target.";
            throw std::runtime_error(message.str());
        }
    }

    void transcode(ktxTexture2* kTexture) {
        KTX_error_code ktxresult;
        ktxresult = ktxTexture2_TranscodeBasis(kTexture, tf, 0);
        if (KTX_SUCCESS != ktxresult) {
            std::stringstream message;

            message << "Transcoding of ktxTexture2 to "
                    << ktxTranscodeFormatString(tf) << " failed: "
                    << ktxErrorString(ktxresult);
            throw std::runtime_error(message.str());
        }
    }

    ktx_transcode_fmt_e getFormat() { return tf; }

  protected:
    ktx_transcode_fmt_e tf;

};

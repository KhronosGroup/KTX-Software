/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2017 Mark Callow.
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

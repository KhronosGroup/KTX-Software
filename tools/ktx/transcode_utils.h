// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "command.h"
#include "formats.h"
#include "image.hpp"
#include "utility.h"
#include "dfdutils/dfd.h"
#include <KHR/khr_df.h>

#include <tuple>
#include <string>
#include <unordered_map>
#include <optional>

// -------------------------------------------------------------------------------------------------

namespace ktx {

struct TranscodeSwizzleInfo {
    uint32_t defaultNumComponents = 0;
    std::string swizzle;
};

std::optional<khr_df_model_channels_e> getChannelType(const KTXTexture2& texture, uint32_t index);
TranscodeSwizzleInfo determineTranscodeSwizzle(const KTXTexture2& texture, Reporter& report);

template <bool TRANSCODE_CMD>
struct OptionsTranscodeTarget {
    std::optional<ktx_transcode_fmt_e> transcodeTarget;
    std::string transcodeTargetName;
    uint32_t transcodeSwizzleComponents = 0;
    std::string transcodeSwizzle;

    void init(cxxopts::Options&) {}

    void process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter& report) {
        // "transcode" command - optional "target" argument
        // "extract" command - optional "transcode" argument
        const auto argName = TRANSCODE_CMD ? "target" : "transcode";

        static const std::unordered_map<std::string, std::pair<ktx_transcode_fmt_e, uint32_t>> targets{
            {"etc-rgb", {KTX_TTF_ETC1_RGB, 0}},
            {"etc-rgba", {KTX_TTF_ETC2_RGBA, 0}},
            {"eac-r11", {KTX_TTF_ETC2_EAC_R11, 0}},
            {"eac-rg11", {KTX_TTF_ETC2_EAC_RG11, 0}},
            {"bc1", {KTX_TTF_BC1_RGB, 0}},
            {"bc3", {KTX_TTF_BC3_RGBA, 0}},
            {"bc4", {KTX_TTF_BC4_R, 0}},
            {"bc5", {KTX_TTF_BC5_RG, 0}},
            {"bc7", {KTX_TTF_BC7_RGBA, 0}},
            {"astc", {KTX_TTF_ASTC_4x4_RGBA, 0}},
            {"r8", {KTX_TTF_RGBA32, 1}},
            {"rg8", {KTX_TTF_RGBA32, 2}},
            {"rgb8", {KTX_TTF_RGBA32, 3}},
            {"rgba8", {KTX_TTF_RGBA32, 4}},
        };
        if (args[argName].count()) {
            const auto argStr = to_lower_copy(args[argName].as<std::string>());
            const auto it = targets.find(argStr);
            if (it == targets.end())
                report.fatal_usage("Invalid transcode target: \"{}\".", argStr);

            transcodeTarget = it->second.first;
            transcodeTargetName = argStr;
            transcodeSwizzleComponents = it->second.second;
        }
    }

    void validateTextureTranscode(const KTXTexture2& texture, Reporter& report) {
        const auto tswizzle = determineTranscodeSwizzle(texture, report);

        if (!transcodeTarget.has_value()) {
            transcodeTarget = KTX_TTF_RGBA32;
            transcodeTargetName = "rgba8";
            transcodeSwizzleComponents = tswizzle.defaultNumComponents;
        }

        transcodeSwizzle = tswizzle.swizzle;
    }
};

template <bool TRANSCODE_CMD>
KTXTexture2 transcode(KTXTexture2&& texture, OptionsTranscodeTarget<TRANSCODE_CMD>& options, Reporter& report) {
    options.validateTextureTranscode(texture, report);

    auto ret = ktxTexture2_TranscodeBasis(texture, options.transcodeTarget.value(), 0);
    if (ret != KTX_SUCCESS)
        report.fatal(rc::INVALID_FILE, "Failed to transcode KTX2 texture: {}", ktxErrorString(ret));

    // Need to perform format conversion and swizzling if needed
    bool needFormatConversion = false;
    bool needSwizzle = false;
    if (options.transcodeSwizzleComponents != 0) {
        if (options.transcodeSwizzleComponents == 4) {
            if (options.transcodeSwizzle != "rgba") {
                needSwizzle = true;
            }
        } else {
            needFormatConversion = true;
            needSwizzle = true;
        }
    }

    KTXTexture2 convertedTexture{nullptr};
    if (needFormatConversion) {
        ktxTextureCreateInfo createInfo;
        std::memset(&createInfo, 0, sizeof(createInfo));

        const bool srgb = (texture->vkFormat == VK_FORMAT_R8G8B8A8_SRGB);
        switch (options.transcodeSwizzleComponents) {
        case 1:
            createInfo.vkFormat = srgb ? VK_FORMAT_R8_SRGB : VK_FORMAT_R8_UNORM;
            break;
        case 2:
            createInfo.vkFormat = srgb ? VK_FORMAT_R8G8_SRGB : VK_FORMAT_R8G8_UNORM;
            break;
        case 3:
            createInfo.vkFormat = srgb ? VK_FORMAT_R8G8B8_SRGB : VK_FORMAT_R8G8B8_UNORM;
            break;
        default:
            assert(false);
        }

        createInfo.baseWidth = texture->baseWidth;
        createInfo.baseHeight = texture->baseHeight;
        createInfo.baseDepth = texture->baseDepth;
        createInfo.generateMipmaps = texture->generateMipmaps;
        createInfo.isArray = texture->isArray;
        createInfo.numDimensions = texture->numDimensions;
        createInfo.numFaces = texture->numFaces;
        createInfo.numLayers = texture->numLayers;
        createInfo.numLevels = texture->numLevels;
        createInfo.pDfd = nullptr;

        ret = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, convertedTexture.pHandle());
        if (KTX_SUCCESS != ret)
            report.fatal(rc::IO_FAILURE, "Failed to create output texture: {}", ktxErrorString(ret));
    }

    KTXTexture2& outputTexture = (convertedTexture.handle() != nullptr) ? convertedTexture : texture;
    if (needFormatConversion || needSwizzle) {
        for (uint32_t levelIndex = 0; levelIndex < texture->numLevels; ++levelIndex) {
            const auto imageWidth = std::max(1u, texture->baseWidth >> levelIndex);
            const auto imageHeight = std::max(1u, texture->baseHeight >> levelIndex);
            const auto imageDepth = std::max(1u, texture->baseDepth >> levelIndex);

            for (uint32_t faceIndex = 0; faceIndex < texture->numFaces; ++faceIndex) {
                for (uint32_t layerIndex = 0; layerIndex < texture->numLayers; ++layerIndex) {
                    for (uint32_t depthIndex = 0; depthIndex < imageDepth; ++depthIndex) {
                        ktx_size_t srcImageOffset;
                        ktxTexture_GetImageOffset(texture, levelIndex, layerIndex, faceIndex + depthIndex, &srcImageOffset);
                        ktx_size_t dstImageOffset;
                        ktxTexture_GetImageOffset(outputTexture, levelIndex, layerIndex, faceIndex + depthIndex, &dstImageOffset);

                        auto srcImageData = texture->pData + srcImageOffset;
                        auto dstImageData = outputTexture->pData + dstImageOffset;

                        rgba8image srcImage(imageWidth, imageHeight, reinterpret_cast<rgba8color*>(srcImageData));

                        switch (options.transcodeSwizzleComponents) {
                        case 1: {
                            r8image dstImage(imageWidth, imageHeight, reinterpret_cast<r8color*>(dstImageData));
                            srcImage.copyToR(dstImage, options.transcodeSwizzle);
                            break;
                        }
                        case 2: {
                            rg8image dstImage(imageWidth, imageHeight, reinterpret_cast<rg8color*>(dstImageData));
                            srcImage.copyToRG(dstImage, options.transcodeSwizzle);
                            break;
                        }
                        case 3: {
                            rgb8image dstImage(imageWidth, imageHeight, reinterpret_cast<rgb8color*>(dstImageData));
                            srcImage.copyToRGB(dstImage, options.transcodeSwizzle);
                            break;
                        }
                        case 4: {
                            // Swizzle in-place
                            assert(srcImageData == dstImageData);
                            srcImage.swizzle(options.transcodeSwizzle);
                            break;
                        }
                        default:
                            assert(false);
                        }
                    }
                }
            }
        }
    }

    return std::move(outputTexture);
}

} // namespace ktx

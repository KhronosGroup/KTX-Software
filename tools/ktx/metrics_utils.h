// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "command.h"
#include "transcode_utils.h"
#include "utility.h"
#include "image.hpp"

#include "ktx.h"
#include "ktxint.h"
#include "texture2.h"

#include <basisu/encoder/basisu_enc.h>
#include <basisu/encoder/basisu_ssim.h>

// -------------------------------------------------------------------------------------------------

namespace ktx {

/**
//! [command options_metrics]
<dl>
    <dt>\--compare-ssim</dt>
    <dd>Calculate encoding structural similarity index measure (SSIM) and print it to stdout. Requires Basis-LZ or UASTC encoding.</dd>
    <dt>\--compare-psnr</dt>
    <dd>Calculate encoding peak signal-to-noise ratio (PSNR) and print it to stdout. Requires Basis-LZ or UASTC encoding.</dd>
</dl>
//! [command options_metrics]
*/
struct OptionsMetrics {
    bool compare_ssim;
    bool compare_psnr;

    void init(cxxopts::Options& opts) {
        opts.add_options()
            ("compare-ssim", "Calculate encoding structural similarity index measure (SSIM) and print it to stdout. Requires Basis-LZ or UASTC encoding.")
            ("compare-psnr", "Calculate encoding peak signal-to-noise ratio (PSNR) and print it to stdout. Requires Basis-LZ or UASTC encoding.");
    }

    void process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter&) {
        compare_ssim = args["compare-ssim"].as<bool>();
        compare_psnr = args["compare-psnr"].as<bool>();
    }
};

class MetricsCalculator {
    uint32_t referenceNumChannels = 0;
    std::vector<basisu::image> referenceImages;

public:
    void saveReferenceImages(KTXTexture2& texture, const OptionsMetrics& opts, Reporter&) {
        if (!opts.compare_ssim && !opts.compare_psnr)
            return;

        const auto numChannels = ktxTexture2_GetNumComponents(texture);
        referenceNumChannels = numChannels;

        // Format is R/RG/RGB/RGBA 8bit UNORM/SRGB
        for (uint32_t levelIndex = 0; levelIndex < texture->numLevels; ++levelIndex) {
            const uint32_t imageWidth = std::max(texture->baseWidth >> levelIndex, 1u);
            const uint32_t imageHeight = std::max(texture->baseHeight >> levelIndex, 1u);
            const uint32_t imageDepths = std::max(texture->baseDepth >> levelIndex, 1u);

            for (uint32_t layerIndex = 0; layerIndex < texture->numLayers; ++layerIndex) {
                for (uint32_t faceIndex = 0; faceIndex < texture->numFaces; ++faceIndex) {
                    for (uint32_t depthSliceIndex = 0; depthSliceIndex < imageDepths; ++depthSliceIndex) {

                        ktx_size_t imageOffset;
                        ktxTexture_GetImageOffset(texture, levelIndex, layerIndex, faceIndex + depthSliceIndex, &imageOffset);
                        const auto* imageData = texture->pData + imageOffset;

                        auto& ref = referenceImages.emplace_back(imageWidth, imageHeight);

                        for (uint32_t y = 0; y < imageHeight; ++y) {
                            for (uint32_t x = 0; x < imageWidth; ++x) {
                                ref(x, y) = basisu::color_rgba(0, 0, 0, 255);
                                for (uint32_t c = 0; c < numChannels; ++c)
                                    ref(x, y)[c] = *(imageData + y * imageWidth * numChannels + x * numChannels + c);
                            }
                        }
                    }
                }
            }
        }
    }

    void decodeAndCalculateMetrics(KTXTexture2& encodedTexture, const OptionsMetrics& opts, Reporter& report) {
        if (!opts.compare_ssim && !opts.compare_psnr)
            return;

        KTXTexture2 texture{static_cast<ktxTexture2*>(malloc(sizeof(ktxTexture2)))};
        ktxTexture2_constructCopy(texture, encodedTexture);

        const auto tSwizzleInfo = determineTranscodeSwizzle(texture, report);

        ktx_error_code_e ec = KTX_SUCCESS;
        // Decode the encoded texture to observe the compression losses
        ec = ktxTexture2_TranscodeBasis(texture, KTX_TTF_RGBA32, 0);
        if (ec != KTX_SUCCESS)
            report.fatal(rc::KTX_FAILURE, "Failed to transcode KTX2 texture to calculate error metrics: {}", ktxErrorString(ec));

        float overallSSIM[4] = {};
        float overallPSNR = 0;

        auto refIt = referenceImages.begin();
        for (uint32_t levelIndex = 0; levelIndex < texture->numLevels; ++levelIndex) {
            const uint32_t imageWidth = std::max(texture->baseWidth >> levelIndex, 1u);
            const uint32_t imageHeight = std::max(texture->baseHeight >> levelIndex, 1u);
            const uint32_t imageDepth = std::max(texture->baseDepth >> levelIndex, 1u);

            for (uint32_t layerIndex = 0; layerIndex < texture->numLayers; ++layerIndex) {
                for (uint32_t faceIndex = 0; faceIndex < texture->numFaces; ++faceIndex) {
                    for (uint32_t depthSliceIndex = 0; depthSliceIndex < imageDepth; ++depthSliceIndex) {
                        assert(refIt != referenceImages.end() && "Internal error");

                        ktx_size_t imageOffset;
                        ktxTexture_GetImageOffset(texture, levelIndex, layerIndex, faceIndex + depthSliceIndex, &imageOffset);
                        auto* imageData = texture->pData + imageOffset;

                        rgba8image imageView(imageWidth, imageHeight, reinterpret_cast<rgba8color*>(imageData));
                        imageView.swizzle(tSwizzleInfo.swizzle);

                        auto& ref = *refIt++;

                        basisu::image basisuImage;
                        basisuImage.resize(imageWidth, imageHeight);

                        for (uint32_t y = 0; y < imageHeight; ++y) {
                            for (uint32_t x = 0; x < imageWidth; ++x) {
                                basisuImage(x, y) = basisu::color_rgba(0, 0, 0, 255);
                                for (uint32_t c = 0; c < referenceNumChannels; ++c)
                                    basisuImage(x, y)[c] = imageView(x, y)[c];
                            }
                        }

                        if (referenceImages.size() != 1)
                            fmt::print("Level {}{}{}{}:\n",
                                    levelIndex,
                                    texture->isArray ? fmt::format(" Layer {}", layerIndex) : "",
                                    texture->isCubemap ? fmt::format(" Face {}", faceIndex) : "",
                                    texture->numDimensions == 3 ? fmt::format(" Depth {}", depthSliceIndex) : "");

                        if (opts.compare_ssim) {
                            const auto ssim = basisu::compute_ssim(ref, basisuImage, false, false);
                            if (referenceImages.size() != 1) {
                                if (referenceNumChannels > 3)
                                    fmt::print("    SSIM R: {:+7.6f}, G: {:+7.6f}, B: {:+7.6f}, A: {:+7.6f}\n", ssim[0], ssim[1], ssim[2], ssim[3]);
                                else if (referenceNumChannels > 2)
                                    fmt::print("    SSIM R: {:+7.6f}, G: {:+7.6f}, B: {:+7.6f}\n", ssim[0], ssim[1], ssim[2]);
                                else if (referenceNumChannels > 1)
                                    fmt::print("    SSIM R: {:+7.6f}, G: {:+7.6f}\n", ssim[0], ssim[1]);
                                else if (referenceNumChannels > 0)
                                    fmt::print("    SSIM R: {:+7.6f}\n", ssim[0]);
                            }
                            for (int i = 0; i < 4; ++i)
                                overallSSIM[i] += ssim[i];
                        }

                        if (opts.compare_psnr) {
                            basisu::image_metrics im;
                            im.calc(ref, basisuImage);
                            if (referenceImages.size() != 1)
                                fmt::print("    PSNR: {:9.6f}\n", im.m_psnr);
                            overallPSNR = std::max(overallPSNR, im.m_psnr);
                        }
                    }
                }
            }
        }
        assert(refIt == referenceImages.end() && "Internal error");

        fmt::print("{}Overall:\n", referenceImages.size() != 1 ? "\n" : "");

        if (opts.compare_ssim) {
            const auto numIf = static_cast<float>(referenceImages.size());
            if (referenceNumChannels > 3)
                fmt::print("    SSIM Avg R: {:+7.6f}, G: {:+7.6f}, B: {:+7.6f}, A: {:+7.6f}\n", overallSSIM[0] / numIf, overallSSIM[1] / numIf, overallSSIM[2] / numIf, overallSSIM[3] / numIf);
            else if (referenceNumChannels > 2)
                fmt::print("    SSIM Avg R: {:+7.6f}, G: {:+7.6f}, B: {:+7.6f}\n", overallSSIM[0] / numIf, overallSSIM[1] / numIf, overallSSIM[2] / numIf);
            else if (referenceNumChannels > 1)
                fmt::print("    SSIM Avg R: {:+7.6f}, G: {:+7.6f}\n", overallSSIM[0] / numIf, overallSSIM[1] / numIf);
            else
                fmt::print("    SSIM Avg R: {:+7.6f}\n", overallSSIM[0] / numIf);
        }

        if (opts.compare_psnr) {
            fmt::print("    PSNR Max: {:9.6f}\n", overallPSNR);
        }
    }
};

} // namespace ktx

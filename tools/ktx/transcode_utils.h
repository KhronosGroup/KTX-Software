// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "command.h"
#include "utility.h"
#include "formats.h"
#include "dfdutils/dfd.h"
#include <KHR/khr_df.h>

#include <tuple>
#include <string>
#include <unordered_map>
#include <optional>

// -------------------------------------------------------------------------------------------------

namespace ktx {

template <bool TRANSCODE_CMD>
struct OptionsTranscodeTarget {
    std::optional<ktx_transcode_fmt_e> transcodeTarget{};
    std::string transcodeTargetName{};
    uint32_t transcodeSwizzleComponents{};
    std::string transcodeSwizzle{};

    void init(cxxopts::Options&) {}

    void process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter& report) {
        const char* argName = nullptr;
        if (TRANSCODE_CMD) {
            // "transcode" command - optional "target" argument
            argName = "target";
        } else {
            // "extract" command - optional "transcode" argument
            argName = "transcode";
        }

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
                report.fatal(RETURN_CODE_INVALID_ARGUMENTS, "Invalid transcode target: \"{}\".", argStr);

            transcodeTarget = it->second.first;
            transcodeTargetName = argStr;
            transcodeSwizzleComponents = it->second.second;
        }
    }

    std::optional<khr_df_model_channels_e> getChannelType(const KTXTexture2& texture, uint32_t index) {
        const auto* bdfd = (texture->pDfd + 1);

        if (KHR_DFDSAMPLECOUNT(bdfd) <= index)
            return std::nullopt;

        return khr_df_model_channels_e(KHR_DFDSVAL(bdfd, index, CHANNELID));
    }

    void validateTextureTranscode(const KTXTexture2& texture, Reporter& report) {
        const auto* bdfd = (texture->pDfd + 1);
        const auto sample0 = getChannelType(texture, 0);
        const auto sample1 = getChannelType(texture, 1);

        if (texture->supercompressionScheme == KTX_SS_BASIS_LZ) {
            uint32_t defaultComponents = 0;
            if (sample0 == KHR_DF_CHANNEL_ETC1S_RGB && sample1 == KHR_DF_CHANNEL_ETC1S_AAA) {
                defaultComponents = 4;
                transcodeSwizzle = "rgba";
            } else if (sample0 == KHR_DF_CHANNEL_ETC1S_RGB) {
                defaultComponents = 3;
                transcodeSwizzle = "rgb1";
            } else if (sample0 == KHR_DF_CHANNEL_ETC1S_RRR && sample1 == KHR_DF_CHANNEL_ETC1S_GGG) {
                defaultComponents = 2;
                transcodeSwizzle = "ra01";
            } else if (sample0 == KHR_DF_CHANNEL_ETC1S_RRR) {
                defaultComponents = 1;
                transcodeSwizzle = "r001";
            } else {
                report.fatal(RETURN_CODE_INVALID_FILE, "Unsupported channel types for Basis-LZ transcoding: {}, {}",
                    sample0 ? toString(KHR_DF_MODEL_ETC1S, *sample0) : "-",
                    sample1 ? toString(KHR_DF_MODEL_ETC1S, *sample1) : "-");
            }

            if (!transcodeTarget.has_value()) {
                transcodeTarget = KTX_TTF_RGBA32;
                transcodeTargetName = "rgba8";
                transcodeSwizzleComponents = defaultComponents;
            }

            switch (transcodeTarget.value()) {
            case KTX_TTF_ETC1_RGB: [[fallthrough]];
            case KTX_TTF_ETC2_RGBA: [[fallthrough]];
            case KTX_TTF_ETC2_EAC_R11: [[fallthrough]];
            case KTX_TTF_ETC2_EAC_RG11: [[fallthrough]];
            case KTX_TTF_BC1_RGB: [[fallthrough]];
            case KTX_TTF_BC3_RGBA: [[fallthrough]];
            case KTX_TTF_BC4_R: [[fallthrough]];
            case KTX_TTF_BC5_RG: [[fallthrough]];
            case KTX_TTF_BC7_RGBA: [[fallthrough]];
            case KTX_TTF_ASTC_4x4_RGBA: [[fallthrough]];
            case KTX_TTF_RGBA32:
                break;
            default:
                // Unsupported transcode target for BasisLZ
                report.fatal(RETURN_CODE_INVALID_ARGUMENTS, "Invalid BasisLZ transcode target: \"{}\"", transcodeTargetName);
            }
        } else if (khr_df_model_e(KHR_DFDVAL(bdfd, MODEL)) == KHR_DF_MODEL_UASTC) {
            uint32_t defaultComponents = 0;
            if (sample0 == KHR_DF_CHANNEL_UASTC_RGBA) {
                defaultComponents = 4;
                transcodeSwizzle = "rgba";
            } else if (sample0 == KHR_DF_CHANNEL_UASTC_RGB) {
                defaultComponents = 3;
                transcodeSwizzle = "rgb1";
            } else if (sample0 == KHR_DF_CHANNEL_UASTC_RRRG) {
                defaultComponents = 2;
                transcodeSwizzle = "ra01";
            } else if (sample0 == KHR_DF_CHANNEL_UASTC_RG) {
                defaultComponents = 2;
                transcodeSwizzle = "rg01";
            } else if (sample0 == KHR_DF_CHANNEL_UASTC_RRR) {
                defaultComponents = 1;
                transcodeSwizzle = "r001";
            } else {
                report.fatal(RETURN_CODE_INVALID_FILE, "Unsupported channel type for UASTC transcoding: {}",
                    sample0 ? toString(KHR_DF_MODEL_UASTC, *sample0) : "-");
            }

            if (!transcodeTarget.has_value()) {
                transcodeTarget = KTX_TTF_RGBA32;
                transcodeTargetName = "rgba8";
                transcodeSwizzleComponents = defaultComponents;
            }

            switch (transcodeTarget.value()) {
            case KTX_TTF_ETC1_RGB: [[fallthrough]];
            case KTX_TTF_ETC2_RGBA: [[fallthrough]];
            case KTX_TTF_ETC2_EAC_R11: [[fallthrough]];
            case KTX_TTF_ETC2_EAC_RG11: [[fallthrough]];
            case KTX_TTF_BC1_RGB: [[fallthrough]];
            case KTX_TTF_BC3_RGBA: [[fallthrough]];
            case KTX_TTF_BC4_R: [[fallthrough]];
            case KTX_TTF_BC5_RG: [[fallthrough]];
            case KTX_TTF_BC7_RGBA: [[fallthrough]];
            case KTX_TTF_ASTC_4x4_RGBA: [[fallthrough]];
            case KTX_TTF_RGBA32:
                break;
            default:
                // Unsupported transcode target for BasisLZ
                report.fatal(RETURN_CODE_INVALID_ARGUMENTS, "Invalid UASTC transcode target: \"{}\"", transcodeTargetName);
            }
        } else if (transcodeTarget.has_value()) {
            // If neither the supercompression is BasisLZ, nor the DFD color model is UASTC,
            // generate an error and exit.
            report.fatal(RETURN_CODE_INVALID_FILE, "Requested transcoding but input file is neither BasisLZ, nor UASTC");
        }
    }
};

} // namespace ktx

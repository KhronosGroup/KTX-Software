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

} // namespace ktx

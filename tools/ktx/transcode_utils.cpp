// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include "transcode_utils.h"
#include "image.hpp"

// -------------------------------------------------------------------------------------------------

namespace ktx {

std::optional<khr_df_model_channels_e> getChannelType(const KTXTexture2& texture, uint32_t index) {
    const auto* bdfd = (texture->pDfd + 1);

    if (KHR_DFDSAMPLECOUNT(bdfd) <= index)
        return std::nullopt;

    return khr_df_model_channels_e(KHR_DFDSVAL(bdfd, index, CHANNELID));
}

TranscodeSwizzleInfo determineTranscodeSwizzle(const KTXTexture2& texture, Reporter& report) {
    TranscodeSwizzleInfo result;

    const auto* bdfd = texture->pDfd + 1;
    const auto sample0 = getChannelType(texture, 0);
    const auto sample1 = getChannelType(texture, 1);

    if (texture->supercompressionScheme == KTX_SS_BASIS_LZ) {
        result.defaultNumComponents = 0;
        if (sample0 == KHR_DF_CHANNEL_ETC1S_RGB && sample1 == KHR_DF_CHANNEL_ETC1S_AAA) {
            result.defaultNumComponents = 4;
            result.swizzle = "rgba";
        } else if (sample0 == KHR_DF_CHANNEL_ETC1S_RGB) {
            result.defaultNumComponents = 3;
            result.swizzle = "rgb1";
        } else if (sample0 == KHR_DF_CHANNEL_ETC1S_RRR && sample1 == KHR_DF_CHANNEL_ETC1S_GGG) {
            result.defaultNumComponents = 2;
            result.swizzle = "ra01";
        } else if (sample0 == KHR_DF_CHANNEL_ETC1S_RRR) {
            result.defaultNumComponents = 1;
            result.swizzle = "r001";
        } else {
            report.fatal(rc::INVALID_FILE, "Unsupported channel types for Basis-LZ transcoding: {}, {}",
                    sample0 ? toString(KHR_DF_MODEL_ETC1S, *sample0) : "-",
                    sample1 ? toString(KHR_DF_MODEL_ETC1S, *sample1) : "-");
        }
    } else if (khr_df_model_e(KHR_DFDVAL(bdfd, MODEL)) == KHR_DF_MODEL_UASTC) {
        result.defaultNumComponents = 0;
        if (sample0 == KHR_DF_CHANNEL_UASTC_RGBA) {
            result.defaultNumComponents = 4;
            result.swizzle = "rgba";
        } else if (sample0 == KHR_DF_CHANNEL_UASTC_RGB) {
            result.defaultNumComponents = 3;
            result.swizzle = "rgb1";
        } else if (sample0 == KHR_DF_CHANNEL_UASTC_RRRG) {
            result.defaultNumComponents = 2;
            result.swizzle = "ra01";
        } else if (sample0 == KHR_DF_CHANNEL_UASTC_RG) {
            result.defaultNumComponents = 2;
            result.swizzle = "rg01";
        } else if (sample0 == KHR_DF_CHANNEL_UASTC_RRR) {
            result.defaultNumComponents = 1;
            result.swizzle = "r001";
        } else {
            report.fatal(rc::INVALID_FILE, "Unsupported channel type for UASTC transcoding: {}",
                    sample0 ? toString(KHR_DF_MODEL_UASTC, *sample0) : "-");
        }
    } else {
        report.fatal(rc::INVALID_FILE, "Requested transcoding but input file is neither BasisLZ, nor UASTC");
    }

    return result;
}

} // namespace ktx

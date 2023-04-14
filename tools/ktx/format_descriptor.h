// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "command.h"
#include "formats.h"
#include "imageio.h"


namespace ktx {

[[nodiscard]] inline FormatDescriptor createFormatDescriptor(const uint32_t* dfd) {
    const auto* bdfd = dfd + 1;

    FormatDescriptor::basicDescriptor basic;
    basic.model = khr_df_model_e(KHR_DFDVAL(bdfd, MODEL));
    basic.primaries = khr_df_primaries_e(KHR_DFDVAL(bdfd, PRIMARIES));
    basic.transfer = khr_df_transfer_e(KHR_DFDVAL(bdfd, TRANSFER));
    basic.flags = khr_df_flags_e(KHR_DFDVAL(bdfd, FLAGS));
    basic.texelBlockDimension0 = KHR_DFDVAL(bdfd, TEXELBLOCKDIMENSION0);
    basic.texelBlockDimension1 = KHR_DFDVAL(bdfd, TEXELBLOCKDIMENSION1);
    basic.texelBlockDimension2 = KHR_DFDVAL(bdfd, TEXELBLOCKDIMENSION2);
    basic.texelBlockDimension3 = KHR_DFDVAL(bdfd, TEXELBLOCKDIMENSION3);
    basic.bytesPlane0 = KHR_DFDVAL(bdfd, BYTESPLANE0);
    basic.bytesPlane1 = KHR_DFDVAL(bdfd, BYTESPLANE1);
    basic.bytesPlane2 = KHR_DFDVAL(bdfd, BYTESPLANE2);
    basic.bytesPlane3 = KHR_DFDVAL(bdfd, BYTESPLANE3);
    basic.bytesPlane4 = KHR_DFDVAL(bdfd, BYTESPLANE4);
    basic.bytesPlane5 = KHR_DFDVAL(bdfd, BYTESPLANE5);
    basic.bytesPlane6 = KHR_DFDVAL(bdfd, BYTESPLANE6);
    basic.bytesPlane7 = KHR_DFDVAL(bdfd, BYTESPLANE7);

    std::vector<FormatDescriptor::sample> samples;
    for (uint32_t i = 0; i < KHR_DFDSAMPLECOUNT(bdfd); ++i) {
        auto& sample = samples.emplace_back();
        sample.bitOffset = KHR_DFDSVAL(bdfd, i, BITOFFSET);
        sample.bitLength = KHR_DFDSVAL(bdfd, i, BITLENGTH);
        sample.channelType = KHR_DFDSVAL(bdfd, i, CHANNELID);
        const auto dataType = KHR_DFDSVAL(bdfd, i, QUALIFIERS);
        sample.qualifierFloat = (dataType & KHR_DF_SAMPLE_DATATYPE_FLOAT) != 0;
        sample.qualifierSigned = (dataType & KHR_DF_SAMPLE_DATATYPE_SIGNED) != 0;
        sample.qualifierExponent = (dataType & KHR_DF_SAMPLE_DATATYPE_EXPONENT) != 0;
        sample.qualifierLinear = (dataType & KHR_DF_SAMPLE_DATATYPE_LINEAR) != 0;
        sample.samplePosition0 = KHR_DFDSVAL(bdfd, i, SAMPLEPOSITION0);
        sample.samplePosition1 = KHR_DFDSVAL(bdfd, i, SAMPLEPOSITION1);
        sample.samplePosition2 = KHR_DFDSVAL(bdfd, i, SAMPLEPOSITION2);
        sample.samplePosition3 = KHR_DFDSVAL(bdfd, i, SAMPLEPOSITION3);
        sample.lower = KHR_DFDSVAL(bdfd, i, SAMPLELOWER);
        sample.upper = KHR_DFDSVAL(bdfd, i, SAMPLEUPPER);
    }

    return FormatDescriptor{basic, std::move(samples)};
}

[[nodiscard]] inline FormatDescriptor createFormatDescriptor(VkFormat vkFormat, Reporter& report) {
    const auto dfd = std::unique_ptr<uint32_t[], decltype(std::free)*>(vk2dfd(vkFormat), std::free);
    if (!dfd)
        report.fatal(rc::DFD_FAILURE, "Failed to create format descriptor for: {}", toString(vkFormat));

    return createFormatDescriptor(dfd.get());
}

} // namespace ktx

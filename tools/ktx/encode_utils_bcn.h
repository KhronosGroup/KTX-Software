// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "command.h"
#include "ktx.h"
#include "utility.h"

#include <string>
#include <thread>

// -------------------------------------------------------------------------------------------------

namespace ktx {

/**
//! [command options_encode_bcn]
<dl>
    <dt>
        BCn:
    </dt>
    <dd></dd>

    <dl>
        <dt>\--bcn-quality &lt;level&gt;</dt>
        <dd>The quality level configures the quality-performance tradeoff for
            BC1, BC3, and BC7 encoders. For BC1/BC3, range is [0,19]. For BC7,
            one of the following presets has to be supplied:
            <table>
                <tr><th>Level      </th> <th> Quality (only BC1/BC3)       </th></tr>
                <tr><td>fastest    </td> <td>(equivalent to quality =   0) </td></tr>
                <tr><td>faster     </td> <td>(equivalent to quality =   2) </td></tr>
                <tr><td>fast       </td> <td>(equivalent to quality =   5) </td></tr>
                <tr><td>medium     </td> <td>(equivalent to quality =  10) </td></tr>
                <tr><td>thorough   </td> <td>(equivalent to quality =  15) </td></tr>
                <tr><td>exhaustive </td> <td>(equivalent to quality =  19) </td></tr>
            </table>
            Note on BC1 vs. BC3 vs. BC7: apart from lower VRAM consumption (4bpp
            vs. 8bpp) and better GPU texture cache efficiency, there's little
            need to use BC1 now. BC7 offers significantly better quality than
            BC1 and BC3. BC3 still has an advantage vs. BC7, because it very
            strongly separates how RGB is encoded from the alpha channel,
            in a predictable way.
        </dd>
        <dt>\--bcn-rdo</dt>
        <dd>Enable BCn LDR RDO post-processing. HDR formats (BC6HU/BC6HS) are
            currently not supported.</dd>
        <dt>\--bcn-rdo-l &lt;lambda&gt;</dt>
        <dd>Set BCn RDO quality scalar to the specified value. Lower values
            yield higher quality/larger supercompressed files, higher values
            yield lower quality/smaller supercompressed files. A good range to
            try is [.25,10]. For normal maps a good range is [.25,.75]. The full
            range is [.001,10.0]. Default is 1.0.</dd>
        <dt>\--bcn-rdo-d &lt;dictsize&gt;</dt>
        <dd>Set BCn RDO dictsize size in bytes. Default is 4096. Lower
            values=faster, but give less compression. Range is [64,65536].</dd>
        <dt>\--bcn-rdo-b &lt;scale&gt;</dt>
        <dd>Set BCn RDO max smooth block error scale. Range is [1.0,300.0].
            Default is to automatically compute this. 1.0 is disabled. Larger
            values suppress more artifacts (and allocate more bits) on smooth
            blocks.</dd>
        <dt>\--bcn-rdo-s &lt;deviation&gt;</dt>
        <dd>Set BCn RDO max smooth block standard deviation. Range is
            [.01,65536.0]. Default is 18.0. Larger values expand the range of
            blocks considered smooth.</dd>
        <dt>\--bcn-rdo-r &lt;ratio&gt;</dt>
        <dd>How much the RMS error of a block is allowed to increase before a
            trial is rejected. 1.0=no increase allowed, 1.05=5% increase
            allowed, etc. Range is [1.001, 100.0]. Default is 10.0.</dd>
        <dt>\--bcn-rdo-no-ultrasmooth</dt>
        <dd>Disable encoding of extremely smooth blocks with a significantly
            higher MSE scale factor. Results in significantly more artifacts on
            regions containing very smooth blocks (e.g., gradients, skies,
            etc.). This does improve rate-distortion performance, though. BC4
            and BC5 formats do not support ultrasmooth block handling.</dd>
        <dt>\--bcn-rdo-try-one-match</dt>
        <dd>Inject up to 1 match into each block instead of up to two matches.
            Results in slightly faster, but lower compression.</dd>
        <dt>\--bcn-rdo-skip-zero-mse</dt>
        <dd>Skip blocks that have zero mean-squared error (MSE). Might result in
            faster but potentially lower compression.</dd>
        <dt>\--bcn-rdo-m</dt>
        <dd>Disable RDO multithreading (potentially slightly higher
            compression).</dd>
    </dl>
</dl>
//! [command options_encode_bcn]
*/

struct OptionsEncodeBCn : public ktxBCnParams {
    /* high-level params */
    // inline static const char* kBCnQuality = "quality";
    // inline static const char* kBCnEffort = "effort";

    /* low-level params */
    inline static const char* kBCnQuality = "bcn-quality";
    inline static const char* kBCnRdo = "bcn-rdo";
    inline static const char* kBCnRdoL = "bcn-rdo-l";
    inline static const char* kBCnRdoD = "bcn-rdo-d";
    inline static const char* kBCnRdoB = "bcn-rdo-b";
    inline static const char* kBCnRdoS = "bcn-rdo-s";
    inline static const char* kBCnRdoR = "bcn-rdo-r";
    inline static const char* kBCnRdoNoUltrasmoothBlocks = "bcn-rdo-no-ultrasmooth";
    inline static const char* kBCnRdoTryOneMatch = "bcn-rdo-try-one-match";
    inline static const char* kBCnRdoSkipZeroMSEBlocks = "bcn-rdo-skip-zero-mse";
    inline static const char* kBCnRdoNoMultithreading = "bcn-rdo-m";

    inline static const char* kBCnOptions[] = {
        // kBCnEffort,
        kBCnQuality,
        kBCnRdo,
        kBCnRdoL,
        kBCnRdoD,
        kBCnRdoB,
        kBCnRdoS,
        kBCnRdoNoUltrasmoothBlocks,
        kBCnRdoR,
        kBCnRdoTryOneMatch,
        kBCnRdoSkipZeroMSEBlocks,
    };

    ClampedOption<ktx_uint32_t> bcnCompressionQuality;
    ClampedOption<float> bcnRDOQualityScalar;
    ClampedOption<ktx_uint32_t> bcnRDODictSize;
    ClampedOption<float> bcnRDOMaxSmoothBlockErrorScale;
    ClampedOption<float> bcnRDOMaxSmoothBlockStdDev;
    ClampedOption<float> bcnRDOMaxAllowedRMSIncreaseRatio;

    std::string bcnOptions{};
    // This is added here so that when OptionsEncodeBCn is combined with other
    // options (e.g., from ASTC) we access this property to know which encoder
    // should be used.
    bool encodeBCn = false;

    OptionsEncodeBCn()
        : bcnCompressionQuality(ktxBCnParams::bcnCompressionQuality, 0u,
                                KTX_PACK_BCN_QUALITY_LEVEL_MAX),
          bcnRDOQualityScalar(ktxBCnParams::bcnRDOQualityScalar, 0.001f, 50.0f),
          bcnRDODictSize(ktxBCnParams::bcnRDODictSize, 64u, 65536u),
          bcnRDOMaxSmoothBlockErrorScale(ktxBCnParams::bcnRDOMaxSmoothBlockErrorScale, 1.0f,
                                         300.0f),
          bcnRDOMaxSmoothBlockStdDev(ktxBCnParams::bcnRDOMaxSmoothBlockStdDev, 0.01f, 65536.0f),
          bcnRDOMaxAllowedRMSIncreaseRatio(ktxBCnParams::bcnRDOMaxAllowedRMSIncreaseRatio, 1.001f,
                                           100.0f) {
        structSize = sizeof(ktxBCnParams);
        threadCount = std::max<ktx_uint32_t>(1u, std::thread::hardware_concurrency());
        /* bcn is set depending in ktx create/encode commands not here */
        bcnCompressionQuality = KTX_PACK_BCN_QUALITY_LEVEL_THOROUGH;
        bcnRDO = false;
        bcnRDOQualityScalar = 1.0f;
        bcnRDOMaxSmoothBlockErrorScale = 10.0f;
        bcnRDOMaxSmoothBlockStdDev = 18.0f;
        bcnRDONoUltrasmoothBlockHandling = false;
        bcnRDOMaxAllowedRMSIncreaseRatio = 10.0f;
        bcnRDODictSize = 4096u;
        bcnRDOTryOneMatch = false;
        bcnRDOSkipZeroMSEBlocks = false;
        bcnRDONoMultithreading = false;
    }

    void init(cxxopts::Options& opts) {
        opts.add_options("Encode BCn")(
            kBCnQuality,
            "The quality level configures the quality-performance tradeoff for BC1, BC3, and BC7 "
            "encoders. For BC1/BC3, range is [0,19]. For BC7, one of the following presets has to "
            "be supplied:\n\n"
            "    Level      |  Quality (only for BC1/BC3)\n"
            "    ---------- | ----------------------------\n"
            "    fastest    | (equivalent to quality =  0)\n"
            "    faster     | (equivalent to quality =  2)\n"
            "    fast       | (equivalent to quality =  5)\n"
            "    medium     | (equivalent to quality = 10)\n"
            "    thorough   | (equivalent to quality = 15)\n"
            "    exhaustive | (equivalent to quality = 19)\n\n"
            "Default is 'thorough'. Note on BC1 vs. BC3 vs. BC7: apart from lower VRAM consumption "
            "(4bpp vs. 8bpp) and better GPU texture cache efficiency, there's little need to use "
            "BC1 now. BC7 offers significantly better quality than BC1 and BC3. BC3 still has an "
            "advantage vs. BC7, because it very strongly separates how RGB is encoded from the "
            "alpha channel, in a predictable way.",
            cxxopts::value<std::string>(),
            "<level>")(kBCnRdo,
                       "Enable BCn LDR RDO post-processing. HDR formats (BC6HU/BC6HS) "
                       "are currently not supported.")(
            kBCnRdoL,
            "Set BCn RDO quality scalar to the specified value. Lower values yield higher "
            "quality/larger supercompressed files, higher values yield lower quality/smaller "
            "supercompressed files. A good range to try is [.25,10]. For normal maps a good range "
            "is [.25,.75]. The full range is [.001,10.0]. Default is 1.0.",
            cxxopts::value<float>(),
            "<lambda>")(kBCnRdoD,
                        "Set BCn RDO dictsize size in bytes. Default is 4096. Lower values=faster, "
                        "but give less compression. Range is [64,65536].",
                        cxxopts::value<uint32_t>(), "<dictsize>")(
            kBCnRdoB,
            "Set BCn RDO max smooth block error scale. Range is [1.0,300.0]. Default is to "
            "automatically compute this. 1.0 is disabled. Larger values suppress more artifacts "
            "(and allocate more bits) on smooth blocks.",
            cxxopts::value<float>(), "<scale>")(
            kBCnRdoS,
            "Set BCn RDO max smooth block standard deviation. Range is [.01,65536.0]. "
            "Default is 18.0. Larger values expand the range of blocks considered smooth.",
            cxxopts::value<float>(), "<deviation>")(
            kBCnRdoR,
            "How much the RMS error of a block is allowed to increase before a trial is rejected. "
            "1.0=no increase allowed, 1.05=5% increase allowed, etc. Range is [1.001, 100.0]. "
            "Default is 10.0.",
            cxxopts::value<float>(),
            "<ratio>")(kBCnRdoNoUltrasmoothBlocks,
                       "Disable encoding of extremely smooth blocks with a significantly "
                       "higher MSE scale factor. Results in significantly more artifacts on "
                       "regions containing very smooth blocks (e.g., gradients, skies, etc.). "
                       "This does improve rate-distortion performance, though. BC4 and BC5 formats "
                       "do not support ultrasmooth block handling.")(
            kBCnRdoTryOneMatch,
            "Inject up to 1 match into each block instead of up to two matches. Results "
            "in slightly faster, but noticeably lower compression.")(
            kBCnRdoSkipZeroMSEBlocks,
            "Skip blocks that have zero mean-squared error (MSE). Might result in faster but "
            "potentially lower compression.")(
            kBCnRdoNoMultithreading,
            "Disable RDO multithreading (potentially slightly higher compression).");
    }

    void captureBCnOption(const char* name) { bcnOptions += fmt::format(" --{}", name); }

    template <typename T>
    T captureBCnOption(cxxopts::ParseResult& args, const char* name) {
        const T value = args[name].as<T>();
        bcnOptions += fmt::format(" --{} {}", name, value);
        return value;
    }

    void process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter& report) {
        const char* rdo_needs_to_be_set_err_msg =
            "RDO has to be enabled (via --rdo flag) in order for RDO-specific arguments to take "
            "effect.";

        if (args[kBCnQuality].count()) {
            static std::unordered_map<std::string, ktx_pack_bcn_quality_levels_e>
                bcn_quality_mapping{{"fastest", KTX_PACK_BCN_QUALITY_LEVEL_FASTEST},
                                    {"faster", KTX_PACK_BCN_QUALITY_LEVEL_FASTER},
                                    {"fast", KTX_PACK_BCN_QUALITY_LEVEL_FAST},
                                    {"medium", KTX_PACK_BCN_QUALITY_LEVEL_MEDIUM},
                                    {"thorough", KTX_PACK_BCN_QUALITY_LEVEL_THOROUGH},
                                    {"exhaustive", KTX_PACK_BCN_QUALITY_LEVEL_EXHAUSTIVE}};
            const auto qualityLevelStr =
                to_lower_copy(captureBCnOption<std::string>(args, kBCnQuality));
            const auto it = bcn_quality_mapping.find(qualityLevelStr);
            if (it == bcn_quality_mapping.end()) {
                // try to parse explicitly provided value (advanced usecase)
                try {
                    bcnCompressionQuality = static_cast<ktx_uint32_t>(std::stoul(qualityLevelStr));
                } catch (const std::exception&) {
                    report.fatal_usage(
                        "Invalid bcn-quality value. Expected a quality level string preset (e.g., "
                        "'medium'), or range [0,19] for BC1/BC3 but got: \"{}\"",
                        qualityLevelStr);
                }
            } else {
                bcnCompressionQuality = it->second;
            }
        }

        /* RDO params */

        if (args[kBCnRdo].count()) {
            captureBCnOption(kBCnRdo);
            bcnRDO = true;
        }

        if (args[kBCnRdoL].count()) {
            if (!bcnRDO) report.fatal_usage(rdo_needs_to_be_set_err_msg);
            bcnRDOQualityScalar = captureBCnOption<float>(args, kBCnRdoL);
        }

        if (args[kBCnRdoD].count()) {
            if (!bcnRDO) report.fatal_usage(rdo_needs_to_be_set_err_msg);
            bcnRDODictSize = captureBCnOption<uint32_t>(args, kBCnRdoD);
        }

        if (args[kBCnRdoB].count()) {
            if (!bcnRDO) report.fatal_usage(rdo_needs_to_be_set_err_msg);
            bcnRDOMaxSmoothBlockErrorScale = captureBCnOption<float>(args, kBCnRdoB);
        }

        if (args[kBCnRdoS].count()) {
            if (!bcnRDO) report.fatal_usage(rdo_needs_to_be_set_err_msg);
            bcnRDOMaxSmoothBlockStdDev = captureBCnOption<float>(args, kBCnRdoS);
        }

        if (args[kBCnRdoNoUltrasmoothBlocks].count()) {
            if (!bcnRDO) report.fatal_usage(rdo_needs_to_be_set_err_msg);
            captureBCnOption(kBCnRdoNoUltrasmoothBlocks);
            bcnRDONoUltrasmoothBlockHandling = true;
        }

        if (args[kBCnRdoR].count()) {
            if (!bcnRDO) report.fatal_usage(rdo_needs_to_be_set_err_msg);
            bcnRDOMaxAllowedRMSIncreaseRatio = captureBCnOption<float>(args, kBCnRdoR);
        }

        if (args[kBCnRdoTryOneMatch].count()) {
            if (!bcnRDO) report.fatal_usage(rdo_needs_to_be_set_err_msg);
            captureBCnOption(kBCnRdoTryOneMatch);
            bcnRDOTryOneMatch = true;
        }

        if (args[kBCnRdoSkipZeroMSEBlocks].count()) {
            if (!bcnRDO) report.fatal_usage(rdo_needs_to_be_set_err_msg);
            captureBCnOption(kBCnRdoSkipZeroMSEBlocks);
            bcnRDOSkipZeroMSEBlocks = true;
        }

        if (args[kBCnRdoNoMultithreading].count()) {
            if (!bcnRDO) report.fatal_usage(rdo_needs_to_be_set_err_msg);
            captureBCnOption(kBCnRdoNoMultithreading);
            bcnRDONoMultithreading = true;
        }
    }
};

}  // namespace ktx

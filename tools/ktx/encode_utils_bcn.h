// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "command.h"
#include "ktx.h"
#include "utility.h"

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
      TODO
    </dl>
</dl>
//! [command options_encode_bcn]
*/

struct OptionsEncodeBCn : public ktxBCnParams {
    inline static const char* kBC1Mode = "bc1-mode";
    inline static const char* kBC1Quality = "bc1-quality";
    inline static const char* kBC7Quality = "bc7-quality";
    inline static const char* kRDO = "rdo";
    inline static const char* kRDOLambda = "rdo-lambda";
    inline static const char* kRDOWindowLoopbackSize = "rdo-window-loopback-size";
    inline static const char* kRDOMaxSmoothBlockMSEScale = "rdo-max-smooth-block-mse-scale";
    inline static const char* kRDOMaxSmoothBlockStdDev = "rdo-max-smooth-block-std-dev";
    inline static const char* kRDOUltrasmoothBlockHandling = "rdo-ultrasmooth-blocks";
    inline static const char* kRDOMaxAllowedRMSIncreaseRatio = "rdo-max-rms-ratio";
    inline static const char* kRDOTryOneMatch = "rdo-try-one-match";
    inline static const char* kRDOSkipZeroMSEBlocks = "rdo-skip-zero-mse";
    inline static const char* kRDOAllowRelativeMovement = "rdo-allow-relative-movement";

    inline static const char* kBCnOptions[] = {
        kBC1Mode,
        kBC1Quality,
        kBC7Quality,
        kRDO,
        kRDOLambda,
        kRDOWindowLoopbackSize,
        kRDOMaxSmoothBlockMSEScale,
        kRDOMaxSmoothBlockStdDev,
        kRDOUltrasmoothBlockHandling,
        kRDOMaxAllowedRMSIncreaseRatio,
        kRDOTryOneMatch,
        kRDOSkipZeroMSEBlocks,
        kRDOAllowRelativeMovement,
    };

    ClampedOption<ktx_uint32_t> bc1CompressionQuality;
    ClampedOption<ktx_uint32_t> bc7CompressionQuality;
    ClampedOption<float> rdoQualityScalar;
    ClampedOption<ktx_uint32_t> rdoWindowLoopbackSize;
    ClampedOption<float> rdoMaxSmoothBlockMseScale;
    ClampedOption<float> rdoMaxSmoothBlockStdDev;
    ClampedOption<float> rdoMaxAllowedRMSIncreaseRatio;

    std::string bcnOptions{};
    // This is added here so that when OptionsEncodeBCn is combined with other
    // options (e.g., from ASTC) we access this property to know which encoder
    // should be used.
    bool encodeBCn = false;

    OptionsEncodeBCn()
        : bc1CompressionQuality(ktxBCnParams::bc1CompressionQuality, 0u,
                                KTX_PACK_BC1_QUALITY_LEVEL_MAX),
          bc7CompressionQuality(ktxBCnParams::bc7CompressionQuality, 0u,
                                KTX_PACK_BC7_QUALITY_LEVEL_MAX),
          rdoQualityScalar(ktxBCnParams::rdoQualityScalar, 0.10f, 50.0f),
          rdoWindowLoopbackSize(ktxBCnParams::rdoWindowLoopbackSize, 64u, 65536u),
          rdoMaxSmoothBlockMseScale(ktxBCnParams::rdoMaxSmoothBlockMseScale, 0.0f, 300.0f),
          rdoMaxSmoothBlockStdDev(ktxBCnParams::rdoMaxSmoothBlockStdDev, 0.01f, 65536.0f),
          rdoMaxAllowedRMSIncreaseRatio(ktxBCnParams::rdoMaxAllowedRMSIncreaseRatio, 1.001f,
                                        100.0f) {
        structSize = sizeof(ktxBCnParams);
        threadCount = std::max<ktx_uint32_t>(1u, std::thread::hardware_concurrency());
        /* bcn is set depending in ktx create/encode commands not here */
        normalMap = false;
        bc1ApproxMode = KTX_PACK_BC1_BLOCK_APPROX_MODE_IDEAL;
        bc1CompressionQuality = KTX_PACK_BC1_QUALITY_LEVEL_THOROUGH;
        bc7CompressionQuality = KTX_PACK_BC7_QUALITY_LEVEL_THOROUGH;
        rdo = false;
        rdoQualityScalar = 0.5f;
        rdoAutoSmoothBlockMaxMSEScale = true;
        rdoMaxSmoothBlockMseScale = 18.0f;
        rdoMaxSmoothBlockStdDev = 10.0f;
        rdoUltrasmoothBlockHandling = false;
        rdoMaxAllowedRMSIncreaseRatio = 10.0f;
        rdoWindowLoopbackSize = 128u;
        rdoTry2Matches = true;
        rdoSkipZeroMSEBlocks = false;
        rdoAllowRelativeMovement = false;
    }

    void init(cxxopts::Options& opts) {
        opts.add_options("Encode BCn")(
            kBC1Mode,
            "BC1 (subsequently BC3) approximation mode (for both: encoding and decoding). Default "
            "is 'ideal'. If "
            "you encode textures for a specific vendor's GPU's, beware that using that texture "
            "data on other GPU's may result in ugly artifacts. Set to 'ideal' unless you know the "
            "texture data will only be deployed or used on a specific vendor's GPU. Can be set to "
            "one of the following:\n\n"
            "    Mode       |  Description                                   \n"
            "    ---------- | -----------------------------------------------\n"
            "    ideal      | The default mode. No rounding for 4-color      \n"
            "               | colors 2,3. This matches the D3D10 docs on BC1.\n"
            "    - - - - -  | - - - - - - - - - - - - - - - - - - - - - - - -\n"
            "    nvidia     | NVidia GPU mode. May produce artifacts on      \n"
            "               | non-NVidia GPUs.                               \n"
            "    - - - - -  | - - - - - - - - - - - - - - - - - - - - - - - -\n"
            "    amd        | AMD GPU mode. May produce artifacts on non-AMD \n"
            "               | GPUs.                                          \n"
            "    - - - - -  | - - - - - - - - - - - - - - - - - - - - - - - -\n"
            "    ideal4     | Matches AMD Compressonator's output. Rounds    \n"
            "               | 4-color colors 2,3 (not 3-color color 2). This \n"
            "               | matches the D3D9 docs on DXT1.                 \n",
            cxxopts::value<std::string>(), "<mode>")(
            kBC1Quality,
            "The quality level configures the quality-performance tradeoff for BC1 and, "
            "subsequently, BC3 encoders. The quality level can be set between 'fastest' "
            "(0) and most 'exhaustive' (19). Default is 'thorough' (15). Can also be set "
            "via the following aliases:\n\n"
            "    Level      |  Quality\n"
            "    ---------- | ---------------------------- \n"
            "    fastest    | (equivalent to quality =  0) \n"
            "    fast       | (equivalent to quality =  5) \n"
            "    medium     | (equivalent to quality = 10) \n"
            "    thorough   | (equivalent to quality = 15) \n"
            "    exhaustive | (equivalent to quality = 19) \n\n"
            "Note on BC1 vs. BC3 vs. BC7: apart from lower VRAM consumption (4bpp vs. "
            "8bpp) and better GPU texture cache efficiency, there's little need to use "
            "BC1 now. BC3 still has an advantage vs. BC7, because it very strongly "
            "separates how RGB is encoded from the alpha channel, in a predictable way.\n",
            cxxopts::value<std::string>(),
            "<level>")(kBC7Quality,
                       "The quality level configures the quality-performance tradeoff"
                       " for BC7 encoder. Default is 'medium'. The quality level can be"
                       " set between fastest and exhaustive via the following fixed"
                       " quality presets where each preset is an OR'ed set of flags:\n\n"
                       "    Level      |  OR'ed flags                 \n"
                       "    ---------- | ---------------------------- \n"
                       "    fastest    | (equivalent to flags =  128) \n"
                       "    faster     | (equivalent to flags =  176) \n"
                       "    fast       | (equivalent to flags =  179) \n"
                       "    medium     | (equivalent to flags =  255) \n"
                       "    thorough   | (equivalent to flags = 1023) \n"
                       "    exhaustive | (equivalent to flags = 3967) \n",
                       cxxopts::value<std::string>(), "<level>")(
            kRDO,
            "Enable Rate Distortion Optimization (RDO) post-processing step on BCn-encoded blocks "
            "to reduce entropy for Deflate-based compressors. This is primarily used to reduce "
            "size on disk when a further compression is applied (Zlib/ZSTD supercompressions). RDO "
            "parameters are only used if this is set. Setting this might result in significantly "
            "slower encoding time at the benefit of potentially significantly lower bit rate for "
            "Deflate-based compressors (i.e., number of bits per encoded texel). Default is "
            "false.\n")(
            kRDOLambda,
            "RDO quality scalar (lambda). Controls rate vs. distortion tradeoff. Lower values "
            "yield higher quality/larger LZ compressed files, higher values yield lower "
            "quality/smaller LZ compressed files. A good range to try is [0.25,8]. Full range is "
            "[0.1,50.0]. Default is 0.5. The post-processor tries to minimize: "
            "distortion*smooth_block_scale + rate*lambda (rate is approximate LZ bits and "
            "distortion is scaled MSE multiplied against the smooth block MSE weighting factor). "
            "Larger values push the post-processor towards optimizing more for lower rate, and "
            "smaller values more for distortion. 0=minimal distortion.\n",
            cxxopts::value<float>(), "<arg>")(
            kRDOMaxSmoothBlockMSEScale,
            "RDO max MSE scaling factor for blocks considered to be smooth/flat. A value of 1.0 "
            "means no smooth block error scaling which may cause very noticeable artifacts for "
            "smooth/flat blocks (e.g., kodim23 test image). This value can be automatically "
            "computed based on the set RDO lamba by setting 'rdo-auto-mse-scale'. "
            "'rdo-max-smooth-block-std-dev' is used to compute, for a given block, the MSE scale "
            "factor in the range: 1.0 (i.e., not a smooth block) up to this max MSE scale factor. "
            "As to why an MSE factor has to be applied to smooth/flat blocks, the MSE for these "
            "blocks is too low relative to the visual impact they have when they get distorted. "
            "The solution implemented here is to compute the max std dev. of any component and use "
            "a linear function of that to scale block/trial MSE. Range is [1,300]. Default is to "
            "automatically compute this.\n",
            cxxopts::value<float>(), "<arg>")(
            kRDOMaxSmoothBlockStdDev,
            "RDO max smooth/flat block standard deviation. If the standard deviation of a block "
            "exceeds this value, then it won't be considered as a smooth block (i.e., the smooth "
            "block MSE scale factor will be set to 1 for this block). The smaller the ratio of the "
            "standard deviation of this block to this value the more the smooth block MSE scale "
            "factor approaches rdoMaxSmoothBlockMseScale. Range is [.01,65536.0]. Larger values "
            "expand the range of blocks considered smooth. Default is 10.0.\n",
            cxxopts::value<float>(), "<arg>")(
            kRDOUltrasmoothBlockHandling,
            "Detect extremely smooth blocks and encode them with a significantly higher MSE scale "
            "factor. When enabled, a per-block mask image is computed, filtered, then an array of "
            "per-block MSE scale factors is supplied to the ERT. The end result is much less "
            "significant artifacts on regions containing very smooth blocks (e.g., gradients). "
            "This does hurt rate-distortion performance.\n")(
            kRDOMaxAllowedRMSIncreaseRatio,
            "How much the RMS error of a block is allowed to increase before a trial is rejected. "
            "1.0=no increase allowed, 1.05=5% increase allowed, etc. Range is [1.001, 100.0]. "
            "Default is 10.0.\n",
            cxxopts::value<float>(), "<arg>")(
            kRDOWindowLoopbackSize,
            "The number of bytes the encoder can look back from each block to find matches. The "
            "larger this value, the slower the encoder but the higher the quality per LZ "
            "compressed bit. You don't need a huge window to get large gains. Even 64-512 byte "
            "windows can be fine. Range is [64,65536]. Default is 128.\n",
            cxxopts::value<uint32_t>(),
            "<arg>")(kRDOTryOneMatch,
                     "Inject up to 1 match into each block instead of up-to-two matches. Results "
                     "in slightly faster, but lower compression.\n")(
            kRDOSkipZeroMSEBlocks,
            "Skip blocks that have zero mean-squared error (MSR). Might result in "
            "faster compression speed but potentially lower compression.\n")(
            kRDOAllowRelativeMovement, "TODO: no idea still what this does.");
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

        /* BC1-5 params */

        if (args[kBC1Mode].count()) {
            static std::unordered_map<std::string, ktx_bc1_approx_mode_e> bc1_mode_mapping{
                {"ideal", KTX_PACK_BC1_BLOCK_APPROX_MODE_IDEAL},
                {"nvidia", KTX_PACK_BC1_BLOCK_APPROX_MODE_NVIDIA},
                {"amd", KTX_PACK_BC1_BLOCK_APPROX_MODE_AMD},
                {"ideal4", KTX_PACK_BC1_BLOCK_APPROX_MODE_IDEAL_ROUND_4}};
            const auto modeStr = to_lower_copy(captureBCnOption<std::string>(args, kBC1Mode));
            const auto it = bc1_mode_mapping.find(modeStr);
            if (it == bc1_mode_mapping.end())
                report.fatal_usage("Invalid bc1-mode: \"{}\"", modeStr);
            bc1ApproxMode = it->second;
        }

        if (args[kBC1Quality].count()) {
            static std::unordered_map<std::string, ktx_pack_bc1_quality_levels_e>
                bc1_quality_mapping{{"fastest", KTX_PACK_BC1_QUALITY_LEVEL_FASTEST},
                                    {"fast", KTX_PACK_BC1_QUALITY_LEVEL_FAST},
                                    {"medium", KTX_PACK_BC1_QUALITY_LEVEL_MEDIUM},
                                    {"thorough", KTX_PACK_BC1_QUALITY_LEVEL_THOROUGH},
                                    {"exhaustive", KTX_PACK_BC1_QUALITY_LEVEL_EXHAUSTIVE}};
            const auto qualityLevelStr =
                to_lower_copy(captureBCnOption<std::string>(args, kBC1Quality));
            const auto it = bc1_quality_mapping.find(qualityLevelStr);
            if (it == bc1_quality_mapping.end()) {
                // try to parse explicitly provided value (advanced usecase)
                try {
                    bc1CompressionQuality = std::stoul(qualityLevelStr);
                } catch (const std::exception&) {
                    report.fatal_usage(
                        "Invalid bc1-quality. Expected a quality level string alias (e.g., "
                        "'medium') or a value in range [0, 19] but got: \"{}\"",
                        qualityLevelStr);
                }
            } else {
                bc1CompressionQuality = it->second;
            }
        }

        /* BC7 params */

        if (args[kBC7Quality].count()) {
            static std::unordered_map<std::string, ktx_pack_bc7_quality_levels_e>
                bc7_quality_mapping{{"fastest", KTX_PACK_BC7_QUALITY_LEVEL_FASTEST},
                                    {"faster", KTX_PACK_BC7_QUALITY_LEVEL_FASTER},
                                    {"fast", KTX_PACK_BC7_QUALITY_LEVEL_FAST},
                                    {"medium", KTX_PACK_BC7_QUALITY_LEVEL_MEDIUM},
                                    {"thorough", KTX_PACK_BC7_QUALITY_LEVEL_THOROUGH},
                                    {"exhaustive", KTX_PACK_BC7_QUALITY_LEVEL_EXHAUSTIVE}};
            const auto qualityLevelStr =
                to_lower_copy(captureBCnOption<std::string>(args, kBC7Quality));
            const auto it = bc7_quality_mapping.find(qualityLevelStr);
            if (it == bc7_quality_mapping.end()) {
                // try to parse explicitly provided OR'ed flags (advanced usecase)
                try {
                    bc7CompressionQuality = std::stoul(qualityLevelStr);
                } catch (const std::exception&) {
                    report.fatal_usage(
                        "Invalid bc7-quality. Expected a quality level string alias (e.g., "
                        "'medium') or an OR'ed set of flags (e.g., 255) but got: \"{}\"",
                        qualityLevelStr);
                }
            }
            bc7CompressionQuality = it->second;
        }

        /* RDO params */

        if (args[kRDO].count()) {
            captureBCnOption(kRDO);
            rdo = true;
        }

        if (args[kRDOLambda].count()) {
            if (!rdo) report.fatal_usage(rdo_needs_to_be_set_err_msg);
            rdoQualityScalar = captureBCnOption<float>(args, kRDOLambda);
        }

        if (args[kRDOWindowLoopbackSize].count()) {
            if (!rdo) report.fatal_usage(rdo_needs_to_be_set_err_msg);
            rdoWindowLoopbackSize = captureBCnOption<uint32_t>(args, kRDOWindowLoopbackSize);
        }

        if (args[kRDOMaxSmoothBlockMSEScale].count()) {
            if (!rdo) report.fatal_usage(rdo_needs_to_be_set_err_msg);
            rdoMaxSmoothBlockMseScale = captureBCnOption<float>(args, kRDOMaxSmoothBlockMSEScale);
            rdoAutoSmoothBlockMaxMSEScale = false;
        }

        if (args[kRDOMaxSmoothBlockStdDev].count()) {
            if (!rdo) report.fatal_usage(rdo_needs_to_be_set_err_msg);
            rdoMaxSmoothBlockStdDev = captureBCnOption<float>(args, kRDOMaxSmoothBlockStdDev);
        }

        if (args[kRDOUltrasmoothBlockHandling].count()) {
            if (!rdo) report.fatal_usage(rdo_needs_to_be_set_err_msg);
            captureBCnOption(kRDOUltrasmoothBlockHandling);
            rdoUltrasmoothBlockHandling = true;
        }

        if (args[kRDOMaxAllowedRMSIncreaseRatio].count()) {
            if (!rdo) report.fatal_usage(rdo_needs_to_be_set_err_msg);
            rdoMaxAllowedRMSIncreaseRatio =
                captureBCnOption<float>(args, kRDOMaxAllowedRMSIncreaseRatio);
        }

        if (args[kRDOTryOneMatch].count()) {
            captureBCnOption(kRDOTryOneMatch);
            rdoTry2Matches = false;
        }

        if (args[kRDOSkipZeroMSEBlocks].count()) {
            captureBCnOption(kRDOSkipZeroMSEBlocks);
            rdoSkipZeroMSEBlocks = true;
        }

        if (args[kRDOAllowRelativeMovement].count()) {
            captureBCnOption(kRDOAllowRelativeMovement);
            rdoAllowRelativeMovement = true;
        }
    }
};

}  // namespace ktx

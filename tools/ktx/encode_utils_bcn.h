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

// TODO: update this once BCn params are agreed upon.
/**
//! [command options_encode_bcn]
<dl>
    <dt>
        BCn:
    </dt>
    <dd></dd>

    <dl>
        <dt>\--bc1-quality &lt;level&gt;</dt>
        <dd>The quality level configures the quality-performance
           tradeoff for the BC1/BC3 encoder. Default is 'medium'. The quality
           level can be set between fastest (0) and exhaustive (19) via the
           following fixed quality presets:
           <table>
               <tr><th>Level      </th> <th> Quality                      </th></tr>
               <tr><td>fastest    </td> <td>(equivalent to quality =   0) </td></tr>
               <tr><td>fast       </td> <td>(equivalent to quality =  10) </td></tr>
               <tr><td>medium     </td> <td>(equivalent to quality =  60) </td></tr>
               <tr><td>thorough   </td> <td>(equivalent to quality =  98) </td></tr>
               <tr><td>exhaustive </td> <td>(equivalent to quality = 100) </td></tr>
          </table>
        </dd>
        <dt>\--bc1-mode &lt;level&gt;</dt>
        <dd>BC1/BC3 approximation mode (for both: encoding and decoding).
           Default is 'ideal'. If you encode textures for a specific vendor's
           GPU, beware that using that texture data on other vendor's GPU may
           result in ugly artifacts. Keep the default unless you know the
           texture data will only be deployed or used on a specific vendor's
           GPU. Available modes:
           <table>
               <tr><th>Mode       </th></tr> <th> Description </th></tr> <tr><td>ideal </td></tr>
<td> No rounding for 4-color colors 2,3. This matches the D3D10 docs on BC1. </td></tr>
               <tr><td>nvidia     </td></tr> <td> NVidia GPU mode. May produce artifacts on
non-NVidia GPUs.                                                                </td></tr>
               <tr><td>amd        </td></tr> <td> AMD GPU mode. May produce artifacts on non-AMD
GPUs.                                                                      </td></tr> <tr><td>ideal4
</td></tr> <td> Matches AMD Compressonator's output. Rounds 4-color colors 2,3 (not 3-color color
2). This matches the D3D9 docs on DXT1. </td></tr>
          </table>
        </dd>
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
    inline static const char* kRDOAutoMSEScale = "rdo-auto-mse-scale";
    inline static const char* kRDOMaxSmoothBlockMSEScale = "rdo-max-smooth-block-mse-scale";
    inline static const char* kRDOMaxSmoothBlockStdDev = "rdo-max-smooth-block-std-dev";
    inline static const char* kBCnOptions[] = {kBC1Mode,
                                               kBC1Quality,
                                               kBC7Quality,
                                               kRDO,
                                               kRDOLambda,
                                               kRDOWindowLoopbackSize,
                                               kRDOAutoMSEScale,
                                               kRDOMaxSmoothBlockMSEScale,
                                               kRDOMaxSmoothBlockStdDev};

    std::string bcnOptions{};
    // This is added here so that when OptionsEncodeBCn is combined with other
    // options (e.g., from ASTC) we access this property to know which encoder
    // should be used.
    bool encodeBCn = false;
    ClampedOption<ktx_uint32_t> bc1CompressionQuality{ktxBCnParams::bc1CompressionQuality, 0,
                                                      KTX_PACK_BC1_QUALITY_LEVEL_MAX};

    OptionsEncodeBCn() : ktxBCnParams() {
        threadCount = std::max<ktx_uint32_t>(1u, std::thread::hardware_concurrency());
        if (threadCount == 0) threadCount = 1;
        structSize = sizeof(ktxBCnParams);
        bc1ApproxMode = KTX_PACK_BC1_BLOCK_APPROX_MODE_IDEAL;
        bc1CompressionQuality = KTX_PACK_BC1_QUALITY_LEVEL_MEDIUM;
    }

    void init(cxxopts::Options& opts) {
        opts.add_options("Encode BCn")(kBC1Mode, "TODO..", cxxopts::value<std::string>(),
                                       "<level>")(
            kBC1Quality,
            "The quality level configures the quality-performance tradeoff"
            " for BC1/BC3 encoder. Default is 10. The quality level can be"
            " set between fastest (0) and exhaustive (19) via the following",
            cxxopts::value<std::string>(),
            "<level>")(kBC7Quality,
                       "The quality level configures the quality-performance tradeoff"
                       " for BC7 encoder. Default is 'medium'. The quality level can be"
                       " set between fastest and exhaustive via the following fixed"
                       " quality presets where each preset is an OR'ed set of flags:\n\n"
                       "    Level      |  Quality\n"
                       "    ---------- | ----------------------------\n"
                       "    fastest    | (equivalent to flags =  128)\n"
                       "    faster     | (equivalent to flags =  176)\n"
                       "    fast       | (equivalent to flags =  179)\n"
                       "    medium     | (equivalent to flags =  255)\n"
                       "    thorough   | (equivalent to flags = 1023)\n"
                       "    exhaustive | (equivalent to flags = 3967)",
                       cxxopts::value<std::string>(), "<level>")(
            kRDO,
            "Enable rate distortion optimization (RDO) post processing step on"
            " BCn-encoded blocks to reduce entropy with Deflate/LZMA/LZHAM"
            " optimizations. This is primarily used to reduce size on disk by"
            " applying a further compression, mainly: Deflate, LZMA, or LZHAM."
            " RDO options only take effect if this is set to true.",
            cxxopts::value<bool>(),
            "<level>")(kRDOLambda,
                       "RDO quality scalar (lambda). Lower values yield higher"
                       " quality/larger LZ compressed files, higher values yield lower"
                       " quality/smaller LZ compressed files. A good range to try is [0.20,4.00]."
                       " Full range is [0.10,50.0]. Default is 1.0.",
                       cxxopts::value<float>(), "<level>")(
            kRDOWindowLoopbackSize,
            "The number of bytes the encoder can look back from each block to"
            " find matches. The larger this value, the slower the encoder but the"
            " higher the quality per LZ compressed bit. You don't need a huge"
            " window to get large gains. Even 64-512 byte windows are fine."
            " Default is 128.",
            cxxopts::value<uint32_t>(),
            "<level>")(kRDOAutoMSEScale,
                       "Whether to try to compute a max smooth block factor based off the"
                       " supplied lambda setting. There is no single calculation/set of"
                       " settings that work perfectly on all input textures, but the formula"
                       " in the code works OK for most textures at low-ish lambdas (For an"
                       " example of a difficult texture the currently formulas/settings"
                       " doesn't handle so well, try encoding kodim03 at"
                       " lambdas 1-3.). Smooth block handling is tuned so lambdas at or near"
                       " 1 looks OK on textures with smooth gradients, skies, etc. If this is"
                       " set, rdo-max-mse-scale option is ignored. Default is true.",
                       cxxopts::value<bool>(), "<level>")(
            kRDOMaxSmoothBlockMSEScale,
            "RDO max MSE scaling factor for blocks considered to be smooth/flat."
            " A value of 1.0 means no smooth block error scaling which may cause"
            " very noticeable artifcats for smooth/flat blocks (e.g., kodim23 test"
            " image). This value can be automatically computed based on the set"
            " RDO lamba by setting rdo-auto-mse-scale. TODO"
            " is used to compute, for a given block, the MSE scale factor in"
            " the range: 1.0 (i.e., not a smooth block) up to this max MSE scale"
            " factor. As to why an MSE factor has to be applied to smooth/flat blocks, the"
            " MSE for these blocks is too low relative to the visual impact they"
            " have when they get distorted. The solution implemented here is to "
            " compute the max std dev. of any component and use a linear function"
            " of that to scale block/trial MSE."
            " Range is [1,300]. Default is 18.0.",
            cxxopts::value<float>(),
            "<level>")(kRDOMaxSmoothBlockStdDev,
                       "RDO max smooth/flat block standard deviation. If the std dev."
                       " of a block exceeds this value, then it won't be considered"
                       " as a smooth block (i.e., the smooth block MSE scale factor will be"
                       " set to 1 for this block). The smaller the ratio of the std dev."
                       " of this block to this value the more the smooth block MSE"
                       " scale factor approaches rdoMaxSmoothBlockMseScale."
                       " Range is [.01,65536.0]. Larger values expand the range of blocks"
                       " considered smooth. Default is 10.0.",
                       cxxopts::value<float>(), "<level>");
    }

    void captureBCnOption(const char* name) { bcnOptions += fmt::format(" --{}", name); }

    template <typename T>
    T captureBCnOption(cxxopts::ParseResult& args, const char* name) {
        const T value = args[name].as<T>();
        bcnOptions += fmt::format(" --{} {}", name, value);
        return value;
    }

    void process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter& report) {
        // bc1/bc3 approximation mode
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
        } else {
            bc1ApproxMode = KTX_PACK_BC1_BLOCK_APPROX_MODE_IDEAL;
        }

        // bc1/bc3 compression quality
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
            if (it == bc1_quality_mapping.end())
                report.fatal_usage("Invalid bc1-quality: \"{}\"", qualityLevelStr);
            bc1CompressionQuality = it->second;
        } else {
            bc1CompressionQuality = KTX_PACK_BC1_QUALITY_LEVEL_MEDIUM;
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
            if (it == bc7_quality_mapping.end())
                report.fatal_usage("Invalid bc7-quality: \"{}\"", qualityLevelStr);
            bc7CompressionQuality = it->second;
        } else {
            bc7CompressionQuality = KTX_PACK_BC7_QUALITY_LEVEL_MEDIUM;
        }

        /* RDO params */

        rdo = args[kRDO].count() ? captureBCnOption<bool>(args, kRDO) : false;
        rdoAutoSmoothBlockMaxMSEScale =
            args[kRDOAutoMSEScale].count() ? captureBCnOption<bool>(args, kRDOAutoMSEScale) : true;

        // TODO: Use ClampedOptions instead of this manual range check...
        if (args[kRDOLambda].count()) {
            auto val = captureBCnOption<float>(args, kRDOLambda);
            if (val < 0.1f || val > 50.0f)
                report.fatal_usage("Invalid rdo-lambda: \"{}\". Should be in range: [0.1, 50.0]",
                                   val);
            rdoQualityScalar = val;
        } else {
            rdoQualityScalar = 1.0f;
        }

        if (args[kRDOWindowLoopbackSize].count()) {
            auto val = captureBCnOption<uint32_t>(args, kRDOWindowLoopbackSize);
            if (val < 64u || val > 65536u)
                report.fatal_usage(
                    "Invalid rdo-window-loopback-size: \"{}\". Should be in range: [64, 65536]",
                    val);
            rdoWindowLoopbackSize = val;
        } else {
            rdoWindowLoopbackSize = 128;
        }

        if (args[kRDOMaxSmoothBlockMSEScale].count()) {
            auto val = captureBCnOption<float>(args, kRDOMaxSmoothBlockMSEScale);
            if (val < 1.0f || val > 300.0f)
                report.fatal_usage(
                    "Invalid rdo-max-smooth-block-mse-scale: \"{}\". Should be in range: [1.0, "
                    "300.0]",
                    val);
            rdoMaxSmoothBlockMseScale = val;
        } else {
            rdoMaxSmoothBlockMseScale = 18.0f;
        }

        if (args[kRDOMaxSmoothBlockStdDev].count()) {
            auto val = captureBCnOption<float>(args, kRDOMaxSmoothBlockStdDev);
            if (val < 0.01f || val > 65536.0f)
                report.fatal_usage(
                    "Invalid rdo-max-smooth-block-std-dev: \"{}\". Should be in range: [0.01, "
                    "65536.0]",
                    val);
            rdoMaxSmoothBlockStdDev = val;
        } else {
            rdoMaxSmoothBlockStdDev = 10.0f;
        }
    }
};

}  // namespace ktx

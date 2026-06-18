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
        <dt>\--bc1-mode &lt;mode&gt;</dt>
        <dd>BC1 (subsequently BC3) approximation mode (for both: encoding and
            decoding). Default is 'ideal'. If you encode textures for a specific
            vendor's GPU, beware that using that texture data on other GPUs may
            result in ugly artifacts. Set to 'ideal' unless you know the texture
            data will only be deployed or used on a specific vendor's GPU. Can
            be set to one of the following:
            <table>
                <tr><th>Mode       </th> <th> Description                                                      </th></tr>
                <tr><td>ideal      </td> <td> The default mode. No rounding for 4-color colors 2,3. This matches the
                                              D3D10 docs on BC1.                                               </td></tr>
                <tr><td>nvidia     </td> <td> NVidia GPU mode. May produce artifacts on non-NVidia GPUs.       </td></tr>
                <tr><td>amd        </td> <td> AMD GPU mode. May produce artifacts on non-AMD GPUs.             </td></tr>
                <tr><td>ideal      </td> <td> Matches AMD Compressonator's output. Rounds 4-color colors 2,3
                                              (not 3-color color 2). This matches the D3D9 docs on DXT1.       </td></tr>
            </table>
        </dd>
        <dt>\--bc1-quality &lt;level&gt;</dt>
        <dd>The quality level configures the quality-performance tradeoff for
            BC1/BC3 encoders. The quality level can be set in the range [0, 19]
            with (0) being the 'fastest' and (19) the slowest but most
            'exhaustive'. Default is (15) 'thorough'. Can also be set via the
            following aliases:
            <table>
                <tr><th>Level      </th> <th> Quality                      </th></tr>
                <tr><td>fastest    </td> <td>(equivalent to quality =   0) </td></tr>
                <tr><td>fast       </td> <td>(equivalent to quality =   5) </td></tr>
                <tr><td>medium     </td> <td>(equivalent to quality =  10) </td></tr>
                <tr><td>thorough   </td> <td>(equivalent to quality =  15) </td></tr>
                <tr><td>exhaustive </td> <td>(equivalent to quality =  19) </td></tr>
            </table>
            Note on BC1 vs. BC3 vs. BC7: apart from lower VRAM consumption (4bpp
            vs. 8bpp) and better GPU texture cache efficiency, there's little
            need to use BC1 now. BC3 still has an advantage vs. BC7, because it
            very strongly separates how RGB is encoded from the alpha channel,
            in a predictable way.
        </dd>
        <dt>\--bc7-quality &lt;level&gt;</dt>
        <dd>The quality level configures the quality-performance tradeoff for
            BC7 encoder. Default is 'medium'. The quality level can be set
            between fastest and exhaustive via the following fixed quality
            presets where each preset is an OR'ed set of flags:
            <table>
                <tr><th>Level      </th> <th> OR'ed flags                    </th></tr>
                <tr><td>fastest    </td> <td>(equivalent to quality =   128) </td></tr>
                <tr><td>faster     </td> <td>(equivalent to quality =   176) </td></tr>
                <tr><td>fast       </td> <td>(equivalent to quality =   179) </td></tr>
                <tr><td>medium     </td> <td>(equivalent to quality =   255) </td></tr>
                <tr><td>thorough   </td> <td>(equivalent to quality =  1023) </td></tr>
                <tr><td>exhaustive </td> <td>(equivalent to quality =  3967) </td></tr>
            </table>
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
        <dd>Inject up to 1 match into each block instead of up-to-two matches.
            Results in slightly faster, but lower compression.</dd>
        <dt>\--bcn-rdo-skip-zero-mse</dt>
        <dd>Skip blocks that have zero mean-squared error (MSE). Might result in
            faster but potentially lower compression.</dd>
    </dl>
</dl>
//! [command options_encode_bcn]
*/

struct OptionsEncodeBCn : public ktxBCnParams {
    /* high-level params */
    // inline static const char* kBCnQuality = "quality";
    // inline static const char* kBCnEffort = "effort";

    /* low-level params */
    inline static const char* kBC1Mode = "bc1-mode";
    inline static const char* kBC1Quality = "bc1-quality";
    inline static const char* kBC7Quality = "bc7-quality";
    inline static const char* kBCnRdo = "bcn-rdo";
    inline static const char* kBCnRdoL = "bcn-rdo-l";
    inline static const char* kBCnRdoD = "bcn-rdo-d";
    inline static const char* kBCnRdoB = "bcn-rdo-b";
    inline static const char* kBCnRdoS = "bcn-rdo-s";
    inline static const char* kBCnRdoR = "bcn-rdo-r";
    inline static const char* kBCnRdoNoUltrasmoothBlocks = "bcn-rdo-no-ultrasmooth";
    inline static const char* kBCnRdoTryOneMatch = "bcn-rdo-try-one-match";
    inline static const char* kBCnRdoSkipZeroMSEBlocks = "bcn-rdo-skip-zero-mse";

    inline static const char* kBCnOptions[] = {
        // kBCnQuality,
        // kBCnEffort,
        kBC1Mode, kBC1Quality,        kBC7Quality,
        kBCnRdo,  kBCnRdoL,           kBCnRdoD,
        kBCnRdoB, kBCnRdoS,           kBCnRdoNoUltrasmoothBlocks,
        kBCnRdoR, kBCnRdoTryOneMatch, kBCnRdoSkipZeroMSEBlocks,
    };

    ClampedOption<ktx_uint32_t> bc1CompressionQuality;
    ClampedOption<ktx_uint32_t> bc7CompressionQuality;
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
        : bc1CompressionQuality(ktxBCnParams::bc1CompressionQuality, 0u,
                                KTX_PACK_BC1_QUALITY_LEVEL_MAX),
          bc7CompressionQuality(ktxBCnParams::bc7CompressionQuality, 0u,
                                KTX_PACK_BC7_QUALITY_LEVEL_MAX),
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
        normalMap = false;
        bc1ApproxMode = KTX_PACK_BC1_BLOCK_APPROX_MODE_IDEAL;
        bc1CompressionQuality = KTX_PACK_BC1_QUALITY_LEVEL_THOROUGH;
        bc7CompressionQuality = KTX_PACK_BC7_QUALITY_LEVEL_THOROUGH;
        bcnRDO = false;
        bcnRDOQualityScalar = 1.0f;
        bcnRDOAutoMaxSmoothBlockErrorScale = true;
        bcnRDOMaxSmoothBlockErrorScale = 10.0f;
        bcnRDOMaxSmoothBlockStdDev = 18.0f;
        bcnRDOUltrasmoothBlockHandling = true;
        bcnRDOMaxAllowedRMSIncreaseRatio = 10.0f;
        bcnRDODictSize = 4096u;
        bcnRDOTry2Matches = true;
        bcnRDOSkipZeroMSEBlocks = false;
    }

    void init(cxxopts::Options& opts) {
        opts.add_options("Encode BCn")(
            kBC1Mode,
            "BC1 (subsequently BC3) approximation mode (for both: encoding and decoding). Default "
            "is 'ideal'. If "
            "you encode textures for a specific vendor's GPU, beware that using that texture "
            "data on other GPUs may result in ugly artifacts. Set to 'ideal' unless you know the "
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
            "               | matches the D3D9 docs on DXT1.                 ",
            cxxopts::value<std::string>(), "<mode>")(
            kBC1Quality,
            "The quality level configures the quality-performance tradeoff for BC1/BC3 encoders. "
            "The quality level can be set in the range [0, 19] with (0) being the 'fastest' and "
            "(19) the slowest but most 'exhaustive'. Default is (15) 'thorough'. "
            "Can also be set via the following aliases:\n\n"
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
            "separates how RGB is encoded from the alpha channel, in a predictable way.",
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
                       "    exhaustive | (equivalent to flags = 3967)",
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
            "Inject up to 1 match into each block instead of up-to-two matches. Results "
            "in slightly faster, but lower compression.")(
            kBCnRdoSkipZeroMSEBlocks,
            "Skip blocks that have zero mean-squared error (MSE). Might result in faster but "
            "potentially lower compression.");
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

        /* high-level params (if specified, low-level params are ignored) */
        // if (args[kBCnQuality].count() && !args[kBCnEffort].count()) {
        //     report.fatal_usage(
        //         "High-level --effort option has to be provided when --quality is used.");
        // }

        // if (!args[kBCnQuality].count() && args[kBCnEffort].count()) {
        //     report.fatal_usage(
        //         "High-level --quality option has to be provided when --effort is used.");
        // }

        // if (args[kBCnQuality].count() && args[kBCnEffort].count()) {
        //     captureBCnOption<int>(args, kBCnEffort);
        //     captureBCnOption<int>(args, kBCnQuality);
        // }

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
                    bc1CompressionQuality = static_cast<ktx_uint32_t>(std::stoul(qualityLevelStr));
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
                    bc7CompressionQuality = static_cast<ktx_uint32_t>(std::stoul(qualityLevelStr));
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
            bcnRDOAutoMaxSmoothBlockErrorScale = false;
        }

        if (args[kBCnRdoS].count()) {
            if (!bcnRDO) report.fatal_usage(rdo_needs_to_be_set_err_msg);
            bcnRDOMaxSmoothBlockStdDev = captureBCnOption<float>(args, kBCnRdoS);
        }

        if (args[kBCnRdoNoUltrasmoothBlocks].count()) {
            if (!bcnRDO) report.fatal_usage(rdo_needs_to_be_set_err_msg);
            captureBCnOption(kBCnRdoNoUltrasmoothBlocks);
            bcnRDOUltrasmoothBlockHandling = false;
        }

        if (args[kBCnRdoR].count()) {
            if (!bcnRDO) report.fatal_usage(rdo_needs_to_be_set_err_msg);
            bcnRDOMaxAllowedRMSIncreaseRatio = captureBCnOption<float>(args, kBCnRdoR);
        }

        if (args[kBCnRdoTryOneMatch].count()) {
            if (!bcnRDO) report.fatal_usage(rdo_needs_to_be_set_err_msg);
            captureBCnOption(kBCnRdoTryOneMatch);
            bcnRDOTry2Matches = false;
        }

        if (args[kBCnRdoSkipZeroMSEBlocks].count()) {
            if (!bcnRDO) report.fatal_usage(rdo_needs_to_be_set_err_msg);
            captureBCnOption(kBCnRdoSkipZeroMSEBlocks);
            bcnRDOSkipZeroMSEBlocks = true;
        }
    }
};

}  // namespace ktx

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
    inline static const char* kBCnOptions[] = {kBC1Mode, kBC1Quality};

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
            " for BC1/BC3 encoder. Default is 'medium'. The quality level can be"
            " set between fastest (0) and exhaustive (19) via the following"
            " fixed quality presets:\n\n"
            "    Level      |  Quality\n"
            "    ---------- | -----------------------------\n"
            "    fastest    | (equivalent to quality =   0)\n"
            "    fast       | (equivalent to quality =   5)\n"
            "    medium     | (equivalent to quality =  10)\n"
            "    thorough   | (equivalent to quality =  15)\n"
            "    exhaustive | (equivalent to quality =  19)",
            cxxopts::value<std::string>(), "<level>");
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
    }
};

}  // namespace ktx

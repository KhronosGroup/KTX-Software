// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "command.h"
#include "utility.h"

#include <thread>

// -------------------------------------------------------------------------------------------------

namespace ktx {

/**
//! [command options_encode_astc]
<dl>
    <dt>
        ASTC:
    </dt>
    <dd></dd>

    <dl>
        <dt>\--astc-quality &lt;level&gt;</dt>
        <dd>The quality level configures the quality-performance
           tradeoff for the compressor; more complete searches of the
           search space improve image quality at the expense of
           compression time. Default is 'medium'. The quality level can be
           set between fastest (0) and exhaustive (100) via the
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
        <dt>\--astc-perceptual</dt>
        <dd>The codec should optimize for perceptual error, instead of
            direct RMS error. This aims to improve perceived image quality,
            but typically lowers the measured PSNR score. Perceptual
            methods are currently only available for normal maps and RGB
            color data.</dd>
    </dl>
</dl>
//! [command options_encode_astc]
*/

struct OptionsEncodeASTC : public ktxAstcParams {
    inline static const char* kAstcQuality = "astc-quality";
    inline static const char* kAstcPerceptual = "astc-perceptual";

    inline static const char* kAstcOptions[] = {
        kAstcQuality,
        kAstcPerceptual
    };

    std::string astcOptions{};
    bool encodeASTC = false;
    ClampedOption<ktx_uint32_t> qualityLevel{ktxAstcParams::qualityLevel, 0, KTX_PACK_ASTC_QUALITY_LEVEL_MAX};

    OptionsEncodeASTC() : ktxAstcParams() {
        threadCount = std::max<ktx_uint32_t>(1u, std::thread::hardware_concurrency());
        if (threadCount == 0)
            threadCount = 1;
        structSize = sizeof(ktxAstcParams);
        normalMap = false;
        for (int i = 0; i < 4; i++)
            inputSwizzle[i] = 0;
        qualityLevel.clear();
    }

    void init(cxxopts::Options& opts) {
        opts.add_options("Encode ASTC")
                (kAstcQuality,
                        "The quality level configures the quality-performance tradeoff for "
                        "the compressor; more complete searches of the search space "
                        "improve image quality at the expense of compression time. Default "
                        "is 'medium'. The quality level can be set between fastest (0) and "
                        "exhaustive (100) via the following fixed quality presets:\n\n"
                        "    Level      |  Quality\n"
                        "    ---------- | -----------------------------\n"
                        "    fastest    | (equivalent to quality =   0)\n"
                        "    fast       | (equivalent to quality =  10)\n"
                        "    medium     | (equivalent to quality =  60)\n"
                        "    thorough   | (equivalent to quality =  98)\n"
                        "    exhaustive | (equivalent to quality = 100)",
                        cxxopts::value<std::string>(), "<level>")
                (kAstcPerceptual,
                        "The codec should optimize for perceptual error, instead of direct "
                        "RMS error. This aims to improve perceived image quality, but "
                        "typically lowers the measured PSNR score. Perceptual methods are "
                        "currently only available for normal maps and RGB color data.");
    }

    void captureASTCOption(const char* name) {
        astcOptions += fmt::format(" --{}", name);
    }

    template <typename T>
    T captureASTCOption(cxxopts::ParseResult& args, const char* name) {
        const T value = args[name].as<T>();
        astcOptions += fmt::format(" --{} {}", name, value);
        return value;
    }

    void process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter& report) {
        if (args[kAstcQuality].count()) {
            static std::unordered_map<std::string, ktx_pack_astc_quality_levels_e> astc_quality_mapping{
                    {"fastest", KTX_PACK_ASTC_QUALITY_LEVEL_FASTEST},
                    {"fast", KTX_PACK_ASTC_QUALITY_LEVEL_FAST},
                    {"medium", KTX_PACK_ASTC_QUALITY_LEVEL_MEDIUM},
                    {"thorough", KTX_PACK_ASTC_QUALITY_LEVEL_THOROUGH},
                    {"exhaustive", KTX_PACK_ASTC_QUALITY_LEVEL_EXHAUSTIVE}
            };
            const auto qualityLevelStr = to_lower_copy(captureASTCOption<std::string>(args, kAstcQuality));
            const auto it = astc_quality_mapping.find(qualityLevelStr);
            if (it == astc_quality_mapping.end())
                report.fatal_usage("Invalid astc-quality: \"{}\"", qualityLevelStr);
            qualityLevel = it->second;
        } else {
            qualityLevel = KTX_PACK_ASTC_QUALITY_LEVEL_MEDIUM;
        }

        if (args[kAstcPerceptual].count()) {
            captureASTCOption(kAstcPerceptual);
            perceptual = KTX_TRUE;
        }
    }
};

} // namespace ktx

// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "command.h"
#include "utility.h"

#include <thread>
#include <unordered_map>

// -------------------------------------------------------------------------------------------------

namespace ktx {

enum class EncodeCodec {
    NONE = 0,
    BasisLZ,
    UASTC,
    INVALID = 0x7FFFFFFF
};

template <bool ENCODE_CMD>
struct OptionsCodec {
    struct BasisOptions : public ktxBasisParams {
        // The remaining numeric fields are clamped within the Basis
        // library.
        ClampedOption<ktx_uint32_t> threadCount;
        ClampedOption<ktx_uint32_t> qualityLevel;
        ClampedOption<ktx_uint32_t> maxEndpoints;
        ClampedOption<ktx_uint32_t> maxSelectors;
        ClampedOption<ktx_uint32_t> uastcRDODictSize;
        ClampedOption<float> uastcRDOQualityScalar;
        ClampedOption<float> uastcRDOMaxSmoothBlockErrorScale;
        ClampedOption<float> uastcRDOMaxSmoothBlockStdDev;

        BasisOptions() :
            threadCount(ktxBasisParams::threadCount, 1, 10000),
            qualityLevel(ktxBasisParams::qualityLevel, 1, 255),
            maxEndpoints(ktxBasisParams::maxEndpoints, 1, 16128),
            maxSelectors(ktxBasisParams::maxSelectors, 1, 16128),
            uastcRDODictSize(ktxBasisParams::uastcRDODictSize, 256, 65536),
            uastcRDOQualityScalar(ktxBasisParams::uastcRDOQualityScalar,
                                    0.001f, 50.0f),
            uastcRDOMaxSmoothBlockErrorScale(
                            ktxBasisParams::uastcRDOMaxSmoothBlockErrorScale,
                            1.0f, 300.0f),
            uastcRDOMaxSmoothBlockStdDev(
                            ktxBasisParams::uastcRDOMaxSmoothBlockStdDev,
                            0.01f, 65536.0f)
        {
            uint32_t tc = std::thread::hardware_concurrency();
            if (tc == 0) tc = 1;
            threadCount.max = tc;
            threadCount = tc;

            structSize = sizeof(ktxBasisParams);
            // - 1 is to match what basisu_tool does (since 1.13).
            compressionLevel = KTX_ETC1S_DEFAULT_COMPRESSION_LEVEL - 1;
            qualityLevel.clear();
            maxEndpoints.clear();
            endpointRDOThreshold = 0.0f;
            maxSelectors.clear();
            selectorRDOThreshold = 0.0f;
            normalMap = false;
            separateRGToRGB_A = false;
            preSwizzle = false;
            noEndpointRDO = false;
            noSelectorRDO = false;
            uastc = false; // Default to ETC1S.
            uastcRDO = false;
            uastcFlags = KTX_PACK_UASTC_LEVEL_DEFAULT;
            uastcRDODictSize.clear();
            uastcRDOQualityScalar.clear();
            uastcRDODontFavorSimplerModes = false;
            uastcRDONoMultithreading = false;
            noSSE = false;
            verbose = false; // Default to quiet operation.
            for (int i = 0; i < 4; i++) inputSwizzle[i] = 0;
        }
#define TRAVIS_DEBUG 0
#if TRAVIS_DEBUG
        void print() {
            cout << "threadCount = " << threadCount.value << endl;
            cout << "qualityLevel = " << qualityLevel.value << endl;
            cout << "maxEndpoints = " << maxEndpoints.value << endl;
            cout << "maxSelectors = " << maxSelectors.value << endl;
            cout << "structSize = " << structSize << endl;
            cout << "threadCount = " << ktxBasisParams::threadCount << endl;
            cout << "compressionLevel = " << compressionLevel << endl;
            cout << "qualityLevel = " << ktxBasisParams::qualityLevel << endl;
            cout << "compressionLevel = " << compressionLevel << endl;
            cout << "maxEndpoints = " << ktxBasisParams::maxEndpoints << endl;
            cout << "endpointRDOThreshold = " << endpointRDOThreshold << endl;
            cout << "maxSelectors = " << ktxBasisParams::maxSelectors << endl;
            cout << "selectorRDOThreshold = " << selectorRDOThreshold << endl;
            cout << "normalMap = " << normalMap << endl;
            cout << "separateRGToRGB_A = " << separateRGToRGB_A << endl;
            cout << "preSwizzle = " << preSwizzle << endl;
            cout << "noEndpointRDO = " << noEndpointRDO << endl;
            cout << "noSelectorRDO = " << noSelectorRDO << endl;
            cout << "uastc = " << uastc << endl;
            cout << "uastcFlags = " << uastcFlags << endl;
            cout << "uastcRDO = " << uastcRDO << endl;
            cout << "uastcRDODictSize = " << uastcRDODictSize << endl;
            cout << "uastcRDOQualityScalar = " << uastcRDOQualityScalar << endl;
        }
#endif
    };

    std::string codecName;
    EncodeCodec codec;
    BasisOptions basisOpts;

    void init(cxxopts::Options& opts) {
        opts.add_options("Encode BasisLZ")
            ("clevel", "BasisLZ compression level, an encoding speed vs. quality level tradeoff. "
                "Range is [0,5], default is 1. Higher values are slower but give higher quality.",
                cxxopts::value<uint32_t>(), "<level>")
            ("qlevel", "BasisLZ quality level. Range is [1,255]. Lower gives better compression/lower "
                "quality/faster. Higher gives less compression/higher quality/slower. --qlevel "
                "automatically determines values for --max_endpoints, --max-selectors, "
                "--endpoint_rdo_threshold and --selector_rdo_threshold for the target quality level. "
                "Setting these options overrides the values determined by --qlevel which defaults to "
                "128 if neither it nor both of --max_endpoints and --max_selectors have been set.",
                cxxopts::value<uint32_t>(), "<level>")
            ("max-endpoints", "Manually set the maximum number of color endpoint clusters. Range "
                "is [1,16128]. Default is 0, unset.",
                cxxopts::value<uint32_t>(), "<arg>")
            ("endpoint-rdo-threshold", "Set endpoint RDO quality threshold. The default is 1.25. Lower "
                "is higher quality but less quality per output bit (try [1.0,3.0]). This will override "
                "the value chosen by --qlevel.", cxxopts::value<float>(), "<arg>")
            ("max-selectors", "Manually set the maximum number of color selector clusters from [1,16128]. "
                "Default is 0, unset.", cxxopts::value<uint32_t>(), "<arg>")
            ("selector-rdo-threshold", "Set selector RDO quality threshold. The default is 1.25. Lower "
                "is higher quality but less quality per output bit (try [1.0,3.0]). This will override "
                "the value chosen by --qlevel.", cxxopts::value<float>(), "<arg>")
            ("no-endpoint-rdo", "Disable endpoint rate distortion optimizations. Slightly faster, "
                "less noisy output, but lower quality per output bit. Default is to do endpoint RDO.")
            ("no-selector-rdo", "Disable selector rate distortion optimizations. Slightly faster, "
                "less noisy output, but lower quality per output bit. Default is to do selector RDO.");
        opts.add_options("Encode UASTC")
            ("uastc-quality", "UASTC compression level, an encoding speed vs. quality level tradeoff. "
                "Range is [0,4], default is 1. Higher values are slower but give higher quality.",
                cxxopts::value<uint32_t>(), "<level>")
            ("uastc-rdo", "Enable UASTC RDO post-processing.")
            ("uastc-rdo-l", "Set UASTC RDO quality scalar to the specified value. Lower values yield "
                "higher quality/larger supercompressed files, higher values yield lower quality/smaller "
                "supercompressed files. A good range to try is [.25,10]. For normal maps a good range is "
                "[.25,.75]. The full range is [.001,10.0]. Default is 1.0.",
                cxxopts::value<float>(), "<lambda>")
            ("uastc-rdo-d", "Set UASTC RDO dictionary size in bytes. Default is 4096. Lower values=faster, "
                "but give less compression. Range is [64,65536].",
                cxxopts::value<uint32_t>(), "<dictsize>")
            ("uastc-rdo-b", "Set UASTC RDO max smooth block error scale. Range is [1.0,300.0]. Default "
                "is 10.0, 1.0 is disabled. Larger values suppress more artifacts (and allocate more bits) "
                "on smooth blocks.", cxxopts::value<float>(), "<scale>")
            ("uastc-rdo-s", "Set UASTC RDO max smooth block standard deviation. Range is [.01,65536.0]. "
                "Default is 18.0. Larger values expand the range of blocks considered smooth.",
                cxxopts::value<float>(), "<deviation>")
            ("uastc-rdo-f", "Do not favor simpler UASTC modes in RDO mode.")
            ("uastc-rdo-m", "Disable RDO multithreading (slightly higher compression, deterministic).");
        opts.add_options("Encode common")
            ("normal-mode", "Optimizes for encoding textures with normal data. If the input texture has "
                "three or four linear components it is assumed to be a three component linear normal "
                "map storing unit length normals as (R=X, G=Y, B=Z). A fourth component will be ignored. "
                "The map will be converted to a two component X+Y normal map stored as (RGB=X, A=Y) "
                "prior to encoding. If unsure that your normals are unit length, use --normalize. "
                "If the input has 2 linear components it is assumed to be an X+Y map of unit normals.\n"
                "The Z component can be recovered programmatically in shader code by using the equations:\n"
                "    nml.xy = texture(...).ga;              // Load in [0,1]\n"
                "    nml.xy = nml.xy * 2.0 - 1.0;           // Unpack to [-1,1]\n"
                "    nml.z = sqrt(1 - dot(nml.xy, nml.xy)); // Compute Z\n"
                "ETC1S encoding, RDO is disabled (no selector RDO, no endpoint RDO) to provide better quality.")
            ("threads", "Sets the number of threads to use during encoding. By default, encoding "
                "will use the number of threads reported by thread::hardware_concurrency or 1 if "
                "value returned is 0.", cxxopts::value<uint32_t>(), "<count>")
            ("no-sse", "Forbit use of the SSE instruction set. Ignored if CPU does not support SSE.");
    }

    EncodeCodec validateEncodeCodec(const cxxopts::OptionValue& codecOpt) const {
        static const std::unordered_map<std::string, EncodeCodec> codecs = {
            { "basis-lz", EncodeCodec::BasisLZ },
            { "uastc", EncodeCodec::UASTC }
        };
        if (codecOpt.count()) {
            auto it = codecs.find(to_lower_copy(codecOpt.as<std::string>()));
            if (it != codecs.end()) {
                return it->second;
            } else {
                return EncodeCodec::INVALID;
            }
        } else {
            return EncodeCodec::NONE;
        }
    }

    void validateCommonEncodeArg(Reporter& report, const char* name) {
        if (codec == EncodeCodec::NONE)
            report.fatal(rc::INVALID_ARGUMENTS,
                "Invalid use of argument --{} that only applies to encoding.", name);
    }

    void validateBasisLZArg(Reporter& report, const char* name) {
        if (codec != EncodeCodec::BasisLZ)
            report.fatal(rc::INVALID_ARGUMENTS,
                "Invalid use of argument --{} that only applies when the used codec is BasisLZ.", name);
    }

    void validateBasisLZEndpointRDOArg(Reporter& report, const char* name) {
        validateBasisLZArg(report, name);
        if (basisOpts.noEndpointRDO)
            report.fatal(rc::INVALID_ARGUMENTS,
                "Invalid use of argument --{} when endpoint RDO is disabled.", name);
    }

    void validateBasisLZSelectorRDOArg(Reporter& report, const char* name) {
        validateBasisLZArg(report, name);
        if (basisOpts.noSelectorRDO)
            report.fatal(rc::INVALID_ARGUMENTS,
                "Invalid use of argument --{} when selector RDO is disabled.", name);
    }

    void validateUASTCArg(Reporter& report, const char* name) {
        if (codec != EncodeCodec::UASTC)
            report.fatal(rc::INVALID_ARGUMENTS,
                "Invalid use of argument --{} that only applies when the used codec is UASTC.", name);
    }

    void validateUASTCRDOArg(Reporter& report, const char* name) {
        validateUASTCArg(report, name);
        if (!basisOpts.uastcRDO)
            report.fatal(rc::INVALID_ARGUMENTS,
                "Invalid use of argument --{} when UASTC RDO post-processing was not enabled.", name);
    }

    void process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter& report) {
        if (ENCODE_CMD) {
            // "encode" command - required "codec" argument
            codec = validateEncodeCodec(args["codec"]);
            switch (codec) {
            case EncodeCodec::NONE:
                report.fatal(rc::INVALID_ARGUMENTS, "Missing codec argument.");
                break;

            case EncodeCodec::BasisLZ:
            case EncodeCodec::UASTC:
                codecName = to_lower_copy(args["codec"].as<std::string>());
                break;

            default:
                report.fatal(rc::INVALID_ARGUMENTS,
                    "Invalid codec: \"{}\".", args["codec"].as<std::string>());
                break;
            }
        } else {
            // "create" command - optional "encode" argument
            codec = validateEncodeCodec(args["encode"]);
            switch (codec) {
            case EncodeCodec::NONE:
                // Not specified
                break;

            case EncodeCodec::BasisLZ:
            case EncodeCodec::UASTC:
                codecName = to_lower_copy(args["encode"].as<std::string>());
                break;

            default:
                report.fatal_usage("Invalid encode codec: \"{}\".", args["encode"].as<std::string>());
                break;
            }
        }

        if (codec == EncodeCodec::UASTC) {
            basisOpts.uastc = 1;
        }

        // NOTE: The order of the validation below matters

        if (args["clevel"].count()) {
            validateBasisLZArg(report, "clevel");
            basisOpts.compressionLevel = args["clevel"].as<uint32_t>();
        }

        if (args["qlevel"].count()) {
            validateBasisLZArg(report, "qlevel");
            basisOpts.qualityLevel = args["qlevel"].as<uint32_t>();
        }

        if (args["no-endpoint-rdo"].count()) {
            validateBasisLZArg(report, "no-endpoint-rdo");
            basisOpts.noEndpointRDO = 1;
        }

        if (args["no-selector-rdo"].count()) {
            validateBasisLZArg(report, "no-selector-rdo");
            basisOpts.noSelectorRDO = 1;
        }

        if (args["max-endpoints"].count()) {
            validateBasisLZEndpointRDOArg(report, "max-endpoints");
            basisOpts.maxEndpoints = args["max-endpoints"].as<uint32_t>();
        }

        if (args["endpoint-rdo-threshold"].count()) {
            validateBasisLZEndpointRDOArg(report, "endpoint-rdo-threshold");
            basisOpts.endpointRDOThreshold = args["endpoint-rdo-threshold"].as<float>();
        }

        if (args["max-selectors"].count()) {
            validateBasisLZSelectorRDOArg(report, "max-selectors");
            basisOpts.maxSelectors = args["max-selectors"].as<uint32_t>();
        }

        if (args["selector-rdo-threshold"].count()) {
            validateBasisLZSelectorRDOArg(report, "selector-rdo-threshold");
            basisOpts.selectorRDOThreshold = args["selector-rdo-threshold"].as<float>();
        }

        if (args["uastc-quality"].count()) {
            validateUASTCArg(report, "uastc-quality");
            uint32_t level = args["uastc-quality"].as<uint32_t>();
            level = std::clamp<uint32_t>(level, 0, KTX_PACK_UASTC_MAX_LEVEL);
            basisOpts.uastcFlags = (unsigned int)~KTX_PACK_UASTC_LEVEL_MASK;
            basisOpts.uastcFlags |= level;
        }

        if (args["uastc-rdo"].count()) {
            validateUASTCArg(report, "uastc-rdo");
            basisOpts.uastcRDO = 1;
        }

        if (args["uastc-rdo-l"].count()) {
            validateUASTCRDOArg(report, "uastc-rdo-l");
            basisOpts.uastcRDOQualityScalar = args["uastc-rdo-l"].as<float>();
        }

        if (args["uastc-rdo-d"].count()) {
            validateUASTCRDOArg(report, "uastc-rdo-d");
            basisOpts.uastcRDODictSize = args["uastc-rdo-d"].as<uint32_t>();
        }

        if (args["uastc-rdo-b"].count()) {
            validateUASTCRDOArg(report, "uastc-rdo-b");
            basisOpts.uastcRDOMaxSmoothBlockErrorScale = args["uastc-rdo-b"].as<float>();
        }

        if (args["uastc-rdo-s"].count()) {
            validateUASTCRDOArg(report, "uastc-rdo-s");
            basisOpts.uastcRDOMaxSmoothBlockStdDev = args["uastc-rdo-s"].as<float>();
        }

        if (args["uastc-rdo-f"].count()) {
            validateUASTCRDOArg(report, "uastc-rdo-f");
            basisOpts.uastcRDODontFavorSimplerModes = 1;
        }

        if (args["uastc-rdo-m"].count()) {
            validateUASTCRDOArg(report, "uastc-rdo-m");
            basisOpts.uastcRDONoMultithreading = 1;
        }

        if (args["normal-mode"].count()) {
            validateCommonEncodeArg(report, "normal-mode");
            basisOpts.normalMap = true;
        }

        if (args["threads"].count()) {
            validateCommonEncodeArg(report, "threads");
            basisOpts.threadCount = args["threads"].as<uint32_t>();
        } else {
            basisOpts.threadCount = std::thread::hardware_concurrency();
        }

        if (args["no-sse"].count()) {
            validateCommonEncodeArg(report, "no-sse");
            basisOpts.noSSE = true;
        }
    }
};

} // namespace ktx

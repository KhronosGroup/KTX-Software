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

/**
//! [command options_codec]
<dl>
    <dt>
        basis-lz:
    </dt>
    <dd>
        Supercompress the image data with ETC1S / BasisLZ.
        RED images will become RGB with RED in each component (RRR). RG
        images will have R in the RGB part and G in the alpha part of
        the compressed texture (RRRG). When set, the following BasisLZ-related
        options become valid, otherwise they are ignored.
    </dd>

    <dl>
        <dt>\--clevel &lt;level&gt;</dt>
             <dd>ETC1S / BasisLZ compression level, an encoding speed vs.
             quality tradeoff. Range is [0,5], default is 1. Higher values
             are slower but give higher quality.</dd>
        <dt>\--qlevel &lt;level&gt;</dt>
             <dd>ETC1S / BasisLZ quality level. Range is [1,255]. Lower
             gives better compression/lower quality/faster. Higher gives
             less compression/higher quality/slower. @b --qlevel
             automatically determines values for @b --max-endpoints,
             @b --max-selectors, @b --endpoint-rdo-threshold and
             @b --selector-rdo-threshold for the target quality level.
             Setting these options overrides the values determined by
             -qlevel which defaults to 128 if neither it nor
             @b --max-endpoints and @b --max-selectors have been set.

             Note that both of @b --max-endpoints and @b --max-selectors
             must be set for them to have any effect. If all three options
             are set, a warning will be issued that @b --qlevel will be
             ignored.

             Note also that @b --qlevel will only determine values for
             @b --endpoint-rdo-threshold and @b --selector-rdo-threshold
             when its value exceeds 128, otherwise their defaults will be
             used.</dd>
        <dt>\--max-endpoints &lt;arg&gt;</dt>
             <dd>Manually set the maximum number of color endpoint clusters.
             Range is [1,16128]. Default is 0, unset.</dd>
        <dt>\--endpoint-rdo-threshold &lt;arg&gt;</dt>
             <dd>Set endpoint RDO quality threshold. The default is 1.25.
             Lower is higher quality but less quality per output bit (try
             [1.0,3.0]). This will override the value chosen by
             @b --qlevel.</dd>
        <dt>\--max-selectors &lt;arg&gt;</dt>
             <dd>Manually set the maximum number of color selector clusters
             from [1,16128]. Default is 0, unset.</dd>
        <dt>\--selector-rdo-threshold &lt;arg&gt;</dt>
             <dd>Set selector RDO quality threshold. The default is 1.25.
             Lower is higher quality but less quality per output bit (try
             [1.0,3.0]). This will override the value chosen by
             @b --qlevel.</dd>
        <dt>\--no-endpoint-rdo</dt>
             <dd>Disable endpoint rate distortion optimizations. Slightly
             faster, less noisy output, but lower quality per output bit.
             Default is to do endpoint RDO.</dd>
        <dt>\--no-selector-rdo</dt>
             <dd>Disable selector rate distortion optimizations. Slightly
             faster, less noisy output, but lower quality per output bit.
             Default is to do selector RDO.</dd>
    </dl>

    <dt>
        uastc:
    </dt>
    <dd>
        Create a texture in high-quality transcodable UASTC format.
    </dd>

    <dl>
        <dt>\--uastc-quality &lt;level&gt;</dt>
        <dd>This optional parameter selects a speed vs quality
            tradeoff as shown in the following table:

            <table>
            <tr><th>Level</th>   <th>Speed</th> <th>Quality</th></tr>
            <tr><td>0   </td><td> Fastest </td><td> 43.45dB</td></tr>
            <tr><td>1   </td><td> Faster </td><td> 46.49dB</td></tr>
            <tr><td>2   </td><td> Default </td><td> 47.47dB</td></tr>
            <tr><td>3   </td><td> Slower </td><td> 48.01dB</td></tr>
            <tr><td>4   </td><td> Very slow </td><td> 48.24dB</td></tr>
            </table>

            You are strongly encouraged to also specify @b --zstd to
            losslessly compress the UASTC data. This and any LZ-style
            compression can be made more effective by conditioning the
            UASTC texture data using the Rate Distortion Optimization (RDO)
            post-process stage. When uastc encoding is set the following
            options become available for controlling RDO:</dd>
        <dt>\--uastc-rdo</dt>
        <dd>Enable UASTC RDO post-processing.</dd>
        <dt>\--uastc-rdo-l &lt;lambda&gt;</dt>
        <dd>Set UASTC RDO quality scalar (lambda) to @e lambda. Lower values yield
            higher quality/larger LZ compressed files, higher values yield
            lower quality/smaller LZ compressed files. A good range to try
            is [.25,10]. For normal maps a good range is [.25,.75]. The
            full range is [.001,10.0]. Default is 1.0.

            Note that previous versions used the @b --uastc-rdo-q option
            which was removed because the RDO algorithm changed.</dd>
        <dt>\--uastc-rdo-d &lt;dictsize&gt;</dt>
        <dd>Set UASTC RDO dictionary size in bytes. Default is 4096.
            Lower values=faster, but give less compression. Range is
            [64,65536].</dd>
        <dt>\--uastc-rdo-b &lt;scale&gt;</dt>
        <dd>Set UASTC RDO max smooth block error scale. Range is
            [1.0,300.0]. Default is 10.0, 1.0 is disabled. Larger values
            suppress more artifacts (and allocate more bits) on smooth
            blocks.</dd>
        <dt>\--uastc-rdo-s &lt;deviation&gt;</dt>
        <dd>Set UASTC RDO max smooth block standard deviation. Range is
            [.01,65536.0]. Default is 18.0. Larger values expand the range
            of blocks considered smooth.</dd>
        <dt>\--uastc-rdo-f</dt>
        <dd>Do not favor simpler UASTC modes in RDO mode.</dd>
        <dt>\--uastc-rdo-m</dt>
        <dd>Disable RDO multithreading (slightly higher compression,
            deterministic).</dd>
    </dl>

    <dt>
        common:
    </dt>
    <dd>
        Common options.
    </dd>

    <dl>
        <dt>\--normal-mode</dt>
        <dd>Only valid for linear textures with two or more components.
            If the input texture has three or four linear components it is
            assumed to be a three component linear normal map storing unit
            length normals as (R=X, G=Y, B=Z). A fourth component will be
            ignored. The map will be converted to a two component X+Y
            normal map stored as (RGB=X, A=Y) prior to encoding. If unsure
            that your normals are unit length, use @b --normalize. If the
            input has 2 linear components it is assumed to be an X+Y map
            of unit normals.

            The Z component can be recovered programmatically in shader
            code by using the equations:
            <pre>
    nml.xy = texture(...).ga;              // Load in [0,1]
    nml.xy = nml.xy * 2.0 - 1.0;           // Unpack to [-1,1]
    nml.z = sqrt(1 - dot(nml.xy, nml.xy)); // Compute Z
            </pre>
            For ETC1S / BasisLZ encoding, @b '--encode basis-lz', RDO is disabled
            (no selector RDO, no endpoint RDO) to provide better quality.</dd>
        <dt>\--threads &lt;count&gt;</dt>
        <dd>Explicitly set the number of threads to use during
            compression. By default, ETC1S / BasisLZ will use the number of
            threads reported by thread::hardware_concurrency or 1 if value
            returned is 0.</dd>
        <dt>\--no-sse</dt>
        <dd>Forbid use of the SSE instruction set. Ignored if CPU does
            not support SSE. SSE can only be disabled on the basis-lz and
            uastc compressors.</dd>
    </dl>
</dl>
//! [command options_codec]
*/
template <bool ENCODE_CMD>
struct OptionsCodec {
    inline static const char* kCLevel = "clevel";
    inline static const char* kQLevel = "qlevel";
    inline static const char* kMaxEndpoints = "max-endpoints";
    inline static const char* kEndpointRdoThreshold = "endpoint-rdo-threshold";
    inline static const char* kMaxSelectors = "max-selectors";
    inline static const char* kSelectorRdoThreshold = "selector-rdo-threshold";
    inline static const char* kNoEndpointRdo = "no-endpoint-rdo";
    inline static const char* kNoSelectorRdo = "no-selector-rdo";
    inline static const char* kUastcQuality = "uastc-quality";
    inline static const char* kUastcRdo = "uastc-rdo";
    inline static const char* kUastcRdoL = "uastc-rdo-l";
    inline static const char* kUastcRdoD = "uastc-rdo-d";
    inline static const char* kUastcRdoB = "uastc-rdo-b";
    inline static const char* kUastcRdoS = "uastc-rdo-s";
    inline static const char* kUastcRdoF = "uastc-rdo-f";
    inline static const char* kUastcRdoM = "uastc-rdo-m";
    inline static const char* kNormalMode = "normal-mode";
    inline static const char* kThreads = "threads";
    inline static const char* kNoSse = "no-sse";

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
    };

    std::string codecOptions{};
    std::string codecName;
    EncodeCodec codec;
    BasisOptions basisOpts;

    void init(cxxopts::Options& opts) {
        opts.add_options("Encode BasisLZ")
            (kCLevel, "BasisLZ compression level, an encoding speed vs. quality level tradeoff. "
                "Range is [0,5], default is 1. Higher values are slower but give higher quality.",
                cxxopts::value<uint32_t>(), "<level>")
            (kQLevel, "BasisLZ quality level. Range is [1,255]. Lower gives better compression/lower "
                "quality/faster. Higher gives less compression/higher quality/slower. --qlevel "
                "automatically determines values for --max-endpoints, --max-selectors, "
                "--endpoint-rdo-threshold and --selector-rdo-threshold for the target quality level. "
                "Setting these options overrides the values determined by --qlevel which defaults to "
                "128 if neither it nor --max-endpoints and --max-selectors have been set.",
                cxxopts::value<uint32_t>(), "<level>")
            (kMaxEndpoints, "Manually set the maximum number of color endpoint clusters. Range "
                "is [1,16128]. Default is 0, unset.",
                cxxopts::value<uint32_t>(), "<arg>")
            (kEndpointRdoThreshold, "Set endpoint RDO quality threshold. The default is 1.25. Lower "
                "is higher quality but less quality per output bit (try [1.0,3.0]). This will override "
                "the value chosen by --qlevel.", cxxopts::value<float>(), "<arg>")
            (kMaxSelectors, "Manually set the maximum number of color selector clusters from [1,16128]. "
                "Default is 0, unset.", cxxopts::value<uint32_t>(), "<arg>")
            (kSelectorRdoThreshold, "Set selector RDO quality threshold. The default is 1.25. Lower "
                "is higher quality but less quality per output bit (try [1.0,3.0]). This will override "
                "the value chosen by --qlevel.", cxxopts::value<float>(), "<arg>")
            (kNoEndpointRdo, "Disable endpoint rate distortion optimizations. Slightly faster, "
                "less noisy output, but lower quality per output bit. Default is to do endpoint RDO.")
            (kNoSelectorRdo, "Disable selector rate distortion optimizations. Slightly faster, "
                "less noisy output, but lower quality per output bit. Default is to do selector RDO.");
        opts.add_options("Encode UASTC")
            (kUastcQuality, "UASTC compression level, an encoding speed vs. quality level tradeoff. "
                "Range is [0,4], default is 1. Higher values are slower but give higher quality.",
                cxxopts::value<uint32_t>(), "<level>")
            (kUastcRdo, "Enable UASTC RDO post-processing.")
            (kUastcRdoL, "Set UASTC RDO quality scalar to the specified value. Lower values yield "
                "higher quality/larger supercompressed files, higher values yield lower quality/smaller "
                "supercompressed files. A good range to try is [.25,10]. For normal maps a good range is "
                "[.25,.75]. The full range is [.001,10.0]. Default is 1.0.",
                cxxopts::value<float>(), "<lambda>")
            (kUastcRdoD, "Set UASTC RDO dictionary size in bytes. Default is 4096. Lower values=faster, "
                "but give less compression. Range is [64,65536].",
                cxxopts::value<uint32_t>(), "<dictsize>")
            (kUastcRdoB, "Set UASTC RDO max smooth block error scale. Range is [1.0,300.0]. Default "
                "is 10.0, 1.0 is disabled. Larger values suppress more artifacts (and allocate more bits) "
                "on smooth blocks.", cxxopts::value<float>(), "<scale>")
            (kUastcRdoS, "Set UASTC RDO max smooth block standard deviation. Range is [.01,65536.0]. "
                "Default is 18.0. Larger values expand the range of blocks considered smooth.",
                cxxopts::value<float>(), "<deviation>")
            (kUastcRdoF, "Do not favor simpler UASTC modes in RDO mode.")
            (kUastcRdoM, "Disable RDO multithreading (slightly higher compression, deterministic).");
        opts.add_options("Encode common")
            (kNormalMode, "Optimizes for encoding textures with normal data. If the input texture has "
                "three or four linear components it is assumed to be a three component linear normal "
                "map storing unit length normals as (R=X, G=Y, B=Z). A fourth component will be ignored. "
                "The map will be converted to a two component X+Y normal map stored as (RGB=X, A=Y) "
                "prior to encoding. If unsure that your normals are unit length, use --normalize. "
                "If the input has 2 linear components it is assumed to be an X+Y map of unit normals.\n"
                "The Z component can be recovered programmatically in shader code by using the equations:\n"
                "    nml.xy = texture(...).ga;              // Load in [0,1]\n"
                "    nml.xy = nml.xy * 2.0 - 1.0;           // Unpack to [-1,1]\n"
                "    nml.z = sqrt(1 - dot(nml.xy, nml.xy)); // Compute Z\n"
                "ETC1S / BasisLZ encoding, RDO is disabled (no selector RDO, no endpoint RDO) to provide better quality.")
            (kThreads, "Sets the number of threads to use during encoding. By default, encoding "
                "will use the number of threads reported by thread::hardware_concurrency or 1 if "
                "value returned is 0.", cxxopts::value<uint32_t>(), "<count>")
            (kNoSse, "Forbid use of the SSE instruction set. Ignored if CPU does "
               "not support SSE. SSE can only be disabled on the basis-lz and "
               "uastc compressors.");
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

    void captureCodecOption(const char* name) {
        codecOptions += fmt::format(" --{}", name);
    }

    template <typename T>
    T captureCodecOption(cxxopts::ParseResult& args, const char* name) {
        const T value = args[name].as<T>();
        codecOptions += fmt::format(" --{} {}", name, value);
        return value;
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
                report.fatal_usage("Invalid encode codec: \"{}\".", args["codec"].as<std::string>());
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

        if (args[kCLevel].count()) {
            validateBasisLZArg(report, kCLevel);
            basisOpts.compressionLevel = captureCodecOption<uint32_t>(args, kCLevel);;
        }

        if (args[kQLevel].count()) {
            validateBasisLZArg(report, kQLevel);
            basisOpts.qualityLevel = captureCodecOption<uint32_t>(args, kQLevel);
        }

        if (args[kNoEndpointRdo].count()) {
            validateBasisLZArg(report, kNoEndpointRdo);
            captureCodecOption(kNoEndpointRdo);
            basisOpts.noEndpointRDO = 1;
        }

        if (args[kNoSelectorRdo].count()) {
            validateBasisLZArg(report, kNoSelectorRdo);
            captureCodecOption(kNoSelectorRdo);
            basisOpts.noSelectorRDO = 1;
        }

        if (args[kMaxEndpoints].count()) {
            validateBasisLZEndpointRDOArg(report, kMaxEndpoints);
            basisOpts.maxEndpoints = captureCodecOption<uint32_t>(args, kMaxEndpoints);
        }

        if (args[kEndpointRdoThreshold].count()) {
            validateBasisLZEndpointRDOArg(report, kEndpointRdoThreshold);
            basisOpts.endpointRDOThreshold = captureCodecOption<float>(args, kEndpointRdoThreshold);
        }

        if (args[kMaxSelectors].count()) {
            validateBasisLZSelectorRDOArg(report, kMaxSelectors);
            basisOpts.maxSelectors = captureCodecOption<uint32_t>(args, kMaxSelectors);
        }

        if (args[kSelectorRdoThreshold].count()) {
            validateBasisLZSelectorRDOArg(report, kSelectorRdoThreshold);
            basisOpts.selectorRDOThreshold = captureCodecOption<float>(args, kSelectorRdoThreshold);
        }

        if (args[kUastcQuality].count()) {
            validateUASTCArg(report, kUastcQuality);
            uint32_t level = captureCodecOption<uint32_t>(args, kUastcQuality);
            level = std::clamp<uint32_t>(level, 0, KTX_PACK_UASTC_MAX_LEVEL);
            basisOpts.uastcFlags = (unsigned int)~KTX_PACK_UASTC_LEVEL_MASK;
            basisOpts.uastcFlags |= level;
        }

        if (args[kUastcRdo].count()) {
            validateUASTCArg(report, kUastcRdo);
            captureCodecOption(kUastcRdo);
            basisOpts.uastcRDO = 1;
        }

        if (args[kUastcRdoL].count()) {
            validateUASTCRDOArg(report, kUastcRdoL);
            basisOpts.uastcRDOQualityScalar = captureCodecOption<float>(args, kUastcRdoL);
        }

        if (args[kUastcRdoD].count()) {
            validateUASTCRDOArg(report, kUastcRdoD);
            basisOpts.uastcRDODictSize = captureCodecOption<uint32_t>(args, kUastcRdoD);
        }

        if (args[kUastcRdoB].count()) {
            validateUASTCRDOArg(report, kUastcRdoB);
            basisOpts.uastcRDOMaxSmoothBlockErrorScale = captureCodecOption<float>(args, kUastcRdoB);
        }

        if (args[kUastcRdoS].count()) {
            validateUASTCRDOArg(report, kUastcRdoS);
            basisOpts.uastcRDOMaxSmoothBlockStdDev = captureCodecOption<float>(args, kUastcRdoS);
        }

        if (args[kUastcRdoF].count()) {
            validateUASTCRDOArg(report, kUastcRdoF);
            captureCodecOption(kUastcRdoF);
            basisOpts.uastcRDODontFavorSimplerModes = 1;
        }

        if (args[kUastcRdoM].count()) {
            validateUASTCRDOArg(report, kUastcRdoM);
            captureCodecOption(kUastcRdoM);
            basisOpts.uastcRDONoMultithreading = 1;
        }

        if (args[kNormalMode].count()) {
            validateCommonEncodeArg(report, kNormalMode);
            captureCodecOption(kNormalMode);
            basisOpts.normalMap = true;
        }

        if (args[kThreads].count()) {
            validateCommonEncodeArg(report, kThreads);
            basisOpts.threadCount = captureCodecOption<uint32_t>(args, kThreads);
        } else {
            basisOpts.threadCount = std::thread::hardware_concurrency();
        }

        if (args[kNoSse].count()) {
            validateCommonEncodeArg(report, kNoSse);
            captureCodecOption(kNoSse);
            basisOpts.noSSE = true;
        }
    }
};

} // namespace ktx

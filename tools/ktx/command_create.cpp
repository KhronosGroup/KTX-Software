// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include "command.h"
#include "encode_utils_common.h"
#include "platform_utils.h"
#include "metrics_utils.h"
#include "deflate_utils.h"
#include "encode_utils_basis.h"
#include "encode_utils_astc.h"
#include "format_descriptor.h"
#include "formats.h"
#include "utility.h"
#include <filesystem>
#include <iostream>
#include <regex>
#include <sstream>
#include <cxxopts.hpp>
#include <fmt/ostream.h>
#include <fmt/printf.h>
#include "ktx.h"
#include "image.hpp"
#include "imageio.h"

/** @file
 * @~English
 * @brief @b create command implementation.
 */

// -------------------------------------------------------------------------------------------------

namespace ktx {

struct ColorSpaceInfo {
    khr_df_transfer_e usedInputTransferFunction;
    khr_df_primaries_e usedInputPrimaries;
    std::unique_ptr<const TransferFunction> srcTransferFunction{};
    std::unique_ptr<const TransferFunction> dstTransferFunction{};
    std::unique_ptr<const ColorPrimaries> srcColorPrimaries{};
    std::unique_ptr<const ColorPrimaries> dstColorPrimaries{};
};

// -------------------------------------------------------------------------------------------------

struct OptionsCreate {
    inline static const char* kFormat = "format";
    inline static const char* k1D = "1d";
    inline static const char* kCubemap = "cubemap";
    inline static const char* kRaw = "raw";
    inline static const char* kWidth = "width";
    inline static const char* kHeight = "height";
    inline static const char* kDepth = "depth";
    inline static const char* kLayers = "layers";
    inline static const char* kLevels = "levels";
    inline static const char* kRuntimeMipmap = "runtime-mipmap";
    inline static const char* kGenerateMipmap = "generate-mipmap";
    inline static const char* kEncode = "encode";
    inline static const char* kNormalize = "normalize";
    inline static const char* kSwizzle = "swizzle";
    inline static const char* kInputSwizzle = "input-swizzle";
    inline static const char* kAssignOetf = "assign-oetf";
    inline static const char* kAssignPrimaries = "assign-primaries";
    inline static const char* kAssignTexcoordOrigin = "assign-texcoord-origin";
    inline static const char* kConvertOetf = "convert-oetf";
    inline static const char* kConvertPrimaries = "convert-primaries";
    inline static const char* kConvertTexcoordOrigin = "convert-texcoord-origin";
    inline static const char* kFailOnColorConversions = "fail-on-color-conversions";
    inline static const char* kWarnOnColorConversions = "warn-on-color-conversions";
    inline static const char* kFailOnOriginChanges = "fail-on-origin-changes";
    inline static const char* kWarnOnOriginChanges = "warn-on-origin-changes";
    inline static const char* kMipmapFilter = "mipmap-filter";
    inline static const char* kMipmapFilterScale = "mipmap-filter-scale";
    inline static const char* kMipmapWrap = "mipmap-wrap";

    bool _1d = false;
    bool cubemap = false;

    VkFormat vkFormat = VK_FORMAT_UNDEFINED;
    FormatDescriptor formatDesc;
    bool raw = false;

    std::optional<uint32_t> width;
    std::optional<uint32_t> height;
    std::optional<uint32_t> depth;
    std::optional<uint32_t> layers;
    std::optional<uint32_t> levels;

    bool mipmapRuntime = false;
    bool mipmapGenerate = false;
    std::optional<std::string> mipmapFilter;
    std::string defaultMipmapFilter = "lanczos4";
    std::optional<float> mipmapFilterScale;
    float defaultMipmapFilterScale = 1.0f;
    std::optional<basisu::Resampler::Boundary_Op> mipmapWrap;
    basisu::Resampler::Boundary_Op defaultMipmapWrap = basisu::Resampler::Boundary_Op::BOUNDARY_WRAP;
    std::optional<std::string> swizzle; /// Sets KTXswizzle
    std::optional<std::string> swizzleInput; /// Used to swizzle the input image data

    std::optional<khr_df_transfer_e> convertOETF = {};
    std::optional<khr_df_transfer_e> assignOETF = {};
    std::optional<khr_df_primaries_e> assignPrimaries = {};
    std::optional<khr_df_primaries_e> convertPrimaries = {};
    std::optional<ImageSpec::Origin> assignTexcoordOrigin;
    std::optional<ImageSpec::Origin> convertTexcoordOrigin;
    bool failOnColorConversions = false;
    bool warnOnColorConversions = false;
    bool failOnOriginChanges = false;
    bool warnOnOriginChanges = false;
    bool normalize = false;

    void init(cxxopts::Options& opts) {
        opts.add_options()
                (kFormat, "KTX format enum that specifies the image data format."
                    " The enum names are matching the VkFormats without the VK_FORMAT_ prefix."
                    " The VK_FORMAT_ prefix is ignored if present."
                    "\nWhen used with --encode it specifies the target format before the encoding step."
                    " In this case it must be one of:"
                    "\n    R8_UNORM"
                    "\n    R8_SRGB"
                    "\n    R8G8_UNORM"
                    "\n    R8G8_SRGB"
                    "\n    R8G8B8_UNORM"
                    "\n    R8G8B8_SRGB"
                    "\n    R8G8B8A8_UNORM"
                    "\n    R8G8B8A8_SRGB"
                    "\nIf the format is an ASTC format the ASTC encoder specific options become valid,"
                    " otherwise they are ignored."
                    "\nThe format will be used to verify and load all input files into a texture before encoding."
                    " Case insensitive. Required.", cxxopts::value<std::string>(), "<enum>")
                (k1D, "Create a 1D texture. If not set the texture will be a 2D or 3D texture.")
                (kCubemap, "Create a cubemap texture. If not set the texture will be a 2D or 3D texture.")
                (kRaw, "Create from raw image data.")
                (kWidth, "Base level width in pixels.", cxxopts::value<uint32_t>(), "[0-9]+")
                (kHeight, "Base level height in pixels.", cxxopts::value<uint32_t>(), "[0-9]+")
                (kDepth, "Base level depth in pixels. If set the texture will be a 3D texture.", cxxopts::value<uint32_t>(), "[0-9]+")
                (kLayers, "Number of layers. If set the texture will be an array texture.", cxxopts::value<uint32_t>(), "[0-9]+")
                (kLevels, "Number of mip levels.", cxxopts::value<uint32_t>(), "[0-9]+")
                (kRuntimeMipmap, "Runtime mipmap generation mode.")
                (kGenerateMipmap, "Causes mipmaps to be generated during texture creation."
                    " It enables the use of \'Generate Mipmap\' options."
                    " If the --levels is not specified the maximum possible mip level will be generated."
                    " This option is mutually exclusive with --runtime-mipmap and cannot be used with UINT or 3D textures.")
                (kEncode, "Encode the created KTX file. Case insensitive."
                    "\nPossible options are: basis-lz | uastc", cxxopts::value<std::string>(), "<codec>")
                (kNormalize, "Normalize input normals to have a unit length. Only valid for\n"
                    "linear normal textures with 2 or more components. For 2-component inputs\n"
                    "2D unit normals are calculated. Do not use these 2D unit normals\n"
                    "to generate X+Y normals with --normal-mode. For 4-component inputs\n"
                    "a 3D unit normal is calculated. 1.0 is used for the value of the\n"
                    "4th component.\n")
                (kSwizzle, "KTX swizzle metadata.", cxxopts::value<std::string>(), "[rgba01]{4}")
                (kInputSwizzle, "Pre-swizzle input channels.", cxxopts::value<std::string>(), "[rgba01]{4}")
                (kAssignOetf, "Force the created texture to have the specified transfer function, ignoring"
                    " the transfer function of the input file(s). Case insensitive."
                    "\nPossible options are: linear | srgb", cxxopts::value<std::string>(), "<oetf>")
                (kAssignPrimaries, "Force the created texture to have the specified color primaries, ignoring"
                    " the color primaries of the input file(s). Case insensitive."
                    "\nPossible options are:"
                    " none | bt709 | srgb | bt601-ebu | bt601-smpte | bt2020 | ciexyz | aces | acescc | ntsc1953 | pal525 | displayp3 | adobergb.",
                    cxxopts::value<std::string>(), "<primaries>")
                (kAssignTexcoordOrigin, "Force the created texture to indicate that the texture coordinate"
                    " origin s=0, t=0 is at the specified corner of the image. Case insensitive."
                    "\nPossible options are top-left | bottom-left. -front | -back can be appended and"
                    " one of these is required when --depth is specified. Must be top-left if --cubemap"
                    " is specified."
                    "\nAbsent --convert-texcoord-origin, the effect of this option is to cause KTXorientation"
                    " metadata indicating the specified origin to be written to the output file.",
                    cxxopts::value<std::string>(), "<origin>")
                (kConvertOetf, "Convert the input image(s) to the specified transfer function, if different"
                    " from the transfer function of the input file(s). If both this and --assign-oetf are specified,"
                    " conversion will be performed from the assigned transfer function to the transfer function"
                    " specified by this option, if different. Case insensitive."
                    "\nPossible options are: linear | srgb", cxxopts::value<std::string>(), "<oetf>")
                (kConvertPrimaries, "Convert the image image(s) to the specified color primaries, if different"
                    " from the color primaries of the input file(s) or the one specified by --assign-primaries."
                    " If both this and --assign-primaries are specified, conversion will be performed from "
                    " the assigned primaries to the primaries specified by this option, if different."
                    " This option is not allowed to be specified when --assign-primaries is set to 'none'."
                    " Case insensitive."
                    "\nPossible options are:"
                    " bt709 | srgb | bt601-ebu | bt601-smpte | bt2020 | ciexyz | aces | acescc | ntsc1953 | pal525 | displayp3 | adobergb.",
                    cxxopts::value<std::string>(), "<primaries>")
                (kConvertTexcoordOrigin, "Convert the input image(s) so the texture coordinate origin s=0,"
                    " t=0, is at the specified corner of the image. If both this and --assign-texcoord-origin"
                    " are specified, conversion will be performed from the assigned origin to the origin"
                    " specified by this option, if different. Case insensitive."
                    "\nPossible options are top-left | bottom-left. -front | -back can be appended and"
                    " one of these is required when --depth is specified. Must be top-left if --cubemap"
                    " is specified."
                    "\nInput images whose origin does not match corner will be flipped vertically."
                    " KTXorientation metadata indicating the specified origin is written to the output file.",
                    cxxopts::value<std::string>(), "<origin>")
                (kFailOnColorConversions, "Generates an error if any of the input images would need to be color converted.")
                (kWarnOnColorConversions, "Generates a warning if any of the input images are color converted.")
                (kFailOnOriginChanges, "Generates an error if any of the input images would need to have their origin changed.")
                (kWarnOnOriginChanges, "Generates a warning if any of the input images have their origin changed.");

        opts.add_options("Generate Mipmap")
                (kMipmapFilter, "Specifies the filter to use when generating the mipmaps. Case insensitive."
                    "\nPossible options are:"
                    " box | tent | bell | b-spline | mitchell | blackman | lanczos3 | lanczos4 | lanczos6 |"
                    " lanczos12 | kaiser | gaussian | catmullrom | quadratic_interp | quadratic_approx | "
                    " quadratic_mix."
                    " Defaults to lanczos4.",
                    cxxopts::value<std::string>(), "<filter>")
                (kMipmapFilterScale, "The filter scale to use. Defaults to 1.0.", cxxopts::value<float>(), "<float>")
                (kMipmapWrap, "Specify how to sample pixels near the image boundaries. Case insensitive."
                    "\nPossible options are:"
                    " wrap | reflect | clamp."
                    " Defaults to clamp.", cxxopts::value<std::string>(), "<mode>");
    }

    std::optional<khr_df_transfer_e> parseTransferFunction(cxxopts::ParseResult& args, const char* argName, Reporter& report) const {
        static const std::unordered_map<std::string, khr_df_transfer_e> values{
            { "LINEAR", KHR_DF_TRANSFER_LINEAR },
            { "SRGB", KHR_DF_TRANSFER_SRGB }
        };

        std::optional<khr_df_transfer_e> result = {};

        if (args[argName].count()) {
            const auto oetfStr = args[argName].as<std::string>();
            const auto it = values.find(to_upper_copy(oetfStr));
            if (it != values.end()) {
                result = it->second;
            } else {
                report.fatal_usage("Invalid or unsupported transfer function specified as --{} argument: \"{}\".", argName, oetfStr);
            }
        }

        return result;
    }

    std::optional<khr_df_primaries_e> parseColorPrimaries(cxxopts::ParseResult& args, const char* argName, Reporter& report) const {
        static const std::unordered_map<std::string, khr_df_primaries_e> values{
            { "NONE", KHR_DF_PRIMARIES_UNSPECIFIED },
            { "BT709", KHR_DF_PRIMARIES_BT709 },
            { "SRGB", KHR_DF_PRIMARIES_SRGB },
            { "BT601-EBU", KHR_DF_PRIMARIES_BT601_EBU },
            { "BT601-SMPTE", KHR_DF_PRIMARIES_BT601_SMPTE },
            { "BT2020", KHR_DF_PRIMARIES_BT2020 },
            { "CIEXYZ", KHR_DF_PRIMARIES_CIEXYZ },
            { "ACES", KHR_DF_PRIMARIES_ACES },
            { "ACESCC", KHR_DF_PRIMARIES_ACESCC },
            { "NTSC1953", KHR_DF_PRIMARIES_NTSC1953 },
            { "PAL525", KHR_DF_PRIMARIES_PAL525 },
            { "DISPLAYP3", KHR_DF_PRIMARIES_DISPLAYP3 },
            { "ADOBERGB", KHR_DF_PRIMARIES_ADOBERGB },
        };

        std::optional<khr_df_primaries_e> result = {};

        if (args[argName].count()) {
            const auto primariesStr = args[argName].as<std::string>();
            const auto it = values.find(to_upper_copy(primariesStr));
            if (it != values.end()) {
                result = it->second;
            } else {
                report.fatal_usage("Invalid or unsupported primaries specified as --{} argument: \"{}\".", argName, primariesStr);
            }
        }

        return result;
    }

  std::optional<ImageSpec::Origin> parseTexcoordOrigin(cxxopts::ParseResult& args, uint32_t numDimensions, const char* argName, Reporter& report) const {
        std::optional<ImageSpec::Origin> result;
        if (args[argName].count()) {
            // RE to extract origin for each dimension.
            // - Match 0 is whole matching string.
            // - Match 1 is the y origin.
            // - Match 2 is the x origin.
            // - Match 3 is the z origin. Empty string, if not specified.
            // Use raw literal to avoid excess blackslashes
            std::regex re(R"--((?:\b(top|bottom)\b-)(?:\b(left)\b)(?:-\b(front|back)\b)?)--");
            // For when support for right origin and 1d textures is added.
            //               y dimension made optional ꜜ right added ꜜ
            //std::regex re(R"--((?:\b(top|bottom)\b-)?(?:\b(left|right)\b)(?:-\b(front|back)\b)?)--");

            // "auto" here leads to no matching function call for regex_match.
            const std::string& originStr = to_lower_copy(args[argName].as<std::string>());
            std::smatch sm;
            std::regex_match(originStr.begin(), originStr.end(), sm, re);
#if DEBUG_REGEX
              std::cout << "match size: " << sm.size() << '\n';
              for(uint32_t i = 0; i < sm.size(); i++) {
                  std::cout << "match " << i << ": " << "\"" << sm.str(i) << "\"" << '\n';
              }
#endif
            if (sm.empty()) {
                report.fatal_usage("Invalid or unsupported origin specified as --{} argument: \"{}\".", argName, originStr);
            }
            if (numDimensions == 3 && sm.str(3).empty()) {
                report.fatal_usage("Z origin must be specified in --{} argument for a 3D texture.", argName);
            }

            ImageSpec::Origin orig;
            // Remember, compare returns 0 for a match.
            orig.x = sm.str(2).compare("left") ? ImageSpec::Origin::eRight
                                               : ImageSpec::Origin::eLeft;
            orig.y = sm.str(1).compare("bottom") ? ImageSpec::Origin::eTop
                                                 : ImageSpec::Origin::eBottom;
            if (args[kCubemap].count()) {
                if (orig.x != ImageSpec::Origin::eLeft || orig.y != ImageSpec::Origin::eTop) {
                    report.fatal_usage("--{} argument must be --top-left for a cubemap.", argName);
                }
            }
            if (numDimensions == 3)
                orig.z = sm.str(3).compare("front") ? ImageSpec::Origin::eFront
                                                    : ImageSpec::Origin::eBack;
            result = std::move(orig);
        }

        return result;
    }


    void process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter& report) {
        _1d = args[k1D].as<bool>();
        cubemap = args[kCubemap].as<bool>();
        raw = args[kRaw].as<bool>();

        if (args[kWidth].count())
            width = args[kWidth].as<uint32_t>();
        if (args[kHeight].count())
            height = args[kHeight].as<uint32_t>();
        if (args[kDepth].count())
            depth = args[kDepth].as<uint32_t>();
        if (args[kLayers].count())
            layers = args[kLayers].as<uint32_t>();
        if (args[kLevels].count())
            levels = args[kLevels].as<uint32_t>();

        mipmapRuntime = args[kRuntimeMipmap].as<bool>();
        mipmapGenerate = args[kGenerateMipmap].as<bool>();

        if (args[kMipmapFilter].count()) {
            static const std::unordered_set<std::string> filter_table{
                "box",
                "tent",
                "bell",
                "b-spline",
                "mitchell",
                "blackman",
                "lanczos3",
                "lanczos4",
                "lanczos6",
                "lanczos12",
                "kaiser",
                "gaussian",
                "catmullrom",
                "quadratic_interp",
                "quadratic_approx",
                "quadratic_mix",
            };

            mipmapFilter = to_lower_copy(args[kMipmapFilter].as<std::string>());
            if (filter_table.count(*mipmapFilter) == 0)
                report.fatal_usage("Invalid or unsupported mipmap filter specified as --mipmap-filter argument: \"{}\".", *mipmapFilter);
        }

        if (args[kMipmapFilterScale].count())
            mipmapFilterScale = args[kMipmapFilterScale].as<float>();

        if (args[kMipmapWrap].count()) {
            static const std::unordered_map<std::string, basisu::Resampler::Boundary_Op> wrap_table{
                { "clamp", basisu::Resampler::Boundary_Op::BOUNDARY_CLAMP },
                { "wrap", basisu::Resampler::Boundary_Op::BOUNDARY_WRAP },
                { "reflect", basisu::Resampler::Boundary_Op::BOUNDARY_REFLECT },
            };

            const auto wrapStr = to_lower_copy(args[kMipmapWrap].as<std::string>());
            const auto it = wrap_table.find(wrapStr);
            if (it == wrap_table.end())
                report.fatal_usage("Invalid or unsupported mipmap wrap mode specified as --mipmap-wrap argument: \"{}\".", wrapStr);
            else
                mipmapWrap = it->second;
        }

        if (args[kNormalize].count())
            normalize = true;

        if (args[kSwizzle].count()) {
            swizzle = to_lower_copy(args[kSwizzle].as<std::string>());
            const auto errorFmt = "Invalid --swizzle value: \"{}\". The value must match the \"[rgba01]{{4}}\" regex.";
            if (swizzle->size() != 4)
                report.fatal_usage(errorFmt, *swizzle);
            for (const auto c : *swizzle)
                if (!contains("rgba01", c))
                    report.fatal_usage(errorFmt, *swizzle);
        }
        if (args[kInputSwizzle].count()) {
            swizzleInput = to_lower_copy(args[kInputSwizzle].as<std::string>());
            const auto errorFmt = "Invalid --input-swizzle value: \"{}\". The value must match the \"[rgba01]{{4}}\" regex.";
            if (swizzleInput->size() != 4)
                report.fatal_usage(errorFmt, *swizzleInput);
            for (const auto c : *swizzleInput)
                if (!contains("rgba01", c))
                    report.fatal_usage(errorFmt, *swizzleInput);
        }

        uint32_t numDimensions = 2;
        if (args[kDepth].count())
            numDimensions = 3;
        else if (args[k1D].count())
            numDimensions = 1;
        assignTexcoordOrigin = parseTexcoordOrigin(args, numDimensions,
                                              kAssignTexcoordOrigin, report);
        convertTexcoordOrigin = parseTexcoordOrigin(args, numDimensions,
                                              kConvertTexcoordOrigin, report);

        if (args[kFormat].count()) {
            const auto formatStr = args[kFormat].as<std::string>();
            const auto parsedVkFormat = parseVkFormat(formatStr);
            if (!parsedVkFormat)
                report.fatal_usage("The requested format is invalid or unsupported: \"{}\".", formatStr);

            vkFormat = *parsedVkFormat;
        } else {
            report.fatal_usage("Required option 'format' is missing.");
        }

        // List of formats that have supported format conversions
        static const std::unordered_set<VkFormat> convertableFormats{
                VK_FORMAT_R8_UNORM,
                VK_FORMAT_R8_SRGB,
                VK_FORMAT_R8G8_UNORM,
                VK_FORMAT_R8G8_SRGB,
                VK_FORMAT_R8G8B8_UNORM,
                VK_FORMAT_R8G8B8_SRGB,
                VK_FORMAT_B8G8R8_UNORM,
                VK_FORMAT_B8G8R8_SRGB,
                VK_FORMAT_R8G8B8A8_UNORM,
                VK_FORMAT_R8G8B8A8_SRGB,
                VK_FORMAT_B8G8R8A8_UNORM,
                VK_FORMAT_B8G8R8A8_SRGB,
                VK_FORMAT_A8B8G8R8_UNORM_PACK32,
                VK_FORMAT_A8B8G8R8_SRGB_PACK32,
                VK_FORMAT_ASTC_4x4_UNORM_BLOCK,
                VK_FORMAT_ASTC_4x4_SRGB_BLOCK,
                VK_FORMAT_ASTC_5x4_UNORM_BLOCK,
                VK_FORMAT_ASTC_5x4_SRGB_BLOCK,
                VK_FORMAT_ASTC_5x5_UNORM_BLOCK,
                VK_FORMAT_ASTC_5x5_SRGB_BLOCK,
                VK_FORMAT_ASTC_6x5_UNORM_BLOCK,
                VK_FORMAT_ASTC_6x5_SRGB_BLOCK,
                VK_FORMAT_ASTC_6x6_UNORM_BLOCK,
                VK_FORMAT_ASTC_6x6_SRGB_BLOCK,
                VK_FORMAT_ASTC_8x5_UNORM_BLOCK,
                VK_FORMAT_ASTC_8x5_SRGB_BLOCK,
                VK_FORMAT_ASTC_8x6_UNORM_BLOCK,
                VK_FORMAT_ASTC_8x6_SRGB_BLOCK,
                VK_FORMAT_ASTC_8x8_UNORM_BLOCK,
                VK_FORMAT_ASTC_8x8_SRGB_BLOCK,
                VK_FORMAT_ASTC_10x5_UNORM_BLOCK,
                VK_FORMAT_ASTC_10x5_SRGB_BLOCK,
                VK_FORMAT_ASTC_10x6_UNORM_BLOCK,
                VK_FORMAT_ASTC_10x6_SRGB_BLOCK,
                VK_FORMAT_ASTC_10x8_UNORM_BLOCK,
                VK_FORMAT_ASTC_10x8_SRGB_BLOCK,
                VK_FORMAT_ASTC_10x10_UNORM_BLOCK,
                VK_FORMAT_ASTC_10x10_SRGB_BLOCK,
                VK_FORMAT_ASTC_12x10_UNORM_BLOCK,
                VK_FORMAT_ASTC_12x10_SRGB_BLOCK,
                VK_FORMAT_ASTC_12x12_UNORM_BLOCK,
                VK_FORMAT_ASTC_12x12_SRGB_BLOCK,
                VK_FORMAT_R4G4_UNORM_PACK8,
                VK_FORMAT_R5G6B5_UNORM_PACK16,
                VK_FORMAT_B5G6R5_UNORM_PACK16,
                VK_FORMAT_R4G4B4A4_UNORM_PACK16,
                VK_FORMAT_B4G4R4A4_UNORM_PACK16,
                VK_FORMAT_R5G5B5A1_UNORM_PACK16,
                VK_FORMAT_B5G5R5A1_UNORM_PACK16,
                VK_FORMAT_A1R5G5B5_UNORM_PACK16,
                VK_FORMAT_A4R4G4B4_UNORM_PACK16,
                VK_FORMAT_A4B4G4R4_UNORM_PACK16,
                VK_FORMAT_R10X6_UNORM_PACK16,
                VK_FORMAT_R10X6G10X6_UNORM_2PACK16,
                VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16,
                VK_FORMAT_R12X4_UNORM_PACK16,
                VK_FORMAT_R12X4G12X4_UNORM_2PACK16,
                VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16,
                VK_FORMAT_R16_UNORM,
                VK_FORMAT_R16G16_UNORM,
                VK_FORMAT_R16G16B16_UNORM,
                VK_FORMAT_R16G16B16A16_UNORM,
                VK_FORMAT_A2R10G10B10_UNORM_PACK32,
                VK_FORMAT_A2B10G10R10_UNORM_PACK32,
                VK_FORMAT_G8B8G8R8_422_UNORM,
                VK_FORMAT_B8G8R8G8_422_UNORM,
                VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,
                VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,
                VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,
                VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,
                VK_FORMAT_G16B16G16R16_422_UNORM,
                VK_FORMAT_B16G16R16G16_422_UNORM,
                VK_FORMAT_R8_UINT,
                VK_FORMAT_R8_SINT,
                VK_FORMAT_R16_UINT,
                VK_FORMAT_R16_SINT,
                VK_FORMAT_R32_UINT,
                VK_FORMAT_R8G8_UINT,
                VK_FORMAT_R8G8_SINT,
                VK_FORMAT_R16G16_UINT,
                VK_FORMAT_R16G16_SINT,
                VK_FORMAT_R32G32_UINT,
                VK_FORMAT_R8G8B8_UINT,
                VK_FORMAT_R8G8B8_SINT,
                VK_FORMAT_B8G8R8_UINT,
                VK_FORMAT_B8G8R8_SINT,
                VK_FORMAT_R16G16B16_UINT,
                VK_FORMAT_R16G16B16_SINT,
                VK_FORMAT_R32G32B32_UINT,
                VK_FORMAT_R8G8B8A8_UINT,
                VK_FORMAT_R8G8B8A8_SINT,
                VK_FORMAT_B8G8R8A8_UINT,
                VK_FORMAT_B8G8R8A8_SINT,
                VK_FORMAT_A8B8G8R8_UINT_PACK32,
                VK_FORMAT_A8B8G8R8_SINT_PACK32,
                VK_FORMAT_R16G16B16A16_UINT,
                VK_FORMAT_R16G16B16A16_SINT,
                VK_FORMAT_R32G32B32A32_UINT,
                VK_FORMAT_A2R10G10B10_UINT_PACK32,
                VK_FORMAT_A2R10G10B10_SINT_PACK32,
                VK_FORMAT_A2B10G10R10_SINT_PACK32,
                VK_FORMAT_A2B10G10R10_UINT_PACK32,
                VK_FORMAT_R16_SFLOAT,
                VK_FORMAT_R16G16_SFLOAT,
                VK_FORMAT_R16G16B16_SFLOAT,
                VK_FORMAT_R16G16B16A16_SFLOAT,
                VK_FORMAT_R32_SFLOAT,
                VK_FORMAT_R32G32_SFLOAT,
                VK_FORMAT_R32G32B32_SFLOAT,
                VK_FORMAT_R32G32B32A32_SFLOAT,
                VK_FORMAT_B10G11R11_UFLOAT_PACK32,
                VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,
                VK_FORMAT_D16_UNORM,
                VK_FORMAT_X8_D24_UNORM_PACK32,
                VK_FORMAT_D32_SFLOAT,
                VK_FORMAT_S8_UINT,
                VK_FORMAT_D16_UNORM_S8_UINT,
                VK_FORMAT_D24_UNORM_S8_UINT,
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                VK_FORMAT_A8_UNORM_KHR,
                VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR,
        };

        if (isProhibitedFormat(vkFormat))
            report.fatal_usage("The requested {} format is prohibited in KTX files.", toString(vkFormat));

        if (!raw && !convertableFormats.count(vkFormat))
            report.fatal_usage("Unsupported format for non-raw create: {}.", toString(vkFormat));

        if (raw) {
            if (!width)
                report.fatal_usage("Option --width is missing but is required for --raw texture creation.");
            if (!height)
                report.fatal_usage("Option --height is missing but is required for --raw texture creation.");
        } else {
            if (width)
                report.warning("Option --width is ignored for non-raw texture creation.");
            if (height)
                report.warning("Option --height is ignored for non-raw texture creation.");
        }

        if (width == 0u)
            report.fatal_usage("The --width cannot be 0.");
        if (height == 0u)
            report.fatal_usage("The --height cannot be 0.");
        if (layers == 0u)
            report.fatal_usage("The --layers cannot be 0.");
        if (levels == 0u)
            report.fatal_usage("The --levels cannot be 0.");
        if (depth == 0u)
            report.fatal_usage("The --depth cannot be 0.");

        if (raw) {
            const auto maxDimension = std::max(width.value_or(1), std::max(height.value_or(1), depth.value_or(1)));
            const auto maxLevels = log2(maxDimension) + 1;

            if (levels.value_or(1) > maxLevels)
                report.fatal_usage("Requested {} levels is too many. With base size {}x{}x{} the texture can only have {} levels at most.",
                        levels.value_or(1), width.value_or(1), height.value_or(1), depth.value_or(1), maxLevels);
        }

        if (_1d && height && height != 1u)
            report.fatal_usage("For --1d textures the --height must be 1.");

        if (layers && depth)
            report.fatal_usage("3D array texture creation is unsupported. --layers is {} and --depth is {}.",
                    *layers, *depth);

        if (cubemap && depth)
            report.fatal_usage("Cubemaps cannot have 3D textures. --depth is {}.", *depth);

        if (mipmapRuntime && levels.value_or(1) > 1u)
            report.fatal_usage("Conflicting options: --runtime-mipmap cannot be used with more than 1 --levels.");

        if (mipmapGenerate && mipmapRuntime)
            report.fatal_usage("Conflicting options: --generate-mipmap and --runtime-mipmap cannot be used together.");

        if (mipmapGenerate && raw)
            report.fatal_usage("Conflicting options: --generate-mipmap cannot be used with --raw.");

        if (mipmapGenerate && depth)
            report.fatal_usage("Mipmap generation for 3D textures is not supported: --generate-mipmap cannot be used with --depth.");

        if (mipmapFilter && !mipmapGenerate)
            report.fatal_usage("Option --mipmap-filter can only be used if --generate-mipmap is set.");

        if (mipmapFilterScale && !mipmapGenerate)
            report.fatal_usage("Option --mipmap-filter-scale can only be used if --generate-mipmap is set.");

        if (mipmapWrap && !mipmapGenerate)
            report.fatal_usage("Option --mipmap-wrap can only be used if --generate-mipmap is set.");

        formatDesc = createFormatDescriptor(vkFormat, report);

        convertOETF = parseTransferFunction(args, kConvertOetf, report);
        assignOETF = parseTransferFunction(args, kAssignOetf, report);

        convertPrimaries = parseColorPrimaries(args, kConvertPrimaries, report);
        assignPrimaries = parseColorPrimaries(args, kAssignPrimaries, report);

        if (convertPrimaries.has_value() && assignPrimaries == KHR_DF_PRIMARIES_UNSPECIFIED)
            report.fatal_usage("Option --{} cannot be used when --{} is set to 'none'.",
                               kConvertPrimaries, kAssignPrimaries);

        if (raw) {
            if (convertOETF.has_value())
                report.fatal_usage("Option {} cannot be used with --{}.", kConvertOetf, kRaw);
            if (convertPrimaries.has_value())
                report.fatal_usage("Option {} cannot be used with --{}.", kConvertPrimaries, kRaw);
            if (convertTexcoordOrigin.has_value())
                report.fatal_usage("Option {} cannot be used with --{}.", kConvertTexcoordOrigin, kRaw);
        }

        if (formatDesc.transfer() == KHR_DF_TRANSFER_SRGB) {
            const auto error_message = "Invalid value to --{} \"{}\" for format \"{}\". Transfer function must be sRGB for sRGB formats.";
            if (!convertOETF.has_value() && assignOETF.has_value()) {
                switch (assignOETF.value()) {
                case KHR_DF_TRANSFER_UNSPECIFIED:
                case KHR_DF_TRANSFER_SRGB:
                    // assign-oetf must either not be specified or must be sRGB for an sRGB format
                    break;
                default:
                    report.fatal_usage(error_message, "assign-oetf", args[kAssignOetf].as<std::string>(), args[kFormat].as<std::string>());
                }
            } else if (convertOETF.has_value() && convertOETF != KHR_DF_TRANSFER_SRGB) {
                report.fatal_usage(error_message, "convert-oetf", args[kConvertOetf].as<std::string>(), args[kFormat].as<std::string>());
            }
        }

        if (isFormatNotSRGBButHasSRGBVariant(vkFormat)) {
            const auto error_message = "Invalid value to --{} \"{}\" for format \"{}\". Transfer function must not be sRGB for a non-sRGB VkFormat with sRGB variant.";
            if (!convertOETF.has_value() && assignOETF.has_value() && assignOETF == KHR_DF_TRANSFER_SRGB) {
                report.fatal_usage(error_message, "assign-oetf", args[kAssignOetf].as<std::string>(), args[kFormat].as<std::string>());
            } else if (convertOETF.has_value() && convertOETF == KHR_DF_TRANSFER_SRGB) {
                report.fatal_usage(error_message, "convert-oetf", args[kConvertOetf].as<std::string>(), args[kFormat].as<std::string>());
            }
        }

        if (args[kFailOnColorConversions].count())
            failOnColorConversions = true;

        if (args[kWarnOnColorConversions].count()) {
            if (failOnColorConversions)
                report.fatal_usage("The options --{} and --{} are mutually exclusive.",
                                   kFailOnColorConversions, kWarnOnColorConversions);
            warnOnColorConversions = true;
        }

        if (args[kFailOnOriginChanges].count())
            failOnOriginChanges = true;

        if (args[kWarnOnOriginChanges].count()) {
            if (failOnOriginChanges)
                report.fatal_usage("The options --{} and --{} are mutually exclusive.",
                                   kFailOnOriginChanges, kWarnOnOriginChanges);
            warnOnOriginChanges = true;
        }
    }
};

// -------------------------------------------------------------------------------------------------

/** @page ktx_create ktx create
@~English

Create a KTX2 file from various input files.

@section ktx_create_synopsis SYNOPSIS
    ktx create [option...] @e input-file... @e output-file

@section ktx_create_description DESCRIPTION
    @b ktx @b create can create, encode and supercompress a KTX2 file from the
    input images specified as the @e input-file... arguments and save it as the
    @e output-file. The last positional argument is treated as the @e output-file.
    If the @e input-file is '-' the file will be read from the stdin.
    If the @e output-path is '-' the output file will be written to the stdout.

    Each @e input-file must be a valid EXR (.exr), PNG (.png) or Raw (.raw) file.
    PNG files with luminance (L) or luminance + alpha (LA) data will be converted
    to RGB as LLL and RGBA as LLLA before processing further.
    The input file formats must be compatible with the requested KTX format enum and
    must have at least the same level of precision and number of channels.
    Any unused channel will be discarded silently.

    The number of input-files specified must match the expected number of input images
    based on the used options.

@section ktx\_create\_options OPTIONS
  @subsection ktx\_create\_options\_general General Options
    The following are available:
    <dl>
        <dt>\--format &lt;enum&gt;</dt>
        <dd>KTX format enum that specifies the data format of the images in the created texture.
            The enum names are matching the VkFormats without the VK_FORMAT_ prefix.
            The VK_FORMAT_ prefix is ignored if present. Case insensitive. Required.<br />
            <br />
            If the format is an ASTC format a texture object with the target format
            @c R8G8B8_{SRGB,UNORM} or  @c R8G8B8A8_{SRGB,UNORM} is created
            then encoded to the specified ASTC format. The latter format is chosen if alpha
            is present in the input. @c SRGB or @c UNORM is chosen depending on the
            specified ASTC format. The ASTC-specific and common encoder options listed
            @ref ktx_create_options_encoding "below" become valid, otherwise they are ignored.
            This matches the functionality of the @ref ktx_encode "ktx encode" command
            when an ASTC format is specified.<br />
            <br />
            When used with @b \--encode it specifies the target format before the encoding step.
            In this case it must be one of:
            <ul>
                <li>R8_UNORM</li>
                <li>R8_SRGB</li>
                <li>R8G8_UNORM</li>
                <li>R8G8_SRGB</li>
                <li>R8G8B8_UNORM</li>
                <li>R8G8B8_SRGB</li>
                <li>R8G8B8A8_UNORM</li>
                <li>R8G8B8A8_SRGB</li>
            </ul>
            The format will be used to verify and load all input files into a texture before
            performing any specified encoding.
        </dd>
        <dt>\--encode basis-lz | uastc</dt>
        <dd>Encode the texture with the specified codec before saving it.
            This option matches the functionality of the @ref ktx_encode "ktx encode" command.
            With each choice, the specific and common encoder options listed
            @ref ktx_create_options_encoding "below"  become valid, otherwise they
            are ignored. Case-insensitive.</dd>

            @snippet{doc} ktx/encode_utils_basis.h command options_basis_encoders
        <dt>\--1d</dt>
        <dd>Create a 1D texture. If not set the texture will be a 2D or 3D texture.</dd>
        <dt>\--cubemap</dt>
        <dd>Create a cubemap texture. If not set the texture will be a 2D or 3D texture.</dd>
        <dt>\--raw</dt>
        <dd>Create from raw image data.</dd>
        <dt>\--width</dt>
        <dd>Base level width in pixels.</dd>
        <dt>\--height</dt>
        <dd>Base level height in pixels.</dd>
        <dt>\--depth</dt>
        <dd>Base level depth in pixels.
            If set the texture will be a 3D texture.</dd>
        <dt>\--layers</dt>
        <dd>Number of layers.
            If set the texture will be an array texture.</dd>
        <dt>\--runtime-mipmap</dt>
        <dd>Runtime mipmap generation mode.
            Sets up the texture to request the mipmaps to be generated by the client application at
            runtime.</dd>
        <dt>\--generate-mipmap</dt>
        <dd>Causes mipmaps to be generated during texture creation.
            If the @b \--levels is not specified the maximum possible mip level will be generated.
            This option is mutually exclusive with @b \--runtime-mipmap and cannot be used with SINT,
            UINT or 3D textures.</dd>
            When set it enables the use of the following \'Generate Mipmap\' options.
        <dl>
            <dt>\--mipmap-filter &lt;filter&gt;</dt>
            <dd>Specifies the filter to use when generating the mipmaps. Case insensitive.<br />
                Possible options are:
                box | tent | bell | b-spline | mitchell | blackman | lanczos3 | lanczos4 | lanczos6 |
                lanczos12 | kaiser | gaussian | catmullrom | quadratic_interp | quadratic_approx |
                quadratic_mix.
                Defaults to lanczos4.</dd>
            <dt>\--mipmap-filter-scale &lt;float&gt;</dt>
            <dd>The filter scale to use.
                Defaults to 1.0.</dd>
            <dt>\--mipmap-wrap &lt;mode&gt;</dt>
            <dd>Specify how to sample pixels near the image boundaries. Case insensitive.<br />
                Possible options are:
                wrap | reflect | clamp.
                Defaults to clamp.</dd>
        </dl>
        <dt>\--normalize</dt>
        <dd>Normalize input normals to have a unit length. Only valid for
                linear normal textures with 2 or more components. For 2-component inputs
                2D unit normals are calculated. Do not use these 2D unit normals
                to generate X+Y normals with @b --normal-mode. For 4-component inputs
                a 3D unit normal is calculated. 1.0 is used for the value of the
                4th component.</dd>
        <dt>\--swizzle [rgba01]{4}</dt>
        <dd>KTX swizzle metadata.</dd>
        <dt>\--input-swizzle [rgba01]{4}</dt>
        <dd>Pre-swizzle input channels.</dd>
        <dt>\--assign-oetf &lt;oetf&gt;</dt>
        <dd>Force the created texture to have the specified transfer function, ignoring
            the transfer function of the input file(s). Case insensitive.
            Possible options are:
            linear | srgb
            </dd>
        <dt>\--assign-primaries &lt;primaries&gt;</dt>
        <dd>Force the created texture to have the specified color primaries, ignoring
            the color primaries of the input file(s). Case insensitive.
            Possible options are:
            none | bt709 | srgb | bt601-ebu | bt601-smpte | bt2020 | ciexyz | aces | acescc |
            ntsc1953 | pal525 | displayp3 | adobergb
            </dd>
        <dt>\--assign-texcoord-origin &lt;corner&gt;</dt>
        <dd>Force the created texture to indicate that the texture coordinate
            origin s=0, t=0 is at the specified @em corner of the logical image.
            Case insensitive. Possible options are top-left | bottom-left.
            -front | -back can be appended and one of these is required when
            @b \--depth is specified. Must be top-left if @b \--cubemap is
            specified. Absent @b —convert-texcoord-origin, the effect of this
            option is to cause @e KTXorientation metadata indicating the
            specified origin to be written to the output file. Example values
            are "rd" (top-left) and "ru" (bottom-left) or, when @b \--depth is
            specified, "rdi" (top-left-front) and "rui" (bottom-left-front).
            </dd>
        <dt>\--convert-oetf &lt;oetf&gt;</dt>
        <dd>Convert the input image(s) to the specified transfer function, if different
            from the transfer function of the input file(s). If both this and @b \--assign-oetf are
            specified, conversion will be performed from the assigned transfer function to the
            transfer function specified by this option, if different. Cannot be
            used with @b \--raw. Case insensitive.
            Possible options are: linear | srgb
            </dd>
        <dt>\--convert-primaries &lt;primaries&gt;</dt>
        <dd>Convert the input image(s) to the specified color primaries, if different
            from the color primaries of the input file(s) or the one specified by @b \--assign-primaries.
            If both this and @b \--assign-primaries are specified, conversion will be performed from
            the assigned primaries to the primaries specified by this option, if different.
            This option is not allowed to be specified when @b \--assign-primaries is set to 'none'.
            Cannot be used with @b \--raw. Case insensitive.
            Possible options are:
            bt709 | srgb | bt601-ebu | bt601-smpte | bt2020 | ciexyz | aces | acescc | ntsc1953 |
            pal525 | displayp3 | adobergb
            </dd>
        <dt>\--convert-texcoord-origin &lt;corner&gt;</dt>
        <dd>Convert the input image(s) so the texture coordinate origin s=0,
            t=0, is at the specified @em corner of the logical image. If both
            this and @b \--assign-texcoord-origin are specified, conversion will
            be performed from the assigned origin to the origin specified by
            this option, if different. The default for images in KTX files is
            top-left which corresponds to the origin in most image file
            formats. Cannot be used with @b \--raw. Case insensitive.
            Possible options are: top-left | bottom-left. -front | -back can be
            appended and one of these is required when @b \--depth is specified.
            Must be top-left if @b \--cubemap is specified.<br />
            <br />
            Input images whose origin does not match @em corner will be flipped
            vertically. @e KTXorientation metadata indicating the
            the specified origin is written to the output file. Example values
            are "rd" (top-left) and "ru" (bottom-left) or, when @b \--depth is
            specified, "rdi" (top-left-front) and "rui" (bottom-left-back).
            An error is raised if the input image origin is unknown as is the
            case with raw image data.  Use @b --assign-texcoord-origin to
            specify the orientation.
            @note ktx create cannot rotate or flip incoming images, except
            for a y-flip, so use an an image processing tool to reorient images
            whose first data stream pixel is not at the logical top-left or
            bottom-left of the image before using as input here. Such images
            may be indicated by Exif-style orientation metadata in the file.
            </dd>
        <dt>\--fail-on-color-conversions</dt>
        <dd>Generates an error if any of the input images would need to be
            color converted.</dd>
        <dt>\--warn-on-color-conversions</dt>
        <dd>Generates a warning if any of the input images are color
            converted.</dd>
        <dt>\--fail-on-origin-changes</dt>
        <dd>Generates an error if any of the input images would need to have
            their origin changed.</dd>
        <dt>\--warn-on-origin-changes</dt>
        <dd>Generates a warning if any of the input images have their origin
            changed..</dd>
    </dl>
    @snippet{doc} ktx/deflate_utils.h command options_deflate
    @snippet{doc} ktx/command.h command options_generic

  @subsection ktx\_create\_options\_encoding Specific and Common Encoding Options
    The following are available. Specific options become valid only if their encoder has been
    selected. Common encoder options become valid when an encoder they apply to has
    been selected. Otherwise they are ignored.
    @snippet{doc} ktx/encode_utils_astc.h command options_encode_astc
    @snippet{doc} ktx/encode_utils_basis.h command options_encode_basis
    @snippet{doc} ktx/encode_utils_common.h command options_encode_common
    @snippet{doc} ktx/metrics_utils.h command options_metrics

@section ktx_create_exitstatus EXIT STATUS
    @snippet{doc} ktx/command.h command exitstatus

@section ktx_create_history HISTORY

@par Version 4.0
 - Initial version

@par Version 4.4
 - Reorganize encoding options.
 - Improve explanation of use of @b \--format with @b \--encode.
 - Improve explanation of ASTC encoding.

@section ktx_create_author AUTHOR
    - Mátyás Császár [Vader], RasterGrid www.rastergrid.com
    - Daniel Rákos, RasterGrid www.rastergrid.com
*/
class CommandCreate : public Command {
private:
    Combine<OptionsCreate, OptionsEncodeASTC, OptionsEncodeBasis<false>, OptionsEncodeCommon, OptionsMetrics, OptionsDeflate, OptionsMultiInSingleOut, OptionsGeneric> options;

    uint32_t targetChannelCount = 0; // Derived from VkFormat

    uint32_t numLevels = 0;
    uint32_t numLayers = 0;
    uint32_t numFaces = 0;
    uint32_t baseDepth = 0;

public:
    virtual int main(int argc, char* argv[]) override;
    virtual void initOptions(cxxopts::Options& opts) override;
    virtual void processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) override;

private:
    void executeCreate();
    void encodeBasis(KTXTexture2& texture, OptionsEncodeBasis<false>& opts);
    void encodeASTC(KTXTexture2& texture, OptionsEncodeASTC& opts);
    void compress(KTXTexture2& texture, const OptionsDeflate& opts);

private:
    template <typename F>
    void foreachImage(const FormatDescriptor& format, F&& func);

    [[nodiscard]] KTXTexture2 createTexture(const ImageSpec& target);
    void generateMipLevels(KTXTexture2& texture, std::unique_ptr<Image> image, ImageInput& inputFile,
            uint32_t numMipLevels, uint32_t layerIndex, uint32_t faceIndex, uint32_t depthSliceIndex);

    [[nodiscard]] std::string readRawFile(const std::filesystem::path& filepath);
    [[nodiscard]] std::unique_ptr<Image> loadInputImage(ImageInput& inputImageFile);
    std::vector<uint8_t> convert(const std::unique_ptr<Image>& image, VkFormat format, ImageInput& inputFile);

    std::unique_ptr<const ColorPrimaries> createColorPrimaries(khr_df_primaries_e primaries) const;

    void selectASTCMode(uint32_t bitLength);
    void determineTargetColorSpace(const ImageInput& in, ImageSpec& target, ColorSpaceInfo& colorSpaceInfo);
    void determineTargetOrigin(const ImageInput& in, ImageSpec& target, ImageSpec::Origin& usedInputOrigin);

    void checkSpecsMatch(const ImageInput& current, const ImageSpec& firstSpec);
};

// -------------------------------------------------------------------------------------------------

int CommandCreate::main(int argc, char* argv[]) {
    try {
        parseCommandLine("ktx create",
                "Create, encode and supercompress a KTX2 file from the input images specified as the\n"
                "    input-file... arguments and save it as the output-file.",
                argc, argv);
        executeCreate();
        return +rc::SUCCESS;
    } catch (const FatalError& error) {
        return +error.returnCode;
    } catch (const std::exception& e) {
        fmt::print(std::cerr, "{} fatal: {}\n", commandName, e.what());
        return +rc::RUNTIME_ERROR;
    }
}

void CommandCreate::initOptions(cxxopts::Options& opts) {
    options.init(opts);
}

void CommandCreate::processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) {
    options.process(opts, args, *this);

    numLevels = options.levels.value_or(1);
    numLayers = options.layers.value_or(1);
    numFaces = options.cubemap ? 6 : 1;
    baseDepth = options.depth.value_or(1u);

    const auto blockSizeZ = isFormat3DBlockCompressed(options.vkFormat) ?
            createFormatDescriptor(options.vkFormat, *this).basic.texelBlockDimension2 + 1u : 1u;
    uint32_t expectedInputImages = 0;
    for (uint32_t i = 0; i < (options.mipmapGenerate ? 1 : numLevels); ++i)
        // If --generate-mipmap is set the input only contains the base level images
        expectedInputImages += numLayers * numFaces * ceil_div(std::max(baseDepth >> i, 1u), blockSizeZ);
    if (options.inputFilepaths.size() != expectedInputImages) {
        fatal_usage("Too {} input image for {} level{}, {} layer, {} face and {} depth. Provided {} but expected {}.",
                options.inputFilepaths.size() > expectedInputImages ? "many" : "few",
                numLevels,
                options.mipmapGenerate ? " (mips generated)" : "",
                numLayers,
                numFaces,
                baseDepth,
                options.inputFilepaths.size(), expectedInputImages);
    }

    if (!isFormatAstc(options.vkFormat)) {
        for (const char* astcOption : OptionsEncodeASTC::kAstcOptions)
            if (args[astcOption].count())
                fatal_usage("--{} can only be used with ASTC formats.", astcOption);
    } else {
        fillOptionsCodecAstc<decltype(options)>(options);
        if (options.OptionsEncodeCommon::noSSE)
            fatal_usage("--{} is not allowed with ASTC encode", OptionsEncodeCommon::kNoSse);
    }

    if (options.codec == BasisCodec::BasisLZ) {
        if (options.zstd.has_value())
            fatal_usage("Cannot encode to BasisLZ and supercompress with Zstd.");

        if (options.zlib.has_value())
            fatal_usage("Cannot encode to BasisLZ and supercompress with ZLIB.");
    }

    if (options.codec != BasisCodec::NONE) {
        switch (options.vkFormat) {
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_SRGB:
        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_SRGB:
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SRGB:
            // Allowed formats
            break;
        default:
            fatal_usage("Only R8, RG8, RGB8, or RGBA8 UNORM and SRGB formats can be encoded, "
                "but format is {}.", toString(VkFormat(options.vkFormat)));
            break;
        }
    }

    const auto basisCodec = options.codec == BasisCodec::BasisLZ || options.codec == BasisCodec::UASTC;
    const auto astcCodec = isFormatAstc(options.vkFormat);
    const auto canCompare = basisCodec || astcCodec;

    if (basisCodec)
        fillOptionsCodecBasis<decltype(options)>(options);

    if (options.compare_ssim && !canCompare)
        fatal_usage("--compare-ssim can only be used with BasisLZ, UASTC or ASTC encoding.");
    if (options.compare_psnr && !canCompare)
        fatal_usage("--compare-psnr can only be used with BasisLZ, UASTC or ASTC encoding.");

    if (isFormatAstc(options.vkFormat) && !options.raw) {
        options.encodeASTC = true;

        switch (options.vkFormat) {
        case VK_FORMAT_ASTC_4x4_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
            options.mode = KTX_PACK_ASTC_ENCODER_MODE_LDR;
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_4x4;
            break;
        case VK_FORMAT_ASTC_5x4_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
            options.mode = KTX_PACK_ASTC_ENCODER_MODE_LDR;
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_5x4;
            break;
        case VK_FORMAT_ASTC_5x5_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
            options.mode = KTX_PACK_ASTC_ENCODER_MODE_LDR;
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_5x5;
            break;
        case VK_FORMAT_ASTC_6x5_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
            options.mode = KTX_PACK_ASTC_ENCODER_MODE_LDR;
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_6x5;
            break;
        case VK_FORMAT_ASTC_6x6_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
            options.mode = KTX_PACK_ASTC_ENCODER_MODE_LDR;
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_6x6;
            break;
        case VK_FORMAT_ASTC_8x5_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
            options.mode = KTX_PACK_ASTC_ENCODER_MODE_LDR;
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_8x5;
            break;
        case VK_FORMAT_ASTC_8x6_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
            options.mode = KTX_PACK_ASTC_ENCODER_MODE_LDR;
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_8x6;
            break;
        case VK_FORMAT_ASTC_8x8_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
            options.mode = KTX_PACK_ASTC_ENCODER_MODE_LDR;
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_8x8;
            break;
        case VK_FORMAT_ASTC_10x5_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
            options.mode = KTX_PACK_ASTC_ENCODER_MODE_LDR;
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_10x5;
            break;
        case VK_FORMAT_ASTC_10x6_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
            options.mode = KTX_PACK_ASTC_ENCODER_MODE_LDR;
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_10x6;
            break;
        case VK_FORMAT_ASTC_10x8_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
            options.mode = KTX_PACK_ASTC_ENCODER_MODE_LDR;
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_10x8;
            break;
        case VK_FORMAT_ASTC_10x10_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
            options.mode = KTX_PACK_ASTC_ENCODER_MODE_LDR;
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_10x10;
            break;
        case VK_FORMAT_ASTC_12x10_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
            options.mode = KTX_PACK_ASTC_ENCODER_MODE_LDR;
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_12x10;
            break;
        case VK_FORMAT_ASTC_12x12_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
            options.mode = KTX_PACK_ASTC_ENCODER_MODE_LDR;
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_12x12;
            break;
        default:
            fatal(rc::NOT_SUPPORTED, "{} is unsupported for ASTC encoding.", toString(options.vkFormat));
            break;
        }
    }

    if (options._1d && options.encodeASTC)
        fatal_usage("ASTC format {} cannot be used for 1 dimensional textures (indicated by --1d).",
                toString(options.vkFormat));
}

template <typename F>
void CommandCreate::foreachImage(const FormatDescriptor& format, F&& func) {
    // Input file ordering is specified as the same order as the
    // "levelImages" structure in the KTX 2.0 specification:
    //      level > layer > face > image (z_slice_of_blocks)
    //      aka foreach level, foreach layer, foreach face, foreach image (z_slice_of_blocks)

    auto inputFileIt = options.inputFilepaths.begin();

    for (uint32_t levelIndex = 0; levelIndex < (options.mipmapGenerate ? 1 : numLevels); ++levelIndex) {
        const auto numDepthSlices = ceil_div(std::max(baseDepth >> levelIndex, 1u), format.basic.texelBlockDimension2 + 1u);
        for (uint32_t layerIndex = 0; layerIndex < numLayers; ++layerIndex) {
            for (uint32_t faceIndex = 0; faceIndex < numFaces; ++faceIndex) {
                for (uint32_t depthSliceIndex = 0; depthSliceIndex < numDepthSlices; ++depthSliceIndex) {
                    assert(inputFileIt != options.inputFilepaths.end() && "Internal error"); // inputFilepaths size was already validated during arg parsing
                    func(*inputFileIt++, levelIndex, layerIndex, faceIndex, depthSliceIndex);
                }
            }
        }
    }
    assert(inputFileIt == options.inputFilepaths.end() && "Internal error"); // inputFilepaths size was already validated during arg parsing
}

std::string CommandCreate::readRawFile(const std::filesystem::path& filepath) {
    std::string result;
    InputStream inputStream(filepath.string(), *this);

    inputStream->seekg(0, std::ios::end);
    if (inputStream->fail())
        fatal(rc::IO_FAILURE, "Failed to seek file \"{}\": {}.", filepath.generic_string(), errnoMessage());

    const auto size = inputStream->tellg();
    inputStream->seekg(0);
    if (inputStream->fail())
        fatal(rc::IO_FAILURE, "Failed to seek file \"{}\": {}.", filepath.generic_string(), errnoMessage());

    result.resize(size);
    inputStream->read(result.data(), size);
    if (inputStream->fail())
        fatal(rc::IO_FAILURE, "Failed to read file \"{}\": {}.", filepath.generic_string(), errnoMessage());

    return result;
}

void CommandCreate::executeCreate() {
    const auto warningFn = [this](const std::string& w) { this->warning(w); };

    KTXTexture2 texture{nullptr};
    targetChannelCount = options.formatDesc.channelCount();

    ImageSpec target;

    bool firstImage = true;
    ImageSpec firstImageSpec{};

    foreachImage(options.formatDesc, [&](
            const auto& inputFilepath,
            uint32_t levelIndex,
            uint32_t layerIndex,
            uint32_t faceIndex,
            uint32_t depthSliceIndex) {

        if (options.raw) {
            if (std::exchange(firstImage, false)) {
                target = ImageSpec{
                        options.width.value_or(1u),
                        options.height.value_or(1u),
                        options.depth.value_or(1u),
                        options.formatDesc};

                if (options.cubemap && target.width() != target.height())
                    fatal(rc::INVALID_FILE, "--cubemap specified but the input image \"{}\" with size {}x{} is not square.",
                            fmtInFile(inputFilepath), target.width(), target.height());

                if (options.assignOETF.has_value())
                    target.format().setTransfer(options.assignOETF.value());

                if (options.assignPrimaries.has_value())
                    target.format().setPrimaries(options.assignPrimaries.value());

                if (options.assignTexcoordOrigin.has_value())
                    target.setOrigin(options.assignTexcoordOrigin.value());

                texture = createTexture(target);
            }

            const auto rawData = readRawFile(inputFilepath);

            const auto expectedFileSize = ktxTexture_GetImageSize(texture, levelIndex);
            if (rawData.size() != expectedFileSize)
                fatal(rc::INVALID_FILE, "Raw input file \"{}\" with {} bytes for level {} does not match the expected size of {} bytes.",
                        fmtInFile(inputFilepath), rawData.size(), levelIndex, expectedFileSize);

            const auto ret = ktxTexture_SetImageFromMemory(
                    texture,
                    levelIndex,
                    layerIndex,
                    faceIndex + depthSliceIndex, // Faces and Depths are mutually exclusive, Addition is acceptable
                    reinterpret_cast<const ktx_uint8_t*>(rawData.data()),
                    rawData.size());
            assert(ret == KTX_SUCCESS && "Internal error"); (void) ret;
        } else {
            const auto inputImageFile = ImageInput::open(inputFilepath, nullptr, warningFn);
            inputImageFile->seekSubimage(0, 0); // Loading multiple subimage from the same input is not supported

            target = ImageSpec{
                    inputImageFile->spec().width(),
                    inputImageFile->spec().height(),
                    options.depth.value_or(1u),
                    options.formatDesc};

            ColorSpaceInfo colorSpaceInfo{};
            ImageSpec::Origin usedInputOrigin;
            determineTargetColorSpace(*inputImageFile, target, colorSpaceInfo);
            determineTargetOrigin(*inputImageFile, target, usedInputOrigin);

            if (std::exchange(firstImage, false)) {
                if (options.cubemap && target.width() != target.height())
                    fatal(rc::INVALID_FILE, "--cubemap specified but the input image \"{}\" with size {}x{} is not square.",
                            fmtInFile(inputFilepath), target.width(), target.height());

                if (options._1d && target.height() != 1)
                    fatal(rc::INVALID_FILE, "For --1d textures the input image height must be 1, but for \"{}\" it was {}.",
                            fmtInFile(inputFilepath), target.height());

                const auto maxDimension = std::max(target.width(), std::max(target.height(), baseDepth));
                const auto maxLevels = log2(maxDimension) + 1;
                if (options.levels.value_or(1) > maxLevels)
                    fatal_usage("Requested {} levels is too many. With input image \"{}\" sized {}x{} and depth {} the texture can only have {} levels at most.",
                            options.levels.value_or(1), fmtInFile(inputFilepath), target.width(), target.height(), baseDepth, maxLevels);

                if (options.encodeASTC)
                    selectASTCMode(inputImageFile->spec().format().largestChannelBitLength());

                firstImageSpec = inputImageFile->spec();
                texture = createTexture(target);
            } else {
                checkSpecsMatch(*inputImageFile, firstImageSpec);
            }

            const uint32_t imageWidth = std::max(firstImageSpec.width() >> levelIndex, 1u);
            const uint32_t imageHeight = std::max(firstImageSpec.height() >> levelIndex, 1u);

            if (inputImageFile->spec().width() != imageWidth || inputImageFile->spec().height() != imageHeight)
                fatal(rc::INVALID_FILE, "Input image \"{}\" with size {}x{} does not match expected size {}x{} for level {}.",
                        fmtInFile(inputFilepath), inputImageFile->spec().width(), inputImageFile->spec().height(), imageWidth, imageHeight, levelIndex);

            auto image = loadInputImage(*inputImageFile);

            if (colorSpaceInfo.dstTransferFunction != nullptr) {
                assert(colorSpaceInfo.srcTransferFunction != nullptr);
                if (colorSpaceInfo.dstColorPrimaries != nullptr) {
                    assert(colorSpaceInfo.srcColorPrimaries != nullptr);
                    auto primaryTransform = colorSpaceInfo.srcColorPrimaries->transformTo(*colorSpaceInfo.dstColorPrimaries);

                    if (options.failOnColorConversions)
                        fatal(rc::INVALID_FILE,
                            "Input file \"{}\" would need color conversion as input and output primaries are different. "
                            "Use --assign-primaries and do not use --convert-primaries to avoid unwanted color conversions.",
                            fmtInFile(inputFilepath));

                    if (options.warnOnColorConversions)
                        warning("Input file \"{}\" is color converted as input and output primaries are different. "
                            "Use --assign-primaries and do not use --convert-primaries to avoid unwanted color conversions.",
                            fmtInFile(inputFilepath));

                    // Transform OETF with primary transform
                    image->transformColorSpace(*colorSpaceInfo.srcTransferFunction, *colorSpaceInfo.dstTransferFunction, &primaryTransform);
                } else {
                    if (options.failOnColorConversions)
                        fatal(rc::INVALID_FILE,
                            "Input file \"{}\" would need color conversion as input and output transfer functions are different. "
                            "Use --assign-oetf and do not use --convert-oetf to avoid unwanted color conversions.",
                            fmtInFile(inputFilepath));

                    if (options.warnOnColorConversions)
                        warning("Input file \"{}\" is color converted as input and output transfer functions are different. "
                            "Use --assign-oetf and do not use --convert-oetf to avoid unwanted color conversions.",
                            fmtInFile(inputFilepath));

                    // Transform OETF without primary transform
                    image->transformColorSpace(*colorSpaceInfo.srcTransferFunction, *colorSpaceInfo.dstTransferFunction);
                }
            }

            // TODO: Add auto conversion and warning? Not needed now
            // because all supported source formats provide top-left images.

            if (target.origin() != usedInputOrigin) {
                    if (options.failOnOriginChanges)
                        fatal(rc::INVALID_FILE,
                            "Input file \"{}\" would need to be y-flipped as input and output origins are different. "
                            "Use --{} and do not use --{} to avoid unwanted color conversions.",
                            fmtInFile(inputFilepath), OptionsCreate::kAssignTexcoordOrigin,
                            OptionsCreate::kConvertTexcoordOrigin);

                    if (options.warnOnOriginChanges)
                        warning("Input file \"{}\" is y-flipped as input and output origins are different. "
                            "Use --{} and do not use --{} to avoid unwanted color conversions.",
                            fmtInFile(inputFilepath), OptionsCreate::kAssignTexcoordOrigin,
                            OptionsCreate::kConvertTexcoordOrigin);

                // Only difference allowed by CLI is y down or y up.
                image->yflip();
            }

            if (options.normalize) {
                if (colorSpaceInfo.usedInputTransferFunction != KHR_DF_TRANSFER_LINEAR) {
                    fatal(rc::INVALID_FILE,
                        "Input file \"{}\" transfer functions is not linear. "
                        "Use --assign-oetf=linear or --convert-oetf=linear to avoid this error.",
                        fmtInFile(inputFilepath));
                    }
                image->normalize();
            }

            if (options.swizzleInput)
                image->swizzle(*options.swizzleInput);

            const auto imageData = convert(image, options.vkFormat, *inputImageFile);

            const auto ret = ktxTexture_SetImageFromMemory(
                    texture,
                    levelIndex,
                    layerIndex,
                    faceIndex + depthSliceIndex, // Faces and Depths are mutually exclusive, Addition is acceptable
                    imageData.data(),
                    imageData.size());
            assert(ret == KTX_SUCCESS && "Internal error"); (void) ret;

            if (options.mipmapGenerate) {
                const auto maxDimension = std::max(target.width(), std::max(target.height(), baseDepth));
                const auto maxLevels = log2(maxDimension) + 1;
                uint32_t numMipLevels = options.levels.value_or(maxLevels);
                generateMipLevels(texture, std::move(image), *inputImageFile, numMipLevels, layerIndex, faceIndex, depthSliceIndex);
            }
        }
    });

    // Add KTXwriter metadata
    const auto writer = fmt::format("{} {}", commandName, version(options.testrun));
    ktxHashList_AddKVPair(&texture->kvDataHead, KTX_WRITER_KEY,
            static_cast<uint32_t>(writer.size() + 1), // +1 to include the \0
            writer.c_str());

    // Add KTXswizzle metadata
    if (options.swizzle) {
        ktxHashList_AddKVPair(&texture->kvDataHead, KTX_SWIZZLE_KEY,
                static_cast<uint32_t>(options.swizzle->size() + 1), // +1 to include the \0
                options.swizzle->c_str());
    }

    // Encode and apply compression

    MetricsCalculator metrics;
    metrics.saveReferenceImages(texture, options, *this);

    if (options.codec != BasisCodec::NONE)
        encodeBasis(texture, options);
    if (options.encodeASTC)
        encodeASTC(texture, options);

    metrics.decodeAndCalculateMetrics(texture, options, *this);

    compress(texture, options);

    // Add KTXwriterScParams metadata if ASTC encoding, BasisU encoding, or other supercompression was used
    const auto writerScParams = fmt::format("{}{}{}{}", options.astcOptions, options.codecOptions, options.commonOptions, options.compressOptions);
    if (writerScParams.size() > 0) {
        // Options always contain a leading space
        assert(writerScParams[0] == ' ');
        ktxHashList_AddKVPair(&texture->kvDataHead, KTX_WRITER_SCPARAMS_KEY,
            static_cast<uint32_t>(writerScParams.size()),
            writerScParams.c_str() + 1); // +1 to exclude leading space
    }

    // Save output file
    const auto outputPath = std::filesystem::path(DecodeUTF8Path(options.outputFilepath));
    if (outputPath.has_parent_path())
        std::filesystem::create_directories(outputPath.parent_path());

    OutputStream outputFile(options.outputFilepath, *this);
    outputFile.writeKTX2(texture, *this);
}

// -------------------------------------------------------------------------------------------------

void CommandCreate::encodeBasis(KTXTexture2& texture, OptionsEncodeBasis<false>& opts) {
    auto ret = ktxTexture2_CompressBasisEx(texture, &opts);
    if (ret != KTX_SUCCESS)
        fatal(rc::KTX_FAILURE, "Failed to encode KTX2 file with codec \"{}\". KTX Error: {}",
                to_underlying(opts.codec), ktxErrorString(ret));
}

void CommandCreate::encodeASTC(KTXTexture2& texture, OptionsEncodeASTC& opts) {
    const auto ret = ktxTexture2_CompressAstcEx(texture, &opts);
    if (ret != KTX_SUCCESS)
        fatal(rc::KTX_FAILURE, "Failed to encode KTX2 file with codec ASTC. KTX Error: {}", ktxErrorString(ret));
}

void CommandCreate::compress(KTXTexture2& texture, const OptionsDeflate& opts) {
    if (opts.zstd) {
        const auto ret = ktxTexture2_DeflateZstd(texture, *opts.zstd);
        if (ret != KTX_SUCCESS)
            fatal(rc::KTX_FAILURE, "Zstd deflation failed. KTX Error: {}", ktxErrorString(ret));
    }

    if (opts.zlib) {
        const auto ret = ktxTexture2_DeflateZLIB(texture, *opts.zlib);
        if (ret != KTX_SUCCESS)
            fatal(rc::KTX_FAILURE, "ZLIB deflation failed. KTX Error: {}", ktxErrorString(ret));
    }
}

// -------------------------------------------------------------------------------------------------

std::unique_ptr<Image> CommandCreate::loadInputImage(ImageInput& inputImageFile) {
    std::unique_ptr<Image> image = nullptr;

    const auto& inputFormat = inputImageFile.spec().format();
    const auto width = inputImageFile.spec().width();
    const auto height = inputImageFile.spec().height();

    const auto inputBitLength = inputFormat.largestChannelBitLength();
    const auto requestBitLength = std::max(imageio::bit_ceil(inputBitLength), 8u);
    FormatDescriptor loadFormat;

    switch (inputImageFile.formatType()) {
    case ImageInputFormatType::exr_uint:
        image = std::make_unique<rgba32image>(width, height);
        loadFormat = createFormatDescriptor(VK_FORMAT_R32G32B32A32_UINT, *this);
        break;

    case ImageInputFormatType::exr_float:
        image = std::make_unique<rgba32fimage>(width, height);
        loadFormat = createFormatDescriptor(VK_FORMAT_R32G32B32A32_SFLOAT, *this);
        break;

    case ImageInputFormatType::npbm: [[fallthrough]];
    case ImageInputFormatType::jpg: [[fallthrough]];
    case ImageInputFormatType::png_l: [[fallthrough]];
    case ImageInputFormatType::png_la: [[fallthrough]];
    case ImageInputFormatType::png_rgb: [[fallthrough]];
    case ImageInputFormatType::png_rgba:
        if (requestBitLength == 8) {
            image = std::make_unique<rgba8image>(width, height);
            loadFormat = createFormatDescriptor(VK_FORMAT_R8G8B8A8_UNORM, *this);
            break;
        } else if (requestBitLength == 16) {
            image = std::make_unique<rgba16image>(width, height);
            loadFormat = createFormatDescriptor(VK_FORMAT_R16G16B16A16_UNORM, *this);
            break;
        } else {
            fatal(rc::INVALID_FILE, "Unsupported format with {}-bit channels.", requestBitLength);
        }
        break;
    }

    inputImageFile.readImage(static_cast<uint8_t*>(*image), image->getByteCount(), 0, 0, loadFormat);
    return image;
}

std::vector<uint8_t> convertUNORMPacked(const std::unique_ptr<Image>& image, uint32_t C0, uint32_t C1, uint32_t C2, uint32_t C3, std::string_view swizzle = "") {
    if (!swizzle.empty())
        image->swizzle(swizzle);

    return image->getUNORMPacked(C0, C1, C2, C3);
}

template <typename T>
std::vector<uint8_t> convertUNORM(const std::unique_ptr<Image>& image, std::string_view swizzle = "") {
    using ComponentT = typename T::Color::value_type;
    static constexpr auto componentCount = T::Color::getComponentCount();
    static constexpr auto bytesPerComponent = sizeof(ComponentT);
    static constexpr auto bits = bytesPerComponent * 8;

    if (!swizzle.empty())
        image->swizzle(swizzle);

    return image->getUNORM(componentCount, bits);
}

template <typename T>
std::vector<uint8_t> convertUNORMSBits(const std::unique_ptr<Image>& image, uint32_t sBits, std::string_view swizzle = "") {
    using ComponentT = typename T::Color::value_type;
    static constexpr auto componentCount = T::Color::getComponentCount();
    static constexpr auto bytesPerComponent = sizeof(ComponentT);
    static constexpr auto bits = bytesPerComponent * 8;

    if (!swizzle.empty())
        image->swizzle(swizzle);

    return image->getUNORM(componentCount, bits, sBits);
}

template <typename T>
std::vector<uint8_t> convertSFLOAT(const std::unique_ptr<Image>& image, std::string_view swizzle = "") {
    using ComponentT = typename T::Color::value_type;
    static constexpr auto componentCount = T::Color::getComponentCount();
    static constexpr auto bytesPerComponent = sizeof(ComponentT);
    static constexpr auto bits = bytesPerComponent * 8;

    if (!swizzle.empty())
        image->swizzle(swizzle);

    return image->getSFloat(componentCount, bits);
}

std::vector<uint8_t> convertB10G11R11(const std::unique_ptr<Image>& image) {
    return image->getB10G11R11();
}

std::vector<uint8_t> convertE5B9G9R9(const std::unique_ptr<Image>& image) {
    return image->getE5B9G9R9();
}

template <typename T>
std::vector<uint8_t> convertUINT(const std::unique_ptr<Image>& image, std::string_view swizzle = "") {
    using ComponentT = typename T::Color::value_type;
    static constexpr auto componentCount = T::Color::getComponentCount();
    static constexpr auto bytesPerComponent = sizeof(ComponentT);
    static constexpr auto bits = bytesPerComponent * 8;

    if (!swizzle.empty())
        image->swizzle(swizzle);

    return image->getUINT(componentCount, bits);
}

std::vector<uint8_t> convertUINTPacked(const std::unique_ptr<Image>& image,
        uint32_t c0 = 0, uint32_t c1 = 0, uint32_t c2 = 0, uint32_t c3 = 0,
        std::string_view swizzle = "") {

    if (!swizzle.empty())
        image->swizzle(swizzle);

    return image->getUINTPacked(c0, c1, c2, c3);
}

std::vector<uint8_t> convertSINTPacked(const std::unique_ptr<Image>& image,
        uint32_t c0 = 0, uint32_t c1 = 0, uint32_t c2 = 0, uint32_t c3 = 0,
        std::string_view swizzle = "") {

    if (!swizzle.empty())
        image->swizzle(swizzle);

    return image->getSINTPacked(c0, c1, c2, c3);
}

template <typename T>
std::vector<uint8_t> convertSINT(const std::unique_ptr<Image>& image, std::string_view swizzle = "") {
    using ComponentT = typename T::Color::value_type;
    static constexpr auto componentCount = T::Color::getComponentCount();
    static constexpr auto bytesPerComponent = sizeof(ComponentT);
    static constexpr auto bits = bytesPerComponent * 8;

    if (!swizzle.empty())
        image->swizzle(swizzle);

    return image->getSINT(componentCount, bits);
}

std::vector<uint8_t> CommandCreate::convert(const std::unique_ptr<Image>& image, VkFormat vkFormat,
        ImageInput& inputFile) {

    const uint32_t inputBitDepth = std::max(8u, inputFile.spec().format().largestChannelBitLength());

    const auto require = [&](uint32_t bitDepth) {
        if (inputBitDepth < bitDepth)
            fatal(rc::INVALID_FILE, "{}: Not enough precision to convert {} bit input to {} bit output for {}.",
                    inputFile.filename(), inputBitDepth, bitDepth, toString(vkFormat));
        if (inputBitDepth > imageio::bit_ceil(bitDepth))
            warning("{}: Possible loss of precision with converting {} bit input to {} bit output for {}.",
                    inputFile.filename(), inputBitDepth, bitDepth, toString(vkFormat));
    };
    const auto requireUNORM = [&](uint32_t bitDepth) {
        switch (inputFile.formatType()) {
        case ImageInputFormatType::png_l: [[fallthrough]];
        case ImageInputFormatType::png_la: [[fallthrough]];
        case ImageInputFormatType::png_rgb: [[fallthrough]];
        case ImageInputFormatType::png_rgba: [[fallthrough]];
        case ImageInputFormatType::npbm: [[fallthrough]];
        case ImageInputFormatType::jpg:
            break; // Accept
        case ImageInputFormatType::exr_uint: [[fallthrough]];
        case ImageInputFormatType::exr_float:
            fatal(rc::INVALID_FILE, "{}: Input file data type \"{}\" does not match the expected input data type of {} bit \"{}\" for {}.",
                    inputFile.filename(), toString(inputFile.formatType()), bitDepth, "UNORM", toString(vkFormat));
        }
        require(bitDepth);
    };
    const auto requireSFloat = [&](uint32_t bitDepth) {
        switch (inputFile.formatType()) {
        case ImageInputFormatType::exr_float:
            break; // Accept
        case ImageInputFormatType::png_l: [[fallthrough]];
        case ImageInputFormatType::png_la: [[fallthrough]];
        case ImageInputFormatType::png_rgb: [[fallthrough]];
        case ImageInputFormatType::png_rgba: [[fallthrough]];
        case ImageInputFormatType::npbm: [[fallthrough]];
        case ImageInputFormatType::jpg: [[fallthrough]];
        case ImageInputFormatType::exr_uint:
            fatal(rc::INVALID_FILE, "{}: Input file data type \"{}\" does not match the expected input data type of {} bit \"{}\" for {}.",
                    inputFile.filename(), toString(inputFile.formatType()), bitDepth, "SFLOAT", toString(vkFormat));
        }
        require(bitDepth);
    };
    const auto requireUINT = [&](uint32_t bitDepth) {
        switch (inputFile.formatType()) {
        case ImageInputFormatType::exr_uint:
            break; // Accept
        case ImageInputFormatType::png_l: [[fallthrough]];
        case ImageInputFormatType::png_la: [[fallthrough]];
        case ImageInputFormatType::png_rgb: [[fallthrough]];
        case ImageInputFormatType::png_rgba: [[fallthrough]];
        case ImageInputFormatType::npbm: [[fallthrough]];
        case ImageInputFormatType::jpg: [[fallthrough]];
        case ImageInputFormatType::exr_float:
            fatal(rc::INVALID_FILE, "{}: Input file data type \"{}\" does not match the expected input data type of {} bit \"{}\" for {}.",
                    inputFile.filename(), toString(inputFile.formatType()), bitDepth, "UINT", toString(vkFormat));
        }
        require(bitDepth);
    };

    // ------------

    switch (vkFormat) {
    // PNG:

    case VK_FORMAT_R8_UNORM: [[fallthrough]];
    case VK_FORMAT_R8_SRGB:
        requireUNORM(8);
        return convertUNORM<r8image>(image);
    case VK_FORMAT_R8G8_UNORM: [[fallthrough]];
    case VK_FORMAT_R8G8_SRGB:
        requireUNORM(8);
        return convertUNORM<rg8image>(image);
    case VK_FORMAT_R8G8B8_UNORM: [[fallthrough]];
    case VK_FORMAT_R8G8B8_SRGB:
        requireUNORM(8);
        return convertUNORM<rgb8image>(image);
    case VK_FORMAT_B8G8R8_UNORM: [[fallthrough]];
    case VK_FORMAT_B8G8R8_SRGB:
        requireUNORM(8);
        return convertUNORM<rgb8image>(image, "bgr1");

        // Verbatim copy with component reordering if needed, extra channels must be dropped.
        //
        // Input files that have 16-bit components must be truncated to
        // 8 bits with a right-shift and a warning must be generated in the stderr.

    case VK_FORMAT_R8G8B8A8_UNORM: [[fallthrough]];
    case VK_FORMAT_R8G8B8A8_SRGB: [[fallthrough]];
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32: [[fallthrough]];
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
        requireUNORM(8);
        return convertUNORM<rgba8image>(image);
    case VK_FORMAT_B8G8R8A8_UNORM: [[fallthrough]];
    case VK_FORMAT_B8G8R8A8_SRGB:
        requireUNORM(8);
        return convertUNORM<rgba8image>(image, "bgra");

        // Verbatim copy with component reordering if needed, extra channels must be dropped.

        // Input files that have 16-bit components must be truncated to
        // 8 bits with a right-shift and a warning must be generated in the stderr.

    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
        // ASTC texture data composition is performed via
        // R8G8B8A8_UNORM followed by the ASTC encoding
        requireUNORM(8);
        assert(false && "Internal error");
        return {};

        // Passthrough CLI options to the ASTC encoder.

    case VK_FORMAT_R4G4_UNORM_PACK8:
        requireUNORM(8);
        return convertUNORMPacked(image, 4, 4, 0, 0);
    case VK_FORMAT_R5G6B5_UNORM_PACK16:
        requireUNORM(8);
        return convertUNORMPacked(image, 5, 6, 5, 0);
    case VK_FORMAT_B5G6R5_UNORM_PACK16:
        requireUNORM(8);
        return convertUNORMPacked(image, 5, 6, 5, 0, "bgr1");

    case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
        requireUNORM(8);
        return convertUNORMPacked(image, 4, 4, 4, 4);
    case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
        requireUNORM(8);
        return convertUNORMPacked(image, 4, 4, 4, 4, "bgra");
    case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
        requireUNORM(8);
        return convertUNORMPacked(image, 5, 5, 5, 1);
    case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
        requireUNORM(8);
        return convertUNORMPacked(image, 5, 5, 5, 1, "bgra");
    case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
        requireUNORM(8);
        return convertUNORMPacked(image, 1, 5, 5, 5, "argb");
    case VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR:
        requireUNORM(8);
        return convertUNORMPacked(image, 1, 5, 5, 5, "abgr");
    case VK_FORMAT_A4R4G4B4_UNORM_PACK16:
        requireUNORM(8);
        return convertUNORMPacked(image, 4, 4, 4, 4, "argb");
    case VK_FORMAT_A4B4G4R4_UNORM_PACK16:
        requireUNORM(8);
        return convertUNORMPacked(image, 4, 4, 4, 4, "abgr");

        // Input values must be rounded to the target precision.
        // When the input file contains an sBIT chunk, its values must be taken into account.

    case VK_FORMAT_R10X6_UNORM_PACK16:
        requireUNORM(10);
        return convertUNORMSBits<r16image>(image, 10);
    case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:
        requireUNORM(10);
        return convertUNORMSBits<rg16image>(image, 10);
    case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:
        requireUNORM(10);
        return convertUNORMSBits<rgba16image>(image, 10);

    case VK_FORMAT_R12X4_UNORM_PACK16:
        requireUNORM(12);
        return convertUNORMSBits<r16image>(image, 12);
    case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:
        requireUNORM(12);
        return convertUNORMSBits<rg16image>(image, 12);
    case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:
        requireUNORM(12);
        return convertUNORMSBits<rgba16image>(image, 12);

        // Input values must be rounded to the target precision.
        // When the input file contains an sBIT chunk, its values must be taken into account.

    case VK_FORMAT_R16_UNORM:
        requireUNORM(16);
        return convertUNORM<r16image>(image);
    case VK_FORMAT_R16G16_UNORM:
        requireUNORM(16);
        return convertUNORM<rg16image>(image);
    case VK_FORMAT_R16G16B16_UNORM:
        requireUNORM(16);
        return convertUNORM<rgb16image>(image);
    case VK_FORMAT_R16G16B16A16_UNORM:
        requireUNORM(16);
        return convertUNORM<rgba16image>(image);

        // Verbatim copy, extra channels must be dropped.
        // Input PNG file must be 16-bit with sBIT chunk missing or signaling 16 bits.

    case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
        requireUNORM(10);
        return convertUNORMPacked(image, 2, 10, 10, 10, "argb");
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        requireUNORM(10);
        return convertUNORMPacked(image, 2, 10, 10, 10, "abgr");

        // Input values must be rounded to the target precision.
        // When the input file contains an sBIT chunk, its values must be taken into account.

    case VK_FORMAT_G8B8G8R8_422_UNORM: [[fallthrough]];
    case VK_FORMAT_B8G8R8G8_422_UNORM: [[fallthrough]];
    case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16: [[fallthrough]];
    case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16: [[fallthrough]];
    case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16: [[fallthrough]];
    case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16: [[fallthrough]];
    case VK_FORMAT_G16B16G16R16_422_UNORM: [[fallthrough]];
    case VK_FORMAT_B16G16R16G16_422_UNORM:
        fatal(rc::INVALID_ARGUMENTS, "Unsupported format for non-raw create: {}.", toString(options.vkFormat));
        break;

    // EXR:

    case VK_FORMAT_R8_UINT:
        requireSFloat(16);
        return convertUINT<r8image>(image);
    case VK_FORMAT_R8_SINT:
        requireSFloat(16);
        return convertSINT<r8image>(image);
    case VK_FORMAT_R16_UINT:
        requireSFloat(32);
        return convertUINT<r16image>(image);
    case VK_FORMAT_R16_SINT:
        requireSFloat(32);
        return convertSINT<r16image>(image);
    case VK_FORMAT_R32_UINT:
        requireUINT(32);
        return convertUINT<r32image>(image);
    case VK_FORMAT_R8G8_UINT:
        requireSFloat(16);
        return convertUINT<rg8image>(image);
    case VK_FORMAT_R8G8_SINT:
        requireSFloat(16);
        return convertSINT<rg8image>(image);
    case VK_FORMAT_R16G16_UINT:
        requireSFloat(32);
        return convertUINT<rg16image>(image);
    case VK_FORMAT_R16G16_SINT:
        requireSFloat(32);
        return convertSINT<rg16image>(image);
    case VK_FORMAT_R32G32_UINT:
        requireUINT(32);
        return convertUINT<rg32image>(image);
    case VK_FORMAT_R8G8B8_UINT:
        requireSFloat(16);
        return convertUINT<rgb8image>(image);
    case VK_FORMAT_R8G8B8_SINT:
        requireSFloat(16);
        return convertSINT<rgb8image>(image);
    case VK_FORMAT_B8G8R8_UINT:
        requireSFloat(16);
        return convertUINT<rgb8image>(image, "bgr1");
    case VK_FORMAT_B8G8R8_SINT:
        requireSFloat(16);
        return convertSINT<rgb8image>(image, "bgr1");
    case VK_FORMAT_R16G16B16_UINT:
        requireSFloat(32);
        return convertUINT<rgb16image>(image);
    case VK_FORMAT_R16G16B16_SINT:
        requireSFloat(32);
        return convertSINT<rgb16image>(image);
    case VK_FORMAT_R32G32B32_UINT:
        requireUINT(32);
        return convertUINT<rgb32image>(image);
    case VK_FORMAT_R8G8B8A8_UINT: [[fallthrough]];
    case VK_FORMAT_A8B8G8R8_UINT_PACK32:
        requireSFloat(16);
        return convertUINT<rgba8image>(image);
    case VK_FORMAT_R8G8B8A8_SINT: [[fallthrough]];
    case VK_FORMAT_A8B8G8R8_SINT_PACK32:
        requireSFloat(16);
        return convertSINT<rgba8image>(image);
    case VK_FORMAT_B8G8R8A8_UINT:
        requireSFloat(16);
        return convertUINT<rgba8image>(image, "bgra");
    case VK_FORMAT_B8G8R8A8_SINT:
        requireSFloat(16);
        return convertSINT<rgba8image>(image, "bgra");
    case VK_FORMAT_R16G16B16A16_UINT:
        requireSFloat(32);
        return convertUINT<rgba16image>(image);
    case VK_FORMAT_R16G16B16A16_SINT:
        requireSFloat(32);
        return convertSINT<rgba16image>(image);
    case VK_FORMAT_R32G32B32A32_UINT:
        requireUINT(32);
        return convertUINT<rgba32image>(image);

    case VK_FORMAT_A2R10G10B10_UINT_PACK32:
        requireSFloat(16);
        return convertUINTPacked(image, 2, 10, 10, 10, "argb");
    case VK_FORMAT_A2R10G10B10_SINT_PACK32:
        requireSFloat(16);
        return convertSINTPacked(image, 2, 10, 10, 10, "argb");
    case VK_FORMAT_A2B10G10R10_UINT_PACK32:
        requireSFloat(16);
        return convertUINTPacked(image, 2, 10, 10, 10, "abgr");
    case VK_FORMAT_A2B10G10R10_SINT_PACK32:
        requireSFloat(16);
        return convertSINTPacked(image, 2, 10, 10, 10, "abgr");

        // The same EXR pixel types as for the decoding must be enforced.
        // Extra channels must be dropped.

    case VK_FORMAT_R16_SFLOAT:
        requireSFloat(16);
        return convertSFLOAT<r16image>(image);
    case VK_FORMAT_R16G16_SFLOAT:
        requireSFloat(16);
        return convertSFLOAT<rg16image>(image);
    case VK_FORMAT_R16G16B16_SFLOAT:
        requireSFloat(16);
        return convertSFLOAT<rgb16image>(image);
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        requireSFloat(16);
        return convertSFLOAT<rgba16image>(image);

    case VK_FORMAT_R32_SFLOAT:
        requireSFloat(32);
        return convertSFLOAT<r32image>(image);
    case VK_FORMAT_R32G32_SFLOAT:
        requireSFloat(32);
        return convertSFLOAT<rg32image>(image);
    case VK_FORMAT_R32G32B32_SFLOAT:
        requireSFloat(32);
        return convertSFLOAT<rgb32image>(image);
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        requireSFloat(32);
        return convertSFLOAT<rgba32image>(image);

        // The same EXR pixel types as for the decoding must be enforced.
        // Extra channels must be dropped.

    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
        requireSFloat(16);
        return convertB10G11R11(image);
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
        requireSFloat(16);
        return convertE5B9G9R9(image);

        // Input data must be rounded to the target precision.

    case VK_FORMAT_D16_UNORM: [[fallthrough]];
    case VK_FORMAT_X8_D24_UNORM_PACK32: [[fallthrough]];
    case VK_FORMAT_D32_SFLOAT: [[fallthrough]];
    case VK_FORMAT_S8_UINT: [[fallthrough]];
    case VK_FORMAT_D16_UNORM_S8_UINT: [[fallthrough]];
    case VK_FORMAT_D24_UNORM_S8_UINT: [[fallthrough]];
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        fatal(rc::INVALID_ARGUMENTS, "Unsupported format for non-raw create: {}.", toString(options.vkFormat));
        break;

    case VK_FORMAT_A8_UNORM_KHR:
        // Special case for alpha-only
        requireUNORM(8);
        return convertUNORM<r8image>(image, "a000");
        break;

        // Not supported

    default:
        fatal(rc::INVALID_ARGUMENTS, "Requested format conversion is not yet implemented for: {}.", toString(options.vkFormat));
    }

    assert(false && "Internal error");
    return {};
}

KTXTexture2 CommandCreate::createTexture(const ImageSpec& target) {
    ktxTextureCreateInfo createInfo;
    std::memset(&createInfo, 0, sizeof(createInfo));

    assert(target.depth() == baseDepth);

    createInfo.vkFormat = options.vkFormat;
    createInfo.numFaces = numFaces;
    createInfo.numLayers = numLayers;
    createInfo.isArray = options.layers > 0u;
    createInfo.baseWidth = target.width();
    createInfo.baseHeight = target.height();
    createInfo.baseDepth = target.depth();
    createInfo.numDimensions = options._1d ? 1 : (options.depth > 0u ? 3 : 2);

    if (options.mipmapRuntime) {
        createInfo.generateMipmaps = true;
        createInfo.numLevels = 1;
    } else {
        createInfo.generateMipmaps = false;
        if (options.mipmapGenerate) {
            const auto maxDimension = std::max(target.width(), std::max(target.height(), target.depth()));
            const auto maxLevels = log2(maxDimension) + 1;
            createInfo.numLevels = options.levels.value_or(maxLevels);
        } else {
            createInfo.numLevels = numLevels;
        }
    }

    KTXTexture2 texture{nullptr};
    ktx_error_code_e ret = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, texture.pHandle());
    if (KTX_SUCCESS != ret)
        fatal(rc::KTX_FAILURE, "Failed to create ktxTexture: libktx error: {}", ktxErrorString(ret));

    KHR_DFDSETVAL(texture->pDfd + 1, PRIMARIES, target.format().primaries());
    KHR_DFDSETVAL(texture->pDfd + 1, TRANSFER, target.format().transfer());

    // Add KTXorientation metadata
    if (options.assignTexcoordOrigin.has_value() || options.convertTexcoordOrigin.has_value()) {
        // This code is future-proofed by supporting "right" origin and 1d
        // textures. Current imitations are enforced by CL option parsing.
        std::string orientation;
        orientation.resize(3);
        orientation = target.origin().x == ImageSpec::Origin::eLeft ? "r" : "l";
        if (!options._1d) {
            orientation += target.origin().y == ImageSpec::Origin::eTop ? "d" : "u";
            if (options.depth.has_value()) {
                orientation += target.origin().z == ImageSpec::Origin::eFront ? "i" : "o";
            }
        }

        ktxHashList_AddKVPair(&texture->kvDataHead, KTX_ORIENTATION_KEY,
                static_cast<uint32_t>(orientation.size() + 1), // +1 to include the \0
                orientation.c_str());
    }

    return texture;
}

void CommandCreate::generateMipLevels(KTXTexture2& texture, std::unique_ptr<Image> image, ImageInput& inputFile,
        uint32_t numMipLevels, uint32_t layerIndex, uint32_t faceIndex, uint32_t depthSliceIndex) {

    if (isFormatINT(static_cast<VkFormat>(texture->vkFormat)))
        fatal(rc::NOT_SUPPORTED, "Mipmap generation for SINT or UINT format {} is not supported.", toString(static_cast<VkFormat>(texture->vkFormat)));

    const auto baseWidth = image->getWidth();
    const auto baseHeight = image->getHeight();

    for (uint32_t mipLevelIndex = 1; mipLevelIndex < numMipLevels; ++mipLevelIndex) {
        const auto mipImageWidth = std::max(1u, baseWidth >> (mipLevelIndex));
        const auto mipImageHeight = std::max(1u, baseHeight >> (mipLevelIndex));

        try {
            image = image->resample(mipImageWidth, mipImageHeight,
                    options.mipmapFilter.value_or(options.defaultMipmapFilter).c_str(),
                    options.mipmapFilterScale.value_or(options.defaultMipmapFilterScale),
                    options.mipmapWrap.value_or(options.defaultMipmapWrap));
        } catch (const std::exception& e) {
            fatal(rc::RUNTIME_ERROR, "Mipmap generation failed: {}", e.what());
        }

        if (options.normalize)
            image->normalize();

        const auto imageData = convert(image, options.vkFormat, inputFile);

        const auto ret = ktxTexture_SetImageFromMemory(
                texture,
                mipLevelIndex,
                layerIndex,
                faceIndex + depthSliceIndex, // Faces and Depths are mutually exclusive, Addition is acceptable
                imageData.data(),
                imageData.size());
        assert(ret == KTX_SUCCESS && "Internal error"); (void) ret;
    }
}

void CommandCreate::selectASTCMode(uint32_t bitLength) {
    if (options.mode == KTX_PACK_ASTC_ENCODER_MODE_DEFAULT) {
        // If no astc mode option is specified and if input is <= 8bit
        // default to LDR otherwise default to HDR
        options.mode = bitLength <= 8 ? KTX_PACK_ASTC_ENCODER_MODE_LDR : KTX_PACK_ASTC_ENCODER_MODE_HDR;
    } else {
        if (bitLength > 8 && options.mode == KTX_PACK_ASTC_ENCODER_MODE_LDR)
            // Input is > 8-bit and user wants LDR, issue quality loss warning.
            warning("Input file is 16-bit but ASTC LDR option is specified. Expect quality loss in the output.");
        else if (bitLength < 16 && options.mode == KTX_PACK_ASTC_ENCODER_MODE_HDR)
            // Input is < 16-bit and user wants HDR, issue warning.
            warning("Input file is not 16-bit but HDR option is specified.");
    }

    // ASTC Encoding is performed by first creating a RGBA8 texture then encode it afterward

    // Encode based on non-8-bit input (aka true HDR) is currently not supported by
    // ktxTexture2_CompressAstcEx. Once supported suitable formats can be chosen here
    if (isFormatSRGB(options.vkFormat))
        options.vkFormat = VK_FORMAT_R8G8B8A8_SRGB;
    else
        options.vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
}

std::unique_ptr<const ColorPrimaries> CommandCreate::createColorPrimaries(khr_df_primaries_e primaries) const {
    switch (primaries) {
    case KHR_DF_PRIMARIES_BT709:
        return std::make_unique<ColorPrimariesBT709>();
    case KHR_DF_PRIMARIES_BT601_EBU:
        return std::make_unique<ColorPrimariesBT601_625_EBU>();
    case KHR_DF_PRIMARIES_BT601_SMPTE:
        return std::make_unique<ColorPrimariesBT601_525_SMPTE>();
    case KHR_DF_PRIMARIES_BT2020:
        return std::make_unique<ColorPrimariesBT2020>();
    case KHR_DF_PRIMARIES_CIEXYZ:
        return std::make_unique<ColorPrimariesCIEXYZ>();
    case KHR_DF_PRIMARIES_ACES:
        return std::make_unique<ColorPrimariesACES>();
    case KHR_DF_PRIMARIES_ACESCC:
        return std::make_unique<ColorPrimariesACEScc>();
    case KHR_DF_PRIMARIES_NTSC1953:
        return std::make_unique<ColorPrimariesNTSC1953>();
    case KHR_DF_PRIMARIES_PAL525:
        return std::make_unique<ColorPrimariesPAL525>();
    case KHR_DF_PRIMARIES_DISPLAYP3:
        return std::make_unique<ColorPrimariesDisplayP3>();
    case KHR_DF_PRIMARIES_ADOBERGB:
        return std::make_unique<ColorPrimariesAdobeRGB>();
    default:
        assert(false);
        // We return BT709 by default if some error happened
        return std::make_unique<ColorPrimariesBT709>();
    }
}

void CommandCreate::determineTargetColorSpace(const ImageInput& in, ImageSpec& target, ColorSpaceInfo& colorSpaceInfo) {
    // Primaries handling:
    //
    // 1. Use assign-primaries option value, if set.
    // 2. Use primaries info given by plugin.
    // 3. If no primaries info and input is PNG use PNG spec.
    //    recommendation of BT709/sRGB otherwise leave as
    //    UNSPECIFIED.
    // 4. If convert-primaries is specified but no primaries info is
    //    given by the plugin then fail.
    // 5. If convert-primaries is specified and primaries info determined
    //    above is different then set up conversion.
    const ImageSpec& spec = in.spec();

    // Set Primaries
    colorSpaceInfo.usedInputPrimaries = spec.format().primaries();
    if (options.assignPrimaries.has_value()) {
        colorSpaceInfo.usedInputPrimaries = options.assignPrimaries.value();
        target.format().setPrimaries(options.assignPrimaries.value());
    } else if (spec.format().primaries() != KHR_DF_PRIMARIES_UNSPECIFIED) {
        target.format().setPrimaries(spec.format().primaries());
    } else {
        if (!in.formatName().compare("png")) {
            warning("No color primaries in PNG input file \"{}\", defaulting to BT.709.", in.filename());
            colorSpaceInfo.usedInputPrimaries = KHR_DF_PRIMARIES_BT709;
            target.format().setPrimaries(KHR_DF_PRIMARIES_BT709);
        } else {
            // Leave as unspecified.
            target.format().setPrimaries(spec.format().primaries());
        }
    }

    if (options.convertPrimaries.has_value()) {
        if (colorSpaceInfo.usedInputPrimaries == KHR_DF_PRIMARIES_UNSPECIFIED) {
            fatal(rc::INVALID_FILE, "Cannot convert primaries as no information about the color primaries "
                "is available in the input file \"{}\". Use --assign-primaries to specify one.", in.filename());
        } else if (options.convertPrimaries.value() != colorSpaceInfo.usedInputPrimaries) {
            colorSpaceInfo.srcColorPrimaries = createColorPrimaries(colorSpaceInfo.usedInputPrimaries);
            colorSpaceInfo.dstColorPrimaries = createColorPrimaries(options.convertPrimaries.value());
            target.format().setPrimaries(options.convertPrimaries.value());
        }
    }

    // OETF / Transfer function handling in priority order:
    //
    // 1. Use assign-oetf option value, if set.
    // 2. Use OETF signalled by plugin as the input transfer function if
    //    linear, sRGB, ITU, or PQ EOTF. For all others, throw error.
    // 3. If ICC profile signalled, throw error. Known ICC profiles are
    //    handled by the plugin.
    // 4. If gamma of 1.0 signalled assume linear input transfer function.
    //    If gamma of .45454 signalled, set up for conversion from gamma
    //    and warn user about the conversion.
    //    If gamma of 0.0 is signalled, for PNG follow W3C recommendation
    //    per step 5. For any other gamma value, just convert it.
    // 5. If no color info is signalled, and input is PNG follow W3C
    //    recommendation of sRGB for 8-bit, linear otherwise. For other input
    //    formats throw error.
    // 6. Convert OETF based on convert-oetf option value or as described
    //    above.

    colorSpaceInfo.usedInputTransferFunction = KHR_DF_TRANSFER_UNSPECIFIED;
    if (options.assignOETF.has_value()) {
        if (options.assignOETF == KHR_DF_TRANSFER_SRGB) {
            colorSpaceInfo.srcTransferFunction = std::make_unique<TransferFunctionSRGB>();
        } else {
            assert(options.assignOETF == KHR_DF_TRANSFER_LINEAR);
            colorSpaceInfo.srcTransferFunction = std::make_unique<TransferFunctionLinear>();
        }
        colorSpaceInfo.usedInputTransferFunction = options.assignOETF.value();
        target.format().setTransfer(options.assignOETF.value());
    } else {
        // Set image's OETF as indicated by metadata.
        if (spec.format().transfer() != KHR_DF_TRANSFER_UNSPECIFIED) {
            colorSpaceInfo.usedInputTransferFunction = spec.format().transfer();
            switch (spec.format().transfer()) {
            case KHR_DF_TRANSFER_LINEAR:
                colorSpaceInfo.srcTransferFunction = std::make_unique<TransferFunctionLinear>();
                break;
            case KHR_DF_TRANSFER_SRGB:
                colorSpaceInfo.srcTransferFunction = std::make_unique<TransferFunctionSRGB>();
                break;
            case KHR_DF_TRANSFER_ITU:
                colorSpaceInfo.srcTransferFunction = std::make_unique<TransferFunctionITU>();
                break;
            case KHR_DF_TRANSFER_PQ_EOTF:
                colorSpaceInfo.srcTransferFunction = std::make_unique<TransferFunctionBT2100_PQ_EOTF>();
                break;
            default:
                fatal(rc::INVALID_FILE, "Transfer function {} used by input file \"{}\" is not supported by KTX. "
                    "Use --assign-oetf to specify a different one.",
                    toString(spec.format().transfer()), in.filename());
            }
        } else if (spec.format().iccProfileName().size()) {
            fatal(rc::INVALID_FILE,
                "Input file \"{}\" contains unsupported ICC profile \"{}\". Use --assign-oetf to specify a different one.",
                in.filename(), spec.format().iccProfileName());
        } else if (spec.format().oeGamma() > 0.0f) {
            if (spec.format().oeGamma() > .45450f && spec.format().oeGamma() < .45460f) {
                // N.B The previous loader matched oeGamma .45455 to the sRGB
                // OETF and did not do an OETF transformation. In this loader
                // we decode and reencode. Previous behavior can be obtained
                // with the --assign_oetf option to toktx.
                //
                // This change results in 1 bit differences in the LSB of
                // some color values noticeable only when directly comparing
                // images produced before and after this change of loader.
                warning("Converting gamma 2.2f to sRGB. Use --assign-oetf srgb to force treating input as sRGB.");
                colorSpaceInfo.srcTransferFunction = std::make_unique<TransferFunctionGamma>(spec.format().oeGamma());
            } else if (spec.format().oeGamma() == 1.0) {
                colorSpaceInfo.usedInputTransferFunction = KHR_DF_TRANSFER_LINEAR;
                colorSpaceInfo.srcTransferFunction = std::make_unique<TransferFunctionLinear>();
            } else if (spec.format().oeGamma() > 0.0f) {
                // We allow any gamma, there is not really a reason why we could not allow such input
                colorSpaceInfo.srcTransferFunction = std::make_unique<TransferFunctionGamma>(spec.format().oeGamma());
            } else if (spec.format().oeGamma() == 0.0f) {
                if (!in.formatName().compare("png")) {
                    // If 8-bit, treat as sRGB, otherwise treat as linear.
                    if (spec.format().channelBitLength() == 8) {
                        colorSpaceInfo.usedInputTransferFunction = KHR_DF_TRANSFER_SRGB;
                        colorSpaceInfo.srcTransferFunction = std::make_unique<TransferFunctionSRGB>();
                    } else {
                        colorSpaceInfo.usedInputTransferFunction = KHR_DF_TRANSFER_LINEAR;
                        colorSpaceInfo.srcTransferFunction = std::make_unique<TransferFunctionLinear>();
                    }
                    warning("Ignoring reported gamma of 0.0f in {}-bit PNG input file \"{}\". Handling as {}.",
                        spec.format().channelBitLength(), in.filename(), toString(colorSpaceInfo.usedInputTransferFunction));
                } else {
                    fatal(rc::INVALID_FILE,
                        "Input file \"{}\" has gamma 0.0f. Use --assign-oetf to specify transfer function.");
                }
            } else {
                if (!options.convertOETF.has_value()) {
                    fatal(rc::INVALID_FILE, "Gamma {} not automatically supported by KTX. Specify handing with "
                        "--convert-oetf or --assign-oetf.");
                }
            }
        } else if (!in.formatName().compare("png")) {
            // If 8-bit, treat as sRGB, otherwise treat as linear.
            if (spec.format().channelBitLength() == 8) {
                colorSpaceInfo.usedInputTransferFunction = KHR_DF_TRANSFER_SRGB;
                colorSpaceInfo.srcTransferFunction = std::make_unique<TransferFunctionSRGB>();
            } else {
                colorSpaceInfo.usedInputTransferFunction = KHR_DF_TRANSFER_LINEAR;
                colorSpaceInfo.srcTransferFunction = std::make_unique<TransferFunctionLinear>();
            }
            warning("No transfer function can be determined from {}-bit PNG input file \"{}\", defaulting to {}. Use --assign-oetf to override.",
                spec.format().channelBitLength(), in.filename(), toString(colorSpaceInfo.usedInputTransferFunction));
        }
    }

    if (options.convertOETF.has_value()) {
        target.format().setTransfer(options.convertOETF.value());
    }

    // Need to do color conversion if either the transfer functions don't match or the primaries
    if (target.format().transfer() != colorSpaceInfo.usedInputTransferFunction ||
        target.format().primaries() != colorSpaceInfo.usedInputPrimaries) {
        if (colorSpaceInfo.srcTransferFunction == nullptr)
            fatal(rc::INVALID_FILE,
                "No transfer function can be determined from input file \"{}\". Use --assign-oetf to specify one.", in.filename());

        switch (target.format().transfer()) {
        case KHR_DF_TRANSFER_LINEAR:
            colorSpaceInfo.dstTransferFunction = std::make_unique<TransferFunctionLinear>();
            break;
        case KHR_DF_TRANSFER_SRGB:
            colorSpaceInfo.dstTransferFunction = std::make_unique<TransferFunctionSRGB>();
            break;
        default:
            assert(false);
            break;
        }
    }
}

void CommandCreate::determineTargetOrigin(const ImageInput& in, ImageSpec& target,
                                          ImageSpec::Origin& usedInputOrigin) {
    const ImageSpec& spec = in.spec();

    // Set Origin
    usedInputOrigin = spec.origin();
    if (options.assignTexcoordOrigin.has_value()) {
        usedInputOrigin = options.assignTexcoordOrigin.value();
        target.setOrigin(options.assignTexcoordOrigin.value());
    } else {
        target.setOrigin(spec.origin());
    }

    if (options.convertTexcoordOrigin.has_value()) {
        if (usedInputOrigin.unspecified()) {
            fatal(rc::INVALID_FILE, "Cannot convert texcoord origin as no information about the origin "
                "is available in the input file \"{}\". Use --{} to specify one.",
                in.filename(), OptionsCreate::kAssignTexcoordOrigin);
        } else if (options.convertTexcoordOrigin.value() != usedInputOrigin) {
            target.setOrigin(options.convertTexcoordOrigin.value());
        }
    }
}



void CommandCreate::checkSpecsMatch(const ImageInput& currentFile, const ImageSpec& firstSpec) {
    const FormatDescriptor& firstFormat = firstSpec.format();
    const FormatDescriptor& currentFormat = currentFile.spec().format();

    if (currentFormat.transfer() != firstFormat.transfer()) {
        if (options.assignOETF.has_value()) {
            warning("Input image \"{}\" has different transfer function ({}) than the first image ({})"
                " but will be treated identically as specified by the --assign-oetf option.",
                currentFile.filename(), toString(currentFormat.transfer()), toString(firstFormat.transfer()));
        } else if (options.convertOETF.has_value()) {
            warning("Input image \"{}\" has different transfer function ({}) than the first image ({})"
                " and thus will go through different transfer function conversion to the target transfer"
                " function specified by the --convert-oetf option.",
                currentFile.filename(), toString(currentFormat.transfer()), toString(firstFormat.transfer()));
        } else {
            fatal(rc::INVALID_FILE, "Input image \"{}\" has different transfer function ({}) than the first image ({})."
                " Use --assign-oetf or --convert-oetf to specify handling and stop this error.",
                currentFile.filename(), toString(currentFormat.transfer()), toString(firstFormat.transfer()));
        }
    }

    if (currentFormat.oeGamma() != firstFormat.oeGamma()) {
        auto currentGamma = currentFormat.oeGamma() != -1 ? std::to_string(currentFormat.oeGamma()) : "no gamma";
        auto firstGamma = firstFormat.oeGamma() != -1 ? std::to_string(firstFormat.oeGamma()) : "no gamma";
        if (options.assignOETF.has_value()) {
            warning("Input image \"{}\" has different gamma ({}) than the first image ({})"
                " but will be treated identically as specified by the --assign-oetf option.",
                currentFile.filename(), currentGamma, firstGamma);
        } else if (options.convertOETF.has_value()) {
            warning("Input image \"{}\" has different gamma ({}) than the first image ({})"
                " and thus will go through different transfer function conversion to the target transfer"
                " function specified by the --convert-oetf option.",
                currentFile.filename(), currentGamma, firstGamma);
        } else {
            fatal(rc::INVALID_FILE, "Input image \"{}\" has different gamma ({}) than the first image ({})."
                " Use --assign-oetf or --convert-oetf to specify handling and stop this error.",
                currentFile.filename(), currentGamma, firstGamma);
        }
    }

    if (currentFormat.primaries() != firstFormat.primaries()) {
        if (options.assignPrimaries.has_value()) {
            warning("Input image \"{}\" has different primaries ({}) than the first image ({})"
                " but will be treated identically as specified by the --assign-primaries option.",
                currentFile.filename(), toString(currentFormat.primaries()), toString(firstFormat.primaries()));
        } else if (options.convertPrimaries.has_value()) {
            warning("Input image \"{}\" has different primaries ({}) than the first image ({})"
                " and thus will go through different primaries conversion to the target primaries"
                " specified by the --convert-primaries option.",
                currentFile.filename(), toString(currentFormat.primaries()), toString(firstFormat.primaries()));
        } else {
            fatal(rc::INVALID_FILE, "Input image \"{}\" has different primaries ({}) than the first image ({})."
                " Use --assign-primaries or --convert-primaries to specify handling and stop this error.",
                currentFile.filename(), toString(currentFormat.primaries()), toString(firstFormat.primaries()));
        }
    }

    if (currentFormat.channelCount() != firstFormat.channelCount()) {
        warning("Input image \"{}\" has a different component count than the first image.", currentFile.filename());
    }
}

} // namespace ktx

KTX_COMMAND_ENTRY_POINT(ktxCreate, ktx::CommandCreate)

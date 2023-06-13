// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include "command.h"
#include "metrics_utils.h"
#include "compress_utils.h"
#include "encode_utils.h"
#include "format_descriptor.h"
#include "formats.h"
#include "utility.h"
#include <filesystem>
#include <iostream>
#include <sstream>
#include <cxxopts.hpp>
#include <fmt/ostream.h>
#include <fmt/printf.h>
#include "ktx.h"
#include "image.hpp"
#include "imageio.h"


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

    khr_df_transfer_e convertOETF = KHR_DF_TRANSFER_UNSPECIFIED;
    khr_df_transfer_e assignOETF = KHR_DF_TRANSFER_UNSPECIFIED;
    khr_df_primaries_e assignPrimaries = KHR_DF_PRIMARIES_UNSPECIFIED;
    khr_df_primaries_e convertPrimaries = KHR_DF_PRIMARIES_UNSPECIFIED;
    bool failOnColorConversions = false;
    bool warnOnColorConversions = false;

    void init(cxxopts::Options& opts) {
        opts.add_options()
                ("format", "KTX format enum. The enum names are matching the VkFormats without the VK_FORMAT_ prefix."
                           " If the format is an ASTC formats the ASTC encoder specific options become valid,"
                           " otherwise they are ignored. Required. The VK_FORMAT_ prefix is ignored if present. Case insensitive.", cxxopts::value<std::string>(), "<enum>")
                ("1d", "Create a 1D texture. If not set the texture will be a 2D or 3D texture.")
                ("cubemap", "Create a cubemap texture. If not set the texture will be a 2D or 3D texture.")
                ("raw", "Create from raw image data.")
                ("width", "Base level width in pixels.", cxxopts::value<uint32_t>(), "[0-9]+")
                ("height", "Base level height in pixels.", cxxopts::value<uint32_t>(), "[0-9]+")
                ("depth", "Base level depth in pixels. If set the texture will be a 3D texture.", cxxopts::value<uint32_t>(), "[0-9]+")
                ("layers", "Number of layers. If set the texture will be an array texture.", cxxopts::value<uint32_t>(), "[0-9]+")
                ("levels", "Number of mip levels.", cxxopts::value<uint32_t>(), "[0-9]+")
                ("runtime-mipmap", "Runtime mipmap generation mode.")
                ("generate-mipmap", "Causes mipmaps to be generated during texture creation."
                    " It enables the use of \'Generate Mipmap\' options."
                    " If the --levels is not specified the maximum possible mip level will be generated."
                    " This option is mutually exclusive with --runtime-mipmap and cannot be used with UINT or 3D textures.")
                ("encode", "Encode the created KTX file. Case insensitive."
                    "\nPossible options are: basis-lz | uastc", cxxopts::value<std::string>(), "<codec>")
                ("swizzle", "KTX swizzle metadata.", cxxopts::value<std::string>(), "[rgba01]{4}")
                ("input-swizzle", "Pre-swizzle input channels.", cxxopts::value<std::string>(), "[rgba01]{4}")
                ("assign-oetf", "Force the created texture to have the specified transfer function, ignoring"
                    " the transfer function of the input file(s). Case insensitive."
                    "\nPossible options are: linear | srgb", cxxopts::value<std::string>(), "<oetf>")
                ("assign-primaries", "Force the created texture to have the specified color primaries, ignoring"
                    " the color primaries of the input file(s). Case insensitive."
                    "\nPossible options are:"
                    " bt709 | srgb | bt601-ebu | bt601-smpte | bt2020 | ciexyz | aces | acescc | ntsc1953 | pal525 | displayp3 | adobergb.",
                    cxxopts::value<std::string>(), "<primaries>")
                ("convert-oetf", "Convert the input image(s) to the specified transfer function, if different"
                    " from the transfer function of the input file(s). If both this and --assign-oetf are specified,"
                    " conversion will be performed from the assigned transfer function to the transfer function"
                    " specified by this option, if different. Case insensitive."
                    "\nPossible options are: linear | srgb", cxxopts::value<std::string>(), "<oetf>")
                ("convert-primaries", "Convert the image image(s) to the specified color primaries, if different"
                    " from the color primaries of the input file(s) or the one specified by --assign-primaries."
                    " If both this and --assign-primaries are specified, conversion will be performed from "
                    " the assigned primaries to the primaries specified by this option, if different."
                    " Case insensitive."
                    "\nPossible options are:"
                    " bt709 | srgb | bt601-ebu | bt601-smpte | bt2020 | ciexyz | aces | acescc | ntsc1953 | pal525 | displayp3 | adobergb.",
                    cxxopts::value<std::string>(), "<primaries>")
                ("fail-on-color-conversions", "Generates an error if any of the input images would need to be color converted.")
                ("warn-on-color-conversions", "Generates a warning if any of the input images are color converted.");

        opts.add_options("Generate Mipmap")
                ("mipmap-filter", "Specifies the filter to use when generating the mipmaps. Case insensitive."
                    "\nPossible options are:"
                    " box | tent | bell | b-spline | mitchell | blackman | lanczos3 | lanczos4 | lanczos6 |"
                    " lanczos12 | kaiser | gaussian | catmullrom | quadratic_interp | quadratic_approx | "
                    " quadratic_mix."
                    " Defaults to lanczos4.",
                    cxxopts::value<std::string>(), "<filter>")
                ("mipmap-filter-scale", "The filter scale to use. Defaults to 1.0.", cxxopts::value<float>(), "<float>")
                ("mipmap-wrap", "Specify how to sample pixels near the image boundaries. Case insensitive."
                    "\nPossible options are:"
                    " wrap | reflect | clamp."
                    " Defaults to clamp.", cxxopts::value<std::string>(), "<mode>");
    }

    khr_df_transfer_e parseTransferFunction(cxxopts::ParseResult& args, const char* argName, Reporter& report) const {
        static const std::unordered_map<std::string, khr_df_transfer_e> values{
            { "LINEAR", KHR_DF_TRANSFER_LINEAR },
            { "SRGB", KHR_DF_TRANSFER_SRGB }
        };

        if (args[argName].count()) {
            const auto oetfStr = to_upper_copy(args[argName].as<std::string>());
            const auto it = values.find(oetfStr);
            if (it != values.end()) {
                return it->second;
            } else {
                report.fatal_usage("Invalid or unsupported transfer function specified as --{} argument: \"{}\".", argName, oetfStr);
            }
        }

        return KHR_DF_TRANSFER_UNSPECIFIED;
    }

    khr_df_primaries_e parseColorPrimaries(cxxopts::ParseResult& args, const char* argName, Reporter& report) const {
        static const std::unordered_map<std::string, khr_df_primaries_e> values{
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

        if (args[argName].count()) {
            const auto primariesStr = to_upper_copy(args[argName].as<std::string>());
            const auto it = values.find(primariesStr);
            if (it != values.end()) {
                return it->second;
            } else {
                report.fatal_usage("Invalid or unsupported transfer function specified as --{} argument: \"{}\".", argName, primariesStr);
            }
        }

        return KHR_DF_PRIMARIES_UNSPECIFIED;
    }

    void process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter& report) {
        _1d = args["1d"].as<bool>();
        cubemap = args["cubemap"].as<bool>();
        raw = args["raw"].as<bool>();

        if (args["width"].count())
            width = args["width"].as<uint32_t>();
        if (args["height"].count())
            height = args["height"].as<uint32_t>();
        if (args["depth"].count())
            depth = args["depth"].as<uint32_t>();
        if (args["layers"].count())
            layers = args["layers"].as<uint32_t>();
        if (args["levels"].count())
            levels = args["levels"].as<uint32_t>();

        mipmapRuntime = args["runtime-mipmap"].as<bool>();
        mipmapGenerate = args["generate-mipmap"].as<bool>();

        if (args["mipmap-filter"].count()) {
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

            mipmapFilter = to_lower_copy(args["mipmap-filter"].as<std::string>());
            if (filter_table.count(*mipmapFilter) == 0)
                report.fatal_usage("Invalid or unsupported mipmap filter specified as --mipmap-filter argument: \"{}\".", *mipmapFilter);
        }

        if (args["mipmap-filter-scale"].count())
            mipmapFilterScale = args["mipmap-filter-scale"].as<float>();

        if (args["mipmap-wrap"].count()) {
            static const std::unordered_map<std::string, basisu::Resampler::Boundary_Op> wrap_table{
                { "clamp", basisu::Resampler::Boundary_Op::BOUNDARY_CLAMP },
                { "wrap", basisu::Resampler::Boundary_Op::BOUNDARY_WRAP },
                { "reflect", basisu::Resampler::Boundary_Op::BOUNDARY_REFLECT },
            };

            const auto wrapStr = to_lower_copy(args["mipmap-wrap"].as<std::string>());
            const auto it = wrap_table.find(wrapStr);
            if (it == wrap_table.end())
                report.fatal_usage("Invalid or unsupported mipmap wrap mode specified as --mipmap-wrap argument: \"{}\".", wrapStr);
            else
                mipmapWrap = it->second;
        }

        if (args["swizzle"].count()) {
            swizzle = to_lower_copy(args["swizzle"].as<std::string>());
            const auto errorFmt = "Invalid --swizzle value: \"{}\". The value must match the \"[rgba01]{{4}}\" regex.";
            if (swizzle->size() != 4)
                report.fatal_usage(errorFmt, *swizzle);
            for (const auto c : *swizzle)
                if (!contains("rgba01", c))
                    report.fatal_usage(errorFmt, *swizzle);
        }
        if (args["input-swizzle"].count()) {
            swizzleInput = to_lower_copy(args["input-swizzle"].as<std::string>());
            const auto errorFmt = "Invalid --input-swizzle value: \"{}\". The value must match the \"[rgba01]{{4}}\" regex.";
            if (swizzleInput->size() != 4)
                report.fatal_usage(errorFmt, *swizzleInput);
            for (const auto c : *swizzleInput)
                if (!contains("rgba01", c))
                    report.fatal_usage(errorFmt, *swizzleInput);
        }

        if (args["format"].count()) {
            const auto formatStr = args["format"].as<std::string>();
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
                VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT,
                VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT,
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

        convertOETF = parseTransferFunction(args, "convert-oetf", report);
        assignOETF = parseTransferFunction(args, "assign-oetf", report);

        convertPrimaries = parseColorPrimaries(args, "convert-primaries", report);
        assignPrimaries = parseColorPrimaries(args, "assign-primaries", report);

        if (raw) {
            if (convertOETF != KHR_DF_TRANSFER_UNSPECIFIED)
                report.fatal_usage("Option --convert-oetf cannot be used with --raw.");
            if (assignOETF != KHR_DF_TRANSFER_UNSPECIFIED)
                report.fatal_usage("Option --assign-oetf cannot be used with --raw.");
            if (convertPrimaries != KHR_DF_PRIMARIES_UNSPECIFIED)
                report.fatal_usage("Option --convert-primaries cannot be used with --raw.");
            if (assignPrimaries != KHR_DF_PRIMARIES_UNSPECIFIED)
                report.fatal_usage("Option --assign-primaries cannot be used with --raw.");
        }

        if (formatDesc.transfer() == KHR_DF_TRANSFER_SRGB) {
            if (convertOETF == KHR_DF_TRANSFER_UNSPECIFIED) {
                switch (assignOETF) {
                case KHR_DF_TRANSFER_UNSPECIFIED:
                case KHR_DF_TRANSFER_SRGB:
                    // assign-oetf must either not be specified or must be sRGB for an sRGB format
                    break;
                default:
                    report.fatal_usage(
                            "Invalid value to --assign-oetf \"{}\" for format \"{}\". Transfer function must be sRGB for sRGB formats.",
                            args["assign-oetf"].as<std::string>(), args["format"].as<std::string>());
                }
            } else if (convertOETF != KHR_DF_TRANSFER_SRGB) {
                report.fatal_usage(
                        "Invalid value to --convert-oetf \"{}\" for format \"{}\". Transfer function must be sRGB for sRGB formats.",
                        args["convert-oetf"].as<std::string>(), args["format"].as<std::string>());
            }
        }

        if (args["fail-on-color-conversions"].count())
            failOnColorConversions = true;

        if (args["warn-on-color-conversions"].count()) {
            if (failOnColorConversions)
                report.fatal_usage("The options --fail-on-color-conversions and warn-on-color-conversions are mutually exclusive.");
            warnOnColorConversions = true;
        }
    }
};

struct OptionsASTC : public ktxAstcParams {
    bool astc = false;
    ClampedOption<ktx_uint32_t> qualityLevel{ktxAstcParams::qualityLevel, 0, KTX_PACK_ASTC_QUALITY_LEVEL_MAX};

    OptionsASTC() {
        threadCount = std::thread::hardware_concurrency();
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
                ("astc-mode",
                        "Specify which encoding mode to use. LDR is the default unless the "
                        "input image is 16-bit in which case the default is HDR.",
                        cxxopts::value<std::string>(), "ldr | hdr")
                ("astc-quality",
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
                ("astc-perceptual",
                        "The codec should optimize for perceptual error, instead of direct "
                        "RMS error. This aims to improve perceived image quality, but "
                        "typically lowers the measured PSNR score. Perceptual methods are "
                        "currently only available for normal maps and RGB color data.");
    }

    void process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter& report) {
        if (args["astc-mode"].count()) {
            const auto modeStr = args["astc-mode"].as<std::string>();
            if (modeStr == "ldr")
                mode = KTX_PACK_ASTC_ENCODER_MODE_LDR;
            else if (modeStr == "hdr")
                mode = KTX_PACK_ASTC_ENCODER_MODE_HDR;
            else
                report.fatal_usage("Invalid astc-mode: \"{}\"", modeStr);
        } else {
            mode = KTX_PACK_ASTC_ENCODER_MODE_DEFAULT;
        }

        if (args["astc-quality"].count()) {
            static std::unordered_map<std::string, ktx_pack_astc_quality_levels_e> astc_quality_mapping{
                    {"fastest", KTX_PACK_ASTC_QUALITY_LEVEL_FASTEST},
                    {"fast", KTX_PACK_ASTC_QUALITY_LEVEL_FAST},
                    {"medium", KTX_PACK_ASTC_QUALITY_LEVEL_MEDIUM},
                    {"thorough", KTX_PACK_ASTC_QUALITY_LEVEL_THOROUGH},
                    {"exhaustive", KTX_PACK_ASTC_QUALITY_LEVEL_EXHAUSTIVE}
            };
            const auto qualityLevelStr = to_lower_copy(args["astc-quality"].as<std::string>());
            const auto it = astc_quality_mapping.find(qualityLevelStr);
            if (it == astc_quality_mapping.end())
                report.fatal_usage("Invalid astc-quality: \"{}\"", qualityLevelStr);
            qualityLevel = it->second;
        } else {
            qualityLevel = KTX_PACK_ASTC_QUALITY_LEVEL_MEDIUM;
        }

        perceptual = args["astc-perceptual"].as<bool>();
    }
};

// -------------------------------------------------------------------------------------------------

/** @page ktxtools_create ktx create
@~English

Create a KTX2 file from various input files.

@section ktxtools_create_synopsis SYNOPSIS
    ktx create [option...] @e input-file... @e output-file

@section ktxtools_create_description DESCRIPTION
    @b ktx @b create can create, encode and supercompress a KTX2 file from the
    input images specified as the @e input-file... arguments and save it as the
    @e output-file. The last positional argument is treated as the output-file.

    Each @e input-file must be a valid EXR (.exr), PNG (.png) or Raw (.raw) file.
    PNG files with luminance (L) or luminance + alpha (LA) data will be converted
    to RGB as LLL and RGBA as LLLA before processing further.
    The input file formats must be compatible with the requested KTX format enum and
    must have at least the same level of precision and number of channels.
    Any unused channel will be discarded silently.

    The number of input-files specified must match the expected number of input images
    based on the used options.

    The following options are available:
    <dl>
        <dt>--format &lt;enum&gt;</dt>
        <dd>KTX format enum. The enum names are matching the VkFormats without the VK_FORMAT_ prefix.
            If the format is an ASTC format the ASTC encoder specific options become valid,
            otherwise they are ignored. Required. The VK_FORMAT_ prefix is ignored if present.
            Case insensitive.</dd>
        <dl>
            <dt>--astc-mode &lt;ldr | hdr&gt;</dt>
            <dd>Specify which encoding mode to use. LDR is the default
                unless the input image is 16-bit in which case the default is
                HDR.</dd>
            <dt>--astc-quality &lt;level&gt;</dt>
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
            <dt>--astc-perceptual</dt>
            <dd>The codec should optimize for perceptual error, instead of
                direct RMS error. This aims to improve perceived image quality,
                but typically lowers the measured PSNR score. Perceptual
                methods are currently only available for normal maps and RGB
                color data.</dd>
        </dl>
        <dt>--1d</dt>
        <dd>Create a 1D texture. If not set the texture will be a 2D or 3D texture.</dd>
        <dt>--cubemap</dt>
        <dd>Create a cubemap texture. If not set the texture will be a 2D or 3D texture.</dd>
        <dt>--raw</dt>
        <dd>Create from raw image data.</dd>
        <dt>--width</dt>
        <dd>Base level width in pixels.</dd>
        <dt>--height</dt>
        <dd>Base level height in pixels.</dd>
        <dt>--depth</dt>
        <dd>Base level depth in pixels.
            If set the texture will be a 3D texture.</dd>
        <dt>--layers</dt>
        <dd>Number of layers.
            If set the texture will be an array texture.</dd>
        <dt>--runtime-mipmap</dt>
        <dd>Runtime mipmap generation mode.
            Sets up the texture to request the mipmaps to be generated by the client application at
            runtime.</dd>
        <dt>--generate-mipmap</dt>
        <dd>Causes mipmaps to be generated during texture creation.
            If the --levels is not specified the maximum possible mip level will be generated.
            This option is mutually exclusive with --runtime-mipmap and cannot be used with SINT,
            UINT or 3D textures.</dd>
            When set it enables the use of the following \'Generate Mipmap\' options.
        <dl>
            <dt>--mipmap-filter &lt;filter&gt;</dt>
            <dd>Specifies the filter to use when generating the mipmaps. Case insensitive.<br />
                Possible options are:
                box | tent | bell | b-spline | mitchell | blackman | lanczos3 | lanczos4 | lanczos6 |
                lanczos12 | kaiser | gaussian | catmullrom | quadratic_interp | quadratic_approx |
                quadratic_mix.
                Defaults to lanczos4.</dd>
            <dt>--mipmap-filter-scale &lt;float&gt;</dt>
            <dd>The filter scale to use.
                Defaults to 1.0.</dd>
            <dt>--mipmap-wrap &lt;mode&gt;</dt>
            <dd>Specify how to sample pixels near the image boundaries. Case insensitive.<br />
                Possible options are:
                wrap | reflect | clamp.
                Defaults to clamp.</dd>
        </dl>
    </dl>
    <dl>
        <dt>--encode basis-lz | uastc<</dt>
        <dd>Encode the texture with the specified codec before saving it.
            This option matches the functionality of the @ref ktxtools_encode "ktx encode" command.
            With each encoding option the following encoder specific options become valid,
            otherwise they are ignored. Case-insensitive.</dd>

        @snippet{doc} ktx/encode_utils.h command options_codec
        @snippet{doc} ktx/metrics_utils.h command options_metrics
    </dl>
    <dl>
        <dt>--swizzle [rgba01]{4}</dt>
        <dd>KTX swizzle metadata.</dd>
        <dt>--input-swizzle [rgba01]{4}</dt>
        <dd>Pre-swizzle input channels.</dd>
        <dt>--assign-oetf &lt;oetf&gt;</dt>
        <dd>Force the created texture to have the specified transfer function, ignoring
            the transfer function of the input file(s). Case insensitive.
            Possible options are:
            linear | srgb
            </dd>
        <dt>--assign-primaries &lt;primaries&gt;</dt>
        <dd>Force the created texture to have the specified color primaries, ignoring
            the color primaries of the input file(s). Case insensitive.
            Possible options are:
            bt709 | srgb | bt601-ebu | bt601-smpte | bt2020 | ciexyz | aces | acescc | ntsc1953 |
            pal525 | displayp3 | adobergb
            </dd>
        <dt>--convert-oetf &lt;oetf&gt;</dt>
        <dd>Convert the input image(s) to the specified transfer function, if different
            from the transfer function of the input file(s). If both this and --assign-oetf are
            specified, conversion will be performed from the assigned transfer function to the
            transfer function specified by this option, if different. Case insensitive.
            Possible options are:
            linear | srgb
            </dd>
        <dt>--convert-primaries &lt;primaries&gt;</dt>
        <dd>Convert the image image(s) to the specified color primaries, if different
            from the color primaries of the input file(s) or the one specified by --assign-primaries.
            If both this and --assign-primaries are specified, conversion will be performed from
            the assigned primaries to the primaries specified by this option, if different.
            Case insensitive.
            Possible options are:
            bt709 | srgb | bt601-ebu | bt601-smpte | bt2020 | ciexyz | aces | acescc | ntsc1953 |
            pal525 | displayp3 | adobergb
            </dd>
        <dt>--fail-on-color-conversions</dt>
        <dd>Generates an error if any of the input images would need to be color converted.</dd>
        <dt>--warn-on-color-conversions</dt>
        <dd>Generates a warning if any of the input images are color converted.</dd>
    </dl>
    @snippet{doc} ktx/compress_utils.h command options_compress
    @snippet{doc} ktx/command.h command options_generic

@section ktxtools_create_exitstatus EXIT STATUS
    @snippet{doc} ktx/command.h command exitstatus

@section ktxtools_create_history HISTORY

@par Version 4.0
 - Initial version

@section ktxtools_create_author AUTHOR
    - Mátyás Császár [Vader], RasterGrid www.rastergrid.com
    - Daniel Rákos, RasterGrid www.rastergrid.com
*/
class CommandCreate : public Command {
private:
    Combine<OptionsCreate, OptionsASTC, OptionsCodec<false>, OptionsMetrics, OptionsCompress, OptionsMultiInSingleOut, OptionsGeneric> options;

    uint32_t targetChannelCount = 0; // Derived from VkFormat

    uint32_t numLevels = 0;
    uint32_t numLayers = 0;
    uint32_t numFaces = 0;
    uint32_t numBaseDepths = 0;

public:
    virtual int main(int argc, _TCHAR* argv[]) override;
    virtual void initOptions(cxxopts::Options& opts) override;
    virtual void processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) override;

private:
    void executeCreate();
    void encode(KTXTexture2& texture, OptionsCodec<false>& opts);
    void encodeASTC(KTXTexture2& texture, OptionsASTC& opts);
    void compress(KTXTexture2& texture, const OptionsCompress& opts);

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

    void checkSpecsMatch(const ImageInput& current, const ImageSpec& firstSpec);
};

// -------------------------------------------------------------------------------------------------

int CommandCreate::main(int argc, _TCHAR* argv[]) {
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
    numBaseDepths = options.depth.value_or(1u);
    // baseDepth is determined by the --depth option. As the loaded images are
    // 2D "z_slice_of_blocks" their depth is always 1 and not relevant for any kind of deduction

    uint32_t expectedInputImages = 0;
    for (uint32_t i = 0; i < (options.mipmapGenerate ? 1 : numLevels); ++i)
        // If --generate-mipmap is set the input only contains the base level images
        expectedInputImages += numLayers * numFaces * std::max(numBaseDepths >> i, 1u);
    if (options.inputFilepaths.size() != expectedInputImages) {
        fatal_usage("Too {} input image for {} level{}, {} layer, {} face and {} depth. Provided {} but expected {}.",
                options.inputFilepaths.size() > expectedInputImages ? "many" : "few",
                numLevels,
                options.mipmapGenerate ? " (mips generated)" : "",
                numLayers,
                numFaces,
                numBaseDepths,
                options.inputFilepaths.size(), expectedInputImages);
    }

    if (options.codec == EncodeCodec::BasisLZ) {
        if (options.zstd.has_value())
            fatal_usage("Cannot encode to BasisLZ and supercompress with Zstd.");

        if (options.zlib.has_value())
            fatal_usage("Cannot encode to BasisLZ and supercompress with ZLIB.");
    }

    if (options.codec != EncodeCodec::NONE) {
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

    const auto canCompare = options.codec == EncodeCodec::BasisLZ || options.codec == EncodeCodec::UASTC;
    if (options.compare_ssim && !canCompare)
        fatal_usage("--compare-ssim can only be used with BasisLZ or UASTC encoding.");
    if (options.compare_psnr && !canCompare)
        fatal_usage("--compare-psnr can only be used with BasisLZ or UASTC encoding.");

    if (isFormatAstc(options.vkFormat) && !options.raw) {
        options.astc = true;

        switch (options.vkFormat) {
        case VK_FORMAT_ASTC_4x4_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_4x4;
            break;
        case VK_FORMAT_ASTC_5x4_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_5x4;
            break;
        case VK_FORMAT_ASTC_5x5_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_5x5;
            break;
        case VK_FORMAT_ASTC_6x5_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_6x5;
            break;
        case VK_FORMAT_ASTC_6x6_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_6x6;
            break;
        case VK_FORMAT_ASTC_8x5_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_8x5;
            break;
        case VK_FORMAT_ASTC_8x6_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_8x6;
            break;
        case VK_FORMAT_ASTC_8x8_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_8x8;
            break;
        case VK_FORMAT_ASTC_10x5_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_10x5;
            break;
        case VK_FORMAT_ASTC_10x6_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_10x6;
            break;
        case VK_FORMAT_ASTC_10x8_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_10x8;
            break;
        case VK_FORMAT_ASTC_10x10_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_10x10;
            break;
        case VK_FORMAT_ASTC_12x10_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_12x10;
            break;
        case VK_FORMAT_ASTC_12x12_UNORM_BLOCK: [[fallthrough]];
        case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
            options.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_12x12;
            break;
        default:
            fatal(rc::NOT_SUPPORTED, "{} is unsupported for ASTC encoding.", toString(options.vkFormat));
            break;
        }
    }

    if (options._1d && options.astc)
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
        // TODO: Tools P5: 3D BC formats currently discard the last partial z block slice
        //          This should be: ceil_div instead of div
        const auto numimageDepth = std::max(numBaseDepths >> levelIndex, 1u) / (format.basic.texelBlockDimension2 + 1);
        for (uint32_t layerIndex = 0; layerIndex < numLayers; ++layerIndex) {
            for (uint32_t faceIndex = 0; faceIndex < numFaces; ++faceIndex) {
                for (uint32_t depthSliceIndex = 0; depthSliceIndex < numimageDepth; ++depthSliceIndex) {
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
    std::ifstream file(filepath, std::ios::binary | std::ios::in | std::ios::ate);
    if (!file)
        fatal(rc::IO_FAILURE, "Failed to open file \"{}\": {}.", filepath.generic_string(), errnoMessage());

    const auto size = file.tellg();
    file.seekg(0);
    if (file.fail())
        fatal(rc::IO_FAILURE, "Failed to seek file \"{}\": {}.", filepath.generic_string(), errnoMessage());

    result.resize(size);
    file.read(result.data(), size);
    if (file.fail())
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
    ColorSpaceInfo colorSpaceInfo{};

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
                            inputFilepath, target.width(), target.height());

                texture = createTexture(target);
            }

            const auto rawData = readRawFile(inputFilepath);

            const auto expectedFileSize = ktxTexture_GetImageSize(texture, levelIndex);
            if (rawData.size() != expectedFileSize)
                fatal(rc::INVALID_FILE, "Raw input file \"{}\" with {} bytes for level {} does not match the expected size of {} bytes.",
                        inputFilepath, rawData.size(), levelIndex, expectedFileSize);

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

            if (std::exchange(firstImage, false)) {
                target = ImageSpec{
                        inputImageFile->spec().width(),
                        inputImageFile->spec().height(),
                        inputImageFile->spec().depth(),
                        options.formatDesc};

                if (options.cubemap && target.width() != target.height())
                    fatal(rc::INVALID_FILE, "--cubemap specified but the input image \"{}\" with size {}x{} is not square.",
                            inputFilepath, target.width(), target.height());

                if (options._1d && target.height() != 1)
                    fatal(rc::INVALID_FILE, "For --1d textures the input image height must be 1, but for \"{}\" it was {}.",
                            inputFilepath, target.height());

                const auto maxDimension = std::max(target.width(), std::max(target.height(), numBaseDepths));
                const auto maxLevels = log2(maxDimension) + 1;
                if (options.levels.value_or(1) > maxLevels)
                    fatal_usage("Requested {} levels is too many. With input image \"{}\" sized {}x{} and depth {} the texture can only have {} levels at most.",
                            options.levels.value_or(1), inputFilepath, target.width(), target.height(), numBaseDepths, maxLevels);

                if (options.astc)
                    selectASTCMode(inputImageFile->spec().format().largestChannelBitLength());

                firstImageSpec = inputImageFile->spec();
                determineTargetColorSpace(*inputImageFile, target, colorSpaceInfo);
                texture = createTexture(target);
            } else {
                checkSpecsMatch(*inputImageFile, firstImageSpec);
            }

            const uint32_t imageWidth = std::max(target.width() >> levelIndex, 1u);
            const uint32_t imageHeight = std::max(target.height() >> levelIndex, 1u);

            if (inputImageFile->spec().width() != imageWidth || inputImageFile->spec().height() != imageHeight)
                fatal(rc::INVALID_FILE, "Input image \"{}\" with size {}x{} does not match expected size {}x{} for level {}.",
                        inputFilepath, inputImageFile->spec().width(), inputImageFile->spec().height(), imageWidth, imageHeight, levelIndex);

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
                            inputFilepath);

                    if (options.warnOnColorConversions)
                        warning("Input file \"{}\" is color converted as input and output primaries are different. "
                            "Use --assign-primaries and do not use --convert-primaries to avoid unwanted color conversions.",
                            inputFilepath);

                    // Transform OETF with primary transform
                    image->transformColorSpace(*colorSpaceInfo.srcTransferFunction, *colorSpaceInfo.dstTransferFunction, &primaryTransform);
                } else {
                    if (options.failOnColorConversions)
                        fatal(rc::INVALID_FILE,
                            "Input file \"{}\" would need color conversion as input and output transfer functions are different. "
                            "Use --assign-oetf and do not use --convert-oetf to avoid unwanted color conversions.",
                            inputFilepath);

                    if (options.warnOnColorConversions)
                        warning("Input file \"{}\" is color converted as input and output transfer functions are different. "
                            "Use --assign-oetf and do not use --convert-oetf to avoid unwanted color conversions.",
                            inputFilepath);

                    // Transform OETF without primary transform
                    image->transformColorSpace(*colorSpaceInfo.srcTransferFunction, *colorSpaceInfo.dstTransferFunction);
                }
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
                const auto maxDimension = std::max(target.width(), std::max(target.height(), numBaseDepths));
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
    encode(texture, options);
    encodeASTC(texture, options);
    compress(texture, options);

    // Save output file
    if (std::filesystem::path(options.outputFilepath).has_parent_path())
        std::filesystem::create_directories(std::filesystem::path(options.outputFilepath).parent_path());
    FILE* f = _tfopen(options.outputFilepath.c_str(), "wb");
    if (!f)
        fatal(rc::IO_FAILURE, "Could not open output file \"{}\": {}.", options.outputFilepath, errnoMessage());

    // #if defined(_WIN32)
    //     if (f == stdout) {
    //         /* Set "stdout" to have binary mode */
    //         (void) _setmode(_fileno(stdout), _O_BINARY);
    //     }
    // #endif

    const auto ret = ktxTexture_WriteToStdioStream(texture, f);
    fclose(f);

    if (KTX_SUCCESS != ret) {
        if (f != stdout)
            std::filesystem::remove(options.outputFilepath);
        fatal(rc::IO_FAILURE, "Failed to write KTX file \"{}\": KTX error: {}", options.outputFilepath, ktxErrorString(ret));
    }
}

// -------------------------------------------------------------------------------------------------

void CommandCreate::encode(KTXTexture2& texture, OptionsCodec<false>& opts) {
    MetricsCalculator metrics;
    metrics.saveReferenceImages(texture, options, *this);

    if (opts.codec != EncodeCodec::NONE) {
        auto ret = ktxTexture2_CompressBasisEx(texture, &opts.basisOpts);
        if (ret != KTX_SUCCESS)
            fatal(rc::KTX_FAILURE, "Failed to encode KTX2 file with codec \"{}\". KTX Error: {}",
                    to_underlying(opts.codec), ktxErrorString(ret));
    }

    metrics.decodeAndCalculateMetrics(texture, options, *this);
}

void CommandCreate::encodeASTC(KTXTexture2& texture, OptionsASTC& opts) {
    if (opts.astc) {
        const auto ret = ktxTexture2_CompressAstcEx(texture, &opts);
        if (ret != KTX_SUCCESS)
            fatal(rc::KTX_FAILURE, "Failed to encode KTX2 file with codec ASTC. KTX Error: {}", ktxErrorString(ret));
    }
}

void CommandCreate::compress(KTXTexture2& texture, const OptionsCompress& opts) {
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
    const auto requestBitLength = std::max(bit_ceil(inputBitLength), 8u);
    const auto requestChannelCount = [&]() -> uint32_t {
        switch (inputImageFile.formatType()) {
        case ImageInputFormatType::png_l:
            // Load luminance images as RGB for processing as: L -> LLL1
            return 3;
        case ImageInputFormatType::png_la:
            // Load luminance-alpha images as RGBA for processing as: L -> LLLA
            return 4;
        default:
            return inputFormat.channelCount();
        }
    }();
    FormatDescriptor loadFormat;

    if (inputImageFile.formatType() == ImageInputFormatType::exr_float) {
        switch (requestChannelCount) {
        case 1:
            image = std::make_unique<r32fimage>(width, height);
            loadFormat = createFormatDescriptor(VK_FORMAT_R32_SFLOAT, *this);
            break;
        case 2:
            image = std::make_unique<rg32fimage>(width, height);
            loadFormat = createFormatDescriptor(VK_FORMAT_R32G32_SFLOAT, *this);
            break;
        case 3:
            image = std::make_unique<rgb32fimage>(width, height);
            loadFormat = createFormatDescriptor(VK_FORMAT_R32G32B32_SFLOAT, *this);
            break;
        case 4:
            image = std::make_unique<rgba32fimage>(width, height);
            loadFormat = createFormatDescriptor(VK_FORMAT_R32G32B32A32_SFLOAT, *this);
            break;
        }
    } else if (requestBitLength == 8) {
        switch (requestChannelCount) {
        case 1:
            image = std::make_unique<r8image>(width, height);
            loadFormat = createFormatDescriptor(VK_FORMAT_R8_UNORM, *this);
            break;
        case 2:
            image = std::make_unique<rg8image>(width, height);
            loadFormat = createFormatDescriptor(VK_FORMAT_R8G8_UNORM, *this);
            break;
        case 3:
            image = std::make_unique<rgb8image>(width, height);
            loadFormat = createFormatDescriptor(VK_FORMAT_R8G8B8_UNORM, *this);
            break;
        case 4:
            image = std::make_unique<rgba8image>(width, height);
            loadFormat = createFormatDescriptor(VK_FORMAT_R8G8B8A8_UNORM, *this);
            break;
        }
    } else if (requestBitLength == 16) {
        switch (requestChannelCount) {
        case 1:
            image = std::make_unique<r16image>(width, height);
            loadFormat = createFormatDescriptor(VK_FORMAT_R16_UNORM, *this);
            break;
        case 2:
            image = std::make_unique<rg16image>(width, height);
            loadFormat = createFormatDescriptor(VK_FORMAT_R16G16_UNORM, *this);
            break;
        case 3:
            image = std::make_unique<rgb16image>(width, height);
            loadFormat = createFormatDescriptor(VK_FORMAT_R16G16B16_UNORM, *this);
            break;
        case 4:
            image = std::make_unique<rgba16image>(width, height);
            loadFormat = createFormatDescriptor(VK_FORMAT_R16G16B16A16_UNORM, *this);
            break;
        }
    } else if (requestBitLength == 32) {
        switch (requestChannelCount) {
        case 1:
            image = std::make_unique<r32image>(width, height);
            loadFormat = createFormatDescriptor(VK_FORMAT_R32_UINT, *this);
            break;
        case 2:
            image = std::make_unique<rg32image>(width, height);
            loadFormat = createFormatDescriptor(VK_FORMAT_R32G32_UINT, *this);
            break;
        case 3:
            image = std::make_unique<rgb32image>(width, height);
            loadFormat = createFormatDescriptor(VK_FORMAT_R32G32B32_UINT, *this);
            break;
        case 4:
            image = std::make_unique<rgba32image>(width, height);
            loadFormat = createFormatDescriptor(VK_FORMAT_R32G32B32A32_UINT, *this);
            break;
        }
    } else {
        fatal(rc::INVALID_FILE, "Unsupported format with {}-bit and {} channel.",
                requestBitLength, requestChannelCount);
    }

    inputImageFile.readImage(static_cast<uint8_t*>(*image), image->getByteCount(), 0, 0, loadFormat);
    return image;
}

std::vector<uint8_t> convertUNORMPackedPadded(const std::unique_ptr<Image>& image,
        uint32_t c0 = 0, uint32_t c0Pad = 0, uint32_t c1 = 0, uint32_t c1Pad = 0,
        uint32_t c2 = 0, uint32_t c2Pad = 0, uint32_t c3 = 0, uint32_t c3Pad = 0,
        std::string_view swizzle = "") {

    if (!swizzle.empty())
        image->swizzle(swizzle);

    return image->getUNORMPackedPadded(c0, c0Pad, c1, c1Pad, c2, c2Pad, c3, c3Pad);
}

std::vector<uint8_t> convertUNORMPacked(const std::unique_ptr<Image>& image, uint32_t C0, uint32_t C1, uint32_t C2, uint32_t C3, std::string_view swizzle = "") {
    return convertUNORMPackedPadded(image, C0, 0, C1, 0, C2, 0, C3, 0, swizzle);
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
std::vector<uint8_t> convertSFLOAT(const std::unique_ptr<Image>& image, std::string_view swizzle = "") {
    using ComponentT = typename T::Color::value_type;
    static constexpr auto componentCount = T::Color::getComponentCount();
    static constexpr auto bytesPerComponent = sizeof(ComponentT);
    static constexpr auto bits = bytesPerComponent * 8;

    if (!swizzle.empty())
        image->swizzle(swizzle);

    return image->getSFloat(componentCount, bits);
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

    const uint32_t inputChannelCount = image->getComponentCount();
    const uint32_t inputBitDepth = std::max(8u, inputFile.spec().format().largestChannelBitLength());

    const auto require = [&](uint32_t channelCount, uint32_t bitDepth) {
        if (inputChannelCount < channelCount)
            fatal(rc::INVALID_FILE, "{}: Input file channel count {} is less than the required {} for {}.",
                    inputFile.filename(), inputChannelCount, channelCount, toString(vkFormat));

        if (inputBitDepth < bitDepth)
            fatal(rc::INVALID_FILE, "{}: Not enough precision to convert {} bit input to {} bit output for {}.",
                    inputFile.filename(), inputBitDepth, bitDepth, toString(vkFormat));
        if (inputBitDepth > bit_ceil(bitDepth))
            warning("{}: Possible loss of precision with converting {} bit input to {} bit output for {}.",
                    inputFile.filename(), inputBitDepth, bitDepth, toString(vkFormat));
    };
    const auto requireUNORM = [&](uint32_t channelCount, uint32_t bitDepth) {
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
        require(channelCount, bitDepth);
    };
    const auto requireSFloat = [&](uint32_t channelCount, uint32_t bitDepth) {
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
        require(channelCount, bitDepth);
    };
    const auto requireUINT = [&](uint32_t channelCount, uint32_t bitDepth) {
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
        require(channelCount, bitDepth);
    };

    // ------------

    switch (vkFormat) {
    // PNG:

    case VK_FORMAT_R8_UNORM: [[fallthrough]];
    case VK_FORMAT_R8_SRGB:
        requireUNORM(1, 8);
        return convertUNORM<r8image>(image);
    case VK_FORMAT_R8G8_UNORM: [[fallthrough]];
    case VK_FORMAT_R8G8_SRGB:
        requireUNORM(2, 8);
        return convertUNORM<rg8image>(image);
    case VK_FORMAT_R8G8B8_UNORM: [[fallthrough]];
    case VK_FORMAT_R8G8B8_SRGB:
        requireUNORM(3, 8);
        return convertUNORM<rgb8image>(image);
    case VK_FORMAT_B8G8R8_UNORM: [[fallthrough]];
    case VK_FORMAT_B8G8R8_SRGB:
        requireUNORM(3, 8);
        return convertUNORM<rgb8image>(image, "bgr1");

        // Verbatim copy with component reordering if needed, extra channels must be dropped.
        //
        // Input files that have 16-bit components must be truncated to
        // 8 bits with a right-shift and a warning must be generated in the stderr.

    case VK_FORMAT_R8G8B8A8_UNORM: [[fallthrough]];
    case VK_FORMAT_R8G8B8A8_SRGB:
        requireUNORM(4, 8);
        return convertUNORM<rgba8image>(image);
    case VK_FORMAT_B8G8R8A8_UNORM: [[fallthrough]];
    case VK_FORMAT_B8G8R8A8_SRGB:
        requireUNORM(4, 8);
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
        requireUNORM(4, 8);
        assert(false && "Internal error");
        return {};

        // Passthrough CLI options to the ASTC encoder.

    case VK_FORMAT_R4G4_UNORM_PACK8:
        requireUNORM(2, 8);
        return convertUNORMPacked(image, 4, 4, 0, 0);
    case VK_FORMAT_R5G6B5_UNORM_PACK16:
        requireUNORM(3, 8);
        return convertUNORMPacked(image, 5, 6, 5, 0);
    case VK_FORMAT_B5G6R5_UNORM_PACK16:
        requireUNORM(3, 8);
        return convertUNORMPacked(image, 5, 6, 5, 0, "bgr1");

    case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
        requireUNORM(4, 8);
        return convertUNORMPacked(image, 4, 4, 4, 4);
    case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
        requireUNORM(4, 8);
        return convertUNORMPacked(image, 4, 4, 4, 4, "bgra");
    case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
        requireUNORM(4, 8);
        return convertUNORMPacked(image, 5, 5, 5, 1);
    case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
        requireUNORM(4, 8);
        return convertUNORMPacked(image, 5, 5, 5, 1, "bgra");
    case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
        requireUNORM(4, 8);
        return convertUNORMPacked(image, 1, 5, 5, 5, "argb");
    case VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT:
        requireUNORM(4, 8);
        return convertUNORMPacked(image, 4, 4, 4, 4, "argb");
    case VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT:
        requireUNORM(4, 8);
        return convertUNORMPacked(image, 4, 4, 4, 4, "abgr");

        // Input values must be rounded to the target precision.
        // When the input file contains an sBIT chunk, its values must be taken into account.

    case VK_FORMAT_R10X6_UNORM_PACK16:
        requireUNORM(1, 10);
        return convertUNORMPackedPadded(image, 10, 6);
    case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:
        requireUNORM(2, 10);
        return convertUNORMPackedPadded(image, 10, 6, 10, 6);
    case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:
        requireUNORM(4, 10);
        return convertUNORMPackedPadded(image, 10, 6, 10, 6, 10, 6, 10, 6);

    case VK_FORMAT_R12X4_UNORM_PACK16:
        requireUNORM(1, 12);
        return convertUNORMPackedPadded(image, 12, 4);
    case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:
        requireUNORM(2, 12);
        return convertUNORMPackedPadded(image, 12, 4, 12, 4);
    case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:
        requireUNORM(4, 12);
        return convertUNORMPackedPadded(image, 12, 4, 12, 4, 12, 4, 12, 4);

        // Input values must be rounded to the target precision.
        // When the input file contains an sBIT chunk, its values must be taken into account.

    case VK_FORMAT_R16_UNORM:
        requireUNORM(1, 16);
        return convertUNORM<r16image>(image);
    case VK_FORMAT_R16G16_UNORM:
        requireUNORM(2, 16);
        return convertUNORM<rg16image>(image);
    case VK_FORMAT_R16G16B16_UNORM:
        requireUNORM(3, 16);
        return convertUNORM<rgb16image>(image);
    case VK_FORMAT_R16G16B16A16_UNORM:
        requireUNORM(4, 16);
        return convertUNORM<rgba16image>(image);

        // Verbatim copy, extra channels must be dropped.
        // Input PNG file must be 16-bit with sBIT chunk missing or signaling 16 bits.

    case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
        requireUNORM(4, 10);
        return convertUNORMPacked(image, 2, 10, 10, 10, "argb");
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        requireUNORM(4, 10);
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
        requireSFloat(1, 16);
        return convertUINT<r8image>(image);
    case VK_FORMAT_R8_SINT:
        requireSFloat(1, 16);
        return convertSINT<r8image>(image);
    case VK_FORMAT_R16_UINT:
        requireSFloat(1, 32);
        return convertUINT<r16image>(image);
    case VK_FORMAT_R16_SINT:
        requireSFloat(1, 32);
        return convertSINT<r16image>(image);
    case VK_FORMAT_R32_UINT:
        requireUINT(1, 32);
        return convertUINT<r32image>(image);
    case VK_FORMAT_R8G8_UINT:
        requireSFloat(2, 16);
        return convertUINT<rg8image>(image);
    case VK_FORMAT_R8G8_SINT:
        requireSFloat(2, 16);
        return convertSINT<rg8image>(image);
    case VK_FORMAT_R16G16_UINT:
        requireSFloat(2, 32);
        return convertUINT<rg16image>(image);
    case VK_FORMAT_R16G16_SINT:
        requireSFloat(2, 32);
        return convertSINT<rg16image>(image);
    case VK_FORMAT_R32G32_UINT:
        requireUINT(2, 32);
        return convertUINT<rg32image>(image);
    case VK_FORMAT_R8G8B8_UINT:
        requireSFloat(3, 16);
        return convertUINT<rgb8image>(image);
    case VK_FORMAT_R8G8B8_SINT:
        requireSFloat(3, 16);
        return convertSINT<rgb8image>(image);
    case VK_FORMAT_B8G8R8_UINT:
        requireSFloat(3, 16);
        return convertUINT<rgb8image>(image, "bgr1");
    case VK_FORMAT_B8G8R8_SINT:
        requireSFloat(3, 16);
        return convertSINT<rgb8image>(image, "bgr1");
    case VK_FORMAT_R16G16B16_UINT:
        requireSFloat(3, 32);
        return convertUINT<rgb16image>(image);
    case VK_FORMAT_R16G16B16_SINT:
        requireSFloat(3, 32);
        return convertSINT<rgb16image>(image);
    case VK_FORMAT_R32G32B32_UINT:
        requireUINT(3, 32);
        return convertUINT<rgb32image>(image);
    case VK_FORMAT_R8G8B8A8_UINT:
        requireSFloat(4, 16);
        return convertUINT<rgba8image>(image);
    case VK_FORMAT_R8G8B8A8_SINT:
        requireSFloat(4, 16);
        return convertSINT<rgba8image>(image);
    case VK_FORMAT_B8G8R8A8_UINT:
        requireSFloat(4, 16);
        return convertUINT<rgba8image>(image, "bgra");
    case VK_FORMAT_B8G8R8A8_SINT:
        requireSFloat(4, 16);
        return convertSINT<rgba8image>(image, "bgra");
    case VK_FORMAT_R16G16B16A16_UINT:
        requireSFloat(4, 32);
        return convertUINT<rgba16image>(image);
    case VK_FORMAT_R16G16B16A16_SINT:
        requireSFloat(4, 32);
        return convertSINT<rgba16image>(image);
    case VK_FORMAT_R32G32B32A32_UINT:
        requireUINT(4, 32);
        return convertUINT<rgba32image>(image);

    case VK_FORMAT_A2R10G10B10_UINT_PACK32:
        requireSFloat(4, 16);
        return convertUINTPacked(image, 2, 10, 10, 10, "argb");
    case VK_FORMAT_A2R10G10B10_SINT_PACK32:
        requireSFloat(4, 16);
        return convertSINTPacked(image, 2, 10, 10, 10, "argb");
    case VK_FORMAT_A2B10G10R10_UINT_PACK32:
        requireSFloat(4, 16);
        return convertUINTPacked(image, 2, 10, 10, 10, "abgr");
    case VK_FORMAT_A2B10G10R10_SINT_PACK32:
        requireSFloat(4, 16);
        return convertSINTPacked(image, 2, 10, 10, 10, "abgr");

        // The same EXR pixel types as for the decoding must be enforced.
        // Extra channels must be dropped.

    case VK_FORMAT_R16_SFLOAT:
        requireSFloat(1, 16);
        return convertSFLOAT<r16image>(image);
    case VK_FORMAT_R16G16_SFLOAT:
        requireSFloat(2, 16);
        return convertSFLOAT<rg16image>(image);
    case VK_FORMAT_R16G16B16_SFLOAT:
        requireSFloat(3, 16);
        return convertSFLOAT<rgb16image>(image);
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        requireSFloat(4, 16);
        return convertSFLOAT<rgba16image>(image);

    case VK_FORMAT_R32_SFLOAT:
        requireSFloat(1, 32);
        return convertSFLOAT<r32image>(image);
    case VK_FORMAT_R32G32_SFLOAT:
        requireSFloat(2, 32);
        return convertSFLOAT<rg32image>(image);
    case VK_FORMAT_R32G32B32_SFLOAT:
        requireSFloat(3, 32);
        return convertSFLOAT<rgb32image>(image);
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        requireSFloat(4, 32);
        return convertSFLOAT<rgba32image>(image);

        // The same EXR pixel types as for the decoding must be enforced.
        // Extra channels must be dropped.

    // case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
    // TODO: Tools P4: Create B10G11R11_UFLOAT_PACK32
    // case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
    // TODO: Tools P4: Create E5B9G9R9_UFLOAT_PACK32

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

    // BT709 is the default for DFDs.
    if (target.format().primaries() != KHR_DF_PRIMARIES_BT709)
        KHR_DFDSETVAL(((ktxTexture2*) texture)->pDfd + 1, PRIMARIES, target.format().primaries());

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
    if (options.assignPrimaries != KHR_DF_PRIMARIES_UNSPECIFIED) {
        colorSpaceInfo.usedInputPrimaries = options.assignPrimaries;
        target.format().setPrimaries(options.assignPrimaries);
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

    if (options.convertPrimaries != KHR_DF_PRIMARIES_UNSPECIFIED) {
        if (colorSpaceInfo.usedInputPrimaries == KHR_DF_PRIMARIES_UNSPECIFIED) {
            fatal(rc::INVALID_FILE, "Cannot convert primaries as no information about the color primaries "
                "is available in the input file \"{}\". Use --assign-primaries to specify one.", in.filename());
        } else if (options.convertPrimaries != colorSpaceInfo.usedInputPrimaries) {
            colorSpaceInfo.srcColorPrimaries = createColorPrimaries(colorSpaceInfo.usedInputPrimaries);
            colorSpaceInfo.dstColorPrimaries = createColorPrimaries(options.convertPrimaries);
        }
    }

    // OETF / Transfer function handling in priority order:
    //
    // 1. Use assign_oetf option value, if set.
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
    if (options.assignOETF != KHR_DF_TRANSFER_UNSPECIFIED) {
        if (options.assignOETF == KHR_DF_TRANSFER_SRGB) {
            colorSpaceInfo.srcTransferFunction = std::make_unique<TransferFunctionSRGB>();
        } else {
            colorSpaceInfo.srcTransferFunction = std::make_unique<TransferFunctionLinear>();
        }
        colorSpaceInfo.usedInputTransferFunction = options.assignOETF;
        target.format().setTransfer(options.assignOETF);
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
                if (options.convertOETF == KHR_DF_TRANSFER_UNSPECIFIED) {
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

    if (options.convertOETF != KHR_DF_TRANSFER_UNSPECIFIED) {
        target.format().setTransfer(options.convertOETF);
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

void CommandCreate::checkSpecsMatch(const ImageInput& currentFile, const ImageSpec& firstSpec) {
    const FormatDescriptor& firstFormat = firstSpec.format();
    const FormatDescriptor& currentFormat = currentFile.spec().format();

    // TODO: Tools P5: Question: Should we allow these with warnings? Spec says fatal, but if a conversion is possible this would just stop valid usecases
    if (currentFormat.transfer() != firstFormat.transfer()) {
        fatal(rc::INVALID_FILE, "Input image \"{}\" has different transfer function ({}) than preceding image(s) ({}).",
            currentFile.filename(), toString(currentFormat.transfer()), toString(firstFormat.transfer()));
    }

    if (currentFormat.primaries() != firstFormat.primaries()) {
        fatal(rc::INVALID_FILE, "Input image \"{}\" has different primaries ({}) than preceding image(s) ({}).",
            currentFile.filename(), toString(currentFormat.primaries()), toString(firstFormat.primaries()));
    }

    if (currentFormat.oeGamma() != firstFormat.oeGamma()) {
        fatal(rc::INVALID_FILE, "Input image \"{}\" has different gamma ({:.4}f) than preceding image(s) ({:.4}f).",
            currentFile.filename(), currentFormat.oeGamma(), firstFormat.oeGamma());
    }

    if (currentFormat.channelCount() != firstFormat.channelCount()) {
        warning("Input image \"{}\" has a different component count than preceding image(s).", currentFile.filename());
    }
}

} // namespace ktx

KTX_COMMAND_ENTRY_POINT(ktxCreate, ktx::CommandCreate)

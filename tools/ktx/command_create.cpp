// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include "command.h"
#include "formats.h"
#include "utility.h"
#include <filesystem>
#include <iostream>
#include <queue>
#include <cxxopts.hpp>
#include <fmt/ostream.h>
#include <fmt/printf.h>
#include "ktx.h"
#include "image.hpp"
#include "imageio.h"


// -------------------------------------------------------------------------------------------------

namespace ktx {

enum oetf_e {
    OETF_LINEAR = 0,
    OETF_SRGB = 1,
    OETF_UNSET = 2
};

struct TargetImageSpec : public ImageSpec {
    OETFFunc decodeFunc = nullptr;  // To be applied to the source image!
    OETFFunc encodeFunc = nullptr;

    TargetImageSpec& operator=(const ImageSpec& s) {
        *static_cast<ImageSpec*>(this) = s;
        encodeFunc = nullptr;
        decodeFunc = nullptr;
        return *this;
    }
};

class cant_create_image : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

// -------------------------------------------------------------------------------------------------

/** @page ktxtools_create ktx create
@~English

Creates a KTX2 file.

@warning TODO Tools P5: This page is incomplete

@section ktxtools_create_synopsis SYNOPSIS
    ktx create [options] @e input_file

@section ktxtools_create_description DESCRIPTION

    The following options are available:
    <dl>
        <dt>-f, --fff</dt>
        <dd>FFF Description</dd>
    </dl>
    @snippet{doc} ktx/command.h command options

@section ktxtools_create_exitstatus EXIT STATUS
    @b ktx @b create exits
        0 - Success
        1 - Command line error
        2 - IO error

@section ktxtools_create_history HISTORY

@par Version 4.0
 - Initial version

@section ktxtools_create_author AUTHOR
    - Mátyás Császár [Vader], RasterGrid www.rastergrid.com
    - Daniel Rákos, RasterGrid www.rastergrid.com
*/
class CommandCreate : public Command {
private:
    std::string outputFile;

    // int          etc1s;
    // int          zcmp;
    // int          astc;
    // ktx_bool_t   normalMode;
    // ktx_bool_t   normalize;
    // clamped<ktx_uint32_t> zcmpLevel;
    // clamped<ktx_uint32_t> threadCount;
    // string inputSwizzle;
    // struct basisOptions bopts;
    // struct astcOptions astcopts;

    struct Options {
        bool _1d = false;
        bool cubemap = false;

        VkFormat vkFormat = VK_FORMAT_UNDEFINED;
        bool raw = false;

        std::optional<uint32_t> width;
        std::optional<uint32_t> height;
        std::optional<uint32_t> depth;
        std::optional<uint32_t> layers;
        std::optional<uint32_t> levels;

        // int          automipmap = 0;
        // int          genmipmap = 0;
        // int          metadata = 1;
        // int          mipmap = 0;
        // khr_df_transfer_e convert_oetf = KHR_DF_TRANSFER_UNSPECIFIED;
        // khr_df_transfer_e assign_oetf = KHR_DF_TRANSFER_UNSPECIFIED;
        // khr_df_primaries_e assign_primaries = KHR_DF_PRIMARIES_MAX;
        // int          useStdin = 0;
        // struct mipgenOptions gmopts;
        // unsigned int depth = 0;
        // unsigned int layers = 0;
        // unsigned int levels = 1;
        // string swizzle;
    } options;

    uint32_t channelCount = 0; // Derived from VkFormat

public:
    using Command::Command;
    virtual ~CommandCreate() {};

    virtual int main(int argc, _TCHAR* argv[]) override;

protected:
    virtual void initOptions(cxxopts::Options& opts) override;
    virtual void processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) override;

private:
    void executeCreate();

    template <typename F>
    void foreachImage(F&& func);

    [[nodiscard]] std::string readRawFile(const std::filesystem::path& filepath);
    [[nodiscard]] std::unique_ptr<Image> createImage(const TargetImageSpec& target, ImageInput& in);
    void convertImageType(std::unique_ptr<Image>& image);

    [[nodiscard]] KTXTexture2 createTexture(const TargetImageSpec& target);

    void determineTargetColorSpace(const ImageInput& in, TargetImageSpec& target);
    void determineTargetTypeBitLengthScale(const ImageInput& in,
            TargetImageSpec& target,
            std::string& defaultSwizzle);
    void determineTargetImageSpec(const ImageInput& in,
            TargetImageSpec& target,
            std::string& defaultSwizzle);

    void checkSpecsMatch(const ImageInput& current, const ImageSpec& firstSpec);

    // void setAstcMode(const TargetImageSpec& target);

private:
    template <typename... Args>
    void warning(Args&&... args) {
        fmt::print(std::cerr, "{} error: ", processName);
        fmt::print(std::cerr, std::forward<Args>(args)...);
        fmt::print(std::cerr, "\n");
    }

    template <typename... Args>
    void error(Args&&... args) {
        fmt::print(std::cerr, "{} warning: ", processName);
        fmt::print(std::cerr, std::forward<Args>(args)...);
        fmt::print(std::cerr, "\n");
    }

    template <typename... Args>
    void fatal(int return_code, Args&&... args) {
        fmt::print(std::cerr, "{} fatal: ", processName);
        fmt::print(std::cerr, std::forward<Args>(args)...);
        fmt::print(std::cerr, "\n");
        throw FatalError(return_code);
    }
};

// -------------------------------------------------------------------------------------------------

int CommandCreate::main(int argc, _TCHAR* argv[]) {
    try {
        parseCommandLine("ktx create", "Creates a KTX2 file from the given input file(s).\n", argc, argv);
        executeCreate();
        return RETURN_CODE_SUCCESS;
    } catch (const FatalError& error) {
        return error.return_code;
    }
}

void CommandCreate::initOptions(cxxopts::Options& opts) {
    opts.add_options()
            ("1d", "Create a 1D texture. Unset by default")
            ("cubemap", "Create a cubemap texture. Unset by default")
            ("raw", "Create from raw image data. Unset by default", cxxopts::value<bool>())
            ("width", "Base level width in pixels", cxxopts::value<uint32_t>(), "[0-9]+")
            ("height", "Base level height in pixels", cxxopts::value<uint32_t>(), "[0-9]+")
            ("depth", "Base level depth in pixels", cxxopts::value<uint32_t>(), "[0-9]+")
            ("layers", "Number of layers", cxxopts::value<uint32_t>(), "[0-9]+")
            ("levels", "Number of mip levels", cxxopts::value<uint32_t>(), "[0-9]+")
            ("mipmap-runtime", "Runtime mipmap generation mode. Unset by default", cxxopts::value<bool>())
            ("mipmap-generate", "Mipmap generation mode followed by filtering options. Unset by default", cxxopts::value<std::string>(), "<filtering_options>")
            ("format", "KTX format enum. Case insensitive", cxxopts::value<std::string>(), "<enum>")
            ("encode", "Encode the created KTX file. Unset by default", cxxopts::value<std::string>(), "<codec>")
            // encode <codec_options>...
            ("swizzle", "KTX swizzle metadata. Unset by default", cxxopts::value<std::string>(), "[rgba01]{4}")
            ("input-swizzle", "Pre-swizzle input channels. Unset by default", cxxopts::value<std::string>(), "[rgba01]{4}")
            ("assign-oetf", "", cxxopts::value<std::string>(), "<oetf>")
            ("assign-primaries", "", cxxopts::value<std::string>(), "<primaries>")
            ("convert-oetf", "", cxxopts::value<std::string>(), "<oetf>")
            ("convert-primaries", "", cxxopts::value<std::string>(), "<primaries>")
            // ("zstd", "", cxxopts::value<std::string>(), "<flags>")
            // ("zlib", "", cxxopts::value<std::string>(), "<flags>")

            // ("files", "Input/output files. Last file specified will be used as output", cxxopts::value<std::vector<std::string>>(), "<filepath>")
            ("o,output-file", "The output file", cxxopts::value<std::string>(), "<filepath>");

    Command::initOptions(opts);
    opts.parse_positional({"input-file", "output-file"});
    opts.positional_help("<input-file...> <output-file>");
}

void CommandCreate::processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) {
    Command::processOptions(opts, args);

    outputFile = args["output-file"].as<std::string>();
    options._1d = args["1d"].as<bool>();
    options.cubemap = args["cubemap"].as<bool>();
    options.raw = args["raw"].as<bool>();

    if (args["width"].count())
        options.width = args["width"].as<uint32_t>();
    if (args["height"].count())
        options.height = args["height"].as<uint32_t>();
    if (args["depth"].count())
        options.depth = args["depth"].as<uint32_t>();
    if (args["layers"].count())
        options.layers = args["layers"].as<uint32_t>();
    if (args["levels"].count())
        options.levels = args["levels"].as<uint32_t>();

    // TODO Tools P5: Include every VkFormat in this lookup table (--raw supports everything)
    static const std::unordered_map<std::string, VkFormat> values{
            { "R8_UNORM", VK_FORMAT_R8_UNORM },
            { "R8_SRGB", VK_FORMAT_R8_SRGB },
            { "R8G8_UNORM", VK_FORMAT_R8G8_UNORM },
            { "R8G8_SRGB", VK_FORMAT_R8G8_SRGB },
            { "R8G8B8_UNORM", VK_FORMAT_R8G8B8_UNORM },
            { "R8G8B8_SRGB", VK_FORMAT_R8G8B8_SRGB },
            { "B8G8R8_UNORM", VK_FORMAT_B8G8R8_UNORM },
            { "B8G8R8_SRGB", VK_FORMAT_B8G8R8_SRGB },
            { "R8G8B8A8_UNORM", VK_FORMAT_R8G8B8A8_UNORM },
            { "R8G8B8A8_SRGB", VK_FORMAT_R8G8B8A8_SRGB },
            { "B8G8R8A8_UNORM", VK_FORMAT_B8G8R8A8_UNORM },
            { "B8G8R8A8_SRGB", VK_FORMAT_B8G8R8A8_SRGB },
            { "ASTC_4X4_UNORM_BLOCK", VK_FORMAT_ASTC_4x4_UNORM_BLOCK },
            { "ASTC_4X4_SRGB_BLOCK", VK_FORMAT_ASTC_4x4_SRGB_BLOCK },
            { "ASTC_5X4_UNORM_BLOCK", VK_FORMAT_ASTC_5x4_UNORM_BLOCK },
            { "ASTC_5X4_SRGB_BLOCK", VK_FORMAT_ASTC_5x4_SRGB_BLOCK },
            { "ASTC_5X5_UNORM_BLOCK", VK_FORMAT_ASTC_5x5_UNORM_BLOCK },
            { "ASTC_5X5_SRGB_BLOCK", VK_FORMAT_ASTC_5x5_SRGB_BLOCK },
            { "ASTC_6X5_UNORM_BLOCK", VK_FORMAT_ASTC_6x5_UNORM_BLOCK },
            { "ASTC_6X5_SRGB_BLOCK", VK_FORMAT_ASTC_6x5_SRGB_BLOCK },
            { "ASTC_6X6_UNORM_BLOCK", VK_FORMAT_ASTC_6x6_UNORM_BLOCK },
            { "ASTC_6X6_SRGB_BLOCK", VK_FORMAT_ASTC_6x6_SRGB_BLOCK },
            { "ASTC_8X5_UNORM_BLOCK", VK_FORMAT_ASTC_8x5_UNORM_BLOCK },
            { "ASTC_8X5_SRGB_BLOCK", VK_FORMAT_ASTC_8x5_SRGB_BLOCK },
            { "ASTC_8X6_UNORM_BLOCK", VK_FORMAT_ASTC_8x6_UNORM_BLOCK },
            { "ASTC_8X6_SRGB_BLOCK", VK_FORMAT_ASTC_8x6_SRGB_BLOCK },
            { "ASTC_8X8_UNORM_BLOCK", VK_FORMAT_ASTC_8x8_UNORM_BLOCK },
            { "ASTC_8X8_SRGB_BLOCK", VK_FORMAT_ASTC_8x8_SRGB_BLOCK },
            { "ASTC_10X5_UNORM_BLOCK", VK_FORMAT_ASTC_10x5_UNORM_BLOCK },
            { "ASTC_10X5_SRGB_BLOCK", VK_FORMAT_ASTC_10x5_SRGB_BLOCK },
            { "ASTC_10X6_UNORM_BLOCK", VK_FORMAT_ASTC_10x6_UNORM_BLOCK },
            { "ASTC_10X6_SRGB_BLOCK", VK_FORMAT_ASTC_10x6_SRGB_BLOCK },
            { "ASTC_10X8_UNORM_BLOCK", VK_FORMAT_ASTC_10x8_UNORM_BLOCK },
            { "ASTC_10X8_SRGB_BLOCK", VK_FORMAT_ASTC_10x8_SRGB_BLOCK },
            { "ASTC_10X10_UNORM_BLOCK", VK_FORMAT_ASTC_10x10_UNORM_BLOCK },
            { "ASTC_10X10_SRGB_BLOCK", VK_FORMAT_ASTC_10x10_SRGB_BLOCK },
            { "ASTC_12X10_UNORM_BLOCK", VK_FORMAT_ASTC_12x10_UNORM_BLOCK },
            { "ASTC_12X10_SRGB_BLOCK", VK_FORMAT_ASTC_12x10_SRGB_BLOCK },
            { "ASTC_12X12_UNORM_BLOCK", VK_FORMAT_ASTC_12x12_UNORM_BLOCK },
            { "ASTC_12X12_SRGB_BLOCK", VK_FORMAT_ASTC_12x12_SRGB_BLOCK },
            { "R4G4_UNORM_PACK8", VK_FORMAT_R4G4_UNORM_PACK8 },
            { "R5G6B5_UNORM_PACK16", VK_FORMAT_R5G6B5_UNORM_PACK16 },
            { "B5G6R5_UNORM_PACK16", VK_FORMAT_B5G6R5_UNORM_PACK16 },
            { "R4G4B4A4_UNORM_PACK16", VK_FORMAT_R4G4B4A4_UNORM_PACK16 },
            { "B4G4R4A4_UNORM_PACK16", VK_FORMAT_B4G4R4A4_UNORM_PACK16 },
            { "R5G5B5A1_UNORM_PACK16", VK_FORMAT_R5G5B5A1_UNORM_PACK16 },
            { "B5G5R5A1_UNORM_PACK16", VK_FORMAT_B5G5R5A1_UNORM_PACK16 },
            { "A1R5G5B5_UNORM_PACK16", VK_FORMAT_A1R5G5B5_UNORM_PACK16 },
            { "A4R4G4B4_UNORM_PACK16_EXT", VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT },
            { "A4B4G4R4_UNORM_PACK16_EXT", VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT },
            // { "R10X6_UNORM_PACK16", VK_FORMAT_R10X6_UNORM_PACK16 },
            // { "R10X6G10X6_UNORM_2PACK16", VK_FORMAT_R10X6G10X6_UNORM_2PACK16 },
            // { "R10X6G10X6B10X6A10X6_UNORM_4PACK16", VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16 },
            // { "R12X4_UNORM_PACK16", VK_FORMAT_R12X4_UNORM_PACK16 },
            // { "R12X4G12X4_UNORM_2PACK16", VK_FORMAT_R12X4G12X4_UNORM_2PACK16 },
            // { "R12X4G12X4B12X4A12X4_UNORM_4PACK16", VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16 },
            { "R16_UNORM", VK_FORMAT_R16_UNORM },
            { "R16G16_UNORM", VK_FORMAT_R16G16_UNORM },
            { "R16G16B16_UNORM", VK_FORMAT_R16G16B16_UNORM },
            { "R16G16B16A16_UNORM", VK_FORMAT_R16G16B16A16_UNORM },
            { "A2R10G10B10_UNORM_PACK32", VK_FORMAT_A2R10G10B10_UNORM_PACK32 },
            { "A2B10G10R10_UNORM_PACK32", VK_FORMAT_A2B10G10R10_UNORM_PACK32 },
            // { "G8B8G8R8_422_UNORM", VK_FORMAT_G8B8G8R8_422_UNORM },
            // { "B8G8R8G8_422_UNORM", VK_FORMAT_B8G8R8G8_422_UNORM },
            // { "G10X6B10X6G10X6R10X6_422_UNORM_4PACK16", VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16 },
            // { "B10X6G10X6R10X6G10X6_422_UNORM_4PACK16", VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16 },
            // { "G12X4B12X4G12X4R12X4_422_UNORM_4PACK16", VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16 },
            // { "B12X4G12X4R12X4G12X4_422_UNORM_4PACK16", VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16 },
            // { "G16B16G16R16_422_UNORM", VK_FORMAT_G16B16G16R16_422_UNORM },
            // { "B16G16R16G16_422_UNORM", VK_FORMAT_B16G16R16G16_422_UNORM },
            { "R8_UINT", VK_FORMAT_R8_UINT },
            { "R8_SINT", VK_FORMAT_R8_SINT },
            { "R16_UINT", VK_FORMAT_R16_UINT },
            { "R16_SINT", VK_FORMAT_R16_SINT },
            { "R32_UINT", VK_FORMAT_R32_UINT },
            { "R8G8_UINT", VK_FORMAT_R8G8_UINT },
            { "R8G8_SINT", VK_FORMAT_R8G8_SINT },
            { "R16G16_UINT", VK_FORMAT_R16G16_UINT },
            { "R16G16_SINT", VK_FORMAT_R16G16_SINT },
            { "R32G32_UINT", VK_FORMAT_R32G32_UINT },
            { "R8G8B8_UINT", VK_FORMAT_R8G8B8_UINT },
            { "R8G8B8_SINT", VK_FORMAT_R8G8B8_SINT },
            { "B8G8R8_UINT", VK_FORMAT_B8G8R8_UINT },
            { "B8G8R8_SINT", VK_FORMAT_B8G8R8_SINT },
            { "R16G16B16_UINT", VK_FORMAT_R16G16B16_UINT },
            { "R16G16B16_SINT", VK_FORMAT_R16G16B16_SINT },
            { "R32G32B32_UINT", VK_FORMAT_R32G32B32_UINT },
            { "R8G8B8A8_UINT", VK_FORMAT_R8G8B8A8_UINT },
            { "R8G8B8A8_SINT", VK_FORMAT_R8G8B8A8_SINT },
            { "B8G8R8A8_UINT", VK_FORMAT_B8G8R8A8_UINT },
            { "B8G8R8A8_SINT", VK_FORMAT_B8G8R8A8_SINT },
            { "R16G16B16A16_UINT", VK_FORMAT_R16G16B16A16_UINT },
            { "R16G16B16A16_SINT", VK_FORMAT_R16G16B16A16_SINT },
            { "R32G32B32A32_UINT", VK_FORMAT_R32G32B32A32_UINT },
            { "A2R10G10B10_UINT_PACK32", VK_FORMAT_A2R10G10B10_UINT_PACK32 },
            { "A2R10G10B10_SINT_PACK32", VK_FORMAT_A2R10G10B10_SINT_PACK32 },
            { "A2B10G10R10_SINT_PACK32", VK_FORMAT_A2B10G10R10_SINT_PACK32 },
            { "A2B10G10R10_UINT_PACK32", VK_FORMAT_A2B10G10R10_UINT_PACK32 },
            { "R16_SFLOAT", VK_FORMAT_R16_SFLOAT },
            { "R16G16_SFLOAT", VK_FORMAT_R16G16_SFLOAT },
            { "R16G16B16_SFLOAT", VK_FORMAT_R16G16B16_SFLOAT },
            { "R16G16B16A16_SFLOAT", VK_FORMAT_R16G16B16A16_SFLOAT },
            { "R32_SFLOAT", VK_FORMAT_R32_SFLOAT },
            { "R32G32_SFLOAT", VK_FORMAT_R32G32_SFLOAT },
            { "R32G32B32_SFLOAT", VK_FORMAT_R32G32B32_SFLOAT },
            { "R32G32B32A32_SFLOAT", VK_FORMAT_R32G32B32A32_SFLOAT },
            { "B10G11R11_UFLOAT_PACK32", VK_FORMAT_B10G11R11_UFLOAT_PACK32 },
            { "E5B9G9R9_UFLOAT_PACK32", VK_FORMAT_E5B9G9R9_UFLOAT_PACK32 },
            // { "D16_UNORM", VK_FORMAT_D16_UNORM },
            // { "X8_D24_UNORM_PACK32", VK_FORMAT_X8_D24_UNORM_PACK32 },
            // { "D32_SFLOAT", VK_FORMAT_D32_SFLOAT },
            // { "S8_UINT", VK_FORMAT_S8_UINT },
            // { "D16_UNORM_S8_UINT", VK_FORMAT_D16_UNORM_S8_UINT },
            // { "D24_UNORM_S8_UINT", VK_FORMAT_D24_UNORM_S8_UINT },
            // { "D32_SFLOAT_S8_UINT", VK_FORMAT_D32_SFLOAT_S8_UINT },
    };

    const auto formatStr = to_upper_copy(args["format"].as<std::string>());
    const auto it = values.find(formatStr);
    if (it == values.end())
        fatal(RETURN_CODE_INVALID_ARGUMENTS, "Invalid or not supported vkFormat: \"{}\"", formatStr);
    else
        options.vkFormat = it->second;

    // TODO Tools P5: Remove cli debugging after cli checks after the cli tests were implemented
    // for (const auto& item : args["input-file"].as<std::vector<std::string>>())
    //     std::cout << "input-file: " << item << std::endl;
    // std::cout << "current_path: " << std::filesystem::current_path().generic_string() << std::endl;
    // std::cout << "input-file: " << inputFile << std::endl;
    // std::cout << "output-file: " << outputFile << std::endl;
    //
    // std::cout << "options.cubemap: " << options.cubemap << std::endl;
    // std::cout << "options.raw: " << options.raw << std::endl;
    // std::cout << "options._1d: " << options._1d << std::endl;
    // std::cout << "options.vkFormat: " << toString(options.vkFormat) << std::endl;
    // std::cout << "options.width: " << options.width.value_or(-1) << std::endl;
    // std::cout << "options.height: " << options.height.value_or(-1) << std::endl;
    // std::cout << "options.depth: " << options.depth.value_or(-1) << std::endl;
    // std::cout << "options.layers: " << options.layers.value_or(-1) << std::endl;
    // std::cout << "options.levels: " << options.levels.value_or(-1) << std::endl;

    // if (...) {
    //     std::cerr << processName << ": too few input images for " << levelCount
    //             << " levels, " << texture->numLayers
    //             << " layers and " << texture->numFaces
    //             << " faces." << std::endl;
    //     fatal(RETURN_CODE_INVALID_ARGUMENTS, msg);
    // }
    //
    // if (...)
    //     warning("Ignoring excess input images.");
    //
    // if (options.layers == 0) {
    //     std::cerr << processName << ": "
    //             << "To create an array texture set --layers > 0." << std::endl;
    //     exit(1);
    // }
    //
    // if (options.depth == 0) {
    //     std::cerr << processName << ": "
    //             << "To create a 3d texture set --depth > 0." << std::endl;
    //     exit(1);
    // }
    //
    // if (options.levels < 2) {
    //     std::cerr << processName << ": "
    //             << "--levels must be > 1." << std::endl;
    //     exit(1);
    // }
    //
    // wrapping {
    //     if (!parser.optarg.compare("wrap")) {
    //         options.gmopts.wrapMode = basisu::Resampler::Boundary_Op::BOUNDARY_WRAP;
    //     } else if (!parser.optarg.compare("clamp")) {
    //         options.gmopts.wrapMode = basisu::Resampler::Boundary_Op::BOUNDARY_CLAMP;
    //     } else if (!parser.optarg.compare("reflect")) {
    //         options.gmopts.wrapMode = basisu::Resampler::Boundary_Op::BOUNDARY_REFLECT;
    //     } else {
    //         std::cerr << "Unrecognized mode \"" << parser.optarg << "\" passed to --wmode" << std::endl;
    //         usage();
    //         exit(1);
    //     }
    // }
    //
    // options.swizzle = validateSwizzle(parser.optarg);
    //
    // convert_oetf {
    //     for_each(parser.optarg.begin(), parser.optarg.end(), [](char& c) {
    //         c = (char) ::tolower(c);
    //     });
    //     if (parser.optarg.compare("linear") == 0)
    //         options.convert_oetf = KHR_DF_TRANSFER_LINEAR;
    //     else if (parser.optarg.compare("srgb") == 0)
    //         options.convert_oetf = KHR_DF_TRANSFER_SRGB;
    // }
    //
    // convert_primaries {
    //     for_each(parser.optarg.begin(), parser.optarg.end(), [](char& c) {
    //         c = (char) ::tolower(c);
    //     });
    //     if (parser.optarg.compare("linear") == 0)
    //         options.convert_oetf = KHR_DF_TRANSFER_LINEAR;
    //     else if (parser.optarg.compare("srgb") == 0)
    //         options.convert_oetf = KHR_DF_TRANSFER_SRGB;
    // }
    //
    // assign_oetf {
    //     for_each(parser.optarg.begin(), parser.optarg.end(), [](char& c) {
    //         c = (char) ::tolower(c);
    //     });
    //     if (parser.optarg.compare("linear") == 0)
    //         options.assign_oetf = KHR_DF_TRANSFER_LINEAR;
    //     else if (parser.optarg.compare("srgb") == 0)
    //         options.assign_oetf = KHR_DF_TRANSFER_SRGB;
    // }
    //
    // assign_primaries {
    //     for_each(parser.optarg.begin(), parser.optarg.end(), [](char& c) {
    //         c = (char) ::tolower(c);
    //     });
    //     if (parser.optarg.compare("bt709") == 0)
    //         options.assign_primaries = KHR_DF_PRIMARIES_BT709;
    //     else if (parser.optarg.compare("none") == 0)
    //         options.assign_primaries = KHR_DF_PRIMARIES_UNSPECIFIED;
    //     if (parser.optarg.compare("srgb") == 0)
    //         options.assign_primaries = KHR_DF_PRIMARIES_SRGB;
    // }

    // scApp::validateOptions():
    //
    // if (options.automipmap + options.genmipmap + options.mipmap > 1) {
    //     error("only one of --automipmap, --genmipmap and "
    //           "--mipmap may be specified.");
    //     usage();
    //     exit(1);
    // }
    // if ((options.automipmap || options.genmipmap) && options.levels > 1) {
    //     error("cannot specify --levels > 1 with --automipmap or --genmipmap.");
    //     usage();
    //     exit(1);
    // }
    // if (options.cubemap && options.lower_left_maps_to_s0t0) {
    //     error("cubemaps require images to have an upper-left origin. "
    //           "Ignoring --lower_left_maps_to_s0t0.");
    //     options.lower_left_maps_to_s0t0 = 0;
    // }
    // if (options.cubemap && options.depth > 0) {
    //     error("cubemaps cannot have 3D textures.");
    //     usage();
    //     exit(1);
    // }
    // if (options.layers && options.depth > 0) {
    //     error("cannot have 3D array textures.");
    //     usage();
    //     exit(1);
    // }
    // if (options.depth > 1 && options.genmipmap) {
    //     error("generation of mipmaps for 3d textures is not supported.\n"
    //           "A PR to add this feature will be gratefully accepted!");
    //     exit(1);
    // }
    //
    // if (options.outfile.compare(_T("-")) != 0
    //         && options.outfile.find_last_of('.') == _tstring::npos) {
    //     options.outfile.append(options.ktx2 ? _T(".ktx2") : _T(".ktx"));
    // }
    //
    // ktx_uint32_t requiredInputFiles = options.cubemap ? 6 : 1 * options.levels;
    // if (requiredInputFiles > options.infiles.size()) {
    //     error("too few input files.");
    //     exit(1);
    // }
}

[[nodiscard]] FormatDescriptor createTargetFormatDescriptor(VkFormat vkFormat) {
    const auto dfd = std::unique_ptr<uint32_t[], decltype(std::free)*>(vk2dfd(vkFormat), std::free);
    const auto* bdfd = dfd.get() + 1;

    std::vector<uint32_t> channelBitLengths;
    for (uint32_t i = 0; i < KHR_DFDSAMPLECOUNT(bdfd); ++i)
        channelBitLengths.emplace_back(KHR_DFDSVAL(bdfd, i, BITLENGTH) + 1);

    std::vector<khr_df_model_channels_e> channelTypes;
    for (uint32_t i = 0; i < KHR_DFDSAMPLECOUNT(bdfd); ++i)
        channelTypes.emplace_back(khr_df_model_channels_e(KHR_DFDSVAL(bdfd, i, CHANNELID)));

    FormatDescriptor desc(
            KHR_DFDSAMPLECOUNT(bdfd),
            channelBitLengths,
            channelTypes,
            khr_df_sample_datatype_qualifiers_e(KHR_DFDSVAL(bdfd, 0, QUALIFIERS)),
            khr_df_transfer_e(KHR_DFDVAL(bdfd, TRANSFER)),
            khr_df_primaries_e(KHR_DFDVAL(bdfd, PRIMARIES)),
            khr_df_model_e(KHR_DFDVAL(bdfd, MODEL)),
            khr_df_flags_e(KHR_DFDVAL(bdfd, FLAGS))
    );

    return desc;
}

template <typename F>
void CommandCreate::foreachImage(F&& func) {
    // Input file ordering is specified as the same order as the
    // "levelImages" structure in the KTX 2.0 specification:
    //      level > layer > face > image (z_slice_of_blocks)
    //      aka foreach level, foreach layer, foreach face, foreach image (z_slice_of_blocks)

    // TODO Tools P2: Multiple input files support
    std::queue<std::string> inputFilepaths{};
    inputFilepaths.push(inputFile);

    for (uint32_t levelIndex = 0; levelIndex < options.levels.value_or(1); ++levelIndex) {
        for (uint32_t layerIndex = 0; layerIndex < options.layers.value_or(1); ++layerIndex) {
            for (uint32_t faceIndex = 0; faceIndex < (options.cubemap ? 6u : 1u); ++faceIndex) {
                for (uint32_t depthIndex = 0; depthIndex < options.depth.value_or(1); ++depthIndex) {
                    assert(!inputFilepaths.empty()); // inputFilepaths were already validated during arg parsing
                    const auto inputFilepath = inputFilepaths.front();
                    inputFilepaths.pop();
                    func(inputFilepath, levelIndex, layerIndex, faceIndex, depthIndex);
                }
            }
        }
    }
}

std::string CommandCreate::readRawFile(const std::filesystem::path& filepath) {
    std::string result;
    std::ifstream file(filepath, std::ios::binary | std::ios::in | std::ios::ate);
    if (!file)
        fatal(RETURN_CODE_IO_FAILURE, "Failed to open file \"{}\": {}", filepath.generic_string(), errnoMessage());

    const auto size = file.tellg();
    file.seekg(0);
    if (file.fail())
        fatal(RETURN_CODE_IO_FAILURE, "Failed to seek file \"{}\": {}", filepath.generic_string(), errnoMessage());

    result.resize(size);
    file.read(result.data(), size);
    if (file.fail())
        fatal(RETURN_CODE_IO_FAILURE, "Failed to read file \"{}\": {}", filepath.generic_string(), errnoMessage());

    return result;
}

void CommandCreate::executeCreate() {
    KTXTexture2 texture{nullptr};

    // Base sizes are derived from either the first image or in case of --raw from command line arguments
    uint32_t baseWidth = 0;
    uint32_t baseHeight = 0;
    uint32_t baseDepth = 0;

    std::string defaultSwizzle;

    FormatDescriptor targetFormat = createTargetFormatDescriptor(options.vkFormat);
    channelCount = targetFormat.channelCount();

    bool firstImage = true;
    ImageSpec firstImageSpec;
    TargetImageSpec target;

    const auto warningFn = [this](const std::string& w) { this->warning(w); };

    foreachImage([&](const auto& inputFilepath,
            uint32_t levelIndex, uint32_t layerIndex, uint32_t faceIndex, uint32_t depthIndex) {
        (void) depthIndex; // TODO Tools P5: Merge z slices into a single 3D image

        ImageSpec config{
                std::max(1u, options.width.value_or(1u) >> levelIndex),
                std::max(1u, options.height.value_or(1u) >> levelIndex),
                std::max(1u, options.depth.value_or(1u) >> levelIndex),
                targetFormat};

        // if (options.cubemap && spec.width() != spec.height())
        //     fatal("--cubemap specified but image is not square.");

        if (options.raw) {
            if (firstImage) {
                target = config;
                texture = createTexture(target);

                baseWidth = texture->baseWidth;
                baseHeight = texture->baseHeight;
                baseDepth = texture->baseDepth;

                firstImage = false;
            }

            const auto rawIn = readRawFile(inputFilepath);
            // if (rawIn.size() != expectedFileSize)
            //     fatal(...);

            const auto ret = ktxTexture_SetImageFromMemory(
                    texture,
                    levelIndex,
                    layerIndex,
                    faceIndex,
                    reinterpret_cast<const ktx_uint8_t*>(rawIn.data()),
                    rawIn.size());
            assert(ret == KTX_SUCCESS && "Internal error"); (void) ret;
        } else {
            const auto in = ImageInput::open(inputFilepath, &config, warningFn);
            in->seekSubimage(0, 0);

            try {
                // Currently loading multiple subimage/miplevel from the same input is not supported
                const ImageSpec& spec = in->spec();

                if (!spec.format().sameUnitAllChannels())
                    fatal(1, "Could not to create image from {}: Components of differing size or type not yet supported.", inputFilepath);

                if (firstImage) {
                    firstImageSpec = spec;
                    determineTargetImageSpec(*in, target, defaultSwizzle);
                    texture = createTexture(target);

                    baseWidth = texture->baseWidth;
                    baseHeight = texture->baseHeight;
                    baseDepth = texture->baseDepth;

                    // if (options.astc)
                    //     setAstcMode(target);

                    firstImage = false;
                } else {
                    checkSpecsMatch(*in, firstImageSpec);
                }

                const uint32_t levelWidth = std::max(baseWidth >> levelIndex, 1u);
                const uint32_t levelHeight = std::max(baseHeight >> levelIndex, 1u);
                // const uint32_t levelDepth = std::max(baseDepth >> levelIndex, 1u);

                const auto image = createImage(target, *in);
                if (image->getWidth() != levelWidth || image->getHeight() != levelHeight) {
                    // TODO: Figure out 3d input images.
                    fatal(1, "Could not to create image from \"{}\": Image has incorrect size for next layer, face or mip level.", inputFilepath);
                }

                // if (options.inputSwizzle.size() > 0
                //         // inputSwizzle is handled during BasisU and astc encoding
                //         && !options.etc1s && !options.bopts.uastc && !options.astc) {
                //     image->swizzle(options.inputSwizzle);
                // }

                const auto ret = ktxTexture_SetImageFromMemory(
                        texture,
                        levelIndex,
                        layerIndex,
                        faceIndex,
                        *image,
                        image->getByteCount());
                assert(ret == KTX_SUCCESS && "Internal error"); (void) ret;
            } catch (const cant_create_image& e) {
                fatal(1, "Could not to create image from {}: {}", inputFilepath, e.what());
            } catch (const std::runtime_error& e) {
                fatal(2, "Failed to create image from {}: {}", inputFilepath, e.what());
            }
        }
    });

    // Add KTXwriter metadata
    const auto writer = fmt::format("{} {}", processName, version());
    ktxHashList_AddKVPair(&texture->kvDataHead, KTX_WRITER_KEY,
            static_cast<uint32_t>(writer.size() + 1), // +1 to include the \0
            writer.c_str());

    // Add KTXswizzle metadata
    // std::string swizzle;
    // if (options.swizzle.size()) {
    //     swizzle = options.swizzle;
    // } else if (!options.etc1s && !options.bopts.uastc && !options.astc && defaultSwizzle.size()) {
    //     swizzle = defaultSwizzle;
    // }
    // if (swizzle.size()) {
    //     ktxHashList_AddKVPair(&texture->kvDataHead, KTX_SWIZZLE_KEY,
    //             static_cast<uint32_t>(swizzle.size() + 1), // +1 to include the \0
    //             swizzle.c_str());
    // }

    // Save output file
    if (std::filesystem::path(outputFile).has_relative_path())
        std::filesystem::create_directories(std::filesystem::path(outputFile).parent_path());
    FILE* f = _tfopen(outputFile.c_str(), "wb");
    if (!f)
        fatal(2, "Could not open output file \"{}\": ", outputFile, errnoMessage());

    // #if defined(_WIN32)
    //     if (f == stdout) {
    //         /* Set "stdout" to have binary mode */
    //         (void) _setmode(_fileno(stdout), _O_BINARY);
    //     }
    // #endif

    // if (options.astc || options.etc1s || options.bopts.uastc || options.zcmp) {
    //     std::string& swizzle = options.inputSwizzle.size() == 0 && defaultSwizzle.size() && !options.normalMode
    //             ? defaultSwizzle
    //             : options.inputSwizzle;
    //     exitCode = encode((ktxTexture2*) texture, swizzle,
    //             f == stdout ? "stdout" : options.outfile);
    //     if (exitCode)
    //         goto closefileandcleanup;
    // }

    const auto ret = ktxTexture_WriteToStdioStream(texture, f);
    fclose(f);

    if (KTX_SUCCESS != ret) {
        if (f != stdout)
            std::filesystem::remove(outputFile);
        fatal(2, "Failed to write KTX file \"{}\": KTX error: {}", outputFile, ktxErrorString(ret));
    }
}

// =================================================================================================

std::unique_ptr<Image> CommandCreate::createImage(const TargetImageSpec& target, ImageInput& in) {
    const ImageSpec& inSpec = in.spec();

    std::unique_ptr<Image> image = nullptr;
    if (target.format().channelBitLength() == 8) {
        switch (inSpec.format().channelCount()) {
        case 1:
            image = std::make_unique<r8image>(inSpec.width(), inSpec.height());
            break;
        case 2:
            image = std::make_unique<rg8image>(inSpec.width(), inSpec.height());
            break;
        case 3:
            image = std::make_unique<rgb8image>(inSpec.width(), inSpec.height());
            break;
        case 4:
            image = std::make_unique<rgba8image>(inSpec.width(), inSpec.height());
            break;
        }
    } else if (target.format().channelBitLength() == 16) {
        switch (inSpec.format().channelCount()) {
        case 1:
            image = std::make_unique<r16image>(inSpec.width(), inSpec.height());
            break;
        case 2:
            image = std::make_unique<rg16image>(inSpec.width(), inSpec.height());
            break;
        case 3:
            image = std::make_unique<rgb16image>(inSpec.width(), inSpec.height());
            break;
        case 4:
            image = std::make_unique<rgba16image>(inSpec.width(), inSpec.height());
            break;
        }
    } else if (target.format().channelBitLength() == 32) {
        switch (inSpec.format().channelCount()) {
        case 1:
            image = std::make_unique<r32image>(inSpec.width(), inSpec.height());
            break;
        case 2:
            image = std::make_unique<rg32image>(inSpec.width(), inSpec.height());
            break;
        case 3:
            image = std::make_unique<rgb32image>(inSpec.width(), inSpec.height());
            break;
        case 4:
            image = std::make_unique<rgba32image>(inSpec.width(), inSpec.height());
            break;
        }
    } else {
        const uint32_t ct = inSpec.format().samples[0].channelType;
        const auto dtq = static_cast<khr_df_sample_datatype_qualifiers_e>(ct);

        throw std::runtime_error(fmt::format("Unsupported format {}-bit {} needed.",
                inSpec.format().channelBitLength(), fmt::streamed(dtq)));
        // TODO: uint64?, float etc.
    }

    in.readImage(static_cast<uint8_t*>(*image), image->getByteCount(), 0, 0, target.format());

    // TODO: Convert primaries?
    image->setPrimaries(target.format().primaries());
    if (target.encodeFunc != nullptr) {
        assert(target.decodeFunc != nullptr);
        image->transformOETF(target.decodeFunc, target.encodeFunc, inSpec.format().oeGamma());
        if (target.encodeFunc == encode_sRGB) {
            image->setOetf(KHR_DF_TRANSFER_SRGB);
        } else {
            image->setOetf(KHR_DF_TRANSFER_LINEAR);
        }
    } else {
        image->setOetf(target.format().transfer());
    }

    convertImageType(image);

    return image;
}

void CommandCreate::convertImageType(std::unique_ptr<Image>& image) {
    // TODO: These copyTo's should be reversed. The image should have
    // a copy constructor for each componentCount src image.
    if (channelCount == image->getComponentCount())
        return;

    std::unique_ptr<Image> newImage;
    // The casts in the following copyTo* definitions only work
    // because, thanks to the switch, at runtime we always pass
    // the image type being cast to.

    if (image->getComponentSize() == 1) {
        switch (channelCount) {
        case 1:
            newImage = std::make_unique<r8image>(image->getWidth(), image->getHeight());
            image->copyToR(*newImage);
            break;
        case 2:
            newImage = std::make_unique<rg8image>(image->getWidth(), image->getHeight());
            image->copyToRG(*newImage);
            break;
        case 3:
            newImage = std::make_unique<rgb8image>(image->getWidth(), image->getHeight());
            image->copyToRGB(*newImage);
            break;
        case 4:
            newImage = std::make_unique<rgba8image>(image->getWidth(), image->getHeight());
            image->copyToRGBA(*newImage);
            break;
        }
    } else if (image->getComponentSize() == 2) {
        switch (channelCount) {
        case 1:
            newImage = std::make_unique<r16image>(image->getWidth(), image->getHeight());
            image->copyToR(*newImage);
            break;
        case 2:
            newImage = std::make_unique<rg16image>(image->getWidth(), image->getHeight());
            image->copyToRG(*newImage);
            break;
        case 3:
            newImage = std::make_unique<rgb16image>(image->getWidth(), image->getHeight());
            image->copyToRGB(*newImage);
            break;
        case 4:
            newImage = std::make_unique<rgba16image>(image->getWidth(), image->getHeight());
            image->copyToRGBA(*newImage);
            break;
        }
    } else if (image->getComponentSize() == 4) {
        switch (channelCount) {
        case 1:
            newImage = std::make_unique<r32image>(image->getWidth(), image->getHeight());
            image->copyToR(*newImage);
            break;
        case 2:
            newImage = std::make_unique<rg32image>(image->getWidth(), image->getHeight());
            image->copyToRG(*newImage);
            break;
        case 3:
            newImage = std::make_unique<rgb32image>(image->getWidth(), image->getHeight());
            image->copyToRGB(*newImage);
            break;
        case 4:
            newImage = std::make_unique<rgba32image>(image->getWidth(), image->getHeight());
            image->copyToRGBA(*newImage);
            break;
        }
    }

    image = std::move(newImage);
    if (!newImage)
        throw std::runtime_error("Unsupported target image type (component size / count).");
}

KTXTexture2 CommandCreate::createTexture(const TargetImageSpec& target) {
    ktxTextureCreateInfo createInfo;

    std::memset(&createInfo, 0, sizeof(createInfo));

    createInfo.vkFormat = options.vkFormat;
    createInfo.numFaces = options.cubemap ? 6 : 1;

    // TODO Tools P2: Array texture
    //     if (options.layers) {
    //     createInfo.numLayers = options.layers;
    //     createInfo.isArray = KTX_TRUE;
    createInfo.numLayers = 1;
    createInfo.isArray = KTX_FALSE;

    createInfo.baseWidth = target.width();
    createInfo.baseHeight = target.height();
    // TODO Tools P2: baseDepth target.depth()
    createInfo.baseDepth = options.depth.value_or(1);

    // TODO Tools P2: 1D/3D textures
    //     createInfo.numDimensions = options._1d ? 1 : options.depth.value_or(0) > 1 ? 3 : 2;
    createInfo.numDimensions = 2;

    // TODO Tools P2: Mipmap, Levels
    //     createInfo.numLevels = 1;
    //     createInfo.numLevels = options.levels;
    //     createInfo.numLevels = log2(max_dim) + 1;
    //     createInfo.generateMipmaps = KTX_TRUE;
    createInfo.generateMipmaps = KTX_FALSE;
    createInfo.numLevels = 1;

    // if (isFormatSRGB(options.vkFormat) && numComponent <= 2)
    //         && !(options.astc || options.etc1s || options.bopts.uastc)) {
    //     // Encoding to BasisU or ASTC will cause conversion to RGB.
    //     warning("GPU support of sRGB variants of R & RG formats is"
    //             " limited.\nConsider using '--target_type' or"
    //             " '--convert_oetf linear' to avoid these formats.");
    // }

    KTXTexture2 texture{nullptr};
    ktx_error_code_e ret = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, texture.pHandle());
    if (KTX_SUCCESS != ret)
        fatal(2, "Failed to create ktxTexture: libktx error: {}", ktxErrorString(ret));

    // BT709 is the default for DFDs.
    if (target.format().primaries() != KHR_DF_PRIMARIES_BT709)
        KHR_DFDSETVAL(((ktxTexture2*) texture)->pDfd + 1, PRIMARIES, target.format().primaries());

    return texture;
}

void CommandCreate::determineTargetColorSpace(const ImageInput& in, TargetImageSpec& target) {
    // Primaries handling:
    //
    // 1. Use assign_primaries option value, if set.
    // 2. Use primaries info given by plugin.
    // 3. If no primaries info and input is PNG use PNG spec.
    //    recommendation of BT709/sRGB otherwise leave as
    //    UNSPECIFIED.
    const ImageSpec& spec = in.spec();
    // Set Primaries
    // if (options.assign_primaries != KHR_DF_PRIMARIES_MAX) {
    //     target.format().setPrimaries(options.assign_primaries);
    // } else if (spec.format().primaries() != KHR_DF_PRIMARIES_UNSPECIFIED) {
    //     target.format().setPrimaries(spec.format().primaries());
    // } else {
    {
        // Leave as unspecified.
        target.format().setPrimaries(spec.format().primaries());
    }

    // OETF / Transfer function handling in priority order:
    //
    // 1. Use assign_oetf option value, if set.
    // 2. Use OETF signalled by plugin, if LINEAR or SRGB. If ITU signalled,
    //    set up conversion to SRGB. For all others, throw error.
    // 3. If ICC profile signalled, throw error.
    // 4. If gamma of 1.0 signalled use LINEAR. If gamma of .45454 signalled,
    //    set up for conversion to SRGB. If gamma of 0.0 is signalled,
    //    set SRGB. For any other gamma value, throw error.
    // 5. If no color info is signalled, and input is PNG follow W3C
    //    recommendation of sRGB. For other input formats throw error.
    // 6. Convert OETF based on convert_oetf option value or as described
    //    above.
    //
    // if (options.assign_oetf != KHR_DF_TRANSFER_UNSPECIFIED) {
    //     target.format().setTransfer(options.assign_oetf);
    // } else {
    {
        // Set image's OETF as indicated by metadata.
        if (spec.format().transfer() != KHR_DF_TRANSFER_UNSPECIFIED) {
            target.format().setTransfer(spec.format().transfer());
            switch (spec.format().transfer()) {
            case KHR_DF_TRANSFER_LINEAR:
                target.decodeFunc = decode_linear;
                break;
            case KHR_DF_TRANSFER_SRGB:
                target.decodeFunc = decode_sRGB;
                break;
            case KHR_DF_TRANSFER_ITU:
                target.format().setTransfer(KHR_DF_TRANSFER_SRGB);
                target.decodeFunc = decode_bt709;
                target.encodeFunc = encode_sRGB;
                break;
            default:
                throw cant_create_image(
                        "Transfer function not supported by KTX."
                        " Use --assign_oetf to specify a different one.");
            }
        } else if (spec.format().iccProfileName().size()) {
            throw cant_create_image(
                    "It has an ICC profile. These are not supported."
                    " Use --assign_oetf to specify handling.");
        } else if (spec.format().oeGamma() >= 0.0f) {
            target.decodeFunc = decode_gamma;
            if (spec.format().oeGamma() > .45450f
                    && spec.format().oeGamma() < .45460f) {
                // N.B The previous loader matched oeGamma .45455 to the sRGB
                // OETF and did not do an OETF transformation. In this loader
                // we decode and reencode. Previous behavior can be obtained
                // with the --assign_oetf option to toktx.
                //
                // This change results in 1 bit differences in the LSB of
                // some color values noticeable only when directly comparing
                // images produced before and after this change of loader.
                target.format().setTransfer(KHR_DF_TRANSFER_SRGB);
                target.encodeFunc = encode_sRGB;
            } else if (spec.format().oeGamma() == 1.0) {
                target.format().setTransfer(KHR_DF_TRANSFER_LINEAR);
            } else if (spec.format().oeGamma() == 0.0f) {
                if (!in.formatName().compare("png")) {
                    warning("Ignoring reported gamma of 0.0f in {}. Handling as sRGB.", in.filename());
                    target.format().setTransfer(KHR_DF_TRANSFER_SRGB);
                    target.decodeFunc = decode_sRGB;
                } else {
                    throw cant_create_image("Its reported gamma is 0.0f."
                                            " Use --assign_oetf to specify handling.");
                }
            } else {
                // if (options.convert_oetf == KHR_DF_TRANSFER_UNSPECIFIED) {
                if (true) {
                    std::stringstream message;
                    message << "Its encoding gamma, "
                            << spec.format().oeGamma()
                            << ", is not automatically supported by KTX." << std::endl
                            << "Specify handling with --convert_oetf or"
                            << " --assign_oetf.";
                    throw cant_create_image(message.str());
                }
            }
        } else {
            if (!in.formatName().compare("png")) {
                // Follow W3C. Treat unspecified as sRGB.
                target.format().setTransfer(KHR_DF_TRANSFER_SRGB);
                target.decodeFunc = decode_sRGB;
            } else {
                throw cant_create_image(
                        "It has no color space information."
                        " Use --assign_oetf to specify handling.");
            }
        }
    }

    // if (options.convert_oetf != KHR_DF_TRANSFER_UNSPECIFIED &&
    //         options.convert_oetf != spec.format().transfer()) {
    //     if (options.convert_oetf == KHR_DF_TRANSFER_SRGB) {
    //         target.encodeFunc = encode_sRGB;
    //         target.format().setTransfer(KHR_DF_TRANSFER_SRGB);
    //     } else {
    //         target.encodeFunc = encode_linear;
    //         target.format().setTransfer(KHR_DF_TRANSFER_LINEAR);
    //     }
    // }
}

void CommandCreate::determineTargetTypeBitLengthScale(
        const ImageInput& in,
        TargetImageSpec& target,
        std::string& defaultSwizzle) {

    const FormatDescriptor& format = in.spec().format();
    FormatDescriptor& targetFormat = target.format();
    uint32_t bitLength = format.channelBitLength();

    // if (format.channelBitLength() > 8 && (options.etc1s || options.bopts.uastc)) {
    //     bitLength = 8;
    // } else if (format.channelBitLength() < 8) {
    if (format.channelBitLength() < 8) {
        bitLength = 8;
    }

    // Currently we only support unsigned normalized input formats.
    const uint32_t maxValue = ((1U << bitLength) - 1U);

    // TODO: Support < 8 bit channels for non-block-compressed?

    if (targetFormat.channelBitLength() != format.channelBitLength()) {
        warning("Rescaling {}-bit image in {} to {} bits.",
                format.channelBitLength(),
                in.filename(),
                targetFormat.channelBitLength());
    }

    (void) defaultSwizzle;
    // if (true) { // options.targetType != TargetType::eUnspecified
    //     channelCount = to_underlying(options.targetType);
    //     targetFormat.setModel(KHR_DF_MODEL_RGBSDA);
    // } else if (format.model() == KHR_DF_MODEL_YUVSDA) {
    //     // It's a luminance image. Override.
    //     assert(format.channelCount() < 3);
    //     targetFormat.setModel(KHR_DF_MODEL_RGBSDA);
    //     if (format.channelCount() == 1) {
    //         defaultSwizzle = "rrr1";
    //     } else {
    //         defaultSwizzle = "rrrg";
    //     }
    // }

    // Must be after setting of model.
    if (bitLength != targetFormat.channelBitLength()
            || maxValue != targetFormat.channelUpper()
            || format.channelCount() != targetFormat.channelCount()) {
        targetFormat.updateSampleInfo(channelCount, bitLength, 0, maxValue, targetFormat.channelDataType());
    }
}

void CommandCreate::determineTargetImageSpec(const ImageInput& in, TargetImageSpec& target, std::string& defaultSwizzle) {
    target = in.spec();
    determineTargetTypeBitLengthScale(in, target, defaultSwizzle);
    determineTargetColorSpace(in, target);
}

void CommandCreate::checkSpecsMatch(
        const ImageInput& currentFile,
        const ImageSpec& firstSpec) {
    const FormatDescriptor& firstFormat = firstSpec.format();
    const FormatDescriptor& currentFormat = currentFile.spec().format();

    // if (currentFormat.transfer() != firstFormat.transfer()
    //         && options.convert_oetf == KHR_DF_TRANSFER_UNSPECIFIED) {
    //     std::stringstream msg;
    //     if (options.assign_oetf == KHR_DF_TRANSFER_UNSPECIFIED) {
    //         msg << "Image";
    //     } else {
    //         msg << "Image in " << currentFile.filename() << "("
    //                 << currentFile.currentSubimage() << ","
    //                 << currentFile.currentMiplevel() << ")";
    //     }
    //     msg << " has a different transfer function (OETF)"
    //             << " than preceding image(s).";
    //     if (options.assign_oetf == KHR_DF_TRANSFER_UNSPECIFIED) {
    //         msg << std::endl
    //                 << "Use --assign_oetf (not recommended) or --convert_oetf to"
    //                 << " stop this error.";
    //         throw cant_create_image(msg.str());
    //     } else {
    //         warning(msg.str());
    //     }
    //     // Don't warn when convert_oetf is set as proper conversions
    //     // will be done so all images will be in the same space.
    // }
    //
    // if (currentFormat.primaries() != firstFormat.primaries()) {
    //     std::stringstream msg;
    //     if (options.assign_primaries == KHR_DF_PRIMARIES_UNSPECIFIED) {
    //         msg << "Image";
    //     } else {
    //         msg << "Image in " << currentFile.filename() << "("
    //                 << currentFile.currentSubimage() << ","
    //                 << currentFile.currentMiplevel() << ")";
    //     }
    //     msg << " has different primaries than preceding images(s).";
    //     if (options.assign_primaries == KHR_DF_PRIMARIES_UNSPECIFIED) {
    //         msg << std::endl
    //                 << "Use --assign_primaries (not recommended) to"
    //                 << " stop this error.";
    //         throw cant_create_image(msg.str());
    //     } else
    //         warning(msg.str());
    //     // There is no convert_primaries option.
    // }

    if (currentFormat.channelCount() != firstFormat.channelCount()) {
        warning("Image in {} ({},{}) has a different component count than preceding images(s).",
                currentFile.filename(),
                currentFile.currentSubimage(),
                currentFile.currentMiplevel());
    }
}

// void CommandCreate::setAstcMode(const TargetImageSpec& target) {
//     // If no astc mode option is specified and if input is <= 8bit
//     // default to LDR otherwise default to HDR
//     if (options.astcopts.mode == KTX_PACK_ASTC_ENCODER_MODE_DEFAULT) {
//         if (target.format().channelBitLength() <= 8)
//             options.astcopts.mode = KTX_PACK_ASTC_ENCODER_MODE_LDR;
//         else
//             options.astcopts.mode = KTX_PACK_ASTC_ENCODER_MODE_HDR;
//     } else {
//         if (target.format().channelBitLength() > 8
//                 && options.astcopts.mode == KTX_PACK_ASTC_ENCODER_MODE_LDR) {
//             // Input is > 8-bit and user wants LDR, issue quality loss warning.
//             std::stringstream msg;
//             msg << "Input file is 16-bit but ASTC LDR option is specified."
//                     << " Expect quality loss in the output." << std::endl;
//             warning(msg.str());
//         } else if (target.format().channelBitLength() < 16
//                 && options.astcopts.mode == KTX_PACK_ASTC_ENCODER_MODE_HDR) {
//             // Input is < 8bit and user wants HDR, issue warning.
//             std::stringstream msg;
//             msg << "Input file is not 16-bit but HDR option is specified." << std::endl;
//             warning(msg.str());
//         }
//     }
// }

} // namespace ktx

KTX_COMMAND_ENTRY_POINT(ktxCreate, ktx::CommandCreate)

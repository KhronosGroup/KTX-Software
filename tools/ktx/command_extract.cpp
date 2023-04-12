// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include "command.h"
#include "format_descriptor.h"
#include "formats.h"
#include "sbufstream.h"
#include "utility.h"
#include "validate.h"
#include "transcode_utils.h"
#include "image.hpp"
#include "ktx.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>

#include <cxxopts.hpp>
#include <fmt/ostream.h>
#include <fmt/printf.h>

#include "png.imageio/lodepng.h"
#include "astc-encoder/Source/tinyexr.h"

// -------------------------------------------------------------------------------------------------

namespace ktx {

// -------------------------------------------------------------------------------------------------

/** @page ktxtools_extract ktx extract
@~English

Extracts a KTX2 file.

@warning TODO Tools P5: This page is incomplete

@section ktxtools_extract_synopsis SYNOPSIS
    ktx extract [options] @e input_file

@section ktxtools_extract_description DESCRIPTION

    The following options are available:
    <dl>
        <dt>-f, --flag</dt>
        <dd>Flag description</dd>
    </dl>
    @snippet{doc} ktx/command.h command options

@section ktxtools_extract_exitstatus EXIT STATUS
    @b ktx @b extract exits
        0 - Success
        1 - Command line error
        2 - IO error
        3 - Invalid input or state

@section ktxtools_extract_history HISTORY

@par Version 4.0
 - Initial version

@section ktxtools_extract_author AUTHOR
    - Mátyás Császár [Vader], RasterGrid www.rastergrid.com
    - Daniel Rákos, RasterGrid www.rastergrid.com
*/
class CommandExtract : public Command {
    enum {
        all = -1,
    };

    struct OptionsExtract {
        std::string outputPath;
        std::string uri;
        std::optional<int32_t> level = 0; /// -1 indicates all
        std::optional<int32_t> layer = 0; /// -1 indicates all
        std::optional<int32_t> face = 0; /// -1 indicates all
        std::optional<int32_t> depth = 0; /// -1 indicates all
        bool raw = false;

        void init(cxxopts::Options& opts);
        void process(cxxopts::Options& opts, cxxopts::ParseResult& args, Reporter& report);
    };

    Combine<OptionsExtract, OptionsTranscodeTarget<false>, OptionsSingleIn, OptionsGeneric> options;

public:
    virtual int main(int argc, _TCHAR* argv[]) override;
    virtual void initOptions(cxxopts::Options& opts) override;
    virtual void processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) override;

private:
    std::size_t transcodeSwizzle(uint32_t width, uint32_t height, char* imageData, std::size_t imageSize);
    void executeExtract();
    void saveRawFile(const std::string& filepath, const char* data, std::size_t size);
    void savePNGorEXRFile(const std::string& filepath, const char* data, std::size_t size, VkFormat format, const uint32_t* dfd, uint32_t width, uint32_t height);

    void savePNG(const std::string& filepath, VkFormat vkFormat, const FormatDescriptor& format, uint32_t width, uint32_t height, LodePNGColorType colorType, const char* data, std::size_t size);
    void saveEXR(const std::string& filepath, VkFormat vkFormat, const FormatDescriptor& format, uint32_t width, uint32_t height, int pixelType, const char* data, std::size_t size);
};

// -------------------------------------------------------------------------------------------------

int CommandExtract::main(int argc, _TCHAR* argv[]) {
    try {
        parseCommandLine("ktx extract", "Export selected images from a KTX2 file.\n", argc, argv);
        executeExtract();
        return RETURN_CODE_SUCCESS;
    } catch (const FatalError& error) {
        return error.return_code;
    } catch (const std::exception& e) {
        fmt::print(std::cerr, "{} fatal: {}\n", processName, e.what());
        return RETURN_CODE_RUNTIME_ERROR;
    }
}

void CommandExtract::OptionsExtract::init(cxxopts::Options& opts) {
    opts.add_options()
            ("output", "Output filepath for single, output directory for multiple image export.", cxxopts::value<std::string>(), "<filepath>")
            ("transcode", "Target transcode format. Case-insensitive.\n"
                          "Possible options are: "
                          "etc-rgb | etc-rgba | eac-r11 | eac-rg11 | bc1 | bc3 | bc4 | bc5 | bc7 | astc | r8 | rg8 | rgb8 | rgba8",
                          cxxopts::value<std::string>(), "<target>")
            ("uri", "KTX Fragment URI.", cxxopts::value<std::string>(), "<uri>")
            ("level", "Level to extract. Defaults to 0. Case-insensitive.", cxxopts::value<std::string>(), "[0-9]+ | all")
            ("layer", "Layer to extract. Defaults to 0. Case-insensitive.", cxxopts::value<std::string>(), "[0-9]+ | all")
            ("face", "Face to extract. Defaults to 0. Case-insensitive.", cxxopts::value<std::string>(), "[0-5] | all")
            ("depth", "Depth slice to extract. Defaults to 0. Case-insensitive.", cxxopts::value<std::string>(), "[0-9]+ | all")
            ("all", "Extract all image slices.")
            ("raw", "Extract raw image data.");
}

void CommandExtract::OptionsExtract::process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter& report) {
    outputPath = args["output"].as<std::string>();

    if (args["uri"].count())
        // TODO Tools P4: Validate and parse fragment URI, Handle error conditions
        uri = args["uri"].as<std::string>();

    const auto parseSelector = [&](const std::string& name) -> std::optional<int32_t> {
        if (!args[name].count())
            return std::nullopt;
        const auto str = to_lower_copy(args[name].as<std::string>());
        try {
            return str == "all" ? all : std::stoi(str);
        } catch (const std::invalid_argument& e) {
            report.fatal(RETURN_CODE_INVALID_ARGUMENTS, "Invalid {} value \"{}\": {}.", name, str, e.what());
        } catch (const std::out_of_range& e) {
            report.fatal(RETURN_CODE_INVALID_ARGUMENTS, "Out of range {} value \"{}\": {}.", name, str, e.what());
        }
        return std::nullopt;
    };
    level = parseSelector("level");
    layer = parseSelector("layer");
    face = parseSelector("face");
    depth = parseSelector("depth");
    raw = args["raw"].as<bool>();

    if (args["all"].as<bool>()) {
        if (level)
            report.fatal(RETURN_CODE_INVALID_ARGUMENTS, "Conflicting arguments 'level' and 'all'. The level cannot be specified if the 'all' flag is set.");
        if (layer)
            report.fatal(RETURN_CODE_INVALID_ARGUMENTS, "Conflicting arguments 'layer' and 'all'. The layer cannot be specified if the 'all' flag is set.");
        if (face)
            report.fatal(RETURN_CODE_INVALID_ARGUMENTS, "Conflicting arguments 'face' and 'all'. The face cannot be specified if the 'all' flag is set.");
        if (depth)
            report.fatal(RETURN_CODE_INVALID_ARGUMENTS, "Conflicting arguments 'depth' and 'all'. The depth cannot be specified if the 'all' flag is set.");

        level = all;
        layer = all;
        face = all;
        depth = all;
    }
}

void CommandExtract::initOptions(cxxopts::Options& opts) {
    options.init(opts);
    opts.parse_positional({"input-file", "output"});
    opts.positional_help("<input-file> <output>");
}

void CommandExtract::processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) {
    options.process(opts, args, *this);

    if (!options.raw && options.transcodeTarget) {
        if (options.transcodeTarget != KTX_TTF_RGBA32)
            fatal(RETURN_CODE_INVALID_ARGUMENTS, "Transcode to \"{}\" for non-raw extract is not supported. "
                    "For PNG/EXR output only r8, rg8, rgb8 and rgba8 are supported", options.transcodeTargetName);
    }
}

std::size_t CommandExtract::transcodeSwizzle(uint32_t width, uint32_t height, char* imageData, std::size_t imageSize) {
    rgba8image srcImage(width, height, reinterpret_cast<rgba8color*>(imageData));

    switch (options.transcodeSwizzleComponents) {
    case 1: {
        // Copy in-place from RGBA8 to R8 with swizzle
        r8image dstImage(width, height, reinterpret_cast<r8color*>(imageData));
        srcImage.copyToR(dstImage, options.transcodeSwizzle);
        return imageSize / 4;
    }
    case 2: {
        // Copy in-place from RGBA8 to RG8 with swizzle
        rg8image dstImage(width, height, reinterpret_cast<rg8color*>(imageData));
        srcImage.copyToRG(dstImage, options.transcodeSwizzle);
        return imageSize / 2;
    }
    case 3: {
        // Copy in-place from RGBA8 to RGB8 with swizzle
        rgb8image dstImage(width, height, reinterpret_cast<rgb8color*>(imageData));
        srcImage.copyToRGB(dstImage, options.transcodeSwizzle);
        return imageSize * 3 / 4;
    }
    case 4: {
        // Swizzle in-place if needed
        if (options.transcodeSwizzle != "rgba") {
            srcImage.swizzle(options.transcodeSwizzle);
        }
        return imageSize;
    }
    default:
        // Nothing to do
        return imageSize;
    }
}

void CommandExtract::executeExtract() {
    std::ifstream file(options.inputFilepath, std::ios::in | std::ios::binary);
    validateToolInput(file, options.inputFilepath, *this);

    KTXTexture2 texture{nullptr};
    StreambufStream<std::streambuf*> ktx2Stream{file.rdbuf(), std::ios::in | std::ios::binary};
    auto ret = ktxTexture2_CreateFromStream(ktx2Stream.stream(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, texture.pHandle());
    if (ret != KTX_SUCCESS)
        fatal(RETURN_CODE_INVALID_FILE, "Failed to create KTX2 texture: {}", ktxErrorString(ret));

    // TODO Tools P4: Command line argument validation
    // if (options.level.value_or(0) > texture->numLevels)
    //     ■ If the "level" option is greater than the maximum level index, generate an error end
    //             exit.
    //     fatal(RETURN_CODE_INVALID_ARGUMENTS, )
    // if (options.layer && !texture->isArray)
    //     ■ If the texture type is not one of Array types and the "layer" option is set,
    //             generate an error and exit.
    // if (...)
    //     ■ If the texture type is one of Array types and the "layer" option is not set or
    //             is out of [0, layerCount) range, generate an error and exit.
    // if (...)
    //     ■ If the texture type is not Cubemap or Cubemap Array and the "face"
    //             option is set, generate an error and exit.
    // if (texture->isCubemap ...)
    //     ■ If the texture type is Cubemap or Cubemap Array and the "face" option is
    //             not set or out of [0-5] range, generate an error and exit.
    // if (...)
    //     ■ If the texture type is not 3D or 3D Array and the "depth" option is set,
    //             generate an error and exit.

    if (ktxTexture2_NeedsTranscoding(texture)) {
        options.validateTextureTranscode(texture, *this);

        ret = ktxTexture2_TranscodeBasis(texture, options.transcodeTarget.value(), 0);
        if (ret != KTX_SUCCESS)
            fatal(RETURN_CODE_INVALID_FILE, "Failed to transcode KTX2 texture: {}", ktxErrorString(ret));
    }

    const auto isMultiOutput =
            options.level == all ||
            options.face == all ||
            options.layer == all ||
            options.depth == all;

    try {
        if (isMultiOutput) {
            if (std::filesystem::exists(options.outputPath) && !std::filesystem::is_directory(options.outputPath))
                fatal(RETURN_CODE_INVALID_ARGUMENTS, "Specified output path must be a directory for multi-output extract: \"{}\".", options.outputPath);
            std::filesystem::create_directories(options.outputPath);
        } else {
            std::filesystem::create_directories(std::filesystem::path(options.outputPath).parent_path());
        }
    } catch (const std::filesystem::filesystem_error& e) {
        fatal(RETURN_CODE_IO_FAILURE, "Failed to create the output directory \"{}\": {}.", e.path1().generic_string(), e.what());
    }

    for (uint32_t levelIndex = 0; levelIndex < texture->numLevels; ++levelIndex) {
        if (options.level != all && options.level.value_or(0) != static_cast<int32_t>(levelIndex))
            continue; // Skip

        std::size_t imageSize = ktxTexture_GetImageSize(texture, levelIndex);
        const auto imageWidth = std::max(1u, texture->baseWidth >> levelIndex);
        const auto imageHeight = std::max(1u, texture->baseHeight >> levelIndex);
        const auto imageDepth = std::max(1u, texture->baseDepth >> levelIndex);

        if (options.depth != all && options.depth.value_or(0) >= static_cast<int32_t>(imageDepth))
            fatal(RETURN_CODE_INVALID_ARGUMENTS, "Requested depth slice {} on level {} is not available. The last depth slice on this level is {}.",
                    options.depth.value_or(0), levelIndex, imageDepth - 1);

        for (uint32_t faceIndex = 0; faceIndex < texture->numFaces; ++faceIndex) {
            if (options.face != all && options.face.value_or(0) != static_cast<int32_t>(faceIndex))
                continue; // Skip

            for (uint32_t layerIndex = 0; layerIndex < texture->numLayers; ++layerIndex) {
                if (options.layer != all && options.layer.value_or(0) != static_cast<int32_t>(layerIndex))
                    continue; // Skip

                if (imageDepth > 1 && !options.depth && options.raw) {
                    // If the texture type is 3D / 3D Array and the "depth" option is not set,
                    // the whole 3D block of pixel data is selected according to the "level" and "layer"
                    // option. This extraction path requires the "raw" option to be enabled.

                    const auto outputFilepath = !isMultiOutput ? options.outputPath :
                            fmt::format("{}/output{}{}{}{}.raw",
                            options.outputPath,
                            texture->numLevels > 1 ? fmt::format("_level{}", levelIndex) : "",
                            texture->isCubemap ? fmt::format("_face{}", faceIndex) : "",
                            texture->isArray ? fmt::format("_layer{}", layerIndex) : ""
                            // Depth is not part of the name as the whole 3D image is raw exported
                    );

                    std::ofstream rawFile(outputFilepath, std::ios::out | std::ios::binary);
                    if (!rawFile)
                        fatal(RETURN_CODE_IO_FAILURE, "Failed to open output file \"{}\": {}", outputFilepath, errnoMessage());

                    for (uint32_t depthIndex = 0; depthIndex < imageDepth; ++depthIndex) {
                        ktx_size_t imageOffset;
                        ktxTexture_GetImageOffset(texture, levelIndex, layerIndex, faceIndex + depthIndex, &imageOffset);
                        const char* imageData = reinterpret_cast<const char*>(texture->pData) + imageOffset;

                        // transcodeSwizzle on this branch is not required as there are no transcodable 3D formats

                        rawFile.write(imageData, imageSize);
                    }

                    if (!rawFile)
                        fatal(RETURN_CODE_IO_FAILURE, "Failed to write output file \"{}\": {}", outputFilepath, errnoMessage());

                    continue;
                }

                // Iterate z_slice_of_blocks (The code currently assumes block z size is 1)
                // TODO Tools P5: 3D-Block Compressed formats are not supported
                for (uint32_t depthIndex = 0; depthIndex < imageDepth; ++depthIndex) {
                    if (options.depth != all && static_cast<uint32_t>(options.depth.value_or(0)) != depthIndex)
                        continue; // Skip

                    ktx_size_t imageOffset;
                    ktxTexture_GetImageOffset(texture, levelIndex, layerIndex, faceIndex + depthIndex, &imageOffset);
                    char* depthSliceData = reinterpret_cast<char*>(texture->pData) + imageOffset;

                    const auto outputFilepath = !isMultiOutput ? options.outputPath :
                            fmt::format("{}/output{}{}{}{}.raw",
                            options.outputPath,
                            texture->numLevels > 1 ? fmt::format("_level{}", levelIndex) : "",
                            texture->isCubemap ? fmt::format("_face{}", faceIndex) : "",
                            texture->isArray ? fmt::format("_layer{}", layerIndex) : "",
                            texture->baseDepth > 1 ? fmt::format("_depth{}", depthIndex) : ""
                    );

                    imageSize = transcodeSwizzle(imageWidth, imageHeight, depthSliceData, imageSize);

                    if (options.raw) {
                        saveRawFile(outputFilepath, depthSliceData, imageSize);
                    } else {
                        savePNGorEXRFile(outputFilepath, depthSliceData, imageSize,
                                static_cast<VkFormat>(texture->vkFormat), texture->pDfd, imageWidth, imageHeight);
                    }
                }
            }
        }
    }

    if (ret != KTX_SUCCESS)
        fatal(RETURN_CODE_INVALID_FILE, "Failed to iterate KTX2 texture: {}", ktxErrorString(ret));
}

void CommandExtract::saveRawFile(const std::string& filepath, const char* data, std::size_t size) {
    std::ofstream file(filepath, std::ios::out | std::ios::binary);
    if (!file)
        fatal(RETURN_CODE_IO_FAILURE, "Failed to open output file \"{}\": {}.", filepath, errnoMessage());

    file.write(data, size);

    if (!file)
        fatal(RETURN_CODE_IO_FAILURE, "Failed to write output file \"{}\": {}.", filepath, errnoMessage());
}

void CommandExtract::savePNG(const std::string& filepath,
        VkFormat vkFormat, const FormatDescriptor& format, uint32_t width, uint32_t height,
        LodePNGColorType colorType,
        const char* data, std::size_t size) {

    uint32_t rOffset = 0;
    uint32_t rBits = 0;
    uint32_t gOffset = 0;
    uint32_t gBits = 0;
    uint32_t bOffset = 0;
    uint32_t bBits = 0;
    uint32_t aOffset = 0;
    uint32_t aBits = 0;

    if (format.model() == KHR_DF_MODEL_RGBSDA) {
        if (const auto sample = format.find(KHR_DF_CHANNEL_RGBSDA_R)) {
            rOffset = sample->bitOffset;
            rBits = sample->bitLength + 1;
        }
        if (const auto sample = format.find(KHR_DF_CHANNEL_RGBSDA_G)) {
            gOffset = sample->bitOffset;
            gBits = sample->bitLength + 1;
        }
        if (const auto sample = format.find(KHR_DF_CHANNEL_RGBSDA_B)) {
            bOffset = sample->bitOffset;
            bBits = sample->bitLength + 1;
        }
        if (const auto sample = format.find(KHR_DF_CHANNEL_RGBSDA_A)) {
            aOffset = sample->bitOffset;
            aBits = sample->bitLength + 1;
        }
        if (const auto sample = format.find(KHR_DF_CHANNEL_RGBSDA_D)) {
            // Use red for depth too (depth channels are exclusive for depth/stencil formats)
            rOffset = sample->bitOffset;
            rBits = sample->bitLength + 1;
        }
    // } else if (formatDescriptor.model() == KHR_DF_MODEL_YUVSDA) {
    // TODO Tools P5: Add support for KHR_DF_MODEL_YUVSDA formats
    } else {
        fatal(RETURN_CODE_UNSUPPORTED, "PNG saving is unsupported for {} with {}.", toString(format.model()), toString(vkFormat));
    }

    const auto largestBits = std::max(std::max(rBits, gBits), std::max(bBits, aBits));
    const auto bitDepth = std::max(bit_ceil(largestBits), 8u);
    const auto byteDepth = bitDepth / 8u;
    const auto pixelBits = rBits + gBits + bBits + aBits;
    const auto pixelBytes = pixelBits / 8u;
    const auto packedChannelCount = (rBits > 0 ? 1u : 0u) + (gBits > 0 ? 1u : 0u) + (bBits > 0 ? 1u : 0u) + (aBits > 0 ? 1u : 0u);
    const auto unpackedChannelCount = [&]{
        switch (colorType) {
        case LCT_GREY:
            return 1u;
        case LCT_GREY_ALPHA:
            return 2u;
        case LCT_RGB:
            return 3u;
        case LCT_RGBA:
            return 4u;
        case LCT_PALETTE: [[fallthrough]];
        case LCT_MAX_OCTET_VALUE:
            break;
        }
        assert(false);
        return 0u;
    }();
    assert(bitDepth == 8 || bitDepth == 16);
    assert(pixelBits % 8 == 0 && pixelBits <= 64);
    assert(size == width * height * pixelBytes); (void) size;

    lodepng::State state{};

    state.info_raw.bitdepth = bitDepth;
    state.info_png.color.colortype = colorType;
    state.info_png.color.bitdepth = bitDepth;
    state.info_raw.colortype = colorType;

    // Include sBit chunk if needed
    const auto includeSBits =
            (rBits != 0 && rBits != bitDepth) ||
            (gBits != 0 && gBits != bitDepth) ||
            (bBits != 0 && bBits != bitDepth) ||
            (aBits != 0 && aBits != bitDepth);
    if (includeSBits) {
        state.info_png.sbit_defined = true;
        state.info_png.sbit_r = rBits == 0 ? bitDepth : rBits;
        state.info_png.sbit_g = gBits == 0 ? bitDepth : gBits;
        state.info_png.sbit_b = bBits == 0 ? bitDepth : bBits;
        state.info_png.sbit_a = aBits == 0 ? bitDepth : aBits;
    }

    std::vector<unsigned char> unpackedImage(width * height * unpackedChannelCount * byteDepth);

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            const char* rawPixel = data + (y * width + x) * pixelBytes;

            const auto copy = [&](uint32_t c, uint32_t offset, uint32_t bits) {
                if (unpackedChannelCount > c && packedChannelCount > c) {
                    const uint32_t value = convertUNORM(extract_bits<uint32_t>(rawPixel, offset, bits), bits, bitDepth);
                    if (byteDepth == 1) {
                        uint8_t temp = static_cast<uint8_t>(value);
                        std::memcpy(unpackedImage.data() + (y * width * unpackedChannelCount + x * unpackedChannelCount + c) * byteDepth, &temp, byteDepth);
                    } else if (byteDepth == 2) {
                        uint16_t temp = static_cast<uint16_t>(value);
                        if (!is_big_endian)
                            temp = byteswap(temp); // LodePNG Uses big endian input
                        std::memcpy(unpackedImage.data() + (y * width * unpackedChannelCount + x * unpackedChannelCount + c) * byteDepth, &temp, byteDepth);
                    }
                }
            };

            copy(0, rOffset, rBits);
            copy(1, gOffset, gBits);
            copy(2, bOffset, bBits);
            copy(3, aOffset, aBits);
        }
    }

    // Include sRGB chunk if needed
    if (format.transfer() == KHR_DF_TRANSFER_SRGB) {
        state.info_png.srgb_defined = 1;
        state.info_png.srgb_intent = 0;
    }

    // Output primaries as cHRM chunk
    Primaries primaries;
    bool foundPrimaries = getPrimaries(format.primaries(), &primaries);
    if (foundPrimaries) {
        state.info_png.chrm_defined = 1;
        state.info_png.chrm_red_x = (unsigned int)(100000 * primaries.Rx);
        state.info_png.chrm_red_y = (unsigned int)(100000 * primaries.Ry);
        state.info_png.chrm_green_x = (unsigned int)(100000 * primaries.Gx);
        state.info_png.chrm_green_y = (unsigned int)(100000 * primaries.Gy);
        state.info_png.chrm_blue_x = (unsigned int)(100000 * primaries.Bx);
        state.info_png.chrm_blue_y = (unsigned int)(100000 * primaries.By);
        state.info_png.chrm_white_x = (unsigned int)(100000 * primaries.Wx);
        state.info_png.chrm_white_y = (unsigned int)(100000 * primaries.Wy);
    }

    std::vector<unsigned char> png;
    auto error = lodepng::encode(png, unpackedImage, width, height, state);
    if (error) {
        fatal(RETURN_CODE_INVALID_FILE, "PNG Encoder error {}: {}.", error, lodepng_error_text(error));
    } else {
        error = lodepng::save_file(png, filepath);
        if (error)
            fatal(RETURN_CODE_IO_FAILURE, "PNG Encoder error {}: {}.", error, lodepng_error_text(error));
    }
}

void CommandExtract::saveEXR(const std::string& filepath,
        VkFormat vkFormat, const FormatDescriptor& format, uint32_t width, uint32_t height,
        int pixelType, const char* data, std::size_t size) {

    uint32_t rOffset = 0;
    uint32_t rBits = 0;
    uint32_t gOffset = 0;
    uint32_t gBits = 0;
    uint32_t bOffset = 0;
    uint32_t bBits = 0;
    uint32_t aOffset = 0;
    uint32_t aBits = 0;

    assert(!format.samples.empty());
    bool isFloat = format.samples.front().qualifierFloat;
    bool isSigned = format.samples.front().qualifierSigned;

    if (format.model() == KHR_DF_MODEL_RGBSDA) {
        if (const auto sample = format.find(KHR_DF_CHANNEL_RGBSDA_R)) {
            rOffset = sample->bitOffset;
            rBits = sample->bitLength + 1;
        }
        if (const auto sample = format.find(KHR_DF_CHANNEL_RGBSDA_G)) {
            gOffset = sample->bitOffset;
            gBits = sample->bitLength + 1;
        }
        if (const auto sample = format.find(KHR_DF_CHANNEL_RGBSDA_B)) {
            bOffset = sample->bitOffset;
            bBits = sample->bitLength + 1;
        }
        if (const auto sample = format.find(KHR_DF_CHANNEL_RGBSDA_A)) {
            aOffset = sample->bitOffset;
            aBits = sample->bitLength + 1;
        }
        if (const auto sample = format.find(KHR_DF_CHANNEL_RGBSDA_D)) {
            // (Re)Use red for depth too (depth channels are exclusive for depth/stencil formats)
            rOffset = sample->bitOffset;
            rBits = sample->bitLength + 1;
        }
        if (const auto sample = format.find(KHR_DF_CHANNEL_RGBSDA_S)) {
            // (Re)Use green for stencil too (stencil channels are exclusive for depth/stencil formats)
            gOffset = sample->bitOffset;
            gBits = sample->bitLength + 1;
        }
    // } else if (formatDescriptor.model() == KHR_DF_MODEL_YUVSDA) {
    // TODO Tools P5: Add support for KHR_DF_MODEL_YUVSDA formats
    } else {
        fatal(RETURN_CODE_UNSUPPORTED, "PNG saving is unsupported for {} with {}.", toString(format.model()), toString(vkFormat));
    }

    const auto largestBits = std::max(std::max(rBits, gBits), std::max(bBits, aBits));
    const auto bitDepth = std::max(bit_ceil(largestBits), 8u);
    const auto pixelBits = rBits + gBits + bBits + aBits;
    const auto pixelBytes = pixelBits / 8u;
    const auto numChannels = (rBits > 0 ? 1u : 0u) + (gBits > 0 ? 1u : 0u) + (bBits > 0 ? 1u : 0u) + (aBits > 0 ? 1u : 0u);
    assert(bitDepth == 8 || bitDepth == 16 || bitDepth == 32); (void) bitDepth;
    assert(pixelBits % 8 == 0);
    assert(size == width * height * pixelBytes);

    // Either filled with floats or uint32 (half output is filled with float and converted during save)
    std::vector<std::vector<uint32_t>> images(numChannels);
    std::array<uint32_t*, 4> imagePtrs;
    imagePtrs.fill(nullptr);
    for (uint32_t i = 0; i < numChannels; ++i)
        images[i].resize(width * height);

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            const char* rawPixel = data + (y * width + x) * pixelBytes;

            const auto copy = [&](uint32_t c, uint32_t offset, uint32_t bits) {
                if (numChannels > c) {
                    const auto value = extract_bits<uint32_t>(rawPixel, offset, bits);
                    auto& target = images[c][y * width + x];

                    if (pixelType == TINYEXR_PIXELTYPE_FLOAT || pixelType == TINYEXR_PIXELTYPE_HALF) {
                        if (isFloat && isSigned) {
                            target = bit_cast<uint32_t>(covertSFloatToFloat(value, bits));
                        } else if (isFloat && !isSigned) {
                            target = bit_cast<uint32_t>(covertUFloatToFloat(value, bits));
                        } else if (!isFloat && isSigned) {
                            target = bit_cast<uint32_t>(covertSIntToFloat(value, bits));
                        } else if (!isFloat && !isSigned) {
                            target = bit_cast<uint32_t>(covertUIntToFloat(value, bits));
                        }
                    } else if (pixelType == TINYEXR_PIXELTYPE_UINT) {
                        if (isFloat && isSigned) {
                            target = covertSFloatToUInt(value, bits);
                        } else if (isFloat && !isSigned) {
                            target = covertUFloatToUInt(value, bits);
                        } else if (!isFloat && isSigned) {
                            target = covertSIntToUInt(value, bits);
                        } else if (!isFloat && !isSigned) {
                            target = covertUIntToUInt(value, bits);
                        }
                    } else
                        assert(false && "Internal error");
                }
            };

            copy(0, rOffset, rBits);
            copy(1, gOffset, gBits);
            copy(2, bOffset, bBits);
            copy(3, aOffset, aBits);
        }
    }

    struct EXRStruct {
        EXRHeader header;
        EXRImage image;
        std::vector<EXRAttribute> attributes;
        const char* err = nullptr;
        EXRStruct() {
            InitEXRHeader(&header);
            InitEXRImage(&image);
        }
        ~EXRStruct() {
            if (header.custom_attributes != nullptr) {
                free(header.custom_attributes);
                header.custom_attributes = nullptr;
                header.num_custom_attributes = 0;
            }
            image.images = nullptr; // null out the images to stop TinyEXR from trying to free them
            FreeEXRImage(&image);
            FreeEXRHeader(&header);
            FreeEXRErrorMessage(err);
        }
        void AddAttributesToHeader() {
            // Add attributes to the header
            header.num_custom_attributes = attributes.size();
            header.custom_attributes = (EXRAttribute*)malloc(sizeof(EXRAttribute) * attributes.size());
            for (size_t i = 0; i < attributes.size(); ++i) {
                header.custom_attributes[i] = attributes[i];
            }
        }
    } exr;

    exr.image.images = reinterpret_cast<unsigned char**>(imagePtrs.data());
    exr.image.width = static_cast<int>(width);
    exr.image.height = static_cast<int>(height);
    exr.image.num_channels = static_cast<int>(numChannels);

    exr.header.num_channels = static_cast<int>(numChannels);
    exr.header.channels = (EXRChannelInfo*) malloc(sizeof(EXRChannelInfo) * exr.header.num_channels);
    // TODO Tools P5: TINYEXR_COMPRESSIONTYPE_NONE
    exr.header.compression_type = TINYEXR_COMPRESSIONTYPE_NONE;
    {
        // Must be ABGR order, since most of EXR viewers expect this channel order.
        int c = 0;
        if (numChannels > 3) {
            strncpy(exr.header.channels[c].name, "A", 255);
            imagePtrs[c] = images[3].data();
            ++c;
        }
        if (numChannels > 2) {
            strncpy(exr.header.channels[c].name, "B", 255);
            imagePtrs[c] = images[2].data();
            ++c;
        }
        if (numChannels > 1) {
            strncpy(exr.header.channels[c].name, isFormatDepthStencil(vkFormat) ? "S" : "G", 255);
            imagePtrs[c] = images[1].data();
            ++c;
        }
        if (numChannels > 0) {
            strncpy(exr.header.channels[c].name, isFormatDepthStencil(vkFormat) ? "D" : "R", 255);
            imagePtrs[c] = images[0].data();
            ++c;
        }
    }

    exr.header.pixel_types = (int *)malloc(sizeof(int) * exr.header.num_channels);
    exr.header.requested_pixel_types = (int *)malloc(sizeof(int) * exr.header.num_channels);
    for (int i = 0; i < exr.header.num_channels; i++) {
        // pixel type of the input image
        exr.header.pixel_types[i] = (pixelType == TINYEXR_PIXELTYPE_UINT) ? TINYEXR_PIXELTYPE_UINT : TINYEXR_PIXELTYPE_FLOAT;
        // pixel type of the output image to be stored in .EXR file
        exr.header.requested_pixel_types[i] = pixelType;
    }

    // Output primaries as chromaticities
    Primaries primaries;
    bool foundPrimaries = getPrimaries(format.primaries(), &primaries);
    if (foundPrimaries) {
        EXRAttribute chromaticities{};
        strncpy(chromaticities.name, "chromaticities", sizeof(chromaticities.name));
        strncpy(chromaticities.type, "chromaticities", sizeof(chromaticities.type));
        chromaticities.size = sizeof(primaries);
        chromaticities.value = reinterpret_cast<unsigned char*>(&primaries);

        exr.attributes.push_back(chromaticities);
    }

    exr.AddAttributesToHeader();
    int ret = SaveEXRImageToFile(&exr.image, &exr.header, filepath.c_str(), &exr.err);
    if (ret != TINYEXR_SUCCESS)
        fatal(RETURN_CODE_IO_FAILURE, "EXR Encoder error {}: {}.", ret, exr.err);
}

void CommandExtract::savePNGorEXRFile(
        const std::string& filepath, const char* data, std::size_t size,
        VkFormat vkFormat, const uint32_t* dfd, uint32_t width, uint32_t height) {

    const auto format = createFormatDescriptor(dfd);

    switch (vkFormat) {
    case VK_FORMAT_R8_UNORM: [[fallthrough]];
    case VK_FORMAT_R8_SRGB: [[fallthrough]];
    case VK_FORMAT_R8G8_UNORM: [[fallthrough]];
    case VK_FORMAT_R8G8_SRGB: [[fallthrough]];
    case VK_FORMAT_R8G8B8_UNORM: [[fallthrough]];
    case VK_FORMAT_R8G8B8_SRGB: [[fallthrough]];
    case VK_FORMAT_B8G8R8_UNORM: [[fallthrough]];
    case VK_FORMAT_B8G8R8_SRGB:
        savePNG(filepath, vkFormat, format, width, height, LCT_RGB, data, size);
        break;

    case VK_FORMAT_R8G8B8A8_UNORM: [[fallthrough]];
    case VK_FORMAT_R8G8B8A8_SRGB: [[fallthrough]];
    case VK_FORMAT_B8G8R8A8_UNORM: [[fallthrough]];
    case VK_FORMAT_B8G8R8A8_SRGB:
        savePNG(filepath, vkFormat, format, width, height, LCT_RGBA, data, size);
        break;

    // TODO Tools P4: Extract ASTC Formats (astc decode)
    // case VK_FORMAT_ASTC_4x4_UNORM_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_4x4_SRGB_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_5x4_UNORM_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_5x4_SRGB_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_5x5_UNORM_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_5x5_SRGB_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_6x5_UNORM_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_6x5_SRGB_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_6x6_UNORM_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_6x6_SRGB_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_8x5_UNORM_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_8x5_SRGB_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_8x6_UNORM_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_8x6_SRGB_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_8x8_UNORM_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_8x8_SRGB_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_10x5_UNORM_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_10x5_SRGB_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_10x6_UNORM_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_10x6_SRGB_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_10x8_UNORM_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_10x8_SRGB_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_10x10_UNORM_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_10x10_SRGB_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_12x10_UNORM_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_12x10_SRGB_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_12x12_UNORM_BLOCK: [[fallthrough]];
    // case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
    //     if (isFormatSRGB(vkFormat)) {
    //         // astc decode sRGB mode()
    //     } else {
    //         // astc decode_unorm8()
    //     }
    //     savePNG(filepath, vkFormat, format, width, height, LCT_RGBA, data, size);
    //     break;

    case VK_FORMAT_R4G4_UNORM_PACK8: [[fallthrough]];
    case VK_FORMAT_R5G6B5_UNORM_PACK16: [[fallthrough]];
    case VK_FORMAT_B5G6R5_UNORM_PACK16:
        savePNG(filepath, vkFormat, format, width, height, LCT_RGB, data, size);
        break;

    case VK_FORMAT_R4G4B4A4_UNORM_PACK16: [[fallthrough]];
    case VK_FORMAT_B4G4R4A4_UNORM_PACK16: [[fallthrough]];
    case VK_FORMAT_R5G5B5A1_UNORM_PACK16: [[fallthrough]];
    case VK_FORMAT_B5G5R5A1_UNORM_PACK16: [[fallthrough]];
    case VK_FORMAT_A1R5G5B5_UNORM_PACK16: [[fallthrough]];
    case VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT: [[fallthrough]];
    case VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT:
        savePNG(filepath, vkFormat, format, width, height, LCT_RGBA, data, size);
        break;

    case VK_FORMAT_R10X6_UNORM_PACK16: [[fallthrough]];
    case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:
        savePNG(filepath, vkFormat, format, width, height, LCT_RGB, data, size);
        break;
    case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:
        savePNG(filepath, vkFormat, format, width, height, LCT_RGBA, data, size);
        break;

    case VK_FORMAT_R12X4_UNORM_PACK16: [[fallthrough]];
    case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:
        savePNG(filepath, vkFormat, format, width, height, LCT_RGB, data, size);
        break;
    case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:
        savePNG(filepath, vkFormat, format, width, height, LCT_RGBA, data, size);
        break;

    case VK_FORMAT_R16_UNORM: [[fallthrough]];
    case VK_FORMAT_R16G16_UNORM: [[fallthrough]];
    case VK_FORMAT_R16G16B16_UNORM:
        savePNG(filepath, vkFormat, format, width, height, LCT_RGB, data, size);
        break;

    case VK_FORMAT_R16G16B16A16_UNORM:
        savePNG(filepath, vkFormat, format, width, height, LCT_RGBA, data, size);
        break;

    case VK_FORMAT_A2R10G10B10_UNORM_PACK32: [[fallthrough]];
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        savePNG(filepath, vkFormat, format, width, height, LCT_RGBA, data, size);
        break;

    // TODO Tools P4: Extract 422 Formats
    // case VK_FORMAT_G8B8G8R8_422_UNORM: [[fallthrough]];
    // case VK_FORMAT_B8G8R8G8_422_UNORM: [[fallthrough]];
    // case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16: [[fallthrough]];
    // case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16: [[fallthrough]];
    // case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16: [[fallthrough]];
    // case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16: [[fallthrough]];
    // case VK_FORMAT_G16B16G16R16_422_UNORM: [[fallthrough]];
    // case VK_FORMAT_B16G16R16G16_422_UNORM:
    // save
    //     break;

    // EXR

    case VK_FORMAT_R8_UINT: [[fallthrough]];
    case VK_FORMAT_R8_SINT:
        saveEXR(filepath, vkFormat, format, width, height, TINYEXR_PIXELTYPE_HALF, data, size);
        break;
    case VK_FORMAT_R16_UINT: [[fallthrough]];
    case VK_FORMAT_R16_SINT:
        saveEXR(filepath, vkFormat, format, width, height, TINYEXR_PIXELTYPE_FLOAT, data, size);
        break;
    case VK_FORMAT_R32_UINT:
        saveEXR(filepath, vkFormat, format, width, height, TINYEXR_PIXELTYPE_UINT, data, size);
        break;
    case VK_FORMAT_R8G8_UINT: [[fallthrough]];
    case VK_FORMAT_R8G8_SINT:
        saveEXR(filepath, vkFormat, format, width, height, TINYEXR_PIXELTYPE_HALF, data, size);
        break;
    case VK_FORMAT_R16G16_UINT: [[fallthrough]];
    case VK_FORMAT_R16G16_SINT:
        saveEXR(filepath, vkFormat, format, width, height, TINYEXR_PIXELTYPE_FLOAT, data, size);
        break;
    case VK_FORMAT_R32G32_UINT:
        saveEXR(filepath, vkFormat, format, width, height, TINYEXR_PIXELTYPE_UINT, data, size);
        break;
    case VK_FORMAT_R8G8B8_UINT: [[fallthrough]];
    case VK_FORMAT_R8G8B8_SINT: [[fallthrough]];
    case VK_FORMAT_B8G8R8_UINT: [[fallthrough]];
    case VK_FORMAT_B8G8R8_SINT:
        saveEXR(filepath, vkFormat, format, width, height, TINYEXR_PIXELTYPE_HALF, data, size);
        break;
    case VK_FORMAT_R16G16B16_UINT: [[fallthrough]];
    case VK_FORMAT_R16G16B16_SINT:
        saveEXR(filepath, vkFormat, format, width, height, TINYEXR_PIXELTYPE_FLOAT, data, size);
        break;
    case VK_FORMAT_R32G32B32_UINT:
        saveEXR(filepath, vkFormat, format, width, height, TINYEXR_PIXELTYPE_UINT, data, size);
        break;
    case VK_FORMAT_R8G8B8A8_UINT: [[fallthrough]];
    case VK_FORMAT_R8G8B8A8_SINT: [[fallthrough]];
    case VK_FORMAT_B8G8R8A8_UINT: [[fallthrough]];
    case VK_FORMAT_B8G8R8A8_SINT:
        saveEXR(filepath, vkFormat, format, width, height, TINYEXR_PIXELTYPE_HALF, data, size);
        break;
    case VK_FORMAT_R16G16B16A16_UINT: [[fallthrough]];
    case VK_FORMAT_R16G16B16A16_SINT:
        saveEXR(filepath, vkFormat, format, width, height, TINYEXR_PIXELTYPE_FLOAT, data, size);
        break;
    case VK_FORMAT_R32G32B32A32_UINT:
        saveEXR(filepath, vkFormat, format, width, height, TINYEXR_PIXELTYPE_UINT, data, size);
        break;

    case VK_FORMAT_A2R10G10B10_UINT_PACK32: [[fallthrough]];
    case VK_FORMAT_A2R10G10B10_SINT_PACK32: [[fallthrough]];
    case VK_FORMAT_A2B10G10R10_UINT_PACK32: [[fallthrough]];
    case VK_FORMAT_A2B10G10R10_SINT_PACK32:
        saveEXR(filepath, vkFormat, format, width, height, TINYEXR_PIXELTYPE_HALF, data, size);
        break;

    case VK_FORMAT_R16_SFLOAT: [[fallthrough]];
    case VK_FORMAT_R16G16_SFLOAT: [[fallthrough]];
    case VK_FORMAT_R16G16B16_SFLOAT: [[fallthrough]];
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        saveEXR(filepath, vkFormat, format, width, height, TINYEXR_PIXELTYPE_HALF, data, size);
        break;
    case VK_FORMAT_R32_SFLOAT: [[fallthrough]];
    case VK_FORMAT_R32G32_SFLOAT: [[fallthrough]];
    case VK_FORMAT_R32G32B32_SFLOAT: [[fallthrough]];
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        saveEXR(filepath, vkFormat, format, width, height, TINYEXR_PIXELTYPE_FLOAT, data, size);
        break;

    // case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
    // TODO Tools P4: Extract B10G11R11_UFLOAT_PACK32
    // case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
    // TODO Tools P4: Extract E5B9G9R9_UFLOAT_PACK32

    case VK_FORMAT_D16_UNORM:
        savePNG(filepath, vkFormat, format, width, height, LCT_GREY, data, size);
        break;

    // case VK_FORMAT_X8_D24_UNORM_PACK32: [[fallthrough]];
    // case VK_FORMAT_D32_SFLOAT:
    //     saveEXR(filepath, vkFormat, format, width, height, TINYEXR_PIXELTYPE_FLOAT, data, size);
    //     break;
    // case VK_FORMAT_S8_UINT:
    //     saveEXR(filepath, vkFormat, format, width, height, TINYEXR_PIXELTYPE_HALF, data, size);
    //     break;
    // case VK_FORMAT_D16_UNORM_S8_UINT: [[fallthrough]];
    // case VK_FORMAT_D24_UNORM_S8_UINT: [[fallthrough]];
    // case VK_FORMAT_D32_SFLOAT_S8_UINT:
    //     saveEXR(filepath, vkFormat, format, width, height, TINYEXR_PIXELTYPE_FLOAT, TINYEXR_PIXELTYPE_HALF, data, size);
    //     break;

    default:
        fatal(RETURN_CODE_INVALID_ARGUMENTS, "Requested format conversion is not yet implemented for: {}.", toString(vkFormat));
    }
}

} // namespace ktx

KTX_COMMAND_ENTRY_POINT(ktxExtract, ktx::CommandExtract)

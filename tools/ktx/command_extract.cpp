// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include "command.h"
#include "platform_utils.h"
#include "format_descriptor.h"
#include "formats.h"
#include "fragment_uri.h"
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
#include "astc-encoder/Source/astcenc.h"

// -------------------------------------------------------------------------------------------------

namespace ktx {

// -------------------------------------------------------------------------------------------------

/** @page ktx_extract ktx extract
@~English

Extract selected images from a KTX2 file.

@section ktx_extract_synopsis SYNOPSIS
    ktx extract [option...] @e input-file @e output-path

@section ktx_extract_description DESCRIPTION
    @b ktx @b extract can extract one or multiple images from the KTX2 file specified as the
    @e input-file argument and, based on the format, save them as Raw, EXR or PNG image files
    to the @e output-path.
    If the @e input-file is '-' the file will be read from the stdin.
    If the @e output-path is '-' the output file will be written to the stdout.
    If the input file is invalid the first encountered validation error is displayed
    to the stderr and the command exits with the relevant non-zero status code.

    The @e output-path is interpreted as output filepath for single and output directory for
    multi-image extracts.
    When extracting multiple images with either '--all' or any of the 'all' args the
    following naming is used for each output file:
    <pre>output-path/output_level{}_face{}_layer{}_depth{}.extension</pre>
    - Where the @e _level{} part is only present if the source texture has more than 1 level
    - Where the @e _face{} part is only present if the source texture is cubemap or cubemap array (Cubemap)
    - Where the @e _layer{} part is only present if the source texture is an array texture (Array)
    - Where the @e _depth{} part is only present if the source texture baseDepth is more than 1 (3D)
    - Where the @e {} is replaced with the numeric index of the given component starting from 0
    - Where the @e extension part is "raw", "png" or "exr" based on the export format<br />
    Note: The inclusion of the optional parts are determined by the source texture regardless of
    which images are requested.

    For non-raw exports the output image format is chosen to be the smallest related lossless
    format:
    - _UNORM formats exported as PNG with RGB/RGBA 8/16 bit
    - _SINT/_UINT formats exported as EXR with R/RG/RGB/RGBA Half/Float/UInt
    - _SFLOAT/_UFLOAT formats exported as EXR with R/RG/RGB/RGBA Half/Float/UInt
    - D16_UNORM exported as PNG with luminance (Gray) 16 bit
    - Other Depth/Stencil formats exported as EXR with D/S/DS Half/Float

    The following options are available:
    <dl>
        <dt>\--transcode &lt;target&gt;</dt>
        <dd>Transcode the texture to the target format before executing the extract.
            Requires the input file to be transcodable (it must be either BasisLZ
            supercompressed or has UASTC color model in the DFD). This option matches the
            functionality of the @ref ktx_transcode "ktx transcode" command.
            If the target option is not set the r8, rg8, rgb8 or rgba8 target will be selected
            based on the number of channels in the input texture.
            Block compressed transcode targets can only be saved in raw format.
            Case-insensitive. Possible options are:
            etc-rgb | etc-rgba | eac-r11 | eac-rg11 | bc1 | bc3 | bc4 | bc5 | bc7 | astc |
            r8 | rg8 | rgb8 | rgba8.
            etc-rgb is ETC1; etc-rgba, eac-r11 and eac-rg11 are ETC2.
        </dd>
    </dl>
    <dl>
        <dt>\--uri &lt;uri&gt;</dt>
        <dd>KTX Fragment URI. https://registry.khronos.org/KTX/specs/2.0/ktx-frag.html
        </dd>
        <dt>\--level [0-9]+ | all</dt>
        <dd>Level to extract. When 'all' is used every level is exported. Defaults to 0.
        </dd>
        <dt>\--layer [0-9]+ | all</dt>
        <dd>Layer to extract. When 'all' is used every layer is exported. Defaults to 0.
        </dd>
        <dt>\--face [0-9]+ | all</dt>
        <dd>Face to extract. When 'all' is used every face is exported. Defaults to 0.
        </dd>
        <dt>\--depth [0-9]+ | all</dt>
        <dd>Depth slice to extract. When 'all' is used every depth is exported. Defaults to 0.
        </dd>
        <dt>\--all</dt>
        <dd>Extract every image slice from the texture.
        </dd>
        <dt>\--raw</dt>
        <dd>Extract the raw image data without any conversion.
        </dd>
    </dl>
    @snippet{doc} ktx/command.h command options_generic

@section ktx_extract_exitstatus EXIT STATUS
    @snippet{doc} ktx/command.h command exitstatus

@section ktx_extract_history HISTORY

@par Version 4.0
 - Initial version

@section ktx_extract_author AUTHOR
    - Mátyás Császár [Vader], RasterGrid www.rastergrid.com
    - Daniel Rákos, RasterGrid www.rastergrid.com
*/
class CommandExtract : public Command {
    struct OptionsExtract {
        inline static const char* kOutput = "output";
        inline static const char* kStdout = "stdout";
        inline static const char* kTranscode = "transcode";
        inline static const char* kUri = "uri";
        inline static const char* kLevel = "level";
        inline static const char* kLayer = "layer";
        inline static const char* kFace = "face";
        inline static const char* kDepth = "depth";
        inline static const char* kAll = "all";
        inline static const char* kRaw = "raw";

        std::string outputPath;
        FragmentURI fragmentURI;
        SelectorRange depth;
        bool levelFlagUsed = false;
        bool layerFlagUsed = false;
        bool faceFlagUsed = false;
        bool depthFlagUsed = false;
        bool uriFlagUsed = false;
        bool globalAll = false;
        bool raw = false;

        void init(cxxopts::Options& opts);
        void process(cxxopts::Options& opts, cxxopts::ParseResult& args, Reporter& report);
    };

    Combine<OptionsExtract, OptionsTranscodeTarget<false>, OptionsSingleIn, OptionsGeneric> options;

public:
    virtual int main(int argc, char* argv[]) override;
    virtual void initOptions(cxxopts::Options& opts) override;
    virtual void processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) override;

private:
    void executeExtract();
    void saveRawFile(std::string filepath, bool appendExtension, const char* data, std::size_t size);
    void saveImageFile(std::string filepath, bool appendExtension, const char* data, std::size_t size, VkFormat vkFormat, const FormatDescriptor& format, uint32_t width, uint32_t height);

    void savePNG(std::string filepath, bool appendExtension, VkFormat vkFormat, const FormatDescriptor& format, uint32_t width, uint32_t height, const char* data, std::size_t size);
    void saveEXR(std::string filepath, bool appendExtension, VkFormat vkFormat, const FormatDescriptor& format, uint32_t width, uint32_t height, int pixelType, const char* data, std::size_t size);
    void saveEXR(std::string filepath, bool appendExtension, VkFormat vkFormat, const FormatDescriptor& format, uint32_t width, uint32_t height, const std::vector<int>& pixelTypes, const char* data, std::size_t size);
    void decodeAndSaveASTC(std::string filepath, bool appendExtension, VkFormat vkFormat, const FormatDescriptor& format, uint32_t width, uint32_t height, const char* data, std::size_t size);
    void unpackAndSave422(std::string filepath, bool appendExtension, VkFormat vkFormat, const FormatDescriptor& format, uint32_t width, uint32_t height, const char* data, std::size_t size);
};

// -------------------------------------------------------------------------------------------------

int CommandExtract::main(int argc, char* argv[]) {
    try {
        parseCommandLine("ktx extract",
                "Extract one or multiple images from the KTX2 file specified as the input-file argument\n"
                "    and, based on the format, save them as Raw, EXR or PNG image files to the output-path.",
                argc, argv);
        executeExtract();
        return +rc::SUCCESS;
    } catch (const FatalError& error) {
        return +error.returnCode;
    } catch (const std::exception& e) {
        fmt::print(std::cerr, "{} fatal: {}\n", commandName, e.what());
        return +rc::RUNTIME_ERROR;
    }
}

void CommandExtract::OptionsExtract::init(cxxopts::Options& opts) {
    opts.add_options()
            (kOutput, "Output filepath for single, output directory for multiple image export.", cxxopts::value<std::string>(), "<filepath>")
            (kStdout, "Use stdout as the output file. (Using a single dash '-' as the output file has the same effect)")
            (kTranscode, "Transcode the texture to the target format before executing the extract steps."
                          " Requires the input file to be transcodable."
                          " Block compressed transcode targets can only be saved in raw format."
                          " Case-insensitive."
                          "\nPossible options are:"
                          " etc-rgb | etc-rgba | eac-r11 | eac-rg11 | bc1 | bc3 | bc4 | bc5 | bc7 | astc |"
                          " r8 | rg8 | rgb8 | rgba8."
                          "\netc-rgb is ETC1; etc-rgba, eac-r11 and eac-rg11 are ETC2.",
                          cxxopts::value<std::string>(), "<target>")
            (kUri, "KTX Fragment URI.", cxxopts::value<std::string>(), "<uri>")
            (kLevel, "Level to extract. When 'all' is used every level is exported. Defaults to 0.", cxxopts::value<std::string>(), "[0-9]+ | all")
            (kLayer, "Layer to extract. When 'all' is used every layer is exported. Defaults to 0.", cxxopts::value<std::string>(), "[0-9]+ | all")
            (kFace, "Face to extract. When 'all' is used every face is exported. Defaults to 0.", cxxopts::value<std::string>(), "[0-5] | all")
            (kDepth, "Depth slice to extract. When 'all' is used every depth is exported. Defaults to 0.", cxxopts::value<std::string>(), "[0-9]+ | all")
            (kAll, "Extract every image slice from the texture.")
            (kRaw, "Extract the raw image data without any conversion.");
}

void CommandExtract::OptionsExtract::process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter& report) {
    if (args.count(kOutput))
        outputPath = args[kOutput].as<std::string>();
    else
        report.fatal_usage("Missing output file or directory path.");

    const auto parseSelector = [&](const std::string& name, bool& found) -> std::optional<SelectorRange> {
        if (!args[name].count())
            return std::nullopt;
        const auto str = to_lower_copy(args[name].as<std::string>());
        try {
            found = true;
            return str == kAll ? SelectorRange(all) : SelectorRange(std::stoi(str));
        } catch (const std::invalid_argument&) {
            report.fatal_usage("Invalid {} value \"{}\". The value must be a either a number or \"all\".", name, str);
        } catch (const std::out_of_range& e) {
            report.fatal_usage("Out of range {} value \"{}\": {}.", name, str, e.what());
        }
        return std::nullopt;
    };

    auto level = parseSelector(kLevel, levelFlagUsed);
    auto layer = parseSelector(kLayer, layerFlagUsed);
    auto face = parseSelector(kFace, faceFlagUsed);
    auto depth_ = parseSelector(kDepth, depthFlagUsed);
    raw = args[kRaw].as<bool>();
    globalAll = args[kAll].as<bool>();

    if (globalAll) {
        if (level)
            report.fatal_usage("Conflicting options: --level cannot be used with --all.");
        if (layer)
            report.fatal_usage("Conflicting options: --layer cannot be used with --all.");
        if (face)
            report.fatal_usage("Conflicting options: --face cannot be used with --all.");
        if (depth_)
            report.fatal_usage("Conflicting options: --depth cannot be used with --all.");

        level = all;
        layer = all;
        face = all;
        depth_ = all;
    }

    if (globalAll && outputPath == "-")
        report.fatal_usage("stdout cannot be used with multi-output '--all' extract.");
    if (level == all && outputPath == "-")
        report.fatal_usage("stdout cannot be used with multi-output '--level all' extract.");
    if (layer == all && outputPath == "-")
        report.fatal_usage("stdout cannot be used with multi-output '--layer all' extract.");
    if (face == all && outputPath == "-")
        report.fatal_usage("stdout cannot be used with multi-output '--face all' extract.");
    if (depth_ == all && outputPath == "-")
        report.fatal_usage("stdout cannot be used with multi-output '--depth all' extract.");

    if (args[kUri].count()) {
        uriFlagUsed = true;

        if (globalAll)
            report.fatal_usage("Conflicting options: --all cannot be used with --uri.");
        if (levelFlagUsed)
            report.fatal_usage("Conflicting options: --level cannot be used with --uri.");
        if (layerFlagUsed)
            report.fatal_usage("Conflicting options: --layer cannot be used with --uri.");
        if (faceFlagUsed)
            report.fatal_usage("Conflicting options: --face cannot be used with --uri.");

        try {
            fragmentURI = parseFragmentURI(args[kUri].as<std::string>());
        } catch (const std::exception& e) {
            report.fatal_usage("Failed to parse Fragment URI: {}", e.what());
        }

        const auto isMultiOutputFragmentURI =
                (!fragmentURI.mip.is_undefined() && fragmentURI.mip.is_multi()) ||
                (!fragmentURI.stratal.is_undefined() && fragmentURI.stratal.is_multi()) ||
                (!fragmentURI.facial.is_undefined() && fragmentURI.facial.is_multi());
        if (isMultiOutputFragmentURI && outputPath == "-")
            report.fatal_usage("stdout cannot be used with multi-output '--uri' extract.");

    } else {
        // Merge every other selection method into the fragmentURI
        fragmentURI.mip = level.value_or(SelectorRange(0));
        fragmentURI.stratal = layer.value_or(SelectorRange(0));
        fragmentURI.facial = face.value_or(SelectorRange(0));
    }

    this->depth = depth_.value_or(SelectorRange(0));
}

void CommandExtract::initOptions(cxxopts::Options& opts) {
    options.init(opts);
    opts.parse_positional({"input-file", OptionsExtract::kOutput});
    opts.positional_help("<input-file> <output>");
}

void CommandExtract::processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) {
    options.process(opts, args, *this);

    if (!options.raw && options.transcodeTarget) {
        if (options.transcodeTarget != KTX_TTF_RGBA32)
            fatal_usage("Transcode to \"{}\" for non-raw extract is not supported. "
                    "For PNG/EXR output only r8, rg8, rgb8 and rgba8 are supported.", options.transcodeTargetName);
    }
}

void CommandExtract::executeExtract() {
    InputStream inputStream(options.inputFilepath, *this);
    validateToolInput(inputStream, fmtInFile(options.inputFilepath), *this);

    KTXTexture2 texture{nullptr};
    StreambufStream<std::streambuf*> ktx2Stream{inputStream->rdbuf(), std::ios::in | std::ios::binary};
    auto ret = ktxTexture2_CreateFromStream(ktx2Stream.stream(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, texture.pHandle());
    if (ret != KTX_SUCCESS)
        fatal(rc::INVALID_FILE, "Failed to create KTX2 texture: {}", ktxErrorString(ret));

    // CLI request validation
    if (!options.fragmentURI.mip.validate(texture->numLevels))
        fatal(rc::INVALID_FILE, "Requested level index {} is missing. The input file only has {} level(s).",
                options.fragmentURI.mip, texture->numLevels);

    if (((options.uriFlagUsed && !options.fragmentURI.stratal.is_undefined()) || options.layerFlagUsed) && !texture->isArray) {
        if (options.fragmentURI.stratal == all)
            fatal(rc::INVALID_FILE, "Requested all layers from a non-array texture.");
        else
            fatal(rc::INVALID_FILE, "Requested layer index {} from a non-array texture.", options.fragmentURI.stratal);
    }

    if (!options.fragmentURI.stratal.validate(texture->numLayers))
        fatal(rc::INVALID_FILE, "Requested layer index {} is missing. The input file only has {} layer(s).",
                options.fragmentURI.stratal, texture->numLayers);

    if (((options.uriFlagUsed && !options.fragmentURI.facial.is_undefined()) || options.faceFlagUsed) && !texture->isCubemap) {
        if (options.fragmentURI.facial == all)
            fatal(rc::INVALID_FILE, "Requested all faces from a non-cubemap texture.");
        else
            fatal(rc::INVALID_FILE, "Requested face index {} from a non-cubemap texture.", options.fragmentURI.facial);
    }

    if (!options.fragmentURI.facial.validate(texture->numFaces))
        fatal(rc::INVALID_FILE, "Requested face index {} is missing. The input file only has {} face(s).",
                options.fragmentURI.facial, texture->numFaces);

    if (!options.globalAll && options.depthFlagUsed && texture->numDimensions != 3) {
        if (options.depth == all)
            fatal(rc::INVALID_FILE, "Requested all depth slices from a non-3D texture.");
        else
            fatal(rc::INVALID_FILE, "Requested depth slice index {} from a non-3D texture.", options.depth);
    }

    // Transcoding
    if (ktxTexture2_NeedsTranscoding(texture)) {
        texture = transcode(std::move(texture), options, *this);

    } else if (options.transcodeTarget) {
        fatal(rc::INVALID_FILE, "Requested transcode \"{}\" but the KTX file is not transcodable.",
                options.transcodeTargetName);
    }

    const auto format = createFormatDescriptor(texture->pDfd);
    const auto blockSizeZ = format.basic.texelBlockDimension2 + 1u;

    const auto lastExportedLevel = options.fragmentURI.mip == all ? texture->numLevels - 1 : options.fragmentURI.mip.last();
    const auto lastExportedLevelDepthCount = std::max(1u, ceil_div(texture->baseDepth, blockSizeZ) >> lastExportedLevel);
    if (options.depthFlagUsed && options.depth != all && options.depth.last() > lastExportedLevelDepthCount)
        fatal(rc::INVALID_FILE, "Requested depth slice index {} is missing. The input file only has {} depth slice(s) in level {}.",
                options.depth, lastExportedLevelDepthCount, lastExportedLevel);

    // Setup output directory
    const auto isMultiOutput =
            (!options.fragmentURI.mip.is_undefined() && options.fragmentURI.mip.is_multi()) ||
            (!options.fragmentURI.stratal.is_undefined() && options.fragmentURI.stratal.is_multi()) ||
            (!options.fragmentURI.facial.is_undefined() && options.fragmentURI.facial.is_multi()) ||
            ((options.globalAll || options.depthFlagUsed) && options.depth.is_multi());
    try {
        const auto outputPath = std::filesystem::path(DecodeUTF8Path(options.outputPath));
        if (isMultiOutput) {
            if (std::filesystem::exists(outputPath) && !std::filesystem::is_directory(outputPath))
                fatal_usage("Specified output path must be a directory for multi-output extract: \"{}\".", options.outputPath);
            std::filesystem::create_directories(outputPath);
        } else {
            if (outputPath.has_parent_path())
                std::filesystem::create_directories(outputPath.parent_path());
        }
    } catch (const std::filesystem::filesystem_error& e) {
        fatal(rc::IO_FAILURE, "Failed to create the output directory \"{}\": {}.", e.path1().generic_string(), e.what());
    }

    // Iterate
    for (uint32_t levelIndex = 0; levelIndex < texture->numLevels; ++levelIndex) {
        if (options.fragmentURI.mip.is_undefined() ?
                levelIndex != 0 :
                !options.fragmentURI.mip.contains(levelIndex))
            continue;

        std::size_t imageSize = ktxTexture_GetImageSize(texture, levelIndex);
        const auto imageWidth = std::max(1u, texture->baseWidth >> levelIndex);
        const auto imageHeight = std::max(1u, texture->baseHeight >> levelIndex);
        const auto imageDepth = std::max(1u, texture->baseDepth >> levelIndex);

        for (uint32_t faceIndex = 0; faceIndex < texture->numFaces; ++faceIndex) {
            if (options.fragmentURI.facial.is_undefined() ?
                    faceIndex != 0 :
                    !options.fragmentURI.facial.contains(faceIndex))
                continue;

            for (uint32_t layerIndex = 0; layerIndex < texture->numLayers; ++layerIndex) {
                if (options.fragmentURI.stratal.is_undefined() ?
                        layerIndex != 0 :
                        !options.fragmentURI.stratal.contains(layerIndex))
                    continue;

                if (imageDepth > 1 && !options.globalAll && !options.depthFlagUsed && options.raw) {
                    // If the texture type is 3D / 3D Array and the "all" or "depth" option is not set,
                    // the whole 3D block of pixel data is selected according to the "level" and "layer"
                    // option. This extraction path requires the "raw" option to be enabled.

                    const auto outputFilepath = !isMultiOutput ? options.outputPath :
                            fmt::format("{}/output{}{}{}.raw",
                            options.outputPath,
                            texture->numLevels > 1 ? fmt::format("_level{}", levelIndex) : "",
                            texture->isCubemap ? fmt::format("_face{}", faceIndex) : "",
                            texture->isArray ? fmt::format("_layer{}", layerIndex) : ""
                            // Depth is not part of the name as the whole 3D image is raw exported
                    );

                    OutputStream file(outputFilepath, *this);
                    for (uint32_t depthIndex = 0; depthIndex < imageDepth; ++depthIndex) {
                        ktx_size_t imageOffset;
                        ktxTexture_GetImageOffset(texture, levelIndex, layerIndex, faceIndex + depthIndex, &imageOffset);
                        const char* imageData = reinterpret_cast<const char*>(texture->pData) + imageOffset;
                        file.write(imageData, imageSize, *this);
                    }

                    continue;
                }

                // Iterate z_slice_of_blocks
                for (uint32_t depthIndex = 0; depthIndex < ceil_div(imageDepth, blockSizeZ); ++depthIndex) {
                    if (!options.depth.contains(depthIndex))
                        continue; // Skip

                    ktx_size_t imageOffset;
                    ktxTexture_GetImageOffset(texture, levelIndex, layerIndex, faceIndex + depthIndex, &imageOffset);
                    char* depthSliceData = reinterpret_cast<char*>(texture->pData) + imageOffset;

                    const auto outputFilepath = !isMultiOutput ? options.outputPath :
                            fmt::format("{}/output{}{}{}{}",
                            options.outputPath,
                            texture->numLevels > 1 ? fmt::format("_level{}", levelIndex) : "",
                            texture->isCubemap ? fmt::format("_face{}", faceIndex) : "",
                            texture->isArray ? fmt::format("_layer{}", layerIndex) : "",
                            texture->baseDepth > 1 ? fmt::format("_depth{}", depthIndex) : ""
                    );

                    if (options.raw) {
                        saveRawFile(outputFilepath, isMultiOutput, depthSliceData, imageSize);
                    } else {
                        saveImageFile(outputFilepath, isMultiOutput, depthSliceData, imageSize,
                                static_cast<VkFormat>(texture->vkFormat), format, imageWidth, imageHeight);
                    }
                }
            }
        }
    }

    if (ret != KTX_SUCCESS)
        fatal(rc::INVALID_FILE, "Failed to iterate KTX2 texture: {}", ktxErrorString(ret));
}

void CommandExtract::decodeAndSaveASTC(std::string filepath, bool appendExtension, VkFormat vkFormat, const FormatDescriptor& format,
        uint32_t width, uint32_t height, const char* compressedData, std::size_t compressedSize) {

    const auto threadCount = 1u;
    const auto blockSizeX = format.basic.texelBlockDimension0 + 1u;
    const auto blockSizeY = format.basic.texelBlockDimension1 + 1u;
    const auto blockSizeZ = format.basic.texelBlockDimension2 + 1u;
    static constexpr astcenc_swizzle swizzle{ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A};

    astcenc_error ec = ASTCENC_SUCCESS;

    const astcenc_profile profile = isFormatSRGB(vkFormat) ? ASTCENC_PRF_LDR_SRGB : ASTCENC_PRF_LDR;
    astcenc_config config{};
    ec = astcenc_config_init(profile, blockSizeX, blockSizeY, blockSizeZ, ASTCENC_PRE_MEDIUM, ASTCENC_FLG_DECOMPRESS_ONLY, &config);
    if (ec != ASTCENC_SUCCESS)
        fatal(rc::RUNTIME_ERROR, "ASTC Codec config init failed: {}", astcenc_get_error_string(ec));

    struct ASTCencStruct {
        astcenc_context* context = nullptr;
        ~ASTCencStruct() {
            astcenc_context_free(context);
        }
    } astcenc;
    astcenc_context*& context = astcenc.context;

    ec = astcenc_context_alloc(&config, threadCount, &context);
    if (ec != ASTCENC_SUCCESS)
        fatal(rc::RUNTIME_ERROR, "ASTC Codec context alloc failed: {}", astcenc_get_error_string(ec));

    astcenc_image image{};
    image.dim_x = width;
    image.dim_y = height;
    image.dim_z = 1; // 3D ASTC formats are currently not supported
    const auto uncompressedSize = width * height * 4 * sizeof(uint8_t);
    const auto uncompressedBuffer = std::make_unique<uint8_t[]>(uncompressedSize);
    auto* bufferPtr = uncompressedBuffer.get();
    image.data = reinterpret_cast<void**>(&bufferPtr);
    image.data_type = ASTCENC_TYPE_U8;

    ec = astcenc_decompress_image(context, reinterpret_cast<const uint8_t*>(compressedData), compressedSize, &image, &swizzle, 0);
    if (ec != ASTCENC_SUCCESS)
        fatal(rc::RUNTIME_ERROR, "ASTC Codec decompress failed: {}", astcenc_get_error_string(ec));
    astcenc_decompress_reset(context);

    const auto uncompressedVkFormat = isFormatSRGB(vkFormat) ?
            VK_FORMAT_R8G8B8A8_SRGB :
            VK_FORMAT_R8G8B8A8_UNORM;
    saveImageFile(
            std::move(filepath),
            appendExtension,
            reinterpret_cast<const char*>(uncompressedBuffer.get()),
            uncompressedSize,
            uncompressedVkFormat,
            createFormatDescriptor(uncompressedVkFormat, *this),
            width,
            height);
}

using namespace imageio;

void CommandExtract::unpackAndSave422(std::string filepath, bool appendExtension,
        VkFormat vkFormat, const FormatDescriptor& format, uint32_t width, uint32_t height,
        const char* data, std::size_t size) {
    (void) vkFormat;

    assert(format.basic.model == KHR_DF_MODEL_YUVSDA);
    assert(format.find(KHR_DF_CHANNEL_YUVSDA_Y));
    // Create a custom format with the same precision but with only 3 channels
    // Reuse similar 4 channel VkFormats and drop the last channel (There is no RGB variant of 10X6 and 12X4)
    const auto precision = format.find(KHR_DF_CHANNEL_YUVSDA_Y)->bitLength + 1u;
    auto unpackedFormat = createFormatDescriptor(
            precision == 8 ? VK_FORMAT_R8G8B8A8_UNORM :
            precision == 10 ? VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16 :
            precision == 12 ? VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16 :
            precision == 16 ? VK_FORMAT_R16G16B16A16_UNORM :
            VK_FORMAT_UNDEFINED, *this);
    unpackedFormat.removeLastChannel();

    // 1 pixel (block) with 4 channel is unpacked to 2 pixel with 3 channels: Y0,Y1,U,V -> R0,G0,B0,R1,G1,B1
    const auto blockYUVBytes = format.pixelByteCount();
    const auto blockDimensionX = format.basic.texelBlockDimension0 + 1;
    const auto blockDimensionY = format.basic.texelBlockDimension1 + 1;
    const auto pixelBytes = unpackedFormat.pixelByteCount();
    const auto channelBytes = pixelBytes / 3;
    assert(format.sampleCount() == 4);
    assert(format.basic.texelBlockDimension0 + 1 == 2);
    assert(format.basic.texelBlockDimension1 + 1 == 1);
    assert(format.basic.texelBlockDimension2 + 1 == 1);
    assert(format.basic.texelBlockDimension3 + 1 == 1);
    assert(size == width * height * blockYUVBytes / blockDimensionX / blockDimensionY);
    (void) size;
    (void) blockDimensionY;

    uint32_t y0Offset = 0;
    uint32_t y0Bits = 0;
    uint32_t y0PositionX = 0;
    uint32_t y1Offset = 0;
    uint32_t y1Bits = 0;
    uint32_t y1PositionX = 0;
    uint32_t uOffset = 0;
    uint32_t uBits = 0;
    uint32_t uPositionX = 0;
    uint32_t vOffset = 0;
    uint32_t vBits = 0;
    uint32_t vPositionX = 0;

    for (const auto& sample : format.samples) {
        switch (sample.channelType) {
        case KHR_DF_CHANNEL_YUVSDA_Y:
            if (y0Bits != 0) {
                y1Offset = sample.bitOffset;
                y1Bits = sample.bitLength + 1;
                y1PositionX = sample.samplePosition0;
            } else {
                y0Offset = sample.bitOffset;
                y0Bits = sample.bitLength + 1;
                y0PositionX = sample.samplePosition0;
            }
            break;
        case KHR_DF_CHANNEL_YUVSDA_U:
            uOffset = sample.bitOffset;
            uBits = sample.bitLength + 1;
            uPositionX = sample.samplePosition0;
            break;
        case KHR_DF_CHANNEL_YUVSDA_V:
            vOffset = sample.bitOffset;
            vBits = sample.bitLength + 1;
            vPositionX = sample.samplePosition0;
            break;
        default:
            assert(false && "Unsupported channel type");
        }
    }
    if (y0PositionX > y1PositionX) {
        // Ensure that y0 (as we refer to) is the left sample
        std::swap(y0Offset, y1Offset);
        std::swap(y0Bits, y1Bits);
        std::swap(y0PositionX, y1PositionX);
    }

    assert(precision == y0Bits);
    assert(precision == y1Bits);
    assert(precision == uBits);
    assert(precision == vBits);

    const float positionY0 = static_cast<float>(y0PositionX * blockDimensionX) / 256.f;
    const float positionY1 = static_cast<float>(y1PositionX * blockDimensionX) / 256.f;
    const float positionU = static_cast<float>(uPositionX * blockDimensionX) / 256.f;
    const float positionV = static_cast<float>(vPositionX * blockDimensionX) / 256.f;
    const float blockSize = static_cast<float>(blockDimensionX);

    std::vector<char> unpackedData(width * height * pixelBytes);

    const auto blockCountX = width / blockDimensionX;
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < blockCountX; x++) {
            const char* rawYUVBlockLeft = data + (y * blockCountX + (x == 0 ? 0 : x - 1)) * blockYUVBytes;
            const char* rawYUVBlock = data + (y * blockCountX + x) * blockYUVBytes;
            const char* rawYUVBlockRight = data + (y * blockCountX + (x == blockCountX - 1 ? blockCountX - 1 : x + 1)) * blockYUVBytes;

            char* pixel0 = unpackedData.data() + (y * width + x * 2) * pixelBytes;
            char* pixel1 = unpackedData.data() + (y * width + x * 2 + 1) * pixelBytes;

            const float valueLeftY1 = convertUNORMToFloat(extract_bits<uint32_t>(rawYUVBlockLeft, y1Offset, y1Bits), y1Bits);
            const float valueLeftU = convertUNORMToFloat(extract_bits<uint32_t>(rawYUVBlockLeft, uOffset, uBits), uBits);
            const float valueLeftV = convertUNORMToFloat(extract_bits<uint32_t>(rawYUVBlockLeft, vOffset, vBits), vBits);
            const float valueY0 = convertUNORMToFloat(extract_bits<uint32_t>(rawYUVBlock, y0Offset, y0Bits), y0Bits);
            const float valueY1 = convertUNORMToFloat(extract_bits<uint32_t>(rawYUVBlock, y1Offset, y1Bits), y1Bits);
            const float valueU = convertUNORMToFloat(extract_bits<uint32_t>(rawYUVBlock, uOffset, uBits), uBits);
            const float valueV = convertUNORMToFloat(extract_bits<uint32_t>(rawYUVBlock, vOffset, vBits), vBits);
            const float valueRightY0 = convertUNORMToFloat(extract_bits<uint32_t>(rawYUVBlockRight, y0Offset, y0Bits), y0Bits);
            const float valueRightU = convertUNORMToFloat(extract_bits<uint32_t>(rawYUVBlockRight, uOffset, uBits), uBits);
            const float valueRightV = convertUNORMToFloat(extract_bits<uint32_t>(rawYUVBlockRight, vOffset, vBits), vBits);

            const auto interpolateY = [](float pos, float pos0, float value0, float pos1, float value1, float pos2, float value2, float pos3, float value3) {
                if (pos < pos1)
                    return remap(pos, pos0, pos1, value0, value1);
                else if (pos < pos2)
                    return remap(pos, pos1, pos2, value1, value2);
                else
                    return remap(pos, pos2, pos3, value2, value3);
            };

            const auto interpolateUV = [](float pos, float pos0, float value0, float pos1, float value1, float pos2, float value2) {
                if (pos < pos1)
                    return remap(pos, pos0, pos1, value0, value1);
                else
                    return remap(pos, pos1, pos2, value1, value2);
            };

            const auto setPixel = [&](auto* pixel, float pos) {
                const auto r = convertFloatToUNORM(interpolateUV(pos,
                        positionV - blockSize, valueLeftV,
                        positionV, valueV,
                        positionV + blockSize, valueRightV), uBits);
                const auto g = convertFloatToUNORM(interpolateY(
                        pos,
                        positionY1 - blockSize, valueLeftY1,
                        positionY0, valueY0,
                        positionY1, valueY1,
                        positionY0 + blockSize, valueRightY0), y0Bits);
                const auto b = convertFloatToUNORM(interpolateUV(pos,
                        positionU - blockSize, valueLeftU,
                        positionU, valueU,
                        positionU + blockSize, valueRightU), uBits);

                const auto offsetToUsedBytes = is_big_endian ? sizeof(r) - channelBytes : 0u;
                std::memcpy(pixel + 0 * channelBytes, &r + offsetToUsedBytes, channelBytes);
                std::memcpy(pixel + 1 * channelBytes, &g + offsetToUsedBytes, channelBytes);
                std::memcpy(pixel + 2 * channelBytes, &b + offsetToUsedBytes, channelBytes);
            };

            setPixel(pixel0, 0.5f);
            setPixel(pixel1, 1.5f);
        }
    }

    // Save unpacked data as RGBSDA with the custom format
    savePNG(filepath, appendExtension, VK_FORMAT_UNDEFINED, unpackedFormat, width, height,
            reinterpret_cast<const char*>(unpackedData.data()),
            unpackedData.size() * sizeof(decltype(unpackedData)::value_type));
}

void CommandExtract::saveRawFile(std::string filepath, bool appendExtension, const char* data, std::size_t size) {
    if (appendExtension && filepath != "-")
        filepath += ".raw";
    OutputStream file(filepath, *this);
    file.write(data, size, *this);
}

void CommandExtract::savePNG(std::string filepath, bool appendExtension,
        VkFormat vkFormat, const FormatDescriptor& format, uint32_t width, uint32_t height,
        const char* data, std::size_t size) {
    if (appendExtension && filepath != "-")
        filepath += ".png";

    const auto depthFormat = (vkFormat == VK_FORMAT_D16_UNORM);
    const auto alphaFormat = (vkFormat == VK_FORMAT_A8_UNORM_KHR);

    uint32_t packedChannelCount = 0;
    uint32_t unpackedChannelCount = 3;
    LodePNGColorType colorType = LCT_RGB;

    uint32_t bitDepth = 8;

    uint32_t srcOffsets[4] = {};
    uint32_t srcBits[4] = {};
    uint32_t dstChannels[4] = {};

    auto addChannel = [&](uint32_t dstChannel, const FormatDescriptor::sample* sample) {
        srcOffsets[packedChannelCount] = sample->bitOffset;
        srcBits[packedChannelCount] = sample->bitLength + 1;
        dstChannels[packedChannelCount] = dstChannel;
        packedChannelCount++;

        if (sample->bitLength >= 8) {
            bitDepth = 16;
        }
    };

    lodepng::State state{};

    if (format.model() != KHR_DF_MODEL_RGBSDA)
        fatal(rc::NOT_SUPPORTED, "PNG saving is not supported for {} with {}.", toString(format.model()), toString(vkFormat));

    if (const auto sample = format.find(KHR_DF_CHANNEL_RGBSDA_R)) {
        addChannel(0, sample);
    }
    if (const auto sample = format.find(KHR_DF_CHANNEL_RGBSDA_G)) {
        addChannel(1, sample);
    }
    if (const auto sample = format.find(KHR_DF_CHANNEL_RGBSDA_B)) {
        addChannel(2, sample);
    }
    if (const auto sample = format.find(KHR_DF_CHANNEL_RGBSDA_A)) {
        if (alphaFormat) {
            // For alpha-only formats we use grey-alpha
            unpackedChannelCount = 2;
            colorType = LCT_GREY_ALPHA;
            addChannel(1, sample);
        } else {
            unpackedChannelCount = 4;
            colorType = LCT_RGBA;
            addChannel(3, sample);
        }
    }
    if (const auto sample = format.find(KHR_DF_CHANNEL_RGBSDA_D)) {
        if (!depthFormat)
            fatal(rc::NOT_SUPPORTED, "PNG saving encountered unexpected depth channel in non-depth format {}.", toString(vkFormat));

        // For depth-only formats we use grey
        unpackedChannelCount = 1;
        colorType = LCT_GREY;
        addChannel(0, sample);
    }

    const auto byteDepth = bitDepth / 8u;
    const auto pixelBytes = format.pixelByteCount();
    assert(bitDepth == 8 || bitDepth == 16);
    assert(size == width * height * pixelBytes); (void) size;

    state.info_raw.bitdepth = bitDepth;
    state.info_png.color.colortype = colorType;
    state.info_png.color.bitdepth = bitDepth;
    state.info_raw.colortype = colorType;

    // Include sBit chunk if needed
    const auto includeSBits =
            (srcBits[0] != 0 && srcBits[0] != bitDepth) ||
            (srcBits[1] != 0 && srcBits[1] != bitDepth) ||
            (srcBits[2] != 0 && srcBits[2] != bitDepth) ||
            (srcBits[3] != 0 && srcBits[3] != bitDepth);
    if (includeSBits) {
        state.info_png.sbit_defined = true;
        state.info_png.sbit_r = srcBits[0] == 0 ? bitDepth : srcBits[0];
        state.info_png.sbit_g = srcBits[1] == 0 ? bitDepth : srcBits[1];
        state.info_png.sbit_b = srcBits[2] == 0 ? bitDepth : srcBits[2];
        state.info_png.sbit_a = srcBits[3] == 0 ? bitDepth : srcBits[3];
    }

    std::vector<unsigned char> unpackedImage(width * height * unpackedChannelCount * byteDepth);

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            const char* rawPixel = data + (y * width + x) * pixelBytes;

            for (uint32_t packedChannelIndex = 0; packedChannelIndex < packedChannelCount; ++packedChannelIndex) {
                uint32_t offset = srcOffsets[packedChannelIndex];
                uint32_t bits = srcBits[packedChannelIndex];
                uint32_t dstChannel = dstChannels[packedChannelIndex];
                const uint32_t value = convertUNORM(extract_bits<uint32_t>(rawPixel, offset, bits), bits, bitDepth);
                if (byteDepth == 1) {
                    uint8_t temp = static_cast<uint8_t>(value);
                    std::memcpy(unpackedImage.data() + (y * width * unpackedChannelCount + x * unpackedChannelCount + dstChannel) * byteDepth, &temp, byteDepth);
                } else if (byteDepth == 2) {
                    uint16_t temp = static_cast<uint16_t>(value);
                    if (!is_big_endian)
                        temp = byteswap(temp); // LodePNG Uses big endian input
                    std::memcpy(unpackedImage.data() + (y * width * unpackedChannelCount + x * unpackedChannelCount + dstChannel) * byteDepth, &temp, byteDepth);
                }
            }
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
    if (error)
        fatal(rc::INVALID_FILE, "PNG Encoder error {}: {}.", error, lodepng_error_text(error));

    OutputStream file(filepath, *this);
    file.write(reinterpret_cast<const char*>(png.data()), png.size(), *this);
}

void CommandExtract::saveEXR(std::string filepath, bool appendExtension,
        VkFormat vkFormat, const FormatDescriptor& format, uint32_t width, uint32_t height,
        int pixelType, const char* data, std::size_t size) {
    saveEXR(std::move(filepath), appendExtension, vkFormat, format, width, height,
            std::vector<int>(vkFormat == VK_FORMAT_E5B9G9R9_UFLOAT_PACK32 ? 3 : format.channelCount(), pixelType),
            data, size);
}

void CommandExtract::saveEXR(std::string filepath, bool appendExtension,
        VkFormat vkFormat, const FormatDescriptor& format, uint32_t width, uint32_t height,
        const std::vector<int>& pixelTypes, const char* data, std::size_t size) {
    assert(!format.samples.empty());

    if (appendExtension && filepath != "-")
        filepath += ".exr";

    struct Channel {
        uint32_t offset;
        uint32_t bits;
        const char* name;
        bool isFloat;
        bool isSigned;
        bool isNormalized;
    };
    std::vector<Channel> channels;

    if (format.model() == KHR_DF_MODEL_RGBSDA) {
        const auto addChannel = [&](auto channelType, const char* channelName) {
            if (const auto sample = format.find(channelType))
                channels.push_back({
                        sample->bitOffset,
                        sample->bitLength + 1u,
                        channelName,
                        sample->qualifierFloat != 0,
                        sample->qualifierSigned != 0,
                        sample->upper != (sample->qualifierFloat != 0 ? bit_cast<uint32_t>(1.0f) : 1u)
                });
        };

        // Must be ABGR order, since most of EXR viewers expect this channel order by convention.
        addChannel(KHR_DF_CHANNEL_RGBSDA_A, "A");
        addChannel(KHR_DF_CHANNEL_RGBSDA_D, "D");
        addChannel(KHR_DF_CHANNEL_RGBSDA_S, "S");
        addChannel(KHR_DF_CHANNEL_RGBSDA_B, "B");
        addChannel(KHR_DF_CHANNEL_RGBSDA_G, "G");
        addChannel(KHR_DF_CHANNEL_RGBSDA_R, "R");
    // } else if (formatDescriptor.model() == KHR_DF_MODEL_YUVSDA) {
    // Other color model support would come here
    } else {
        fatal(rc::NOT_SUPPORTED, "EXR saving is unsupported for {} with {}.", toString(format.model()), toString(vkFormat));
    }

    const auto pixelBytes = format.pixelByteCount();
    const auto numChannels = vkFormat == VK_FORMAT_E5B9G9R9_UFLOAT_PACK32 ? 3 : format.channelCount();
    assert(size == width * height * pixelBytes); (void) size;
    assert(numChannels == pixelTypes.size());

    // Image data is prepared with either floats or uint32
    // (half output is will be converted from float by TinyEXR during save)
    std::vector<std::vector<uint32_t>> images(numChannels);
    std::array<uint32_t*, 4> imagePtrs{};
    for (uint32_t i = 0; i < numChannels; ++i)
        images[i].resize(width * height);

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            const char* rawPixel = data + (y * width + x) * pixelBytes;

            if (vkFormat == VK_FORMAT_E5B9G9R9_UFLOAT_PACK32) {
                // Special case for VK_FORMAT_E5B9G9R9_UFLOAT_PACK32
                assert(numChannels == 3);
                assert(pixelTypes[0] == TINYEXR_PIXELTYPE_HALF);
                uint32_t pixel;
                std::memcpy(&pixel, rawPixel, sizeof(uint32_t));
                const auto values = glm::unpackF3x9_E1x5(pixel);
                images[2][y * width + x] = bit_cast<uint32_t>(values.r);
                images[1][y * width + x] = bit_cast<uint32_t>(values.g);
                images[0][y * width + x] = bit_cast<uint32_t>(values.b);
            } else {
                for (uint32_t c = 0; c < numChannels; c++) {
                    const auto& channel = channels[c];
                    const auto offset = channel.offset;
                    const auto bits = channel.bits;

                    const auto value = extract_bits<uint32_t>(rawPixel, offset, bits);
                    auto& target = images[c][y * width + x];

                    if (pixelTypes[c] == TINYEXR_PIXELTYPE_FLOAT || pixelTypes[c] == TINYEXR_PIXELTYPE_HALF) {
                        if (channel.isFloat) {
                            if (channel.isSigned)
                                target = bit_cast<uint32_t>(convertSFloatToFloat(value, bits));
                            else
                                target = bit_cast<uint32_t>(convertUFloatToFloat(value, bits));
                        } else {
                            if (channel.isNormalized) {
                                if (channel.isSigned)
                                    target = bit_cast<uint32_t>(convertSNORMToFloat(value, bits));
                                else
                                    target = bit_cast<uint32_t>(convertUNORMToFloat(value, bits));
                            } else {
                                if (channel.isSigned)
                                    target = bit_cast<uint32_t>(convertSIntToFloat(value, bits));
                                else
                                    target = bit_cast<uint32_t>(convertUIntToFloat(value, bits));
                            }
                        }
                    } else if (pixelTypes[c] == TINYEXR_PIXELTYPE_UINT) {
                        if (channel.isFloat && channel.isSigned) {
                            target = convertSFloatToUInt(value, bits);
                        } else if (channel.isFloat && !channel.isSigned) {
                            target = convertUFloatToUInt(value, bits);
                        } else if (!channel.isFloat && channel.isSigned) {
                            target = convertSIntToUInt(value, bits);
                        } else if (!channel.isFloat && !channel.isSigned) {
                            target = convertUIntToUInt(value, bits);
                        }
                    } else
                        assert(false && "Internal error");
                }
            }
        }
    }

    struct EXRStruct {
        EXRHeader header;
        EXRImage image;
        std::vector<EXRAttribute> attributes;
        const char* err = nullptr;
        unsigned char* fileData = nullptr;
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
            free(fileData);
        }
        void addAttributesToHeader() {
            header.num_custom_attributes = static_cast<int>(attributes.size());
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
    // TODO: Question: Should we use a compression for exr outputs?
    exr.header.compression_type = TINYEXR_COMPRESSIONTYPE_NONE;

    for (uint32_t c = 0; c < numChannels; c++) {
        const auto& channel = channels[c];
        strncpy(exr.header.channels[c].name, channel.name, 255);
        imagePtrs[c] = images[c].data();
    }

    exr.header.pixel_types = (int *)malloc(sizeof(int) * exr.header.num_channels);
    exr.header.requested_pixel_types = (int *)malloc(sizeof(int) * exr.header.num_channels);
    for (int i = 0; i < exr.header.num_channels; i++) {
        // pixel type of the input image
        exr.header.pixel_types[i] = (pixelTypes[i] == TINYEXR_PIXELTYPE_UINT) ? TINYEXR_PIXELTYPE_UINT : TINYEXR_PIXELTYPE_FLOAT;
        // pixel type of the output image to be stored in .EXR file
        exr.header.requested_pixel_types[i] = pixelTypes[i];
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

    exr.addAttributesToHeader();
    std::size_t fileDataSize = SaveEXRImageToMemory(&exr.image, &exr.header, &exr.fileData, &exr.err);
    if (fileDataSize == 0)
        fatal(rc::IO_FAILURE, "EXR Encoder error: {}.", exr.err);

    OutputStream file(filepath, *this);
    file.write(reinterpret_cast<const char*>(exr.fileData), fileDataSize, *this);
}

void CommandExtract::saveImageFile(
        std::string filepath, bool appendExtension,
        const char* data, std::size_t size,
        VkFormat vkFormat, const FormatDescriptor& format, uint32_t width, uint32_t height) {

    switch (vkFormat) {
    case VK_FORMAT_A8_UNORM_KHR: [[fallthrough]];
    case VK_FORMAT_R8_UNORM: [[fallthrough]];
    case VK_FORMAT_R8_SRGB: [[fallthrough]];
    case VK_FORMAT_R8G8_UNORM: [[fallthrough]];
    case VK_FORMAT_R8G8_SRGB: [[fallthrough]];
    case VK_FORMAT_R8G8B8_UNORM: [[fallthrough]];
    case VK_FORMAT_R8G8B8_SRGB: [[fallthrough]];
    case VK_FORMAT_B8G8R8_UNORM: [[fallthrough]];
    case VK_FORMAT_B8G8R8_SRGB: [[fallthrough]];
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32: [[fallthrough]];
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32: [[fallthrough]];
    case VK_FORMAT_R8G8B8A8_UNORM: [[fallthrough]];
    case VK_FORMAT_R8G8B8A8_SRGB: [[fallthrough]];
    case VK_FORMAT_B8G8R8A8_UNORM: [[fallthrough]];
    case VK_FORMAT_B8G8R8A8_SRGB:
        savePNG(std::move(filepath), appendExtension, vkFormat, format, width, height, data, size);
        break;

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
        // ASTC decode will recurse into this function with the uncompressed data and format
        decodeAndSaveASTC(std::move(filepath), appendExtension, vkFormat, format, width, height, data, size);
        break;

    case VK_FORMAT_R4G4_UNORM_PACK8: [[fallthrough]];
    case VK_FORMAT_R5G6B5_UNORM_PACK16: [[fallthrough]];
    case VK_FORMAT_B5G6R5_UNORM_PACK16: [[fallthrough]];
    case VK_FORMAT_R4G4B4A4_UNORM_PACK16: [[fallthrough]];
    case VK_FORMAT_B4G4R4A4_UNORM_PACK16: [[fallthrough]];
    case VK_FORMAT_R5G5B5A1_UNORM_PACK16: [[fallthrough]];
    case VK_FORMAT_B5G5R5A1_UNORM_PACK16: [[fallthrough]];
    case VK_FORMAT_A1R5G5B5_UNORM_PACK16: [[fallthrough]];
    case VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR: [[fallthrough]];
    case VK_FORMAT_A4R4G4B4_UNORM_PACK16: [[fallthrough]];
    case VK_FORMAT_A4B4G4R4_UNORM_PACK16: [[fallthrough]];
    case VK_FORMAT_R10X6_UNORM_PACK16: [[fallthrough]];
    case VK_FORMAT_R10X6G10X6_UNORM_2PACK16: [[fallthrough]];
    case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16: [[fallthrough]];
    case VK_FORMAT_R12X4_UNORM_PACK16: [[fallthrough]];
    case VK_FORMAT_R12X4G12X4_UNORM_2PACK16: [[fallthrough]];
    case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16: [[fallthrough]];
    case VK_FORMAT_R16_UNORM: [[fallthrough]];
    case VK_FORMAT_R16G16_UNORM: [[fallthrough]];
    case VK_FORMAT_R16G16B16_UNORM: [[fallthrough]];
    case VK_FORMAT_R16G16B16A16_UNORM: [[fallthrough]];
    case VK_FORMAT_A2R10G10B10_UNORM_PACK32: [[fallthrough]];
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        savePNG(std::move(filepath), appendExtension, vkFormat, format, width, height, data, size);
        break;

    case VK_FORMAT_G8B8G8R8_422_UNORM: [[fallthrough]];
    case VK_FORMAT_B8G8R8G8_422_UNORM: [[fallthrough]];
    case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16: [[fallthrough]];
    case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16: [[fallthrough]];
    case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16: [[fallthrough]];
    case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16: [[fallthrough]];
    case VK_FORMAT_G16B16G16R16_422_UNORM: [[fallthrough]];
    case VK_FORMAT_B16G16R16G16_422_UNORM:
        // Unpack and save 4:2:2 formats as UNORM8, UNORM10X6, UNORM12X4 or UNORM16 formats
        unpackAndSave422(std::move(filepath), appendExtension, vkFormat, format, width, height, data, size);
        break;

    // EXR

    case VK_FORMAT_R8_UINT: [[fallthrough]];
    case VK_FORMAT_R8_SINT:
        saveEXR(std::move(filepath), appendExtension, vkFormat, format, width, height, TINYEXR_PIXELTYPE_HALF, data, size);
        break;
    case VK_FORMAT_R16_UINT: [[fallthrough]];
    case VK_FORMAT_R16_SINT:
        saveEXR(std::move(filepath), appendExtension, vkFormat, format, width, height, TINYEXR_PIXELTYPE_FLOAT, data, size);
        break;
    case VK_FORMAT_R32_UINT:
        saveEXR(std::move(filepath), appendExtension, vkFormat, format, width, height, TINYEXR_PIXELTYPE_UINT, data, size);
        break;
    case VK_FORMAT_R8G8_UINT: [[fallthrough]];
    case VK_FORMAT_R8G8_SINT:
        saveEXR(std::move(filepath), appendExtension, vkFormat, format, width, height, TINYEXR_PIXELTYPE_HALF, data, size);
        break;
    case VK_FORMAT_R16G16_UINT: [[fallthrough]];
    case VK_FORMAT_R16G16_SINT:
        saveEXR(std::move(filepath), appendExtension, vkFormat, format, width, height, TINYEXR_PIXELTYPE_FLOAT, data, size);
        break;
    case VK_FORMAT_R32G32_UINT:
        saveEXR(std::move(filepath), appendExtension, vkFormat, format, width, height, TINYEXR_PIXELTYPE_UINT, data, size);
        break;
    case VK_FORMAT_R8G8B8_UINT: [[fallthrough]];
    case VK_FORMAT_R8G8B8_SINT: [[fallthrough]];
    case VK_FORMAT_B8G8R8_UINT: [[fallthrough]];
    case VK_FORMAT_B8G8R8_SINT:
        saveEXR(std::move(filepath), appendExtension, vkFormat, format, width, height, TINYEXR_PIXELTYPE_HALF, data, size);
        break;
    case VK_FORMAT_R16G16B16_UINT: [[fallthrough]];
    case VK_FORMAT_R16G16B16_SINT:
        saveEXR(std::move(filepath), appendExtension, vkFormat, format, width, height, TINYEXR_PIXELTYPE_FLOAT, data, size);
        break;
    case VK_FORMAT_R32G32B32_UINT:
        saveEXR(std::move(filepath), appendExtension, vkFormat, format, width, height, TINYEXR_PIXELTYPE_UINT, data, size);
        break;
    case VK_FORMAT_R8G8B8A8_UINT: [[fallthrough]];
    case VK_FORMAT_R8G8B8A8_SINT: [[fallthrough]];
    case VK_FORMAT_B8G8R8A8_UINT: [[fallthrough]];
    case VK_FORMAT_B8G8R8A8_SINT:
        saveEXR(std::move(filepath), appendExtension, vkFormat, format, width, height, TINYEXR_PIXELTYPE_HALF, data, size);
        break;
    case VK_FORMAT_R16G16B16A16_UINT: [[fallthrough]];
    case VK_FORMAT_R16G16B16A16_SINT:
        saveEXR(std::move(filepath), appendExtension, vkFormat, format, width, height, TINYEXR_PIXELTYPE_FLOAT, data, size);
        break;
    case VK_FORMAT_R32G32B32A32_UINT:
        saveEXR(std::move(filepath), appendExtension, vkFormat, format, width, height, TINYEXR_PIXELTYPE_UINT, data, size);
        break;

    case VK_FORMAT_A2R10G10B10_UINT_PACK32: [[fallthrough]];
    case VK_FORMAT_A2R10G10B10_SINT_PACK32: [[fallthrough]];
    case VK_FORMAT_A2B10G10R10_UINT_PACK32: [[fallthrough]];
    case VK_FORMAT_A2B10G10R10_SINT_PACK32:
        saveEXR(std::move(filepath), appendExtension, vkFormat, format, width, height, TINYEXR_PIXELTYPE_HALF, data, size);
        break;

    case VK_FORMAT_R16_SFLOAT: [[fallthrough]];
    case VK_FORMAT_R16G16_SFLOAT: [[fallthrough]];
    case VK_FORMAT_R16G16B16_SFLOAT: [[fallthrough]];
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        saveEXR(std::move(filepath), appendExtension, vkFormat, format, width, height, TINYEXR_PIXELTYPE_HALF, data, size);
        break;
    case VK_FORMAT_R32_SFLOAT: [[fallthrough]];
    case VK_FORMAT_R32G32_SFLOAT: [[fallthrough]];
    case VK_FORMAT_R32G32B32_SFLOAT: [[fallthrough]];
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        saveEXR(std::move(filepath), appendExtension, vkFormat, format, width, height, TINYEXR_PIXELTYPE_FLOAT, data, size);
        break;

    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
        saveEXR(std::move(filepath), appendExtension, vkFormat, format, width, height, TINYEXR_PIXELTYPE_HALF, data, size);
        break;

    case VK_FORMAT_D16_UNORM:
        savePNG(std::move(filepath), appendExtension, vkFormat, format, width, height, data, size);
        break;

    case VK_FORMAT_X8_D24_UNORM_PACK32: [[fallthrough]];
    case VK_FORMAT_D32_SFLOAT:
        saveEXR(std::move(filepath), appendExtension, vkFormat, format, width, height, TINYEXR_PIXELTYPE_FLOAT, data, size);
        break;
    case VK_FORMAT_S8_UINT:
        saveEXR(std::move(filepath), appendExtension, vkFormat, format, width, height, TINYEXR_PIXELTYPE_HALF, data, size);
        break;
    case VK_FORMAT_D16_UNORM_S8_UINT: [[fallthrough]];
    case VK_FORMAT_D24_UNORM_S8_UINT: [[fallthrough]];
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        saveEXR(std::move(filepath), appendExtension, vkFormat, format, width, height, {TINYEXR_PIXELTYPE_FLOAT, TINYEXR_PIXELTYPE_HALF}, data, size);
        break;

    default:
        fatal(rc::INVALID_FILE, "Requested format conversion from {} is not supported.", toString(vkFormat));
    }
}

} // namespace ktx

KTX_COMMAND_ENTRY_POINT(ktxExtract, ktx::CommandExtract)

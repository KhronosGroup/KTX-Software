// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include "command.h"
#include "compress_utils.h"
#include "transcode_utils.h"
#include "formats.h"
#include "sbufstream.h"
#include "utility.h"
#include "validate.h"
#include "ktx.h"
#include "image.hpp"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>

#include <cxxopts.hpp>
#include <fmt/ostream.h>
#include <fmt/printf.h>


// -------------------------------------------------------------------------------------------------

namespace ktx {

// -------------------------------------------------------------------------------------------------

/** @page ktxtools_transcode ktx transcode
@~English

Transcode a KTX2 file.

@section ktxtools_transcode_synopsis SYNOPSIS
    ktx transcode [option...] @e input-file @e output-file

@section ktxtools_transcode_description DESCRIPTION
    @b ktx @b transcode can transcode the KTX file specified as the @e input-file argument,
    optionally supercompress the result, and save it as the @e output-file.
    The input file must be transcodable (it must be either BasisLZ supercompressed or has UASTC
    color model in the DFD).
    If the input file is invalid the first encountered validation error is displayed
    to the stderr and the command exits with the relevant non-zero status code.

    The following options are available:
    <dl>
        <dt>--target &lt;target&gt;</dt>
        <dd>Target transcode format.
            If the target option is not set the r8, rg8, rgb8 or rgba8 target will be
            selected based on the number of channels in the input texture.
            Block compressed transcode targets can only be saved in raw format.
            Case-insensitive. Possible options are:
            etc-rgb | etc-rgba | eac-r11 | eac-rg11 | bc1 | bc3 | bc4 | bc5 | bc7 | astc |
            r8 | rg8 | rgb8 | rgba8.
            etc-rgb is ETC1; etc-rgba, eac-r11 and eac-rg11 are ETC2.
        </dd>
    </dl>
    @snippet{doc} ktx/compress_utils.h command options_compress
    @snippet{doc} ktx/command.h command options_generic

@section ktxtools_transcode_exitstatus EXIT STATUS
    @snippet{doc} ktx/command.h command exitstatus

@section ktxtools_transcode_history HISTORY

@par Version 4.0
 - Initial version

@section ktxtools_transcode_author AUTHOR
    - Mátyás Császár [Vader], RasterGrid www.rastergrid.com
    - Daniel Rákos, RasterGrid www.rastergrid.com
*/
class CommandTranscode : public Command {
    enum {
        all = -1,
    };

    struct OptionsTranscode {
        void init(cxxopts::Options& opts);
        void process(cxxopts::Options& opts, cxxopts::ParseResult& args, Reporter& report);
    };

    Combine<OptionsTranscode, OptionsTranscodeTarget<true>, OptionsCompress, OptionsSingleInSingleOut, OptionsGeneric> options;

public:
    virtual int main(int argc, _TCHAR* argv[]) override;
    virtual void initOptions(cxxopts::Options& opts) override;
    virtual void processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) override;

private:
    void executeTranscode();
};

// -------------------------------------------------------------------------------------------------

int CommandTranscode::main(int argc, _TCHAR* argv[]) {
    try {
        parseCommandLine("ktx transcode",
                "Transcode the KTX file specified as the input-file argument,\n"
                "    optionally supercompress the result, and save it as the output-file.",
                argc, argv);
        executeTranscode();
        return +rc::SUCCESS;
    } catch (const FatalError& error) {
        return +error.returnCode;
    } catch (const std::exception& e) {
        fmt::print(std::cerr, "{} fatal: {}\n", commandName, e.what());
        return +rc::RUNTIME_ERROR;
    }
}

void CommandTranscode::OptionsTranscode::init(cxxopts::Options& opts) {
    opts.add_options()
        ("target", "Target transcode format."
                   " Block compressed transcode targets can only be saved in raw format."
                   " Case-insensitive."
                   "\nPossible options are:"
                   " etc-rgb | etc-rgba | eac-r11 | eac-rg11 | bc1 | bc3 | bc4 | bc5 | bc7 | astc |"
                   " r8 | rg8 | rgb8 | rgba8."
                   "\netc-rgb is ETC1; etc-rgba, eac-r11 and eac-rg11 are ETC2.",
                   cxxopts::value<std::string>(), "<target>");
}

void CommandTranscode::OptionsTranscode::process(cxxopts::Options&, cxxopts::ParseResult&, Reporter&) {
}

void CommandTranscode::initOptions(cxxopts::Options& opts) {
    options.init(opts);
}

void CommandTranscode::processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) {
    options.process(opts, args, *this);
}

void CommandTranscode::executeTranscode() {
    std::ifstream file(options.inputFilepath, std::ios::in | std::ios::binary);
    validateToolInput(file, options.inputFilepath, *this);

    KTXTexture2 texture{nullptr};
    StreambufStream<std::streambuf*> ktx2Stream{file.rdbuf(), std::ios::in | std::ios::binary};
    auto ret = ktxTexture2_CreateFromStream(ktx2Stream.stream(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, texture.pHandle());
    if (ret != KTX_SUCCESS)
        fatal(rc::INVALID_FILE, "Failed to create KTX2 texture: {}", ktxErrorString(ret));

    if (!ktxTexture2_NeedsTranscoding(texture))
        fatal(rc::INVALID_FILE, "KTX file is not transcodable.");

    options.validateTextureTranscode(texture, *this);

    ret = ktxTexture2_TranscodeBasis(texture, options.transcodeTarget.value(), 0);
    if (ret != KTX_SUCCESS)
        fatal(rc::INVALID_FILE, "Failed to transcode KTX2 texture: {}", ktxErrorString(ret));

    // Need to perform format conversion and swizzling if needed
    bool needFormatConversion = false;
    bool needSwizzle = false;
    if (options.transcodeSwizzleComponents != 0) {
        if (options.transcodeSwizzleComponents == 4) {
            if (options.transcodeSwizzle != "rgba") {
                needSwizzle = true;
            }
        } else {
            needFormatConversion = true;
            needSwizzle = true;
        }
    }

    KTXTexture2 convertedTexture{nullptr};
    if (needFormatConversion) {
        ktxTextureCreateInfo createInfo;
        std::memset(&createInfo, 0, sizeof(createInfo));

        const bool srgb = (texture->vkFormat == VK_FORMAT_R8G8B8A8_SRGB);
        switch (options.transcodeSwizzleComponents) {
        case 1:
            createInfo.vkFormat = srgb ? VK_FORMAT_R8_SRGB : VK_FORMAT_R8_UNORM;
            break;
        case 2:
            createInfo.vkFormat = srgb ? VK_FORMAT_R8G8_SRGB : VK_FORMAT_R8G8_UNORM;
            break;
        case 3:
            createInfo.vkFormat = srgb ? VK_FORMAT_R8G8B8_SRGB : VK_FORMAT_R8G8B8_UNORM;
            break;
        default:
            assert(false);
        }

        createInfo.baseWidth = texture->baseWidth;
        createInfo.baseHeight = texture->baseHeight;
        createInfo.baseDepth = texture->baseDepth;
        createInfo.generateMipmaps = texture->generateMipmaps;
        createInfo.isArray = texture->isArray;
        createInfo.numDimensions = texture->numDimensions;
        createInfo.numFaces = texture->numFaces;
        createInfo.numLayers = texture->numLayers;
        createInfo.numLevels = texture->numLevels;
        createInfo.pDfd = nullptr;

        ret = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, convertedTexture.pHandle());
        if (KTX_SUCCESS != ret)
            fatal(rc::IO_FAILURE, "Failed to create output texture: {}", ktxErrorString(ret));
    }

    KTXTexture2& outputTexture = (convertedTexture.handle() != nullptr) ? convertedTexture : texture;
    if (needFormatConversion || needSwizzle) {
        for (uint32_t levelIndex = 0; levelIndex < texture->numLevels; ++levelIndex) {
            const auto imageWidth = std::max(1u, texture->baseWidth >> levelIndex);
            const auto imageHeight = std::max(1u, texture->baseHeight >> levelIndex);
            const auto imageDepth = std::max(1u, texture->baseDepth >> levelIndex);

            for (uint32_t faceIndex = 0; faceIndex < texture->numFaces; ++faceIndex) {
                for (uint32_t layerIndex = 0; layerIndex < texture->numLayers; ++layerIndex) {
                    for (uint32_t depthIndex = 0; depthIndex < imageDepth; ++depthIndex) {
                        ktx_size_t srcImageOffset;
                        ktxTexture_GetImageOffset(texture, levelIndex, layerIndex, faceIndex + depthIndex, &srcImageOffset);
                        ktx_size_t dstImageOffset;
                        ktxTexture_GetImageOffset(outputTexture, levelIndex, layerIndex, faceIndex + depthIndex, &dstImageOffset);

                        auto srcImageData = texture->pData + srcImageOffset;
                        auto dstImageData = outputTexture->pData + dstImageOffset;

                        rgba8image srcImage(imageWidth, imageHeight, reinterpret_cast<rgba8color*>(srcImageData));

                        switch (options.transcodeSwizzleComponents) {
                        case 1: {
                            r8image dstImage(imageWidth, imageHeight, reinterpret_cast<r8color*>(dstImageData));
                            srcImage.copyToR(dstImage, options.transcodeSwizzle);
                            break;
                        }
                        case 2: {
                            rg8image dstImage(imageWidth, imageHeight, reinterpret_cast<rg8color*>(dstImageData));
                            srcImage.copyToRG(dstImage, options.transcodeSwizzle);
                            break;
                        }
                        case 3: {
                            rgb8image dstImage(imageWidth, imageHeight, reinterpret_cast<rgb8color*>(dstImageData));
                            srcImage.copyToRGB(dstImage, options.transcodeSwizzle);
                            break;
                        }
                        case 4: {
                            // Swizzle in-place
                            assert(srcImageData == dstImageData);
                            srcImage.swizzle(options.transcodeSwizzle);
                            break;
                        }
                        default:
                            assert(false);
                        }
                    }
                }
            }
        }
    }

    if (options.zstd) {
        ret = ktxTexture2_DeflateZstd(outputTexture, *options.zstd);
        if (ret != KTX_SUCCESS)
            fatal(rc::KTX_FAILURE, "Zstd deflation failed. KTX Error: {}", ktxErrorString(ret));
    }

    if (options.zlib) {
        ret = ktxTexture2_DeflateZLIB(outputTexture, *options.zlib);
        if (ret != KTX_SUCCESS)
            fatal(rc::KTX_FAILURE, "ZLIB deflation failed. KTX Error: {}", ktxErrorString(ret));
    }

    // Modify KTXwriter metadata
    const auto writer = fmt::format("{} {}", commandName, version(options.testrun));
    ktxHashList_DeleteKVPair(&outputTexture->kvDataHead, KTX_WRITER_KEY);
    ktxHashList_AddKVPair(&outputTexture->kvDataHead, KTX_WRITER_KEY,
            static_cast<uint32_t>(writer.size() + 1), // +1 to include the \0
            writer.c_str());

    // Save output file
    if (std::filesystem::path(options.outputFilepath).has_parent_path())
        std::filesystem::create_directories(std::filesystem::path(options.outputFilepath).parent_path());
    FILE* f = _tfopen(options.outputFilepath.c_str(), "wb");
    if (!f)
        fatal(rc::IO_FAILURE, "Could not open output file \"{}\": ", options.outputFilepath, errnoMessage());

    ret = ktxTexture_WriteToStdioStream(outputTexture, f);
    fclose(f);

    if (KTX_SUCCESS != ret) {
        if (f != stdout)
            std::filesystem::remove(options.outputFilepath);
        fatal(rc::IO_FAILURE, "Failed to write KTX file \"{}\": KTX error: {}",
            options.outputFilepath, ktxErrorString(ret));
    }
}

} // namespace ktx

KTX_COMMAND_ENTRY_POINT(ktxTranscode, ktx::CommandTranscode)

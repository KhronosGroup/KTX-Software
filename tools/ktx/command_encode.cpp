// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include "command.h"
#include "encode_utils_astc.h"
#include "encode_utils_common.h"
#include "platform_utils.h"
#include "metrics_utils.h"
#include "deflate_utils.h"
#include "encode_utils_basis.h"
#include "formats.h"
#include "sbufstream.h"
#include "utility.h"
#include "validate.h"
#include "ktx.h"
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

/** @page ktx_encode ktx encode
@~English

Encode a KTX2 file.

@section ktx_encode_synopsis SYNOPSIS
    ktx encode [option...] @e input-file @e output-file

@section ktx_encode_description DESCRIPTION
    @b ktx @b encode can encode the KTX file specified as the @e input-file argument
    to a universal format or one of the ASTC formats, optionally supercompress the result,
    and save it as the @e output-file.
    If the @e input-file is '-' the file will be read from the stdin.
    If the @e output-path is '-' the output file will be written to the stdout.

    For universal and ASTC LDR formats, the input file must be R8, R8G8, R8G8B8
    or R8G8B8A8 (or their sRGB variants).

    <!--For ASTC HDR formats the input file must be TBD (e.g. R16_{,S}FLOAT,
    R16G16_{,S}FLOAT ...
-->
    If the input file is invalid the first encountered validation error is displayed
    to the stderr and the command exits with the relevant non-zero status code.

@section ktx\_encode\_options OPTIONS
  @subsection ktx\_encode\_options\_general General Options
    <!--Specifying both @e \--codec and @e \--format options is an error.
-->
    The following options are available:
    <dl>
        <dt>\--codec basis-lz | uastc</dt>
        <dd>Target codec followed by the codec specific options. With each choice
            the specific and common encoder options listed
            @ref ktx\_encode\_options\_encoding "below" become valid, otherwise
            they are ignored. Case-insensitive.</dd>

            @snippet{doc} ktx/encode_utils_basis.h command options_basis_encoders
        <dt>\--format</dt>
        <dd>KTX format enum that specifies the target ASTC format. Non-ASTC
            formats are invalid. When specified the ASTC-specific and common
            encoder options listed @ref ktx\_encode\_options\_encoding "below"
            become valid, otherwise they are ignored.
    </dl>
    @snippet{doc} ktx/deflate_utils.h command options_deflate
    @snippet{doc} ktx/command.h command options_generic

  @subsection ktx\_encode\_options\_encoding Specific and Common Encoding Options
    The following specific and common encoder options are available. Specific options
    become valid only if their encoder has been selected. Common encoder options
    become valid when an encoder they apply to has been selected. Otherwise they are ignored.
    @snippet{doc} ktx/encode_utils_astc.h command options_encode_astc
    @snippet{doc} ktx/encode_utils_basis.h command options_encode_basis
    @snippet{doc} ktx/encode_utils_common.h command options_encode_common
    @snippet{doc} ktx/metrics_utils.h command options_metrics

@section ktx_encode_exitstatus EXIT STATUS
    @snippet{doc} ktx/command.h command exitstatus

@section ktx_encode_history HISTORY

@par Version 4.0
 - Initial version.

 @par Version 4.4
  - Reorganize encoding options.

@section ktx_encode_author AUTHOR
    - Mátyás Császár [Vader], RasterGrid www.rastergrid.com
    - Daniel Rákos, RasterGrid www.rastergrid.com
*/
class CommandEncode : public Command {
    struct OptionsEncode {
        inline static const char* kFormat = "format";
        inline static const char* kCodec = "codec";

        VkFormat vkFormat = VK_FORMAT_UNDEFINED;

        void init(cxxopts::Options& opts);
        void process(cxxopts::Options& opts, cxxopts::ParseResult& args, Reporter& report);
    };

    Combine<OptionsEncode, OptionsEncodeASTC, OptionsEncodeBasis<true>, OptionsEncodeCommon, OptionsMetrics, OptionsDeflate, OptionsSingleInSingleOut, OptionsGeneric> options;

public:
    virtual int main(int argc, char* argv[]) override;
    virtual void initOptions(cxxopts::Options& opts) override;
    virtual void processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) override;

private:
    void executeEncode();
};

// -------------------------------------------------------------------------------------------------

int CommandEncode::main(int argc, char* argv[]) {
    try {
        parseCommandLine("ktx encode",
                "Encode the KTX file specified as the input-file argument,\n"
                "    optionally supercompress the result, and save it as the output-file.",
                argc, argv);
        executeEncode();
        return +rc::SUCCESS;
    } catch (const FatalError& error) {
        return +error.returnCode;
    } catch (const std::exception& e) {
        fmt::print(std::cerr, "{} fatal: {}\n", commandName, e.what());
        return +rc::RUNTIME_ERROR;
    }
}

void CommandEncode::OptionsEncode::init(cxxopts::Options& opts) {
    opts.add_options()
        (kFormat, "KTX format enum that specifies the KTX file output format."
                  " The enum names are matching the VkFormats without the VK_FORMAT_ prefix."
                  " The VK_FORMAT_ prefix is ignored if present."
                  "\nIt can't be used with --codec."
                  "\nThe value must be an ASTC format. When specified the ASTC encoder specific"
                  " options becomes valid."
                  " Case insensitive.", cxxopts::value<std::string>(), "<enum>")
        (kCodec, "Target codec."
                  " With each encoding option the encoder specific options become valid,"
                  " otherwise they are ignored. Case-insensitive."
                  "\nPossible options are: basis-lz | uastc", cxxopts::value<std::string>(), "<target>");
}

void CommandEncode::OptionsEncode::process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter& report) {
    if (args[kCodec].count() && args[kFormat].count())
        report.fatal_usage("Format and codec can't be both specified together.");

    if (args[kFormat].count()) {
        const auto formatStr = args[kFormat].as<std::string>();
        const auto parsedVkFormat = parseVkFormat(formatStr);
        if (!parsedVkFormat)
            report.fatal_usage("The requested format is invalid or unsupported: \"{}\".", formatStr);

        vkFormat = *parsedVkFormat;

        if (!isFormatAstc(vkFormat)) {
            report.fatal_usage("Optional option 'format' is not an ASTC format.");
        }
    }
}

void CommandEncode::initOptions(cxxopts::Options& opts) {
    options.init(opts);
}

void CommandEncode::processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) {
    options.process(opts, args, *this);

    fillOptionsCodecBasis<decltype(options)>(options);

    if ((options.codec == BasisCodec::NONE || options.codec == BasisCodec::INVALID) &&
        options.vkFormat == VK_FORMAT_UNDEFINED)
        fatal_usage("Either codec or format must be specified");

    if (options.codec == BasisCodec::BasisLZ) {
        if (options.zstd.has_value())
            fatal_usage("Cannot encode to BasisLZ and supercompress with Zstd.");

        if (options.zlib.has_value())
            fatal_usage("Cannot encode to BasisLZ and supercompress with ZLIB.");
    }

    const auto basisCodec = options.codec == BasisCodec::BasisLZ || options.codec == BasisCodec::UASTC;
    const auto astcCodec = isFormatAstc(options.vkFormat);
    const auto canCompare = basisCodec || astcCodec;

    if (options.compare_ssim && !canCompare)
        fatal_usage("--compare-ssim can only be used with BasisLZ, UASTC or ASTC encoding.");
    if (options.compare_psnr && !canCompare)
        fatal_usage("--compare-psnr can only be used with BasisLZ, UASTC or ASTC encoding.");

    if (astcCodec)
        options.encodeASTC = true;
}

void CommandEncode::executeEncode() {
    InputStream inputStream(options.inputFilepath, *this);
    validateToolInput(inputStream, fmtInFile(options.inputFilepath), *this);

    KTXTexture2 texture{nullptr};
    StreambufStream<std::streambuf*> ktx2Stream{inputStream->rdbuf(), std::ios::in | std::ios::binary};
    auto ret = ktxTexture2_CreateFromStream(ktx2Stream.stream(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, texture.pHandle());
    if (ret != KTX_SUCCESS)
        fatal(rc::INVALID_FILE, "Failed to create KTX2 texture: {}", ktxErrorString(ret));

    if (texture->supercompressionScheme != KTX_SS_NONE)
        fatal(rc::INVALID_FILE, "Cannot encode KTX2 file with {} supercompression.",
            toString(ktxSupercmpScheme(texture->supercompressionScheme)));

    const auto* bdfd = texture->pDfd + 1;
    if (khr_df_model_e(KHR_DFDVAL(bdfd, MODEL)) == KHR_DF_MODEL_ASTC && options.encodeASTC)
        fatal_usage("Encoding from ASTC format {} to another ASTC format {} is not supported.", toString(VkFormat(texture->vkFormat)), toString(options.vkFormat));

    switch (texture->vkFormat) {
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
            "but format is {}.", toString(VkFormat(texture->vkFormat)));
        break;
    }

    // Convert 1D textures to 2D (we could consider 1D as an invalid input)
    texture->numDimensions = std::max(2u, texture->numDimensions);

    // Modify KTXwriter metadata
    const auto writer = fmt::format("{} {}", commandName, version(options.testrun));
    ktxHashList_DeleteKVPair(&texture->kvDataHead, KTX_WRITER_KEY);
    ktxHashList_AddKVPair(&texture->kvDataHead, KTX_WRITER_KEY,
            static_cast<uint32_t>(writer.size() + 1), // +1 to include the \0
            writer.c_str());

    khr_df_transfer_e tf = ktxTexture2_GetTransferFunction_e(texture);
    if (options.ktxBasisParams::normalMap && tf != KHR_DF_TRANSFER_LINEAR)
        fatal(rc::INVALID_FILE,
            "--normal-mode specified but the input file uses non-linear transfer function {}.",
            toString(tf));

    MetricsCalculator metrics;
    metrics.saveReferenceImages(texture, options, *this);

    if (options.vkFormat != VK_FORMAT_UNDEFINED) {
       options.mode = KTX_PACK_ASTC_ENCODER_MODE_LDR; // TODO: Fix me for HDR textures
       ret = ktxTexture2_CompressAstcEx(texture, &options);
       if (ret != KTX_SUCCESS)
           fatal(rc::IO_FAILURE, "Failed to encode KTX2 file to ASTC. KTX Error: {}", ktxErrorString(ret));
    } else {
       ret = ktxTexture2_CompressBasisEx(texture, &options);
       if (ret != KTX_SUCCESS)
           fatal(rc::IO_FAILURE, "Failed to encode KTX2 file with codec \"{}\". KTX Error: {}", options.codecName, ktxErrorString(ret));
    }

    metrics.decodeAndCalculateMetrics(texture, options, *this);

    if (options.zstd) {
        ret = ktxTexture2_DeflateZstd(texture, *options.zstd);
        if (ret != KTX_SUCCESS)
            fatal(rc::IO_FAILURE, "Zstd deflation failed. KTX Error: {}", ktxErrorString(ret));
    }

    if (options.zlib) {
        ret = ktxTexture2_DeflateZLIB(texture, *options.zlib);
        if (ret != KTX_SUCCESS)
            fatal(rc::IO_FAILURE, "ZLIB deflation failed. KTX Error: {}", ktxErrorString(ret));
    }

    // Add KTXwriterScParams metadata
    const auto writerScParams = fmt::format("{}{}{}", options.codecOptions, options.commonOptions, options.compressOptions);
    ktxHashList_DeleteKVPair(&texture->kvDataHead, KTX_WRITER_SCPARAMS_KEY);
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

} // namespace ktx

KTX_COMMAND_ENTRY_POINT(ktxEncode, ktx::CommandEncode)

// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include "command.h"
#include "platform_utils.h"
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

/** @page ktx_transcode ktx transcode
@~English

Transcode a KTX2 file.

@section ktx_transcode_synopsis SYNOPSIS
    ktx transcode [option...] @e input-file @e output-file

@section ktx_transcode_description DESCRIPTION
    @b ktx @b transcode can transcode the KTX file specified as the @e input-file argument,
    optionally supercompress the result, and save it as the @e output-file.
    If the @e input-file is '-' the file will be read from the stdin.
    If the @e output-path is '-' the output file will be written to the stdout.
    The input file must be transcodable (it must be either BasisLZ supercompressed or has UASTC
    color model in the DFD).
    If the input file is invalid the first encountered validation error is displayed
    to the stderr and the command exits with the relevant non-zero status code.

    The following options are available:
    <dl>
        <dt>\--target &lt;target&gt;</dt>
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

@section ktx_transcode_exitstatus EXIT STATUS
    @snippet{doc} ktx/command.h command exitstatus

@section ktx_transcode_history HISTORY

@par Version 4.0
 - Initial version

@section ktx_transcode_author AUTHOR
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
    virtual int main(int argc, char* argv[]) override;
    virtual void initOptions(cxxopts::Options& opts) override;
    virtual void processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) override;

private:
    void executeTranscode();
};

// -------------------------------------------------------------------------------------------------

int CommandTranscode::main(int argc, char* argv[]) {
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
    InputStream inputStream(options.inputFilepath, *this);
    validateToolInput(inputStream, fmtInFile(options.inputFilepath), *this);

    KTXTexture2 texture{nullptr};
    StreambufStream<std::streambuf*> ktx2Stream{inputStream->rdbuf(), std::ios::in | std::ios::binary};
    auto ret = ktxTexture2_CreateFromStream(ktx2Stream.stream(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, texture.pHandle());
    if (ret != KTX_SUCCESS)
        fatal(rc::INVALID_FILE, "Failed to create KTX2 texture: {}", ktxErrorString(ret));

    if (!ktxTexture2_NeedsTranscoding(texture))
        fatal(rc::INVALID_FILE, "KTX file is not transcodable.");

    texture = transcode(std::move(texture), options, *this);

    if (options.zstd) {
        ret = ktxTexture2_DeflateZstd(texture, *options.zstd);
        if (ret != KTX_SUCCESS)
            fatal(rc::KTX_FAILURE, "Zstd deflation failed. KTX Error: {}", ktxErrorString(ret));
    }

    if (options.zlib) {
        ret = ktxTexture2_DeflateZLIB(texture, *options.zlib);
        if (ret != KTX_SUCCESS)
            fatal(rc::KTX_FAILURE, "ZLIB deflation failed. KTX Error: {}", ktxErrorString(ret));
    }

    // Modify KTXwriter metadata
    const auto writer = fmt::format("{} {}", commandName, version(options.testrun));
    ktxHashList_DeleteKVPair(&texture->kvDataHead, KTX_WRITER_KEY);
    ktxHashList_AddKVPair(&texture->kvDataHead, KTX_WRITER_KEY,
            static_cast<uint32_t>(writer.size() + 1), // +1 to include the \0
            writer.c_str());

    // Add KTXwriterScParams metadata if supercompression was used
    const auto writerScParams = options.compressOptions;
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

KTX_COMMAND_ENTRY_POINT(ktxTranscode, ktx::CommandTranscode)

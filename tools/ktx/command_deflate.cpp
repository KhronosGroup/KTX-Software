// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include "command.h"
#include "compress_utils.h"
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

/** @page ktx_deflate ktx deflate
@~English

Deflate (supercompress) a KTX2 file.

@section ktx_deflate_synopsis SYNOPSIS
    ktx deflate [option...] @e input-file @e output-file

@section ktx_deflate_description DESCRIPTION
    @b ktx @b deflate deflates (supercompresses) the KTX file specified as the
    @e input-file and saves it as the @e output-file.
    If the @e input-file is '-' the file will be read from the stdin.
    If the @e output-path is '-' the output file will be written to the stdout.
    If the input file is already supercompressed it will be inflated then
    supercompressed again using the options specified here and a warning will
    be issued. If the input file is invalid the first encountered validation
    error is displayed to the stderr and the command exits with the relevant
    non-zero status code.

    @ktx @b deflate cannot be applied to KTX files that have been
    supercompressed with BasisLZ.

    The following options are available:
    @snippet{doc} ktx/compress_utils.h command options_compress
    <dl>
        <dt>-q, --quiet</dt>
        <dd>Silence warning about already supercompressed input fiile..</dd>
        <dt>-e, --warnings-as-errors</dt>
        <dd>Treat warnings as errors.</dd>
    </dl>
    @snippet{doc} ktx/command.h command options_generic

@section ktx_deflate_exitstatus EXIT STATUS
    @snippet{doc} ktx/command.h command exitstatus

@section ktx_deflate_history HISTORY

@par Version 4.0
 - Initial version

@section ktx_deflate_author AUTHOR
    - Mark Callow [@MarkCallow]
*/
class CommandDeflate : public Command {
    enum {
        all = -1,
    };

    struct OptionsDeflate {
        bool quiet = false;
        bool warningsAsErrors = false;
        void init(cxxopts::Options& opts);
        void process(cxxopts::Options& opts, cxxopts::ParseResult& args, Reporter& report);
    };

    Combine<OptionsDeflate, OptionsCompress, OptionsSingleInSingleOut, OptionsGeneric> options;

public:
    virtual int main(int argc, _TCHAR* argv[]) override;
    virtual void initOptions(cxxopts::Options& opts) override;
    virtual void processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) override;

private:
    void executeDeflate();
};

// -------------------------------------------------------------------------------------------------

int CommandDeflate::main(int argc, _TCHAR* argv[]) {
    try {
        parseCommandLine("ktx deflate",
                "Deflate (supercompress) the KTX file specified as the input-file\n"
                "    and save it as the output-file.",
                argc, argv);
        executeDeflate();
        return +rc::SUCCESS;
    } catch (const FatalError& error) {
        return +error.returnCode;
    } catch (const std::exception& e) {
        fmt::print(std::cerr, "{} fatal: {}\n", commandName, e.what());
        return +rc::RUNTIME_ERROR;
    }
}

void CommandDeflate::OptionsDeflate::init(cxxopts::Options& opts) {
    opts.add_options()
        ("q,quiet", "Don't print warning when input file is already supercompressed.")
        ("w, warnings-as-errors", "Exit with error when input file is already supercompressed");

}

void CommandDeflate::OptionsDeflate::process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter&) {
    quiet = args["quiet"].as<bool>();
    warningsAsErrors = args["warnings-as-errors"].as<bool>();
}

void CommandDeflate::initOptions(cxxopts::Options& opts) {
    options.init(opts);
}

void CommandDeflate::processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) {
    options.process(opts, args, *this);
    if (options.quiet && options.warningsAsErrors) {
        fatal_usage("Cannot specify both --quiet and --warnings-as-errors");
    }
    if (!options.zstd && !options.zlib) {
        fatal_usage("Must specifiy one of --zstd or --zlib.");
    }
}

void CommandDeflate::executeDeflate() {
    InputStream inputStream(options.inputFilepath, *this);
    validateToolInput(inputStream, fmtInFile(options.inputFilepath), *this);

    KTXTexture2 texture{nullptr};
    StreambufStream<std::streambuf*> ktx2Stream{inputStream->rdbuf(), std::ios::in | std::ios::binary};
    auto ret = ktxTexture2_CreateFromStream(ktx2Stream.stream(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, texture.pHandle());
    if (ret != KTX_SUCCESS)
        fatal(rc::INVALID_FILE, "Failed to create KTX2 texture: {}", ktxErrorString(ret));

    if (texture->supercompressionScheme == KTX_SS_BASIS_LZ)
        fatal(rc::INVALID_FILE, "Cannot deflate a KTX2 file supercompressed with {}.",
            toString(ktxSupercmpScheme(texture->supercompressionScheme)));

    if (texture->supercompressionScheme != KTX_SS_NONE && !options.quiet) {
        warning("Modifying existing supercompression of {}.", options.inputFilepath);
    }

    // Modify KTXwriter metadata
    const auto writer = fmt::format("{} {}", commandName, version(options.testrun));
    ktxHashList_DeleteKVPair(&texture->kvDataHead, KTX_WRITER_KEY);
    ktxHashList_AddKVPair(&texture->kvDataHead, KTX_WRITER_KEY,
            static_cast<uint32_t>(writer.size() + 1), // +1 to include the \0
            writer.c_str());

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
    const auto writerScParams = fmt::format("{}{}", options.compressOptions);
    if (writerScParams.size() > 0) {
        // Options always contain a leading space
        assert(writerScParams[0] == ' ');
        ktxHashList_AddKVPair(&texture->kvDataHead, KTX_WRITER_SCPARAMS_KEY,
            static_cast<uint32_t>(writerScParams.size()),
            writerScParams.c_str() + 1); // +1 to exclude leading space
    }

    // Save output file
    if (std::filesystem::path(options.outputFilepath).has_parent_path())
        std::filesystem::create_directories(std::filesystem::path(options.outputFilepath).parent_path());

    OutputStream outputFile(options.outputFilepath, *this);
    outputFile.writeKTX2(texture, *this);
}

} // namespace ktx

KTX_COMMAND_ENTRY_POINT(ktxDeflate, ktx::CommandDeflate)

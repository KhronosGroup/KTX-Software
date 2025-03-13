// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include "command.h"
#include "platform_utils.h"
#include "deflate_utils.h"
#include "formats.h"
#include "sbufstream.h"
#include "utility.h"
#include "validate.h"
#include "ktx.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
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

    @b ktx @b deflate cannot be applied to KTX files that have been
    supercompressed with BasisLZ.

@section ktx\_deflate\_options OPTIONS
    The following options are available:
    @snippet{doc} ktx/deflate_utils.h command options_deflate
    <dl>
        <dt>-q, --quiet</dt>
        <dd>Silence warning about already supercompressed input fiile.</dd>
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
    - Mark Callow [\@MarkCallow]
*/
class CommandDeflate : public Command {
    enum {
        all = -1,
    };

    struct Options {
        inline static const char* kQuiet = "quiet";
        inline static const char* kWarningsAsErrors = "warnings-as-errors";
        bool quiet = false;
        bool warningsAsErrors = false;
        void init(cxxopts::Options& opts);
        void process(cxxopts::Options& opts, cxxopts::ParseResult& args, Reporter& report);
    };

    Combine<Options, OptionsDeflate, OptionsSingleInSingleOut, OptionsGeneric> options;

public:
    virtual int main(int argc, char* argv[]) override;
    virtual void initOptions(cxxopts::Options& opts) override;
    virtual void processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) override;

private:
    void executeDeflate();
};

// -------------------------------------------------------------------------------------------------

int CommandDeflate::main(int argc, char* argv[]) {
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

void CommandDeflate::Options::init(cxxopts::Options& opts) {
    opts.add_options()
        (kQuiet, "Don't print warning when input file is already supercompressed.")
        (kWarningsAsErrors, "Exit with error when input file is already supercompressed");
}

void CommandDeflate::Options::process(cxxopts::Options&,
                                             cxxopts::ParseResult& args,
                                             Reporter& report) {
    quiet = args[kQuiet].as<bool>();
    warningsAsErrors = args[kWarningsAsErrors].as<bool>();
    if (quiet && warningsAsErrors) {
        report.fatal_usage("Cannot specify both --{} and --{}.",
                           this->kQuiet, this->kWarningsAsErrors);
    }
}

void CommandDeflate::initOptions(cxxopts::Options& opts) {
    options.init(opts);
}

void CommandDeflate::processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) {
    options.process(opts, args, *this);
    if (!options.zstd && !options.zlib) {
        fatal_usage("Must specify --{}  or --{}.",
                    OptionsDeflate::kZStd, OptionsDeflate::kZLib);
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

    if (texture->supercompressionScheme != KTX_SS_NONE) {
        switch (texture->supercompressionScheme) {
          case KTX_SS_ZLIB:
          case KTX_SS_ZSTD:
            if (!options.quiet) {
                warning("Modifying existing {} supercompression of {}.",
                        toString(texture->supercompressionScheme),
                        options.inputFilepath);
            }
            break;
          default:
            fatal(rc::INVALID_FILE,
                  "Cannot further deflate a KTX2 file supercompressed with {}.",
                  toString(texture->supercompressionScheme));
        }
    }

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

    const auto& findMetadataValue = [&](const char* const key) {
        const char* value;
        uint32_t valueLen;
        std::string result;
        auto ret = ktxHashList_FindValue(&texture->kvDataHead, key,
                      &valueLen, (void**)&value);
        if (ret == KTX_SUCCESS) {
            // The values we are looking for are required to be NUL terminated.
            result.assign(value, valueLen - 1);
        }
        return result;
    };

    const auto updateMetadataValue = [&](const char* const key,
                                 const std::string& value) {
        ktxHashList_DeleteKVPair(&texture->kvDataHead, key);
        ktxHashList_AddKVPair(&texture->kvDataHead, key,
                static_cast<uint32_t>(value.size() + 1), // +1 to include \0
                value.c_str());
    };

    // ======= KTXwriter and KTXwriterScParams metadata handling =======
    //
    // In order to preserve encoding parameters applied to the data with
    // other apps prior to this deflate operation, `deflate` does the
    // following if KTXwriterScParams data exists in the input file:
    //
    // 1. If the writer was one of the ktx suite (i.e. create or encode)
    //    and KTXwriterScParams contains non-deflate options, use the
    //    original KTXwriter. Replace an existing deflate option with
    //    that currently specified or append it as new.
    //
    //    The original writer will obviously understand its own
    //    non-deflate options and, since it is part of the ktx suite
    //    it will understand the updated or new deflate option that will
    //    be added.
    //
    //    Cheeky! Spec. for KTXwriter says "only the most recent writer
    //    Should be identified." For KTXwriterScParams it says the writer
    //    should "append the (new) options" when "building on operations
    //    done previously." To somewhat resolve the conflict it changes
    //    the previous "only" to "in general."
    //
    // 2. If the writer was another tool, preserve its options in
    //    KTWwriterScParams labelled with its name and append the
    //    currently specified deflate option like so
    //
    //        --zstd 18 | (from <name>) option1 option2 ...
    //
    //    where <name> is the first word of the original KTXwriter metadata,
    //    e.g, "tokt". Rewrite KTXwriter with the name of this tool.
    //
    // 3. If the writer was ktxsc or toktx remove any original deflate
    //    option from the preserved parameters as we know those option
    //    names.

    bool changeWriter = true;
    std::string writerScParams;
    std::string origWriterName;
    writerScParams = findMetadataValue(KTX_WRITER_SCPARAMS_KEY);
    if (!writerScParams.empty()) {
        std::string writer = findMetadataValue(KTX_WRITER_KEY);
        if (!writer.empty()) {
            std::regex e("ktx (?:create|deflate|encode|transcode)");
            std::smatch deflateOptionMatch;
            if (std::regex_search(writer, e)) {
                // Writer is member of the ktx suite.
                // Look for existing deflate option
                e = " ?--(?:zlib|zstd) [1-9][0-9]?";
                (void)std::regex_search(writerScParams, deflateOptionMatch, e);
            } else {
                // Writer is not a member of the ktx suite
                e = "ktxsc|toktx";
                if (std::regex_search(writer, e)) {
                    // Look for toktx/ktxsc deflate option
                    e = " ?--zcmp ?[1-9]?[0-9]?";
                    (void)std::regex_search(writerScParams,
                                            deflateOptionMatch, e);
                }
                origWriterName = writer.substr(0, writer.find_first_of(' '));
            }
            // Remove existing deflate option since its value will not apply
            // to the newly deflated data.
            for (uint32_t i = 0; i < deflateOptionMatch.size(); i++) {
                 writerScParams.replace(deflateOptionMatch.position(i),
                                        deflateOptionMatch.length(i),
                                        "");
            }
            // Does ScParams still have data and is the original writer a
            // member of the ktx suite?
            if (!writerScParams.empty() && origWriterName.empty()) {
                changeWriter = false;
            }
        }
    }

    if (changeWriter) {
        // Create or modify KTXwriter metadata.
        const auto writer = fmt::format("{} {}", commandName, version(options.testrun));
        updateMetadataValue(KTX_WRITER_KEY, writer);
    }

    // Format new writerScParams.
    auto newScParams = fmt::format("{}", options.compressOptions);
    // Options always contain a leading space
    assert(newScParams[0] == ' ');
    if (!writerScParams.empty()) {
        if (changeWriter) {
            // Leading space unneeded as this param will be first.
            newScParams.erase(newScParams.begin());
            writerScParams = fmt::format("{} / (from {}) {}", newScParams,
                                         origWriterName, writerScParams);
        } else {
            writerScParams.append(newScParams);
        }
    } else {
        writerScParams = newScParams;
        writerScParams.erase(writerScParams.begin()); // Erase leading space.
    }

    // Add KTXwriterScParams metadata
    updateMetadataValue(KTX_WRITER_SCPARAMS_KEY, writerScParams);

    // Save output file
    const auto outputPath = std::filesystem::path(DecodeUTF8Path(options.outputFilepath));
    if (outputPath.has_parent_path())
        std::filesystem::create_directories(outputPath.parent_path());

    OutputStream outputFile(options.outputFilepath, *this);
    outputFile.writeKTX2(texture, *this);
}

} // namespace ktx

KTX_COMMAND_ENTRY_POINT(ktxDeflate, ktx::CommandDeflate)

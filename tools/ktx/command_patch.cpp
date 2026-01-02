// Copyright 2025 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

#include "command.h"
#include "platform_utils.h"
#include "sbufstream.h"
#include "utility.h"
#include "ktx.h"
#include "dfd.h"
#include "../lib/ktxint.h"
#include <filesystem>
#include <fstream>
#include <iostream>

#include <cxxopts.hpp>
#include <fmt/ostream.h>
#include <fmt/printf.h>


// -------------------------------------------------------------------------------------------------

namespace ktx {

// -------------------------------------------------------------------------------------------------

#define SUPPORT_LIBKTX_OPTION 0

/** @page ktx_patch ktx patch
@internal
@~English

Apply a specified patch to a KTX2 file

@section ktx_patch_synopsis SYNOPSIS
    ktx patch [option...] @e input-file

@section ktx_patch_description DESCRIPTION
    @b ktx @b patch applies the specified patch to the KTX file specified as @e input-file.
    Currently only two operations are supported: make-sized and make-unsized.

@section ktx\_patch\_options OPTIONS
    The following options are available:
    <dl>
        <dt>--op &lt;operation&gt;</dt>
        <dd>Specify the operation to carry out. Must be one of the following:
          <dt>make_sized</dt>
          <dd>If the file is supercompressed and the DFD's bytePlane0 value is 0, change
          the bytesPlane values to the correct values for the DFD's color model and
          write out the modified file.</dd>
          <dt>make_unsized</dt>
          <dd>If the file is supercompressed, change the DFD's bytesPlane values to zero
          making the file unsized. Use only to create files for testing.</dd>
        </dd>
        <dt>-v, --verbose</dt>
        <dd>Print a warning if the input file is not  supercompressed.</dd>
    </dl>
    @snippet{doc} ktx/command.h command options_generic

@section ktx_patch_exitstatus EXIT STATUS
    @snippet{doc} ktx/command.h command exitstatus

@section ktx_patch_history HISTORY

@par Version 4.0
 - Initial version

@section ktx_patch_author AUTHOR
    - Mark Callow [\@MarkCallow]
*/
class CommandPatch : public Command {
    typedef enum  {
        makeSized_e,
        makeUnsized_e
    } Operation;

    struct Options {
        inline static const char* kOperation = "op";
        inline static const char* kVerbose = "verbose";
        std::optional<Operation> operation = makeSized_e;
        bool verbose = false;
#if SUPPORT_LIBKTX_OPTION
        inline static const char* kUseLibktx = "use-libktx"
        inline static const char* kChangeWriter = "change-writer";
        bool changeWriter = false;
        bool useLibKtx = false;
#endif
        void init(cxxopts::Options& opts);
        void process(cxxopts::Options& opts, cxxopts::ParseResult& args, Reporter& report);

            std::optional<CommandPatch::Operation> parseOperation(cxxopts::ParseResult& args,
                                                                  Reporter& report) const {
            static const std::unordered_map<std::string, CommandPatch::Operation> values {
                { "make-sized", Operation::makeSized_e },
                { "make-unsized", Operation::makeUnsized_e },
            };
            std::optional<CommandPatch::Operation> result = {};
            if (args[kOperation].count()) {
                auto opStr = to_lower_copy(args[kOperation].as<std::string>());
                const auto it = values.find(opStr);
                if (it != values.end()) {
                    result = it->second;
                } else {
                    report.fatal_usage("Invalid or unsupported operation specified as --{} argument: \"{}\".",
                                       kOperation,
                                       args[kOperation].as<std::string>());
                }
            }
            return result;
        }
    };

    Combine<Options, OptionsSingleIn, OptionsGeneric> options;

public:
    virtual int main(int argc, char* argv[]) override;
    virtual void initOptions(cxxopts::Options& opts) override;
    virtual void processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) override;

private:
    void patch(std::FILE* input);
    void patchWithLibKtx(std::FILE* input);
    void executePatch();
};

// -------------------------------------------------------------------------------------------------

int CommandPatch::main(int argc, char* argv[]) {
    try {
        parseCommandLine("ktx patch",
                "Apply specified operation to patch the KTX file specified as the input-file.\n",
                argc, argv);
        executePatch();
        return +rc::SUCCESS;
    } catch (const FatalError& error) {
        return +error.returnCode;
    } catch (const std::exception& e) {
        fmt::print(std::cerr, "{} fatal: {}\n", commandName, e.what());
        return +rc::RUNTIME_ERROR;
    }
}

void CommandPatch::Options::init(cxxopts::Options& opts) {
    opts.add_options()
#if SUPPORT_LIBKTX_OPTION
        (kChangeWriter, "Change KTXwriter metadata to show this program. Only usable when making sized.")
#endif
        (kOperation, "The patch operation to perform. It must be one of:"
             "\n    make-sized"
             "\n    make-unsized",
             cxxopts::value<std::string>(), "<operation>")
        (kVerbose, "Print a warning if the input file is not supercompressed.");
}

void CommandPatch::Options::process(cxxopts::Options&,
                                    cxxopts::ParseResult& args,
                                    Reporter& report) {
    verbose = args[kVerbose].as<bool>();
#if SUPPORTLIBKTX_OPTION
    changeWriter = args[kChangeWriter].as<bool>();
#endif
    operation = parseOperation(args, report);
}

void CommandPatch::initOptions(cxxopts::Options& opts) {
    options.init(opts);
}

void CommandPatch::processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) {
    options.process(opts, args, *this);
}

// This way of patching has the following advantages:
//
// 1. Can patch a file with a deliberate error that would be rejected by libktx, such as
//    many files in the CTS. But this does expect the file to have a valid identifier and
//    DFD byteOffset and byteLength;
// 2. It will not modify KTXwriter metadata.
//
// and the following disadvantage:
//
// 1. it cannot be used for operations that will change the length of the file, such as
//    may happen when changing metadata.
void CommandPatch::patch(std::FILE* input) {
    KTX_header2 header;

    std::size_t count = std::fread(&header, KTX2_HEADER_SIZE, 1, input);
    if (count != 1) {
        fatal(rc::IO_FAILURE, "Failed to read KTX header from input file \"{}\".",
              options.inputFilepath);
    }

    ktx_uint8_t identifier_reference[12] = KTX2_IDENTIFIER_REF;
    /* Compare identifier, is this a KTX2 file? */
    if (memcmp(header.identifier, identifier_reference, 12) != 0)
        fatal(rc::INVALID_FILE, "Input file \"{}\" is not a KTX v2 file.", options.inputFilepath);

    if (header.supercompressionScheme == KTX_SS_NONE) {
        if (options.verbose)
            warning("Input file \"{}\" is not supercompressed.",
                    options.inputFilepath);
        return;
    }

    if (header.dataFormatDescriptor.byteOffset == 0 || header.dataFormatDescriptor.byteLength == 0) {
        fatal(rc::INVALID_FILE, "KTX header has 0 DFD offset or length.");
    }
    if (std::fseek(input, header.dataFormatDescriptor.byteOffset, SEEK_SET)) {
        fatal(rc::IO_FAILURE, "Failed to seek to DFD in input file \"{}\".",
              options.inputFilepath);
    }
    uint32_t* DFD = reinterpret_cast<uint32_t*>(new uint8_t[header.dataFormatDescriptor.byteLength]);
    count = std::fread(DFD, header.dataFormatDescriptor.byteLength, 1, input);
    if (count != 1) {
        fatal(rc::IO_FAILURE, "Failed to read DFD from input file \"{}\".",
              options.inputFilepath);
    }
    if (options.operation == Operation::makeUnsized_e) {
        KHR_DFDSETVAL(DFD+1, BYTESPLANE0, 0);
        KHR_DFDSETVAL(DFD+1, BYTESPLANE1, 0);
    } else {
        reconstructDFDBytesPlanesFromSamples(DFD);
    }
    std::fseek(input, header.dataFormatDescriptor.byteOffset, SEEK_SET); // Seek to start of DFD.
    fwrite(DFD, header.dataFormatDescriptor.byteLength, 1, input);
}

#if SUPPORT_LIBKTX_OPTION
// This is an alternate way to perform patching bu using libktx to create a ktxTexture.
// Kept for reference. This was has the following advantage:
//
// 1. it can be used for operations that will change the length of the file;
//
// and the following disadvantages:
//
// 1. it will only work on valid (per libktx's checks) KTX files;
// 2. KTXwriter will be updated to show either the current version of libktx or "__default__".
//    The latter happens if the current or newly provided app id contains "__default__".
void CommandPatch::patchWithLibKtx(std::FILE* input) {
    ktxTexture2* texture;
    auto ret = ktxTexture2_CreateFromStdioStream(input, KTX_TEXTURE_CREATE_NO_FLAGS, &texture);
    if (ret != KTX_SUCCESS)
        fatal(rc::INVALID_FILE, "Failed to create KTX2 texture: {}", ktxErrorString(ret));
    // Creating the texture2 object fixes the bytesPlane fields.
    switch (texture->supercompressionScheme) {
      case KTX_SS_BASIS_LZ:
      {
        uint32_t* pBdb = texture->pDfd + 1;
        if (KHR_DFDVAL(pBdb, MODEL) != KHR_DF_MODEL_ETC1S) {
            std::fclose(input);
            fatal(rc::DFD_FAILURE, "Input file \"{}\" has invalid color model for supercompression scheme",
                  options.inputFilepath);
        }
      }
        [[fallthrough]];
      case KTX_SS_ZLIB:
      case KTX_SS_ZSTD:
        // Loading the data causes any unsized DFDs to be reconstructed into sized.
        ret = ktxTexture2_LoadDeflatedImageData(texture, nullptr, 0);
        if (ret != KTX_SUCCESS)
            fatal(rc::INVALID_FILE, "Failed to load data for KTX2 texture: {}", ktxErrorString(ret));
        break;
      case KTX_SS_NONE:
        if (options.verbose)
            warning("Input file \"{}\" is not supercompressed.",
                    options.inputFilepath);
        [[fallthrough]];
      default:
        std::fclose(input);
        return;
    }

    if (options.operation == Operation::makeUnsized_e) {
      KHR_DFDSETVAL(texture->pDfd+1, BYTESPLANE0, 0);
      KHR_DFDSETVAL(texture->pDfd+1, BYTESPLANE1, 0);
    }

    const auto updateMetadataValue = [&](const char* const key,
                                 const std::string& value) {
        ktxHashList_DeleteKVPair(&texture->kvDataHead, key);
        ktxHashList_AddKVPair(&texture->kvDataHead, key,
                static_cast<uint32_t>(value.size() + 1), // +1 to include \0
                value.c_str());
    };

    if (options.changeWriter) {
        // Create or modify KTXwriter metadata.
        const auto writer = fmt::format("{} {}", commandName, version(options.testrun));
        updateMetadataValue(KTX_WRITER_KEY, writer);
    }

    std::fseek(input, 0, SEEK_SET); // Seek to start.
    ktxTexture_WriteToStdioStream(ktxTexture(texture), input);
}
#endif

void CommandPatch::executePatch() {

    std::FILE* input = std::fopen(options.inputFilepath.c_str(), "r+b");
    if (input == nullptr)
        fatal(rc::IO_FAILURE, "Could not open input file \"{}\": {}.",
              options.inputFilepath, errnoMessage());

#if SUPPORT_LIBKTX_OPTION
    if (options.useLibKtx)
        patchWithLibKtx(input);
    else
#endif
        patch(input);

    std::fclose(input);
}

} // namespace ktx

KTX_COMMAND_ENTRY_POINT(ktxPatch, ktx::CommandPatch)

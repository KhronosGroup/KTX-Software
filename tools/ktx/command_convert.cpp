// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include "ktx.h"
#include "ktxint.h"
#include "command.h"
#include "platform_utils.h"
#include "sbufstream.h"
#include "validate.h"

#include <filesystem>
#include <regex>
#include <cxxopts.hpp>
#include <fmt/ostream.h>
#include <fmt/printf.h>

extern "C" const char* glInternalformatString(GLenum);

// -------------------------------------------------------------------------------------------------

namespace ktx {

// -------------------------------------------------------------------------------------------------

/** @page ktx_convert ktx convert
@~English

Convert another texture file type to a KTX2 file

@section ktx\_convert\_synopsis SYNOPSIS
    ktx convert [option...] @e input-file [@e output-path]

@section ktx\_convert\_description DESCRIPTION
    @b ktx @b convert converts the file specified as the @e input-file argument
    to KTX v2 and saves it in the @e output-path. If the @e input-file is '-'
    the file will be read from the stdin. If the @e output-path is '-' the output
    file will be written to the stdout. If @e output-path is a directory, the
    output is saved to a file in that directory whose name is the stem of
    the value given in @e input-file with the extension  @c ktx2. If no @e output-path
    is provided, the output is saved to a file whose name is the parent path and stem of
    the value given in  @e input-file with the extension @c ktx2.

    The input file must be of a supported file type. Currently the only supported
    type is KTX v1. Generates an error if the input file type is unrecognized.

    To encode or supercompress the converted file, pipe it to @b ktx @b encode or
    @b ktx @b deflate via stdout.

    Unrecognized metadata with keys beginning "KTX" or "ktx" found in an input
    KTX v1 file, is dropped and a warning is generated.

@section ktx\_convert\_options OPTIONS
    The following options are available:
    <dl>
        <dt>\--input-type &lt;type&gt;</dt>
        <dd>Type of input file. Currently @b type must be @c ktx1. Case insensitive.</dd>
        <dt>\--drop-bad-orientation</dt>
        <dd>Some in-the-wild KTX v1 files have orientation metadata with the key
            "KTXOrientation" instead of KTXorientaion. By default such metadata is
            rewritten with the correct name. This option causes such bad metadata
            to be dropped. Ignored unless @b type is @c ktx1.
        </dd>
    </dl>
    @snippet{doc} ktx/command.h command options_generic

@section ktx_convert_exitstatus EXIT STATUS
    @snippet{doc} ktx/command.h command exitstatus

@section ktx_convert_history HISTORY

@par Version 4.0
 - Initial version

@section ktx_convert_author AUTHOR
    - Mark Callow
*/

class OutputStreamEx : public OutputStream {
public:
    OutputStreamEx(const std::string& filepath, Reporter& report)
        : OutputStream(filepath, report) { }

    void writeKTX2(ktxTexture1* texture, Reporter& report) {
        const auto ret = ktxTexture1_WriteKTX2ToStdioStream(texture, file);
        if (KTX_SUCCESS != ret) {
            if (file != stdout)
                std::filesystem::remove(DecodeUTF8Path(filepath).c_str());
            report.fatal(rc::IO_FAILURE, "Failed to write KTX file \"{}\": KTX error: {}.",
                         filepath, ktxErrorString(ret));
        }
    }
};

class CommandConvert : public Command {
    enum class input_type_e { ktx1, dds };
    struct OptionsConvert {
        inline static const char* kDropBadOrientation = "drop-bad-orientation";
        inline static const char* kType = "type";

        bool dropBadOrientation = false;
        std::optional<input_type_e> inputType;

        void init(cxxopts::Options& opts) {
            opts.add_options()
                (kType, "Specify the type of input file. Currently must be ktx1.",
                  cxxopts::value<std::string>(), "<type>")
                (kDropBadOrientation, "Drop bad orientation metadata, such as \"KTXOrientation\","
                    " instead of fixing it.");
        }

        std::optional<input_type_e> parseInputType(cxxopts::ParseResult& args, const char* argName, Reporter& report) const {
            static const std::unordered_map<std::string, input_type_e> values {
                { "KTX1", input_type_e::ktx1}
            };
            std::optional<input_type_e> result = {};
            if (args[argName].count()) {
                const std::string typeStr = to_upper_copy(args[argName].as<std::string>());
                const auto it = values.find(typeStr);
                if (it != values.end()) {
                    result = it->second;
                } else {
                    report.fatal_usage("Invalid or unsupported type specified as --{} argument: \"{}\".",
                                       argName, args[argName].as<std::string>());
                }
            }
            return result;
        }

        void process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter& report) {
            inputType = parseInputType(args, kType, report);
            if (!inputType.has_value())
                report.fatal_usage("--{} <type> must be specified", kType);

            dropBadOrientation = args[kDropBadOrientation].as<bool>();
        }
    };

    Combine<OptionsConvert, OptionsSingleInSingleOut<true>, OptionsGeneric> options;

public:
    virtual int main(int argc, char* argv[]) override;
    virtual void initOptions(cxxopts::Options& opts) override;
    virtual void processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) override;

private:
    void convertKtx1(InputStream&, OutputStreamEx&);
    void executeConvert();
};

// -------------------------------------------------------------------------------------------------

int CommandConvert::main(int argc, char* argv[]) {
    try {
        parseCommandLine("ktx convert",
                "Convert the non-KTX2 texture file specified as the input-file argument,\n"
                "    optionally supercompress the result, and save it as the output-file.",
                argc, argv);
        executeConvert();
        return +rc::SUCCESS;
    } catch (const FatalError& error) {
        return +error.returnCode;
    } catch (const std::exception& e) {
        fmt::print(std::cerr, "{} fatal: {}\n", commandName, e.what());
        return +rc::RUNTIME_ERROR;
    }
}

void CommandConvert::initOptions(cxxopts::Options& opts) {
    options.init(opts);
}

void CommandConvert::processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) {
    options.process(opts, args, *this);
}

void CommandConvert::executeConvert() {
    InputStream inputStream(options.inputFilepath, *this);

    // In c++20 options.{input,output}Filepath should be changed to u8string
    // and u8path() replaced with path.
    auto outputFilepath = std::filesystem::u8path(options.outputFilepath);
    bool usingInputName = false;
    // If no output path given or output is a directory, use input path/filename
    // changing extension to or adding ".ktx2".
    if (outputFilepath.empty()) {
        outputFilepath = options.inputFilepath;
        usingInputName = true;
    } else if (std::filesystem::is_directory(outputFilepath)) {
        auto inputFilepath = std::filesystem::u8path(options.inputFilepath);
        outputFilepath /= inputFilepath.filename();
        usingInputName = true;
    }
    if (usingInputName) {
        outputFilepath.replace_extension("ktx2");
    }

    // Create or open output file
    if (outputFilepath.has_parent_path()) {
        std::filesystem::create_directories(outputFilepath.parent_path());
    }
    OutputStreamEx outputStream(outputFilepath.u8string(), *this);

    if (options.inputType == input_type_e::ktx1)
        convertKtx1(inputStream, outputStream);

    outputStream.flush();
    std::ostringstream messagesOS;
    InputStream converted(outputFilepath, *this);
    const auto validationResult = validateIOStream(converted,
        fmtInFile(outputFilepath.u8string()), false, false, [&](const ValidationReport& issue) {
        fmt::print(messagesOS, "{}-{:04}: {}\n", toString(issue.type), issue.id, issue.message);
        fmt::print(messagesOS, "    {}\n", issue.details);
    });

    if (validationResult) {
        fatal(ReturnCode(validationResult),
              "Validation of converted file failed. This is likely due to an internal issue"
              " in the tool. If you feel this is so after looking at the validation messages"
              " below, please open an issue at"
              " https://github.com/KhronosGroup/KTX-Software/issues.\n\n{}",
              messagesOS.str());
    }
}

void CommandConvert::convertKtx1(InputStream& inputStream, OutputStreamEx& outputStream) {
    ktxTexture1* texture = nullptr;
    StreambufStream<std::streambuf*> ktx1Stream{inputStream->rdbuf(), std::ios::in | std::ios::binary};
    auto ret = ktxTexture1_CreateFromStream(ktx1Stream.stream(),
                                            KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);
    if (ret != KTX_SUCCESS) {
        if (ret == KTX_UNSUPPORTED_TEXTURE_TYPE) {
            inputStream->seekg(0);
            KTX_header header;
            inputStream->read(reinterpret_cast<char*>(&header), KTX_HEADER_SIZE);
            fatal(rc::NOT_SUPPORTED,
                  "Format of input file, {}, is unsupported or has no equivalent VkFormat.",
                  ::glInternalformatString(header.glInternalformat));
        } else {
            fatal(rc::INVALID_FILE, "Failed to create KTX texture: {}", ktxErrorString(ret));
        }
    }

    // Some in-the-wild KTX files have incorrect KTXOrientation
    // Warn about dropping invalid metadata.
    ktxHashListEntry* pEntry;
    for (pEntry = texture->kvDataHead;
         pEntry != NULL;
         pEntry = ktxHashList_Next(pEntry)) {
        unsigned int keyLen;
        char* rawKey;

        ktxHashListEntry_GetKey(pEntry, &keyLen, &rawKey);
        auto key = std::string_view(rawKey);
        std::regex re("ktx|KTX");
        std::smatch ktxMatch;
        if (std::regex_search(static_cast<std::string>(key), re)) {
            if (key.compare(KTX_ORIENTATION_KEY)
                && key.compare(KTX_WRITER_KEY)) {
                if (key.compare("KTXOrientation") == 0
                    && !options.dropBadOrientation) {
                        unsigned int orientLen;
                        char* orientation;
                        ktxHashListEntry_GetValue(pEntry,
                                            &orientLen,
                                            (void**)&orientation);
                        ktxHashList_AddKVPair(&texture->kvDataHead,
                                              KTX_ORIENTATION_KEY,
                                              orientLen,
                                              orientation);
               } else {
                   warning("Dropping unrecognized KTX metadata \"{}\"", key);
               }
               ktxHashList_DeleteEntry(&texture->kvDataHead,
                                       pEntry);
            }
        }
    }

    // Add required writer metadata.
    const auto writer = fmt::format("{} {}", commandName, version(options.testrun));
    ktxHashList_AddKVPair(&texture->kvDataHead, KTX_WRITER_KEY,
                         static_cast<uint32_t>(writer.size() + 1),
                         writer.c_str());

    outputStream.writeKTX2(texture, *this);
    ktxTexture_Destroy(ktxTexture(texture));
}

} // namespace ktx

KTX_COMMAND_ENTRY_POINT(ktxConvert, ktx::CommandConvert)

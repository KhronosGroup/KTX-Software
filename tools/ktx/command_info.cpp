// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include "command.h"
#include "sbufstream.h"
#include "stdafx.h"
#include "utility.h"
#include "validate.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

#include <cxxopts.hpp>
#include <fmt/printf.h>
#include <ktx.h>
#include <ktxint.h>


// -------------------------------------------------------------------------------------------------

namespace ktx {

/** @page ktx_info ktx info
@~English

Print information about a KTX2 file.

@section ktx_info_synopsis SYNOPSIS
    ktx info [option...] @e input-file

@section ktx_info_description DESCRIPTION
    @b ktx @b info prints information about the KTX2 file specified as the @e input-file argument.
    If the @e input-file is '-' the file will be read from the stdin.
    The command implicitly calls @ref ktx_validate "validate" and prints any found errors
    and warnings to stdout.
    If the specified input file is invalid the information is displayed based on best effort and
    may be incomplete.

    The JSON output formats conform to the https://schema.khronos.org/ktx/info_v0.json
    schema even if the input file is invalid and certain information cannot be parsed or
    displayed.
    Additionally, for JSON outputs the KTX file identifier is printed using "\u001A" instead of
    "\x1A" as an unescaped "\x1A" sequence inside a JSON string breaks nearly every JSON tool.
    Note that this does not change the value of the string only its representation.

    @note @b ktx @b info prints using UTF-8 encoding. If your console is not
    set for UTF-8 you will see incorrect characters in output of the file
    identifier on each side of the "KTX nn".

    The following options are available:
    @snippet{doc} ktx/command.h command options_format
    @snippet{doc} ktx/command.h command options_generic

@section ktx_info_exitstatus EXIT STATUS
    @snippet{doc} ktx/command.h command exitstatus

@section ktx_info_history HISTORY

@par Version 4.0
 - Initial version

@section ktx_info_author AUTHOR
    - Mátyás Császár [Vader], RasterGrid www.rastergrid.com
    - Daniel Rákos, RasterGrid www.rastergrid.com
*/
class CommandInfo : public Command {
    Combine<OptionsFormat, OptionsSingleIn, OptionsGeneric> options;

public:
    virtual int main(int argc, char* argv[]) override;
    virtual void initOptions(cxxopts::Options& opts) override;
    virtual void processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) override;

private:
    void executeInfo();
    KTX_error_code printInfoText(std::istream& file);
    KTX_error_code printInfoJSON(std::istream& file, bool minified);
};

// -------------------------------------------------------------------------------------------------

int CommandInfo::main(int argc, char* argv[]) {
    try {
        parseCommandLine("ktx info",
                "Prints information about the KTX2 file specified as the input-file argument.\n"
                "    The command implicitly calls validate and prints any found errors\n"
                "    and warnings to stdout.",
                argc, argv);
        executeInfo();
        return to_underlying(rc::SUCCESS);
    } catch (const FatalError& error) {
        return +error.returnCode;
    } catch (const std::exception& e) {
        fmt::print(std::cerr, "{} fatal: {}\n", commandName, e.what());
        return +rc::RUNTIME_ERROR;
    }
}

void CommandInfo::initOptions(cxxopts::Options& opts) {
    options.init(opts);
}

void CommandInfo::processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) {
    options.process(opts, args, *this);
}

void CommandInfo::executeInfo() {
    InputStream inputStream(options.inputFilepath, *this);

    KTX_error_code result;

    switch (options.format) {
    case OutputFormat::text:
        result = printInfoText(inputStream);
        break;
    case OutputFormat::json:
        result = printInfoJSON(inputStream, false);
        break;
    case OutputFormat::json_mini:
        result = printInfoJSON(inputStream, true);
        break;
    default:
        assert(false && "Internal error");
        return;
    }

    if (result != KTX_SUCCESS)
        fatal(rc::INVALID_FILE, "Failed to process KTX2 file \"{}\": {}", fmtInFile(options.inputFilepath), ktxErrorString(result));
}

KTX_error_code CommandInfo::printInfoText(std::istream& file) {
    std::ostringstream messagesOS;
    const auto validationResult = validateIOStream(file, fmtInFile(options.inputFilepath), false, false, [&](const ValidationReport& issue) {
        fmt::print(messagesOS, "{}-{:04}: {}\n", toString(issue.type), issue.id, issue.message);
        fmt::print(messagesOS, "    {}\n", issue.details);
    });

    fmt::print("Validation {}\n", validationResult == 0 ? "successful" : "failed");
    const auto validationMessages = std::move(messagesOS).str();
    if (!validationMessages.empty()) {
        fmt::print("\n");
        fmt::print("{}", validationMessages);
    }
    fmt::print("\n");

    file.clear(); // Clear any unexpected EOF from validation
    file.seekg(0);
    if (!file)
        return validationResult == 0 ? KTX_FILE_SEEK_ERROR : KTX_SUCCESS;

    StreambufStream<std::streambuf*> ktx2Stream{file.rdbuf(), std::ios::in | std::ios::binary};
    const auto result = ktxPrintKTX2InfoTextForStream(ktx2Stream.stream());

    return validationResult == 0 ? result : KTX_SUCCESS;
}

KTX_error_code CommandInfo::printInfoJSON(std::istream& file, bool minified) {
    const auto base_indent = minified ? 0 : +0;
    const auto indent_width = minified ? 0 : 4;
    const auto space = minified ? "" : " ";
    const auto nl = minified ? "" : "\n";

    std::ostringstream messagesOS;
    PrintIndent pi{messagesOS, base_indent, indent_width};

    bool first = true;
    const auto validationResult = validateIOStream(file, fmtInFile(options.inputFilepath), false, false, [&](const ValidationReport& issue) {
        if (!std::exchange(first, false)) {
            pi(2, "}},{}", nl);
        }
        pi(2, "{{{}", nl);
        pi(3, "\"id\":{}{},{}", space, issue.id, nl);
        pi(3, "\"type\":{}\"{}\",{}", space, toString(issue.type), nl);
        pi(3, "\"message\":{}\"{}\",{}", space, escape_json_copy(issue.message), nl);
        pi(3, "\"details\":{}\"{}\"{}", space, escape_json_copy(issue.details), nl);
    });

    file.clear(); // Clear any unexpected EOF from validation

    // Workaround detection to decide if ktx will produce any output into the json
    // to eliminate a trailing comma
    file.seekg(0, std::ios_base::end);
    const auto fileSize = file.tellg();
    bool fileIdentifierIsCorrect = false;
    if (fileSize >= 12) {
        static constexpr uint8_t ktx2_identifier_reference[12] = KTX2_IDENTIFIER_REF;
        ktx_uint8_t identifier[12];
        file.seekg(0, std::ios_base::beg);
        file.read(reinterpret_cast<char*>(identifier), 12);
        fileIdentifierIsCorrect = std::memcmp(identifier, ktx2_identifier_reference, 12) == 0;
    }
    const auto ktxWillPrintOutput = fileIdentifierIsCorrect && fileSize >= KTX2_HEADER_SIZE;

    PrintIndent out{std::cout, base_indent, indent_width};
    out(0, "{{{}", nl);
    out(1, "\"$schema\":{}\"https://schema.khronos.org/ktx/info_v0.json\",{}", space, nl);
    out(1, "\"valid\":{}{},{}", space, validationResult == 0, nl);
    if (!first) {
        out(1, "\"messages\":{}[{}", space, nl);
        fmt::print("{}", std::move(messagesOS).str());
        out(2, "}}{}", nl);
        out(1, "]{}{}", ktxWillPrintOutput ? "," : "", nl);
    } else {
        out(1, "\"messages\":{}[]{}{}", space, ktxWillPrintOutput ? "," : "", nl);
    }

    file.seekg(0, std::ios_base::beg);
    if (!file) {
        out(0, "}}{}", nl);
        return validationResult == 0 ? KTX_FILE_SEEK_ERROR : KTX_SUCCESS;
    }

    StreambufStream<std::streambuf*> ktx2Stream{file.rdbuf(), std::ios::in | std::ios::binary};
    const auto result = ktxPrintKTX2InfoJSONForStream(ktx2Stream.stream(), base_indent + 1, indent_width, minified);
    out(0, "}}{}", nl);

    return validationResult == 0 ? result : KTX_SUCCESS;
}

} // namespace ktx

KTX_COMMAND_ENTRY_POINT(ktxInfo, ktx::CommandInfo)

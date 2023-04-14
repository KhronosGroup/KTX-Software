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

/** @page ktxtools_info ktx info
@~English

Prints information about a KTX2 file.

@section ktxtools_info_synopsis SYNOPSIS
    ktx info [options] @e input_file

@section ktxtools_info_description DESCRIPTION
    @b ktx @b info prints information about the KTX2 file provided as argument.

    @note @b ktx @b info prints using UTF-8 encoding. If your console is not
    set for UTF-8 you will see incorrect characters in output of the file
    identifier on each side of the "KTX nn".

    The following options are available:
    <dl>
        <dt>--format &lt;text|json|mini-json&gt;</dt>
        <dd>Specifies the output format.
            @b text - Human readable text based format.
            @b json - Formatted JSON.
            @b mini-json - Minified JSON (Every optional formatting is skipped).
            The default format is @b text.
        </dd>
    </dl>
    @snippet{doc} ktx/command.h command options

@section ktxtools_info_exitstatus EXIT STATUS
    @b ktx @b info exits
        0 - Success
        1 - Command line error
        2 - IO error

@section ktxtools_info_history HISTORY

@par Version 4.0
 - Initial version

@section ktxtools_info_author AUTHOR
    - Mátyás Császár [Vader], RasterGrid www.rastergrid.com
    - Daniel Rákos, RasterGrid www.rastergrid.com
*/
class CommandInfo : public Command {
    Combine<OptionsFormat, OptionsSingleIn, OptionsGeneric> options;

public:
    virtual int main(int argc, _TCHAR* argv[]) override;
    virtual void initOptions(cxxopts::Options& opts) override;
    virtual void processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) override;

private:
    void executeInfo();
    KTX_error_code printInfoText(std::istream& file);
    KTX_error_code printInfoJSON(std::istream& file, bool minified);
};

// -------------------------------------------------------------------------------------------------

int CommandInfo::main(int argc, _TCHAR* argv[]) {
    try {
        parseCommandLine("ktx info",
                "Prints information about the KTX2 file provided as argument to the stdout.",
                argc, argv);
        executeInfo();
        return RETURN_CODE_SUCCESS;
    } catch (const FatalError& error) {
        return error.return_code;
    } catch (const std::exception& e) {
        fmt::print(std::cerr, "{} fatal: {}\n", commandName, e.what());
        return RETURN_CODE_RUNTIME_ERROR;
    }
}

void CommandInfo::initOptions(cxxopts::Options& opts) {
    options.init(opts);
}

void CommandInfo::processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) {
    options.process(opts, args, *this);
}

void CommandInfo::executeInfo() {
    std::ifstream file(options.inputFilepath, std::ios::binary | std::ios::in);
    if (!file)
        fatal(rc::IO_FAILURE, "Could not open input file \"{}\": {}", options.inputFilepath, errnoMessage());

    KTX_error_code result;

    switch (options.format) {
    case OutputFormat::text:
        result = printInfoText(file);
        break;
    case OutputFormat::json:
        result = printInfoJSON(file, false);
        break;
    case OutputFormat::json_mini:
        result = printInfoJSON(file, true);
        break;
    default:
        assert(false && "Internal error");
        return;
    }

    if (result != KTX_SUCCESS)
        fatal(rc::INVALID_FILE, "Failed to process KTX2 file \"{}\": {}", options.inputFilepath, ktxErrorString(result));
}

KTX_error_code CommandInfo::printInfoText(std::istream& file) {
    std::ostringstream messagesOS;
    const auto validationResult = validateIOStream(file, options.inputFilepath, false, false, [&](const ValidationReport& issue) {
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
    const auto validationResult = validateIOStream(file, options.inputFilepath, false, false, [&](const ValidationReport& issue) {
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

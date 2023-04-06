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
class CommandInfo : public CommandWithFormat {
public:
    using CommandWithFormat::CommandWithFormat;
    virtual ~CommandInfo() {};

    virtual int main(int argc, _TCHAR* argv[]) override;

protected:
    virtual void initOptions(cxxopts::Options& options) override;
    virtual void processOptions(cxxopts::Options& options, cxxopts::ParseResult& args) override;

private:
    void executeInfo();
    KTX_error_code printInfoText(std::istream& file);
    KTX_error_code printInfoJSON(std::istream& file, bool minified);
};

// -------------------------------------------------------------------------------------------------

int CommandInfo::main(int argc, _TCHAR* argv[]) {
    try {
        parseCommandLine("ktx info",
                "Prints information about the KTX2 file provided as argument to the stdout.\n",
                argc, argv);
        executeInfo();
        return RETURN_CODE_SUCCESS;
    } catch (const FatalError& error) {
        return error.return_code;
    }
}

void CommandInfo::initOptions(cxxopts::Options& options) {
    CommandWithFormat::initOptions(options);
}

void CommandInfo::processOptions(cxxopts::Options& options, cxxopts::ParseResult& args) {
    CommandWithFormat::processOptions(options, args);
}

void CommandInfo::executeInfo() {
    std::ifstream file(inputFile, std::ios::binary | std::ios::in);

    if (!file) {
        fmt::print(stderr, "{}: Could not open input file \"{}\": {}\n", processName, inputFile, errnoMessage());
        throw FatalError(RETURN_CODE_IO_FAILURE);
    }

    KTX_error_code result;

    switch (format) {
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

    if (result ==  KTX_FILE_UNEXPECTED_EOF) {
        fmt::print(stderr, "{}: Unexpected end of file reading \"{}\".\n", processName, inputFile);
        throw FatalError(RETURN_CODE_IO_FAILURE);
    } else if (result == KTX_UNKNOWN_FILE_FORMAT) {
        fmt::print(stderr, "{}: \"{}\" is not a KTX2 file.\n", processName, inputFile);
        throw FatalError(RETURN_CODE_IO_FAILURE);
    } else if (result != KTX_SUCCESS) {
        fmt::print(stderr, "{}: {} failed to process KTX2 file: {} - {}\n", processName, inputFile, static_cast<int>(result), ktxErrorString(result));
        throw FatalError(RETURN_CODE_IO_FAILURE);
    }
}

KTX_error_code CommandInfo::printInfoText(std::istream& file) {
    std::ostringstream messagesOS;
    const auto validationResult = validateIOStream(file, false, false, [&](const ValidationReport& issue) {
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

    file.seekg(0);
    if (!file) {
        fmt::print(stderr, "{}: Could not rewind the input file: {}\n", processName, errnoMessage());
        return KTX_FILE_SEEK_ERROR;
    }

    StreambufStream<std::streambuf*> ktx2Stream{file.rdbuf(), std::ios::in | std::ios::binary};
    return ktxPrintKTX2InfoTextForStream(ktx2Stream.stream());
}

KTX_error_code CommandInfo::printInfoJSON(std::istream& file, bool minified) {
    const auto base_indent = minified ? 0 : +0;
    const auto indent_width = minified ? 0 : 4;
    const auto space = minified ? "" : " ";
    const auto nl = minified ? "" : "\n";

    std::ostringstream messagesOS;
    PrintIndent pi{messagesOS, base_indent, indent_width};

    bool first = true;
    const auto validationResult = validateIOStream(file, false, false, [&](const ValidationReport& issue) {
        if (!std::exchange(first, false)) {
            pi(2, "}},{}", nl);
        }
        pi(2, "{{{}", nl);
        pi(3, "\"id\":{}{},{}", space, issue.id, nl);
        pi(3, "\"type\":{}\"{}\",{}", space, toString(issue.type), nl);
        pi(3, "\"message\":{}\"{}\",{}", space, escape_json_copy(issue.message), nl);
        pi(3, "\"details\":{}\"{}\"{}", space, escape_json_copy(issue.details), nl);
    });

    PrintIndent out{std::cout, base_indent, indent_width};
    out(0, "{{{}", nl);
    out(1, "\"$schema\":{}\"https://schema.khronos.org/ktx/info_v0.json\",{}", space, nl);
    out(1, "\"valid\":{}{},{}", space, validationResult == 0, nl);
    if (!first) {
        out(1, "\"messages\":{}[{}", space, nl);
        fmt::print("{}", std::move(messagesOS).str());
        out(2, "}}{}", nl);
        out(1, "],{}", nl);
    } else {
        out(1, "\"messages\":{}[],{}", space, nl);
    }

    file.seekg(0);
    if (!file) {
        out(0, "}}{}", nl);
        fmt::print(stderr, "{}: Could not rewind the input file: {}\n", processName, errnoMessage());
        return KTX_FILE_SEEK_ERROR;
    }

    StreambufStream<std::streambuf*> ktx2Stream{file.rdbuf(), std::ios::in | std::ios::binary};
    const auto ktx_ec = ktxPrintKTX2InfoJSONForStream(ktx2Stream.stream(), base_indent + 1, indent_width, minified);
    out(0, "}}{}", nl);

    return ktx_ec;
}

} // namespace ktx

KTX_COMMAND_ENTRY_POINT(ktxInfo, ktx::CommandInfo)

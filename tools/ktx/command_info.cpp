// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#include "command.h"

#include <ktx.h>
#include <fmt/os.h> // For std::error_code
#include <fmt/printf.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

#include "sbufstream.h"
#include "stdafx.h"
#include "utility.h"
#include "validate.h"


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
        0 on success,
        1 on command line errors and
        2 if the input file parsing failed.

@section ktxtools_info_history HISTORY

@par Version 4.0
 - Initial version

@section ktxtools_info_author AUTHOR
    - Mátyás Császár [Vader], RasterGrid www.rastergrid.com
    - Daniel Rákos, RasterGrid www.rastergrid.com
*/

class CommandInfo : public Command {
    struct Options {
        enum class OutputFormat {
            text,
            json,
            json_mini,
        };

        OutputFormat format = OutputFormat::text;
        _tstring inputFilepath;
    };

    Options options;

public:
    // TODO Tools P5: Support --version
    // TODO Tools P5: Support --help with proper usage
    void initializeOptions();
    virtual bool processOption(argparser& parser, int opt) override;
    void processPositional(const std::vector<_tstring>& infiles, const _tstring& outfile);
    virtual int main(int argc, _TCHAR* argv[]) override;

    using Command::Command;
    virtual ~CommandInfo() {};

private:
    int printInfo(const _tstring& infile, Options::OutputFormat format);
    KTX_error_code printInfoText(std::istream& file);
    KTX_error_code printInfoJSON(std::istream& file, bool minified);
};

// -------------------------------------------------------------------------------------------------

void CommandInfo::initializeOptions() {
    option_list.emplace(option_list.begin(), "format", argparser::option::required_argument, nullptr, 'f');
    short_opts += "f:";
}

bool CommandInfo::processOption(argparser& parser, int opt) {
    switch (opt) {
    case 'f':
        if (parser.optarg == "text") {
            options.format = Options::OutputFormat::text;
        } else if (parser.optarg == "json") {
            options.format = Options::OutputFormat::json;
        } else if (parser.optarg == "mini-json") {
            options.format = Options::OutputFormat::json_mini;
        } else {
            // TODO Tools P5: Print usage, Failure: unsupported format
            std::cerr << "Print usage, Failure: unsupported format" << std::endl;
            return false;
        }
        break;
    default:
        return false;
    }

    return true;
}

void CommandInfo::processPositional(const std::vector<_tstring>& infiles, const _tstring& outfile) {
    if (infiles.size() > 1) {
        // TODO Tools P5: Print usage, Failure: infiles.size() > 1
        std::cerr << "Print usage, Failure: infiles.size() > 1" << std::endl;
        // TODO Tools P1: Instead of std::exit handle argument parsing failures and stop execution
        std::exit(1);
        // return false;
    }

    options.inputFilepath = infiles[0];
    (void) outfile;
}

int CommandInfo::main(int argc, _TCHAR* argv[]) {
    initializeOptions();
    processCommandLine(argc, argv, StdinUse::eDisallowStdin, OutfilePos::eNone);
    processPositional(genericOptions.infiles, genericOptions.outfile);

    return printInfo(options.inputFilepath, options.format);
}

int CommandInfo::printInfo(const _tstring& infile, Options::OutputFormat format) {
    std::ifstream file(infile, std::ios::binary | std::ios::in);

    if (!file) {
        fmt::print(stderr, "{}: Could not open input file \"{}\": {}\n", processName, infile, errnoMessage());
        return 2;
    }

    KTX_error_code result;

    switch (format) {
    case Options::OutputFormat::text:
        result = printInfoText(file);
        break;

    case Options::OutputFormat::json:
        result = printInfoJSON(file, false);
        break;

    case Options::OutputFormat::json_mini:
        result = printInfoJSON(file, true);
        break;

    default:
        assert(false && "Internal error");
        return EXIT_FAILURE;
    }

    if (result ==  KTX_FILE_UNEXPECTED_EOF) {
        fmt::print(stderr, "{}: Unexpected end of file reading \"{}\".\n", processName, infile);
        return 2;

    } else if (result == KTX_UNKNOWN_FILE_FORMAT) {
        fmt::print(stderr, "{}: \"{}\" is not a KTX2 file.\n", processName, infile);
        return 2;

    } else if (result != KTX_SUCCESS) {
        fmt::print(stderr, "{}: {} failed to process KTX2 file: ERROR_CODE {}\n", processName, infile, static_cast<int>(result));
        return 2;
    }

    return EXIT_SUCCESS;
}

KTX_error_code CommandInfo::printInfoText(std::istream& file) {
    std::ostringstream messagesOS;
    const auto validationResult = validateIOStream(file, false, [&](const ValidationReport& issue) {
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
    const char* space = minified ? "" : " ";
    const char* nl = minified ? "" : "\n";

    std::ostringstream messagesOS;
    PrintIndent pi{messagesOS, base_indent, indent_width};

    bool first = true;
    const auto validationResult = validateIOStream(file, false, [&](const ValidationReport& issue) {
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

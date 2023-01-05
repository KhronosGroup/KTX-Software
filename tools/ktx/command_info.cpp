// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#include "command.h"

#include <ktx.h>

#include <iostream>

#include "stdafx.h"


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
    // TODO KTX Tools P5: Support --version
    // TODO KTX Tools P5: Support --help with proper usage
    void initializeOptions();
    virtual bool processOption(argparser& parser, int opt) override;
    void processPositional(const std::vector<_tstring>& infiles, const _tstring& outfile);
    virtual int main(int argc, _TCHAR* argv[]) override;

    using Command::Command;
    virtual ~CommandInfo() {};

private:
    int printInfo(const _tstring& infile, Options::OutputFormat format);
    KTX_error_code printInfoText(FILE* inf);
    KTX_error_code printInfoJSON(FILE* inf, bool minified);
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
            // TODO KTX Tools P5: Print usage, Failure: unsupported format
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
        // TODO KTX Tools P5: Print usage, Failure: infiles.size() > 1
        std::cerr << "Print usage, Failure: infiles.size() > 1" << std::endl;
        // TODO KTX Tools P1: Instead of std::exit handle argument parsing failures and stop execution
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
    FILE* inf;

    // TODO KTX Tools P5: fclose?
#ifdef _WIN32
    _tfopen_s(&inf, infile.c_str(), "rb");
#else
    inf = _tfopen(infile.c_str(), "rb");
#endif

    if (!inf) {
        // TODO KTX Tools P5: Is strerror depricated?
        std::cerr << processName << ": Could not open input file \"" << infile << "\". " << strerror(errno) << std::endl;
        return 2;
    }

    KTX_error_code result;

    switch (format) {
    case Options::OutputFormat::text:
        result = printInfoText(inf);
        break;

    case Options::OutputFormat::json:
        result = printInfoJSON(inf, false);
        break;

    case Options::OutputFormat::json_mini:
        result = printInfoJSON(inf, true);
        break;

    default:
        assert(false && "Internal error");
        return EXIT_FAILURE;
    }

    if (result ==  KTX_FILE_UNEXPECTED_EOF) {
        std::cerr << processName << ": Unexpected end of file reading \"" << infile << "\"." << std::endl;
        return 2;

    } else if (result == KTX_UNKNOWN_FILE_FORMAT) {
        std::cerr << processName << ": " << infile << " is not a KTX2 file." << std::endl;
        return 2;

    } else if (result != KTX_SUCCESS) {
        std::cerr << processName << ": " << infile << " failed to process KTX2 file: ERROR_CODE " << result << std::endl;
        return 2;
    }

    return EXIT_SUCCESS;
}

KTX_error_code CommandInfo::printInfoText(FILE* inf) {
    return ktxPrintKTX2InfoTextForStdioStream(inf);
}

KTX_error_code CommandInfo::printInfoJSON(FILE* inf, bool minified) {
    const char* space = minified ? "" : " ";
    const char* nl = minified ? "" : "\n";

    std::cout << "{" << nl;
    // TODO KTX Tools P5: ktx-schema-url-1.0 will has to be replaced with the actual URL
    std::cout << (minified ? "" : "    ") << "\"$id\":" << space << "\"ktx-schema-url-1.0\"," << nl;

    // TODO KTX Tools P4: Call validate JSON print and include "valid" and "messages" in the JSON output
    // result = validateAndPrintJSON(inf, 1, 4, minified);
    // std::cout << "," << nl;
    const auto ec = ktxPrintKTX2InfoJSONForStdioStream(inf, 1, 4, minified);
    std::cout << "}" << nl;

    return ec;
}

} // namespace ktx

KTX_COMMAND_ENTRY_POINT(ktxInfo, ktx::CommandInfo)

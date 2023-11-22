// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#include "command.h"
#include "platform_utils.h"
#include "stdafx.h"
#include <iostream>
#include <string>
#include <unordered_map>

#include <cxxopts.hpp>
#include <fmt/ostream.h>
#include <fmt/printf.h>

// -------------------------------------------------------------------------------------------------

namespace ktx {

/** @page ktx ktx
@~English

Unified CLI frontend for the KTX-Software library.

@section ktx_synopsis SYNOPSIS
    ktx &lt;command&gt; [command-option...]<br>
    ktx [option...]

@section ktx_description DESCRIPTION
    Unified CLI frontend for the KTX-Software library with sub-commands for specific operations
    for the KTX File Format Specification
    https://registry.khronos.org/KTX/specs/2.0/ktxspec.v2.html.

    The following commands are available:
    <dl>
        <dt>@ref ktx_create "create"</dt>
        <dd>
            Create a KTX2 file from various input files.
        </dd>
        <dt>@ref ktx_extract "extract"</dt>
        <dd>
            Extract selected images from a KTX2 file.
        </dd>
        <dt>@ref ktx_encode "encode"</dt>
        <dd>
            Encode a KTX2 file.
        </dd>
        <dt>@ref ktx_transcode "transcode"</dt>
        <dd>
            Transcode a KTX2 file.
        </dd>
        <dt>@ref ktx_info "info"</dt>
        <dd>
            Print information about a KTX2 file.
        </dd>
        <dt>@ref ktx_validate "validate"</dt>
        <dd>
            Validate a KTX2 file.
        </dd>
        <dt>@ref ktx_help "help"</dt>
        <dd>
            Display help information about the ktx tool.
        </dd>
    </dl>

    The following options are also available without a command:
    @snippet{doc} ktx/command.h command options_generic

@section ktx_exitstatus EXIT STATUS
    @snippet{doc} ktx/command.h command exitstatus

@section ktx_history HISTORY

@par Version 4.0
 - Initial version

@section ktx_author AUTHOR
    - Mátyás Császár [Vader], RasterGrid www.rastergrid.com
    - Daniel Rákos, RasterGrid www.rastergrid.com
*/

class Tools : public Command {
    bool testrun = false; /// Indicates test run. If enabled ktx tools will only include the default version information in any output

public:
    using Command::Command;
    virtual ~Tools() {};

public:
    virtual int main(int argc, char* argv[]) override;
    void printUsage(std::ostream& os, const cxxopts::Options& options);
};

// -------------------------------------------------------------------------------------------------

int Tools::main(int argc, char* argv[]) {
    cxxopts::Options options("ktx", "");
    options.custom_help("[--version] [--help] <command> <command-args>");
    options.set_width(CONSOLE_USAGE_WIDTH);
    options.add_options()
            ("h,help", "Print this usage message and exit")
            ("v,version", "Print the version number of this program and exit")
            ("testrun", "Indicates test run. If enabled the tool will produce deterministic output whenever possible");

    options.allow_unrecognised_options();

    cxxopts::ParseResult args;
    try {
        args = options.parse(argc, argv);
    } catch (const std::exception& ex) {
        fmt::print(std::cerr, "{}: {}\n", options.program(), ex.what());
        printUsage(std::cerr, options);
        return +rc::INVALID_ARGUMENTS;
    }

    testrun = args["testrun"].as<bool>();

    if (args.count("help")) {
        fmt::print(std::cout, "{}: Unified CLI frontend for the KTX-Software library with sub-commands for specific operations.\n", options.program());
        printUsage(std::cout, options);
        return +rc::SUCCESS;
    }

    if (args.count("version")) {
        fmt::print("{} version: {}\n", options.program(), version(testrun));
        return +rc::SUCCESS;
    }

    if (args.unmatched().empty()) {
        fmt::print(std::cerr, "{}: Missing command.\n", options.program());
        printUsage(std::cerr, options);
    } else {
        fmt::print(std::cerr, "{}: Unrecognized command: \"{}\"\n", options.program(), args.unmatched()[0]);
        printUsage(std::cerr, options);
    }

    return +rc::INVALID_ARGUMENTS;
}

void Tools::printUsage(std::ostream& os, const cxxopts::Options& options) {
    fmt::print(os, "{}", options.help());

    fmt::print(os, "\n");
    fmt::print(os, "Available commands:\n");
    fmt::print(os, "  create     Create a KTX2 file from various input files\n");
    fmt::print(os, "  extract    Extract selected images from a KTX2 file\n");
    fmt::print(os, "  encode     Encode a KTX2 file\n");
    fmt::print(os, "  transcode  Transcode a KTX2 file\n");
    fmt::print(os, "  info       Print information about a KTX2 file\n");
    fmt::print(os, "  validate   Validate a KTX2 file\n");
    fmt::print(os, "  help       Display help information about the ktx tool\n");
    fmt::print(os, "\n");
    fmt::print(os, "For detailed usage and description of each subcommand use 'ktx help <command>'\n"
                   "or 'ktx <command> --help'\n");
}

} // namespace ktx ---------------------------------------------------------------------------------

KTX_COMMAND_BUILTIN(ktxCreate)
KTX_COMMAND_BUILTIN(ktxExtract)
KTX_COMMAND_BUILTIN(ktxEncode)
KTX_COMMAND_BUILTIN(ktxTranscode)
KTX_COMMAND_BUILTIN(ktxInfo)
KTX_COMMAND_BUILTIN(ktxValidate)
KTX_COMMAND_BUILTIN(ktxHelp)

std::unordered_map<std::string, ktx::pfnBuiltinCommand> builtinCommands = {
    { "create",     ktxCreate },
    { "extract",    ktxExtract },
    { "encode",     ktxEncode },
    { "transcode",  ktxTranscode },
    { "info",       ktxInfo },
    { "validate",   ktxValidate },
    { "help",       ktxHelp }
};

int main(int argc, char* argv[]) {
    // If -NSDocumentRevisionsDebugMode YES ever causes any problem it should be discarded here
    // by creating a new argc and argv pair and excluding the problematic arguments from them.
    // This way downstream tools will not have to deal with this issue
    //      // -NSDocumentRevisionsDebugMode YES is appended to the end
    //      // of the command by Xcode when debugging and "Allow debugging when
    //      // using document Versions Browser" is checked in the scheme. It
    //      // defaults to checked and is saved in a user-specific file not the
    //      // pbxproj file, so it can't be disabled in a generated project.
    //      // Remove these from the arguments under consideration.

    InitUTF8CLI(argc, argv);

    if (argc >= 2) {
        // Has a subcommand, attempt to lookup

        const auto it = builtinCommands.find(argv[1]);
        if (it != builtinCommands.end()) {
            // Call built-in subcommand, trimming the first parameter.
            return it->second(argc - 1, argv + 1);
        } else {
            // In the future it is possible to add further logic here to allow loading command plugins
            // from shared libraries or call external commands. There is no defined configuration
            // mechanism to do so, but the command framework has been designed to be able to build
            // subcommands as separate executables or shared libraries.
        }
    }

    // If no sub-command was specified or if it was not found, call the main command's entry point.
    ktx::Tools cmd;
    return cmd.main(argc, argv);
}

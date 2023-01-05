// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#include "command.h"
#include "stdafx.h"

#include <iostream>
#include <unordered_map>
#include <string>
#include <string_view>

// -------------------------------------------------------------------------------------------------

namespace ktx {

/** @page ktxtools ktx
@~English

Unified CLI frontend for the KTX-Software library.

@section ktxtools_synopsis SYNOPSIS
    ktx @e command @e command_args...<br>
    ktx [options]

@section ktxtools_description DESCRIPTION
    Unified CLI frontend for the KTX-Software library with sub-commands for specific operations.

    The following commands are available:
    <dl>
        <dt>validate</dt>
        <dd>
            Prints information about KTX2 file.
        </dd>
        <dt>info</dt>
        <dd>
            Validates a KTX2 file.
        </dd>
        <dt>transcode</dt>
        <dd>
            @warning TODO KTX Tools P5: This section is incomplete
        </dd>
        <dt>encode</dt>
        <dd>
            @warning TODO KTX Tools P5: This section is incomplete
        </dd>
        <dt>extract</dt>
        <dd>
            @warning TODO KTX Tools P5: This section is incomplete
        </dd>
        <dt>create</dt>
        <dd>
            @warning TODO KTX Tools P5: This section is incomplete
        </dd>
        <dt>help</dt>
        <dd>
            @warning TODO KTX Tools P5: This section is incomplete
        </dd>
    </dl>

    The following options are also available without a command:
    @snippet{doc} ktx/command.h command options

@section ktxtools_exitstatus EXIT STATUS
    @b ktx @b info exits
        0 on success,
        1 on command line errors and
        2 if the input file parsing failed.

@section ktxtools_history HISTORY

@par Version 4.0
 - Initial version

@section ktxtools_author AUTHOR
    - Mátyás Császár [Vader], RasterGrid www.rastergrid.com
    - Daniel Rákos, RasterGrid www.rastergrid.com
*/

class Tools : public Command {
public:
    Tools() {}
    virtual ~Tools() {};

public:
    virtual bool processOption(argparser& parser, int opt) override {
        (void) parser;
        (void) opt;
        // TODO KTX Tools P5: Parse --version
        // TODO KTX Tools P5: Parse --help
        return true;
    }
    virtual int main(int argc, _TCHAR* argv[]) override;
};

int Tools::main(int argc, _TCHAR* argv[]) {
    (void) argv;

    if (argc < 2) {
        // TODO KTX Tools P5: Print usage, Failure: missing sub command
        std::cerr << "Print usage, Failure: missing sub command" << std::endl;
        return EXIT_FAILURE;
    }

    // TODO KTX Tools P5: Print usage, Failure: incorrect sub command {commandName}
    std::cerr << "Print usage, Failure: incorrect sub command {commandName}" << std::endl;
    return EXIT_FAILURE;
}

} // namespace ktx ---------------------------------------------------------------------------------

KTX_COMMAND_BUILTIN(ktxInfo)
KTX_COMMAND_BUILTIN(ktxValidate)

std::unordered_map<std::string, ktx::pfnBuiltinCommand> builtinCommands = {
    { "info",       ktxInfo },
    { "validate",   ktxValidate }
};

int _tmain(int argc, _TCHAR* argv[]) {
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

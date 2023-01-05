// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#include "command.h"
#include <iostream>


// -------------------------------------------------------------------------------------------------

namespace ktx {

// TODO KTX Tools P5: Document the rest of the ktx validate options:
//          --gltf-basisu
//              Check compatibility with KHR_texture_basisu glTF extension. Unset by default.
//          --warnings-as-errors
//              Treat warnings as errors. Unset by default.

/** @page ktxtools_validate ktx validate
@~English

Validates a KTX2 file.

@warning TODO KTX Tools P5: This page is incomplete

@section ktxtools_validate_synopsis SYNOPSIS
    ktx validate [options] @e input_file

@section ktxtools_validate_description DESCRIPTION
    @b ktx @b validate validates and prints validation information about the KTX2 file provided as argument.

    @note @b ktx @b validate prints using UTF-8 encoding. If your console is not
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

@section ktxtools_validate_exitstatus EXIT STATUS
    @b ktx @b validate exits
        0 on success,
        1 on command line errors and
        2 if the input file parsing failed.

@section ktxtools_validate_history HISTORY

@par Version 4.0
 - Initial version

@section ktxtools_validate_author AUTHOR
    - Mátyás Császár [Vader], RasterGrid www.rastergrid.com
    - Daniel Rákos, RasterGrid www.rastergrid.com
*/
class CommandValidate : public Command {
public:
    void initializeOptions() {
    }

    virtual bool processOption(argparser& parser, int opt) override {
        (void) parser;
        (void) opt;
        return false;
    }

    void processPositional(const std::vector<_tstring>& infiles, const _tstring& outfile) {
        (void) infiles;
        (void) outfile;
    }

    virtual int main(int argc, _TCHAR* argv[]) override {
        std::cout << "Hello, Validate" << std::endl;

        initializeOptions();
        processCommandLine(argc, argv, StdinUse::eAllowStdin, OutfilePos::eNone);
        processPositional(genericOptions.infiles, genericOptions.outfile);

        return EXIT_SUCCESS;
    }

    using Command::Command;
    virtual ~CommandValidate() {};
};

} // namespace ktx

KTX_COMMAND_ENTRY_POINT(ktxValidate, ktx::CommandValidate)

// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#include "command.h"
#include "version.h"
#include <iostream>

#include <fmt/ostream.h>
#include <fmt/printf.h>


// -------------------------------------------------------------------------------------------------

#define QUOTE(x) #x
#define STR(x) QUOTE(x)

namespace ktx {

void Command::parseCommandLine(const std::string& name, const std::string& desc, int argc, _TCHAR* argv[]) {
    commandName = name;
    commandDescription = desc;

    cxxopts::Options commandOpts(name, "");
    commandOpts.custom_help("[OPTION...]");
    commandOpts.set_width(CONSOLE_USAGE_WIDTH);
    initOptions(commandOpts); // virtual customization point

    cxxopts::ParseResult args;
    try {
        args = commandOpts.parse(argc, argv);
        processOptions(commandOpts, args); // virtual customization point

    } catch (const cxxopts::exceptions::parsing& ex) {
        fatal_usage("{}.", ex.what());
    }
}

std::string version(bool testrun) {
    return testrun ? STR(KTX_DEFAULT_VERSION) : STR(KTX_VERSION);
}

// -------------------------------------------------------------------------------------------------

} // namespace ktx

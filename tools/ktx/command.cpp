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
    processName = name;

    cxxopts::Options opts(name, desc);
    opts.custom_help("[OPTION...]");
    opts.set_width(CONSOLE_USAGE_WIDTH);
    initOptions(opts); // virtual customization point

    cxxopts::ParseResult args;
    try {
        args = opts.parse(argc, argv);
    } catch (const std::exception& ex) {
        fmt::print(std::cerr, "Failed to parse command line arguments: {}\n", ex.what());
        fmt::print(std::cerr, "{}", opts.help());
        throw FatalError(RETURN_CODE_INVALID_ARGUMENTS);
    }

    processOptions(opts, args); // virtual customization point
}

std::string version(bool testrun) {
    return testrun ? STR(KTX_DEFAULT_VERSION) : STR(KTX_VERSION);
}

// -------------------------------------------------------------------------------------------------

} // namespace ktx

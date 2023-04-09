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

void Command::initOptions(cxxopts::Options& opts) {
    opts.add_options()
        ("i,input-file", "The input file", cxxopts::value<std::string>(), "filepath")
        ("s,stdin", "Use stdin as the input file")
        ("h,help", "Print this usage message and exit")
        ("v,version", "Print the version number of this program and exit")
        ("testrun", "Indicates test run. If enabled ktx tools will only include the default version information in any output")
    ;
    opts.parse_positional("input-file");
    opts.positional_help("<input-file>");
}

void Command::processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) {
    if (args.count("help")) {
        fmt::print("{}", opts.help());
        throw FatalError(RETURN_CODE_SUCCESS);
    }

    if (args.count("version")) {
        fmt::print("{} version: {}\n", opts.program(), version());
        throw FatalError(RETURN_CODE_SUCCESS);
    }

    if (args.count("stdin") + args.count("input-file") == 0) {
        fmt::print(std::cerr, "Missing input file. Either <input-file> or <stdin> must be specified.\n");
        fmt::print(std::cerr, "{}", opts.help());
        throw FatalError(RETURN_CODE_INVALID_ARGUMENTS);
    }

    if (args.count("stdin") + args.count("input-file") > 1) {
        fmt::print(std::cerr, "Failed to parse command line arguments: Only one can be specified from <input-file> and <stdin>.\n");
        fmt::print(std::cerr, "{}", opts.help());
        throw FatalError(RETURN_CODE_INVALID_ARGUMENTS);
    }

    // TODO Tools P4: Add support for stdin (To support '-' alias argv has to be scanned as cxxopts has no direct support for it)
    if (args.count("stdin"))
        inputFile = "-";
    else
        inputFile = args["input-file"].as<std::string>();

    testrun = args["testrun"].as<bool>();
}

std::string Command::version() const {
    return testrun ? STR(KTX_DEFAULT_VERSION) : STR(KTX_VERSION);
}

// -------------------------------------------------------------------------------------------------

void CommandWithFormat::initOptions(cxxopts::Options& opts) {
    opts.add_options()
        ("f,format", "Specifies the output format. The default format is 'text'.\n"
                     "  text: Human readable text based format\n"
                     "  json: Formatted JSON\n"
                     "  mini-json: Minified JSON",
                     cxxopts::value<std::string>()->default_value("text"),
                     "text|json|mini-json")
    ;
    Command::initOptions(opts);
}

void CommandWithFormat::processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) {
    Command::processOptions(opts, args);

    const auto& formatStr = args["format"].as<std::string>();
    if (formatStr == "text") {
        format = OutputFormat::text;
    } else if (formatStr == "json") {
        format = OutputFormat::json;
    } else if (formatStr == "mini-json") {
        format = OutputFormat::json_mini;
    } else {
        fmt::print(std::cerr, "Failed to parse command line arguments: Unsupported format: \"{}\"\n", formatStr);
        fmt::print(std::cerr, "{}", opts.help());
        throw FatalError(RETURN_CODE_INVALID_ARGUMENTS);
    }
}

} // namespace ktx

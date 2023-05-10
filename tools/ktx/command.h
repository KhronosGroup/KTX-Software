// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#pragma once

#include "stdafx.h"
#include <string>
#include <vector>
#include <iostream>
#include "utility.h"

#include <cxxopts.hpp>
#include <fmt/ostream.h>
#include <fmt/printf.h>


#if defined(_WIN32)
    #define KTX_COMMAND_EXPORT extern "C" __declspec(dllexport)
    #define KTX_COMMAND_CALL __stdcall
    #define KTX_COMMAND_PTR KTX_COMMAND_CALL
#else
    #define KTX_COMMAND_EXPORT extern "C" __attribute__((visibility="default"))
    #define KTX_COMMAND_CALL
    #define KTX_COMMAND_PTR
#endif

#define KTX_COMMAND_ENTRY_POINT_DEF(CMDCLASS) \
    (int argc, _TCHAR* argv[]) { CMDCLASS cmd{}; return cmd.main(argc, argv); }

#if defined(KTX_COMMAND_EXECUTABLE)
    // Command is built as a separate executable
    // (parent command can issue it using a system call)
    #define KTX_COMMAND_ENTRY_POINT(NAME, CMDCLASS) \
        int _tmain KTX_COMMAND_ENTRY_POINT_DEF(CMDCLASS)
#elif defined(KTX_COMMAND_SHARED_LIB)
    // Command is built as a separate shared library
    // (parent command can issue it by loading its entry point)
    #define KTX_COMMAND_ENTRY_POINT(NAME, CMDCLASS) \
        KTX_COMMAND_EXPORT int KTX_COMMAND_CALL ktxCommandMain KTX_COMMAND_ENTRY_POINT_DEF(CMDCLASS)
#else
    // Command is built statically into the executable
    #define KTX_COMMAND_ENTRY_POINT(NAME, CMDCLASS) \
        int NAME KTX_COMMAND_ENTRY_POINT_DEF(CMDCLASS)
#endif

#define KTX_COMMAND_BUILTIN(NAME) int NAME(int argc, _TCHAR* argv[]);

// -------------------------------------------------------------------------------------------------

namespace ktx {

using pfnBuiltinCommand = int (*)(int argc, _TCHAR* argv[]);
using pfnImportedCommand = int (KTX_COMMAND_PTR *)(int argc, _TCHAR* argv[]);

static constexpr int RETURN_CODE_SUCCESS = 0;
static constexpr int RETURN_CODE_INVALID_ARGUMENTS = 1;
static constexpr int RETURN_CODE_IO_FAILURE = 2;
static constexpr int RETURN_CODE_INVALID_FILE = 3;
static constexpr int RETURN_CODE_RUNTIME_ERROR = 4;
static constexpr int RETURN_CODE_KTX_FAILURE = RETURN_CODE_RUNTIME_ERROR;
static constexpr int RETURN_CODE_DFD_FAILURE = RETURN_CODE_RUNTIME_ERROR;
static constexpr int RETURN_CODE_NOT_SUPPORTED = 5;

enum class ReturnCode {
    SUCCESS = RETURN_CODE_SUCCESS,
    INVALID_ARGUMENTS = RETURN_CODE_INVALID_ARGUMENTS,
    IO_FAILURE = RETURN_CODE_IO_FAILURE,
    INVALID_FILE = RETURN_CODE_INVALID_FILE,
    RUNTIME_ERROR = RETURN_CODE_RUNTIME_ERROR,
    KTX_FAILURE = RETURN_CODE_KTX_FAILURE,
    DFD_FAILURE = RETURN_CODE_DFD_FAILURE,
    NOT_SUPPORTED = RETURN_CODE_NOT_SUPPORTED,
};
using rc = ReturnCode;

static constexpr int CONSOLE_USAGE_WIDTH = 100;

struct FatalError : public std::exception {
    int return_code; /// Desired process return code
    explicit FatalError(int returnCode) : return_code(returnCode) {}
    explicit FatalError(ReturnCode returnCode) : FatalError(to_underlying(returnCode)) {}
};

struct Reporter {
    std::string commandName;
    std::string commandDescription;

    template <typename... Args>
    void warning(Args&&... args) {
        fmt::print(std::cerr, "{} warning: ", commandName);
        fmt::print(std::cerr, std::forward<Args>(args)...);
        fmt::print(std::cerr, "\n");
    }

    template <typename... Args>
    void error(Args&&... args) {
        fmt::print(std::cerr, "{} error: ", commandName);
        fmt::print(std::cerr, std::forward<Args>(args)...);
        fmt::print(std::cerr, "\n");
    }

    template <typename... Args>
    void fatal(int return_code, Args&&... args) {
        fmt::print(std::cerr, "{} fatal: ", commandName);
        fmt::print(std::cerr, std::forward<Args>(args)...);
        fmt::print(std::cerr, "\n");
        throw FatalError(return_code);
    }

    template <typename... Args>
    void fatal(ReturnCode return_code, Args&&... args) {
        fatal(to_underlying(return_code), std::forward<Args>(args)...);
    }

    template <typename... Args>
    void fatal_usage(Args&&... args) {
        fmt::print(std::cerr, "{} fatal: ", commandName);
        fmt::print(std::cerr, std::forward<Args>(args)...);
        fmt::print(std::cerr, " See '{} --help'.\n", commandName);
        throw FatalError(rc::INVALID_ARGUMENTS);
    }
};

[[nodiscard]] std::string version(bool testrun);

/**
//! [command options]
<dl>
    <dt>-h, --help</dt>
    <dd>Print this usage message and exit.</dd>
    <dt>-v, --version</dt>
    <dd>Print the version number of this program and exit.</dd>
</dl>
//! [command options]
*/
class Command : public Reporter {
public:
    Command() = default;
    virtual ~Command() = default;

public:
    virtual int main(int argc, _TCHAR* argv[]) = 0;

protected:
    void parseCommandLine(const std::string& name, const std::string& desc, int argc, _TCHAR* argv[]);

    virtual void initOptions(cxxopts::Options& /*opts*/) { }
    virtual void processOptions(cxxopts::Options& /*opts*/, cxxopts::ParseResult& /*args*/) { };
};

// -------------------------------------------------------------------------------------------------

struct OptionsGeneric {
    // --help
    // --version
    bool testrun = false; /// Indicates test run. If enabled ktx tools will only include the default version information in any output

    void init(cxxopts::Options& opts) {
        opts.add_options()
                ("h,help", "Print this usage message and exit")
                ("v,version", "Print the version number of this program and exit")
                ("testrun", "Indicates test run. If enabled ktx tools will only include the default version information in any output");
    }

    void process(cxxopts::Options& opts, cxxopts::ParseResult& args, Reporter& report) {
        testrun = args["testrun"].as<bool>();

        if (args.count("help")) {
            fmt::print("{}: {}\n", report.commandName, report.commandDescription);
            fmt::print("{}", opts.help());
            throw FatalError(rc::SUCCESS);
        }

        if (args.count("version")) {
            fmt::print("{} version: {}\n", opts.program(), version(testrun));
            throw FatalError(rc::SUCCESS);
        }
    }
};

enum class OutputFormat {
    text,
    json,
    json_mini,
};

struct OptionsFormat {
    OutputFormat format;

    void init(cxxopts::Options& opts) {
        opts.add_options()
                ("f,format", "Specifies the output format. The default format is 'text'.\n"
                        "  text: Human readable text based format\n"
                        "  json: Formatted JSON\n"
                        "  mini-json: Minified JSON",
                        cxxopts::value<std::string>()->default_value("text"),
                        "text|json|mini-json");
    }

    void process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter& report) {
        const auto& formatStr = to_lower_copy(args["format"].as<std::string>());
        if (formatStr == "text") {
            format = OutputFormat::text;
        } else if (formatStr == "json") {
            format = OutputFormat::json;
        } else if (formatStr == "mini-json") {
            format = OutputFormat::json_mini;
        } else {
            report.fatal_usage("Unsupported format: \"{}\"", formatStr);
        }
    }
};

struct OptionsSingleIn {
    std::string inputFilepath;

    void init(cxxopts::Options& opts) {
        opts.add_options()
                ("s,stdin", "Use stdin as the input file")
                ("i,input-file", "The input file", cxxopts::value<std::string>(), "filepath");
        opts.parse_positional("input-file");
        opts.positional_help("<input-file>");
    }

    void process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter& report) {
        if (args.count("stdin") + args.count("input-file") == 0)
            report.fatal_usage("Missing input file. Either <input-file> or <stdin> must be specified.");
        if (args.count("stdin") + args.count("input-file") > 1)
            report.fatal_usage("Conflicting options: Only one can be specified from <input-file> and <stdin>.");

        // TODO Tools P4: Add support for stdin (To support '-' alias argv has to be scanned as cxxopts has no direct support for it)
        if (args.count("stdin"))
            inputFilepath = "-";
        else
            inputFilepath = args["input-file"].as<std::string>();
    }
};

struct OptionsSingleInSingleOut {
    std::string inputFilepath;
    std::string outputFilepath;

    void init(cxxopts::Options& opts) {
        opts.add_options()
                ("s,stdin", "Use stdin as the input file")
                ("i,input-file", "The input file", cxxopts::value<std::string>(), "filepath")
                ("o,output-file", "The output file", cxxopts::value<std::string>(), "filepath");
        opts.parse_positional("input-file", "output-file");
        opts.positional_help("<input-file> <output-file>");
    }

    void process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter& report) {
        if (args.count("stdin") + args.count("input-file") == 0)
            report.fatal_usage("Missing input file. Either <input-file> or <stdin> must be specified.");
        if (args.count("stdin") + args.count("input-file") > 1)
            report.fatal_usage("Conflicting options: Only one can be specified from <input-file> and <stdin>.");

        // TODO Tools P4: Add support for stdin (To support '-' alias argv has to be scanned as cxxopts has no direct support for it)
        if (args.count("stdin"))
            inputFilepath = "-";
        else
            inputFilepath = args["input-file"].as<std::string>();

        outputFilepath = args["output-file"].as<std::string>();
    }
};

struct OptionsMultiInSingleOut {
    std::vector<std::string> inputFilepaths;
    std::string outputFilepath;

    void init(cxxopts::Options& opts) {
        opts.add_options()
                ("files", "Input/output files. Last file specified will be used as output", cxxopts::value<std::vector<std::string>>(), "<filepath>");
        opts.parse_positional("files");
        opts.positional_help("<input-file...> <output-file>");
    }

    void process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter& report) {
        std::vector<std::string> files;
        if (args.count("files"))
            files = args["files"].as<std::vector<std::string>>();
        if (files.size() < 1)
            report.fatal_usage("Input and output files must be specified.");
        if (files.size() < 2)
            report.fatal_usage("Output file must be specified.");

        outputFilepath = std::move(files.back());
        files.pop_back();
        inputFilepaths = std::move(files);
    }
};

/// Convenience helper to combine multiple options struct together.
/// Init functions are called left to right and process functions are called in reverse order from right to left.
template <typename... Args>
struct Combine : Args... {
    void init(cxxopts::Options& opts) {
        (Args::init(opts), ...);
    }
    void process(cxxopts::Options& opts, cxxopts::ParseResult& args, Reporter& report) {
        int dummy; // Reverse fold via operator= on a dummy int
        (dummy = ... = (Args::process(opts, args, report), 0));
    }
};

} // namespace ktx

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
    (int argc, char* argv[]) { CMDCLASS cmd{}; return cmd.main(argc, argv); }

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

#define KTX_COMMAND_BUILTIN(NAME) int NAME(int argc, char* argv[]);

// -------------------------------------------------------------------------------------------------

namespace ktx {

using pfnBuiltinCommand = int (*)(int argc, char* argv[]);
using pfnImportedCommand = int (KTX_COMMAND_PTR *)(int argc, char* argv[]);

static constexpr int CONSOLE_USAGE_WIDTH = 100;

/**
//! [command exitstatus]
- 0 - Success
- 1 - Command line error
- 2 - IO failure
- 3 - Invalid input file
- 4 - Runtime or library error
- 5 - Not supported state or operation
- 6 - Requested feature is not yet implemented
//! [command exitstatus]
*/
enum class ReturnCode {
    SUCCESS = 0,
    INVALID_ARGUMENTS = 1,
    IO_FAILURE = 2,
    INVALID_FILE = 3,
    RUNTIME_ERROR = 4,
    KTX_FAILURE = RUNTIME_ERROR,
    DFD_FAILURE = RUNTIME_ERROR,
    NOT_SUPPORTED = 5,
    NOT_IMPLEMENTED = 6,
};
using rc = ReturnCode;

[[nodiscard]] constexpr inline auto operator+(ReturnCode value) noexcept {
	return to_underlying(value);
}

struct FatalError : public std::exception {
    ReturnCode returnCode; /// Desired process exit code
    explicit FatalError(ReturnCode returnCode) : returnCode(returnCode) {}
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
    void fatal(ReturnCode return_code, Args&&... args) {
        fmt::print(std::cerr, "{} fatal: ", commandName);
        fmt::print(std::cerr, std::forward<Args>(args)...);
        fmt::print(std::cerr, "\n");
        throw FatalError(return_code);
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

class Command : public Reporter {
public:
    Command() = default;
    virtual ~Command() = default;

public:
    virtual int main(int argc, char* argv[]) = 0;

protected:
    void parseCommandLine(const std::string& name, const std::string& desc, int argc, char* argv[]);

    virtual void initOptions(cxxopts::Options& /*opts*/) { }
    virtual void processOptions(cxxopts::Options& /*opts*/, cxxopts::ParseResult& /*args*/) { };
#if defined(_WIN32) && defined(DEBUG)
    bool launchDebugger();
#endif
};

// -------------------------------------------------------------------------------------------------

/**
//! [command options_generic]
<dl>
    <dt>-h, \--help</dt>
    <dd>Print this usage message and exit.</dd>
    <dt>-v, \--version</dt>
    <dd>Print the version number of this program and exit.</dd>
</dl>
//! [command options_generic]
*/
struct OptionsGeneric {
    // --help
    // --version
    bool testrun = false; /// Indicates test run. If enabled ktx tools will only include the default version information in any output

    void init(cxxopts::Options& opts) {
        opts.add_options()
                ("h,help", "Print this usage message and exit")
                ("v,version", "Print the version number of this program and exit")
                ("testrun", "Indicates test run. If enabled the tool will produce deterministic output whenever possible")
#if defined(_WIN32) && defined(DEBUG)
                ("ld", "Launch debugger on startup.")
#endif
                ;
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

/**
//! [command options_format]
<dl>
    <dt>\--format text | json | mini-json</dt>
    <dd>Specifies the report output format. Possible options are: <br />
        @b text - Human readable text based format. <br />
        @b json - Formatted JSON. <br />
        @b mini-json - Minified JSON. <br />
        The default format is @b text.
    </dd>
</dl>
//! [command options_format]
*/
struct OptionsFormat {
    OutputFormat format;

    void init(cxxopts::Options& opts) {
        opts.add_options()
                ("f,format", "Specifies the report output format. Possible options are:\n"
                        "  text: Human readable text based format\n"
                        "  json: Formatted JSON\n"
                        "  mini-json: Minified JSON\n",
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
            report.fatal_usage("Unsupported format: \"{}\".", formatStr);
        }
    }
};

struct OptionsSingleIn {
    std::string inputFilepath;

    void init(cxxopts::Options& opts) {
        opts.add_options()
                ("stdin", "Use stdin as the input file. (Using a single dash '-' as the input file has the same effect)")
                ("i,input-file", "The input file. Using a single dash '-' as the input file will use stdin.", cxxopts::value<std::string>(), "filepath");
        opts.parse_positional("input-file");
        opts.positional_help("<input-file>");
    }

    void process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter& report) {
        if (!args.unmatched().empty())
            report.fatal_usage("Too many filenames specified.");

        if (args.count("stdin") + args.count("input-file") == 0)
            report.fatal_usage("Missing input file. Either <input-file> or --stdin must be specified.");
        if (args.count("stdin") + args.count("input-file") > 1)
            report.fatal_usage("Conflicting options: Only one can be specified from <input-file> and --stdin.");

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
                ("stdin", "Use stdin as the input file. (Using a single dash '-' as the input file has the same effect)")
                ("stdout", "Use stdout as the output file. (Using a single dash '-' as the output file has the same effect)")
                ("i,input-file", "The input file. Using a single dash '-' as the input file will use stdin.", cxxopts::value<std::string>(), "filepath")
                ("o,output-file", "The output file. Using a single dash '-' as the output file will use stdout.", cxxopts::value<std::string>(), "filepath");
        opts.parse_positional("input-file", "output-file");
        opts.positional_help("<input-file> <output-file>");
    }

    void process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter& report) {
        if (!args.unmatched().empty())
            report.fatal_usage("Too many filenames specified.");

        if (args.count("stdin") + args.count("input-file") == 0)
            report.fatal_usage("Missing input file. Either <input-file> or --stdin must be specified.");
        if (args.count("stdin") + args.count("input-file") > 1)
            report.fatal_usage("Conflicting options: Only one can be specified from <input-file> and --stdin.");

        if (args.count("stdout") + args.count("output-file") == 0)
            report.fatal_usage("Missing output file. Either <output-file> or --stdout must be specified.");
        if (args.count("stdout") + args.count("output-file") > 1)
            report.fatal_usage("Conflicting options: Only one can be specified from <output-file> and --stdout.");

        if (args.count("stdin"))
            inputFilepath = "-";
        else
            inputFilepath = args["input-file"].as<std::string>();

        if (args.count("stdout"))
            outputFilepath = "-";
        else
            outputFilepath = args["output-file"].as<std::string>();
    }
};

struct OptionsMultiInSingleOut {
    std::vector<std::string> inputFilepaths;
    std::string outputFilepath;

    void init(cxxopts::Options& opts) {
        opts.add_options()
                ("stdin", "Use stdin as the first input file. (Using a single dash '-' as the first input file has the same effect)")
                ("stdout", "Use stdout as the output file. (Using a single dash '-' as the output file has the same effect)")
                ("files", "Input/output files. Last file specified will be used as output."
                          " Using a single dash '-' as an input or output file will use stdin/stdout.", cxxopts::value<std::vector<std::string>>(), "<filepath>");
        opts.parse_positional("files");
        opts.positional_help("<input-file...> <output-file>");
    }

    void process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter& report) {
        std::vector<std::string> files;
        if (args.count("stdin"))
            files.emplace_back("-");
        if (args.count("files")) {
            const auto& argFiles = args["files"].as<std::vector<std::string>>();
            files.insert(files.end(), argFiles.begin(), argFiles.end());
        }
        if (args.count("stdout"))
            files.emplace_back("-");
        if (files.size() < 1)
            report.fatal_usage("Input and output files must be specified.");
        if (files.size() < 2)
            report.fatal_usage("{} file must be specified.", args.count("stdout") == 0 ? "Output" : "Input");

        outputFilepath = std::move(files.back());
        files.pop_back();
        inputFilepaths = std::move(files);

        if (std::count(inputFilepaths.begin(), inputFilepaths.end(), "-") > 1)
            report.fatal_usage("'-' or --stdin as input file was specified more than once.");
    }
};

/// Convenience helper to combine multiple options struct together.
/// Init functions are called left to right.
/// Process functions are called in reverse order from right to left.
template <typename... Args>
struct Combine : Args... {
    void init(cxxopts::Options& opts) {
        (Args::init(opts), ...);
    }
    void process(cxxopts::Options& opts, cxxopts::ParseResult& args, Reporter& report) {
        int dummy; // Reverse fold via operator= on a dummy int
        (dummy = ... = (Args::process(opts, args, report), 0));
        (void) dummy;
    }
};

/// Helper to handle stdin and fstream uniformly
class InputStream {
    std::string filepath;
    std::istream* activeStream = nullptr;
    std::ifstream file; // Unused for stdin/stdout
    std::stringstream stdinBuffer;

public:
    InputStream(const std::string& filepath, Reporter& report);

    /*explicit(false)*/ operator std::istream&() {
        return *activeStream;
    }

    std::istream* operator->() {
        return activeStream;
    }
    std::istream& operator*() {
        return *activeStream;
    }
};

/// Helper to handle stdout and fstream uniformly
class OutputStream {
    std::string filepath;
    FILE* file;
    // std::ostream* activeStream = nullptr;
    // std::ofstream file; // Unused for stdin/stdout

public:
    OutputStream(const std::string& filepath, Reporter& report);
    ~OutputStream();
    void writeKTX2(ktxTexture* texture, Reporter& report);
    void write(const char* data, std::size_t size, Reporter& report);
};

} // namespace ktx

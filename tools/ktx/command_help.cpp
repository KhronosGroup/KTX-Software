// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include "command.h"
#include <filesystem>
#include <optional>

#include <cxxopts.hpp>
#include <fmt/format.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h> // For GetModuleFileNameW
#include <shellapi.h> // For ShellExecuteW
#include <pathcch.h> // For PathCchRemoveFileSpec
#include <fmt/xchar.h> // For wchat_t format
#endif
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

// -------------------------------------------------------------------------------------------------

namespace ktx {

struct OptionsHelp {
    std::optional<std::string> command;

    void init(cxxopts::Options& opts) {
        opts.add_options()
                ("command", "The command for which usage should be displayed.", cxxopts::value<std::string>());
        opts.parse_positional("command");
        opts.positional_help("<command>");
    }

    void process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter& report) {
        if (args.count("command")) {
            static const std::unordered_set<std::string> command_table{
                "create",
                "extract",
                "encode",
                "transcode",
                "info",
                "validate",
                "help",
            };

            command = to_lower_copy(args["command"].as<std::string>());
            if (command_table.count(*command) == 0)
                report.fatal_usage("Invalid command specified: \"{}\".", *command);
        }
    }
};

/** @page ktx_help ktx help
@~English

Display help information about the ktx tool.

@section ktx_help_synopsis SYNOPSIS
    ktx help [option...] @e [command]

@section ktx_help_description DESCRIPTION
    @b ktx @b help displays the man page of a specific ktx command specified as the @e command
    argument.
    On windows systems the man pages are opened with the system default browser in html format.
    On systems derived from Unix the man pages are opened with the man command.

    To support custom install locations the tool first tries to use the man files relative to
    the executable and falls back to the system man pages.

    The following options are available:
    <dl>
        <dt>command</dt>
        <dd>Specifies which command's man page will be displayed. If the command option is missing
        the main ktx tool man page will be displayed. Possible options are: <br />
            @ref ktx_create "create" <br />
            @ref ktx_extract "extract" <br />
            @ref ktx_encode "encode" <br />
            @ref ktx_transcode "transcode" <br />
            @ref ktx_info "info" <br />
            @ref ktx_validate "validate" <br />
            @ref ktx_help "help"
        </dd>
    </dl>
    @snippet{doc} ktx/command.h command options_generic

@section ktx_help_exitstatus EXIT STATUS
    @snippet{doc} ktx/command.h command exitstatus

@section ktx_help_history HISTORY

@par Version 4.0
 - Initial version

@section ktx_help_author AUTHOR
    - Mátyás Császár [Vader], RasterGrid www.rastergrid.com
    - Daniel Rákos, RasterGrid www.rastergrid.com
*/
class CommandHelp : public Command {
    Combine<OptionsHelp, OptionsGeneric> options;

public:
    virtual int main(int argc, char* argv[]) override;
    virtual void initOptions(cxxopts::Options& opts) override;
    virtual void processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) override;

private:
    void executeHelp();
};

// -------------------------------------------------------------------------------------------------

int CommandHelp::main(int argc, char* argv[]) {
    try {
        parseCommandLine("ktx help",
                "Displays the man page of a specific ktx command specified as the command argument."
                "\nIf the command option is missing the main ktx tool man page will be displayed.",
                argc, argv);
        executeHelp();
        return +rc::SUCCESS;
    } catch (const FatalError& error) {
        return +error.returnCode;
    } catch (const std::exception& e) {
        fmt::print(std::cerr, "{} fatal: {}\n", commandName, e.what());
        return +rc::RUNTIME_ERROR;
    }
}

void CommandHelp::initOptions(cxxopts::Options& opts) {
    options.init(opts);
}

void CommandHelp::processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) {
    options.process(opts, args, *this);
}

void CommandHelp::executeHelp() {
    // On windows open the html pages with the browser
    // On linux/macos open the man pages with man

#if defined(_WIN32)
    DWORD size = 256;
    std::wstring executablePath;
    do {
        size *= 2;
        executablePath.resize(size);
    } while (GetModuleFileNameW(nullptr, executablePath.data(), size) == size);

    PathCchRemoveFileSpec(executablePath.data(), executablePath.size());
    executablePath.resize(wcslen(executablePath.c_str()));

    const auto commandStr = options.command.value_or("");
    const auto systemCommand = fmt::format(L"{}\\..\\share\\doc\\KTX-Software\\html\\ktxtools\\ktx{}{}.html",
            executablePath,
            options.command ? L"_" : L"",
            std::wstring(commandStr.begin(), commandStr.end()));

    auto result = ShellExecuteW(nullptr, nullptr, systemCommand.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    auto r = reinterpret_cast<INT_PTR>(result);
    if (r <= 32) // WinAPI is weird
        fatal(rc::RUNTIME_ERROR, "Failed to open the html documentation: ERROR {}", r);

#else
#   if defined(__APPLE__)
    char buf[PATH_MAX];
    uint32_t bufsize = PATH_MAX;
    if (const auto ec = _NSGetExecutablePath(buf, &bufsize))
        fatal(rc::RUNTIME_ERROR, "Failed to determine executable path: ERROR {}", ec);
    const auto executablePath = std::filesystem::canonical(buf);
#   else // Linux
    const auto executablePath = std::filesystem::canonical("/proc/self/exe");
#   endif

    const auto executableDir = std::filesystem::path(executablePath).remove_filename();
    const auto manFile = fmt::format("{}/../share/man/man1/ktx{}{}.1",
                executableDir.string(),
                options.command ? "_" : "",
                options.command.value_or(""));
    if (std::filesystem::exists(manFile)) {
        // We have relative access to the man file, prioritze opening it
        // that way to support custom install locations
        const auto systemCommand = fmt::format("man \"{}\"", manFile);
        const auto result = std::system(systemCommand.c_str());
        (void) result;
    } else {
        const auto systemCommand = fmt::format("man ktx{}{}",
                options.command ? "_" : "",
                options.command.value_or(""));
        const auto result = std::system(systemCommand.c_str());
        (void) result;
    }
#endif
}

// -------------------------------------------------------------------------------------------------

} // namespace ktx

KTX_COMMAND_ENTRY_POINT(ktxHelp, ktx::CommandHelp)

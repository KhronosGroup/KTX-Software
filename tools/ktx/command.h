// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#pragma once

#include "stdafx.h"
#include <string>
#include <vector>

#include <cxxopts.hpp>


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

static constexpr int CONSOLE_USAGE_WIDTH = 100;

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
class Command {
protected:
    std::string processName;
    std::string inputFile;
    bool testrun = false; /// Indicates test run. If enabled ktx tools will only include the default version information in any output

public:
    Command() = default;
    virtual ~Command() = default;

public:
    virtual int main(int argc, _TCHAR* argv[]) = 0;

protected:
    void parseCommandLine(const std::string& name, const std::string& desc, int argc, _TCHAR* argv[]);

    virtual void initOptions(cxxopts::Options& options);
    virtual void processOptions(cxxopts::Options& options, cxxopts::ParseResult& args);

    [[nodiscard]] std::string version() const;

protected:
    struct FatalError : public std::exception {
        int return_code; /// Desired process return code
        explicit FatalError(int returnCode) : return_code(returnCode) {}
    };
};

class CommandWithFormat : public Command {
protected:
    enum class OutputFormat {
        text,
        json,
        json_mini,
    };
    OutputFormat format;

protected:
    virtual void initOptions(cxxopts::Options& options) override;
    virtual void processOptions(cxxopts::Options& options, cxxopts::ParseResult& args) override;
};

} // namespace ktx

// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#pragma once

#include "argparser.h"

#include <iostream>
#include <vector>


#if defined(_WIN32)
    #include <tchar.h>
    #define KTX_COMMAND_EXPORT extern "C" __declspec(dllexport)
    #define KTX_COMMAND_CALL __stdcall
    #define KTX_COMMAND_PTR KTX_COMMAND_CALL
#else
    #define _TCHAR char
    #define _T(x) x
    #define KTX_COMMAND_EXPORT extern "C" __attribute__((visibility="default"))
    #define KTX_COMMAND_CALL
    #define KTX_COMMAND_PTR
#endif
#if defined(_UNICODE)
    #define _tstring std::wstring
#else
    #define _tstring std::string
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

typedef int (*pfnBuiltinCommand)(int argc, _TCHAR* argv[]);
typedef int (KTX_COMMAND_PTR *pfnImportedCommand)(int argc, _TCHAR* argv[]);

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
    _tstring processName;

    struct CommandOptions {
        std::vector<_tstring> infiles;
        _tstring outfile;
        int test = false;
    };

    CommandOptions genericOptions;

    _tstring short_opts = _T("hv");

    std::vector<argparser::option> option_list{
            {"help", argparser::option::no_argument, nullptr, 'h'},
            {"version", argparser::option::no_argument, nullptr, 'v'},
            {"test", argparser::option::no_argument, &genericOptions.test, 1},
            // -NSDocumentRevisionsDebugMode YES is appended to the end
            // of the command by Xcode when debugging and "Allow debugging when
            // using document Versions Browser" is checked in the scheme. It
            // defaults to checked and is saved in a user-specific file not the
            // pbxproj file so it can't be disabled in a generated project.
            // Remove these from the arguments under consideration.
            {"-NSDocumentRevisionsDebugMode", argparser::option::required_argument, nullptr, 10000},
            {nullptr, argparser::option::no_argument, nullptr, 0}
    };

public:
    Command();
    virtual ~Command() = default;

public:
    virtual int main(int argc, _TCHAR* argv[]) = 0;
    virtual void usage() {
        std::cerr <<
                "  -h, --help    Print this usage message and exit.\n"
                "  -v, --version Print the version number of this program and exit.\n";
    };

protected:
    void error(const char* pFmt, ...);

    enum class StdinUse { eDisallowStdin, eAllowStdin };
    enum class OutfilePos { eNone, eFirst, eLast };

    void processCommandLine(int argc, _TCHAR* argv[],
            StdinUse stdinStat = StdinUse::eAllowStdin,
            OutfilePos outfilePos = OutfilePos::eNone);
    bool loadFileList(const _tstring& f, bool relativize, std::vector<_tstring>& filenames);

    void processOptions(argparser& parser);
    virtual bool processOption(argparser& parser, int opt) = 0;

    void writeId(std::ostream& dst, bool chktest);
    void printVersion();
};

} // namespace ktx

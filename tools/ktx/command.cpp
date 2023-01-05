// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0


#include "command.h"
#include "version.h"

#include "stdafx.h"
#include <stdarg.h>
#include <iostream>


// -------------------------------------------------------------------------------------------------

namespace ktx {

#define QUOTE(x) #x
#define STR(x) QUOTE(x)

_tstring version(STR(KTX_VERSION));
_tstring defaultVersion(STR(KTX_DEFAULT_VERSION));

// -------------------------------------------------------------------------------------------------

Command::Command() {}

void Command::error(const char* pFmt, ...) {
    va_list args;
    va_start(args, pFmt);

    std::cerr << processName << ": ";
    vfprintf(stderr, pFmt, args);
    va_end(args);
    std::cerr << "\n";
}

void Command::processCommandLine(int argc, _TCHAR* argv[], StdinUse stdinStat, OutfilePos outfilePos) {
    uint32_t i;
    size_t slash, dot;

    processName = argv[0];
    // For consistent Id, only use the stem of name;
    slash = processName.find_last_of(_T('\\'));
    if (slash == _tstring::npos)
        slash = processName.find_last_of(_T('/'));
    if (slash != _tstring::npos)
        processName.erase(0, slash + 1);  // Remove directory name.
    dot = processName.find_last_of(_T('.'));
    if (dot != _tstring::npos)
        processName.erase(dot, _tstring::npos); // Remove extension.

    argparser parser(argc, argv);
    processOptions(parser);

    i = parser.optind;
    if (argc - i > 0) {
        if (outfilePos == OutfilePos::eFirst)
            genericOptions.outfile = parser.argv[i++];
        uint32_t infileCount = outfilePos == OutfilePos::eLast ? argc - 1 : argc;
        for (; i < infileCount; i++) {
            if (parser.argv[i][0] == _T('@')) {
                if (!loadFileList(parser.argv[i],
                        parser.argv[i][1] == _T('@'),
                        genericOptions.infiles)) {
                    std::exit(1);
                }
            } else {
                genericOptions.infiles.push_back(parser.argv[i]);
            }
        }
        if (genericOptions.infiles.size() > 1) {
            std::vector<_tstring>::const_iterator it;
            for (it = genericOptions.infiles.begin(); it < genericOptions.infiles.end(); it++) {
                if (it->compare(_T("-")) == 0) {
                    error("cannot use stdin as one among many inputs.");
                    usage();
                    std::exit(1);
                }
            }
        }
        if (outfilePos == OutfilePos::eLast)
            genericOptions.outfile = parser.argv[i];
    }

    if (genericOptions.infiles.empty()) {
        if (stdinStat == StdinUse::eAllowStdin) {
            genericOptions.infiles.emplace_back(_T("-")); // Use stdin as 0 files.
        } else {
            error("need some input files.");
            usage();
            std::exit(1);
        }
    }
    if (outfilePos != OutfilePos::eNone && genericOptions.outfile.empty()) {
        error("need an output file");
    }
}

bool Command::loadFileList(const _tstring& f, bool relativize, std::vector<_tstring>& filenames) {
    _tstring listName(f);
    listName.erase(0, relativize ? 2 : 1);

    FILE* lf = nullptr;
#ifdef _WIN32
    _tfopen_s(&lf, listName.c_str(), "r");
#else
    lf = _tfopen(listName.c_str(), "r");
#endif

    if (!lf) {
        error("failed opening filename list: \"%s\": %s\n",
                listName.c_str(), strerror(errno));
        return false;
    }

    uint32_t totalFilenames = 0;
    _tstring dirname;

    if (relativize) {
        size_t dirnameEnd = listName.find_last_of('/');
        if (dirnameEnd == std::string::npos) {
            relativize = false;
        } else {
            dirname = listName.substr(0, dirnameEnd + 1);
        }
    }

    for (;;) {
        // Cross platform PATH_MAX def is too much trouble!
        char buf[4096];
        buf[0] = '\0';

        char* p = fgets(buf, sizeof(buf), lf);
        if (!p) {
            if (ferror(lf)) {
                error("failed reading filename list: \"%s\": %s\n",
                        listName.c_str(), strerror(errno));
                fclose(lf);
                return false;
            } else
                break;
        }

        std::string readFilename(p);
        while (readFilename.size()) {
            if (readFilename[0] == _T(' '))
                readFilename.erase(0, 1);
            else
                break;
        }

        while (readFilename.size()) {
            const char c = readFilename.back();
            if ((c == _T(' ')) || (c == _T('\n')) || (c == _T('\r')))
                readFilename.erase(readFilename.size() - 1, 1);
            else
                break;
        }

        if (readFilename.size()) {
            if (relativize)
                filenames.push_back(dirname + readFilename);
            else
                filenames.push_back(readFilename);
            totalFilenames++;
        }
    }

    fclose(lf);

    return true;
}

void Command::processOptions(argparser& parser) {
    int opt;
    while ((opt = parser.getopt(&short_opts, option_list.data(), NULL)) != -1) {
        switch (opt) {
        case 0:
            break;
        case 10000:
            break;
        case 'h':
            usage();
            exit(0);
        case 'v':
            printVersion();
            exit(0);
        case ':':
            error("missing required option argument.");
            usage();
            exit(0);
        default:
            if (!processOption(parser, opt)) {
                usage();
                exit(1);
            }
        }
    }
}

void Command::writeId(std::ostream& dst, bool chktest) {
    dst << processName << " ";
    dst << (!chktest || !genericOptions.test ? version : defaultVersion);
}

void Command::printVersion() {
    writeId(std::cerr, false);
    std::cerr << std::endl;
}

} // namespace ktx

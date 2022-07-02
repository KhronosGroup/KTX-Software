// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 sts=4 expandtab:

// Copyright 2019-2020 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

#include "stdafx.h"

#include <stdarg.h>
#if (_MSVC_LANG >= 201703L || __cplusplus >= 201703L)
#include <algorithm>
#endif

#include <iostream>
#include <vector>
#include <ktx.h>

#include "argparser.h"

#define QUOTE(x) #x
#define STR(x) QUOTE(x)

// Thanks Windows!!!
#if defined(min)
  #undef min
#endif
#if defined(max)
  #undef max
#endif

using namespace std;

// clamp is in std:: from c++17.
#if !(_MSVC_LANG >= 201703L || __cplusplus >= 201703L)
template <typename T> inline T clamp(T value, T low, T high) {
    return (value < low) ? low : ((value > high) ? high : value);
}
#endif

template<typename T>
struct clamped
{
  clamped(T def_v, T min_v, T max_v) :
    def(def_v),
    min(min_v),
    max(max_v),
    value(def_v)
  {
  }

  void clear()
  {
    value = def;
  }

  operator T() const
  {
    return value;
  }

  T operator= (T v)
  {
    value = clamp<T>(v, min, max);
    return value;
  }

  T def;
  T min;
  T max;
  T value;
};

/**
//! [ktxApp options]
  <dl>
  <dt>-h, --help</dt>
  <dd>Print this usage message and exit.</dd>
  <dt>-v, --version</dt>
  <dd>Print the version number of this program and exit.</dd>
  </dl>

//! [ktxApp options]
*/

class ktxApp {
  public:
    virtual int main(int argc, _TCHAR* argv[]) = 0;
    virtual void usage() {
        cerr <<
          "  -h, --help    Print this usage message and exit.\n"
          "  -v, --version Print the version number of this program and exit.\n";
    };

  protected:
    struct commandOptions {
        std::vector<_tstring> infiles;
        _tstring outfile;
        int test;

        commandOptions() : test(false) { }
    };

    ktxApp(std::string& version, std::string& defaultVersion,
           commandOptions& options)
        : version(version), defaultVersion(defaultVersion),
          options(options) { }

    void error(const char *pFmt, ...) {
        va_list args;
        va_start(args, pFmt);

        cerr << name << ": ";
        vfprintf(stderr, pFmt, args);
        va_end(args);
        cerr << "\n";
    }

    /** @internal
     * @~English
     * @brief Open a file for writing, failing if it exists.
     *
     * Assumes binary mode is wanted.
     *
     * Works around an annoying limitation of the VS2013-era msvcrt's
     * @c fopen that implements an early version of the @c fopen spec.
     * that does not accept 'x' as a mode character. For some reason
     * Mingw uses this ancient version. Rather than use ifdef heuristics
     * to identify sufferers of the limitation, it handles the error case
     * and uses an alternate way to check for file existence.
     *
     * @return A stdio FILE* for the created file. If the file already exists
     *         returns nullptr and sets errno to EEXIST.
     */
    static FILE* fopen_write_if_not_exists(const _tstring& path) {
        FILE* file = ::_tfopen(path.c_str(), "wxb");
        if (!file && errno == EINVAL) {
            file = ::_tfopen(path.c_str(), "r");
            if (file) {
                fclose(file);
                file = nullptr;
                errno = EEXIST;
            } else {
                file = ::_tfopen(path.c_str(), "wb");
            }
        }
        return file;
    }

    int strtoi(const char* str)
    {
        char* endptr;
        int value = (int)strtol(str, &endptr, 0);
        // Some implementations set errno == EINVAL but we can't rely on it.
        if (value == 0 && endptr && *endptr != '\0') {
            cerr << "Argument \"" << endptr << "\" not a number." << endl;
            usage();
            exit(1);
        }
        return value;
    }

    enum StdinUse { eDisallowStdin, eAllowStdin };
    enum OutfilePos { eNone, eFirst, eLast };
    void processCommandLine(int argc, _TCHAR* argv[],
                            StdinUse stdinStat = eAllowStdin,
                            OutfilePos outfilePos = eNone)
    {
        uint32_t i;
        size_t slash, dot;

        name = argv[0];
        // For consistent Id, only use the stem of name;
        slash = name.find_last_of(_T('\\'));
        if (slash == _tstring::npos)
            slash = name.find_last_of(_T('/'));
        if (slash != _tstring::npos)
            name.erase(0, slash+1);  // Remove directory name.
        dot = name.find_last_of(_T('.'));
            if (dot != _tstring::npos)
                name.erase(dot, _tstring::npos); // Remove extension.

        argparser parser(argc, argv);
        processOptions(parser);

        i = parser.optind;
        if (argc - i > 0) {
            if (outfilePos == eFirst)
                options.outfile = parser.argv[i++];
            uint32_t infileCount = outfilePos == eLast ? argc - 1 : argc;
            for (; i < infileCount; i++) {
                if (parser.argv[i][0] == _T('@')) {
                    if (!loadFileList(parser.argv[i],
                                      parser.argv[i][1] == _T('@'),
                                      options.infiles)) {
                        exit(1);
                    }
                } else {
                    options.infiles.push_back(parser.argv[i]);
                }
            }
            if (options.infiles.size() > 1) {
                std::vector<_tstring>::const_iterator it;
                for (it = options.infiles.begin(); it < options.infiles.end(); it++) {
                    if (it->compare(_T("-")) == 0) {
                        error("cannot use stdin as one among many inputs.");
                        usage();
                        exit(1);
                    }
                }
            }
            if (outfilePos == eLast)
                options.outfile = parser.argv[i];
        }

        if (options.infiles.size() == 0) {
            if (stdinStat == eAllowStdin) {
                options.infiles.push_back(_T("-")); // Use stdin as 0 files.
            } else {
                error("need some input files.");
                usage();
                exit(1);
            }
        }
        if (outfilePos != eNone && options.outfile.empty()) {
            error("need an output file");
        }
    }

    bool loadFileList(const _tstring &f, bool relativize,
                      vector<_tstring>& filenames)
    {
        _tstring listName(f);
        listName.erase(0, relativize ? 2 : 1);

        FILE *lf = nullptr;
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
            if (dirnameEnd == string::npos) {
                relativize = false;
            } else {
                dirname = listName.substr(0, dirnameEnd + 1);
            }
        }

        for (;;) {
            // Cross platform PATH_MAX def is too much trouble!
            char buf[4096];
            buf[0] = '\0';

            char *p = fgets(buf, sizeof(buf), lf);
            if (!p) {
              if (ferror(lf)) {
                error("failed reading filename list: \"%s\": %s\n",
                      listName.c_str(), strerror(errno));
                fclose(lf);
                return false;
              } else
                break;
            }

            string readFilename(p);
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

    virtual void processOptions(argparser& parser) {
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

    virtual bool processOption(argparser& parser, int opt) = 0;

    void writeId(std::ostream& dst, bool chktest) {
        dst << name << " ";
        dst << (!chktest || !options.test ? version : defaultVersion);
    }

    void printVersion() {
        writeId(cerr, false);
        cerr << endl;
    }

    _tstring        name;
    _tstring&       version;
    _tstring&       defaultVersion;

    commandOptions& options;

    virtual void validateOptions() { }

    std::vector<argparser::option> option_list {
        { "help", argparser::option::no_argument, NULL, 'h' },
        { "version", argparser::option::no_argument, NULL, 'v' },
        { "test", argparser::option::no_argument, &options.test, 1},
        // -NSDocumentRevisionsDebugMode YES is appended to the end
        // of the command by Xcode when debugging and "Allow debugging when
        // using document Versions Browser" is checked in the scheme. It
        // defaults to checked and is saved in a user-specific file not the
        // pbxproj file so it can't be disabled in a generated project.
        // Remove these from the arguments under consideration.
        { "-NSDocumentRevisionsDebugMode", argparser::option::required_argument, NULL, 10000 },
        { nullptr, argparser::option::no_argument, nullptr, 0 }
    };

    _tstring short_opts = _T("hv");
};



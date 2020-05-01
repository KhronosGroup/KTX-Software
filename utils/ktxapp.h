// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 sts=4 expandtab:

//
// ©2019 The Khronos Group, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <vector>
#include <stdarg.h>
#include <ktx.h>

#include "argparser.h"

#define QUOTE(x) #x
#define STR(x) QUOTE(x)

using namespace std;

template <typename T> inline T clamp(T value, T low, T high) {
    return (value < low) ? low : ((value > high) ? high : value);
}

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
  <dt>--help</dt>
  <dd>Print this usage message and exit.</dd>
  <dt>--version</dt>
  <dd>Print the version number of this program and exit.</dd>
  </dl>

//! [ktxApp options]
*/

class ktxApp {
  public:
    virtual int main(int argc, _TCHAR* argv[]) = 0;
    virtual void usage() {
        cerr <<
          "  --help       Print this usage message and exit.\n"
          "  --version    Print the version number of this program and exit.\n";
    };

  protected:
    struct commandOptions {
        std::vector<_tstring> infiles;
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
    void processCommandLine(int argc, _TCHAR* argv[], StdinUse stdinStat) {
        int i;
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
            for (; i < argc; i++) {
                options.infiles.push_back(parser.argv[i]);
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
        } else if (stdinStat == eAllowStdin) {
            options.infiles.push_back(_T("-")); // Use stdin as 0 files.
        } else {
            error("no input files.");
            usage();
            exit(1);
        }
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



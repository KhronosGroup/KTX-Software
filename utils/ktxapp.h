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

#include "argparser.h"

using namespace std;

template <typename T> inline T clamp(T value, T low, T high) {
    return (value < low) ? low : ((value > high) ? high : value);
}

template<typename T>
struct clampedOption
{
  clampedOption(T& val, T min_v, T max_v) :
    value(val),
    min(min_v),
    max(max_v)
  {
  }

  void clear()
  {
    value = 0;
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

  T& value;
  T min;
  T max;
};

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
    };

    ktxApp(std::string& version, commandOptions& options)
              : version(version), options(options) {
        // Don't use in-class initializers so we can build on VS2013. Sigh!
        argparser::option init_option_list[] = {
            { "help", argparser::option::no_argument, NULL, 'h' },
            { "version", argparser::option::no_argument, NULL, 'v' },
            // -NSDocumentRevisionsDebugMode YES is appended to the end
            // of the command by Xcode when debugging and "Allow debugging when
            // using document Versions Browser" is checked in the scheme. It
            // defaults to checked and is saved in a user-specific file not the
            // pbxproj file so it can't be disabled in a generated project.
            // Remove these from the arguments under consideration.
            { "-NSDocumentRevisionsDebugMode", argparser::option::required_argument, NULL, 'i' },
            { nullptr, argparser::option::no_argument, nullptr, 0 }
        };
        const int lastOptionIndex = sizeof(init_option_list)
                                    / sizeof(argparser::option);
        option_list.insert(option_list.begin(), init_option_list,
                           init_option_list + lastOptionIndex);

        short_opts = _T("hv");
    }

    void error(const char *pFmt, ...) {
        va_list args;
        va_start(args, pFmt);

        cerr << name << ": ";
        vfprintf(stderr, pFmt, args);
        va_end(args);
        cerr << "\n";
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
        _TCHAR opt;
        while ((opt = parser.getopt(&short_opts, option_list.data(), NULL)) != -1) {
            switch (opt) {
              case 0:
                break;
              case 'h':
                usage();
                exit(0);
              case 'v':
                printVersion();
                exit(0);
              default:
                processOption(parser, opt);
            }
        }
    }

    virtual void processOption(argparser& parser, _TCHAR opt) = 0;

    void writeId(std::ostream& dst) {
        dst << name << " version " << version;
    }

    void printVersion() {
        writeId(cerr);
        cerr << std::endl;
    }

    _tstring        name;
    _tstring&       version;

    commandOptions& options;

    std::vector<argparser::option> option_list;

    _tstring short_opts;
};

#if 0
struct commandOptions {
    struct basisOptions : public ktxBasisParams {
        // The remaining numeric fields are clamped within the Basis library.
        clampedOption<ktx_uint32_t> threadCount;
        clampedOption<ktx_uint32_t> qualityLevel;
        clampedOption<ktx_uint32_t> maxEndpoints;
        clampedOption<ktx_uint32_t> maxSelectors;
        int noMultithreading;

        basisOptions() :
            threadCount(ktxBasisParams::threadCount, 1, 10000),
            qualityLevel(ktxBasisParams::qualityLevel, 1, 255),
            maxEndpoints(ktxBasisParams::maxEndpoints, 1, 16128),
            maxSelectors(ktxBasisParams::maxSelectors, 1, 16128),
            noMultithreading(0)
        {
            uint32_t tc = std::thread::hardware_concurrency();
            if (tc == 0) tc = 1;
            threadCount.max = tc;
            threadCount = tc;

            structSize = sizeof(ktxBasisParams);
            compressionLevel = 0;
            maxEndpoints.clear();
            endpointRDOThreshold = 0.0f;
            maxSelectors.clear();
            selectorRDOThreshold = 0.0f;
            normalMap = false;
            separateRGToRGB_A = false;
            preSwizzle = false;
            noEndpointRDO = false;
            noSelectorRDO = false;
        }
    };
    
    _tstring            appName;
    _tstring            outfile;
    bool                useStdout;
    bool                force;
    struct basisOptions bopts;
    std::vector<_tstring> infilenames;

    commandOptions() {
        force = false;
        useStdout = false;
    }
};
#endif


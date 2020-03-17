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

#include "stdafx.h"
#include <cstdlib>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

#include <KHR/khr_df.h>

#include "ktx.h"
#include "argparser.h"
#include "version.h"

#if defined(_MSC_VER)
  #define strncasecmp _strnicmp
  #define fileno _fileno
  #define mktemp _mkstemp
  #define isatty _isatty
  #define unlink _unlink
#endif

#if defined(_MSC_VER)
  #undef min
  #undef max
#endif

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
    int                 test;
    struct basisOptions bopts;
    std::vector<_tstring> infilenames;

    commandOptions() {
        force = false;
        test = false;
        useStdout = false;
    }
};

static _tstring appName;

static void processCommandLine(int argc, _TCHAR* argv[],
                               struct commandOptions& options);
static void processOptions(argparser& parser, struct commandOptions& options);
#if IMAGE_DEBUG
static void dumpImage(_TCHAR* name, int width, int height, int components,
                      int componentSize, bool isLuminance,
                      unsigned char* srcImage);
#endif

using namespace std;

/** @page ktxsc ktxsc
@~English

Supercompress the images in a KTX2 file.

@section ktxsc_synopsis SYNOPSIS
    ktxsc [options] [@e infile ...]

@section ktxsc_description DESCRIPTION
    @b ktxsc supercompresses the images in Khronos texture format version 2
    files (KTX2) that have uncompressed images, i.e those whose vkFormat name
    does not end in @c _BLOCK. It first compresses to ETC1S format then
    supercompresses with Basis Universal.

    RED images will become RGB with RED in each component. RG images will
    have R in each component of the RGB part and G in the alpha part of the
    compressed texture.

    @b ktxsc reads each named @e infile and compresses it in place. When @e
    infile is not specified, a single file will be read from @e stdin. and the
    output written to @e stdout. When one or more files is specified each will
    be comoressed in place.

    The following options are available:
    <dl>
    <dt>-o outfile, --output=outfile</dt>
    <dd>Write the output to @e outfile. If @e outfile is 'stdout', output will
        be written to stdout. If there is more than 1 @e infile the command
        prints its usage message and exits.</dd>
    <dt>-f, --force</dt>
    <dd>If the destination file cannot be opened, remove it and create a
        new file, without prompting for confirmation regardless of its
        permissions.</dd>
    <dt>--bcmp</dt>
    <dt>--no_multithreading</dt>
    <dd>Disable multithreading. By default Basis compression will use the
        numnber of threads reported by @c std::thread::hardware_concurrency or
        1 if value returned is 0.</dd>
    <dt>--threads &lt;count&gt;</dt>
    <dd>Explicitly set the number of threads to use during compression.
        @b --no_multithreading must not be set.</dd>
    <dt>--clevel &lt;level&gt;</dt>
    <dd>Basis compression level, an encoding speed vs. quality tradeoff. Range
        is 0 - 5, default is 1. Higher values are slower, but give higher
        quality.</dd>
    <dt>--qlevel &lt;level&gt;</dt>
    <dd>Basis quality level. Range is 1 - 255.  Lower gives better
        compression/lower quality/faster. Higher gives less compression
        /higher quality/slower. Values of @b --max_endpoints and
        @b --max-selectors computed from this override any explicitly set
        values. Default is 128, if either of @b --max_endpoints or
        @b --max_selectors is unset, otherwise those settings rule.</dd>
    <dt>--max_endpoints &lt;arg&gt;</dt>
    <dd>Manually set the maximum number of color endpoint clusters
        from 1-16128. Default is 0, unset.</dd>
    <dt>--endpoint_rdo_threshold &lt;arg&gt;</dt>
    <dd>Set endpoint RDO quality threshold. The default is 1.25. Lower is
        higher quality but less quality per output bit (try 1.0-3.0).
        This will override the value chosen by @c --qual.</dd>
    <dt>--max_selectors &lt;arg&gt;</dt>
    <dd>Manually set the maximum number of color selector clusters
        from 1-16128. Default is 0, unset.</dd>
    <dt>--selector_rdo_threshold &lt;arg&gt;</dt>
    <dd>Set selector RDO quality threshold. The default is 1.5. Lower is
        higher quality but less quality per output bit (try 1.0-3.0).
        This will override the value chosen by @c --qual.</dd>
    <dt>--normal_map</dt>
    <dd>Tunes codec parameters for better quality on normal maps (no
        selector RDO, no endpoint RDO). Only valid for linear textures.</dd>
    <dt>--separate_rg_to_color_alpha</dt>
    <dd>Separates the input R and G channels to RGB and A (for tangent
        space XY normal maps). Automatically selected if the input images
        are 2-component.</dd>
    <dt>--no_endpoint_rdo</dt>
    <dd>Disable endpoint rate distortion optimizations. Slightly faster,
        less noisy output, but lower quality per output bit. Default is
        to do endpoint RDO.</dd>
    <dt>--no_selector_rdo</dt>
    <dd>Disable selector rate distortion optimizations. Slightly faster,
        less noisy output, but lower quality per output bit. Default is
        to do selector RDO.</dd>
    </dd>
    <dt>--help</dt>
    <dd>Print this usage message and exit.</dd>
    <dt>--version</dt>
    <dd>Print the version number of this program and exit.</dd>
    </dl>

@section ktxsc_exitstatus EXIT STATUS
    @b toktx exits 0 on success, 1 on command line errors and 2 on
    functional errors.

@section ktxsc_history HISTORY

@par Version 4.0
 - Initial version.

@section ktxsc_author AUTHOR
    Mark Callow, Edgewise Consulting www.edgewise-consulting.com
*/

static void
usage(_tstring& appName)
{
    fprintf(stderr,
        "Usage: %s [options] [<infile> ...]\n"
        "\n"
        "  infile       The ktx2 file(s) to supercompress. The output is written to a\n"
        "               file of the same name. If infile not specified input will be read\n"
        "               from stdin and the compressed texture written to stdout.\n"
        "\n"
        "  Options are:\n"
        "\n"
        "  -o outfile, --output=outfile\n"
        "               Writes the output to outfile. If there is more than 1 input\n"
        "               file the ommand prints its usage message and exits. If outfile\n"
        "               is 'stdout', output will be written to stdout. If there is more\n"
        "               than 1 infile the command prints its usage message and exits.\n"
        "  -f, --force  If the output file cannot be opened, remove it and create a\n"
        "               new file, without prompting for confirmation regardless of\n"
        "               its permissions.\n"
        " --no_multithreading\n"
        "               Disable multithreading. By default Basis compression will use\n"
        "               the numnber of threads reported by\n"
        "               @c std::thread::hardware_concurrency or 1 if value returned is 0.\n"
        " --threads <count>\n"
        "               Explicitly set the number of threads to use during compression.\n"
        "               --no_multithreading must not be set.\n"
        " --clevel <level>\n"
        "               Basis compression level, an encoding speed vs. quality tradeoff.\n"
        "               Range is 0 - 5, default is 1. Higher values are slower, but give\n"
        "               higher quality.\n"
        " --qlevel <level>\n"
        "               Basis quality level. Range is 1 - 255.  Lower gives better\n"
        "               compression/lower quality/faster. Higher gives less compression\n"
        "               /higher quality/slower. Values of --max_endpoints and\n"
        "               --max-selectors computed from this override any explicitly set\n"
        "               values. Default is 128, if either of --max_endpoints or\n"
        "               --max_selectors is unset, otherwise those settings rule.\n"
        " --max_endpoints <arg>\n"
        "               Manually set the maximum number of color endpoint clusters from\n"
        "               1-16128. Default is 0, unset.\n"
        " --endpoint_rdo_threshold <arg>\n"
        "               Set endpoint RDO quality threshold. The default is 1.25. Lower\n"
        "               is higher quality but less quality per output bit (try 1.0-3.0).\n"
        "               This will override the value chosen by --qual.\n"
        " --max_selectors <arg>\n"
        "               Manually set the maximum number of color selector clusters from\n"
        "               1-16128. Default is 0, unset.\n"
        " --selector_rdo_threshold <arg>\n"
        "               Set selector RDO quality threshold. The default is 1.25. Lower\n"
        "               is higher quality but less quality per output bit (try 1.0-3.0).\n"
        "               This will override the value chosen by --qual.\n"
        " --normal_map\n"
        "               Tunes codec parameters for better quality on normal maps (no\n"
        "               selector RDO, no endpoint RDO). Only valid for linear textures.\n"
        " --separate_rg_to_color_alpha\n"
        "               Separates the input R and G channels to RGB and A (for tangent\n"
        "               space XY normal maps). Automatically selected if the input\n"
        "               image(s) are 2-component."
        " --no_endpoint_rdo\n"
        "               Disable endpoint rate distortion optimizations. Slightly faster,\n"
        "               less noisy output, but lower quality per output bit. Default is\n"
        "               to do endpoint RDO.\n"
        " --no_selector_rdo\n"
        "               Disable selector rate distortion optimizations. Slightly faster,\n"
        "               less noisy output, but lower quality per output bit. Default is\n"
        "               to do selector RDO.\n"
        "  --help       Print this usage message and exit.\n"
        "  --version    Print the version number of this program and exit.\n",
        appName.c_str());
}

#define QUOTE(x) #x
#define STR(x) QUOTE(x)

static void
writeId(std::ostream& dst, const _tstring& appName, bool test = false)
{
    dst << appName << " " << (test ? STR(KTXSC_DEFAULT_VERSION)
                                   : STR(KTXSC_VERSION));
}


static void
version(const _tstring& appName)
{
    writeId(cerr, appName);
    cerr << endl;
}


int _tmain(int argc, _TCHAR* argv[])
{
    FILE *inf, *outf;
    KTX_error_code result;
    ktxTexture2* texture = 0;
    struct commandOptions options;
    int exitCode = 0;
    const _TCHAR* pTmpFile;

    processCommandLine(argc, argv, options);

    std::vector<_tstring>::const_iterator it;
    for (it = options.infilenames.begin(); it < options.infilenames.end(); it++) {
        _tstring infile = *it;
        _tstring tmpfile = _T("/tmp/ktxsc.XXXXXX");

        if (infile.compare(_T("-")) == 0) {
            //infile = 0;
            inf = stdin;
#if defined(_WIN32)
            /* Set "stdin" to have binary mode */
            (void)_setmode( _fileno( stdin ), _O_BINARY );
#endif
        } else {
            inf = _tfopen(infile.c_str(), "rb");
        }

        if (inf) {
            if (options.useStdout) {
                outf = stdout;
#if defined(_WIN32)
                /* Set "stdout" to have binary mode */
                (void)_setmode( _fileno( stdout ), _O_BINARY );
#endif
            } else if (options.outfile.length()) {
                outf = _tfopen(options.outfile.c_str(), "wxb");
            } else {
#if defined(_WIN32)
                pTmpFile = _mktemp(&tmpfile[0]);
                if (pTmpFile != nullptr)
                    outf = _tfopen(tmpfile.c_str(), "wb");
                else
                    outf = nullptr;
#else
                outf = fdopen(mkstemp(&tmpfile[0]), "wb");
                pTmpFile = tmpfile.c_str();
#endif
            }

            if (!outf && errno == EEXIST) {
                bool force = options.force;
                if (!force) {
                    if (isatty(fileno(stdin))) {
                        char answer;
                        cout << "Output file " << options.outfile
                             << " exists. Overwrite? [Y or n] ";
                        cin >> answer;
                        if (answer == 'Y') {
                            force = true;
                        }
                    }
                }
                if (force) {
                    outf = _tfopen(options.outfile.c_str(), "wb");
                }
            }

            if (outf) {
                result = ktxTexture_CreateFromStdioStream(inf,
                                        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                        (ktxTexture**)&texture);

                if (result != KTX_SUCCESS) {
                    cerr << appName
                         << " failed to create ktxTexture; "
                         << ktxErrorString(result) << endl;
                    exitCode = 2;
                    goto cleanup;
                }
               (void)fclose(inf);

                // Modify the required writer metadata.
                std::stringstream writer;
                writeId(writer, appName);
                ktxHashList_DeleteKVPair(&texture->kvDataHead, KTX_WRITER_KEY);
                ktxHashList_AddKVPair(&texture->kvDataHead, KTX_WRITER_KEY,
                                      (ktx_uint32_t)writer.str().length() + 1,
                                      writer.str().c_str());

                commandOptions::basisOptions& bopts = options.bopts;
                ktx_uint32_t transfer = ktxTexture2_GetOETF(texture);
                if (bopts.normalMap && transfer != KHR_DF_TRANSFER_LINEAR) {
                    fprintf(stderr, "%s: --normal_map specified but input file(s) are"
                            " not linear.", appName.c_str());
                    exitCode = 1;
                    goto cleanup;
                }
                if (bopts.noMultithreading)
                    bopts.threadCount = 1;
                uint32_t components, componentByteLength;
                ktxTexture2_GetComponentInfo(texture,
                                             &components,
                                             &componentByteLength);
                if (components == 2) {
                    bopts.separateRGToRGB_A = true;
                }

                result = ktxTexture2_CompressBasisEx(texture, &bopts);
                if (result != KTX_SUCCESS) {
                    cerr << appName
                         << " failed to compress KTX2 file; "
                         << ktxErrorString(result) << endl;
                    exitCode = 2;
                    goto cleanup;
                }

                result = ktxTexture_WriteToStdioStream(ktxTexture(texture), outf);
                if (result != KTX_SUCCESS) {
                    cerr << appName
                         << " failed to write KTX2 file; "
                         << ktxErrorString(result) << endl;
                    exitCode = 2;
                    goto cleanup;
                }
                (void)fclose(outf);
                if (!options.outfile.length() && !options.useStdout) {
                    // Move the new file over the original.
                    assert(pTmpFile && infile.length());
                    int err = _trename(tmpfile.c_str(), infile.c_str());
                    if (err) {
                        cerr << appName
                             << ": rename of \"%s\" to \"%s\" failed: "
                             << strerror(errno) << endl;
                        exitCode = 2;
                        goto cleanup;
                    }
                }
            } else {
                cerr << appName
                     << " could not open output file \""
                     << (options.useStdout ? "stdout" : options.outfile) << "\". "
                     << strerror(errno) << endl;
                exitCode = 2;
                goto cleanup;
            }
        } else {
            cerr << appName
                 << " could not open input file \""
                 << (infile.compare(_T("-")) == 0 ? "stdin" : infile) << "\". "
                 << strerror(errno) << endl;
            exitCode = 2;
            goto cleanup;
        }
    }
    return 0;

cleanup:
  if (pTmpFile) (void)_tunlink(pTmpFile);
    if (options.outfile.length()) (void)_tunlink(options.outfile.c_str());
    return exitCode;
}


static void processCommandLine(int argc, _TCHAR* argv[], struct commandOptions& options)
{
    int i;
    size_t slash, dot;

    appName = argv[0];
      // For consistent Id, only use the stem of appName;
    slash = appName.find_last_of(_T('\\'));
    if (slash == _tstring::npos)
      slash = appName.find_last_of(_T('/'));
    if (slash != _tstring::npos)
      appName.erase(0, slash+1);  // Remove directory name.
    dot = appName.find_last_of(_T('.'));
      if (dot != _tstring::npos)
      appName.erase(dot, _tstring::npos); // Remove extension.

    argparser parser(argc, argv);
    processOptions(parser, options);

    i = parser.optind;
    if (argc - i > 0) {
        for (; i < argc; i++) {
            options.infilenames.push_back(parser.argv[i]);
        }
    }

    switch (options.infilenames.size()) {
      case 0:
        options.infilenames.push_back(_T("-")); // Use stdin
        break;
      case 1:
        break;
      default: {
        /* Check for attempt to use stdin as one of the
         * input files.
         */
        std::vector<_tstring>::const_iterator it;
        for (it = options.infilenames.begin(); it < options.infilenames.end(); it++) {
            if (it->compare(_T("-")) == 0) {
                fprintf(stderr, "%s: cannot use stdin as one among many inputs.\n",
                        appName.c_str());
                usage(appName);
                exit(1);
            }
        }
      }
      break;
    }

    if (!options.outfile.compare(_T("stdout")))
        options.useStdout = true;

    if (options.infilenames.size() > 1 && options.outfile.length()) {
        usage(appName);
        exit(1);
    }
}

/*
 * @brief process potential command line options
 *
 * @return
 *
 * @param[in]     parser,     an @c argparser holding the options to process.
 * @param[in,out] options     commandOptions struct in which option information
 *                            is set.
 */
static void
processOptions(argparser& parser,
               struct commandOptions& options)
{
    _TCHAR ch;
    static struct argparser::option option_list[] = {
        { "force", argparser::option::no_argument, NULL, 'f' },
        { "help", argparser::option::no_argument, NULL, 'h' },
        { "outfile", argparser::option::required_argument, NULL, 'o' },
        { "version", argparser::option::no_argument, NULL, 'v' },
        { "mo_multithreading", argparser::option::no_argument, NULL, 'm' },
        { "threads", argparser::option::required_argument, NULL, 't' },
        { "clevel", argparser::option::required_argument, NULL, 'c' },
        { "qlevel", argparser::option::required_argument, NULL, 'q' },
        { "max_endpoints", argparser::option::required_argument, NULL, 'e' },
        { "endpoint_rdo_threshold", argparser::option::required_argument, NULL, 'g' },
        { "max_selectors", argparser::option::required_argument, NULL, 's' },
        { "selector_rdo_threshold", argparser::option::required_argument, NULL, 'u' },
        { "normal_map", argparser::option::no_argument, NULL, 'n' },
        { "separate_rg_to_color_alpha", argparser::option::no_argument, NULL, 'r' },
        { "no_endpoint_rdo", argparser::option::no_argument, NULL, 'b' },
        { "no_selector_rdo", argparser::option::no_argument, NULL, 'p' },
        { "test", argparser::option::no_argument, &options.test, 1},
        // -NSDocumentRevisionsDebugMode YES is appended to the end
        // of the command by Xcode when debugging and "Allow debugging when
        // using document Versions Browser" is checked in the scheme. It
        // defaults to checked and is saved in a user-specific file not the
        // pbxproj file so it can't be disabled in a generated project.
        // Remove these from the arguments under consideration.
        { "-NSDocumentRevisionsDebugMode", argparser::option::required_argument, NULL, 'i' },
        { nullptr, argparser::option::no_argument, nullptr, 0 }
    };

    _tstring shortopts("bc:e:fg:hmno:pq:rs:t:u:o:v");
    while ((ch = parser.getopt(&shortopts, option_list, NULL)) != -1) {
        switch (ch) {
          case 0:
            break;
          case 'f':
            options.force = true;
            break;
          case 'o':
            options.outfile = parser.optarg;
            if (!options.outfile.compare(_T("stdout"))) {
                options.useStdout = true;
            } else {
                size_t dot;
                dot = options.outfile.find_last_of('.');
                if (dot == _tstring::npos) {
                    options.outfile += _T(".ktx2");
                }
            }
            break;
          case 'c':
            options.bopts.compressionLevel = atoi(parser.optarg.c_str());
            break;
          case 'e':
            options.bopts.maxEndpoints = atoi(parser.optarg.c_str());
            break;
          case 'g':
            options.bopts.endpointRDOThreshold = strtof(parser.optarg.c_str(), nullptr);
            break;
          case 'm':
            options.bopts.noMultithreading = 1;
            break;
          case 'n':
            options.bopts.normalMap = 1;
            break;
          case 'b':
            options.bopts.noEndpointRDO = 1;
            break;
          case 'p':
            options.bopts.noSelectorRDO = 1;
            break;
          case 'q':
            options.bopts.qualityLevel = atoi(parser.optarg.c_str());
            break;
          case 'r':
            options.bopts.separateRGToRGB_A = 1;
            break;
          case 's':
            options.bopts.maxSelectors = atoi(parser.optarg.c_str());
            break;
          case 't':
            options.bopts.threadCount = atoi(parser.optarg.c_str());
            break;
          case 'u':
            options.bopts.selectorRDOThreshold = strtof(parser.optarg.c_str(), nullptr);
            break;
          case 'h':
            usage(appName);
            exit(0);
          case 'v':
            version(appName);
            exit(0);
          case '?':
          case ':':
          default:
            usage(appName);
            exit(1);
        }
    }
}

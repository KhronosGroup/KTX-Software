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
#include <vector>

#include "ktx.h"
#include "argparser.h"

#if defined(_MSC_VER)
  #define strncasecmp _strnicmp
  #define fileno _fileno
  #define isatty _isatty
  #define unlink _unlink
#endif

#define VERSION "1.0.0"

struct commandOptions {
    _TCHAR*      appName;
    _TCHAR*      outfile;
    bool         useStdin;
    bool         useStdout;
    bool         force;
    unsigned int numInputFiles;
    unsigned int firstInfileIndex;

    commandOptions() {
        appName = 0;
        outfile = 0;
        numInputFiles = 0;
        firstInfileIndex = 0;
        force = false;
        useStdin = false;
        useStdout = false;
    }

    ~commandOptions() {
        if (outfile) delete outfile;
    }
};

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
    files (KTX2). @b ktxsc reads each named @e infile and compresses it in
    place. When no file is specified, a single image will be read from stdin.
    and the output written to standard out. When one or more files is specified
    each will be comoressed in place.

    The following options are available:
    <dl>
    <dt>-o outfile, --output=outfile</dt>
    <dd>Write the output to @e outfile. If @e outfile is 'stdout', output will
        be written to stdout. If there is more than 1 input file the command
        prints its usage message and exits.</dd>
    <dt>-f, --force</dt>
    <dd>If the destination file cannot be opened, remove it and create a
        new file, without prompting for confirmation regardless of its
        permissions.</dd>

@section ktxsc_exitstatus EXIT STATUS
    @b toktx exits 0 on success, 1 on command line errors and 2 on
    functional errors.

@section ktxsc_history HISTORY

@version 1.0.alpha1:
Mon, 15 Jul 2019 19:25:43 -0700
 - Initial version.


@section ktxsc_author AUTHOR
    Mark Callow, Edgewise Consulting www.edgewise-consulting.com
*/

static void
usage(_TCHAR* appName)
{
    fprintf(stderr,
        "Usage: %s [options] [<infile> ...]\n"
        "\n"
        "  infile       The ktx2 file(s) to supercompress. The output is written to a\n"
        "               file of the same name.\n" /*If it is '-' or not specified input will\n"
        "               be read from stdin and the compressed texture written to\n"
        "               stdout.\n"*/
        "\n"
        "  Options are:\n"
        "\n"

        "  -o outfile, --output=outfile\n"
        "               Writes the output to outfile. If there is more than 1 input\n"
        "               file the ommand prints its usage message and exits.\n" /*If\n"
        "               outfile is 'stdout', output will be written to stdout.\n"*/
        "  -f, --force  If the output file cannot be opened, remove it and create a\n"
        "               new file, without prompting for confirmation regardless of\n"
        "               its permissions.\n",
        appName);
}


static void
writeId(std::ostream& dst, _TCHAR* appName)
{
    dst << appName << " version " << VERSION;
}


static void
version(_TCHAR* appName)
{
    writeId(cerr, appName);
    cerr << std::endl;
}


int _tmain(int argc, _TCHAR* argv[])
{
    FILE *inf, *outf;
    _TCHAR *outfile;
    _TCHAR* tmpfile;
    KTX_error_code result;
    ktxTexture2* texture = 0;
    struct commandOptions options;
    int exitCode = 0;

    processCommandLine(argc, argv, options);

    for (ktx_uint32_t i = 0; i < options.numInputFiles; i++) {
        _TCHAR *infile;

        if (options.useStdin) {
            infile = 0;
            inf = stdin;
#if defined(_WIN32)
            /* Set "stdin" to have binary mode */
            (void)_setmode( _fileno( stdin ), _O_BINARY );
#endif
        } else {
            infile = argv[options.firstInfileIndex + i];
            inf = fopen(infile, "rb");
        }

        if (inf) {
            tmpfile = NULL;

            if (options.useStdout) {
                outf = stdout;
#if defined(_WIN32)
                /* Set "stdout" to have binary mode */
                (void)_setmode( _fileno( stdout ), _O_BINARY );
#endif
            } else if (options.outfile) {
                outf = fopen(outfile, "wxb");
            } else {
                char nametmpl[] =  { "/tmp/temp.XXXXXX" };
                tmpfile = mktemp(nametmpl);
                outf = fopen(tmpfile, "wb");
            }

            if (!outf && errno == EEXIST) {
                bool force = options.force;
                if (!force) {
                    if (isatty(fileno(stdin))) {
                        char answer;
                        cout << "Output file " << outfile
                             << " exists. Overwrite? [Y or n] ";
                        cin >> answer;
                        if (answer == 'Y') {
                            force = true;
                        }
                    }
                }
                if (force) {
                    outf = fopen(outfile, "wb");
                }
            }

            if (outf) {
                result = ktxTexture_CreateFromStdioStream(inf,
                                        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                        (ktxTexture**)&texture);

                if (result != KTX_SUCCESS) {
                    cerr << options.appName
                         << " failed to create ktxTexture; "
                         << ktxErrorString(result) << endl;
                    exitCode = 2;
                    goto cleanup;
                }
               (void)fclose(inf);

                // Modify the required writer metadata.
                std::stringstream writer;
                writeId(writer, options.appName);
                ktxHashList_DeleteKVPair(&texture->kvDataHead, KTX_WRITER_KEY);
                ktxHashList_AddKVPair(&texture->kvDataHead, KTX_WRITER_KEY,
                                      (ktx_uint32_t)writer.str().length() + 1,
                                      writer.str().c_str());

                result = ktxTexture2_CompressBasis(texture, 0);
                if (result != KTX_SUCCESS) {
                    cerr << options.appName
                         << " failed to compress KTX2 file; "
                         << ktxErrorString(result) << endl;
                    exitCode = 2;
                    goto cleanup;
                }

                result = ktxTexture_WriteToStdioStream(ktxTexture(texture), outf);
                if (result != KTX_SUCCESS) {
                    cerr << options.appName
                         << " failed to write KTX2 file; "
                         << ktxErrorString(result) << endl;
                    exitCode = 2;
                    goto cleanup;
                }
                (void)fclose(outf);
                if (!options.outfile && !options.useStdout) {
                    // Move the new file over the original.
                    assert(tmpfile && infile);
                    int err = rename(tmpfile, infile);
                    if (err) {
                        cerr << options.appName
                             << ": rename of \"%s\" to \"%s\" failed: "
                             << strerror(errno) << endl;
                        exitCode = 2;
                        goto cleanup;
                    }
                }
            } else {
                cerr << options.appName
                     << " could not open output file \""
                     << (options.useStdout ? "stdout" : outfile) << "\". "
                     << strerror(errno) << endl;
                exitCode = 2;
                goto cleanup;
            }
        } else {
            cerr << options.appName
                 << " could not open input file \""
                 << (infile ? infile : "stdin") << "\". "
                 << strerror(errno) << endl;
            exitCode = 2;
            goto cleanup;
        }
    }
    return 0;

cleanup:
    if (tmpfile) (void)unlink(tmpfile);
    if (options.outfile) (void)unlink(outfile);
    return exitCode;
}


static void processCommandLine(int argc, _TCHAR* argv[], struct commandOptions& options)
{
    int i;
    _TCHAR* slash;

    slash = _tcsrchr(argv[0], '\\');
    if (slash == NULL)
        slash = _tcsrchr(argv[0], '/');
    options.appName = slash != NULL ? slash + 1 : argv[0];

    argparser parser(argc, argv);
    processOptions(parser, options);

    i = parser.optind;
    options.numInputFiles = argc - i;
    options.firstInfileIndex = i;
    switch (options.numInputFiles) {
      case 0:
        options.numInputFiles = 1;
        options.useStdin = true;
        break;

      case 1:
        if (_tcscmp(argv[i], "-") == 0) {
            options.numInputFiles = 1;
            options.useStdin = true;
        }
        break;

      default:
        /* Check for attempt to use stdin as one of the
         * input files.
         */
        for (++i; i < argc; i++) {
            if (_tcscmp(argv[i], "-") == 0) {
                usage(options.appName);
                exit(1);
            }
        }
    }

    if (options.numInputFiles > 1 && options.outfile) {
        usage(options.appName);
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
    bool addktx2 = false;
    _TCHAR ch;
    const _TCHAR* filename;
    unsigned int filenamelen;
    static struct argparser::option option_list[] = {
        { "force", argparser::option::no_argument, NULL, 'f' },
        { "help", argparser::option::no_argument, NULL, 'h' },
        { "outfile", argparser::option::required_argument, NULL, 'o' },
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

    tstring shortopts("fho:v");
    while ((ch = parser.getopt(&shortopts, option_list, NULL)) != -1) {
        switch (ch) {
          case 0:
            break;
          case 'f':
            options.force = true;
            break;
          case 'o':
            filename = parser.optarg.c_str();
            if (!_tcscmp(filename, "stdout")) {
                options.useStdout = true;
            } else {
                filenamelen = (unsigned int)_tcslen(filename) + 1;
                if (_tcsrchr(filename, '.') == NULL) {
                    addktx2 = true;
                    filenamelen += 5;
                }
                options.outfile = new _TCHAR[filenamelen];
                if (options.outfile) {
                    _tcscpy(options.outfile, filename);
                    if (addktx2) {
                        _tcscat(options.outfile, ".ktx2");
                    }
                } else {
                    fprintf(stderr, "%s: out of memory.\n", options.appName);
                    exit(2);
                }
            }
            break;
          case 'h':
            usage(options.appName);
            exit(0);
          case 'v':
            version(options.appName);
            exit(0);
          case '?':
          case ':':
          default:
            usage(options.appName);
            exit(1);
        }
    }
}

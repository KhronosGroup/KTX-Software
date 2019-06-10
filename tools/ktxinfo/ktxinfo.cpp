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

// To use, download from http://www.billbaxter.com/projects/imdebug/
// Put imdebug.dll in %SYSTEMROOT% (usually C:\WINDOWS), imdebug.h in
// ../../include, imdebug.lib in ../../build/msvs/<platform>/vs<ver> &
// add ..\imdebug.lib to the libraries list in the project properties.
#define IMAGE_DEBUG 0

#include "stdafx.h"
#include <cstdlib>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <vector>

#include "ktx.h"
#include "argparser.h"
#if (IMAGE_DEBUG) && defined(_DEBUG) && defined(_WIN32) && !defined(_WIN32_WCE)
#  include "imdebug.h"
#elif defined(IMAGE_DEBUG) && IMAGE_DEBUG
#  undef IMAGE_DEBUG
#  define IMAGE_DEBUG 0
#endif

#define COMMIT "$Id$"
#define VERSION "1.0.0"

struct commandOptions {
    _TCHAR*      appName;
    _TCHAR*      outfile;
    _TCHAR*      outdir;
    bool         useStdin;
    bool         force;
    unsigned int numInputFiles;
    unsigned int firstInfileIndex;

    commandOptions() {
        appName = 0;
        numInputFiles = 0;
        firstInfileIndex = 0;
        useStdin = false;
        force = false;
    }

    ~commandOptions() {
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

/** @page ktxinfo ktxinfo
@~English

Print information about a KTX or KTX2 file.

@section ktxinfo_synopsis SYNOPSIS
    ktxinfo [options] [@e infile ...]

@section ktxinfo_description DESCRIPTION
    @b ktxinfo prints information about the KTX files provided as arguments.
    If no arguments are given, it prints information about a single file be
    read from standard input.

@section ktxinfo_exitstatus EXIT STATUS
    @b ktxinfo exits 0 on success, 1 on command line errors and 2 if one of
    the input files is not a KTX file.

@section ktxinfo_history HISTORY

@version 1.0.alpha2:
Mon, Jun 10 2019 08:49:22 +0900
 - Support latest KTX2 draft and fix man page typos.
@version 1.0.alpha1:
Sat, 28 Apr 2018 14:41:22 +0900
 - Initial version

@section ktxinfo_author AUTHOR
    Mark Callow, Edgewise Consulting www.edgewise-consulting.com
*/

static void
usage(_TCHAR* appName)
{
    fprintf(stderr,
        "Usage: %s [options] [<infile> ...]\n"
        "\n"
        "  infile ...   The file or files about which to print information. If\n"
        "               not specified, stdin is read.\n"
//       "\n"
//        "  Options are:\n"
//       "\n"
        ,
        appName);
}


static void
version(_TCHAR* appName)
{
    fprintf(stderr, "%s version %s. Commit %s\n", appName, VERSION, COMMIT);
}


int _tmain(int argc, _TCHAR* argv[])
{
    FILE *inf;
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
            KTX_error_code result;

            result = ktxPrintInfoForStdioStream(inf);
            if (result ==  KTX_FILE_UNEXPECTED_EOF) {
                cerr << options.appName
                     << ": Unexpected end of file reading \""
                     << (infile ? infile : "stdin") << "\"."
                     << endl;
                     exit(2);
            }
            if (result == KTX_UNKNOWN_FILE_FORMAT) {
                cerr << options.appName
                     << ": " << (infile ? infile : "stdin")
                     << " is not a KTX or KTX2 file."
                     << endl;
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

cleanup:
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

    tstring shortopts("fd:ho:v");
    while ((ch = parser.getopt(&shortopts, option_list, NULL)) != -1) {
        switch (ch) {
          case 0:
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


#if IMAGE_DEBUG
static void
dumpImage(_TCHAR* name, int width, int height, int components, int componentSize,
          bool isLuminance, unsigned char* srcImage)
{
    char formatstr[2048];
    char *imagefmt;
    char *fmtname;
    int bitsPerComponent = componentSize == 2 ? 16 : 8;

    switch (components) {
      case 1:
        if (isLuminance) {
            imagefmt = "lum b=";
            fmtname = "LUMINANCE";
        } else {
            imagefmt = "a b=";
            fmtname = "ALPHA";
        }
        break;
      case 2:
        imagefmt = "luma b=";
        fmtname = "LUMINANCE_ALPHA";
        break;
      case 3:
        imagefmt = "rgb b=";
        fmtname = "RGB";
        break;
      case 4:
        imagefmt = "rgba b=";
        fmtname = "RGBA";
        break;
      default:
        assert(0);
    }
    sprintf(formatstr, "%s%d w=%%d h=%%d t=\'%s %s%d\' %%p",
            imagefmt,
            bitsPerComponent,
            name,
            fmtname,
            bitsPerComponent);
    imdebug(formatstr, width, height, srcImage);
}
#endif

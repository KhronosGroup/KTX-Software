// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 sts=4 expandtab:

//
// Copyright 2019-2020 The Khronos Group, Inc.
// SPDX-License-Identifier: Apache-2.0
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
#include "version.h"

struct commandOptions {
    _tstring      outfile;
    _tstring      outdir;
    bool         force;
    std::vector<_tstring> infiles;

    commandOptions() {
        force = false;
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

/** @page ktxinfo ktxinfo
@~English

Print information about a KTX or KTX2 file.

@section ktxinfo_synopsis SYNOPSIS
    ktxinfo [options] [@e infile ...]

@section ktxinfo_description DESCRIPTION
    @b ktxinfo prints information about the KTX files provided as arguments.
    If no arguments are given, it prints information about a single file
    read from standard input.

    @note @b ktxinfo prints using UTF-8 encoding. If your console is not
    set for UTF-8 you will see incorrect characters in output of the file
    identifier on each side of the "KTX nn".

    The following options are available:
    <dl>
    <dt>--help</dt>
    <dd>Print this usage message and exit.</dd>
    <dt>--version</dt>
    <dd>Print the version number of this program and exit.</dd>
    </dl>

@section ktxinfo_exitstatus EXIT STATUS
    @b ktxinfo exits 0 on success, 1 on command line errors and 2 if one of
    the input files is not a KTX file.

@section ktxinfo_history HISTORY

@par Version 4.0
 - Initial version

@section ktxinfo_author AUTHOR
    Mark Callow, Edgewise Consulting www.edgewise-consulting.com
*/

static void
usage(_tstring& appName)
{
    fprintf(stderr,
        "Usage: %s [options] [<infile> ...]\n"
        "\n"
        "  infile ...   The file or files about which to print information. If\n"
        "               not specified, stdin is read.\n"
        "\n"
        "  Note that ktxinfo prints using UTF-8 encoding. If your console is not\n"
        "  set for UTF-8 you will see incorrect characters in output of the file\n"
        "  identifier on each side of the \"KTX nn\".\n"
        "\n"
        "  Options are:\n"
        "  --help       Print this usage message and exit.\n"
        "  --version    Print the version number of this program and exit.\n"
        "\n"
        ,
        appName.c_str());
}

#define QUOTE(x) #x
#define STR(x) QUOTE(x)

static void
version(_tstring& appName)
{
    std::cerr << appName << " " << STR(KTXINFO_VERSION) << std::endl;
}


int _tmain(int argc, _TCHAR* argv[])
{
    FILE *inf;
    struct commandOptions options;
    int exitCode = 0;

    processCommandLine(argc, argv, options);

    std::vector<_tstring>::const_iterator it;
    for (it = options.infiles.begin(); it < options.infiles.end(); it++) {
        _tstring infile = *it;

        if (!infile.compare(_T("-"))) {
            inf = stdin;
#if defined(_WIN32)
            /* Set "stdin" to have binary mode */
            (void)_setmode( _fileno( stdin ), _O_BINARY );
#endif
        } else {
            inf = _tfopen(infile.c_str(), "rb");
        }

        if (inf) {
            KTX_error_code result;

            result = ktxPrintInfoForStdioStream(inf);
            if (result ==  KTX_FILE_UNEXPECTED_EOF) {
                cerr << appName
                     << ": Unexpected end of file reading \""
                     << (infile.compare(_T("-")) ? infile : "stdin" ) << "\"."
                     << endl;
                     exit(2);
            }
            if (result == KTX_UNKNOWN_FILE_FORMAT) {
                cerr << appName
                     << ": " << (infile.compare(_T("-")) ? infile : "stdin")
                     << " is not a KTX or KTX2 file."
                     << endl;
                     exitCode = 2;
                     goto cleanup;
            }
        } else {
            cerr << appName
                 << " could not open input file \""
                 << (infile.compare(_T("-")) ? infile : "stdin") << "\". "
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
            options.infiles.push_back(parser.argv[i]);
        }
    }

    switch (options.infiles.size()) {
      case 0:
        options.infiles.push_back(_T("-")); // Use stdin
        break;
      case 1:
        break;
      default: {
        /* Check for attempt to use stdin as one of the
         * input files.
         */
        std::vector<_tstring>::const_iterator it;
        for (it = options.infiles.begin(); it < options.infiles.end(); it++) {
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

    _tstring shortopts("fd:ho:v");
    while ((ch = parser.getopt(&shortopts, option_list, NULL)) != -1) {
        switch (ch) {
          case 0:
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

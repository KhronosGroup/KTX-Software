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

#include "ktxapp.h"

#include <cstdlib>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <ktx.h>

#include "argparser.h"
#include "version.h"
#if (IMAGE_DEBUG) && defined(_DEBUG) && defined(_WIN32) && !defined(_WIN32_WCE)
#  include "imdebug.h"
#elif defined(IMAGE_DEBUG) && IMAGE_DEBUG
#  undef IMAGE_DEBUG
#  define IMAGE_DEBUG 0
#endif

struct commandOptions {
    string      outfile;
    string      outdir;
    bool        force;
    std::vector<string> infiles;

    commandOptions() {
        force = false;
    }
};

#if IMAGE_DEBUG
static void dumpImage(char* name, int width, int height, int components,
                      int componentSize, bool isLuminance,
                      unsigned char* srcImage);
#endif

using namespace std;

/** @page ktxinfo ktxinfo
@~English

Print information about KTX or KTX2 files.

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
    @snippet{doc} ktxapp.h ktxApp options

@section ktxinfo_exitstatus EXIT STATUS
    @b ktxinfo exits 0 on success, 1 on command line errors and 2 if one of
    the input files is not a KTX file.

@section ktxinfo_history HISTORY

@par Version 4.0
 - Initial version

@section ktxinfo_author AUTHOR
    Mark Callow, Edgewise Consulting www.edgewise-consulting.com
*/

#define QUOTE(x) #x
#define STR(x) QUOTE(x)

std::string myversion(STR(KTXINFO_VERSION));
std::string mydefversion(STR(KTXINFO_DEFAULT_VERSION));

class ktxInfo : public ktxApp {
  public:
    ktxInfo();

    virtual int main(int argc, char* argv[]);
    virtual void usage();

  protected:
    virtual bool processOption(argparser& parser, int opt);

    ktxApp::commandOptions options;
};


ktxInfo::ktxInfo() : ktxApp(myversion, mydefversion, options)
{

}


void
ktxInfo::usage()
{
    cerr <<
        "Usage: " << name << " [options] [<infile> ...]\n"
        "\n"
        "  infile ...   The file or files about which to print information. If\n"
        "               not specified, stdin is read.\n"
        "\n"
        "  Note that ktxinfo prints using UTF-8 encoding. If your console is not\n"
        "  set for UTF-8 you will see incorrect characters in output of the file\n"
        "  identifier on each side of the \"KTX nn\".\n"
        "\n"
        "  Options are:\n\n";
        ktxApp::usage();
}

static ktxInfo ktxinfo;
ktxApp& theApp = ktxinfo;

int
ktxInfo::main(int argc, char* argv[])
{
    FILE *inf;
    int exitCode = 0;

    processCommandLine(argc, argv);

    std::vector<string>::const_iterator it;
    for (it = options.infiles.begin(); it < options.infiles.end(); it++) {
       string infile = *it;

        if (!infile.compare("-")) {
            inf = stdin;
#if defined(_WIN32)
            /* Set "stdin" to have binary mode */
            (void)_setmode( _fileno( stdin ), _O_BINARY );
#endif
        } else {
            inf = fopenUTF8(infile, "rb");
        }

        if (inf) {
            KTX_error_code result;

            result = ktxPrintInfoForStdioStream(inf);
            if (result ==  KTX_FILE_UNEXPECTED_EOF) {
                cerr << name
                     << ": Unexpected end of file reading \""
                     << (infile.compare("-") ? infile : "stdin" ) << "\"."
                     << endl;
                exitCode = 2;
                goto cleanup;
            }
            if (result == KTX_UNKNOWN_FILE_FORMAT) {
                cerr << name
                     << ": " << (infile.compare("-") ? infile : "stdin")
                     << " is not a KTX or KTX2 file."
                     << endl;
                exitCode = 2;
                goto cleanup;
            }
            if (result == KTX_FILE_READ_ERROR) {
                cerr << name
                    << ": Error reading \""
                    << (infile.compare("-") ? infile : "stdin") << "\"."
                    << strerror(errno) << endl;
                exitCode = 2;
                goto cleanup;
            }
        } else {
            cerr << name
                 << " could not open input file \""
                 << (infile.compare("-") ? infile : "stdin") << "\". "
                 << strerror(errno) << endl;
            exitCode = 2;
            goto cleanup;
        }
    }

cleanup:
    return exitCode;
}


bool
ktxInfo::processOption(argparser&, int)
{
    return false;
}


#if IMAGE_DEBUG
static void
dumpImage(char* name, int width, int height, int components, int componentSize,
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

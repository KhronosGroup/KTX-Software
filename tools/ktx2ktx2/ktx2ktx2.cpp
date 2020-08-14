// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 sts=4 expandtab:

// $Id$

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
#include "version.h"
#if (IMAGE_DEBUG) && defined(_DEBUG) && defined(_WIN32) && !defined(_WIN32_WCE)
#  include "imdebug.h"
#elif defined(IMAGE_DEBUG) && IMAGE_DEBUG
#  undef IMAGE_DEBUG
#  define IMAGE_DEBUG 0
#endif

#if defined(_MSC_VER)
  #define strncasecmp _strnicmp
  #define fileno _fileno
  #define isatty _isatty
  #define unlink _unlink
#endif

struct commandOptions {
    _tstring     appName;
    _tstring     outfile;
    _tstring     outdir;
    bool         useStdout;
    bool         force;
    bool         rewriteBadOrientation;
    int          test;
    std::vector<_tstring> infiles;

    commandOptions() {
        useStdout = false;
        force = false;
        rewriteBadOrientation = false;
        test = 0;
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

/** @page ktx2ktx2 ktx2ktx2
@~English

Create a KTX 2 file from a KTX file.

@section ktx2ktx2_synopsis SYNOPSIS
    ktx2ktx2 [options] [@e infile ...]

@section ktx2ktx2_description DESCRIPTION
    @b ktx2ktx2 creates Khronos texture format version 2 files (KTX2) from
    Khronos texture format version 1 files. @b ktx2ktx2 reads each named
    @e infile. Output files have the same name as the input but with the
    extension changed to @c .ktx2. When @b infile is not specified, a single
    file will be read from stdin and the output written to standard out.

    If unrecognized metadata with keys beginning "KTX" or "ktx" is found in
    the input file, it is dropped and a warning is written to standard error.

    The following options are available:
    <dl>
    <dt>-b, --rewritebado</dt>
    <dd>Rewrite bad orientation metadata. Some in-the-wild KTX files
        have orientation metadata with the key "KTXOrientation"
        instead of KTXorientaion. This option will rewrite such
        bad metadata instead of dropping it.
    <dt>-o outfile, --output=outfile</dt>
    <dd>Name the output file @e outfile. If @e outfile is 'stdout', output will
        be written to stdout. If there is more than 1 input file, the command
        prints its usage message and exits.</dd>
    <dt>-d outdir, --output-dir=outdir</dt>
    <dd>Writes the output files to the directory @e outdir. If both
        @b --output and @b --output-dir are specified, @e outfile will
        be written in @e outdir. If @b infile is @e stdin or @b outfile is
        @e stdout, the command prints its usage message and exits.</dd>
    <dt>-f, --force</dt>
    <dd>If the destination file already exists, remove it and create a
        new file, without prompting for confirmation regardless of its
        permissions.</dd>

@section ktx2ktx2_exitstatus EXIT STATUS
    @b toktx exits 0 on success, 1 on command line errors and 2 on
    functional errors.

@section ktx2ktx2_history HISTORY

@par Version 4.0
 - Initial version.

@section ktx2ktx2_author AUTHOR
    Mark Callow, Edgewise Consulting www.edgewise-consulting.com
*/

static void
usage(_tstring& appName)
{
    fprintf(stderr,
        "Usage: %s [options] [<infile> ...]\n"
        "\n"
        "  infile       The source ktx file. The output is written to a file of the\n"
        "               same name with the extension changed to '.ktx2'. If it is not\n"
        "               specified input will be read from stdin and the converted texture\n"
        "               written to stdout.\n"
        "\n"
        "  Options are:\n"
        "\n"
        "  -b, --rewritebado\n"
        "               Rewrite bad orientation metadata. Some in-the-wild KTX files\n"
        "               have orientation metadata with the key \"KTXOrientation\"\n"
        "               instead of \"KTXorientaion\". This option will rewrite such\n"
        "               bad metadata instead of dropping it.\n"
        "  -o outfile, --output=outfile\n"
        "               Name the output file outfile. If @e outfile is 'stdout', output\n"
        "               will be written to stdout. If there is more than 1 infile,\n"
        "               the command prints its usage message and exits.\n"
        "  -d outdir, --output-dir=outdir\n"
        "               Writes the output files to the directory outdir. If both\n"
        "               --output and --output-dir are specified, outfile\n"
        "               will be written in outdir. If infile is stdin or outfile is\n"
        "               stdout, the command prints its usage message and exits.\n"
        "  -f, --force  If the output file already exists, remove it and create a\n"
        "               new file, without prompting for confirmation regardless of\n"
        "               its permissions.\n",
        appName.c_str());
}

#define QUOTE(x) #x
#define STR(x) QUOTE(x)

static void
writeId(std::ostream& dst, const _tstring& appName, bool test = false)
{
    dst << appName << " " << (test ? STR(KTX2KTX2_DEFAULT_VERSION)
                                   : STR(KTX2KTX2_VERSION));
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
    ktxTexture1* texture = 0;
    struct commandOptions options;
    int exitCode = 0;

    processCommandLine(argc, argv, options);

    std::vector<_tstring>::const_iterator it;
    for (it = options.infiles.begin(); it < options.infiles.end(); it++) {
        _tstring infile = *it;
        _tstring outfile;

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
            if (infile.compare(_T("-"))
                && !options.useStdout && !options.outfile.length())
            {
                size_t dot;

                outfile = infile;
                dot = outfile.find_last_of(_T('.'));
                if (dot != _tstring::npos) {
                    outfile.erase(dot, _tstring::npos);
                }
                outfile += _T(".ktx2");
            } else if (options.outfile.length()) {
                outfile = options.outfile;
            }

            if (options.useStdout) {
                outf = stdout;
#if defined(_WIN32)
                /* Set "stdout" to have binary mode */
                (void)_setmode( _fileno( stdout ), _O_BINARY );
#endif
            } else if (outfile.length()) {
                outf = _tfopen(outfile.c_str(), "wxb");
            }

            if (!outf && errno == EEXIST) {
                bool force = options.force;
                if (!force) {
                    if (isatty(fileno(stdin))) {
                        char answer;
                        cout << "Output file " << options.outfile.c_str()
                             << " exists. Overwrite? [Y or n] ";
                        cin >> answer;
                        if (answer == 'Y') {
                            force = true;
                        }
                    }
                }
                if (force) {
                    outf = _tfopen(outfile.c_str(), "wb");
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

                // Some in-the-wild KTX files have incorrect KTXOrientation
                // Warn about dropping invalid metadata.
                ktxHashListEntry* pEntry;
                for (pEntry = texture->kvDataHead;
                     pEntry != NULL;
                     pEntry = ktxHashList_Next(pEntry)) {
                    unsigned int keyLen;
                    char* key;

                    ktxHashListEntry_GetKey(pEntry, &keyLen, &key);
                    if (strncasecmp(key, "KTX", 3) == 0) {
                        if (strcmp(key, KTX_ORIENTATION_KEY)
                            && strcmp(key, KTX_WRITER_KEY)) {
                            if (strcmp(key, "KTXOrientation") == 0
                                && options.rewriteBadOrientation) {
                                    unsigned int orientLen;
                                    char* orientation;
                                    ktxHashListEntry_GetValue(pEntry,
                                                        &orientLen,
                                                        (void**)&orientation);
                                    ktxHashList_AddKVPair(&texture->kvDataHead,
                                                          KTX_ORIENTATION_KEY,
                                                          orientLen,
                                                          orientation);
                           } else {
                                cerr << options.appName
                                     << ": Warning: Dropping unrecognized "
                                     << "metadata \"" << key << "\""
                                     << std::endl;
                            }
                            ktxHashList_DeleteEntry(&texture->kvDataHead,
                                                    pEntry);
                        }
                    }
                }

                // Add required writer metadata.
                std::stringstream writer;
                writeId(writer, options.appName, options.test != 0);
                ktxHashList_AddKVPair(&texture->kvDataHead, KTX_WRITER_KEY,
                                      (ktx_uint32_t)writer.str().length() + 1,
                                      writer.str().c_str());

                result = ktxTexture1_WriteKTX2ToStdioStream(texture, outf);
                ktxTexture_Destroy(ktxTexture(texture));
                (void)fclose(inf);
                (void)fclose(outf);
                if (result != KTX_SUCCESS) {
                    cerr << options.appName
                         << " failed to write KTX2 file; "
                         << ktxErrorString(result) << endl;
                    (void)_tunlink(options.outfile.c_str());
                    exitCode = 2;
                    goto cleanup;
                }
            } else {
                cerr << options.appName
                     << " could not open output file \""
                     << (options.outfile.length() ? options.outfile.c_str()
                                                  : "stdout")
                     << "\". " << strerror(errno) << endl;
                exitCode = 2;
                goto cleanup;
            }
        } else {
            cerr << options.appName
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

    options.appName = argv[0];
      // For consistent Id, only use the stem of appName;
    slash = options.appName.find_last_of(_T('\\'));
    if (slash == _tstring::npos)
      slash = options.appName.find_last_of(_T('/'));
    if (slash != _tstring::npos)
      options.appName.erase(0, slash+1);  // Remove directory name.
    dot = options.appName.find_last_of(_T('.'));
      if (dot != _tstring::npos)
      options.appName.erase(dot, _tstring::npos); // Remove extension.

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
                        options.appName.c_str());
                usage(options.appName);
                exit(1);
            }
        }
      }
      break;
    }

    if (!options.infiles[0].compare(_T("-")) && !options.outfile.length())
        options.useStdout = true;
    if (options.infiles.size() > 1 && options.outfile.length()) {
        usage(options.appName);
        exit(1);
    }
    if (!options.infiles[0].compare(_T("-")) && options.outdir.length()) {
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
    _TCHAR ch;
    static struct argparser::option option_list[] = {
        { "force", argparser::option::no_argument, NULL, 'f' },
        { "help", argparser::option::no_argument, NULL, 'h' },
        { "outfile", argparser::option::required_argument, NULL, 'o' },
        { "outdir", argparser::option::required_argument, NULL, 'd' },
        { "rewritebado", argparser::option::no_argument, NULL, 'b' },
        { "test", argparser::option::no_argument, &options.test, 1},
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

    _tstring shortopts("bfd:ho:v");
    while ((ch = parser.getopt(&shortopts, option_list, NULL)) != -1) {
        switch (ch) {
          case 0:
            break;
          case 'b':
            options.rewriteBadOrientation = true;
            break;
          case 'd':
            options.outdir = parser.optarg.c_str();
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

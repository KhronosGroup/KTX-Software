// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 sts=4 expandtab:

// $Id$

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
    _TCHAR*      outdir;
    bool         useStdin;
    bool         force;
    bool         rewriteBadOrientation;
    unsigned int numInputFiles;
    unsigned int firstInfileIndex;

    commandOptions() {
        appName = 0;
        outdir = 0;
        outfile = 0;
        numInputFiles = 0;
        firstInfileIndex = 0;
        useStdin = false;
        force = false;
        rewriteBadOrientation = false;
    }

    ~commandOptions() {
        if (outdir) delete outdir;
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

/** @page ktx2ktx2 ktx2ktx2
@~English

Create a KTX 2 file from a KTX file.

@section ktx2ktx2_synopsis SYNOPSIS
    ktx2ktx2 [options] [@e infile ...]

@section ktx2ktx2_description DESCRIPTION
    @b ktx2ktx2 creates Khronos texture format version 2 files (KTX2) from
    Khronos texture format version 1 files. @b ktx2ktx2 reads each named
    @e infile. Output files have the same name as the input but with the
    extension changed to @c .ktx2. When no infile is specified, a single
    image will be read from stdin and the output written to standard out.

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
    <dd>Name the output file @e outfile. If there is more than 1 input
        file, the command prints its usage message and exits.</dd>
    <dt>-d outdir, --output-dir=outdir</dt>
    <dd>Writes the output files to the directory @e outdir. If both
        @b --output and @b --output-dir are specified, @e outfile will
        be written in @e outdir. If @b infile is stdin, the command prints
        its usage message and exits.</dd>
    <dt>-f, --force</dt>
    <dd>If the destination file cannot be opened, remove it and create a
        new file, without prompting for confirmation regardless of its
        permissions.</dd>

@section ktx2ktx2_exitstatus EXIT STATUS
    @b toktx exits 0 on success, 1 on command line errors and 2 on
    functional errors.

@section ktx2ktx2_history HISTORY

@version 1.0.alpha2:
Thu, 6 Jun 2019 08:05:00 +0900
 - Update for latest KTX2 draft.
@version 1.0.alpha1:
Sat, 28 Apr 2019 14:41:22 +0900
 - Initial version.

@section ktx2ktx2_author AUTHOR
    Mark Callow, Edgewise Consulting www.edgewise-consulting.com
*/

static void
usage(_TCHAR* appName)
{
    fprintf(stderr,
        "Usage: %s [options] [<infile> ...]\n"
        "\n"
        "  infile       The source ktx file. The output is written to a file of the\n"
        "               same name with the extension changed to '.ktx2'. If it is '-'\n"
        "               or not specified input will be read from stdin and the\n"
        "               converted texture written to stdout.\n"
        "\n"
        "  Options are:\n"
        "\n"
        "  -b, --rewritebado\n"
        "               Rewrite bad orientation metadata. Some in-the-wild KTX files\n"
        "               have orientation metadata with the key \"KTXOrientation\"\n"
        "               instead of \"KTXorientaion\". This option will rewrite such\n"
        "               bad metadata instead of dropping it.\n"
        "  -o outfile, --output=outfile\n"
        "               Name the output file outfile. If there is more than 1 input\n"
        "               file, the command prints its usage message and exits.\n"
        "  -d outdir, --output-dir=outdir\n"
        "               Writes the output files to the directory outdir. If both\n"
        "               --output and --output-dir are specified, outfile\n"
        "               will be written in outdir. If infile is stdin, the\n"
        "               command prints its usage message and exits.\n"
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
    KTX_error_code result;
    ktxTexture1* texture = 0;
    struct commandOptions options;
    int exitCode = 0;

    processCommandLine(argc, argv, options);

    for (ktx_uint32_t i = 0; i < options.numInputFiles; i++) {
        _TCHAR *infile, *outfile;

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
            size_t basenamelen;
            outfile = options.outfile;
            if (!options.useStdin && !outfile) {
                size_t outfilelen = _tcslen(infile) + 1;
                _TCHAR* extension = _tcsrchr(infile, '.');
                if ( extension == NULL) {
                    basenamelen = outfilelen;
                    outfilelen += 5;
                } else {
                    size_t extlen = _tcslen(extension);
                    basenamelen = extension - infile;
                    if (extlen < 5) {
                        outfilelen += 5 - extlen;
                    }
                }
                outfile = new _TCHAR[outfilelen];
                if (outfile) {
                    _tcsncpy(outfile, infile, basenamelen);
                    _tcscpy(&outfile[basenamelen], ".ktx2");
                }
            }

            if (!outfile) {
                outf = stdout;
#if defined(_WIN32)
                /* Set "stdout" to have binary mode */
                (void)_setmode( _fileno( stdout ), _O_BINARY );
#endif
            } else {
                outf = fopen(outfile, "wxb");
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

                // Some in-the-wild KTX 2 files have incorrect KTXOrientation
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
                writeId(writer, options.appName);
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
                    (void)unlink(outfile);
                    exitCode = 2;
                    goto cleanup;
                }
            } else {
                cerr << options.appName
                     << " could not open output file \""
                     << (outfile ? outfile : "stdout") << "\". "
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
    if (options.useStdin && options.outdir) {
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
        { "outdir", argparser::option::required_argument, NULL, 'd' },
        { "rewritebado", argparser::option::no_argument, NULL, 'b' },
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

    tstring shortopts("bfd:ho:v");
    while ((ch = parser.getopt(&shortopts, option_list, NULL)) != -1) {
        switch (ch) {
          case 0:
            break;
          case 'b':
            options.rewriteBadOrientation = true;
            break;
          case 'd':
            filename = parser.optarg.c_str();
            filenamelen = (unsigned int)_tcslen(filename) + 1;
            options.outdir = new _TCHAR[filenamelen];
            if (options.outdir) {
                _tcscpy(options.outdir, filename);
            } else {
                fprintf(stderr, "%s: out of memory.\n", options.appName);
                exit(2);
            }
            break;
         case 'f':
            options.force = true;
            break;
         case 'o':
            filename = parser.optarg.c_str();
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

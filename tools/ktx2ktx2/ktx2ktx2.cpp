// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 sts=4 expandtab:

// Copyright 2019-2020 The Khronos Group, Inc.
// SPDX-License-Identifier: Apache-2.0

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

#if defined(_MSC_VER)
  #define strncasecmp _strnicmp
  #define fileno _fileno
  #define isatty _isatty
#endif

#if IMAGE_DEBUG
static void dumpImage(char* name, int width, int height, int components,
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
    <dt>-b, \--rewritebado</dt>
    <dd>Rewrite bad orientation metadata. Some in-the-wild KTX files
        have orientation metadata with the key "KTXOrientation"
        instead of KTXorientaion. This option will rewrite such
        bad metadata instead of dropping it.
    <dt>-o outfile, --output=outfile</dt>
    <dd>Name the output file @e outfile. If @e outfile is 'stdout', output will
        be written to stdout. If there is more than 1 input file, the command
        prints its usage message and exits.</dd>
    <dt>-f, \--force</dt>
    <dd>If the destination file already exists, remove it and create a
        new file, without prompting for confirmation regardless of its
        permissions.</dd>
    </dl>
    @snippet{doc} ktxapp.h ktxApp options

@section ktx2ktx2_exitstatus EXIT STATUS
    @b ktx2ktx2 exits 0 on success, 1 on command line errors and 2 on
    functional errors.

@section ktx2ktx2_history HISTORY

@par Version 4.0
 - Initial version.

@section ktx2ktx2_author AUTHOR
    Mark Callow, Edgewise Consulting www.edgewise-consulting.com
*/


#define QUOTE(x) #x
#define STR(x) QUOTE(x)

std::string myversion(STR(KTX2KTX2_VERSION));
std::string mydefversion(STR(KTX2KTX2_DEFAULT_VERSION));

class ktxUpgrader : public ktxApp {
  public:
    ktxUpgrader();

    virtual int main(int argc, char* argv[]);
    virtual void usage();

  protected:
    virtual bool processOption(argparser& parser, int opt);
    void validateOptions();

    struct commandOptions : public ktxApp::commandOptions {
        bool         useStdout;
        bool         force;
        bool         rewriteBadOrientation;

        commandOptions() {
            useStdout = false;
            force = false;
            rewriteBadOrientation = false;
        }
    } options;
};


ktxUpgrader::ktxUpgrader() : ktxApp(myversion, mydefversion, options)
{
    argparser::option my_option_list[] = {
        { "force", argparser::option::no_argument, NULL, 'f' },
        { "outfile", argparser::option::required_argument, NULL, 'o' },
        { "rewritebado", argparser::option::no_argument, NULL, 'b' },
    };
    const int lastOptionIndex = sizeof(my_option_list)
                                / sizeof(argparser::option);
    option_list.insert(option_list.begin(), my_option_list,
                       my_option_list + lastOptionIndex);
    short_opts += "bd:fo:";
}


void
ktxUpgrader::usage()
{
    cerr <<
        "Usage: " << name << " [options] [<infile> ...]\n"
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
        "  -f, --force  If the output file already exists, remove it and create a\n"
        "               new file, without prompting for confirmation regardless of\n"
        "               its permissions.\n";
        ktxApp::usage();
}


static ktxUpgrader ktx2ktx2;
ktxApp& theApp = ktx2ktx2;


int
ktxUpgrader::main(int argc, char* argv[])
{
    FILE *inf, *outf = nullptr;
    KTX_error_code result;
    ktxTexture1* texture = 0;
    int exitCode = 0;

    processCommandLine(argc, argv);
    validateOptions();

    std::vector<string>::const_iterator it;
    for (it = options.infiles.begin(); it < options.infiles.end(); it++) {
       string infile = *it;
       string outfile;

        if (infile.compare("-") == 0) {
            inf = stdin;
#if defined(_WIN32)
            /* Set "stdin" to have binary mode */
            (void)_setmode( _fileno( stdin ), _O_BINARY );
#endif
        } else {
            inf = fopenUTF8(infile, "rb");
        }

        if (inf) {
            if (infile.compare("-")
                && !options.useStdout && !options.outfile.length())
            {
                size_t dot;

                outfile = infile;
                dot = outfile.find_last_of('.');
                if (dot !=string::npos) {
                    outfile.erase(dot,string::npos);
                }
                outfile += ".ktx2";
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
                outf = fopen_write_if_not_exists(outfile);
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
                    outf = fopenUTF8(outfile, "wb");
                }
            }

            if (outf) {
                result = ktxTexture1_CreateFromStdioStream(inf,
                                        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                        &texture);
                if (result != KTX_SUCCESS) {
                    if (result == KTX_UNKNOWN_FILE_FORMAT) {
                        cerr << infile << " is not a KTX v1 file." << endl;
                    } else if (result != KTX_SUCCESS) {
                        cerr << name
                             << " failed to create ktxTexture from " << infile
                             << ": " << ktxErrorString(result) << endl;
                    }
                    (void)fclose(inf);
                    (void)fclose(outf);
                    (void)unlinkUTF8(options.outfile.c_str());
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
                                cerr << name
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
                writeId(writer, options.test != 0);
                ktxHashList_AddKVPair(&texture->kvDataHead, KTX_WRITER_KEY,
                                      (ktx_uint32_t)writer.str().length() + 1,
                                      writer.str().c_str());

                result = ktxTexture1_WriteKTX2ToStdioStream(texture, outf);
                ktxTexture_Destroy(ktxTexture(texture));
                (void)fclose(inf);
                (void)fclose(outf);
                if (result != KTX_SUCCESS) {
                    cerr << name
                         << " failed to write KTX2 file; "
                         << ktxErrorString(result) << endl;
                    (void)unlinkUTF8(options.outfile.c_str());
                    exitCode = 2;
                    goto cleanup;
                }
            } else {
                cerr << name
                     << " could not open output file \""
                     << (options.outfile.length() ? options.outfile.c_str()
                                                  : "stdout")
                     << "\". " << strerror(errno) << endl;
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


void
ktxUpgrader::validateOptions()
{
    if (options.infiles.size() > 1 && options.outfile.length()) {
        cerr << "Can't use -o when there are multiple infiles." << endl;
        usage();
        exit(1);
    }
}


/*
 * @brief process a command line option
 *
 * @return   true if option processed, false otherwise.
 *
 * @param[in]     parser,     an @c argparser holding the options to process.
 * @param[in,out] options     commandOptions struct in which option information
 *                            is set.
 */
bool
ktxUpgrader::processOption(argparser& parser, int opt)
{
    switch (opt) {
      case 'b':
        options.rewriteBadOrientation = true;
        break;
     case 'f':
        options.force = true;
        break;
     case 'o':
        options.outfile = parser.optarg;
        if (!options.outfile.compare("stdout")) {
            options.useStdout = true;
        } else {
            size_t dot;
            dot = options.outfile.find_last_of('.');
            if (dot ==string::npos) {
                options.outfile += ".ktx2";
            }
        }
        break;
      default:
        return false;
    }
    return true;
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

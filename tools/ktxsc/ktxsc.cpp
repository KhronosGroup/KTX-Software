// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 sts=4 expandtab:

// Copyright 2019-2020 Mark Callow
// SPDX-License-Identifier: Apache-2.0

#include "scapp.h"

#include <cstdlib>
#include <errno.h>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
#include <ktx.h>

#include "argparser.h"
#include "version.h"

#if defined(_MSC_VER)
  #define strncasecmp _strnicmp
  #define fileno _fileno
  #define mktemp _mkstemp
  #define isatty _isatty
#endif

#if defined(_MSC_VER)
  #undef min
  #undef max
#endif

using namespace std;

/** @page ktxsc ktxsc
@~English

Supercompress the images in a KTX2 file.

@section ktxsc_synopsis SYNOPSIS
    ktxsc [options] [@e infile ...]

@section ktxsc_description DESCRIPTION
    @b ktxsc can encode and supercompress the images in Khronos texture
    format version 2 files (KTX2).  Uncompressed files, i.e those whose vkFormat
    name does not end in @c _BLOCK can be encoded to ASTC, Basis Universal
    (encoded to ETC1S then supercompressed with an integrated LZ step)
    or UASTC and optionally supercompressed with Zstandard (zstd). Any image
    format, except Basis Universal, can be supercompressed with zstd. For best
    results with UASTC, the data should be conditioned for zstd by using the
    @e --uastc_rdo_q and, optionally, @e --uastc_rdo_d options.

    @b ktxsc reads each named @e infile and compresses it in place. When
    @e infile is not specified, a single file will be read from @e stdin and the
    output written to @e stdout. When one or more files is specified each will
    be compressed in place.

    The following options are available:
    <dl>
    <dt>-o outfile, --output=outfile</dt>
    <dd>Write the output to @e outfile. If @e outfile is 'stdout', output will
        be written to stdout. Parent directories will be created, if
        necessary. If there is more than 1 @e infile the command prints its
        usage message and exits.</dd>
    <dt>-f, \--force</dt>
    <dd>If the destination file cannot be opened, remove it and create a
        new file, without prompting for confirmation regardless of its
        permissions.</dd>
    <dt>\--t2</dt>
    <dd>Output a KTX version2 file. Always true.</dd>
    </dl>
    @snippet{doc} scapp.h scApp options

@section ktxsc_exitstatus EXIT STATUS
    @b ktxsc exits 0 on success, 1 on command line errors and 2 on
    functional errors.

@section ktxsc_history HISTORY

@par Version 4.0
 - Initial version.

@section ktxsc_author AUTHOR
    Mark Callow, Edgewise Consulting www.edgewise-consulting.com
*/

#define QUOTE(x) #x
#define STR(x) QUOTE(x)

std::string myversion(STR(KTXSC_VERSION));
std::string mydefversion(STR(KTXSC_DEFAULT_VERSION));

class ktxSupercompressor : public scApp {
  public:
    ktxSupercompressor();

    virtual int main(int argc, char* argv[]);
    virtual void usage();

  protected:
    virtual bool processOption(argparser& parser, int opt);
    void validateOptions();

    struct commandOptions : public scApp::commandOptions {
        bool        useStdout;
        bool        force;

        commandOptions() {
            force = false;
            useStdout = false;
        }
    } options;
};

ktxSupercompressor::ktxSupercompressor() : scApp(myversion, mydefversion, options)
{
    argparser::option my_option_list[] = {
        { "force", argparser::option::no_argument, NULL, 'f' },
        { "outfile", argparser::option::required_argument, NULL, 'o' },
    };
    const int lastOptionIndex = sizeof(my_option_list)
                                / sizeof(argparser::option);
    option_list.insert(option_list.begin(), my_option_list,
                       my_option_list + lastOptionIndex);
    short_opts += "fo:";
}

void
ktxSupercompressor::usage()
{
    cerr <<
        "Usage: " << name  << " [options] [<infile> ...]\n"
        "\n"
        "  infile       The ktx2 file(s) to supercompress. The output is written to a\n"
        "               file of the same name. If infile not specified input will be read\n"
        "               from stdin and the compressed texture written to stdout.\n"
        "\n"
        "  Options are:\n"
        "\n"
        "  -o outfile, --output=outfile\n"
        "               Writes the output to outfile. If outfile is 'stdout', output\n"
        "               will be written to stdout. Parent directories will be\n"
        "               created if necessary. If there is more than 1 input file\n"
        "               the command prints its usage message and exits.\n"
        "  -f, --force  If the output file cannot be opened, remove it and create a\n"
        "               new file, without prompting for confirmation regardless of\n"
        "               its permissions.\n";
        scApp::usage();
}


static string dir_name(const string& path)
{
    // Supports both Unix-style and Windows-style.
    size_t last_separator = path.find_last_of("/\\");
    if (last_separator != string::npos) {
        return path.substr(0, last_separator + 1);
    } else {
        return std::basic_string<char>();
    }
}

static ktxSupercompressor ktxsc;
ktxApp& theApp = ktxsc;

int
ktxSupercompressor::main(int argc, char* argv[])
{
    FILE *inf, *outf = nullptr;
    KTX_error_code result;
    ktxTexture2* texture = 0;
    int exitCode = 0;
    string tmpfile;

    processCommandLine(argc, argv, eAllowStdin);
    validateOptions();

    std::vector<string>::const_iterator it;
    for (it = options.infiles.begin(); it < options.infiles.end(); it++) {
        string infile = *it;

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
            if (options.useStdout) {
                outf = stdout;
#if defined(_WIN32)
                /* Set "stdout" to have binary mode */
                (void)_setmode( _fileno( stdout ), _O_BINARY );
#endif
            } else if (options.outfile.length()) {
                const auto outputPath = filesystem::path(DecodeUTF8Path(options.outfile));
                if (outputPath.has_parent_path())
                    filesystem::create_directories(outputPath.parent_path());
                outf = fopen_write_if_not_exists(options.outfile);
            } else {
                // Make a temporary file in the same directory as the source
                // file to avoid cross-device rename issues later.
                tmpfile = dir_name(infile) + "ktxsc.tmp.XXXXXX";
#if defined(_WIN32)
                // Despite receiving size() the debug CRT version of mktemp_s
                // asserts that the string template is NUL terminated.
                tmpfile.push_back('\0');
                if (_wmktemp_s(&DecodeUTF8Path(tmpfile)[0], tmpfile.size()) == 0)
                    outf = fopenUTF8(tmpfile, "wb");
#else
                int fd_tmp = mkstemp(&tmpfile[0]);
                outf = fdopen(fd_tmp, "wb");
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
                    outf = fopenUTF8(options.outfile, "wb");
                }
            }

            if (outf) {
                result = ktxTexture2_CreateFromStdioStream(inf,
                                        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                        &texture);
                if (result == KTX_UNKNOWN_FILE_FORMAT) {
                    cerr << infile << " is not a KTX v2 file." << endl;
                    exitCode = 2;
                    goto cleanup;
                } else if (result != KTX_SUCCESS) {
                    cerr << name
                         << " failed to create ktxTexture from " << infile
                         << ": " << ktxErrorString(result) << endl;
                    exitCode = 2;
                    goto cleanup;
                }
                (void)fclose(inf);

                if (texture->classId != ktxTexture2_c) {
                    cerr << name << ": "
                         << "Only KTX texture version 2 files can be supercompressed."
                         << endl;
                    exitCode = 1;
                    goto cleanup;
                }
                if (texture->supercompressionScheme != KTX_SS_NONE) {
                    cerr << name << ": "
                         << "Cannot supercompress already supercompressed files."
                         << endl;
                    exitCode = 1;
                    goto cleanup;
                }
                if ((options.astc || options.etc1s || options.bopts.uastc) && texture->isCompressed) {
                    cerr << name << ": "
                         << "Cannot encode already block-compressed textures "
                         << "to ASTC, Basis Universal or UASTC."
                         << endl;
                    exitCode = 1;
                    goto cleanup;
                }

                // Modify the writer metadata.
                stringstream writer;
                writeId(writer, true);
                ktxHashList_DeleteKVPair(&texture->kvDataHead, KTX_WRITER_KEY);
                ktxHashList_AddKVPair(&texture->kvDataHead, KTX_WRITER_KEY,
                                      (ktx_uint32_t)writer.str().length() + 1,
                                      writer.str().c_str());

                exitCode = encode(texture, options.inputSwizzle, infile);
                if (exitCode)
                    goto cleanup;
                result = ktxTexture_WriteToStdioStream(ktxTexture(texture), outf);
                if (result != KTX_SUCCESS) {
                    cerr << name
                         << " failed to write KTX file; "
                         << ktxErrorString(result) << endl;
                    exitCode = 2;
                    goto cleanup;
                }
                (void)fclose(outf);
                if (!options.outfile.length() && !options.useStdout) {
                    // Move the new file over the original.
                    assert(tmpfile.size() > 0 && infile.length());
#if defined(_WIN32)
                    // Windows' rename() fails if the destination file exists!
                    if (!MoveFileExW(DecodeUTF8Path(tmpfile).c_str(), DecodeUTF8Path(infile).c_str(),
                                     MOVEFILE_REPLACE_EXISTING))
#else
                    if (rename(tmpfile.c_str(), infile.c_str()))
#endif
                    {
                        cerr << name
                             << ": rename of \"" << tmpfile << "\" to \""
                             << infile << "\" failed: "
                             << strerror(errno) << endl;
                        exitCode = 2;
                        goto cleanup;
                    }
                }
            } else {
                cerr << name
                     << " could not open output file \""
                     << (options.useStdout ? "stdout" : options.outfile) << "\". "
                     << strerror(errno) << endl;
                exitCode = 2;
                goto cleanup;
            }
        } else {
            cerr << name
                 << " could not open input file \""
                 << (infile.compare("-") == 0 ? "stdin" : infile) << "\". "
                 << strerror(errno) << endl;
            exitCode = 2;
            goto cleanup;
        }
    }
    return 0;

cleanup:
    if (outf) { // Windows debug CRT fclose asserts that outf != nullptr...
        (void)fclose(outf); // N.B Windows refuses to unlink an open file.
    }
    if (tmpfile.size() > 0) (void)unlinkUTF8(tmpfile);
    if (options.outfile.length()) (void)unlinkUTF8(options.outfile);
    return exitCode;
}


void
ktxSupercompressor::validateOptions()
{
    scApp::validateOptions();

    if (options.infiles.size() > 1 && options.outfile.length()) {
        cerr << "Can't use -o when there are multiple infiles." << endl;
        usage();
        exit(1);
    }
    if (options.etc1s && options.zcmp) {
        cerr << "Can't encode to etc1s and supercompress with zstd." << endl;
        usage();
        exit(1);
    }
    if (!options.astc && !options.etc1s && !options.zcmp && !options.bopts.uastc) {
       cerr << "Must specify one of --zcmp, --etc1s (deprecated --bcmp) or --uastc." << endl;
       usage();
       exit(1);
    }
}

/*
 * @brief process a command line option
 *
 * @return true of option processed.
 *
 * @param[in]     parser,     an @c argparser holding the options to process.
 */
bool
ktxSupercompressor::processOption(argparser& parser, int opt)
{
    switch (opt) {
      case 'f':
        options.force = true;
        break;
      case 'o':
        options.outfile = parser.optarg;
        if (!options.outfile.compare("stdout")) {
            options.useStdout = true;
        } else {
            size_t dot;
            size_t slash;
            dot = options.outfile.find_last_of('.');
            slash = options.outfile.find_last_of('/');
            if (slash == string::npos) {
                slash = options.outfile.find_last_of('\\');
            }
            // dot < slash means there's a dot but it is not prefixing
            // a file extension.
            if (dot == string::npos
                || (slash != string::npos && dot < slash)) {
                options.outfile += ".ktx2";
            }
        }
        break;
      default:
        return scApp::processOption(parser, opt);
    }
    return true;
}

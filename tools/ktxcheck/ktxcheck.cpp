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
#include <stdexcept>
#include <thread>
#include <vector>

#include <ktx.h>

#include <KHR/khr_df.h>

#include "ktxapp.h"
#include "ktxint.h"
#include "vkformat_enum.h"

#if defined(_MSC_VER)
  #define strncasecmp _strnicmp
  #define fileno _fileno
  #define mktemp _mkstemp
  #define isatty _isatty
  #define unlink _unlink
#endif

#define VERSION "1.0.0"
std::string myversion(VERSION);

#if defined(_MSC_VER)
  #undef min
  #undef max
#endif

/////////////////////////////////////////////////////////////////////
//                       Message Definitions                       //
/////////////////////////////////////////////////////////////////////

struct issue {
    uint32_t code;
    const std::string message;
};

#define WARNING 0x00010000
#define ERROR 0x00100000
#define FATAL 0x01000000

namespace IOError {
    issue FileOpen { FATAL | 0x0001, "File open failed: %s." };
    issue FileRead { FATAL | 0x0002, "File read failed: %s." };
    issue UnexpectedEOF { FATAL | 0x0003, "Unexpected end of file." };
};

namespace FileError {
    issue NotKTX2{ FATAL | 0x0010, "Not a KTX2 file." };
}

namespace HeaderData {
    issue ProhibitedFormat {
        ERROR | 0x0020, "vkFormat is one of the prohibited formats."
    };
    issue InvalidFormat {
        ERROR | 0x0021, "vkFormat, %#x, is not a valid VkFormat value."
    };
    issue WidthZero {
        ERROR | 0x0022,
       "pixelWidth is 0. Textures must have width."
    };
    issue DepthNoHeight {
        ERROR | 0x0023,
        "pixelDepth != 0 but pixelHeight == 0. Depth textures must have height."
    };
    issue ThreeDArray {
        WARNING | 0x0024,
        "File contains a 3D array texture. No APIs support these."
    };
    issue CubeFaceNot2d { ERROR | 0x0025, "Cube map faces must be 2d." };
    issue InvalidFaceCount {
        ERROR | 0x0026, "faceCount is %d. It must be 1 or 6."
    };
    issue TooManyMipLevels {
        ERROR | 0x0027, "%d is too many levels for the largest image dimension %d."
    };
    issue UnknownSupercompression {
        WARNING | 0x0028, "Unknown vendor supercompressionScheme."
    };
    issue InvalidSupercompression {
        ERROR | 0x0029, "Invalid supercompressionScheme: %#x"
    };
    issue InvalidIndexEntry {
        ERROR | 0x002a, "Invalid %s index entry. Only 1 of offset & length != 0."
    };
}


/////////////////////////////////////////////////////////////////////
//                    External Functions                           //
//     These are in libktx but not part of its public API.         //
/////////////////////////////////////////////////////////////////////

extern "C" bool isProhibitedFormat(VkFormat format);

/////////////////////////////////////////////////////////////////////
//                    Validator Class Definition                   //
/////////////////////////////////////////////////////////////////////

using namespace std;

class fatal : public runtime_error {
  public:
    fatal()
         : runtime_error("Aborting validation.") { }
};

class ktxValidator : public ktxApp {
  public:
    ktxValidator();

    virtual int main(int argc, _TCHAR* argv[]);
    virtual void usage();

  protected:
    class logger {
      public:
        logger() {
            maxIssues = -1U;
            errorCount = 0;
            headerWritten = false;
        }
        enum severity { eWarning, eError, eFatal };
        void addIssue(severity severity, issue issue, ...);
        void startFile(const std::string& filename) {
            nameOfFileBeingValidated = filename;
            errorCount = 0;
            headerWritten = false;
        }
        uint32_t maxIssues;

      protected:
        uint32_t errorCount;
        bool headerWritten;
        std::string nameOfFileBeingValidated;
    } logger;

    void addIssue(logger::severity severity, issue issue, ...) {
        va_list args;
        va_start(args, issue);

        logger.addIssue(severity, issue, args);
    }
    virtual void processOption(argparser& parser, _TCHAR opt);
    void validateHeader(FILE* f);

    struct commandOptions : public ktxApp::commandOptions {
        uint32_t maxIssues;
        bool quiet;

        commandOptions() {
            maxIssues = -1U;
            quiet = false;
        }
    } options;
};

ktxValidator::ktxValidator() : ktxApp(myversion, options)
{
    argparser::option my_option_list[] = {
        { "quiet", argparser::option::no_argument, NULL, 'q' },
        { "max-issues", argparser::option::required_argument, NULL, 'm' }
    };

    option_list.insert(option_list.begin(), my_option_list, my_option_list+1);
    short_opts += "qm:";
}

// Why is severity passed here?
// -  Because it is convenient when browsing the code to see the severity
//    at the place an issue is raised.
void
ktxValidator::logger::addIssue(severity severity, issue issue, ...) {
    if (!headerWritten) {
        cout << "Issues in: " << nameOfFileBeingValidated << std::endl;
        headerWritten = true;
    }
    if (errorCount < maxIssues) {
        va_list args;
        va_start(args, issue);

        cout << "    ";
        switch (severity) {
          case eError:
            cerr << "ERROR: ";
            break;
          case eFatal:
            cerr << "FATAL: ";
            break;
          case eWarning:
            cerr << "WARNING: ";
            break;
        }
        vfprintf(stdout, issue.message.c_str(), args);
        va_end(args);
        cout << std::endl;
        errorCount++;
        if (severity == eFatal)
            throw fatal();
    } else {
        cout << "Max errors exceeded. Aborting validation." << std::endl;
    }
}


void
ktxValidator::usage()
{
    cerr <<
        "Usage: " << name << " [options] [<infile> ...]\n"
        "\n"
        "  infile       The ktx2 file(s) to validate. If infile not specified input\n"
        "               will be read from stdin.\n"
        "\n"
        "  Options are:\n"
        "\n";
    ktxApp::usage();
}


int _tmain(int argc, _TCHAR* argv[])
{

    ktxValidator ktxcheck;

    return ktxcheck.main(argc, argv);
}

int
ktxValidator::main(int argc, _TCHAR *argv[])
{
    processCommandLine(argc, argv, allow_stdin);

    vector<_tstring>::const_iterator it;
    for (it = options.infiles.begin(); it < options.infiles.end(); it++) {
        FILE* inf;

        logger.startFile(*it);
        if (it->compare(_T("-")) == 0) {
            inf = stdin;
#if defined(_WIN32)
            /* Set "stdin" to have binary mode */
            (void)_setmode( _fileno( stdin ), _O_BINARY );
#endif
        } else {
            inf = _tfopen(it->c_str(), "rb");
        }

        logger.startFile(inf == stdin ? "stdin" : *it);
        if (inf) {
            try {
                validateHeader(inf);
            } catch (fatal& e) {
                cout << e.what() << endl;
            }
            fclose(inf);
        } else {
            addIssue(logger::eFatal, IOError::FileOpen, strerror(errno));
        }
    }

    return 0;
}

void
ktxValidator::processOption(argparser& parser, _TCHAR opt)
{
    switch (opt) {
      case 'q':
        options.quiet = true;
        break;
      case 'm':
        options.maxIssues = atoi(parser.optarg.c_str());
        break;
      default:
        usage();
    }
}

void
ktxValidator::validateHeader(FILE* f)
{
    KTX_header2 header2;
    ktx_uint8_t identifier_reference[12] = KTX2_IDENTIFIER_REF;
    ktx_uint32_t dimensionCount, levelCount;
    ktx_uint32_t max_dim;

    if (fread(&header2, sizeof(KTX_header2), 1, f) != 1) {
        if (ferror(f))
            addIssue(logger::eFatal, IOError::FileRead, strerror(errno));
        else
            addIssue(logger::eFatal, IOError::UnexpectedEOF);
    }
    // Is this a KTX2 file?
    if (memcmp(&header2.identifier, identifier_reference, 12) != 0) {
        addIssue(logger::eFatal, FileError::NotKTX2);
    }

    if (isProhibitedFormat((VkFormat)header2.vkFormat))
        addIssue(logger::eError, HeaderData::ProhibitedFormat);

    /* Check texture dimensions. KTX files can store 8 types of textures:
       1D, 2D, 3D, cube, and array variants of these. There is currently
       no extension for 3D array textures in any 3D API. */
    if (header2.pixelWidth == 0)
        addIssue(logger::eError, HeaderData::WidthZero);

    if (header2.pixelDepth > 0 && header2.pixelHeight == 0)
        addIssue(logger::eError, HeaderData::DepthNoHeight);

    if (header2.pixelDepth > 0)
    {
        if (header2.layerCount > 0) {
            /* No 3D array textures yet. */
            addIssue(logger::eWarning, HeaderData::ThreeDArray);
        } else
            dimensionCount = 3;
    }
    else if (header2.pixelHeight > 0)
    {
        dimensionCount = 2;
    }
    else
    {
        dimensionCount = 1;
    }

    if (header2.faceCount == 6)
    {
        if (dimensionCount != 2)
        {
            /* cube map needs 2D faces */
            addIssue(logger::eError, HeaderData::CubeFaceNot2d);
        }
    }
    else if (header2.faceCount != 1)
    {
        /* numberOfFaces must be either 1 or 6 */
        addIssue(logger::eError, HeaderData::InvalidFaceCount,
                 header2.faceCount);
    }

    // Check number of mipmap levels
    levelCount = MAX(header2.levelCount, 1);

    // This test works for arrays too because height or depth will be 0.
    max_dim = MAX(MAX(header2.pixelWidth, header2.pixelHeight), header2.pixelDepth);
    if (max_dim < ((ktx_uint32_t)1 << (header2.levelCount - 1)))
    {
        // Can't have more mip levels than 1 + log2(max(width, height, depth))
        addIssue(logger::eError, HeaderData::TooManyMipLevels,
                 levelCount, max_dim);
    }

    if (header2.supercompressionScheme > KTX_SUPERCOMPRESSION_BEGIN_VENDOR_RANGE
        && header2.supercompressionScheme < KTX_SUPERCOMPRESSION_END_VENDOR_RANGE)
    {
        addIssue(logger::eWarning, HeaderData::UnknownSupercompression);
    } else if (header2.supercompressionScheme < KTX_SUPERCOMPRESSION_BEGIN_RANGE
        || header2.supercompressionScheme > KTX_SUPERCOMPRESSION_END_RANGE)
    {
        addIssue(logger::eError, HeaderData::InvalidSupercompression,
                 header2.supercompressionScheme);
    }

#define checkIndexEntry(index, issue, name)     \
    if (!index.byteOffset == !index.byteLength) \
        addIssue(logger::eError, issue, name)

    checkIndexEntry(header2.dataFormatDescriptor,
                    HeaderData::InvalidIndexEntry, "dfd");

    checkIndexEntry(header2.keyValueData,
                    HeaderData::InvalidIndexEntry, "kvd");

    checkIndexEntry(header2.supercompressionGlobalData,
                    HeaderData::InvalidIndexEntry, "sgd");
}



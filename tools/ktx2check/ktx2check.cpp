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
#include <math.h>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

#include <ktx.h>

#include <KHR/khr_df.h>

#include "ktxapp.h"
#include "ktxint.h"
#include "vkformat_enum.h"
#define LIBKTX // To stop dfdutils including vulkan_core.h.
#include "dfdutils/dfd.h"
#include "texture.h"
#include "basis_sgd.h"
#if defined(_MSC_VER)
  #define strncasecmp _strnicmp
#endif

#define VERSION "1.0.0 alpha"
std::string myversion(VERSION);

/** @page ktx2check ktx2check
@~English

Check the validity of a KTX 2 file.

@section ktx2check_synopsis SYNOPSIS
    ktx2check [options] [@e infile ...]

@section ktx2check_description DESCRIPTION
    @b ktx2check validates Khronos texture format version 2 files (KTX2).
    It reads each named @e infile and validates it writing to stdout messages
    about any issues found. When @b infile is not specified, it validates a
    single file from stdin.

    The following options are available:
    <dl>
    <dt>-q, --quiet</dt>
    <dd>Validate silently. Indicate valid or invalid via exit code.</dd>
    <dt>-m &lt;num%gt;, --max-issues &lt;num&gt;</dt>
    <dd>Set the maximum number of issues to be reported per file"
        provided -q is not set.</dd>

@section ktx2check_exitstatus EXIT STATUS
    @b toktx exits 0 on success, 1 on command line errors and 2 on
    validation errors.

@section ktx2check_history HISTORY

@version 1.0.alpha:
Sun, 08 Sep 2019 13:14:28 -0700
 - Initial version.

@section ktx2check_author AUTHOR
    Mark Callow, Edgewise Consulting www.edgewise-consulting.com
*/


/////////////////////////////////////////////////////////////////////
//                       Message Definitions                       //
/////////////////////////////////////////////////////////////////////

struct issue {
    uint32_t code;
    const string message;
};

#define WARNING 0x00010000
#define ERROR 0x00100000
#define FATAL 0x01000000

struct {
    issue FileOpen {
        FATAL | 0x0001, "File open failed: %s."
    };
    issue FileRead {
        FATAL | 0x0002, "File read failed: %s."
    };
    issue UnexpectedEOF {
        FATAL | 0x0003, "Unexpected end of file."
    };
    issue RewindFailure {
        FATAL | 0x0004, "Seek to start of file failed: %s."
    };
} IOError;

struct {
    issue NotKTX2 {
        FATAL | 0x0010, "Not a KTX2 file."
    };
    issue CreateFailure {
        FATAL | 0x0011, "ktxTexture2 creation failed: %s."
    };
} FileError;

struct {
    issue ProhibitedFormat {
        ERROR | 0x0020, "vkFormat is one of the prohibited formats."
    };
    issue InvalidFormat {
        ERROR | 0x0021, "vkFormat, %#x, is not a valid VkFormat value."
    };
    issue UnknownFormat {
        WARNING | 0x0022, "vkFormat, %#x is unknown, possibly an extension format."
    };
    issue WidthZero {
        ERROR | 0x0023, "pixelWidth is 0. Textures must have width."
    };
    issue DepthNoHeight {
        ERROR | 0x0024, "pixelDepth != 0 but pixelHeight == 0. Depth textures must have height."
    };
    issue ThreeDArray {
        WARNING| 0x0025, "File contains a 3D array texture. No APIs support these."
    };
    issue CubeFaceNot2d {
        ERROR | 0x0026, "Cube map faces must be 2d."
    };
    issue InvalidFaceCount {
        ERROR | 0x0027, "faceCount is %d. It must be 1 or 6."
    };
    issue TooManyMipLevels {
        ERROR | 0x0028, "%d is too many levels for the largest image dimension %d."
    };
    issue VendorSupercompression {
        WARNING | 0x0029, "Using vendor supercompressionScheme. Can't validate."
    };
    issue InvalidSupercompression {
        ERROR | 0x002a, "Invalid supercompressionScheme: %#x"
    };
    issue InvalidOptionalIndexEntry {
        ERROR | 0x002b, "Invalid %s index entry. Only 1 of offset & length != 0."
    };
    issue InvalidRequiredIndexEntry {
        ERROR | 0x002c, "Index for required entry has offset or length == 0."
    };
    issue InvalidDFDOffset {
        ERROR | 0x002d, "Invalid dfdByteOffset. DFD must immediately follow level index."
    };
    issue InvalidKVDOffset {
        ERROR | 0x002e, "Invalid kvdByteOffset. KVD must immediately follow DFD."
    };
    issue InvalidSGDOffset {
        ERROR | 0x002f, "Invalid sgdByteOffset. SGD must follow KVD."
    };
    issue TypeSizeMismatch {
        ERROR | 0x0030, "typeSize, %d, does not match data described by the DFD."
    };
    issue TypeSizeNotOne {
        ERROR | 0x0031, "typeSize for a block compressed or supercompressed format must be 1."
  };
} HeaderData;

struct {
    issue CreateDfdFailure {
        FATAL | 0x0040, "Creation of DFD matching %s failed."
    };
    issue IncorrectDfd {
        FATAL | 0x0041, "DFD created for %s confused interpretDFD()."
    };
    issue OnlyBasisSupported {
        FATAL | 0x0042, "Validator can only currently validate Basis supercompression."
    };
} ValidatorError;

struct {
    issue InvalidTransferFunction {
        ERROR | 0x0050, "Transfer function is not KHR_DF_TRANSFER_LINEAR or KHR_DF_TRANSFER_SRGB"
    };
    issue IncorrectBasics {
        ERROR | 0x0051, "DFD format is not the correct type or version."
    };
    issue IncorrectModelForBlock {
        ERROR | 0x0052, "DFD color model is not that of a block-compressed texture."
    };
    issue TooComplex {
        ERROR | 0x0053, "DFD is too complex for analysis. Does not describe a VkFormat or is wrong endianness."
    };
    issue sRGBMismatch {
        ERROR | 0x0054, "DFD says sRGB but vkFormat is not an sRGB format."
    };
    issue UnsignedFloat {
        ERROR | 0x0055, "DFD says data is unsigned float but there are no such texture formats."
    };
    issue FormatMismatch {
        ERROR | 0x0056, "DFD does not match VK_FORMAT w.r.t. sign, float or normalization."
    };
    issue NonZeroSamplesForBasis {
        ERROR | 0x0057, "DFD for a Basis Compressed texture must have 0 samples."
    };
    issue IncorrectModelForBasis {
        ERROR | 0x0058, "DFD color model for a Basis Compressed texture must be KHR_DF_MODEL_UNSPECIFIED."
    };
    issue IncorrectSizesForBasis {
        ERROR | 0x0059, "DFD texel block dimensions and bytes/plane for Basis must be 0."
    };
} DFD;

struct {
    issue IncorrectByteLength {
        ERROR | 0x0060, "Level %d byteLength or uncompressedByteLength does not match expected value."
    };
    issue IncorrectByteOffset {
        ERROR | 0x0061, "Level %d byteOffset does not match expected value."
    };
    issue ZeroOffsetOrLength {
        ERROR | 0x0062, "Level %d's byteOffset or byteLength is 0."
    };
    issue ZeroUncompressedLength {
        ERROR | 0x0063, "Level %d's uncompressedByteLength is 0."
    };
} LevelIndex;

struct {
    issue OutOfOrder {
        ERROR | 0x0070, "Metadata keys are not sorted in codepoint order."
    };
    issue CustomMetadata {
        WARNING | 0x0071, "Custom metadata \"%s\" found."
    };
    issue IllegalMetadata {
        ERROR | 0x0072, "Unrecognized metadata \"%s\" found with KTX or ktx prefix found."
    };
    issue ValueNotNulTerminated {
        ERROR | 0x0073, "%s value missing required NUL termination."
    };
    issue InvalidValue {
        ERROR | 0x0074, "%s has invalid value."
    };
    issue NoKTXwriter {
        ERROR | 0x0075, "Required KTXwriter key is missing."
    };
} Metadata;

struct {
    issue UnexpectedSupercompressionGlobalData {
        ERROR | 0x0080, "Supercompression global data found scheme that is not Basis."
    };
    issue MissingSupercompressionGlobalData {
        ERROR | 0x0081, "Basis supercompression global data missing."
    };
    issue IncorrectGlobalDataSize {
        ERROR | 0x0082, "Basis supercompression global data has incorrect size."
    };
    issue ExtendedByteLengthNotZero {
        ERROR | 0x0083, "extendedByteLength != 0 in Basis supercompression global data."
    };
} SGD;

struct {
    issue OutOfMemory {
        ERROR | 0x0080, "System out of memory."
    };
} System;

/////////////////////////////////////////////////////////////////////
//                       External Functions                        //
//     These are in libktx but not part of its public API.         //
/////////////////////////////////////////////////////////////////////

extern "C" {
    bool isProhibitedFormat(VkFormat format);
    bool isValidFormat(VkFormat format);
    char* vkFormatString(VkFormat format);
}

/////////////////////////////////////////////////////////////////////
//                      Define Useful Exceptions                   //
/////////////////////////////////////////////////////////////////////

using namespace std;

class fatal : public runtime_error {
  public:
    fatal()
        : runtime_error("Aborting validation.") { }
};

class max_issues_exceeded : public runtime_error {
  public:
    max_issues_exceeded()
        : runtime_error("Max issues exceeded. Stopping validation.") { }
};

class validation_failed : public runtime_error {
  public:
    validation_failed()
        : runtime_error("One or more files failed validation.") { }
};

/////////////////////////////////////////////////////////////////////
//                    Validator Class Definition                   //
/////////////////////////////////////////////////////////////////////

class ktxValidator : public ktxApp {
  public:
    ktxValidator();

    virtual int main(int argc, _TCHAR* argv[]);
    virtual void usage();

  protected:
    class logger {
      public:
        logger() {
            maxIssues = 0xffffffffU;
            issueCount = 0;
            headerWritten = false;
            quiet = false;
        }
        enum severity { eWarning, eError, eFatal };
        void addIssue(severity severity, issue issue, va_list args);
        void startFile(const std::string& filename) {
            nameOfFileBeingValidated = filename;
            issueCount = 0;
            headerWritten = false;
        }
        uint32_t getIssueCount() { return this->issueCount; }
        uint32_t maxIssues;
        bool quiet;

      protected:
        uint32_t issueCount;
        bool headerWritten;
        string nameOfFileBeingValidated;
    } logger;

    struct validationContext {
        FILE* inf;
        KTX_header2 header;
        size_t levelIndexSize;
        uint32_t layerCount;
        uint32_t levelCount;
        uint32_t dimensionCount;
        uint32_t* pDfd4Format;
        struct formatInfo {
            struct {
                uint32_t x;
                uint32_t y;
                uint32_t z;
            } blockDimension;
            uint32_t wordSize;
            uint32_t blockByteLength;
            bool isBlockCompressed;
        } formatInfo;

        validationContext() {
            inf = nullptr;
            pDfd4Format = nullptr;
        }

        ~validationContext() {
            if (pDfd4Format != nullptr) delete pDfd4Format;
        }

        size_t calcImageSize(uint32_t level) {
            struct blockCount {
                uint32_t x, y;
            } blockCount;

            float levelWidth  = (float)(header.pixelWidth >> level);
            float levelHeight = (float)(header.pixelHeight >> level);
            // Round up to next whole block.
            blockCount.x
				= (uint32_t)ceilf(levelWidth / formatInfo.blockDimension.x);
            blockCount.y
				= (uint32_t)ceilf(levelHeight / formatInfo.blockDimension.y);
            blockCount.x = MAX(1, blockCount.x);
            blockCount.y = MAX(1, blockCount.y);

            return blockCount.x * blockCount.y * formatInfo.blockByteLength;
        }

        size_t calcLayerSize(uint32_t level) {
            /*
             * As there are no 3D cubemaps, the image's z block count will always be
             * 1 for cubemaps and numFaces will always be 1 for 3D textures so the
             * multiply is safe. 3D cubemaps, if they existed, would require
             * imageSize * (blockCount.z + This->numFaces);
             */
            uint32_t blockCountZ;
            size_t imageSize, layerSize;

            float levelDepth = (float)(header.pixelDepth >> level);
            blockCountZ
				= (uint32_t)ceilf(levelDepth / formatInfo.blockDimension.z);
            blockCountZ = MAX(1, blockCountZ);
            imageSize = calcImageSize(level);
            layerSize = imageSize * blockCountZ;
            return layerSize * header.faceCount;
        }

        size_t calcLevelOffset(uint32_t level) {
            assert (level < levelCount);
            size_t levelOffset;
            // Calculate the expected base offset in the file
            levelOffset = sizeof(KTX_header2) + levelIndexSize
                          + header.dataFormatDescriptor.byteLength
                          + header.keyValueData.byteLength;
            levelOffset = _KTX_PAD8(levelOffset);
            for (uint32_t i = levelCount - 1; i > level; i--) {
                size_t levelSize;
                levelSize = calcLevelSize(i);
                levelOffset += _KTX_PAD8(levelSize);
            }
            return levelOffset;
        }

        size_t calcLevelSize(uint32_t level)
        {
            return calcLayerSize(level) * layerCount;
        }

        bool extractFormatInfo(uint32_t* pDfd4Format) {
            this->pDfd4Format = pDfd4Format;
            uint32_t* bdb = pDfd4Format + 1;
            struct formatInfo& fi = formatInfo;
            fi.blockDimension.x = KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) + 1;
            fi.blockDimension.y = KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) + 1;
            fi.blockDimension.z = KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION2) + 1;
            fi.blockByteLength = KHR_DFDVAL(bdb, BYTESPLANE0);
            if (KHR_DFDVAL(bdb, MODEL) >= KHR_DF_MODEL_DXT1A) {
                // A block compressed format. Entire block is a single sample.
                fi.isBlockCompressed = true;
            } else {
                // An uncompressed format.
                InterpretedDFDChannel r, g, b, a;
                InterpretDFDResult result;

                fi.isBlockCompressed = false;
                result = interpretDFD(pDfd4Format, &r, &g, &b, &a, &fi.wordSize);
                if (result > i_UNSUPPORTED_ERROR_BIT)
                    return false;
            }
            return true;
        }

        void init (FILE* f) {
            if (pDfd4Format != nullptr) delete pDfd4Format;
            inf = f;
        }
    };

    void addIssue(logger::severity severity, issue issue, ...) {
        va_list args;
        va_start(args, issue);

        logger.addIssue(severity, issue, args);
        va_end(args);
    }
    virtual void processOption(argparser& parser, _TCHAR opt);
    int validateFile(const string&);
    void validateHeader(validationContext& ctx);
    void validateLevelIndex(validationContext& ctx);
    void validateDfd(validationContext& ctx);
    void validateKvd(validationContext& ctx);
    void validateSgd(validationContext& ctx);
    bool validateMetadata(validationContext& ctx, char* key, uint8_t* value,
                          uint32_t valueLen);

    typedef void (ktxValidator::*validateMetadataFunc)(validationContext& ctx,
                                                       char* key,
                                                       uint8_t* value,
                                                       uint32_t valueLen);
    void validateCubemapIncomplete(validationContext& ctx, char* key,
                                   uint8_t* value, uint32_t valueLen);
    void validateOrientation(validationContext& ctx, char* key,
                             uint8_t* value, uint32_t valueLen);
    void validateGlFormat(validationContext& ctx, char* key,
                          uint8_t* value, uint32_t valueLen);
    void validateDxgiFormat(validationContext& ctx, char* key,
                            uint8_t* value, uint32_t valueLen);
    void validateMetalPixelFormat(validationContext& ctx, char* key,
                                  uint8_t* value, uint32_t valueLen);
    void validateSwizzle(validationContext& ctx, char* key,
                        uint8_t* value, uint32_t valueLen);
    void validateWriter(validationContext& ctx, char* key,
                        uint8_t* value, uint32_t valueLen);
    void validateAstcDecodeRGB9E5(validationContext& ctx, char* key,
                                  uint8_t* value, uint32_t valueLen);

    typedef struct {
        string name;
        validateMetadataFunc validateFunc;
    } metadataValidator;
    static vector<metadataValidator> metadataValidators;

    struct commandOptions : public ktxApp::commandOptions {
        uint32_t maxIssues;
        bool quiet;

        commandOptions() {
            maxIssues = 0xffffffffU;
            quiet = false;
        }
    } options;
};

vector<ktxValidator::metadataValidator> ktxValidator::metadataValidators {
    { "KTXcubemapIncomplete", &ktxValidator::validateCubemapIncomplete },
    { "KTXorientation", &ktxValidator::validateOrientation },
    { "KTXglFormat", &ktxValidator::validateGlFormat },
    { "KTXdxgiFormat__", &ktxValidator::validateDxgiFormat },
    { "KTXMetalPixelFormat", &ktxValidator::validateMetalPixelFormat },
    { "KTXswizzle", &ktxValidator::validateSwizzle },
    { "KTXwriter", &ktxValidator::validateWriter },
    { "KTXastcDecodeRGB9E5", &ktxValidator::validateAstcDecodeRGB9E5 }
};

/////////////////////////////////////////////////////////////////////
//                     Validator Implementation                    //
/////////////////////////////////////////////////////////////////////

ktxValidator::ktxValidator() : ktxApp(myversion, options)
{
    argparser::option my_option_list[] = {
        { "quiet", argparser::option::no_argument, NULL, 'q' },
        { "max-issues", argparser::option::required_argument, NULL, 'm' }
    };
    const int lastOptionIndex = sizeof(my_option_list)
                                / sizeof(argparser::option);
    option_list.insert(option_list.begin(), my_option_list,
                       my_option_list + lastOptionIndex);
    short_opts += "qm:";
}

// Why is severity passed here?
// -  Because it is convenient when browsing the code to see the severity
//    at the place an issue is raised.
void
ktxValidator::logger::addIssue(severity severity, issue issue, va_list args) {
    if (!quiet) {
        if (!headerWritten) {
            cout << "Issues in: " << nameOfFileBeingValidated << std::endl;
            headerWritten = true;
        }
        if (issueCount < maxIssues) {
            cout << "    ";
            switch (severity) {
              case eError:
                cout << "ERROR: ";
                break;
              case eFatal:
                cout << "FATAL: ";
                break;
              case eWarning:
                cout << "WARNING: ";
                break;
            }
            vfprintf(stdout, issue.message.c_str(), args);
            cout << std::endl;
        } else {
            throw max_issues_exceeded();
        }
    }
    issueCount++;
    if (severity == eFatal)
        throw fatal();
}


void
ktxValidator::usage()
{
    cerr <<
        "Usage: " << name << " [options] [<infile> ...]\n"
        "\n"
        "  infile       The ktx2 file(s) to validate. If infile is not specified, input\n"
        "               will be read from stdin.\n"
        "\n"
        "  Options are:\n"
        "\n"
        "  -q, --quiet  Validate silently. Indicate valid or invalid via exit code.\n"
        "  -m <num>, --max-issues <num>\n"
        "               Set the maximum number of issues to be reported per file\n"
        "               provided -q is not set.\n";
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
    processCommandLine(argc, argv, eAllowStdin);

    uint32_t totalIssues = 0;

    logger.quiet = options.quiet;
    logger.maxIssues = options.maxIssues;

    vector<_tstring>::const_iterator it;
    for (it = options.infiles.begin(); it < options.infiles.end(); it++) {
        try {
            totalIssues += validateFile(*it);
        } catch (fatal&) {
            // File could not be opened.
            totalIssues++;
        }
    }
    if (totalIssues > 0)
        return 2;
    else
        return 0;
}

int
ktxValidator::validateFile(const string& filename)
{
    FILE* inf;
    validationContext context;

    if (filename.compare(_T("-")) == 0) {
        inf = stdin;
#if defined(_WIN32)
        /* Set "stdin" to have binary mode */
        (void)_setmode( _fileno( stdin ), _O_BINARY );
#endif
    } else {
        inf = _tfopen(filename.c_str(), "rb");
    }

    logger.startFile(inf == stdin ? "stdin" : filename);
    if (inf) {
        try {
            context.init(inf);
            validateHeader(context);
            validateLevelIndex(context);
            validateDfd(context);
            validateKvd(context);
            validateSgd(context);
        } catch (fatal& e) {
            if (!options.quiet)
                cout << "    " << e.what() << endl;
            throw;
        } catch (max_issues_exceeded& e) {
            cout << e.what() << endl;
        }
        fclose(inf);
    } else {
        addIssue(logger::eFatal, IOError.FileOpen, strerror(errno));
    }
    return logger.getIssueCount();
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
        exit(1);
    }
}

void
ktxValidator::validateHeader(validationContext& ctx)
{
    ktx_uint8_t identifier_reference[12] = KTX2_IDENTIFIER_REF;
    ktx_uint32_t max_dim;

    if (fread(&ctx.header, sizeof(KTX_header2), 1, ctx.inf) != 1) {
        if (ferror(ctx.inf))
            addIssue(logger::eFatal, IOError.FileRead, strerror(errno));
        else
            addIssue(logger::eFatal, IOError.UnexpectedEOF);
    }
    // Is this a KTX2 file?
    if (memcmp(&ctx.header.identifier, identifier_reference, 12) != 0) {
        addIssue(logger::eFatal, FileError.NotKTX2);
    }

    if (isProhibitedFormat((VkFormat)ctx.header.vkFormat))
        addIssue(logger::eError, HeaderData.ProhibitedFormat);

    if (!isValidFormat((VkFormat)ctx.header.vkFormat)) {
        if (ctx.header.vkFormat < VK_FORMAT_END_RANGE || ctx.header.vkFormat > 0x10010000)
            addIssue(logger::eError, HeaderData.InvalidFormat, ctx.header.vkFormat);
        else
            addIssue(logger::eError, HeaderData.UnknownFormat, ctx.header.vkFormat);
    }

    /* Check texture dimensions. KTX files can store 8 types of textures:
       1D, 2D, 3D, cube, and array variants of these. There is currently
       no extension for 3D array textures in any 3D API. */
    if (ctx.header.pixelWidth == 0)
        addIssue(logger::eError, HeaderData.WidthZero);

    if (ctx.header.pixelDepth > 0 && ctx.header.pixelHeight == 0)
        addIssue(logger::eError, HeaderData.DepthNoHeight);

    if (ctx.header.pixelDepth > 0)
    {
        if (ctx.header.layerCount > 0) {
            /* No 3D array textures yet. */
            addIssue(logger::eWarning, HeaderData.ThreeDArray);
        } else
            ctx.dimensionCount = 3;
    }
    else if (ctx.header.pixelHeight > 0)
    {
        ctx.dimensionCount = 2;
    }
    else
    {
        ctx.dimensionCount = 1;
    }

    if (ctx.header.faceCount == 6)
    {
        if (ctx.dimensionCount != 2)
        {
            /* cube map needs 2D faces */
            addIssue(logger::eError, HeaderData.CubeFaceNot2d);
        }
    }
    else if (ctx.header.faceCount != 1)
    {
        /* numberOfFaces must be either 1 or 6 */
        addIssue(logger::eError, HeaderData.InvalidFaceCount,
                 ctx.header.faceCount);
    }

    // Check number of mipmap levels
    ctx.levelCount = MAX(ctx.header.levelCount, 1);

    // This test works for arrays too because height or depth will be 0.
    max_dim = MAX(MAX(ctx.header.pixelWidth, ctx.header.pixelHeight), ctx.header.pixelDepth);
    if (max_dim < ((ktx_uint32_t)1 << (ctx.header.levelCount - 1)))
    {
        // Can't have more mip levels than 1 + log2(max(width, height, depth))
        addIssue(logger::eError, HeaderData.TooManyMipLevels,
                 ctx.levelCount, max_dim);
    }

    // Set layerCount to actual number of layers.
    ctx.layerCount = MAX(ctx.header.layerCount, 1);

    if (ctx.header.vkFormat != VK_FORMAT_UNDEFINED) {
        uint32_t* pDfd = vk2dfd((VkFormat)ctx.header.vkFormat);
        if (pDfd == nullptr)
            addIssue(logger::eFatal, ValidatorError.CreateDfdFailure,
                     vkFormatString((VkFormat)ctx.header.vkFormat));

        if (!ctx.extractFormatInfo(pDfd))
            addIssue(logger::eError, ValidatorError.IncorrectDfd,
                     vkFormatString((VkFormat)ctx.header.vkFormat));

        if (ctx.formatInfo.isBlockCompressed) {
            if (ctx.header.typeSize != 1)
                addIssue(logger::eError, HeaderData.TypeSizeNotOne);
        } else {
            if (ctx.header.typeSize != ctx.formatInfo.wordSize)
                 addIssue(logger::eError, HeaderData.TypeSizeMismatch);
        }
    } else if (ctx.header.supercompressionScheme != KTX_SUPERCOMPRESSION_NONE) {
        if (ctx.header.typeSize != 1)
            addIssue(logger::eError, HeaderData.TypeSizeNotOne);
    }

    if (ctx.header.supercompressionScheme > KTX_SUPERCOMPRESSION_BEGIN_VENDOR_RANGE
        && ctx.header.supercompressionScheme < KTX_SUPERCOMPRESSION_END_VENDOR_RANGE)
    {
        addIssue(logger::eWarning, HeaderData.VendorSupercompression);
    } else if (ctx.header.supercompressionScheme < KTX_SUPERCOMPRESSION_BEGIN_RANGE
        || ctx.header.supercompressionScheme > KTX_SUPERCOMPRESSION_END_RANGE)
    {
        addIssue(logger::eError, HeaderData.InvalidSupercompression,
                 ctx.header.supercompressionScheme);
    }

#define checkRequiredIndexEntry(index, issue, name)     \
    if (index.byteOffset == 0 || index.byteLength == 0) \
        addIssue(logger::eError, issue, name)

#define checkOptionalIndexEntry(index, issue, name)     \
    if (!index.byteOffset != !index.byteLength)         \
        addIssue(logger::eError, issue, name)

    checkRequiredIndexEntry(ctx.header.dataFormatDescriptor,
                    HeaderData.InvalidRequiredIndexEntry, "dfd");

    // This is required because KTXwriter is required.
    checkRequiredIndexEntry(ctx.header.keyValueData,
                    HeaderData.InvalidRequiredIndexEntry, "kvd");

    if (ctx.header.supercompressionScheme == KTX_SUPERCOMPRESSION_BASIS) {
        checkRequiredIndexEntry(ctx.header.supercompressionGlobalData,
                                HeaderData.InvalidOptionalIndexEntry, "sgd");
    } else {
        checkOptionalIndexEntry(ctx.header.supercompressionGlobalData,
                                HeaderData.InvalidOptionalIndexEntry, "sgd");
    }

    ctx.levelIndexSize = sizeof(ktxLevelIndexEntry) * ctx.levelCount;
    uint64_t offset = KTX2_HEADER_SIZE + ctx.levelIndexSize;
    if (offset != ctx.header.dataFormatDescriptor.byteOffset)
        addIssue(logger::eError, HeaderData.InvalidDFDOffset);
    offset += ctx.header.dataFormatDescriptor.byteLength;

    if (ctx.header.keyValueData.byteOffset != 0) {
        if (offset != ctx.header.keyValueData.byteOffset)
            addIssue(logger::eError, HeaderData.InvalidKVDOffset);
        offset += ctx.header.keyValueData.byteLength;
        offset = _KTX_PAD8(offset);
    }

    if (ctx.header.supercompressionGlobalData.byteOffset != 0) {
        if (offset != ctx.header.supercompressionGlobalData.byteOffset)
            addIssue(logger::eError, HeaderData.InvalidSGDOffset);
    }
}

void
ktxValidator::validateLevelIndex(validationContext& ctx)
{
     ktxLevelIndexEntry* levelIndex = new ktxLevelIndexEntry[ctx.levelCount];
     if (fread(levelIndex, ctx.levelIndexSize, 1, ctx.inf) != 1) {
        if (ferror(ctx.inf))
            addIssue(logger::eFatal, IOError.FileRead, strerror(errno));
        else
            addIssue(logger::eFatal, IOError.UnexpectedEOF);
    }

    if (ctx.header.vkFormat != VK_FORMAT_UNDEFINED
        && ctx.header.supercompressionScheme == KTX_SUPERCOMPRESSION_NONE) {
        for (uint32_t level = 0; level < ctx.levelCount; level++) {
            if (levelIndex[level].uncompressedByteLength !=
                ctx.calcLevelSize(level))
                addIssue(logger::eError, LevelIndex.IncorrectByteLength, level);

            if (levelIndex[level].byteLength !=
                levelIndex[level].uncompressedByteLength)
                addIssue(logger::eError, LevelIndex.IncorrectByteLength, level);

            if (levelIndex[level].byteOffset != ctx.calcLevelOffset(level))
                addIssue(logger::eError, LevelIndex.IncorrectByteOffset, level);

        }
    } else {
        // Can only do minimal validation as we have no idea what the
        // level sizes are.
        for (uint32_t level = 0; level < ctx.levelCount; level++) {
            if (levelIndex[level].byteLength == 0 || levelIndex[level].byteOffset == 0)
               addIssue(logger::eError, LevelIndex.ZeroOffsetOrLength, level);
            if (ctx.header.supercompressionScheme != KTX_SUPERCOMPRESSION_BASIS) {
                if (levelIndex[level].uncompressedByteLength == 0)
               addIssue(logger::eError, LevelIndex.ZeroUncompressedLength,
                        level);
            }
        }

    }
    delete[] levelIndex;
}

void
ktxValidator::validateDfd(validationContext& ctx)
{
    if (ctx.header.dataFormatDescriptor.byteLength == 0)
        return;

    // We are right after the levelIndex. We've already checked that
    // header.dataFormatDescriptor.byteOffset points to this location.
    uint32_t* pDfd = new uint32_t[ctx.header.dataFormatDescriptor.byteLength
                                   / sizeof(uint32_t)];
    if (fread(pDfd, ctx.header.dataFormatDescriptor.byteLength, 1, ctx.inf) != 1) {
        if (ferror(ctx.inf))
            addIssue(logger::eFatal, IOError.FileRead, strerror(errno));
        else
            addIssue(logger::eFatal, IOError.UnexpectedEOF);
    }
    uint32_t* bdb = pDfd+ 1; // Basic descriptor block.

    uint32_t xferFunc;
    if ((xferFunc = KHR_DFDVAL(bdb, TRANSFER)) != KHR_DF_TRANSFER_SRGB
        && xferFunc != KHR_DF_TRANSFER_LINEAR)
        addIssue(logger::eError, DFD.InvalidTransferFunction);

    if (ctx.header.vkFormat != VK_FORMAT_UNDEFINED
        && ctx.header.supercompressionScheme == KTX_SUPERCOMPRESSION_NONE) {
        // Do a simple comparison.
        if (memcmp(pDfd, ctx.pDfd4Format, *ctx.pDfd4Format)) {
            // pDfd differs from what is expected. To help developers, do a
            // more in depth analysis.
            if (KHR_DFDVAL(bdb, VENDORID) != KHR_DF_VENDORID_KHRONOS
                || KHR_DFDVAL(bdb, DESCRIPTORTYPE) != KHR_DF_KHR_DESCRIPTORTYPE_BASICFORMAT
                || KHR_DFDVAL(bdb, VERSIONNUMBER) < KHR_DF_VERSIONNUMBER_1_3)
                addIssue(logger::eError, DFD.IncorrectBasics);

            if (ctx.formatInfo.isBlockCompressed) {
                // _BLOCK formats.
                if (KHR_DFDVAL(bdb, MODEL) < KHR_DF_MODEL_DXT1A)
                  addIssue(logger::eError, DFD.IncorrectModelForBlock);
            } else {
                InterpretedDFDChannel r, g, b, a;
                uint32_t componentByteLength;
                InterpretDFDResult result;
                string vkFormatStr(vkFormatString((VkFormat)ctx.header.vkFormat));

                result = interpretDFD(pDfd, &r, &g, &b, &a, &componentByteLength);
                if (result > i_UNSUPPORTED_ERROR_BIT)
                    addIssue(logger::eError, DFD.TooComplex);

                if ((result & i_FLOAT_FORMAT_BIT) && !(result & i_SIGNED_FORMAT_BIT))
                    addIssue(logger::eWarning, DFD.UnsignedFloat);

                if (result & i_SRGB_FORMAT_BIT) {
                    if (vkFormatStr.find("SRGB") == string::npos)
                        addIssue(logger::eError, DFD.sRGBMismatch);
                } else {
                    string findStr;
                    if (result & i_SIGNED_FORMAT_BIT)
                        findStr += 'S';
                    else
                        findStr += 'U';

                    if (result & i_FLOAT_FORMAT_BIT)
                        findStr += "FLOAT";
                    if (result & i_NORMALIZED_FORMAT_BIT)
                        findStr += "NORM";
                    else
                        findStr += "INT";

                    if (vkFormatStr.find(findStr) == string::npos)
                        addIssue(logger::eError, DFD.FormatMismatch);
                }
            }
        }
    } else {
        switch (ctx.header.supercompressionScheme) {
          case KTX_SUPERCOMPRESSION_BASIS:
            // This descriptor should have 0 samples.
            if (*pDfd != sizeof(uint32_t) * (1 + KHR_DF_WORD_SAMPLESTART))
                addIssue(logger::eError, DFD.NonZeroSamplesForBasis);

            if (KHR_DFDVAL(bdb, MODEL) != KHR_DF_MODEL_UNSPECIFIED)
                addIssue(logger::eError, DFD.IncorrectModelForBasis);

            if (bdb[KHR_DF_WORD_TEXELBLOCKDIMENSION0] != 0
               || bdb[KHR_DF_WORD_BYTESPLANE0]  != 0
               || bdb[KHR_DF_WORD_BYTESPLANE4]  != 0)
               addIssue(logger::eError, DFD.IncorrectSizesForBasis);
            break;
          default:
            addIssue(logger::eError, ValidatorError.OnlyBasisSupported);
        }
        return;
    }
}

void
ktxValidator::validateKvd(validationContext& ctx)
{
    uint32_t kvdLen = ctx.header.keyValueData.byteLength;
    if (kvdLen == 0)
        return;

    uint8_t* kvd = new uint8_t[kvdLen];
    if (fread(kvd, kvdLen, 1, ctx.inf) != 1) {
        if (ferror(ctx.inf))
            addIssue(logger::eFatal, IOError.FileRead, strerror(errno));
        else
            addIssue(logger::eFatal, IOError.UnexpectedEOF);
    }

    ktxHashList kvDataHead = 0;
    ktxHashListEntry* entry;
    char* prevKey;
    uint32_t prevKeyLen;
    KTX_error_code result;
    bool writerFound = false;

    // TODO Deserialize will likely fail badly if keys are not NUL terminated.
    //      Therefore we should check before calling this.
    result = ktxHashList_Deserialize(&kvDataHead, kvdLen, kvd);
    if (result != KTX_SUCCESS) {
        addIssue(logger::eError, System.OutOfMemory);
        return;
    }

    // Check the entries are sorted
    ktxHashListEntry_GetKey(kvDataHead, &prevKeyLen, &prevKey);
    entry = ktxHashList_Next(kvDataHead);
    for (; entry != NULL; entry = ktxHashList_Next(entry)) {
        uint32_t keyLen;
        char* key;

        ktxHashListEntry_GetKey(entry, &keyLen, &key);
        if (strcmp(prevKey, key) > 0) {
            addIssue(logger::eError, Metadata.OutOfOrder);
            break;
        }
    }

    for (entry = kvDataHead; entry != NULL; entry = ktxHashList_Next(entry)) {
        uint32_t keyLen, valueLen;
        char* key;
        uint8_t* value;

        ktxHashListEntry_GetKey(entry, &keyLen, &key);
        ktxHashListEntry_GetValue(entry, &valueLen, (void**)&value);
        if (strncasecmp(key, "KTX", 3) == 0) {
            if (!validateMetadata(ctx, key, value, valueLen)) {
                addIssue(logger::eError, Metadata.IllegalMetadata, key);
            }
            if (strncmp(key, "KTXwriter", 9) == 0)
                writerFound = true;
        } else {
            addIssue(logger::eWarning, Metadata.CustomMetadata, key);
        }
    }
    if (!writerFound)
        addIssue(logger::eError, Metadata.NoKTXwriter);

    // Advance the read past any padding.
    // Use read not skip so stdin can be used.
    uint32_t pLen = _KTX_PAD8_LEN(ctx.header.keyValueData.byteOffset + kvdLen);
    uint8_t padBuf[8];
    if (fread(padBuf, pLen, 1, ctx.inf) != 1) {
        if (ferror(ctx.inf))
            addIssue(logger::eFatal, IOError.FileRead, strerror(errno));
        else
            addIssue(logger::eFatal, IOError.UnexpectedEOF);
    }
}

bool
ktxValidator::validateMetadata(validationContext& ctx, char* key,
                               uint8_t* pValue, uint32_t valueLen)
{
#define CALL_MEMBER_FN(object,ptrToMember)  ((object)->*(ptrToMember))
    vector<metadataValidator>::const_iterator it;

    for (it = metadataValidators.begin(); it < metadataValidators.end(); it++) {
        if (!it->name.compare(key)) {
            //validateMetadataFunc vf = it->validateFunc;
            CALL_MEMBER_FN(this, it->validateFunc)(ctx, key, pValue, valueLen);
            break;
        }
    }
    if (it == metadataValidators.end())
        return false; // Unknown KTX-prefixed and therefore illegal metadata.
    else
        return true;
}

void
ktxValidator::validateCubemapIncomplete(validationContext& ctx, char* key,
                                        uint8_t* value, uint32_t valueLen)
{
    if (valueLen != 1)
        addIssue(logger::eError, Metadata.InvalidValue, key);
}

void
ktxValidator::validateOrientation(validationContext& ctx, char* key,
                                  uint8_t* value, uint32_t valueLen)
{
    if (value[valueLen-1] != '\0')
        addIssue(logger::eError, Metadata.ValueNotNulTerminated, key);

    if (valueLen != ctx.dimensionCount + 1)
        addIssue(logger::eError, Metadata.InvalidValue, key);

    switch (ctx.dimensionCount) {
      case 1:
        if (!regex_match ((char*)value, regex("^[rl]$") ))
            addIssue(logger::eError, Metadata.InvalidValue, key);
        break;
      case 2:
        if (!regex_match ((char*)value, regex("^[rl][du]$") ))
            addIssue(logger::eError, Metadata.InvalidValue, key);
        break;
      case 3:
        if (!regex_match((char*)value, regex("^[rl][du][oi]$")))
            addIssue(logger::eError, Metadata.InvalidValue, key);
        break;
    }
}

void
ktxValidator::validateGlFormat(validationContext& ctx, char* key,
                               uint8_t* value, uint32_t valueLen)
{
    if (valueLen != sizeof(uint32_t) * 3)
        addIssue(logger::eError, Metadata.InvalidValue, key);
}

void
ktxValidator::validateDxgiFormat(validationContext& ctx, char* key,
                                 uint8_t* value, uint32_t valueLen)
{
    if (valueLen != sizeof(uint32_t))
        addIssue(logger::eError, Metadata.InvalidValue, key);}

void
ktxValidator::validateMetalPixelFormat(validationContext& ctx, char* key,
                                       uint8_t* value, uint32_t valueLen)
{
    if (valueLen != sizeof(uint32_t))
        addIssue(logger::eError, Metadata.InvalidValue, key);
}

void
ktxValidator::validateSwizzle(validationContext& ctx, char* key,
                              uint8_t* value, uint32_t valueLen)
{
    if (value[valueLen-1] != '\0')
        addIssue(logger::eError, Metadata.ValueNotNulTerminated, key);
    if (!regex_match((char*)value, regex("^[rgba01]{4}$")))
        addIssue(logger::eError, Metadata.InvalidValue, key);
}

void
ktxValidator::validateWriter(validationContext& ctx, char* key,
                             uint8_t* value, uint32_t valueLen)
{
    if (value[valueLen-1] != '\0')
        addIssue(logger::eError, Metadata.ValueNotNulTerminated, key);
}

void
ktxValidator::validateAstcDecodeRGB9E5(validationContext& ctx, char* key,
                                       uint8_t* value, uint32_t valueLen)
{
    if (valueLen != 0)
        addIssue(logger::eError, Metadata.InvalidValue, key);
}

void
ktxValidator::validateSgd(validationContext& ctx)
{
    uint64_t sgdByteLength = ctx.header.supercompressionGlobalData.byteLength;
    if (ctx.header.supercompressionScheme == KTX_SUPERCOMPRESSION_BASIS) {
        if (sgdByteLength == 0) {
            addIssue(logger::eError, SGD.MissingSupercompressionGlobalData);
            return;
        }
    } else {
        if (sgdByteLength > 0)
            addIssue(logger::eError, SGD.UnexpectedSupercompressionGlobalData);
        return;
    }

    uint8_t* sgd = new uint8_t[sgdByteLength];
    if (fread(sgd, sgdByteLength, 1, ctx.inf) != 1) {
        if (ferror(ctx.inf))
            addIssue(logger::eFatal, IOError.FileRead, strerror(errno));
        else
            addIssue(logger::eFatal, IOError.UnexpectedEOF);
    }

    // firstImages contains the indices of the first images for each level.
    // The last array entry contains the total number of images which is what
    // we need here.
    uint32_t* firstImages = new uint32_t[ctx.levelCount+1];
    // Temporary invariant value
    uint32_t layersFaces = ctx.layerCount * ctx.header.faceCount;
    firstImages[0] = 0;
    for (uint32_t level = 1; level <= ctx.levelCount; level++) {
        // NOTA BENE: numFaces * depth is only reasonable because they can't
        // both be > 1. I.e there are no 3d cubemaps.
        firstImages[level] = firstImages[level - 1]
                           + layersFaces * MAX(ctx.header.pixelDepth >> (level - 1), 1);
    }
    uint32_t& imageCount = firstImages[ctx.levelCount];

    ktxBasisGlobalHeader& bgdh = *reinterpret_cast<ktxBasisGlobalHeader*>(sgd);
    uint64_t expectedBgdByteLength = sizeof(ktxBasisGlobalHeader)
                                   + sizeof(ktxBasisSliceDesc) * imageCount
                                   + bgdh.endpointsByteLength
                                   + bgdh.selectorsByteLength
                                   + bgdh.tablesByteLength;

    if (sgdByteLength != expectedBgdByteLength)
        addIssue(logger::eError, SGD.IncorrectGlobalDataSize);

    if (bgdh.extendedByteLength != 0)
        addIssue(logger::eError, SGD.ExtendedByteLengthNotZero);

    // Can't do anymore as we have no idea how many endpoints, etc there
    // should be.
}

// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 sts=4 expandtab:

//
// Copyright 2019-2020 The Khronos Group, Inc.
// SPDX-License-Identifier: Apache-2.0
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
// Gotta love Windows :-)
#if defined(_MSC_VER)
  #define strncasecmp _strnicmp
  #if defined(_WIN64)
    #define ftello _ftelli64
    #define fseeko _fseeki64
  #else
    #define ftello ftell
    #define fseeko fseek
  #endif
#endif
#include "version.h"

std::string myversion(STR(KTX2CHECK_VERSION));
std::string mydefversion(STR(KTX2CHECK_DEFAULT_VERSION));

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
    <dt>-m &lt;num&gt;, --max-issues &lt;num&gt;</dt>
    <dd>Set the maximum number of issues to be reported per file
        provided -q is not set.</dd>
    <dt>-w, --warn-as-error</dt>
    <dd>Treat warnings as errors. Changes exit code from success to error.
    </dl>
    @snippetdoc ktxapp.h ktxApp options

@section ktx2check_exitstatus EXIT STATUS
    @b toktx exits 0 on success, 1 on command line errors and 2 on
    validation errors.

@section ktx2check_history HISTORY

@par Version 4.0
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
    issue FileSeekEndFailure {
        FATAL | 0x0005, "Seek to end of file failed: %s."
    };
    issue FileTellFailure {
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
    issue IncorrectDataSize {
        FATAL | 0x0012, "Size of image data in file does not match size calculated from levelIndex."
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
    issue VkFormatAndBasis {
        ERROR | 0x0031, "VkFormat must be VK_FORMAT_UNDEFINED for supercompressionScheme BASIS_LZ."
    };
    issue TypeSizeNotOne {
        ERROR | 0x0032, "typeSize for a block compressed or supercompressed format must be 1."
    };
    issue ZeroLevelCountForBC {
        ERROR | 0x0033, "levelCount must be > 0 for block-compressed formats."
    };
} HeaderData;

struct {
    issue CreateDfdFailure {
        FATAL | 0x0040, "Creation of DFD matching %s failed."
    };
    issue IncorrectDfd {
        FATAL | 0x0041, "DFD created for %s confused interpretDFD()."
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
    issue MultiplePlanes {
        ERROR | 0x0053, "DFD is for a multiplane format. These are not supported."
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
    issue ZeroSamples {
        ERROR | 0x0057, "DFD for a %s texture must have sample information."
    };
    issue TexelBlockDimensionZeroForUndefined {
        ERROR | 0x0058, "DFD texel block dimensions must be non-zero for non-supercompressed texture"
                        " with VK_FORMAT_UNDEFINED."
    };
    issue FourDimensionalTexturesNotSupported {
        ERROR | 0x0059, "DFD texelBlockDimension3 is non-zero indicating an unsupported four-dimensional texture."
    };
    issue BytesPlane0Zero {
        ERROR | 0x005a, "DFD bytesPlane0 must be non-zero for non-supercompressed texture with %s."
    };
    issue MultiplaneFormatsNotSupported {
        ERROR | 0x005b, "DFD has non-zero value in bytesPlane[1-7] indicating unsupported multiplane format."
    };
    issue InvalidSampleCount {
        ERROR | 0x005c, "DFD for a %s texture must have %s sample(s)."
    };
    issue IncorrectModelForBLZE {
        ERROR | 0x005d, "DFD colorModel for BasisLZ/ETC1S must be KHR_DF_MODEL_ETC1S."
    };
    issue InvalidTexelBlockDimension {
        ERROR | 0x005e, "DFD texel block dimension must be %dx%d for %s textures."
    };
    issue NotUnsized {
        ERROR | 0x005f, "DFD bytes/plane must be 0 for a supercompressed texture."
    };
    issue InvalidChannelForBLZE {
        ERROR | 0x0060, "Only ETC1S_RGB (0), ETC1S_RRR (3), ETC1S_GGG (4) or ETC1S_AAA (15)"
                        " channels allowed for BasisLZ/ETC1S textures."
    };
    issue InvalidBitOffsetForBLZE {
        ERROR | 0x0061, "DFD sample bitOffsets for BasisLZ/ETC1S textures must be 0 and 64."
    };
    issue InvalidBitLength {
        ERROR | 0x0062, "DFD sample bitLength for %s textures must be %d."
    };
    issue InvalidLowerOrUpper {
        ERROR | 0x0063, "All DFD samples' sampleLower must be 0 and sampleUpper must be 0xFFFFFFFF for"
                        "%s textures."
    };
    issue InvalidChannelForUASTC {
        ERROR | 0x0064, "Only UASTC_RGB (0), UASTC_RGBA (3), UASTC_RRR (4) or UASTC_RRRG (5) channels"
                        " allowed for UASTC textures."
    };
    issue InvalidBitOffsetForUASTC {
        ERROR | 0x0065, "DFD sample bitOffset for UASTC textures must be 0."
    };
    issue SizeMismatch {
        ERROR | 0x0066, "DFD totalSize differs from header's dfdByteLength."
    };
    issue InvalidColorModel {
        ERROR | 0x0067, "DFD colorModel for non block-compressed textures must be RGBSDA."
    };
    issue MixedChannels {
        ERROR | 0x0067, "DFD has channels with differing flags, e.g. some float, some integer."
    };
    issue Multisample {
        ERROR | 0x0067, "DFD indicates multiple sample locations."
    };
    issue NonTrivialEndianness {
        ERROR | 0x0067, "DFD describes non little-endian data."
    };
} DFD;

struct {
    issue IncorrectByteLength {
        ERROR | 0x0070, "Level %d byteLength or uncompressedByteLength does not match expected value."
    };
    issue ByteOffsetTooSmall {
        ERROR | 0x0071, "Level %d byteOffset is smaller than expected value."
    };
    issue IncorrectByteOffset {
        ERROR | 0x0072, "Level %d byteOffset does not match expected value."
    };
    issue UnalignedOffset {
        ERROR | 0x0073, "Level %d byteOffset is not aligned to required %d byte alignment."
    };
    issue ExtraPadding {
        ERROR | 0x0074, "Level %d has disallowed extra padding."
    };
    issue ZeroOffsetOrLength {
        ERROR | 0x0075, "Level %d's byteOffset or byteLength is 0."
    };
    issue ZeroUncompressedLength {
        ERROR | 0x0076, "Level %d's uncompressedByteLength is 0."
    };
    issue IncorrectLevelOrder {
        ERROR | 0x0077, "Larger mip levels are before smaller."
    };
} LevelIndex;

struct {
    issue MissingNulTerminator {
        ERROR | 0x0080, "Required NUL terminator missing from metadata key beginning \"%5s\"."
                        "Abandoning validation of individual metadata entries."
    };
    issue ForbiddenBOM1 {
        ERROR | 0x0081, "Metadata key beginning \"%5s\" has forbidden BOM."
    };
    issue ForbiddenBOM2 {
        ERROR | 0x0082, "Metadata key beginning \"%s\" has forbidden BOM."
    };
    issue InvalidStructure {
        ERROR | 0x0083, "Invalid metadata structure? keyAndValueByteLengths failed to total kvdByteLength"
                        " after %d KV pairs."
    };
    issue MissingFinalPadding {
        ERROR | 0x0084, "Required valuePadding after last metadata value missing."
    };
    issue OutOfOrder {
        ERROR | 0x0085, "Metadata keys are not sorted in codepoint order."
    };
    issue CustomMetadata {
        WARNING | 0x0086, "Custom metadata \"%s\" found."
    };
    issue IllegalMetadata {
        ERROR | 0x0087, "Unrecognized metadata \"%s\" found with KTX or ktx prefix found."
    };
    issue ValueNotNulTerminated {
        ERROR | 0x0088, "%s value missing required NUL termination."
    };
    issue InvalidValue {
        ERROR | 0x0089, "%s has invalid value."
    };
    issue NoRequiredKTXwriter {
        ERROR | 0x008a, "No KTXwriter key. Required when KTXwriterScParams is present."
    };
    issue MissingValue {
        ERROR | 0x008b, "Missing required value for \"%s\" key."
    };
    issue NotAllowed {
        ERROR | 0x008c, "\"%s\" key not allowed %s."
    };
    issue NoKTXwriter {
        WARNING | 0x008f, "No KTXwriter key. Writers are strongly urged to identify themselves via this."
    };
} Metadata;

struct {
    issue UnexpectedSupercompressionGlobalData {
        ERROR | 0x0090, "Supercompression global data found scheme that is not Basis."
    };
    issue MissingSupercompressionGlobalData {
        ERROR | 0x0091, "Basis supercompression global data missing."
    };
    issue InvalidImageFlagBit {
        ERROR | 0x0092, "Basis supercompression global data imageDesc.imageFlags has an invalid bit set."
    };
    issue IncorrectGlobalDataSize {
        ERROR | 0x0093, "Basis supercompression global data has incorrect size."
    };
    issue ExtendedByteLengthNotZero {
        ERROR | 0x0094, "extendedByteLength != 0 in Basis supercompression global data."
    };
    issue DfdMismatchAlpha {
        ERROR | 0x0095, "supercompressionGlobalData indicates no alpha but DFD indicates alpha channel."
    };
    issue DfdMismatchNoAlpha {
        ERROR | 0x0096, "supercompressionGlobalData indicates an alpha channel but DFD indicates no alpha channel."
    };
} SGD;

struct {
    issue OutOfMemory {
        ERROR | 0x00a0, "System out of memory."
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
//                      Define Helpful Functions                   //
/////////////////////////////////////////////////////////////////////

// Increase nbytes to make it a multiple of n. Works for any n.
size_t padn(uint32_t n, size_t nbytes) {
    return (size_t)(n * ceilf((float)nbytes / n));
}

// Calculate number of bytes to add to nbytes to make it a multiple of n.
// Works for any n.
uint32_t padn_len(uint32_t n, size_t nbytes) {
    return (uint32_t)((n * ceilf((float)nbytes / n)) - nbytes);
}

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
            errorCount = 0;
            warningCount = 0;
            headerWritten = false;
            quiet = false;
        }
        enum severity { eWarning, eError, eFatal };
        void addIssue(severity severity, issue issue, va_list args);
        void startFile(const std::string& filename) {
            nameOfFileBeingValidated = filename;
            errorCount = 0;
            warningCount = 0;
            headerWritten = false;
        }
        uint32_t getErrorCount() { return this->errorCount; }
        uint32_t getWarningCount() { return this->warningCount; }
        uint32_t maxIssues;
        bool quiet;

      protected:
        uint32_t errorCount;
        uint32_t warningCount;
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
        uint32_t* pActualDfd;
        uint64_t dataSizeFromLevelIndex;
        uint64_t curFileOffset;  // Keep our own so can use stdin.
        bool cubemapIncompleteFound;

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
            pActualDfd = nullptr;
            cubemapIncompleteFound = false;
            dataSizeFromLevelIndex = 0;
            curFileOffset = 0;
        }

        ~validationContext() {
            if (pDfd4Format != nullptr) delete pDfd4Format;
            if (pActualDfd != nullptr) delete pActualDfd;
        }

        size_t kvDataEndOffset() {
            return sizeof(KTX_header2) + levelIndexSize
                   + header.dataFormatDescriptor.byteLength
                   + header.keyValueData.byteLength;
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

        // Recursive function to return the greatest common divisor of a and b.
        uint32_t gcd(uint32_t a, uint32_t b) {
            if (a == 0)
                return b;
            return gcd(b % a, a);
        }

        // Function to return the least common multiple of a & 4.
        uint32_t lcm4(uint32_t a)
        {
            if (!(a & 0x03))
                return a;  // a is a multiple of 4.
            return (a*4) / gcd(a, 4);
        }

        size_t calcLevelOffset(uint32_t level) {
            // This function is only useful when the following 2 conditions
            // are met as otherwise we have no idea what the size of a level
            // ought to be.
            assert (header.vkFormat != VK_FORMAT_UNDEFINED);
            assert (header.supercompressionScheme == KTX_SS_NONE);

            assert (level < levelCount);
            // Calculate the expected base offset in the file
            size_t levelOffset = kvDataEndOffset();
            levelOffset
                  = padn(lcm4(formatInfo.blockByteLength), levelOffset);
            for (uint32_t i = levelCount - 1; i > level; i--) {
                size_t levelSize;
                levelSize = calcLevelSize(i);
                levelOffset
                    += padn(lcm4(formatInfo.blockByteLength), levelSize);
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

        uint32_t requiredLevelAlignment() {
            if (header.supercompressionScheme != KTX_SS_NONE)
                return 1;
            else if (header.vkFormat == VK_FORMAT_UNDEFINED)
                return 16;
            else
                return lcm4(formatInfo.blockByteLength);
        }

        void init (FILE* f) {
            if (pDfd4Format != nullptr) delete pDfd4Format;
            inf = f;
            dataSizeFromLevelIndex = 0;
        }

        ktx_size_t fileRead(void* ptr, size_t size, size_t nitems) {
            ktx_size_t result;
            result = fread(ptr, size, nitems, inf);
            if (result == nitems) curFileOffset += (size * nitems);
            return result;
        }

        int fileError() {
            return ferror(inf);
        }

        // Move read point from curOffset to next multiple of alignment bytes.
        // Use read not fseeko/setpos so stdin can be used.
        ktx_uint32_t skipPadding(uint32_t alignment) {
            uint32_t padLen = padn_len(alignment, curFileOffset);
            uint8_t padBuf[32];
            ktx_uint32_t result = 1;
            if (padLen) {
                if (fileRead(padBuf, padLen, 1) != 1)
                    result = 0;
            }
            return result;
        }
    };

    void addIssue(logger::severity severity, issue issue, ...) {
        va_list args;
        va_start(args, issue);

        logger.addIssue(severity, issue, args);
        va_end(args);
    }
    virtual bool processOption(argparser& parser, int opt);
    void validateFile(const string&);
    void validateHeader(validationContext& ctx);
    void validateLevelIndex(validationContext& ctx);
    void validateDfd(validationContext& ctx);
    void validateKvd(validationContext& ctx);
    void validateSgd(validationContext& ctx);
    void validateDataSize(validationContext& ctx);
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
    void validateWriterScParams(validationContext& ctx, char* key,
                                uint8_t* value, uint32_t valueLen);
    void validateAstcDecodeMode(validationContext& ctx, char* key,
                                uint8_t* value, uint32_t valueLen);
    void validateAnimData(validationContext& ctx, char* key,
                          uint8_t* value, uint32_t valueLen);

    typedef struct {
        string name;
        validateMetadataFunc validateFunc;
    } metadataValidator;
    static vector<metadataValidator> metadataValidators;

    struct commandOptions : public ktxApp::commandOptions {
        uint32_t maxIssues;
        bool quiet;
        bool errorOnWarning;

        commandOptions() {
            maxIssues = 0xffffffffU;
            quiet = false;
			errorOnWarning = false;
        }
    } options;

    void skipPadding(validationContext& ctx, uint32_t alignment) {
        if (ctx.skipPadding(alignment) == 0) {
            if (ctx.fileError())
                addIssue(logger::eFatal, IOError.FileRead, strerror(errno));
            else
                addIssue(logger::eFatal, IOError.UnexpectedEOF);
        }
    }
};

vector<ktxValidator::metadataValidator> ktxValidator::metadataValidators {
    // cubemapIncomplete must appear in this list before animData.
    { "KTXcubemapIncomplete", &ktxValidator::validateCubemapIncomplete },
    { "KTXorientation", &ktxValidator::validateOrientation },
    { "KTXglFormat", &ktxValidator::validateGlFormat },
    { "KTXdxgiFormat__", &ktxValidator::validateDxgiFormat },
    { "KTXMetalPixelFormat", &ktxValidator::validateMetalPixelFormat },
    { "KTXswizzle", &ktxValidator::validateSwizzle },
    { "KTXwriter", &ktxValidator::validateWriter },
    { "KTXwriterScParams", &ktxValidator::validateWriterScParams },
    { "KTXastcDecodeMode", &ktxValidator::validateAstcDecodeMode },
    { "KTXanimData", &ktxValidator::validateAnimData }
};

/////////////////////////////////////////////////////////////////////
//                     Validator Implementation                    //
/////////////////////////////////////////////////////////////////////

ktxValidator::ktxValidator() : ktxApp(myversion, mydefversion, options)
{
    argparser::option my_option_list[] = {
        { "quiet", argparser::option::no_argument, NULL, 'q' },
        { "max-issues", argparser::option::required_argument, NULL, 'm' },
        { "warn-as-error", argparser::option::no_argument, NULL, 'w' }
    };
    const int lastOptionIndex = sizeof(my_option_list)
                                / sizeof(argparser::option);
    option_list.insert(option_list.begin(), my_option_list,
                       my_option_list + lastOptionIndex);
    short_opts += "qm:";
}

#if defined(_MSC_VER)
int vasprintf(char **strp, const char *format, va_list ap)
{
    int len = _vscprintf(format, ap);
    if (len == -1)
        return -1;
    char *str = (char*)malloc((size_t) len + 1);
    if (!str)
        return -1;
    int retval = _vsnprintf(str, len + 1, format, ap);
    if (retval == -1) {
        free(str);
        return -1;
    }
    *strp = str;
    return retval;
}
#endif

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
        const uint32_t baseIndent = 4;
        uint32_t indent;
        if ((errorCount + warningCount ) < maxIssues) {
            for (uint32_t j = 0; j < baseIndent; j++)
              cout.put(' ');
            switch (severity) {
              case eError:
                cout << "ERROR: ";
                indent = baseIndent + 7;
                errorCount++;
                break;
              case eFatal:
                cout << "FATAL: ";
                indent = baseIndent + 7;
                break;
              case eWarning:
                cout << "WARNING: ";
                indent = baseIndent + 9;
                warningCount++;
                break;
            }
            //vfprintf(stdout, issue.message.c_str(), args);
            char* pBuf;
            int nchars = vasprintf(&pBuf, issue.message.c_str(), args);
            if (nchars < 0) {
                std::stringstream message;

                message << "Could not create issue message: ";
                message << strerror(errno);
                throw std::runtime_error(message.str());
            }
            // Wrap lines on spaces.
            uint32_t line = 0;
            uint32_t lsi = 0;  // line start index.
            uint32_t lei; // line end index
            while (nchars + indent > 80) {
                uint32_t ll; // line length
                lei = lsi + 79 - indent;
                while (pBuf[lei] != ' ') lei--;
                ll = lei - lsi;
                for (uint32_t j = 0; j < (line ? indent : 0); j++) {
                    cout.put(' ');
                }
                cout.write(&pBuf[lsi], ll) << std::endl;
                lsi = lei + 1; // +1 to skip the space
                nchars -= ll;
                line++;
            }
            for (uint32_t j = 0; j < (line ? baseIndent : 0); j++) {
                cout.put(' ');
            }
            cout.write(&pBuf[lsi], nchars);
            cout << std::endl;
            free(pBuf);
        } else {
            throw max_issues_exceeded();
        }
    }
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
        "               provided -q is not set.\n"
        "  -w, --warn-as-error\n"
        "               Treat warnings as errors. Changes error code from success\n"
        "               to error\n";
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

    logger.quiet = options.quiet;
    logger.maxIssues = options.maxIssues;

    vector<_tstring>::const_iterator it;
    for (it = options.infiles.begin(); it < options.infiles.end(); it++) {
        try {
            validateFile(*it);
        } catch (fatal&) {
            // File could not be opened.
            return 2;
        }
    }
    if (logger.getErrorCount() > 0)
        return 2;
    else if (logger.getWarningCount() > 0 && options.errorOnWarning)
        return 2;
    else
        return 0;
}

void
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
            if (context.header.supercompressionGlobalData.byteLength > 0)
                skipPadding(context, 8);
            validateSgd(context);
            skipPadding(context, context.requiredLevelAlignment());
            validateDataSize(context);
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
}

bool
ktxValidator::processOption(argparser& parser, int opt)
{
    switch (opt) {
      case 'q':
        options.quiet = true;
        break;
      case 'm':
        options.maxIssues = atoi(parser.optarg.c_str());
        break;
      case 'w':
        options.errorOnWarning = true;
      default:
        return false;
    }
    return true;
}

void
ktxValidator::validateHeader(validationContext& ctx)
{
    ktx_uint8_t identifier_reference[12] = KTX2_IDENTIFIER_REF;
    ktx_uint32_t max_dim;

    if (ctx.fileRead(&ctx.header, sizeof(KTX_header2), 1) != 1) {
        if (ctx.fileError())
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
        if (ctx.header.vkFormat <= VK_FORMAT_MAX_STANDARD_ENUM || ctx.header.vkFormat > 0x10010000)
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
    if (max_dim < ((ktx_uint32_t)1 << (ctx.levelCount - 1)))
    {
        // Can't have more mip levels than 1 + log2(max(width, height, depth))
        addIssue(logger::eError, HeaderData.TooManyMipLevels,
                 ctx.levelCount, max_dim);
    }

    // Set layerCount to actual number of layers.
    ctx.layerCount = MAX(ctx.header.layerCount, 1);

    if (ctx.header.supercompressionScheme > KTX_SS_BEGIN_VENDOR_RANGE
        && ctx.header.supercompressionScheme < KTX_SS_END_VENDOR_RANGE)
    {
        addIssue(logger::eWarning, HeaderData.VendorSupercompression);
    } else if (ctx.header.supercompressionScheme < KTX_SS_BEGIN_RANGE
        || ctx.header.supercompressionScheme > KTX_SS_END_RANGE)
    {
        addIssue(logger::eError, HeaderData.InvalidSupercompression,
                 ctx.header.supercompressionScheme);
    }

    if (ctx.header.vkFormat != VK_FORMAT_UNDEFINED) {
        if (ctx.header.supercompressionScheme != KTX_SS_BASIS_LZ) {
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
                if (ctx.header.levelCount == 0)
                    addIssue(logger::eError, HeaderData.ZeroLevelCountForBC);
            } else {
                if (ctx.header.typeSize != ctx.formatInfo.wordSize)
                     addIssue(logger::eError, HeaderData.TypeSizeMismatch);
            }
        } else {
            addIssue(logger::eError, HeaderData.VkFormatAndBasis);
        }
    } else {
        if (ctx.header.typeSize != 1)
            addIssue(logger::eError, HeaderData.TypeSizeNotOne);
    }

#define checkRequiredIndexEntry(index, issue, name)     \
    if (index.byteOffset == 0 || index.byteLength == 0) \
        addIssue(logger::eError, issue, name)

#define checkOptionalIndexEntry(index, issue, name)     \
    if (!index.byteOffset != !index.byteLength)         \
        addIssue(logger::eError, issue, name)

    checkRequiredIndexEntry(ctx.header.dataFormatDescriptor,
                    HeaderData.InvalidRequiredIndexEntry, "dfd");

    checkOptionalIndexEntry(ctx.header.keyValueData,
                    HeaderData.InvalidRequiredIndexEntry, "kvd");

    if (ctx.header.supercompressionScheme == KTX_SS_BASIS_LZ) {
        checkRequiredIndexEntry(ctx.header.supercompressionGlobalData,
                                HeaderData.InvalidOptionalIndexEntry, "sgd");
    } else {
        checkOptionalIndexEntry(ctx.header.supercompressionGlobalData,
                                HeaderData.InvalidOptionalIndexEntry, "sgd");
    }

    ctx.levelIndexSize = sizeof(ktxLevelIndexEntry) * ctx.levelCount;
    ctx.curFileOffset = KTX2_HEADER_SIZE + ctx.levelIndexSize;
    uint64_t offset = ctx.curFileOffset;
    if (offset != ctx.header.dataFormatDescriptor.byteOffset)
        addIssue(logger::eError, HeaderData.InvalidDFDOffset);
    offset += ctx.header.dataFormatDescriptor.byteLength;

    if (ctx.header.keyValueData.byteOffset != 0) {
        if (offset != ctx.header.keyValueData.byteOffset)
            addIssue(logger::eError, HeaderData.InvalidKVDOffset);
        offset += ctx.header.keyValueData.byteLength;
        if (ctx.header.supercompressionGlobalData.byteOffset != 0)
            // Pad before SGD.
            offset = padn(8, offset);
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
     if (ctx.fileRead(levelIndex, ctx.levelIndexSize, 1) != 1) {
        if (ctx.fileError())
            addIssue(logger::eFatal, IOError.FileRead, strerror(errno));
        else
            addIssue(logger::eFatal, IOError.UnexpectedEOF);
    }
    ctx.curFileOffset += ctx.levelIndexSize;

    uint32_t requiredLevelAlignment = ctx.requiredLevelAlignment();
    size_t expectedOffset;
    size_t lastByteLength = 0;
    switch (ctx.header.supercompressionScheme) {
      case KTX_SS_NONE:
      case KTX_SS_ZSTD:
        expectedOffset = padn(requiredLevelAlignment, ctx.kvDataEndOffset());
        break;
      case KTX_SS_BASIS_LZ:
        ktxIndexEntry64 sgdIndex = ctx.header.supercompressionGlobalData;
        // No padding here.
        expectedOffset = sgdIndex.byteOffset + sgdIndex.byteLength;
        break;
    }
    expectedOffset = padn(requiredLevelAlignment, expectedOffset);
    // Last mip level is first in the file. Count down so we can check the
    // distance between levels for the UNDEFINED and SUPERCOMPRESSION cases.
    for (int32_t level = ctx.levelCount-1; level >= 0; level--) {
        if (ctx.header.vkFormat != VK_FORMAT_UNDEFINED
            && ctx.header.supercompressionScheme == KTX_SS_NONE) {
            if (levelIndex[level].uncompressedByteLength !=
                ctx.calcLevelSize(level))
                addIssue(logger::eError, LevelIndex.IncorrectByteLength, level);

            if (levelIndex[level].byteLength !=
                levelIndex[level].uncompressedByteLength)
                addIssue(logger::eError, LevelIndex.IncorrectByteLength, level);

            ktx_size_t expectedByteOffset = ctx.calcLevelOffset(level);
            if (levelIndex[level].byteOffset != expectedByteOffset) {
                if (levelIndex[level].byteOffset % requiredLevelAlignment != 0)
                    addIssue(logger::eError, LevelIndex.UnalignedOffset,
                             level, requiredLevelAlignment);
                if (levelIndex[level].byteOffset > expectedByteOffset)
                    addIssue(logger::eError, LevelIndex.ExtraPadding, level);
                else
                    addIssue(logger::eError, LevelIndex.ByteOffsetTooSmall,
                             level);
            }
        } else {
            // Can only do minimal validation as we have no idea what the
            // level sizes are so we have to trust the byteLengths. We do
            // at least know where the first level must be in the file and
            // we can calculate how much padding, if any, there must be
            // between levels.
            if (levelIndex[level].byteLength == 0
                || levelIndex[level].byteOffset == 0) {
                 addIssue(logger::eError, LevelIndex.ZeroOffsetOrLength, level);
                 continue;
            }
            if (levelIndex[level].byteOffset != expectedOffset) {
                addIssue(logger::eError,
                         LevelIndex.IncorrectByteOffset,
                         level);
            }
            if (ctx.header.supercompressionScheme == KTX_SS_NONE) {
                if (levelIndex[level].byteLength < lastByteLength)
                    addIssue(logger.eError, LevelIndex.IncorrectLevelOrder);
                if (levelIndex[level].byteOffset % requiredLevelAlignment != 0)
                    addIssue(logger::eError, LevelIndex.UnalignedOffset,
                             level, requiredLevelAlignment);
                if (levelIndex[level].uncompressedByteLength == 0) {
                    addIssue(logger::eError, LevelIndex.ZeroUncompressedLength,
                             level);
                }
                lastByteLength = levelIndex[level].byteLength;
            }
            expectedOffset += padn(requiredLevelAlignment,
                                   levelIndex[level].byteLength);
        }
        ctx.dataSizeFromLevelIndex += padn(ctx.requiredLevelAlignment(),
                                           levelIndex[level].byteLength);
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
    ctx.pActualDfd = new uint32_t[ctx.header.dataFormatDescriptor.byteLength
                               / sizeof(uint32_t)];
    if (ctx.fileRead(ctx.pActualDfd, ctx.header.dataFormatDescriptor.byteLength, 1) != 1) {
        if (ctx.fileError())
            addIssue(logger::eFatal, IOError.FileRead, strerror(errno));
        else
            addIssue(logger::eFatal, IOError.UnexpectedEOF);
    }

    if (ctx.header.dataFormatDescriptor.byteLength != *ctx.pActualDfd)
        addIssue(logger::eError, DFD.SizeMismatch);

    uint32_t* bdb = ctx.pActualDfd + 1; // Basic descriptor block.

    uint32_t xferFunc;
    if ((xferFunc = KHR_DFDVAL(bdb, TRANSFER)) != KHR_DF_TRANSFER_SRGB
        && xferFunc != KHR_DF_TRANSFER_LINEAR)
        addIssue(logger::eError, DFD.InvalidTransferFunction);

    bool analyze = false;
    uint32_t numSamples = KHR_DFDSAMPLECOUNT(bdb);
    switch (ctx.header.supercompressionScheme) {
      case KTX_SS_NONE:
      case KTX_SS_ZSTD:
        if (ctx.header.vkFormat != VK_FORMAT_UNDEFINED) {
            if (ctx.header.supercompressionScheme != KTX_SS_ZSTD) {
                // Do a simple comparison with the expected DFD.
                analyze = memcmp(ctx.pActualDfd, ctx.pDfd4Format,
                                  *ctx.pDfd4Format);
            } else {
                // Compare up to BYTESPLANE.
                analyze = memcmp(ctx.pActualDfd, ctx.pDfd4Format,
                                  KHR_DF_WORD_BYTESPLANE0 * 4);
                // Check for unsized.
                if (bdb[KHR_DF_WORD_BYTESPLANE0]  != 0
                    || bdb[KHR_DF_WORD_BYTESPLANE4]  != 0)
                    addIssue(logger::eError, DFD.NotUnsized);
                // Compare the sample information.
                if (!analyze) {
                    analyze = memcmp(&ctx.pActualDfd[KHR_DF_WORD_SAMPLESTART+1],
                                    &ctx.pDfd4Format[KHR_DF_WORD_SAMPLESTART+1],
                                    numSamples * KHR_DF_WORD_SAMPLEWORDS);
                }
            }
        } else {
            if (KHR_DFDVAL(bdb, MODEL) == KHR_DF_MODEL_UASTC) {
                // Validate UASTC
                if (numSamples == 0)
                    addIssue(logger::eError, DFD.ZeroSamples, "UASTC");
                if (numSamples > 1)
                    addIssue(logger::eError, DFD.InvalidSampleCount,
                             "UASTC", "1");
                if (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) != 3
                    && KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) != 3
                    && (bdb[KHR_DF_WORD_TEXELBLOCKDIMENSION0] & 0xffff0000) != 0)
                    addIssue(logger::eError, DFD.InvalidTexelBlockDimension,
                             4, 4, "UASTC");
                if (ctx.header.supercompressionScheme != KTX_SS_ZSTD) {
                    if (KHR_DFDVAL(bdb, BYTESPLANE0) == 0)
                        addIssue(logger::eError, DFD.BytesPlane0Zero, "UASTC");
                } else {
                     if (KHR_DFDVAL(bdb, BYTESPLANE0) != 0) {
                          addIssue(logger::eError, DFD.NotUnsized, "UASTC");
                     }
                }
                uint8_t channelID = KHR_DFDSVAL(bdb, 0, CHANNELID);
                if (channelID != KHR_DF_CHANNEL_UASTC_RGB
                    && channelID != KHR_DF_CHANNEL_UASTC_RGBA
                    && channelID != KHR_DF_CHANNEL_UASTC_RRR
                    && channelID != KHR_DF_CHANNEL_UASTC_RRRG)
                    addIssue(logger::eError, DFD.InvalidChannelForUASTC);
                if (KHR_DFDSVAL(bdb, 0, BITOFFSET) != 0)
                    addIssue(logger::eError, DFD.InvalidBitOffsetForUASTC);
                if (KHR_DFDSVAL(bdb, 0, BITLENGTH) != 127)
                    addIssue(logger::eError, DFD.InvalidBitLength,
                             "UASTC", 127);
                if (KHR_DFDSVAL(bdb, 0, SAMPLELOWER) != 0
                    && KHR_DFDSVAL(bdb, 0, SAMPLEUPPER) != UINT32_MAX)
                    addIssue(logger::eError, DFD.InvalidLowerOrUpper, "UASTC");
            } else {
                // Check the basics
                if (KHR_DFDVAL(bdb, VENDORID) != KHR_DF_VENDORID_KHRONOS
                    || KHR_DFDVAL(bdb, DESCRIPTORTYPE) != KHR_DF_KHR_DESCRIPTORTYPE_BASICFORMAT
                    || KHR_DFDVAL(bdb, VERSIONNUMBER) < KHR_DF_VERSIONNUMBER_1_3)
                    addIssue(logger::eError, DFD.IncorrectBasics);

                // Ensure there are at least some samples
                if (KHR_DFDSAMPLECOUNT(bdb) == 0)
                    addIssue(logger::eError, DFD.ZeroSamples,
                             "non-supercompressed texture with VK_FORMAT_UNDEFINED");
                // Check for properly sized format
                // This checks texelBlockDimension[0-3] and bytesPlane[0-7]
                // as each is a byte and bdb is unit32_t*.
                if (bdb[KHR_DF_WORD_TEXELBLOCKDIMENSION0] == 0)
                    addIssue(logger::eError, DFD.TexelBlockDimensionZeroForUndefined);
                if (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION3) != 0)
                    addIssue(logger::eError, DFD.FourDimensionalTexturesNotSupported);
                if (ctx.header.supercompressionScheme != KTX_SS_ZSTD) {
                    if (KHR_DFDVAL(bdb, BYTESPLANE0) == 0)
                        addIssue(logger::eError, DFD.BytesPlane0Zero,
                                 "VK_FORMAT_UNDEFINED");
                } else {
                     if (KHR_DFDVAL(bdb, BYTESPLANE0) != 0) {
                          addIssue(logger::eError, DFD.NotUnsized);
                     }
                }
                if ((bdb[KHR_DF_WORD_BYTESPLANE0] & KHR_DF_MASK_BYTESPLANE0) != 0
                    || bdb[KHR_DF_WORD_BYTESPLANE4] != 0)
                    addIssue(logger::eError, DFD.MultiplaneFormatsNotSupported);
            }
        }
        break;

      case KTX_SS_BASIS_LZ:
          // validateHeader has already checked if vkFormat is the required
          // VK_FORMAT_UNDEFINED so no check here.

          // The colorModel must be ETC1S, currently the only format supported
          // with BasisLZ.
          if (KHR_DFDVAL(bdb, MODEL) != KHR_DF_MODEL_ETC1S)
              addIssue(logger::eError, DFD.IncorrectModelForBLZE);
          // This descriptor should have 1 or 2 samples with bitLength 63
          // and bitOffsets 0 and 64.
          if (numSamples == 0)
              addIssue(logger::eError, DFD.ZeroSamples, "BasisLZ/ETC1S");
          if (numSamples > 2)
              addIssue(logger::eError, DFD.InvalidSampleCount, "BasisLZ/ETC1S", "1 or 2");
          if (KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION0) != 3
              && KHR_DFDVAL(bdb, TEXELBLOCKDIMENSION1) != 3
              && (bdb[KHR_DF_WORD_TEXELBLOCKDIMENSION0] & 0xffff0000) != 0)
              addIssue(logger::eError, DFD.InvalidTexelBlockDimension,
                       4, 4, "BasisLZ/ETC1S");
          // Check for unsized.
          if (bdb[KHR_DF_WORD_BYTESPLANE0]  != 0
              || bdb[KHR_DF_WORD_BYTESPLANE4]  != 0)
              addIssue(logger::eError, DFD.NotUnsized);

          for (uint32_t sample = 0; sample < numSamples; sample++) {
              uint8_t channelID = KHR_DFDSVAL(bdb, sample, CHANNELID);
              if (channelID != KHR_DF_CHANNEL_ETC1S_RGB
                  && channelID != KHR_DF_CHANNEL_ETC1S_RRR
                  && channelID != KHR_DF_CHANNEL_ETC1S_GGG
                  && channelID != KHR_DF_CHANNEL_ETC1S_AAA)
                  addIssue(logger::eError, DFD.InvalidChannelForBLZE);
              int bo = KHR_DFDSVAL(bdb, sample, BITOFFSET);
              //if (KHR_DFDSVAL(bdb, sample, BITOFFSET) != sample == 0 ? 0 : 64)
              if (bo != (sample == 0 ? 0 : 64))
                  addIssue(logger::eError, DFD.InvalidBitOffsetForBLZE);
              if (KHR_DFDSVAL(bdb, sample, BITLENGTH) != 63)
                  addIssue(logger::eError, DFD.InvalidBitLength,
                           "BasisLZ/ETC1S", 63);
              if (KHR_DFDSVAL(bdb, sample, SAMPLELOWER) != 0
                  && KHR_DFDSVAL(bdb, sample, SAMPLEUPPER) != UINT32_MAX)
                  addIssue(logger::eError, DFD.InvalidLowerOrUpper,
                           "BasisLZ/ETC1S");
          }
          break;

      default:
        break;
    }

    if (analyze) {
        string vkFormatStr(vkFormatString((VkFormat)ctx.header.vkFormat));

        // ctx.pActualDfd differs from what is expected. To help developers, do a
        // more in depth analysis.
        if (KHR_DFDVAL(bdb, VENDORID) != KHR_DF_VENDORID_KHRONOS
            || KHR_DFDVAL(bdb, DESCRIPTORTYPE) != KHR_DF_KHR_DESCRIPTORTYPE_BASICFORMAT
            || KHR_DFDVAL(bdb, VERSIONNUMBER) < KHR_DF_VERSIONNUMBER_1_3)
            addIssue(logger::eError, DFD.IncorrectBasics);
        if (KHR_DFDSAMPLECOUNT(bdb) == 0)
            addIssue(logger::eError, DFD.ZeroSamples, vkFormatStr.c_str());
        if (!(bdb[KHR_DF_WORD_BYTESPLANE0] & ~KHR_DF_MASK_BYTESPLANE0))
             addIssue(logger::eError, DFD.BytesPlane0Zero, vkFormatStr.c_str());

        if (ctx.formatInfo.isBlockCompressed) {
            // _BLOCK formats.
            if (KHR_DFDVAL(bdb, MODEL) < KHR_DF_MODEL_DXT1A)
              addIssue(logger::eError, DFD.IncorrectModelForBlock);
        } else {
            InterpretedDFDChannel r, g, b, a;
            uint32_t componentByteLength;
            InterpretDFDResult result;

            result = interpretDFD(ctx.pActualDfd, &r, &g, &b, &a, &componentByteLength);
            if (result > i_UNSUPPORTED_ERROR_BIT) {
                switch (result) {
                  case i_UNSUPPORTED_CHANNEL_TYPES:
                    addIssue(logger::eError, DFD.InvalidColorModel);
                    break;
                  case i_UNSUPPORTED_MULTIPLE_PLANES:
                    addIssue(logger::eError, DFD.MultiplePlanes);
                    break;
                  case i_UNSUPPORTED_MIXED_CHANNELS:
                    addIssue(logger::eError, DFD.MixedChannels);
                    break;
                  case i_UNSUPPORTED_MULTIPLE_SAMPLE_LOCATIONS:
                    addIssue(logger::eError, DFD.Multisample);
                    break;
                  case i_UNSUPPORTED_NONTRIVIAL_ENDIANNESS:
                    addIssue(logger::eError, DFD.NonTrivialEndianness);
                    break;
                  default:
                    break;
                }
            } else {
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
                    // else here because Vulkan format names do not reflect
                    // both normalized and float. E.g, BC6H is just
                    // VK_FORMAT_BC6H_[SU]FLOAT_BLOCK.
                    else if (result & i_NORMALIZED_FORMAT_BIT)
                        findStr += "NORM";
                    else
                        findStr += "INT";

                    if (vkFormatStr.find(findStr) == string::npos)
                        addIssue(logger::eError, DFD.FormatMismatch);
                }
            }
        }
    }
}

void
ktxValidator::validateKvd(validationContext& ctx)
{
    uint32_t kvdLen = ctx.header.keyValueData.byteLength;
    uint32_t lengthCheck = 0;
    bool allKeysNulTerminated = true;

    if (kvdLen == 0)
        return;

    uint8_t* kvd = new uint8_t[kvdLen];
    if (ctx.fileRead(kvd, kvdLen, 1) != 1) {
        if (ctx.fileError())
            addIssue(logger::eFatal, IOError.FileRead, strerror(errno));
        else
            addIssue(logger::eFatal, IOError.UnexpectedEOF);
    }

    // Check all kv pairs have valuePadding and it's included in kvdLen;
    uint8_t* pCurKv = kvd;
    uint32_t safetyCount;
    // safetyCount ensures we don't get stuck in an infinite loop in the event
    // the kv data is completely bogus and the "lengths" never add up to kvdLen.
#define MAX_KVPAIRS 75
    for (safetyCount = 0; lengthCheck < kvdLen && safetyCount < MAX_KVPAIRS; safetyCount++) {
        uint32_t curKvLen = *(uint32_t *)pCurKv;
        lengthCheck += sizeof(uint32_t); // Add keyAndValueByteLength to total.
        pCurKv += sizeof(uint32_t); // Move pointer past keyAndValueByteLength.
        uint8_t* p = pCurKv;
        uint8_t* pCurKvEnd = pCurKv + curKvLen;

        // Check for BOM.
        bool bom = false;
        if (*p == 0xEF && *(p+1) == 0xBB && *(p+2) == 0xBF) {
            bom = true;
            p += 3;
        }
        for (; p < pCurKvEnd; p++) {
            if (*p == '\0')
              break;
        }
        bool noNul = (p == pCurKvEnd);
        if (noNul) {
            addIssue(logger::eError, Metadata.MissingNulTerminator, pCurKv);
            allKeysNulTerminated = false;
        }
        if (bom) {
            if (noNul)
                addIssue(logger::eError, Metadata.ForbiddenBOM1, pCurKv);
            else
                addIssue(logger::eError, Metadata.ForbiddenBOM2, pCurKv);
        }
        curKvLen = (uint32_t)padn(4, curKvLen);
        lengthCheck += curKvLen;
        pCurKv += curKvLen;
    }
    if (safetyCount == 75)
        addIssue(logger::eError, Metadata.InvalidStructure, MAX_KVPAIRS);
    else if (lengthCheck != kvdLen)
        addIssue(logger::eError, Metadata.MissingFinalPadding);

    ktxHashList kvDataHead = 0;
    ktxHashListEntry* entry;
    char* prevKey;
    uint32_t prevKeyLen;
    KTX_error_code result;
    bool writerFound = false;
    bool writerScParamsFound = false;

    if (allKeysNulTerminated) {
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
                if (strncmp(key, "KTXwriterScParams", 17) == 0)
                    writerScParamsFound = true;
            } else {
                addIssue(logger::eWarning, Metadata.CustomMetadata, key);
            }
        }
        if (!writerFound) {
            if (writerScParamsFound)
                addIssue(logger::eError, Metadata.NoRequiredKTXwriter);
            else
                addIssue(logger::eWarning, Metadata.NoKTXwriter);
        }
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
    ctx.cubemapIncompleteFound = true;
    if (valueLen != 1)
        addIssue(logger::eError, Metadata.InvalidValue, key);
}

void
ktxValidator::validateOrientation(validationContext& ctx, char* key,
                                  uint8_t* value, uint32_t valueLen)
{
    if (valueLen == 0) {
        addIssue(logger::eError, Metadata.MissingValue, key);
        return;
    }

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
        if (!regex_match((char*)value, regex("^[rl][du]$")))
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
ktxValidator::validateWriterScParams(validationContext& ctx, char* key,
                                     uint8_t* value, uint32_t valueLen)
{
    if (value[valueLen-1] != '\0')
        addIssue(logger::eError, Metadata.ValueNotNulTerminated, key);
}

void
ktxValidator::validateAstcDecodeMode(validationContext& ctx, char* key,
                                     uint8_t* value, uint32_t valueLen)
{
    if (valueLen == 0) {
        addIssue(logger::eError, Metadata.MissingValue, key);
        return;
    }

    if (!regex_match((char*)value, regex("rgb9e5"))
       && !regex_match((char*)value, regex("unorm8")))
         addIssue(logger::eError, Metadata.InvalidValue, key);

    if (!ctx.pActualDfd)
        return;

    uint32_t* bdb = ctx.pDfd4Format + 1;
    if (KHR_DFDVAL(bdb, MODEL) != KHR_DF_MODEL_ASTC) {
         addIssue(logger::eError, Metadata.NotAllowed, key,
                  "for non-ASTC texture formats");
    }
    if (KHR_DFDVAL(bdb, TRANSFER) == KHR_DF_TRANSFER_SRGB) {
         addIssue(logger::eError, Metadata.NotAllowed, key,
                  "with sRGB transfer function");
    }
}

void
ktxValidator::validateAnimData(validationContext& ctx, char* key,
                               uint8_t* value, uint32_t valueLen)
{
    if (ctx.cubemapIncompleteFound) {
         addIssue(logger::eError, Metadata.NotAllowed, key,
                  "together with KTXcubemapIncomplete");
    }
    if (ctx.layerCount == 0)
        addIssue(logger::eError, Metadata.NotAllowed, key,
                 "except with array textures");

    if (valueLen != sizeof(uint32_t) * 3)
        addIssue(logger::eError, Metadata.InvalidValue, key);
}

void
ktxValidator::validateSgd(validationContext& ctx)
{
    uint64_t sgdByteLength = ctx.header.supercompressionGlobalData.byteLength;
    if (ctx.header.supercompressionScheme == KTX_SS_BASIS_LZ) {
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
    if (ctx.fileRead(sgd, sgdByteLength, 1) != 1) {
        if (ctx.fileError())
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

    ktxBasisLzGlobalHeader& bgdh = *reinterpret_cast<ktxBasisLzGlobalHeader*>(sgd);
    uint32_t numSamples = KHR_DFDSAMPLECOUNT(ctx.pActualDfd + 1);

    uint64_t expectedBgdByteLength = sizeof(ktxBasisLzGlobalHeader)
                                   + sizeof(ktxBasisLzEtc1sImageDesc) * imageCount
                                   + bgdh.endpointsByteLength
                                   + bgdh.selectorsByteLength
                                   + bgdh.tablesByteLength;

    ktxBasisLzEtc1sImageDesc* imageDescs = BGD_ETC1S_IMAGE_DESCS(sgd);
    ktxBasisLzEtc1sImageDesc* image = imageDescs;
    for (; image < imageDescs + imageCount; image++) {
      if (image->imageFlags & ~eBUImageIsPframe)
            addIssue(logger::eError, SGD.InvalidImageFlagBit);
        // Crosscheck the DFD.
        if (image->alphaSliceByteOffset == 0 && numSamples == 2)
            addIssue(logger::eError, SGD.DfdMismatchAlpha);
        if (image->alphaSliceByteOffset > 0 && numSamples == 1)
            addIssue(logger::eError, SGD.DfdMismatchNoAlpha);
    }

    if (sgdByteLength != expectedBgdByteLength)
        addIssue(logger::eError, SGD.IncorrectGlobalDataSize);

    if (bgdh.extendedByteLength != 0)
        addIssue(logger::eError, SGD.ExtendedByteLengthNotZero);

    // Can't do anymore as we have no idea how many endpoints, etc there
    // should be.
    // TODO: attempt transcode
}

void
ktxValidator::validateDataSize(validationContext& ctx)
{
    // Expects to be called after validateSgd so current file offset is at
    // the start of the data.
    off_t dataSizeInFile;
    if (ctx.inf != stdin) {
        off_t dataStart = (off_t)ftello(ctx.inf);
        if (fseeko(ctx.inf, 0, SEEK_END) < 0)
            addIssue(logger::eFatal, IOError.FileSeekEndFailure,
                     strerror(errno));
        off_t dataEnd = (off_t)ftello(ctx.inf);
        if (dataEnd < 0)
            addIssue(logger::eFatal, IOError.FileTellFailure, strerror(errno));
        dataSizeInFile = dataEnd - dataStart;
    } else {
        // Is this going to be really slow?
        dataSizeInFile = 0;
        while (getc(ctx.inf) != EOF)
            dataSizeInFile++;
        if (ctx.fileError())
            addIssue(logger::eFatal, IOError.FileRead, strerror(errno));
    }
    if (dataSizeInFile != ctx.dataSizeFromLevelIndex)
        addIssue(logger::eError, FileError.IncorrectDataSize);
}

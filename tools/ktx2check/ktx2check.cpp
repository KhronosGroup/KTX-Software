// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 sts=4 expandtab:

//
// Copyright 2019-2020 The Khronos Group, Inc.
// SPDX-License-Identifier: Apache-2.0
//

#include "ktxapp.h"

#include <cstdlib>
#include <errno.h>
#include <math.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

#include <ktx.h>
#include <KHR/khr_df.h>

#include "ktxint.h"
#include "vkformat_enum.h"
#define LIBKTX // To stop dfdutils including vulkan_core.h.
#include "dfdutils/dfd.h"
#include "texture.h"
#include "basis_sgd.h"
#include "sbufstream.h"

// Gotta love Microsoft & Windows :-)
#if defined(_MSC_VER)
  #define strncasecmp _strnicmp
#endif
#include "version.h"

std::string myversion(STR(KTX2CHECK_VERSION));
std::string mydefversion(STR(KTX2CHECK_DEFAULT_VERSION));

#if !defined(BITFIELD_ORDER_FROM_MSB)
// This declaration is solely to make debugging of certain problems easier.
// Most compilers, including all those tested so far, including clang, gcc
// and msvc, order bitfields from the lsb so these struct declarations work.
// Possibly this is because I've only tested on little-endian machines?
struct sampleType {
    uint32_t bitOffset: 16;
    uint32_t bitLength: 8;
    uint32_t channelType: 8; // Includes qualifiers
    uint32_t samplePosition0: 8;
    uint32_t samplePosition1: 8;
    uint32_t samplePosition2: 8;
    uint32_t samplePosition3: 8;
    uint32_t lower;
    uint32_t upper;
};

struct BDFD {
    uint32_t vendorId: 17;
    uint32_t descriptorType: 15;
    uint32_t versionNumber: 16;
    uint32_t descriptorBlockSize: 16;
    uint32_t model: 8;
    uint32_t primaries: 8;
    uint32_t transfer: 8;
    uint32_t flags: 8;
    uint32_t texelBlockDimension0: 8;
    uint32_t texelBlockDimension1: 8;
    uint32_t texelBlockDimension2: 8;
    uint32_t texelBlockDimension3: 8;
    uint32_t bytesPlane0: 8;
    uint32_t bytesPlane1: 8;
    uint32_t bytesPlane2: 8;
    uint32_t bytesPlane3: 8;
    uint32_t bytesPlane4: 8;
    uint32_t bytesPlane5: 8;
    uint32_t bytesPlane6: 8;
    uint32_t bytesPlane7: 8;
    struct sampleType samples[6];
};
#endif

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
    <dt>-q, \--quiet</dt>
    <dd>Validate silently. Indicate valid or invalid via exit code.</dd>
    <dt>-m &lt;num&gt;, --max-issues &lt;num&gt;</dt>
    <dd>Set the maximum number of issues to be reported per file
        provided -q is not set.</dd>
    <dt>-w, \--warn-as-error</dt>
    <dd>Treat warnings as errors. Changes exit code from success to error.
    </dl>
    @snippetdoc ktxapp.h ktxApp options

@section ktx2check_exitstatus EXIT STATUS
    @b ktx2check exits 0 on success, 1 on command line errors and 2 on
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
#if defined(ERROR) // windows.h defines this and is included by ktxapp.h.
  #undef ERROR
#endif
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
    issue DfdValidationFailure {
        FATAL | 0x0042, "DFD validation passed a DFD which extactFormatInfo() could not handle."
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
        ERROR | 0x005a, "DFD bytesPlane0 must be non-zero for non-supercompressed %s texture."
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
        ERROR | 0x0068, "DFD has channels with differing flags, e.g. some float, some integer."
    };
    issue Multisample {
        ERROR | 0x0069, "DFD indicates multiple sample locations."
    };
    issue NonTrivialEndianness {
        ERROR | 0x006a, "DFD describes non little-endian data."
    };
    issue InvalidPrimaries {
        ERROR | 0x006b, "DFD primaries value, %d, is invalid."
    };
    issue SampleCountMismatch {
        ERROR | 0x006c, "DFD sample count %d differs from expected %d."
    };
    issue BytesPlane0Mismatch {
        ERROR | 0x006d, "DFD bytesPlane0 value %d differs from expected %d."
    };
} DFD;

struct {
    issue IncorrectByteLength {
        ERROR | 0x0070, "Level %d byteLength %#x does not match expected value %#x."
    };
    issue ByteOffsetTooSmall {
        ERROR | 0x0071, "Level %d byteOffset %#x is smaller than expected value %#x."
    };
    issue IncorrectByteOffset {
        ERROR | 0x0072, "Level %d byteOffset %#x does not match expected value %#x."
    };
    issue IncorrectUncompressedByteLength {
        ERROR | 0x0073, "Level %d uncompressedByteLength %#x does not match expected value %#x."
    };
    issue UnequalByteLengths {
        ERROR | 0x0074, "Level %d uncompressedByteLength does not match byteLength."
    };
    issue UnalignedOffset {
        ERROR | 0x0075, "Level %d byteOffset is not aligned to required %d byte alignment."
    };
    issue ExtraPadding {
        ERROR | 0x0076, "Level %d has disallowed extra padding."
    };
    issue ZeroOffsetOrLength {
        ERROR | 0x0077, "Level %d's byteOffset or byteLength is 0."
    };
    issue ZeroUncompressedLength {
        ERROR | 0x0078, "Level %d's uncompressedByteLength is 0."
    };
    issue IncorrectLevelOrder {
        ERROR | 0x0079, "Larger mip levels are before smaller."
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
        WARNING | 0x0088, "%s value missing encouraged NUL termination."
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

struct {
    issue Failure {
        ERROR | 0x0100, "Transcode of BasisU payload failed: %s"
    };
} Transcode;

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
//                        A RAIIfied ktxTexture.                   //
/////////////////////////////////////////////////////////////////////

template <typename T>
class KtxTexture final
{
public:
    KtxTexture(std::nullptr_t null = nullptr)
        : _handle{nullptr}
    {
        (void)null;
    }

    KtxTexture(T* handle)
        : _handle{handle}
    {
    }

    KtxTexture(const KtxTexture&) = delete;
    KtxTexture &operator=(const KtxTexture&) = delete;

    KtxTexture(KtxTexture&& toMove)
        : _handle{toMove._handle}
    {
        toMove._handle = nullptr;
    }

    KtxTexture &operator=(KtxTexture&& toMove)
    {
        _handle = toMove._handle;
        toMove._handle = nullptr;
        return *this;
    }

    ~KtxTexture()
    {
        if (_handle)
        {
            ktxTexture_Destroy(handle<ktxTexture>()); _handle = nullptr;
        }
    }

    template <typename U = T>
    inline U* handle() const
    {
        return reinterpret_cast<U*>(_handle);
    }

    template <typename U = T>
    inline U** pHandle()
    {
        return reinterpret_cast<U**>(&_handle);
    }

    inline operator T*() const
    {
        return _handle;
    }

private:
    T* _handle;
};

/////////////////////////////////////////////////////////////////////
//                    Validator Class Definition                   //
/////////////////////////////////////////////////////////////////////

class ktxValidator : public ktxApp {
  public:
    ktxValidator();

    virtual int main(int argc, char* argv[]);
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
        template<typename ... Args>
        void addIssue(severity severity, issue issue, Args ... args);
        void startFile(const std::string& filename) {
            // {error,warning}Count are cumulative so don't clear them.
            nameOfFileBeingValidated = filename;
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
        istream* inp;
        KTX_header2 header;
        size_t levelIndexSize;
        uint32_t layerCount;
        uint32_t levelCount;
        uint32_t dimensionCount;
        uint32_t* pDfd4Format;
        uint32_t* pActualDfd;
        uint64_t dataSizeFromLevelIndex;
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
            //inf = nullptr;
            inp = nullptr;
            pDfd4Format = nullptr;
            pActualDfd = nullptr;
            cubemapIncompleteFound = false;
            dataSizeFromLevelIndex = 0;
        }

        ~validationContext() {
            delete pDfd4Format;
            delete pActualDfd;
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

        bool extractFormatInfo(uint32_t* dfd) {
            uint32_t* bdb = dfd + 1;
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
                result = interpretDFD(dfd, &r, &g, &b, &a, &fi.wordSize);
                if (result > i_UNSUPPORTED_ERROR_BIT)
                    return false;
            }
            return true;
        }

        uint32_t requiredLevelAlignment() {
            if (header.supercompressionScheme != KTX_SS_NONE)
                return 1;
            else
                return lcm4(formatInfo.blockByteLength);
        }

        //
        // This KTX-specific function adds support for combined depth stencil
        // formats which are not supported by @e dfdutils' @c vk2dfd function
        // because they are not seen outside a Vulkan device. KTX has its own
        // definitions for these.
        //
        void createDfd4Format()
        {
            switch(header.vkFormat) {
              case VK_FORMAT_D16_UNORM_S8_UINT:
                // 2 16-bit words. D16 in the first. S8 in the 8 LSBs of the second.
                pDfd4Format = createDFDDepthStencil(16, 8, 4);
                break;
              case VK_FORMAT_D24_UNORM_S8_UINT:
                // 1 32-bit word. D24 in the MSBs. S8 in the LSBs.
                pDfd4Format = createDFDDepthStencil(24, 8, 4);
                break;
              case VK_FORMAT_D32_SFLOAT_S8_UINT:
                // 2 32-bit words. D32 float in the first word. S8 in LSBs of the
                // second.
                pDfd4Format = createDFDDepthStencil(32, 8, 8);
                break;
              default:
                pDfd4Format = vk2dfd((VkFormat)header.vkFormat);
            }
        }

        void init(istream* is) {
            delete pDfd4Format;
            inp = is;
            dataSizeFromLevelIndex = 0;
        }

        // Move read point from curOffset to next multiple of alignment bytes.
        // Use read not fseeko/setpos so stdin can be used.
        void skipPadding(uint32_t alignment) {
            uint32_t padLen = padn_len(alignment, inp->tellg());
            if (padLen) {
                inp->seekg(padLen, ios_base::cur);
            }
        }
    };

    // Using template because having a struct as last arg before the
    // variable args when using va_start etc. is non-portable.
    template <typename ... Args>
    void addIssue(logger::severity severity, issue issue, Args ... args)
    {
        logger.addIssue(severity, issue, args...);
    }
    virtual bool processOption(argparser& parser, int opt);
    void validateFile(const string&);
    void validateHeader(validationContext& ctx);
    void validateLevelIndex(validationContext& ctx);
    void validateDfd(validationContext& ctx);
    void validateKvd(validationContext& ctx);
    void validateSgd(validationContext& ctx);
    void validateDataSize(validationContext& ctx);
    bool validateTranscode(validationContext& ctx); // Must be called last.
    bool validateMetadata(validationContext& ctx, const char* key,
                          const uint8_t* value, uint32_t valueLen);

    typedef void (ktxValidator::*validateMetadataFunc)(validationContext& ctx,
                                                       const char* key,
                                                       const uint8_t* value,
                                                       uint32_t valueLen);
    void validateCubemapIncomplete(validationContext& ctx, const char* key,
                                   const uint8_t* value, uint32_t valueLen);
    void validateOrientation(validationContext& ctx, const char* key,
                             const uint8_t* value, uint32_t valueLen);
    void validateGlFormat(validationContext& ctx, const char* key,
                          const uint8_t* value, uint32_t valueLen);
    void validateDxgiFormat(validationContext& ctx, const char* key,
                            const uint8_t* value, uint32_t valueLen);
    void validateMetalPixelFormat(validationContext& ctx, const char* key,
                                  const uint8_t* value, uint32_t valueLen);
    void validateSwizzle(validationContext& ctx, const char* key,
                        const uint8_t* value, uint32_t valueLen);
    void validateWriter(validationContext& ctx, const char* key,
                        const uint8_t* value, uint32_t valueLen);
    void validateWriterScParams(validationContext& ctx, const char* key,
                                const uint8_t* value, uint32_t valueLen);
    void validateAstcDecodeMode(validationContext& ctx, const char* key,
                                const uint8_t* value, uint32_t valueLen);
    void validateAnimData(validationContext& ctx, const char* key,
                          const uint8_t* value, uint32_t valueLen);

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
        ctx.skipPadding(alignment);
        if (ctx.inp->fail())
            addIssue(logger::eFatal, IOError.FileRead, strerror(errno));
        else if (ctx.inp->eof())
            addIssue(logger::eFatal, IOError.UnexpectedEOF, 0);
    }
};

vector<ktxValidator::metadataValidator> ktxValidator::metadataValidators {
    // cubemapIncomplete must appear in this list before animData.
    { "KTXcubemapIncomplete", &ktxValidator::validateCubemapIncomplete },
    { "KTXorientation", &ktxValidator::validateOrientation },
    { "KTXglFormat", &ktxValidator::validateGlFormat },
    { "KTXdxgiFormat__", &ktxValidator::validateDxgiFormat },
    { "KTXmetalPixelFormat", &ktxValidator::validateMetalPixelFormat },
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
    short_opts += "qm:w";
}

void
streamout(stringstream& oss, const char* s, int length)
{
    // Can't find a way to get stringstream to truncate a stream.
    if (length != 0)
        oss.write(s, length);
    else
        oss << s;
}

template <typename T>
void streamout(stringstream&oss, T value, int)
{
    oss << value;
}

void
sprintf(stringstream& oss, const string& fmt)
{
    for (auto it = fmt.cbegin() ; it != fmt.cend(); ++it) {
        if (*it == '%' && *++it != '%')
            throw std::runtime_error("invalid format string: missing arguments");
        oss << *it;
    }
}

// Does not support repordering of arguments which would be needed for
// multi-language support. Don't know how to do that with variadic templates.
template <typename T, typename ... Args>
void
sprintf(stringstream& oss, const string& fmt, T value, Args ... args)
{
    for (size_t pos = 0; pos < fmt.size(); pos++) {
        if (fmt[pos] == '%' && fmt[++pos] != '%') {
            bool alternateForm = false;
            // Find the format character
            size_t fpos = fmt.find_first_of("diouXxfFeEgGaAcsb", pos);
            for (; pos < fpos; pos++) {
                switch (fmt[pos]) {
                  case '#':
                    alternateForm = true;
                    continue;
                  case '-':
                    oss << left;
                    continue;
                  case '+':
                    oss << showpos;
                    continue;
                  case ' ':
                    continue;
                  case '0':
                    if (!(oss.flags() & oss.left))
                        oss << setfill('0');
                    continue;
                  default:
                    break;
                }
                break;
            }
            try {
                size_t afterpos;
                int width = stoi(fmt.substr(pos, fpos - pos), &afterpos);
                oss << setw(width);
                pos += afterpos;
            } catch (invalid_argument& e) {
                (void)e;
            }
            int precision = 0;
            if (fmt[pos] == '.') try {
                size_t afterpos;
                ++pos;
                precision = stoi(fmt.substr(pos, fpos - pos), &afterpos);
                if (!std::is_same<T, const char*>::value) {
                     oss << setprecision(precision);
                     precision = 0;
                }
                pos += afterpos;
            } catch (invalid_argument& e) {
                throw std::runtime_error("Expected precision value in sprintf");
                (void)e;
            }
            if (fmt[pos] == 'x' || fmt[pos] == 'X') {
                oss << hex;
                if (alternateForm) oss << showbase;
            }
            // Having another function call sucks. See streamout for the reason.
            streamout(oss, value, precision);
            return sprintf(oss, fmt.substr(++pos), args...);
        }
        oss << fmt[pos];
    }
    throw std::runtime_error("extra arguments provided to sprintf");
}

// Why is severity passed here?
// -  Because it is convenient when browsing the code to see the severity
//    at the place an issue is raised.

template<typename ... Args>
void
ktxValidator::logger::addIssue(severity severity, issue issue, Args ... args)
{
    if (quiet) {
        switch (severity) {
          case eError:
            errorCount++;
            break;
          case eFatal:
            break;
          case eWarning:
            warningCount++;
            break;
        }
    } else {
        if (!headerWritten) {
            cout << "Issues in: " << nameOfFileBeingValidated << std::endl;
            headerWritten = true;
        }
        const uint32_t baseIndent = 4;
        uint32_t indent = 0;
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
            //fprintf(stdout, issue.message.c_str(), args...);
            std::stringstream oss;
            sprintf(oss, issue.message, args...);
            // Wrap lines on spaces.
            std::string message = oss.str();
            size_t nchars = message.size();
            uint32_t line = 0;
            uint32_t lsi = 0;  // line start index.
            uint32_t lei; // line end index
            while (nchars + indent > 80) {
                uint32_t ll; // line length
                lei = lsi + 79 - indent;
                while (message[lei] != ' ') lei--;
                ll = lei - lsi;
                for (uint32_t j = 0; j < (line ? indent : 0); j++) {
                    cout.put(' ');
                }
                cout.write(&message[lsi], ll) << std::endl;
                lsi = lei + 1; // +1 to skip the space
                nchars -= ll;
                line++;
            }
            for (uint32_t j = 0; j < (line ? baseIndent : 0); j++) {
                cout.put(' ');
            }
            cout.write(&message[lsi], nchars);
            cout << std::endl;
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

static ktxValidator ktxcheck;
ktxApp& theApp = ktxcheck;

int
ktxValidator::main(int argc, char *argv[])
{
    processCommandLine(argc, argv, eAllowStdin);

    logger.quiet = options.quiet;
    logger.maxIssues = options.maxIssues;

    vector<string>::const_iterator it;
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
    validationContext context;
    istream* isp;
    // These 2 need to be declared here so they stay around for the life
    // of this method.
    ifstream ifs;
    stringstream buffer;
    bool doBuffer;

    if (filename.compare("-") == 0) {
#if defined(_WIN32)
        /* Set "stdin" to have binary mode */
        (void)_setmode( _fileno( stdin ), _O_BINARY );
        // Windows shells set the FILE_SYNCHRONOUS_IO_NONALERT option when
        // creating pipes. Cygwin since 3.4.x does the same thing, a change
        // which affects anything dependent on it, e.g. Git for Windows
        // (since 2.41.0) and MSYS2. When this option is set, cin.seekg(0)
        // erroneously returns success. Always buffer.
        doBuffer = true;
#else
        // Can we seek in this cin?
        cin.seekg(0);
        doBuffer = cin.fail();
#endif
        if (doBuffer) {
            // Read entire file into a stringstream so we can seek.
            buffer << cin.rdbuf();
            buffer.seekg(0, ios::beg);
            isp = &buffer;
        } else {
            isp = &cin;
        }
    } else {
        // MS's STL has `open` overloads that accept wchar_t* and wstring to
        // handle Window's Unicode file names. Unfortunately non-MS STL has
        // only wchar_t*.
        ifs.open(DecodeUTF8Path(filename).c_str(),
                 ios_base::in | ios_base::binary);
        isp = &ifs;
    }

    logger.startFile(isp != &ifs ? "stdin" : filename);
    if (!isp->fail()) {
        try {
            context.init(isp);
            validateHeader(context);
            validateLevelIndex(context);
            // DFD is validated from within validateLevelIndex.
            validateKvd(context);
            if (context.header.supercompressionGlobalData.byteLength > 0)
                skipPadding(context, 8);
            validateSgd(context);
            skipPadding(context, context.requiredLevelAlignment());
            validateDataSize(context);
            validateTranscode(context);
        } catch (fatal& e) {
            if (!options.quiet)
                cout << "    " << e.what() << endl;
            throw;
        } catch (max_issues_exceeded& e) {
            cout << e.what() << endl;
        }
        if (isp == &ifs) ifs.close();
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
        break;
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

    ctx.inp->read((char *)&ctx.header, sizeof(KTX_header2));
    if (ctx.inp->fail())
        addIssue(logger::eFatal, IOError.FileRead, strerror(errno));
    else if (ctx.inp->eof())
        addIssue(logger::eFatal, IOError.UnexpectedEOF);

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
            ctx.createDfd4Format();
            if (ctx.pDfd4Format == nullptr) {
                addIssue(logger::eFatal, ValidatorError.CreateDfdFailure,
                         vkFormatString((VkFormat)ctx.header.vkFormat));
            } else if (!ctx.extractFormatInfo(ctx.pDfd4Format)) {
                addIssue(logger::eError, ValidatorError.IncorrectDfd,
                         vkFormatString((VkFormat)ctx.header.vkFormat));
            }

            if (ctx.formatInfo.isBlockCompressed) {
                if (ctx.header.typeSize != 1)
                    addIssue(logger::eError, HeaderData.TypeSizeNotOne);
                if (ctx.header.levelCount == 0)
                    addIssue(logger::eError, HeaderData.ZeroLevelCountForBC);
            } else {
                if (ctx.header.typeSize != ctx.formatInfo.wordSize)
                     addIssue(logger::eError, HeaderData.TypeSizeMismatch,
                              ctx.header.typeSize);
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
                    HeaderData.InvalidOptionalIndexEntry, "kvd");

    if (ctx.header.supercompressionScheme == KTX_SS_BASIS_LZ) {
        checkRequiredIndexEntry(ctx.header.supercompressionGlobalData,
                                HeaderData.InvalidRequiredIndexEntry, "sgd");
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
    ctx.inp->read((char *)levelIndex, ctx.levelIndexSize);
    if (ctx.inp->fail())
        addIssue(logger::eFatal, IOError.FileRead, strerror(errno));
    else if (ctx.inp->eof())
        addIssue(logger::eFatal, IOError.UnexpectedEOF);

    validateDfd(ctx);
    if (!ctx.pDfd4Format) {
        // VK_FORMAT_UNDEFINED so we have to get info from the actual DFD.
        // Not hugely robust but validateDfd does check known undefineds such
        // as UASTC.
        if (!ctx.extractFormatInfo(ctx.pActualDfd)) {
            addIssue(logger::eError, ValidatorError.DfdValidationFailure);
        }
    }

    uint32_t requiredLevelAlignment = ctx.requiredLevelAlignment();
    size_t expectedOffset = 0;
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
            ktx_size_t actualUBL = levelIndex[level].uncompressedByteLength;
            ktx_size_t expectedUBL = ctx.calcLevelSize(level);
            if (actualUBL != expectedUBL)
                addIssue(logger::eError,
                         LevelIndex.IncorrectUncompressedByteLength,
                         level, actualUBL, expectedUBL);

            if (levelIndex[level].byteLength !=
                levelIndex[level].uncompressedByteLength)
                addIssue(logger::eError, LevelIndex.UnequalByteLengths, level);

            ktx_size_t expectedByteOffset = ctx.calcLevelOffset(level);
            ktx_size_t actualByteOffset = levelIndex[level].byteOffset;
            if (actualByteOffset != expectedByteOffset) {
                if (actualByteOffset % requiredLevelAlignment != 0)
                    addIssue(logger::eError, LevelIndex.UnalignedOffset,
                             level, requiredLevelAlignment);
                if (levelIndex[level].byteOffset > expectedByteOffset)
                    addIssue(logger::eError, LevelIndex.ExtraPadding, level);
                else
                    addIssue(logger::eError, LevelIndex.ByteOffsetTooSmall,
                             level, actualByteOffset, expectedByteOffset);
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
                         level, levelIndex[level].byteOffset, expectedOffset);
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
            if (ctx.header.vkFormat != VK_FORMAT_UNDEFINED) {
                // We can validate the uncompressedByteLength.
                ktx_size_t actualUBL = levelIndex[level].uncompressedByteLength;
                ktx_size_t expectedUBL = ctx.calcLevelSize(level);
                if (actualUBL != expectedUBL)
                    addIssue(logger::eError,
                             LevelIndex.IncorrectUncompressedByteLength,
                             level, actualUBL, expectedUBL);
            }
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
    ctx.inp->read((char *)ctx.pActualDfd,
                  ctx.header.dataFormatDescriptor.byteLength);
    if (ctx.inp->fail())
        addIssue(logger::eFatal, IOError.FileRead, strerror(errno));
    else if (ctx.inp->eof())
        addIssue(logger::eFatal, IOError.UnexpectedEOF);

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
                uint32_t bytesPlane0 = KHR_DFDVAL(bdb, BYTESPLANE0);
                if (ctx.header.supercompressionScheme == KTX_SS_NONE) {
                    if (bytesPlane0 != 16) {
                        addIssue(logger::eError, DFD.BytesPlane0Mismatch,
                                 bytesPlane0, 16);
                    }
                } else {
                     if (bytesPlane0 != 0) {
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
                if (ctx.header.supercompressionScheme == KTX_SS_NONE) {
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
        // ctx.pActualDfd differs from what is expected. To help developers, do
        // a more in depth analysis.

        string vkFormatStr(vkFormatString((VkFormat)ctx.header.vkFormat));
        uint32_t* expBdb = ctx.pDfd4Format + 1; // Expected basic block.

        if (KHR_DFDVAL(bdb, VENDORID) != KHR_DF_VENDORID_KHRONOS
            || KHR_DFDVAL(bdb, DESCRIPTORTYPE) != KHR_DF_KHR_DESCRIPTORTYPE_BASICFORMAT
            || KHR_DFDVAL(bdb, VERSIONNUMBER) < KHR_DF_VERSIONNUMBER_1_3)
            addIssue(logger::eError, DFD.IncorrectBasics);

        khr_df_primaries_e aPrim, ePrim;
        aPrim = (khr_df_primaries_e)KHR_DFDVAL(bdb, PRIMARIES);
        ePrim = (khr_df_primaries_e)KHR_DFDVAL(expBdb, PRIMARIES);
        if (aPrim != ePrim) {
            // Okay. Any valid PRIMARIES value can be used. Check validity.
            if (aPrim < 0 || aPrim > KHR_DF_PRIMARIES_ADOBERGB)
                 addIssue(logger::eError, DFD.InvalidPrimaries, aPrim);
        }

        // Don't check flags because all the expected DFDs we create have
        // ALPHA_STRAIGHT but ALPHA_PREMULTIPLIED is also valid.

        int aVal, eVal;
        if (KHR_DFDSAMPLECOUNT(bdb) == 0) {
            addIssue(logger::eError, DFD.ZeroSamples, vkFormatStr.c_str());
        } else {
            aVal = KHR_DFDSAMPLECOUNT(bdb);
            eVal = KHR_DFDSAMPLECOUNT(expBdb);
            if (aVal != eVal)
                addIssue(logger::eError, DFD.SampleCountMismatch, aVal, eVal);
        }

        if (ctx.header.supercompressionScheme == KTX_SS_NONE) {
            // bP0 for supercompressed has already been checked.
            aVal = KHR_DFDVAL(bdb, BYTESPLANE0);
            eVal = KHR_DFDVAL(expBdb, BYTESPLANE0);
            if (aVal != eVal) {
                if (aVal == 0)
                    addIssue(logger::eError, DFD.BytesPlane0Zero,
                             vkFormatStr.c_str());
                else
                    addIssue(logger::eError, DFD.BytesPlane0Mismatch, aVal, eVal);
            }
        }

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
    ctx.inp->read((char *)kvd, kvdLen);
    if (ctx.inp->fail())
        addIssue(logger::eFatal, IOError.FileRead, strerror(errno));
    else if (ctx.inp->eof())
        addIssue(logger::eFatal, IOError.UnexpectedEOF);

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
ktxValidator::validateMetadata(validationContext& ctx, const char* key,
                               const uint8_t* pValue, uint32_t valueLen)
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
ktxValidator::validateCubemapIncomplete(validationContext& ctx,
                                        const char* key,
                                        const uint8_t*,
                                        uint32_t valueLen)
{
    ctx.cubemapIncompleteFound = true;
    if (valueLen != 1)
        addIssue(logger::eError, Metadata.InvalidValue, key);
}

void
ktxValidator::validateOrientation(validationContext& ctx,
                                  const char* key,
                                  const uint8_t* value,
                                  uint32_t valueLen)
{
    if (valueLen == 0) {
        addIssue(logger::eError, Metadata.MissingValue, key);
        return;
    }

    string orientation;
    const char* pOrientation = reinterpret_cast<const char*>(value);
    if (value[valueLen - 1] != '\0') {
        // regex_match on some platforms will fail to match an otherwise
        // valid swizzle due to lack of a NUL terminator even IF there is
        // no '$' at the end of the regex. Make a copy to avoid this.    
        orientation.assign(pOrientation, valueLen);
        pOrientation = orientation.c_str();
        addIssue(logger::eWarning, Metadata.ValueNotNulTerminated, key);
    }

    if (valueLen != ctx.dimensionCount + 1)
        addIssue(logger::eError, Metadata.InvalidValue, key);

    switch (ctx.dimensionCount) {
      case 1:
        if (!regex_match (pOrientation, regex("^[rl]$") ))
            addIssue(logger::eError, Metadata.InvalidValue, key);
        break;
      case 2:
        if (!regex_match(pOrientation, regex("^[rl][du]$")))
            addIssue(logger::eError, Metadata.InvalidValue, key);
        break;
      case 3:
        if (!regex_match(pOrientation, regex("^[rl][du][oi]$")))
            addIssue(logger::eError, Metadata.InvalidValue, key);
        break;
    }
}

void
ktxValidator::validateGlFormat(validationContext& /*ctx*/,
                               const char* key,
                               const uint8_t* /*value*/,
                               uint32_t valueLen)
{
    if (valueLen != sizeof(uint32_t) * 3)
        addIssue(logger::eError, Metadata.InvalidValue, key);
}

void
ktxValidator::validateDxgiFormat(validationContext& /*ctx*/,
                                 const char* key,
                                 const uint8_t* /*value*/,
                                 uint32_t valueLen)
                            {
    if (valueLen != sizeof(uint32_t))
        addIssue(logger::eError, Metadata.InvalidValue, key);}

void
ktxValidator::validateMetalPixelFormat(validationContext& /*ctx*/,
                                       const char* key,
                                       const uint8_t* /*value*/,
                                       uint32_t valueLen)
{
    if (valueLen != sizeof(uint32_t))
        addIssue(logger::eError, Metadata.InvalidValue, key);
}

void
ktxValidator::validateSwizzle(validationContext& /*ctx*/,
                              const char* key,
                              const uint8_t* value,
                              uint32_t valueLen)
{
    string swizzle;
    const char* pSwizzle = reinterpret_cast<const char*>(value);
    if (value[valueLen - 1] != '\0') {
        addIssue(logger::eWarning, Metadata.ValueNotNulTerminated, key);
        // See comment in validateOrientation.    
        swizzle.assign(pSwizzle, valueLen);
        pSwizzle = swizzle.c_str();
    }
    if (!regex_match(pSwizzle, regex("^[rgba01]{4}$")))
        addIssue(logger::eError, Metadata.InvalidValue, key);
}

void
ktxValidator::validateWriter(validationContext& /*ctx*/,
                             const char* key,
                             const uint8_t* value,
                             uint32_t valueLen)
{
    if (value[valueLen-1] != '\0')
        addIssue(logger::eWarning, Metadata.ValueNotNulTerminated, key);
}

void
ktxValidator::validateWriterScParams(validationContext& /*ctx*/,
                                     const char* key,
                                     const uint8_t* value,
                                     uint32_t valueLen)
{
    if (value[valueLen-1] != '\0')
        addIssue(logger::eWarning, Metadata.ValueNotNulTerminated, key);
}

void
ktxValidator::validateAstcDecodeMode(validationContext& ctx,
                                     const char* key,
                                     const uint8_t* value,
                                     uint32_t valueLen)
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
ktxValidator::validateAnimData(validationContext& ctx,
                               const char* key,
                               const uint8_t* /*value*/,
                               uint32_t valueLen)
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
    ctx.inp->read((char *)sgd, sgdByteLength);
    if (ctx.inp->fail())
        addIssue(logger::eFatal, IOError.FileRead, strerror(errno));
    else if (ctx.inp->eof())
        addIssue(logger::eFatal, IOError.UnexpectedEOF);

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
      if (image->imageFlags & ~ETC1S_P_FRAME)
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
    uint64_t dataSizeInFile;
    off_t dataStart = (off_t)(ctx.inp->tellg());

    ctx.inp->seekg(0, ios_base::end);
    if (ctx.inp->fail())
        addIssue(logger::eFatal, IOError.FileSeekEndFailure,
                 strerror(errno));
    off_t dataEnd = (off_t)(ctx.inp->tellg());
    if (dataEnd < 0)
        addIssue(logger::eFatal, IOError.FileTellFailure, strerror(errno));
    dataSizeInFile = dataEnd - dataStart;
    if (dataSizeInFile != ctx.dataSizeFromLevelIndex)
        addIssue(logger::eError, FileError.IncorrectDataSize);
}

// Must be called last as it rewinds the file.
bool
ktxValidator::validateTranscode(validationContext& ctx)
{
    uint32_t* bdb = ctx.pActualDfd + 1; // Basic descriptor block.
    uint32_t model = KHR_DFDVAL(bdb, MODEL);
    if (model != KHR_DF_MODEL_UASTC && model != KHR_DF_MODEL_ETC1S) {
        // Nothin to do. Not transcodable.
        return true;
    }

    bool retval;
    istream& is = *ctx.inp;
    is.seekg(0);
    streambuf* _streambuf = (is.rdbuf());
    StreambufStream<streambuf*> ktx2Stream(_streambuf, ios::in);
    KtxTexture<ktxTexture2> texture2;
    ktx_error_code_e result = ktxTexture2_CreateFromStream(ktx2Stream.stream(),
                                        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                        texture2.pHandle());
    if (result != KTX_SUCCESS) {
        addIssue(logger::eError, FileError.CreateFailure,
                 ktxErrorString(result));
        retval = false;
    }

    if (model == KHR_DF_MODEL_ETC1S)
        result = ktxTexture2_TranscodeBasis(texture2.handle(),
                                            KTX_TTF_ETC2_RGBA, 0);
    else
        result = ktxTexture2_TranscodeBasis(texture2.handle(),
                                            KTX_TTF_ASTC_4x4_RGBA,  0);
    if (result != KTX_SUCCESS) {
        addIssue(logger::eError, Transcode.Failure, ktxErrorString(result));
        retval = false;
    } else {
        retval = true;
    }
    return retval;
}


// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include "validate.h"

#include <ktx.h>
#include "ktxint.h"
#include "basis_sgd.h"
#define LIBKTX // To stop dfdutils including vulkan_core.h.
#include "dfdutils/dfd.h"
#include <KHR/khr_df.h>
#include <fmt/format.h>
#include <fmt/printf.h>

#include <algorithm>
#include <array>
#include <numeric>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <sstream>
#include <fmt/ostream.h>
#include <fmt/printf.h>

#include "utility.h"
#include "validation_messages.h"
#include "formats.h"
#include "sbufstream.h"


// -------------------------------------------------------------------------------------------------

namespace ktx {

struct DFDHeader {
    uint32_t vendorId: 17;
    uint32_t descriptorType: 15;
    uint32_t versionNumber: 16;
    uint32_t descriptorBlockSize: 16;
};
static_assert(sizeof(DFDHeader) == 8);

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
    std::array<uint8_t, 8> bytesPlanes;

    [[nodiscard]] bool matchTexelBlockDimensions(uint8_t dim0, uint8_t dim1, uint8_t dim2, uint8_t dim3) const {
        return texelBlockDimension0 == dim0
                && texelBlockDimension1 == dim1
                && texelBlockDimension2 == dim2
                && texelBlockDimension3 == dim3;
    }

    [[nodiscard]] bool hasNonZeroBytePlane() const {
        return bytesPlanes[0] != 0 || bytesPlanes[1] != 0
                || bytesPlanes[2] != 0 || bytesPlanes[3] != 0
                || bytesPlanes[4] != 0 || bytesPlanes[5] != 0
                || bytesPlanes[6] != 0 || bytesPlanes[7] != 0;
    }
};
static_assert(sizeof(BDFD) == 24);

struct SampleType {
    uint32_t bitOffset: 16;
    uint32_t bitLength: 8;
    uint32_t channelType: 4;
    uint32_t qualifierLinear: 1;
    uint32_t qualifierExponent: 1;
    uint32_t qualifierSigned: 1;
    uint32_t qualifierFloat: 1;
    uint32_t samplePosition0: 8;
    uint32_t samplePosition1: 8;
    uint32_t samplePosition2: 8;
    uint32_t samplePosition3: 8;
    uint32_t lower;
    uint32_t upper;
};
static_assert(sizeof(SampleType) == 16);

class FatalValidationError : public std::runtime_error {
public:
    ValidationReport report;

public:
    explicit FatalValidationError(ValidationReport report) :
        std::runtime_error(report.details),
        report(std::move(report)) {}
};

static constexpr uint32_t MAX_NUM_DFD_BLOCKS = 10;
static constexpr uint32_t MAX_NUM_BDFD_SAMPLES = 16;
static constexpr uint32_t MAX_NUM_KV_ENTRIES = 100;

// -------------------------------------------------------------------------------------------------

struct ValidationContext {
private:
    std::function<void(const ValidationReport&)> callback;

    bool treatWarningsAsError = false;
    bool checkGLTFBasisU = false;

    int returnCode = +rc::SUCCESS;;
    uint32_t numError = 0;
    uint32_t numWarning = 0;

private:
    KTX_header2 header{};

    uint32_t numLayers = 0; // The actual number of layers, After header parsing always at least one
    uint32_t numLevels = 0; // The actual number of levels, After header parsing always at least one
    uint32_t dimensionCount = 0;
    uint32_t numSamples = 0;

    std::vector<ktxLevelIndexEntry> levelIndices;

private:
    /// Expected data members are calculated solely from the VkFormat in the header.
    /// Based on parsing and support any of these member can be empty.
    std::optional<khr_df_model_e> expectedColorModel;
    std::optional<std::array<uint8_t, 8>> expectedBytePlanes;
    std::optional<uint8_t> expectedBlockDimension0;
    std::optional<uint8_t> expectedBlockDimension1;
    std::optional<uint8_t> expectedBlockDimension2;
    std::optional<uint8_t> expectedBlockDimension3;
    std::optional<bool> expectedColorModelIsBlockCompressed;
    std::optional<uint32_t> expectedTypeSize;
    std::optional<std::vector<SampleType>> expectedSamples;

private:
    /// The actually parsed BDFD members.
    /// Based on parsing and support any of these member can be empty.
    std::optional<khr_df_model_e> parsedColorModel;
    std::optional<khr_df_transfer_e> parsedTransferFunction;
    std::optional<uint8_t> parsedBlockByteLength;
    std::optional<uint8_t> parsedBlockDimension0; /// X
    std::optional<uint8_t> parsedBlockDimension1; /// Y
    std::optional<uint8_t> parsedBlockDimension2; /// Z

private:
    bool foundKTXanimData = false;
    bool foundKTXastcDecodeMode = false;
    bool foundKTXcubemapIncomplete = false;
    bool foundKTXdxgiFormat = false;
    bool foundKTXglFormat = false;
    bool foundKTXmetalPixelFormat = false;
    bool foundKTXorientation = false;
    bool foundKTXswizzle = false;
    bool foundKTXwriter = false;
    bool foundKTXwriterScParams = false;

public:
    ValidationContext(bool warningsAsErrors, bool GLTFBasisU, std::function<void(const ValidationReport&)> callback) :
        callback(std::move(callback)),
        treatWarningsAsError(warningsAsErrors),
        checkGLTFBasisU(GLTFBasisU) {
        std::memset(&header, 0, sizeof(header));
    }
    virtual ~ValidationContext() = default;

protected:
    // warning, error and fatal member functions are only used for validation readability
    template <typename... Args>
    void warning(const IssueWarning& issue, Args&&... args) {
        ++numWarning;
        if (treatWarningsAsError) {
            returnCode = +rc::INVALID_FILE;
            callback(ValidationReport{IssueType::error, issue.id, std::string{issue.message}, fmt::format(issue.detailsFmt, std::forward<Args>(args)...)});

        } else {
            callback(ValidationReport{issue.type, issue.id, std::string{issue.message}, fmt::format(issue.detailsFmt, std::forward<Args>(args)...)});
        }
    }

    template <typename... Args>
    void error(const IssueError& issue, Args&&... args) {
        ++numError;
        returnCode = +rc::INVALID_FILE;
        callback(ValidationReport{issue.type, issue.id, std::string{issue.message}, fmt::format(issue.detailsFmt, std::forward<Args>(args)...)});
    }

    template <typename... Args>
    void fatal(const IssueFatal& issue, Args&&... args) {
        ++numError;
        returnCode = +rc::INVALID_FILE;
        const auto report = ValidationReport{issue.type, issue.id, std::string{issue.message}, fmt::format(issue.detailsFmt, std::forward<Args>(args)...)};
        callback(report);

        throw FatalValidationError(report);
    }

private:
    virtual void read(std::size_t offset, void* readDst, std::size_t readSize, std::string_view name) = 0;
    virtual KTX_error_code createKTXTexture(ktxTextureCreateFlags createFlags, ktxTexture2** newTex) = 0;

private:
    template <typename... Args>
    void validateAlignmentPaddingZeros(const void* ptr, const void* bufferEnd, std::size_t alignment, const IssueError& issue, Args&&... args) {
        const auto* begin = static_cast<const char*>(ptr);
        const auto* end = std::min(bufferEnd, align(ptr, alignment));

        for (auto it = begin; it < end; ++it)
            if (*it != 0)
                error(issue, static_cast<uint8_t>(*it), std::forward<Args>(args)...);
    }

public:
    int validate(bool doCreateAndTranscodeChecks = true);

private:
    void validateHeader();
    void validateIndices();
    void calculateExpectedDFD(VkFormat format);
    void validateLevelIndex();
    void validateDFD();
    void validateKVD();
    void validateSGD();
    void validatePaddings();
    void validateCreateAndTranscode();

    void validateDFDBasic(uint32_t blockIndex, const uint32_t* dfd, const BDFD& block, const std::vector<SampleType>& samples);

    void validateKTXcubemapIncomplete(const uint8_t* data, uint32_t size);
    void validateKTXorientation(const uint8_t* data, uint32_t size);
    void validateKTXglFormat(const uint8_t* data, uint32_t size);
    void validateKTXdxgiFormat(const uint8_t* data, uint32_t size);
    void validateKTXmetalPixelFormat(const uint8_t* data, uint32_t size);
    void validateKTXswizzle(const uint8_t* data, uint32_t size);
    void validateKTXwriter(const uint8_t* data, uint32_t size);
    void validateKTXwriterScParams(const uint8_t* data, uint32_t size);
    void validateKTXastcDecodeMode(const uint8_t* data, uint32_t size);
    void validateKTXanimData(const uint8_t* data, uint32_t size);

private:
    size_t calcImageSize(uint32_t level);
    size_t calcLayerSize(uint32_t level);
    size_t calcLevelSize(uint32_t level);
    size_t calcLevelOffset(size_t firstLevelOffset, size_t alignment, uint32_t level);
};

// -------------------------------------------------------------------------------------------------

class ValidationContextIOStream : public ValidationContext {
private:
    std::ifstream file; /// Only used if the file is owned by the validation context
    std::istream& stream;
    std::optional<StreambufStream<std::streambuf*>> ktx2Stream;

public:
    ValidationContextIOStream(bool warningsAsErrors, bool GLTFBasisU, std::function<void(const ValidationReport&)> callback, std::istream& stream, const std::string& filepath) :
        ValidationContext(warningsAsErrors, GLTFBasisU, std::move(callback)),
        stream(stream) {

        if (!stream)
            fatal(IOError::FileOpen, filepath, errnoMessage());
    }

    ValidationContextIOStream(bool warningsAsErrors, bool GLTFBasisU, std::function<void(const ValidationReport&)> callback, const std::string& filepath) :
        ValidationContext(warningsAsErrors, GLTFBasisU, std::move(callback)),
        file(filepath, std::ios::in | std::ios::binary),
        stream(file) {

        if (!file)
            fatal(IOError::FileOpen, filepath, errnoMessage());
    }

private:
    virtual void read(std::size_t offset, void* readDst, std::size_t readSize, std::string_view name) override {
        stream.seekg(offset);
        if (!stream)
            fatal(IOError::FileSeekFailure, offset, name, errnoMessage());

        stream.read(reinterpret_cast<char*>(readDst), readSize);
        const auto bytesRead = stream.gcount();
        if (stream.eof()) {
            fatal(IOError::UnexpectedEOF, readSize, offset, name, bytesRead);
        } else if (stream.fail()) {
            fatal(IOError::FileReadFailure, readSize, bytesRead, offset, name, errnoMessage());
        }
    }

    virtual KTX_error_code createKTXTexture(ktxTextureCreateFlags createFlags, ktxTexture2** newTex) override {
        stream.seekg(0);
        if (!stream)
            fatal(IOError::RewindFailure, errnoMessage());

        ktx2Stream.emplace(stream.rdbuf(), std::ios::in | std::ios::binary);
        return ktxTexture2_CreateFromStream(ktx2Stream->stream(), createFlags, newTex);
    }
};

class ValidationContextStdioStream : public ValidationContext {
private:
    FILE* file;

public:
    ValidationContextStdioStream(bool warningsAsErrors, bool GLTFBasisU, std::function<void(const ValidationReport&)> callback, FILE* file, const std::string& filepath) :
        ValidationContext(warningsAsErrors, GLTFBasisU, std::move(callback)),
        file(file) {

        if (!file)
            fatal(IOError::FileOpen, filepath, errnoMessage());
    }

private:
    virtual void read(std::size_t offset, void* readDst, std::size_t readSize, std::string_view name) override {
        const auto seekResult = fseek(file, static_cast<long>(offset), SEEK_SET);
        if (seekResult != 0)
            fatal(IOError::FileSeekFailure, offset, name, seekResult);

        const auto bytesRead = fread(readDst, 1, readSize, file);
        if (bytesRead != readSize) {
            if (feof(file))
                fatal(IOError::UnexpectedEOF, readSize, offset, name, bytesRead);
            else
                fatal(IOError::FileReadFailure, readSize, bytesRead, offset, name, ferror(file));
        }
    }

    virtual KTX_error_code createKTXTexture(ktxTextureCreateFlags createFlags, ktxTexture2** newTex) override {
        const auto seekResult = fseek(file, 0, SEEK_SET);
        if (seekResult != 0)
            fatal(IOError::RewindFailure, seekResult);

        return ktxTexture2_CreateFromStdioStream(file, createFlags, newTex);
    }
};

class ValidationContextMemory : public ValidationContext {
private:
    const char* memoryData = nullptr;
    std::size_t memorySize = 0;

public:
    ValidationContextMemory(
            bool warningsAsErrors,
            bool GLTFBasisU,
            std::function<void(const ValidationReport&)> callback,
            const char* memoryData,
            std::size_t memorySize) :
        ValidationContext(warningsAsErrors, GLTFBasisU, std::move(callback)),
        memoryData(memoryData),
        memorySize(memorySize) {}

private:
    virtual void read(std::size_t offset, void* readDst, std::size_t readSize, std::string_view name) override {
        const auto bytesAvailable = offset > memorySize ? 0 : memorySize - offset;

        if (bytesAvailable < readSize)
            fatal(IOError::UnexpectedEOF, readSize, offset, name, bytesAvailable);

        std::memcpy(readDst, memoryData + offset, readSize);
    }

    virtual KTX_error_code createKTXTexture(ktxTextureCreateFlags createFlags, ktxTexture2** newTex) override {
        return ktxTexture2_CreateFromMemory(reinterpret_cast<const ktx_uint8_t*>(memoryData), memorySize, createFlags, newTex);
    }
};

// -------------------------------------------------------------------------------------------------

void validateToolInput(std::istream& stream, const std::string& filepath, Reporter& report) {
    if (!stream)
        report.fatal(rc::IO_FAILURE, "Could not open input file \"{}\": {}", filepath, errnoMessage());

    auto callback = [&](const ValidationReport& issue) {
        fmt::print(std::cerr, "{}-{:04}: {}\n", toString(issue.type), issue.id, issue.message);
        fmt::print(std::cerr, "    {}\n", issue.details);
    };
    const auto validationResult = validateIOStream(stream, filepath, false, false, callback);

    if (validationResult != +rc::SUCCESS)
        throw FatalError(ReturnCode{validationResult});

    stream.seekg(0);
    if (!stream)
        report.fatal(rc::IO_FAILURE, "Could not rewind the input file \"{}\": {}", filepath, errnoMessage());
}

int validateIOStream(std::istream& stream, const std::string& filepath, bool warningsAsErrors, bool GLTFBasisU, std::function<void(const ValidationReport&)> callback) {
    try {
        ValidationContextIOStream ctx{warningsAsErrors, GLTFBasisU, std::move(callback), stream, filepath};
        return ctx.validate();
    } catch (const FatalValidationError&) {
        return +rc::INVALID_FILE;
    }
}

int validateMemory(const char* data, std::size_t size, bool warningsAsErrors, bool GLTFBasisU, std::function<void(const ValidationReport&)> callback) {
    try {
        ValidationContextMemory ctx{warningsAsErrors, GLTFBasisU, std::move(callback), data, size};
        return ctx.validate();
    } catch (const FatalValidationError&) {
        return +rc::INVALID_FILE;
    }
}

int validateNamedFile(const std::string& filepath, bool warningsAsErrors, bool GLTFBasisU, std::function<void(const ValidationReport&)> callback) {
    try {
        ValidationContextIOStream ctx{warningsAsErrors, GLTFBasisU, std::move(callback), filepath};
        return ctx.validate();
    } catch (const FatalValidationError&) {
        return +rc::INVALID_FILE;
    }
}

int validateStdioStream(FILE* file, const std::string& filepath, bool warningsAsErrors, bool GLTFBasisU, std::function<void(const ValidationReport&)> callback) {
    try {
        ValidationContextStdioStream ctx{warningsAsErrors, GLTFBasisU, std::move(callback), file, filepath};
        return ctx.validate();
    } catch (const FatalValidationError&) {
        return +rc::INVALID_FILE;
    }
}

// -------------------------------------------------------------------------------------------------

int ValidationContext::validate(bool doCreateAndTranscodeChecks) {
    const auto call = [&](const auto& fn, std::string_view name, auto&&... args) {
        try {
            (this->*fn)(std::forward<decltype(args)>(args)...);
        } catch (const std::bad_alloc& ex) {
            error(System::OutOfMemory, name, ex.what());
        }
    };

    call(&ValidationContext::validateHeader, "Header");
    call(&ValidationContext::validateIndices, "Index");
    call(&ValidationContext::calculateExpectedDFD, "Expected DFD", VkFormat(header.vkFormat));
    call(&ValidationContext::validateDFD, "DFD");
    call(&ValidationContext::validateLevelIndex, "Level Index"); // Must come after the DFD parsed
    call(&ValidationContext::validateKVD, "KVD");
    call(&ValidationContext::validateSGD, "SGD");
    call(&ValidationContext::validatePaddings, "padding");
    if (doCreateAndTranscodeChecks)
        call(&ValidationContext::validateCreateAndTranscode, "Create and Transcode");

    return returnCode;
}

void ValidationContext::validateHeader() {
    static constexpr uint8_t ktx2_identifier_reference[12] = KTX2_IDENTIFIER_REF;

    read(0, &header, sizeof(KTX_header2), "the header");
    const auto vkFormat = VkFormat(header.vkFormat);
    const auto supercompressionScheme = ktxSupercmpScheme(header.supercompressionScheme);

    // Validate file identifier
    if (std::memcmp(&header.identifier, ktx2_identifier_reference, 12) != 0)
        fatal(FileError::NotKTX2);

    // Validate vkFormat
    if (isProhibitedFormat(vkFormat)) {
        error(HeaderData::ProhibitedFormat, toString(vkFormat));

    } else if (!isFormatValid(vkFormat)) {
        if (vkFormat <= VK_FORMAT_MAX_STANDARD_ENUM)
            error(HeaderData::InvalidFormat, toString(vkFormat));
        if (VK_FORMAT_MAX_STANDARD_ENUM < vkFormat && vkFormat < 1000001000)
            error(HeaderData::InvalidFormat, toString(vkFormat));
        if (1000001000 <= vkFormat && !isFormatKnown(vkFormat))
            warning(HeaderData::UnknownFormat, toString(vkFormat));
    }

    if (header.supercompressionScheme == KTX_SS_BASIS_LZ) {
        if (header.vkFormat != VK_FORMAT_UNDEFINED)
            error(HeaderData::VkFormatAndBasis, toString(vkFormat));
    }

    // Validate typeSize
    if (header.vkFormat == VK_FORMAT_UNDEFINED) {
        if (header.typeSize != 1)
            error(HeaderData::TypeSizeNotOne, header.typeSize, toString(vkFormat));

    } else if (isFormatBlockCompressed(vkFormat)) {
        if (header.typeSize != 1)
            error(HeaderData::TypeSizeNotOne, header.typeSize, toString(vkFormat));
    }
    // Additional checks are performed on typeSize after the DFD is parsed

    // Validate image dimensions
    if (header.pixelWidth == 0)
        error(HeaderData::WidthZero);

    if (isFormatBlockCompressed(vkFormat))
        if (header.pixelHeight == 0)
            error(HeaderData::BlockCompressedNoHeight, toString(vkFormat));
    if (isSupercompressionBlockCompressed(supercompressionScheme))
        if (header.pixelHeight == 0)
            error(HeaderData::BlockCompressedNoHeight, toString(supercompressionScheme));
    // Additional block-compressed formats (like UASTC) are detected after the DFD is parsed to validate pixelHeight

    if (header.faceCount == 6)
        if (header.pixelWidth != header.pixelHeight)
            error(HeaderData::CubeHeightWidthMismatch, header.pixelWidth, header.pixelHeight);

    if (header.pixelDepth != 0 && header.pixelHeight == 0)
        error(HeaderData::DepthNoHeight, header.pixelDepth);

    if (isFormat3DBlockCompressed(vkFormat))
        if (header.pixelDepth == 0)
            error(HeaderData::DepthBlockCompressedNoDepth, toString(vkFormat));

    if (isFormatDepth(vkFormat) || isFormatStencil(vkFormat))
        if (header.pixelDepth != 0)
            error(HeaderData::DepthStencilFormatWithDepth, header.pixelDepth, toString(vkFormat));

    if (header.faceCount == 6)
        if (header.pixelDepth != 0)
            error(HeaderData::CubeWithDepth, header.pixelDepth);

    // Detect dimension counts
    if (header.pixelDepth != 0) {
        dimensionCount = 3;
        if (header.layerCount != 0)
            warning(HeaderData::ThreeDArray); // Warning on 3D Array textures
    } else if (header.pixelHeight != 0) {
        dimensionCount = 2;
    } else {
        dimensionCount = 1;
    }

    // Validate layerCount to actual number of layers.
    numLayers = std::max(header.layerCount, 1u);

    // Validate faceCount
    if (header.faceCount != 6 && header.faceCount != 1)
        error(HeaderData::InvalidFaceCount, header.faceCount);

    // 2D Cube map faces were validated by CubeHeightWidthMismatch and CubeWithDepth

    // Validate levelCount
    if (isFormatBlockCompressed(vkFormat))
        if (header.levelCount == 0)
            error(HeaderData::BlockCompressedNoLevel, toString(vkFormat));
    if (isSupercompressionBlockCompressed(supercompressionScheme))
        if (header.levelCount == 0)
            error(HeaderData::BlockCompressedNoLevel, toString(supercompressionScheme));
    // Additional block-compressed formats (like UASTC) are detected after the DFD is parsed to validate levelCount

    numLevels = std::max(header.levelCount, 1u);

    // This test works for arrays too because height or depth will be 0.
    const auto max_dim = std::max(std::max(header.pixelWidth, header.pixelHeight), header.pixelDepth);
    if (max_dim < (1u << (numLevels - 1u))) {
        // Can't have more mip levels than 1 + log2(max(width, height, depth))
        error(HeaderData::TooManyMipLevels, numLevels, max_dim);
    }

    // Validate supercompressionScheme
    if (KTX_SS_BEGIN_VENDOR_RANGE <= header.supercompressionScheme && header.supercompressionScheme <= KTX_SS_END_VENDOR_RANGE)
        warning(HeaderData::VendorSupercompression, toString(supercompressionScheme));
    else if (header.supercompressionScheme < KTX_SS_BEGIN_RANGE || KTX_SS_END_RANGE < header.supercompressionScheme)
        error(HeaderData::InvalidSupercompression, toString(supercompressionScheme));

    // Validate GLTF KHR_texture_basisu compatibility, if needed
    if (checkGLTFBasisU) {
        // Check for allowed supercompression schemes
        switch (header.supercompressionScheme) {
        case KTX_SS_NONE: [[fallthrough]];
        case KTX_SS_BASIS_LZ: [[fallthrough]];
        case KTX_SS_ZSTD:
            break;
        default:
            error(HeaderData::InvalidSupercompressionGLTFBU, toString(supercompressionScheme));
            break;
        }

        // Check that texture type is 2D
        // NOTE: pixelHeight == 0 already covered by other error codes
        if (header.pixelDepth != 0)
            error(HeaderData::InvalidTextureTypeGLTFBU, "pixelDepth", header.pixelDepth, 0);
        if (header.layerCount != 0)
            error(HeaderData::InvalidTextureTypeGLTFBU, "layerCount", header.layerCount, 0);
        if (header.faceCount != 1)
            error(HeaderData::InvalidTextureTypeGLTFBU, "faceCount", header.faceCount, 1);

        // Check that width and height are multiples of 4
        if (header.pixelWidth % 4 != 0)
            error(HeaderData::InvalidPixelWidthHeightGLTFBU, "pixelWidth", header.pixelWidth);
        if (header.pixelHeight % 4 != 0)
            error(HeaderData::InvalidPixelWidthHeightGLTFBU, "pixelHeight", header.pixelHeight);

        // Check that levelCount is 1 or that the full mip pyramid is present
        uint32_t fullMipPyramidLevelCount = 1 + (uint32_t)log2(max_dim);
        if (header.levelCount != 1 && header.levelCount != fullMipPyramidLevelCount)
            error(HeaderData::InvalidLevelCountGLTFBU, header.levelCount, fullMipPyramidLevelCount);
    }
}

void ValidationContext::validateIndices() {
    const auto supercompressionScheme = ktxSupercmpScheme(header.supercompressionScheme);

    // Validate dataFormatDescriptor index
    if (header.dataFormatDescriptor.byteOffset == 0 || header.dataFormatDescriptor.byteLength == 0)
        error(HeaderData::IndexDFDMissing, header.dataFormatDescriptor.byteOffset, header.dataFormatDescriptor.byteLength);

    const auto levelIndexSize = sizeof(ktxLevelIndexEntry) * numLevels;
    std::size_t expectedOffset = KTX2_HEADER_SIZE + levelIndexSize;
    expectedOffset = align(expectedOffset, std::size_t{4});
    if (expectedOffset != header.dataFormatDescriptor.byteOffset)
        error(HeaderData::IndexDFDInvalidOffset, header.dataFormatDescriptor.byteOffset, expectedOffset);
    expectedOffset += header.dataFormatDescriptor.byteLength;

    if (header.dataFormatDescriptor.byteOffset != 0 && header.dataFormatDescriptor.byteLength != 0)
        if (header.keyValueData.byteOffset != 0)
            if (header.dataFormatDescriptor.byteLength != header.keyValueData.byteOffset - header.dataFormatDescriptor.byteOffset)
                error(HeaderData::IndexDFDInvalidLength, header.dataFormatDescriptor.byteLength, header.keyValueData.byteOffset - header.dataFormatDescriptor.byteOffset);

    // Validate keyValueData index
    if (header.keyValueData.byteLength != 0) {
        expectedOffset = align(expectedOffset, std::size_t{4});
        if (expectedOffset != header.keyValueData.byteOffset)
            error(HeaderData::IndexKVDInvalidOffset, header.keyValueData.byteOffset, expectedOffset);
        expectedOffset += header.keyValueData.byteLength;
    } else {
        if (header.keyValueData.byteOffset != 0)
            error(HeaderData::IndexKVDOffsetWithoutLength, header.keyValueData.byteOffset);
    }

    // Validate supercompressionGlobalData index
    if (header.supercompressionGlobalData.byteLength != 0) {
        expectedOffset = align(expectedOffset, std::size_t{8});
        if (expectedOffset != header.supercompressionGlobalData.byteOffset)
            error(HeaderData::IndexSGDInvalidOffset, header.supercompressionGlobalData.byteOffset, expectedOffset);
        expectedOffset += header.supercompressionGlobalData.byteLength;
    } else {
        if (header.supercompressionGlobalData.byteOffset != 0)
            error(HeaderData::IndexSGDOffsetWithoutLength, header.supercompressionGlobalData.byteOffset);
    }

    if (isSupercompressionWithGlobalData(supercompressionScheme)) {
        if (header.supercompressionGlobalData.byteLength == 0)
            error(HeaderData::IndexSGDMissing, toString(supercompressionScheme));
    } else if (isSupercompressionWithNoGlobalData(supercompressionScheme)) {
        if (header.supercompressionGlobalData.byteLength != 0)
            error(HeaderData::IndexSGDNotApplicable, header.supercompressionGlobalData.byteLength, toString(supercompressionScheme));
    }
}

inline uint32_t calcLevelAlignment(ktxSupercmpScheme scheme, uint8_t blockByteLength) {
    if (scheme != KTX_SS_NONE)
        return 1;
    else
        return std::lcm(blockByteLength, 4);
}

size_t ValidationContext::calcImageSize(uint32_t level) {
    const auto levelWidth = std::max(1u, header.pixelWidth >> level);
    const auto levelHeight = std::max(1u, header.pixelHeight >> level);
    const auto blockDimensionX = 1u + expectedBlockDimension0.value_or(parsedBlockDimension0.value_or(0u));
    const auto blockDimensionY = 1u + expectedBlockDimension1.value_or(parsedBlockDimension1.value_or(0u));
    const auto blockCountX = ceil_div(levelWidth, blockDimensionX);
    const auto blockCountY = ceil_div(levelHeight, blockDimensionY);
    const auto blockSize = expectedBytePlanes ?
            expectedBytePlanes->at(0) :
            parsedBlockByteLength.value_or(0u);

    return blockCountX * blockCountY * blockSize;
}

size_t ValidationContext::calcLayerSize(uint32_t level) {
    const auto levelDepth = std::max(1u, header.pixelDepth >> level);
    const auto blockDimensionZ = 1u + expectedBlockDimension2.value_or(parsedBlockDimension2.value_or(0u));
    const auto blockCountZ = ceil_div(levelDepth, blockDimensionZ);

    const auto imageSize = calcImageSize(level);
    // As there are no 3D cubemaps, the image's z block count will always be 1 for
    // cubemaps and numFaces will always be 1 for 3D textures so the multiplication is safe.
    // 3D cubemaps, if they existed, would require imageSize * (blockCount.z + This->numFaces);
    return imageSize * blockCountZ * header.faceCount;
}

size_t ValidationContext::calcLevelSize(uint32_t level) {
    return calcLayerSize(level) * numLayers;
}

size_t ValidationContext::calcLevelOffset(size_t firstLevelOffset, size_t alignment, uint32_t level) {
    // This function is only useful when the following 2 conditions are met
    // as otherwise we have no idea what the size of a level ought to be.
    assert(header.vkFormat != VK_FORMAT_UNDEFINED);
    assert(header.supercompressionScheme == KTX_SS_NONE);

    assert(level < numLevels);
    // Calculate the expected base offset in the file
    size_t levelOffset = align(firstLevelOffset, alignment);
    for (uint32_t i = numLevels - 1; i > level; --i) {
        levelOffset += calcLevelSize(i);
        levelOffset = align(levelOffset, alignment);
    }
    return levelOffset;
}

void ValidationContext::validateLevelIndex() {
    levelIndices.resize(numLevels);

    const auto levelIndexOffset = sizeof(KTX_header2);
    const auto levelIndexSize = sizeof(ktxLevelIndexEntry) * numLevels;
    read(levelIndexOffset, levelIndices.data(), levelIndexSize, "the level index");

    const auto blockByteLength = (expectedBytePlanes ? expectedBytePlanes->at(0) : (parsedBlockByteLength == 0 ? uint8_t(1) : parsedBlockByteLength.value_or(0)));
    std::size_t requiredLevelAlignment = calcLevelAlignment(ktxSupercmpScheme(header.supercompressionScheme), blockByteLength);

    std::size_t expectedFirstLevelOffset = 0;
    if (header.supercompressionGlobalData.byteLength != 0)
        expectedFirstLevelOffset = header.supercompressionGlobalData.byteLength + header.supercompressionGlobalData.byteOffset;
    else if (header.keyValueData.byteLength != 0)
        expectedFirstLevelOffset = header.keyValueData.byteLength + header.keyValueData.byteOffset;
    else if (header.dataFormatDescriptor.byteLength != 0)
        expectedFirstLevelOffset = header.dataFormatDescriptor.byteLength + header.dataFormatDescriptor.byteOffset;
    else
        expectedFirstLevelOffset = levelIndexOffset + levelIndexSize;
    expectedFirstLevelOffset = align(expectedFirstLevelOffset, requiredLevelAlignment);

    // The first (largest) mip level is the first in the index and the last in the file.
    for (std::size_t i = 1; i < levelIndices.size(); ++i) {
        if (levelIndices[i].byteLength > levelIndices[i - 1].byteLength)
            error(LevelIndex::IncorrectIndexOrder, i - 1, levelIndices[i - 1].byteLength, i, levelIndices[i].byteLength);

        if (levelIndices[i].byteOffset > levelIndices[i - 1].byteOffset)
            error(LevelIndex::IncorrectLevelOrder, i - 1, levelIndices[i - 1].byteOffset, i, levelIndices[i].byteOffset);
    }

    std::size_t lastByteOffset = expectedFirstLevelOffset; // Reuse lastByteOffset to inject first offset to expectedOffset
    std::size_t lastByteLength = 0;

    // Count down, so we can check the distance between levels for the UNDEFINED and SUPERCOMPRESSION cases.
    for (auto it = static_cast<uint32_t>(levelIndices.size()); it != 0; --it) {
        const auto index = it - 1;
        const auto& level = levelIndices[index];

        // Validate byteOffset
        const auto knownLevelOffset = header.vkFormat != VK_FORMAT_UNDEFINED &&
                header.supercompressionScheme == KTX_SS_NONE;
        // If the exact level sizes are unknown we have to trust the byteLengths.
        // In that case we know where the first level must be in the file, and we can calculate
        // the offsets by progressively summing the lengths and paddings so far.
        const auto expectedOffset = knownLevelOffset ?
                calcLevelOffset(expectedFirstLevelOffset, requiredLevelAlignment, index) :
                align(lastByteOffset + lastByteLength, requiredLevelAlignment);

        if (level.byteOffset % requiredLevelAlignment != 0)
            error(LevelIndex::IncorrectByteOffsetUnaligned, index, level.byteOffset, requiredLevelAlignment, expectedOffset);
        else if (level.byteOffset != expectedOffset)
            error(LevelIndex::IncorrectByteOffset, index, level.byteOffset, expectedOffset);

        // Workaround: Disable ByteLength validations for the 3D ASTC encoder which currently ignores partial Z blocks in our test files
        const auto disableByteLengthValidation = isFormat3DBlockCompressed(VkFormat(header.vkFormat))
                && header.pixelDepth % (expectedBlockDimension2.value_or(0) + 1) != 0;

        if (!disableByteLengthValidation) {
            // Validate byteLength
            if (header.vkFormat != VK_FORMAT_UNDEFINED && header.supercompressionScheme == KTX_SS_NONE) {
                const auto expectedLength = calcLevelSize(index);
                if (level.byteLength != expectedLength)
                    error(LevelIndex::IncorrectByteLength, index, level.byteLength, expectedLength);
            }

            // Validate uncompressedByteLength
            if (header.supercompressionScheme == KTX_SS_BASIS_LZ) {
                if (level.uncompressedByteLength != 0)
                    error(LevelIndex::NonZeroUBLForBLZE, index, level.uncompressedByteLength);
            } else if (header.vkFormat != VK_FORMAT_UNDEFINED) {
                if (header.supercompressionScheme == KTX_SS_NONE) {
                    const auto expectedUncompressedLength = calcLevelSize(index);
                    if (level.uncompressedByteLength != expectedUncompressedLength)
                        error(LevelIndex::IncorrectUncompressedByteLength, index, level.uncompressedByteLength, expectedUncompressedLength);
                }
            } else {
                if (level.uncompressedByteLength == 0)
                    error(LevelIndex::ZeroUncompressedLength, index);
                else if (level.uncompressedByteLength % std::max(header.faceCount * numLayers, 1u) != 0)
                    // On the other branches uncompressedByteLength is always checked exactly,
                    // so this is the only branch where this checks yields useful information
                    error(LevelIndex::InvalidUncompressedLength, index, level.uncompressedByteLength);
            }
        }

        lastByteOffset = level.byteOffset;
        lastByteLength = level.byteLength;
    }
}

void ValidationContext::calculateExpectedDFD(VkFormat format) {
    if (format == VK_FORMAT_UNDEFINED || !isFormatValid(format) || isProhibitedFormat(format))
        return;

    const auto dfd = std::unique_ptr<uint32_t, decltype(&free)>(vk2dfd(format), &free);
    if (dfd == nullptr) {
        error(Validator::CreateExpectedDFDFailure, toString(format));
        return;
    }

    const uint32_t* bdfd = dfd.get() + 1;

    const auto expectedSampleCount = KHR_DFDSAMPLECOUNT(bdfd);
    expectedSamples.emplace(expectedSampleCount, SampleType{});
    std::memcpy(expectedSamples->data(), reinterpret_cast<const char*>(bdfd) + sizeof(BDFD), expectedSampleCount * sizeof(SampleType));

    expectedColorModel = khr_df_model_e(KHR_DFDVAL(bdfd, MODEL));
    expectedColorModelIsBlockCompressed = isColorModelBlockCompressed(*expectedColorModel);
    expectedBytePlanes.emplace();
    expectedBytePlanes.value()[0] = KHR_DFDVAL(bdfd, BYTESPLANE0);
    expectedBytePlanes.value()[1] = KHR_DFDVAL(bdfd, BYTESPLANE1);
    expectedBytePlanes.value()[2] = KHR_DFDVAL(bdfd, BYTESPLANE2);
    expectedBytePlanes.value()[3] = KHR_DFDVAL(bdfd, BYTESPLANE3);
    expectedBytePlanes.value()[4] = KHR_DFDVAL(bdfd, BYTESPLANE4);
    expectedBytePlanes.value()[5] = KHR_DFDVAL(bdfd, BYTESPLANE5);
    expectedBytePlanes.value()[6] = KHR_DFDVAL(bdfd, BYTESPLANE6);
    expectedBytePlanes.value()[7] = KHR_DFDVAL(bdfd, BYTESPLANE7);
    expectedBlockDimension0 = static_cast<uint8_t>(KHR_DFDVAL(bdfd, TEXELBLOCKDIMENSION0));
    expectedBlockDimension1 = static_cast<uint8_t>(KHR_DFDVAL(bdfd, TEXELBLOCKDIMENSION1));
    expectedBlockDimension2 = static_cast<uint8_t>(KHR_DFDVAL(bdfd, TEXELBLOCKDIMENSION2));
    expectedBlockDimension3 = static_cast<uint8_t>(KHR_DFDVAL(bdfd, TEXELBLOCKDIMENSION3));

    expectedTypeSize = vkFormatTypeSize(format);
}

void ValidationContext::validateDFD() {
    const auto dfdByteOffset = header.dataFormatDescriptor.byteOffset;
    const auto dfdByteLength = header.dataFormatDescriptor.byteLength;

    if (dfdByteOffset == 0 || dfdByteLength == 0)
        return; // There is no DFD block

    const auto buffer = std::make_unique<uint8_t[]>(dfdByteLength);
    read(dfdByteOffset, buffer.get(), dfdByteLength, "the DFD");
    const auto* ptrDFD = buffer.get();
    const auto* ptrDFDEnd = ptrDFD + dfdByteLength;
    const auto* ptrDFDIt = ptrDFD;

    uint32_t dfdTotalSize;
    std::memcpy(&dfdTotalSize, ptrDFDIt, sizeof(uint32_t));
    ptrDFDIt += sizeof(uint32_t);

    if (dfdByteLength != dfdTotalSize)
        error(DFD::SizeMismatch, dfdByteLength, dfdTotalSize);

    uint32_t numBlocks = 0;
    bool foundBDFD = false;

    while (ptrDFDIt < ptrDFDEnd) {
        const auto remainingDFDBytes = static_cast<std::size_t>(ptrDFDEnd - ptrDFDIt);

        if (++numBlocks > MAX_NUM_DFD_BLOCKS) {
            warning(DFD::TooManyDFDBlocks, numBlocks, remainingDFDBytes);
            break;
        }

        DFDHeader blockHeader{};
        if (remainingDFDBytes < sizeof(DFDHeader)) {
            error(DFD::NotEnoughDataForBlockHeader, remainingDFDBytes);
            break;
        }

        std::memcpy(&blockHeader, ptrDFDIt, sizeof(DFDHeader));

        if (blockHeader.descriptorBlockSize < sizeof(DFDHeader)) {
            error(DFD::DescriptorBlockSizeTooSmall, numBlocks, +blockHeader.descriptorBlockSize);

        } else if (blockHeader.descriptorBlockSize > remainingDFDBytes) {
            error(DFD::DescriptorBlockSizeTooBig, numBlocks, +blockHeader.descriptorBlockSize, remainingDFDBytes);

        } else {
            if (blockHeader.vendorId == KHR_DF_VENDORID_KHRONOS && blockHeader.descriptorType == KHR_DF_KHR_DESCRIPTORTYPE_BASICFORMAT) {
                if (std::exchange(foundBDFD, true)) {
                    warning(DFD::MultipleBDFD, numBlocks);

                } else if (blockHeader.descriptorBlockSize < sizeof(BDFD)) {
                    error(DFD::BasicDescriptorBlockSizeTooSmall, numBlocks, +blockHeader.descriptorBlockSize);

                } else if (numBlocks != 1) {
                    // The Basic DFD block has to be the first block.
                    foundBDFD = false;

                } else {
                    BDFD block{};
                    std::memcpy(&block, ptrDFDIt, sizeof(BDFD));

                    if ((block.descriptorBlockSize - 24) % 16 != 0)
                         error(DFD::BasicDescriptorBlockSizeInvalid, numBlocks, +block.descriptorBlockSize);

                    const auto numSamplesStored = (block.descriptorBlockSize - 24u) / 16u;
                    const auto numSamplesValidating = std::min(MAX_NUM_BDFD_SAMPLES, numSamplesStored);

                    if (numSamplesStored > MAX_NUM_BDFD_SAMPLES)
                        warning(DFD::TooManySample, numBlocks, numSamplesStored, MAX_NUM_BDFD_SAMPLES,
                                numSamplesStored - numSamplesValidating,
                                block.descriptorBlockSize - sizeof(BDFD) - numSamplesValidating * sizeof(SampleType));

                    // Samples are located at the end of the block
                    std::vector<SampleType> samples(numSamplesValidating);
                    std::memcpy(samples.data(), ptrDFDIt + sizeof(BDFD), numSamplesValidating * sizeof(SampleType));

                    validateDFDBasic(numBlocks, reinterpret_cast<uint32_t*>(buffer.get()), block, samples);
                }

            } else if (blockHeader.vendorId == KHR_DF_VENDORID_KHRONOS && blockHeader.descriptorType == KHR_DF_KHR_DESCRIPTORTYPE_ADDITIONAL_DIMENSIONS) {
                // TODO: Implement DFD validation for ADDITIONAL_DIMENSIONS

            } else if (blockHeader.vendorId == KHR_DF_VENDORID_KHRONOS && blockHeader.descriptorType == KHR_DF_KHR_DESCRIPTORTYPE_ADDITIONAL_PLANES) {
                // TODO: Implement DFD validation for ADDITIONAL_PLANES

            } else {
                warning(DFD::UnknownDFDBlock,
                        numBlocks,
                        toString(khr_df_vendorid_e(blockHeader.vendorId)),
                        toString(khr_df_vendorid_e(blockHeader.vendorId), khr_df_khr_descriptortype_e(blockHeader.descriptorType)));
            }
        }

        ptrDFDIt += std::max(blockHeader.descriptorBlockSize, 8u);
    }

    if (!foundBDFD)
        error(DFD::MissingBDFD);
}

void ValidationContext::validateDFDBasic(uint32_t blockIndex, const uint32_t* dfd, const BDFD& block, const std::vector<SampleType>& samples) {

    numSamples = static_cast<uint32_t>(samples.size());

    parsedColorModel = khr_df_model_e(block.model);
    parsedTransferFunction = khr_df_transfer_e(block.transfer);
    parsedBlockByteLength = block.bytesPlanes[0];
    parsedBlockDimension0 = static_cast<uint8_t>(block.texelBlockDimension0);
    parsedBlockDimension1 = static_cast<uint8_t>(block.texelBlockDimension1);
    parsedBlockDimension2 = static_cast<uint8_t>(block.texelBlockDimension2);

    // Validate versionNumber
    if (block.versionNumber != KHR_DF_VERSIONNUMBER_1_3)
        error(DFD::BasicVersionNotSupported, blockIndex, toString(khr_df_versionnumber_e(block.versionNumber)));

    // Validate transferFunction
    if (block.transfer != KHR_DF_TRANSFER_SRGB && block.transfer != KHR_DF_TRANSFER_LINEAR)
        error(DFD::BasicInvalidTransferFunction, blockIndex, toString(khr_df_transfer_e(block.transfer)));

    if (isFormatSRGB(VkFormat(header.vkFormat)) && block.transfer != KHR_DF_TRANSFER_SRGB)
        error(DFD::BasicSRGBMismatch, blockIndex, toString(khr_df_transfer_e(block.transfer)), toString(VkFormat(header.vkFormat)));

    if (isFormatNotSRGBButHasSRGBVariant(VkFormat(header.vkFormat)) && block.transfer == KHR_DF_TRANSFER_SRGB)
        error(DFD::BasicNotSRGBMismatch, blockIndex, toString(VkFormat(header.vkFormat)));

    // Validate colorModel
    if (isFormat422(VkFormat(header.vkFormat))) {
        if (!isProhibitedFormat(VkFormat(header.vkFormat)))
            if (block.model != KHR_DF_MODEL_YUVSDA)
                error(DFD::IncorrectModelFor422, blockIndex, toString(khr_df_model_e(block.model)), toString(VkFormat(header.vkFormat)));

    } else if (isFormatBlockCompressed(VkFormat(header.vkFormat))) {
        const auto expectedBCColorModel = getColorModelForBlockCompressedFormat(VkFormat(header.vkFormat));
        if (khr_df_model_e(block.model) != expectedBCColorModel)
            error(DFD::IncorrectModelForBlock, blockIndex, toString(khr_df_model_e(block.model)), toString(VkFormat(header.vkFormat)), toString(expectedBCColorModel));

    } else if (header.vkFormat != VK_FORMAT_UNDEFINED) {
        if (block.model != KHR_DF_MODEL_RGBSDA)
            error(DFD::IncorrectModelForRGB, blockIndex, toString(khr_df_model_e(block.model)), toString(VkFormat(header.vkFormat)));
    }

    if (header.supercompressionScheme == KTX_SS_BASIS_LZ)
        if (block.model != KHR_DF_MODEL_ETC1S)
            error(DFD::IncorrectModelForBLZE, blockIndex, toString(khr_df_model_e(block.model)));

    // Check GLTF KHR_texture_basisu specific errors
    if (checkGLTFBasisU) {
        switch (block.model) {
        case KHR_DF_MODEL_ETC1S:
            // Supercompression already verified above, only need to check samples.
            if (samples.size() > 0) {
                switch (samples[0].channelType) {
                case KHR_DF_CHANNEL_ETC1S_RGB:
                    if (samples.size() > 1 && samples[1].channelType != KHR_DF_CHANNEL_ETC1S_AAA)
                        error(DFD::InvalidChannelGLTFBU, blockIndex, "KHR_DF_MODEL_ETC1S", 2,
                                toString(KHR_DF_MODEL_ETC1S, khr_df_model_channels_e(samples[1].channelType)),
                                "KHR_DF_CHANNEL_ETC1S_AAA when sample #0 channelType is KHR_DF_CHANNEL_ETC1S_RGB");
                    break;
                case KHR_DF_CHANNEL_ETC1S_RRR:
                    if (samples.size() > 1 && samples[1].channelType != KHR_DF_CHANNEL_ETC1S_GGG)
                        error(DFD::InvalidChannelGLTFBU, blockIndex, "KHR_DF_MODEL_ETC1S", 2,
                                toString(KHR_DF_MODEL_ETC1S, khr_df_model_channels_e(samples[1].channelType)),
                                "KHR_DF_CHANNEL_ETC1S_GGG when sample #0 channelType is KHR_DF_CHANNEL_ETC1S_RRR");
                    break;
                default:
                    error(DFD::InvalidChannelGLTFBU, blockIndex, "KHR_DF_MODEL_ETC1S", 1,
                            toString(KHR_DF_MODEL_ETC1S, khr_df_model_channels_e(samples[0].channelType)),
                            "KHR_DF_CHANNEL_ETC1S_RGB or KHR_DF_CHANNEL_ETC1S_RRR");
                    break;
                }
            }
            break;
        case KHR_DF_MODEL_UASTC:
            if (header.supercompressionScheme != KTX_SS_NONE && header.supercompressionScheme != KTX_SS_ZSTD)
                error(DFD::IncompatibleModelGLTFBU, blockIndex, "KHR_DF_MODEL_UASTC",
                        toString(ktxSupercmpScheme(header.supercompressionScheme)),
                        "KTX_SS_NONE or KTX_SS_ZSTD");

            if (samples.size() > 0) {
                switch (samples[0].channelType) {
                case KHR_DF_CHANNEL_UASTC_RGB: [[fallthrough]];
                case KHR_DF_CHANNEL_UASTC_RGBA: [[fallthrough]];
                case KHR_DF_CHANNEL_UASTC_RRR: [[fallthrough]];
                case KHR_DF_CHANNEL_UASTC_RG:
                    break;
                default:
                    error(DFD::InvalidChannelGLTFBU, blockIndex, "KHR_DF_MODEL_UASTC", 0,
                            toString(KHR_DF_MODEL_UASTC, khr_df_model_channels_e(samples[0].channelType)),
                            "KHR_DF_CHANNEL_UASTC_RGB, KHR_DF_CHANNEL_UASTC_RGBA, KHR_DF_CHANNEL_UASTC_RRR, or KHR_DF_CHANNEL_UASTC_RG");
                    break;
                }
            }
            break;
        default:
            error(DFD::IncorrectModelGLTFBU, blockIndex, toString(khr_df_model_e(block.model)));
            break;
        }

        if ((block.primaries != KHR_DF_PRIMARIES_BT709 || block.transfer != KHR_DF_TRANSFER_SRGB) &&
            (block.primaries != KHR_DF_PRIMARIES_UNSPECIFIED || block.transfer != KHR_DF_TRANSFER_LINEAR))
            error(DFD::InvalidColorSpaceGLTFBU, blockIndex,
                    toString(khr_df_primaries_e(block.primaries)), toString(khr_df_transfer_e(block.transfer)));
    }

    // Validate colorPrimaries
    if (!isColorPrimariesValid(khr_df_primaries_e(block.primaries)))
        error(DFD::InvalidColorPrimaries, blockIndex, block.primaries);

    // Validate samples[].channelType
    for (std::size_t i = 0; i < samples.size(); ++i) {
        const auto& sample = samples[i];
        if (!isChannelTypeValid(khr_df_model_e(block.model), khr_df_model_channels_e(sample.channelType)))
            error(DFD::InvalidChannelForModel,
                    blockIndex,
                    i + 1,
                    toString(khr_df_model_e(block.model), khr_df_model_channels_e(sample.channelType)),
                    toString(khr_df_model_e(block.model)));
    }

    // Validate: bytesPlanes, texelBlockDimensions and samples
    switch (header.supercompressionScheme) {
    case KTX_SS_NONE: [[fallthrough]];
    case KTX_SS_ZSTD: [[fallthrough]];
    case KTX_SS_ZLIB:
        if (header.vkFormat != VK_FORMAT_UNDEFINED) {
            if (expectedSamples) {
                if (samples.size() != expectedSamples->size())
                    error(DFD::SampleCountMismatch, blockIndex, samples.size(), toString(VkFormat(header.vkFormat)), expectedSamples->size());

                for (std::size_t i = 0; i < std::min(samples.size(), expectedSamples->size()); ++i) {
                    const auto& parsed = samples[i];
                    const auto& expected = (*expectedSamples)[i];

                    if (parsed.bitOffset != expected.bitOffset)
                        error(DFD::FormatMismatch, blockIndex, i + 1, "bitOffset", parsed.bitOffset,
                                expected.bitOffset, toString(VkFormat(header.vkFormat)));
                    if (parsed.bitLength != expected.bitLength)
                        error(DFD::FormatMismatch, blockIndex, i + 1, "bitLength", parsed.bitLength,
                                expected.bitLength, toString(VkFormat(header.vkFormat)));
                    if (parsed.channelType != expected.channelType)
                        error(DFD::FormatMismatch, blockIndex, i + 1, "channelType",
                                toString(khr_df_model_e(block.model), khr_df_model_channels_e(parsed.channelType)),
                                toString(expectedColorModel.value_or(KHR_DF_MODEL_UNSPECIFIED), khr_df_model_channels_e(expected.channelType)),
                                toString(VkFormat(header.vkFormat)));
                    if (parsed.qualifierLinear != expected.qualifierLinear)
                        error(DFD::FormatMismatch, blockIndex, i + 1, "qualifierLinear", parsed.qualifierLinear,
                                expected.qualifierLinear, toString(VkFormat(header.vkFormat)));
                    if (parsed.qualifierExponent != expected.qualifierExponent)
                        error(DFD::FormatMismatch, blockIndex, i + 1, "qualifierExponent", parsed.qualifierExponent,
                                expected.qualifierExponent, toString(VkFormat(header.vkFormat)));
                    if (parsed.qualifierSigned != expected.qualifierSigned)
                        error(DFD::FormatMismatch, blockIndex, i + 1, "qualifierSigned", parsed.qualifierSigned,
                                expected.qualifierSigned, toString(VkFormat(header.vkFormat)));
                    if (parsed.qualifierFloat != expected.qualifierFloat)
                        error(DFD::FormatMismatch, blockIndex, i + 1, "qualifierFloat", parsed.qualifierFloat,
                                expected.qualifierFloat, toString(VkFormat(header.vkFormat)));
                    // For 4:2:2 formats the X sample positions can vary
                    if (!isFormat422(VkFormat(header.vkFormat)) && parsed.samplePosition0 != expected.samplePosition0)
                        error(DFD::FormatMismatch, blockIndex, i + 1, "samplePosition0", parsed.samplePosition0,
                                expected.samplePosition0, toString(VkFormat(header.vkFormat)));
                    if (parsed.samplePosition1 != expected.samplePosition1)
                        error(DFD::FormatMismatch, blockIndex, i + 1, "samplePosition1", parsed.samplePosition1,
                                expected.samplePosition1, toString(VkFormat(header.vkFormat)));
                    if (parsed.samplePosition2 != expected.samplePosition2)
                        error(DFD::FormatMismatch, blockIndex, i + 1, "samplePosition2", parsed.samplePosition2,
                                expected.samplePosition2, toString(VkFormat(header.vkFormat)));
                    if (parsed.samplePosition3 != expected.samplePosition3)
                        error(DFD::FormatMismatch, blockIndex, i + 1, "samplePosition3", parsed.samplePosition3,
                                expected.samplePosition3, toString(VkFormat(header.vkFormat)));
                    if (parsed.lower != expected.lower)
                        error(DFD::FormatMismatch, blockIndex, i + 1, "sampleLower", parsed.lower,
                                expected.lower, toString(VkFormat(header.vkFormat)));
                    if (parsed.upper != expected.upper)
                        error(DFD::FormatMismatch, blockIndex, i + 1, "sampleUpper", parsed.upper,
                                expected.upper, toString(VkFormat(header.vkFormat)));
                }
            }

            if (header.supercompressionScheme == KTX_SS_NONE) {
                if (expectedBytePlanes && !std::equal(
                        expectedBytePlanes->begin(), expectedBytePlanes->end(),
                        block.bytesPlanes.begin(), block.bytesPlanes.end())) {
                    error(DFD::BytesPlanesMismatch,
                            blockIndex,
                            block.bytesPlanes[0], block.bytesPlanes[1],
                            block.bytesPlanes[2], block.bytesPlanes[3],
                            block.bytesPlanes[4], block.bytesPlanes[5],
                            block.bytesPlanes[6], block.bytesPlanes[7],
                            toString(VkFormat(header.vkFormat)),
                            (*expectedBytePlanes)[0], (*expectedBytePlanes)[1],
                            (*expectedBytePlanes)[2], (*expectedBytePlanes)[3],
                            (*expectedBytePlanes)[4], (*expectedBytePlanes)[5],
                            (*expectedBytePlanes)[6], (*expectedBytePlanes)[7]);
                }
            }

            if (!isFormatBlockCompressed(VkFormat(header.vkFormat))
                    && !isProhibitedFormat(VkFormat(header.vkFormat))) {

                InterpretDFDResult result;
                InterpretedDFDChannel r, g, b, a;
                uint32_t componentByteLength = 0;

                result = interpretDFD(dfd, &r, &g, &b, &a, &componentByteLength);

                // Reset the "false" positive error in interpretDFD with VK_FORMAT_E5B9G9R9_UFLOAT_PACK32
                if (header.vkFormat == VK_FORMAT_E5B9G9R9_UFLOAT_PACK32 && result == i_UNSUPPORTED_NONTRIVIAL_ENDIANNESS)
                    result = InterpretDFDResult(0);

                if (result >= i_UNSUPPORTED_ERROR_BIT) {
                    switch (result) {
                    case i_UNSUPPORTED_CHANNEL_TYPES:
                        // We already checked channel types elsewhere with more detailed error message
                        break;
                    case i_UNSUPPORTED_MULTIPLE_PLANES:
                        error(DFD::MultiplaneFormatsNotSupported, blockIndex,
                                block.bytesPlanes[0], block.bytesPlanes[1],
                                block.bytesPlanes[2], block.bytesPlanes[3],
                                block.bytesPlanes[4], block.bytesPlanes[5],
                                block.bytesPlanes[6], block.bytesPlanes[7]);
                        break;
                    case i_UNSUPPORTED_MIXED_CHANNELS:
                        error(DFD::InterpretDFDMixedChannels, blockIndex);
                        break;
                    case i_UNSUPPORTED_MULTIPLE_SAMPLE_LOCATIONS:
                        error(DFD::InterpretDFDMultisample, blockIndex);
                        break;
                    case i_UNSUPPORTED_NONTRIVIAL_ENDIANNESS:
                        error(DFD::InterpretDFDNonTrivialEndianness, blockIndex);
                        break;
                    default:
                        break;
                    }
                }
            }
        } else { // header.vkFormat == VK_FORMAT_UNDEFINED
            if (block.model == KHR_DF_MODEL_UASTC) {
                // Validate UASTC
                if (samples.size() != 1)
                    error(DFD::InvalidSampleCount, blockIndex, samples.size(), "UASTC", "1");

                if (!block.matchTexelBlockDimensions(3, 3, 0, 0))
                    error(DFD::InvalidTexelBlockDimension,
                            blockIndex,
                            block.texelBlockDimension0 + 1,
                            block.texelBlockDimension1 + 1,
                            block.texelBlockDimension2 + 1,
                            block.texelBlockDimension3 + 1,
                            4, 4, 1, 1, "UASTC");

                if (header.supercompressionScheme == KTX_SS_NONE) {
                    if (block.bytesPlanes[0] != 16 || block.bytesPlanes[1] != 0
                            || block.bytesPlanes[2] != 0 || block.bytesPlanes[3] != 0
                            || block.bytesPlanes[4] != 0 || block.bytesPlanes[5] != 0
                            || block.bytesPlanes[6] != 0 || block.bytesPlanes[7] != 0) {
                        error(DFD::BytesPlanesMismatch,
                                blockIndex,
                                block.bytesPlanes[0], block.bytesPlanes[1],
                                block.bytesPlanes[2], block.bytesPlanes[3],
                                block.bytesPlanes[4], block.bytesPlanes[5],
                                block.bytesPlanes[6], block.bytesPlanes[7],
                                "UASTC",
                                16, 0, 0, 0, 0, 0, 0, 0);
                    }
                } else {
                    if (block.hasNonZeroBytePlane())
                        error(DFD::BytesPlanesNotUnsized,
                                blockIndex,
                                block.bytesPlanes[0], block.bytesPlanes[1],
                                block.bytesPlanes[2], block.bytesPlanes[3],
                                block.bytesPlanes[4], block.bytesPlanes[5],
                                block.bytesPlanes[6], block.bytesPlanes[7],
                                "UASTC");
                }

                if (samples.size() > 0) {
                    if (samples[0].bitOffset != 0)
                        error(DFD::InvalidBitOffsetForUASTC, blockIndex, 1, samples[0].bitOffset);

                    if (samples[0].bitLength != 127)
                        error(DFD::InvalidBitLengthForUASTC, blockIndex, 1, samples[0].bitLength);

                    if (samples[0].lower != 0)
                        error(DFD::InvalidLower, blockIndex, 1, samples[0].lower, "UASTC", 0);

                    if (samples[0].upper != UINT32_MAX)
                        error(DFD::InvalidUpper, blockIndex, 1, samples[0].upper, "UASTC", "UINT32_MAX (0xFFFFFFFF)");
                }
            } else {
                // Ensure there are at least some samples
                if (samples.size() == 0)
                    error(DFD::ZeroSamples, blockIndex);

                if (header.supercompressionScheme == KTX_SS_NONE) {
                    if (block.bytesPlanes[0] == 0)
                        error(DFD::BytesPlane0Zero, blockIndex, block.bytesPlanes[0]);
                    if (block.bytesPlanes[1] != 0
                            || block.bytesPlanes[2] != 0 || block.bytesPlanes[3] != 0
                            || block.bytesPlanes[4] != 0 || block.bytesPlanes[5] != 0
                            || block.bytesPlanes[6] != 0 || block.bytesPlanes[7] != 0)
                        error(DFD::MultiplaneFormatsNotSupported, blockIndex,
                                block.bytesPlanes[0], block.bytesPlanes[1],
                                block.bytesPlanes[2], block.bytesPlanes[3],
                                block.bytesPlanes[4], block.bytesPlanes[5],
                                block.bytesPlanes[6], block.bytesPlanes[7]);
                }
            }
        }
        break;

    case KTX_SS_BASIS_LZ:
        // This descriptor should have 1 or 2 samples with bitLength 63 and bitOffsets 0 and 64.
        if (samples.size() == 0 || samples.size() > 2)
            error(DFD::InvalidSampleCount, blockIndex, samples.size(), "BasisLZ/ETC1S", "1 or 2");

        if (!block.matchTexelBlockDimensions(3, 3, 0, 0))
            error(DFD::InvalidTexelBlockDimension,
                    blockIndex,
                    block.texelBlockDimension0 + 1,
                    block.texelBlockDimension1 + 1,
                    block.texelBlockDimension2 + 1,
                    block.texelBlockDimension3 + 1,
                    4, 4, 1, 1, "BasisLZ/ETC1S");

        if (block.hasNonZeroBytePlane())
            error(DFD::BytesPlanesNotUnsized,
                    blockIndex,
                    block.bytesPlanes[0], block.bytesPlanes[1],
                    block.bytesPlanes[2], block.bytesPlanes[3],
                    block.bytesPlanes[4], block.bytesPlanes[5],
                    block.bytesPlanes[6], block.bytesPlanes[7],
                    "BasisLZ/ETC1S");

        for (uint32_t i = 0; i < std::min(static_cast<uint32_t>(samples.size()), 2u); ++i) {
            if (samples[i].bitOffset != (i == 0 ? 0u : 64u))
                error(DFD::InvalidBitOffsetForBLZE, blockIndex, i + 1, samples[i].bitOffset, (i == 0 ? 0u : 64u));

            if (samples[i].bitLength != 63u)
                error(DFD::InvalidBitLengthForBLZE, blockIndex, i + 1, samples[i].bitLength);

            if (samples[i].lower != 0)
                error(DFD::InvalidLower, blockIndex, i + 1, samples[i].lower, "BasisLZ/ETC1S", 0);

            if (samples[i].upper != UINT32_MAX)
                error(DFD::InvalidUpper, blockIndex, i + 1, samples[i].upper, "BasisLZ/ETC1S", "UINT32_MAX (0xFFFFFFFF)");
        }
        break;

    default:
        break;
    }

    // -------------------------------------------------------------------------------------------------
    // Checks that were deferred during header parsing until the BDFD is available

    if (header.vkFormat == VK_FORMAT_UNDEFINED && !isSupercompressionBlockCompressed(ktxSupercmpScheme(header.supercompressionScheme))) {
        // Not VK_FORMAT_UNDEFINED and BlockCompressed Supercompressions were already checked before

        if (isColorModelBlockCompressed(khr_df_model_e(block.model))) {
            if (header.pixelHeight == 0)
                error(HeaderData::BlockCompressedNoHeight, toString(khr_df_model_e(block.model)));

            if (header.levelCount == 0)
                error(HeaderData::BlockCompressedNoLevel, toString(khr_df_model_e(block.model)));
        }
    }

    if (header.vkFormat != VK_FORMAT_UNDEFINED && !isFormatBlockCompressed(static_cast<VkFormat>(header.vkFormat))) {
        // VK_FORMAT_UNDEFINED and BlockCompressed VkFormats were already checked before

        if (isColorModelBlockCompressed(khr_df_model_e(block.model))) {
            if (header.typeSize != 1)
                error(HeaderData::TypeSizeNotOne, toString(khr_df_model_e(block.model)));
        } else {
            if (expectedTypeSize && header.typeSize != expectedTypeSize)
                error(HeaderData::TypeSizeMismatch, header.typeSize, toString(VkFormat(header.vkFormat)), *expectedTypeSize);
        }
    }
}

void ValidationContext::validateKVD() {
    const auto kvdByteOffset = header.keyValueData.byteOffset;
    const auto kvdByteLength = header.keyValueData.byteLength;

    if (kvdByteOffset == 0 || kvdByteLength == 0)
        return; // There is no KVD block

    const auto buffer = std::make_unique<uint8_t[]>(kvdByteLength);
    read(kvdByteOffset, buffer.get(), kvdByteLength, "the Key/Value Data");
    const auto* ptrKVD = buffer.get();
    const auto* ptrKVDEnd = ptrKVD + kvdByteLength;

    struct KeyValueEntry {
        std::string key;
        const uint8_t* data;
        uint32_t size;

        KeyValueEntry(std::string_view key, const uint8_t* data, uint32_t size) :
                key(key), data(data), size(size) {}
    };
    std::vector<KeyValueEntry> entries;
    std::unordered_set<std::string_view> keys;

    uint32_t numKVEntry = 0;
    // Process Key-Value entries {size, key, \0, value} until the end of the KVD block
    // Where size is an uint32_t, and it equals to: sizeof(key) + 1 + sizeof(value)
    const auto* ptrEntry = ptrKVD;
    while (ptrEntry < ptrKVDEnd) {
        const auto remainingKVDBytes = ptrKVDEnd - ptrEntry;

        if (++numKVEntry > MAX_NUM_KV_ENTRIES) {
            warning(Metadata::TooManyEntries, numKVEntry - 1, remainingKVDBytes);
            ptrEntry = ptrKVDEnd;
            break;
        }

        if (remainingKVDBytes < 6) {
            error(Metadata::NotEnoughDataForAnEntry, remainingKVDBytes);
            ptrEntry = ptrKVDEnd;
            break;
        }

        uint32_t sizeKeyValuePair;
        std::memcpy(&sizeKeyValuePair, ptrEntry, sizeof(uint32_t));

        const auto* ptrKeyValuePair = ptrEntry + sizeof(uint32_t);
        const auto* ptrKey = ptrKeyValuePair;

        if (sizeKeyValuePair < 2) {
            error(Metadata::KeyAndValueByteLengthTooSmall, sizeKeyValuePair);
        } else {
            if (ptrKeyValuePair + sizeKeyValuePair > ptrKVDEnd) {
                const auto bytesLeft = ptrKVDEnd - ptrKeyValuePair;
                error(Metadata::KeyAndValueByteLengthTooLarge, sizeKeyValuePair, bytesLeft);
                sizeKeyValuePair = static_cast<uint32_t>(bytesLeft); // Attempt recovery to read out at least the key
            }

            // Determine key, finding the null terminator
            uint32_t sizeKey = 0;
            while (sizeKey < sizeKeyValuePair && ptrKey[sizeKey] != '\0')
                ++sizeKey;

            if (sizeKey == 0) {
                error(Metadata::KeyEmpty);
            } else {
                const auto keyHasNullTerminator = sizeKey != sizeKeyValuePair;
                auto key = std::string_view(reinterpret_cast<const char*>(ptrKey), sizeKey);

                // Determine the value
                const auto* ptrValue = ptrKey + sizeKey + 1;
                const auto sizeValue = keyHasNullTerminator ? sizeKeyValuePair - sizeKey - 1 : 0;

                // Check for BOM
                if (starts_with(key, "\xEF\xBB\xBF")) {
                    key.remove_prefix(3);
                    error(Metadata::KeyForbiddenBOM, key);
                }

                if (auto invalidIndex = validateUTF8(key))
                    error(Metadata::KeyInvalidUTF8, key, *invalidIndex);

                if (!keyHasNullTerminator)
                    error(Metadata::KeyMissingNullTerminator, key);

                entries.emplace_back(key, ptrValue, sizeValue);

                if (!keys.emplace(key).second)
                    error(Metadata::DuplicateKey, key);
            }
        }

        // Finish entry
        ptrEntry += sizeof(uint32_t) + sizeKeyValuePair;
        validateAlignmentPaddingZeros(ptrEntry, ptrKVDEnd, 4, Metadata::PaddingNotZero, "after a Key-Value entry");
        ptrEntry = align(ptrEntry, 4);
    }

    if (ptrEntry != ptrKVDEnd)
        // Being super explicit about the specs. This check might be overkill as other checks often cover this case
        error(Metadata::SizesDontAddUp, ptrEntry - ptrKVD, kvdByteLength);

    if (header.supercompressionGlobalData.byteLength != 0)
        validateAlignmentPaddingZeros(ptrEntry, ptrKVDEnd, 8, Metadata::PaddingNotZero, "after the last KVD entry");

    if (!is_sorted(entries, std::less<>{}, &KeyValueEntry::key)) {
        error(Metadata::OutOfOrder);
        sort(entries, std::less<>{}, &KeyValueEntry::key);
    }

    using MemberFN = void(ValidationContext::*)(const uint8_t*, uint32_t);
    std::unordered_map<std::string, MemberFN> kvValidators;

    kvValidators.emplace("KTXcubemapIncomplete", &ValidationContext::validateKTXcubemapIncomplete);
    kvValidators.emplace("KTXorientation", &ValidationContext::validateKTXorientation);
    kvValidators.emplace("KTXglFormat", &ValidationContext::validateKTXglFormat);
    kvValidators.emplace("KTXdxgiFormat__", &ValidationContext::validateKTXdxgiFormat);
    kvValidators.emplace("KTXmetalPixelFormat", &ValidationContext::validateKTXmetalPixelFormat);
    kvValidators.emplace("KTXswizzle", &ValidationContext::validateKTXswizzle);
    kvValidators.emplace("KTXwriter", &ValidationContext::validateKTXwriter);
    kvValidators.emplace("KTXwriterScParams", &ValidationContext::validateKTXwriterScParams);
    kvValidators.emplace("KTXastcDecodeMode", &ValidationContext::validateKTXastcDecodeMode);
    kvValidators.emplace("KTXanimData", &ValidationContext::validateKTXanimData);

    for (const auto& entry : entries) {
        const auto it = kvValidators.find(entry.key);
        if (it != kvValidators.end()) {
            (this->*it->second)(entry.data, entry.size);
        } else {
            if (starts_with(entry.key, "KTX") || starts_with(entry.key, "ktx"))
                error(Metadata::UnknownReservedKey, entry.key);
            else
                warning(Metadata::CustomMetadata, entry.key);
        }
    }

    if (foundKTXanimData && foundKTXcubemapIncomplete)
        error(Metadata::KTXanimDataWithCubeIncomplete);

    if (!foundKTXwriter) {
        if (foundKTXwriterScParams)
            error(Metadata::KTXwriterRequiredButMissing);
        else
            warning(Metadata::KTXwriterMissing);
    }
}

void ValidationContext::validateKTXcubemapIncomplete(const uint8_t* data, uint32_t size) {
    foundKTXcubemapIncomplete = true;

    if (size != 1)
        error(Metadata::KTXcubemapIncompleteInvalidSize, size);

    if (size == 0)
        return;

    uint8_t value = *data;

    if (size > 0 && (value & 0b11000000u) != 0)
        error(Metadata::KTXcubemapIncompleteInvalidBitSet, value);
    value = value & 0b00111111u; // Error recovery

    const auto popCount = popcount(value);
    if (popCount == 6)
        warning(Metadata::KTXcubemapIncompleteAllBitsSet);

    if (popCount == 0)
        error(Metadata::KTXcubemapIncompleteNoBitSet);

    if (popCount != 0 && (header.layerCount % popCount != 0))
        error(Metadata::KTXcubemapIncompleteIncompatibleLayerCount, header.layerCount, popCount);

    if (header.faceCount != 1)
        error(Metadata::KTXcubemapIncompleteWithFaceCountNot1, header.faceCount);

    if (header.pixelHeight != header.pixelWidth)
        error(HeaderData::CubeHeightWidthMismatch, header.pixelWidth, header.pixelHeight);

    if (header.pixelDepth != 0)
        error(HeaderData::CubeWithDepth, header.pixelDepth);
}

void ValidationContext::validateKTXorientation(const uint8_t* data, uint32_t size) {
    foundKTXorientation = true;

    const auto hasNull = size > 0 && data[size - 1] == '\0';
    if (!hasNull)
        error(Metadata::KTXorientationMissingNull);

    const auto value = std::string_view(reinterpret_cast<const char*>(data), hasNull ? size - 1 : size);

    if (value.size() != dimensionCount)
        error(Metadata::KTXorientationIncorrectDimension, value.size(), dimensionCount);

    if (value.size() > 0 && dimensionCount > 0 && value[0] != 'r' && value[0] != 'l')
        error(Metadata::KTXorientationInvalidValue, 0, value[0], 'r', 'l');

    if (value.size() > 1 && dimensionCount > 1 && value[1] != 'd' && value[1] != 'u')
        error(Metadata::KTXorientationInvalidValue, 1, value[1], 'd', 'u');

    if (value.size() > 2 && dimensionCount > 2 && value[2] != 'o' && value[2] != 'i')
        error(Metadata::KTXorientationInvalidValue, 2, value[2], 'o', 'i');

    if (checkGLTFBasisU && value != "rd")
        error(Metadata::KTXorientationInvalidGLTFBU, value);
}

void ValidationContext::validateKTXglFormat(const uint8_t* data, uint32_t size) {
    foundKTXglFormat = true;

    if (header.vkFormat != VK_FORMAT_UNDEFINED)
        error(Metadata::KTXglFormatWithVkFormat, toString(VkFormat(header.vkFormat)));

    if (size != 12) {
        error(Metadata::KTXglFormatInvalidSize, size);
        return;
    }

    struct Value {
        uint32_t glInternalformat = 0;
        uint32_t glFormat = 0;
        uint32_t glType = 0;
    };

    Value value{};
    std::memcpy(&value, data, size);

    if (value.glFormat != 0 || value.glType != 0) {
        if (isSupercompressionBlockCompressed(ktxSupercmpScheme(header.supercompressionScheme)))
            error(Metadata::KTXglFormatInvalidValueForCompressed, value.glFormat, value.glType, toString(ktxSupercmpScheme(header.supercompressionScheme)));

        else if (parsedColorModel && isColorModelBlockCompressed(*parsedColorModel))
            error(Metadata::KTXglFormatInvalidValueForCompressed, value.glFormat, value.glType, toString(*parsedColorModel));
    }
}

void ValidationContext::validateKTXdxgiFormat(const uint8_t* data, uint32_t size) {
    (void) data;
    foundKTXdxgiFormat = true;

    if (header.vkFormat != VK_FORMAT_UNDEFINED)
        error(Metadata::KTXdxgiFormatWithVkFormat, toString(VkFormat(header.vkFormat)));

    if (size != 4)
        error(Metadata::KTXdxgiFormatInvalidSize, size);
}

void ValidationContext::validateKTXmetalPixelFormat(const uint8_t* data, uint32_t size) {
    (void) data;
    foundKTXmetalPixelFormat = true;

    if (header.vkFormat != VK_FORMAT_UNDEFINED)
        error(Metadata::KTXmetalPixelFormatWithVkFormat, toString(VkFormat(header.vkFormat)));

    if (size != 4)
        error(Metadata::KTXmetalPixelFormatInvalidSize, size);
}

void ValidationContext::validateKTXswizzle(const uint8_t* data, uint32_t size) {
    foundKTXswizzle = true;

    const auto hasNull = size > 0 && data[size - 1] == '\0';
    if (!hasNull)
        error(Metadata::KTXswizzleMissingNull);

    const auto value = std::string_view(reinterpret_cast<const char*>(data), hasNull ? size - 1 : size);

    if (value.size() != 4)
        error(Metadata::KTXswizzleInvalidSize, size);

    for (std::size_t i = 0; i < std::min(std::size_t{4}, value.size()); ++i) {
        if (value.size() > i && !contains("rgba01", value[i]))
            error(Metadata::KTXswizzleInvalidValue, i, value[i]);
    }

    if (isFormatStencil(VkFormat(header.vkFormat)) || isFormatDepth(VkFormat(header.vkFormat)))
        warning(Metadata::KTXswizzleWithDepthOrStencil, toString(VkFormat(header.vkFormat)));

    if (checkGLTFBasisU && value != "rgba")
        error(Metadata::KTXswizzleInvalidGLTFBU, value);
}

void ValidationContext::validateKTXwriter(const uint8_t* data, uint32_t size) {
    foundKTXwriter = true;

    const auto hasNull = size > 0 && data[size - 1] == '\0';
    if (!hasNull)
        error(Metadata::KTXwriterMissingNull);

    const auto value = std::string_view(reinterpret_cast<const char*>(data), hasNull ? size - 1 : size);
    if (auto invalidIndex = validateUTF8(value))
        error(Metadata::KTXwriterInvalidUTF8, *invalidIndex);
}

void ValidationContext::validateKTXwriterScParams(const uint8_t* data, uint32_t size) {
    foundKTXwriterScParams = true;

    const auto hasNull = size > 0 && data[size - 1] == '\0';
    if (!hasNull)
        error(Metadata::KTXwriterScParamsMissingNull);

    const auto value = std::string_view(reinterpret_cast<const char*>(data), hasNull ? size - 1 : size);
    if (auto invalidIndex = validateUTF8(value))
        error(Metadata::KTXwriterScParamsInvalidUTF8, *invalidIndex);
}

void ValidationContext::validateKTXastcDecodeMode(const uint8_t* data, uint32_t size) {
    foundKTXastcDecodeMode = true;

    const auto hasNull = size > 0 && data[size - 1] == '\0';
    if (!hasNull)
        error(Metadata::KTXastcDecodeModeMissingNull);

    const auto value = std::string_view(reinterpret_cast<const char*>(data), hasNull ? size - 1 : size);

    if (value != "rgb9e5" && value != "unorm8")
        error(Metadata::KTXastcDecodeModeInvalidValue, value);

    if (parsedColorModel && *parsedColorModel != KHR_DF_MODEL_ASTC)
        warning(Metadata::KTXastcDecodeModeNotASTC, toString(*parsedColorModel));
    else if (value == "unorm8" && !isFormatAstcLDR(VkFormat(header.vkFormat)))
        error(Metadata::KTXastcDecodeModeunorm8NotLDR, toString(VkFormat(header.vkFormat)));

    if (parsedTransferFunction && *parsedTransferFunction == KHR_DF_TRANSFER_SRGB)
         warning(Metadata::KTXastcDecodeModeWithsRGB, toString(*parsedTransferFunction));
}

void ValidationContext::validateKTXanimData(const uint8_t* data, uint32_t size) {
    (void) data;
    foundKTXanimData = true;

    if (size != 12)
        error(Metadata::KTXanimDataInvalidSize, size);

    if (header.layerCount == 0)
        error(Metadata::KTXanimDataNotArray, header.layerCount);
}

void ValidationContext::validateSGD() {
    const auto sgdByteOffset = header.supercompressionGlobalData.byteOffset;
    const auto sgdByteLength = header.supercompressionGlobalData.byteLength;

    if (sgdByteOffset == 0 || sgdByteLength == 0)
        return; // There is no SGD block

    const auto buffer = std::make_unique<uint8_t[]>(sgdByteLength);
    read(sgdByteOffset, buffer.get(), sgdByteLength, "the SGD");

    if (header.supercompressionScheme != KTX_SS_BASIS_LZ)
        return;

    // Validate BASIS_LZ SGD

    uint32_t imageCount = 0;
    // uint32_t layersFaces = numLayers * header.faceCount;
    for (uint32_t level = 0; level < numLevels; ++level)
        // numFaces * depth is only reasonable because they can't both be > 1. There are no 3D cubemaps
        imageCount += numLayers * header.faceCount * std::max(header.pixelDepth >> level, 1u);

    // Validate GlobalHeader
    if (sgdByteLength < sizeof(ktxBasisLzGlobalHeader)) {
        error(SGD::BLZESizeTooSmallHeader, sgdByteLength);
        return;
    }

    const ktxBasisLzGlobalHeader& bgh = *reinterpret_cast<const ktxBasisLzGlobalHeader*>(buffer.get());

    const uint64_t expectedBgdByteLength =
            sizeof(ktxBasisLzGlobalHeader) +
            sizeof(ktxBasisLzEtc1sImageDesc) * imageCount +
            bgh.endpointsByteLength +
            bgh.selectorsByteLength +
            bgh.tablesByteLength +
            bgh.extendedByteLength;
    if (sgdByteLength != expectedBgdByteLength)
        error(SGD::BLZESizeIncorrect, sgdByteLength, imageCount, expectedBgdByteLength);

    if (parsedColorModel && *parsedColorModel == KHR_DF_MODEL_ETC1S && bgh.extendedByteLength != 0)
        error(SGD::BLZEExtendedByteLengthNotZero, bgh.extendedByteLength);

    // Validate ImageDesc
    if (sgdByteLength < sizeof(ktxBasisLzGlobalHeader) + sizeof(ktxBasisLzEtc1sImageDesc) * imageCount)
        return;

    const ktxBasisLzEtc1sImageDesc* imageDescs = BGD_ETC1S_IMAGE_DESCS(buffer.get());

    bool foundPFrame = false;
    uint32_t i = 0;
    for (uint32_t level = 0; level < numLevels; ++level) {
        for (uint32_t layer = 0; layer < numLayers; ++layer) {
            for (uint32_t face = 0; face < header.faceCount; ++face) {
                for (uint32_t zSlice = 0; zSlice < std::max(header.pixelDepth >> level, 1u); ++zSlice) {
                    const auto imageIndex = i++;
                    const auto& image = imageDescs[imageIndex];

                    if (image.imageFlags & ETC1S_P_FRAME)
                        foundPFrame = true;

                    if (image.imageFlags & ~ETC1S_P_FRAME)
                        error(SGD::BLZEInvalidImageFlagBit, level, layer, face, zSlice, image.imageFlags);

                    if (image.rgbSliceByteLength == 0)
                        error(SGD::BLZEZeroRGBLength, level, layer, face, zSlice, image.rgbSliceByteLength);

                    if (image.rgbSliceByteOffset + image.rgbSliceByteLength > levelIndices[level].byteLength)
                        error(SGD::BLZEInvalidRGBSlice, level, layer, face, zSlice, image.rgbSliceByteOffset, image.rgbSliceByteLength, levelIndices[level].byteLength);
                    if (image.alphaSliceByteOffset + image.alphaSliceByteLength > levelIndices[level].byteLength)
                        error(SGD::BLZEInvalidAlphaSlice, level, layer, face, zSlice, image.alphaSliceByteOffset, image.alphaSliceByteLength, levelIndices[level].byteLength);

                    // Crosscheck with the DFD numSamples
                    if (image.alphaSliceByteLength == 0 && numSamples == 2)
                        error(SGD::BLZEDFDMismatchAlpha, level, layer, face, zSlice);
                    if (image.alphaSliceByteLength != 0 && numSamples == 1)
                        error(SGD::BLZEDFDMismatchNoAlpha, level, layer, face, zSlice, image.alphaSliceByteLength);
                }
            }
        }
    }

    if (foundPFrame)
        if (!foundKTXanimData)
            error(SGD::BLZENoAnimationSequencesPFrame);
}

void ValidationContext::validatePaddings() {
    const auto levelIndexOffset = sizeof(KTX_header2);
    const auto levelIndexSize = sizeof(ktxLevelIndexEntry) * numLevels;
    const auto dfdByteOffset = header.dataFormatDescriptor.byteOffset;
    const auto dfdByteLength = header.dataFormatDescriptor.byteLength;
    const auto kvdByteOffset = header.keyValueData.byteOffset;
    const auto kvdByteLength = header.keyValueData.byteLength;
    const auto sgdByteOffset = header.supercompressionGlobalData.byteOffset;
    const auto sgdByteLength = header.supercompressionGlobalData.byteLength;

    size_t position = levelIndexOffset + levelIndexSize;
    const auto check = [&](size_t offset, size_t size, std::string name) {
        if (offset == 0 || size == 0)
            return; // Block is missing, skip

        if (offset < position) {
            position = std::max(position, offset + size);
            return; // Just ignore invalid block placements regarding padding checks
        }

        const auto paddingSize = offset - position;
        const auto buffer = std::make_unique<uint8_t[]>(paddingSize);
        read(position, buffer.get(), paddingSize, "the padding before " + name);

        for (size_t i = 0; i < paddingSize; ++i)
            if (buffer[i] != 0) {
                error(Metadata::PaddingNotZero, buffer[i], fmt::format("before {} at offset {}", name, position + i));
                break; // Only report the first non-zero byte per padding, no need to spam
            }

        position = offset + size;
    };

    check(dfdByteOffset, dfdByteLength, "DFD");
    check(kvdByteOffset, kvdByteLength, "KVD");
    check(sgdByteOffset, sgdByteLength, "SGD");
    size_t i = levelIndices.size() - 1;
    for (auto it = levelIndices.rbegin(); it != levelIndices.rend(); ++it)
        check(it->byteOffset, it->byteLength, "image level " + std::to_string(i--));
}

void ValidationContext::validateCreateAndTranscode() {
    KTXTexture2 texture{nullptr};
    ktxTextureCreateFlags flags = KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT;

    if (checkGLTFBasisU)
        flags |= KTX_TEXTURE_CREATE_CHECK_GLTF_BASISU_BIT;

    ktx_error_code_e result = createKTXTexture(flags, texture.pHandle());

    if (numError == 0 && numWarning == 0) {
        switch (result) {
        case KTX_UNSUPPORTED_FEATURE:
            warning(Validator::UnsupportedFeature);
            break;

        case KTX_DECOMPRESS_LENGTH_ERROR:
            error(LevelIndex::UncompressedByteLengthMismatch,
                    toString(ktxSupercmpScheme(header.supercompressionScheme)));
            break;

        case KTX_DECOMPRESS_CHECKSUM_ERROR:
            error(Validator::DecompressChecksumError,
                    toString(ktxSupercmpScheme(header.supercompressionScheme)));
            break;

        case KTX_SUCCESS:
            if (parsedColorModel == KHR_DF_MODEL_ETC1S)
                result = ktxTexture2_TranscodeBasis(texture.handle(), KTX_TTF_ETC2_RGBA, 0);
            else if (parsedColorModel == KHR_DF_MODEL_UASTC)
                result = ktxTexture2_TranscodeBasis(texture.handle(), KTX_TTF_ASTC_4x4_RGBA, 0);

            if (result != KTX_SUCCESS)
                error(Validator::TranscodeFailure, toString(*parsedColorModel), ktxErrorString(result));
            break;

        default:
            fatal(Validator::CreateFailure, ktxErrorString(result));
            break;
        }
    } else if (result == KTX_SUCCESS && numError != 0) {
        warning(Validator::SupportedNonConformantFile);
    }
}

} // namespace ktx

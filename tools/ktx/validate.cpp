// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include "validate.h"

#include <fmt/format.h>
#include <fmt/printf.h>
#include <KHR/khr_df.h>
#include <ktx.h>
#include "ktxint.h"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <utility>
#include <unordered_map>
#include <unordered_set>

#include "utility.h"
#include "validation_messages.h"
#include "formats.h"
#define LIBKTX // To stop dfdutils including vulkan_core.h.
#include "dfdutils/dfd.h"


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

/// RAII Handler for ktxTexture
template <typename T>
class KTXTexture final {
private:
    T* _handle;

public:
    explicit KTXTexture(std::nullptr_t) : _handle{nullptr} { }
    explicit KTXTexture(T* handle) : _handle{handle} { }

    KTXTexture(const KTXTexture&) = delete;
    KTXTexture& operator=(const KTXTexture&) = delete;

    KTXTexture(KTXTexture&& other) noexcept : _handle{other._handle} {
        other._handle = nullptr;
    }

    KTXTexture& operator=(KTXTexture&& toMove) & {
        _handle = toMove._handle;
        toMove._handle = nullptr;
        return *this;
    }

    ~KTXTexture() {
        if (_handle) {
            ktxTexture_Destroy(handle<ktxTexture>());
            _handle = nullptr;
        }
    }

    template <typename U = T>
    inline U* handle() const {
        return reinterpret_cast<U*>(_handle);
    }

    template <typename U = T>
    inline U** pHandle() {
        return reinterpret_cast<U**>(&_handle);
    }

    inline operator T*() const {
        return _handle;
    }
};

class FatalValidationError : public std::runtime_error {
public:
    ValidationReport report;

public:
    explicit FatalValidationError(ValidationReport report) :
        std::runtime_error(report.details),
        report(std::move(report)) {}
};

static constexpr int RETURN_CODE_VALIDATION_FAILURE = 3;
static constexpr int RETURN_CODE_VALIDATION_SUCCESS = 0;
static constexpr int MAX_NUM_KV_ENTRY = 100;
static constexpr int MAX_NUM_DFD_BLOCK = 10;

// -------------------------------------------------------------------------------------------------

struct ValidationContext {
private:
    std::function<void(const ValidationReport&)> callback;

    bool treatWarningsAsError = false;
    // bool checkGLTFBasisu = false; // TODO Tools P2: implement checkGLTFBasisu

    uint32_t numError = 0;
    uint32_t numWarning = 0;

private:
    KTX_header2 header{};

    uint32_t layerCount = 0;
    uint32_t levelCount = 0;
    uint32_t dimensionCount = 0;

private:
    /// Expected data members are calculated solely from the VkFormat in the header.
    /// Based on parsing and support any of these member can be empty.
    std::optional<khr_df_model_e> expectedColorModel;
    std::optional<std::array<uint32_t, 8>> expectedBytePlanes;
    std::optional<uint32_t> expectedBlockDimension0;
    std::optional<uint32_t> expectedBlockDimension1;
    std::optional<uint32_t> expectedBlockDimension2;
    std::optional<uint32_t> expectedBlockDimension3;
    std::optional<bool> expectedBlockCompressedColorModel;
    std::optional<uint32_t> expectedTypeSize;
    std::optional<std::vector<SampleType>> expectedSamples;

private:
    /// The actually parsed BDFD members.
    /// Based on parsing and support any of these member can be empty.
    std::optional<khr_df_model_e> parsedColorModel;
    std::optional<khr_df_transfer_e> parsedTransferFunction;

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
    ValidationContext(bool warningsAsErrors, std::function<void(const ValidationReport&)> callback) :
        callback(std::move(callback)),
        treatWarningsAsError(warningsAsErrors) {
        std::memset(&header, 0, sizeof(header));
    }
    virtual ~ValidationContext() = default;

protected:
    // warning, error and fatal functions are only used for validation readability
    template <typename... Args>
    void warning(const IssueWarning& issue, Args&&... args) {
        if (treatWarningsAsError) {
            ++numError;
            callback(ValidationReport{IssueType::error, issue.id, std::string{issue.message}, fmt::format(issue.detailsFmt, std::forward<Args>(args)...)});

        } else {
            ++numWarning;
            callback(ValidationReport{issue.type, issue.id, std::string{issue.message}, fmt::format(issue.detailsFmt, std::forward<Args>(args)...)});
        }
    }

    template <typename... Args>
    void error(const IssueError& issue, Args&&... args) {
        ++numError;
        callback(ValidationReport{issue.type, issue.id, std::string{issue.message}, fmt::format(issue.detailsFmt, std::forward<Args>(args)...)});
    }

    template <typename... Args>
    void fatal(const IssueFatal& issue, Args&&... args) {
        ++numError;
        const auto report = ValidationReport{issue.type, issue.id, std::string{issue.message}, fmt::format(issue.detailsFmt, std::forward<Args>(args)...)};
        callback(report);

        throw FatalValidationError(report);
    }

private:
    virtual void read(std::size_t offset, void* readDst, std::size_t readSize, std::string_view name) = 0;
    virtual KTX_error_code createKTXTexture(ktxTextureCreateFlags createFlags, ktxTexture2** newTex) = 0;

private:
    template <typename... Args>
    void validatePaddingZeros(const void* ptr, const void* bufferEnd, std::size_t alignment, const IssueError& issue, Args&&... args) {
        const auto* begin = static_cast<const char*>(ptr);
        const auto* end = std::min(bufferEnd, align(ptr, alignment));

        for (auto it = begin; it != end; ++it)
            if (*it != 0)
                error(issue, static_cast<uint8_t>(*it), std::forward<Args>(args)...);
    }

public:
    int validate();

private:
    void validateHeader();
    void validateIndices();
    void calculateExpectedDFD(VkFormat format);
    // void validateLevelIndex();
    void validateDFD();
    void validateKVD();
    // void validateSGD();
    // void validateCreateAndTranscode();

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
};

// -------------------------------------------------------------------------------------------------

class ValidationContextStream : public ValidationContext {
private:
    FileGuard fileGuard; /// Only used if the file is owned by the validation context
    FILE* file;

public:
    ValidationContextStream(bool warningsAsErrors, std::function<void(const ValidationReport&)> callback, FILE* file) :
            ValidationContext(warningsAsErrors, std::move(callback)),
            file(file) {
    }

    ValidationContextStream(bool warningsAsErrors, std::function<void(const ValidationReport&)> callback, const _tstring& filepath) :
            ValidationContext(warningsAsErrors, std::move(callback)),
            fileGuard(filepath.c_str(), "rb"),
            file(fileGuard) {

        if (!file) {
            const auto ec = std::make_error_code(static_cast<std::errc>(errno));
            fatal(IOError::FileOpen, filepath, ec.message());
        }
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
            std::function<void(const ValidationReport&)> callback,
            const char* memoryData,
            std::size_t memorySize) :
        ValidationContext(warningsAsErrors, std::move(callback)),
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

int validateFile(const _tstring& filepath, bool warningsAsErrors, std::function<void(const ValidationReport&)> callback) {
    try {
        ValidationContextStream ctx{warningsAsErrors, std::move(callback), filepath};
        return ctx.validate();
    } catch (const FatalValidationError&) {
        return RETURN_CODE_VALIDATION_FAILURE;
    }
}

int validateStream(FILE* file, bool warningsAsErrors, std::function<void(const ValidationReport&)> callback) {
    try {
        ValidationContextStream ctx{warningsAsErrors, std::move(callback), file};
        return ctx.validate();
    } catch (const FatalValidationError&) {
        return RETURN_CODE_VALIDATION_FAILURE;
    }
}

int validateMemory(const char* data, std::size_t size, bool warningsAsErrors, std::function<void(const ValidationReport&)> callback) {
    try {
        ValidationContextMemory ctx{warningsAsErrors, std::move(callback), data, size};
        return ctx.validate();
    } catch (const FatalValidationError&) {
        return RETURN_CODE_VALIDATION_FAILURE;
    }
}

// -------------------------------------------------------------------------------------------------

int ValidationContext::validate() {
    validateHeader();
    validateIndices();
    // validateLevelIndex();
    calculateExpectedDFD(VkFormat(header.vkFormat));
    validateDFD();
    validateKVD();
    // validateSGD();
    // validateCreateAndTranscode();

    // TODO Tools P3: Verify alignment (4) padding zeros between levelIndex and DFD
    // TODO Tools P3: Verify alignment (4) padding zeros between DFD and KVD
    // TODO Tools P3: Verify alignment (8) padding zeros between KVD and SGD
    // TODO Tools P3: Verify alignment (?) padding zeros between SGD and image levels
    // TODO Tools P3: Verify alignment (?) padding zeros between image levels

    return numError == 0 ? RETURN_CODE_VALIDATION_SUCCESS : RETURN_CODE_VALIDATION_FAILURE;
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
        if (1000001000 <= vkFormat)
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
    layerCount = std::max(header.layerCount, 1u);

    // Validate faceCount
    if (header.faceCount != 6 && header.faceCount != 1)
        error(HeaderData::InvalidFaceCount, header.faceCount);

    // Cube map faces are validated 2D by checking: CubeHeightWidthMismatch and CubeWithDepth

    // Validate levelCount
    if (isFormatBlockCompressed(vkFormat))
        if (header.levelCount == 0)
            error(HeaderData::BlockCompressedNoLevel, toString(vkFormat));
    if (isSupercompressionBlockCompressed(supercompressionScheme))
        if (header.levelCount == 0)
            error(HeaderData::BlockCompressedNoLevel, toString(supercompressionScheme));
    // Additional block-compressed formats (like UASTC) are detected after the DFD is parsed to validate levelCount

    levelCount = std::max(header.levelCount, 1u);

    // This test works for arrays too because height or depth will be 0.
    const auto max_dim = std::max(std::max(header.pixelWidth, header.pixelHeight), header.pixelDepth);
    // TODO Tools P4: Verify 'log2'
    if (max_dim < ((uint32_t) 1 << (levelCount - 1))) {
        // Can't have more mip levels than 1 + log2(max(width, height, depth))
        error(HeaderData::TooManyMipLevels, levelCount, max_dim);
    }

    // Validate supercompressionScheme
    if (KTX_SS_BEGIN_VENDOR_RANGE <= header.supercompressionScheme && header.supercompressionScheme <= KTX_SS_END_VENDOR_RANGE)
        warning(HeaderData::VendorSupercompression, toString(supercompressionScheme));
    else if (header.supercompressionScheme < KTX_SS_BEGIN_RANGE || KTX_SS_END_RANGE < header.supercompressionScheme)
        error(HeaderData::InvalidSupercompression, toString(supercompressionScheme));
}

void ValidationContext::validateIndices() {
    const auto supercompressionScheme = ktxSupercmpScheme(header.supercompressionScheme);

    // Validate dataFormatDescriptor index
    if (header.dataFormatDescriptor.byteOffset == 0 || header.dataFormatDescriptor.byteLength == 0)
        error(HeaderData::IndexDFDMissing, header.dataFormatDescriptor.byteOffset, header.dataFormatDescriptor.byteLength);

    const auto levelIndexSize = sizeof(ktxLevelIndexEntry) * levelCount;
    std::size_t expectedOffset = KTX2_HEADER_SIZE + levelIndexSize;
    expectedOffset = align(expectedOffset, std::size_t{4});
    if (expectedOffset != header.dataFormatDescriptor.byteOffset)
        error(HeaderData::IndexDFDInvalidOffset, header.dataFormatDescriptor.byteOffset, expectedOffset);
    expectedOffset += header.dataFormatDescriptor.byteLength;

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

// =================================================================================================
// TODO Tools P2: Validate Level Index
//
// void ValidationContext::validateLevelIndex() {
//     std::vector<ktxLevelIndexEntry> levelIndices(levelCount);
//
//     const auto levelIndexSize = sizeof(ktxLevelIndexEntry) * levelCount;
//     read(levelIndices.data(), levelIndexSize, "the level index");
//
//     validateDfd(ctx);
//     if (!ctx.pDfd4Format) {
//         // VK_FORMAT_UNDEFINED so we have to get info from the actual DFD.
//         // Not hugely robust but validateDfd does check known undefineds such
//         // as UASTC.
//         if (!ctx.extractFormatInfo(ctx.pActualDfd)) {
//             addIssue(logger::eError, ValidatorError.DfdValidationFailure);
//         }
//     }
//
//     uint32_t requiredLevelAlignment = ctx.requiredLevelAlignment();
//     // size_t expectedOffset = 0;
//     // size_t lastByteLength = 0;
//     //
//     // switch (ctx.header.supercompressionScheme) {
//     //   case KTX_SS_NONE:
//     //   case KTX_SS_ZSTD:
//     //     expectedOffset = padn(requiredLevelAlignment, ctx.kvDataEndOffset());
//     //     break;
//     //   case KTX_SS_BASIS_LZ:
//     //     ktxIndexEntry64 sgdIndex = ctx.header.supercompressionGlobalData;
//     //     // No padding here.
//     //     expectedOffset = sgdIndex.byteOffset + sgdIndex.byteLength;
//     //     break;
//     // }
//     //
//     // expectedOffset = padn(requiredLevelAlignment, expectedOffset);
//
//     // Last mip level is first in the file. Count down, so we can check the
//     // distance between levels for the UNDEFINED and SUPERCOMPRESSION cases.
//     for (int32_t levelIt = static_cast<int32_t>(levelCount) - 1; levelIt >= 0; --levelIt) {
//         const auto& level = levelIndices[static_cast<std::size_t>(levelIt)];
//
//         const auto knownLevelSizes =
//                 header.vkFormat != VK_FORMAT_UNDEFINED &&
//                 header.supercompressionScheme == KTX_SS_NONE;
//
//         if (knownLevelSizes) {
//             ktx_size_t expectedUBL = ctx.calcLevelSize(level);
//             if (level.uncompressedByteLength != expectedUBL)
//                 error(LevelIndex::IncorrectUncompressedByteLength, level, level.uncompressedByteLength, expectedUBL);
//
//             if (level.byteLength != level.uncompressedByteLength)
//                 error(LevelIndex::UnequalByteLengths, level);
//
//             ktx_size_t expectedOffset = ctx.calcLevelOffset(level);
//             if (level.byteOffset != expectedOffset) {
//                 if (level.byteOffset % requiredLevelAlignment != 0)
//                     error(LevelIndex::UnalignedOffset, level, requiredLevelAlignment);
//
//                 if (level.byteOffset > expectedOffset)
//                     error(LevelIndex::ExtraPadding, level);
//
//                 else
//                     error(LevelIndex::ByteOffsetTooSmall, level, level.byteOffset, expectedOffset);
//             }
//
//         } else {
//             // Can only do minimal validation as we have no idea what the
//             // level sizes are, so we have to trust the byteLengths. We do
//             // at least know where the first level must be in the file, and
//             // we can calculate how much padding, if any, there must be
//             // between levels.
//             // if (level.byteLength == 0 || level.byteOffset == 0) {
//             //      error(LevelIndex::ZeroOffsetOrLength, level);
//             //      continue;
//             // }
//             //
//             // if (level.byteOffset != expectedOffset) {
//             //     error(LevelIndex::IncorrectByteOffset, level, level.byteOffset, expectedOffset);
//             // }
//             //
//             // if (header.supercompressionScheme == KTX_SS_NONE) {
//             //     if (level.byteLength < lastByteLength)
//             //         addIssue(logger.eError, LevelIndex::IncorrectLevelOrder);
//             //     if (level.byteOffset % requiredLevelAlignment != 0)
//             //         error(LevelIndex::UnalignedOffset, level, requiredLevelAlignment);
//             //     if (level.uncompressedByteLength == 0) {
//             //         error(LevelIndex::ZeroUncompressedLength, level);
//             //     }
//             //     lastByteLength = level.byteLength;
//             // }
//             //
//             // expectedOffset += padn(requiredLevelAlignment, level.byteLength);
//             // if (header.vkFormat != VK_FORMAT_UNDEFINED) {
//             //     // We can validate the uncompressedByteLength.
//             //     ktx_size_t level.uncompressedByteLength = level.uncompressedByteLength;
//             //     ktx_size_t expectedUBL = ctx.calcLevelSize(level);
//             //
//             //     if (level.uncompressedByteLength != expectedUBL)
//             //         error(LevelIndex::IncorrectUncompressedByteLength, level, level.uncompressedByteLength, expectedUBL);
//             // }
//         }
//
//         // ctx.dataSizeFromLevelIndex += padn(ctx.requiredLevelAlignment(), level.byteLength);
//     }
// }
// =================================================================================================

void ValidationContext::calculateExpectedDFD(VkFormat format) {
    if (format == VK_FORMAT_UNDEFINED || !isFormatValid(format) || isProhibitedFormat(format))
        return;

    std::unique_ptr<uint32_t[]> dfd;

    switch (header.vkFormat) {
    case VK_FORMAT_D16_UNORM_S8_UINT:
        // 2 16-bit words. D16 in the first. S8 in the 8 LSBs of the second.
        dfd = std::unique_ptr<uint32_t[]>(createDFDDepthStencil(16, 8, 4));
        break;
    case VK_FORMAT_D24_UNORM_S8_UINT:
        // 1 32-bit word. D24 in the MSBs. S8 in the LSBs.
        dfd = std::unique_ptr<uint32_t[]>(createDFDDepthStencil(24, 8, 4));
        break;
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        // 2 32-bit words. D32 float in the first word. S8 in LSBs of the second.
        dfd = std::unique_ptr<uint32_t[]>(createDFDDepthStencil(32, 8, 8));
        break;
    default:
        dfd = std::unique_ptr<uint32_t[]>(vk2dfd(format));
    }

    if (dfd == nullptr) {
        error(ValidatorError::CreateExpectedDFDFailure, toString(format));
        return;
    }

    const uint32_t* bdfd = dfd.get() + 1;

    const auto expectedSampleCount = KHR_DFDSAMPLECOUNT(bdfd);
    expectedSamples.emplace(expectedSampleCount, SampleType{});
    std::memcpy(expectedSamples->data(), reinterpret_cast<const char*>(bdfd) + sizeof(BDFD), expectedSampleCount * sizeof(SampleType));

    expectedColorModel = khr_df_model_e(KHR_DFDVAL(bdfd, MODEL));
    expectedBytePlanes.emplace();
    expectedBytePlanes.value()[0] = KHR_DFDVAL(bdfd, BYTESPLANE0);
    expectedBytePlanes.value()[1] = KHR_DFDVAL(bdfd, BYTESPLANE1);
    expectedBytePlanes.value()[2] = KHR_DFDVAL(bdfd, BYTESPLANE2);
    expectedBytePlanes.value()[3] = KHR_DFDVAL(bdfd, BYTESPLANE3);
    expectedBytePlanes.value()[4] = KHR_DFDVAL(bdfd, BYTESPLANE4);
    expectedBytePlanes.value()[5] = KHR_DFDVAL(bdfd, BYTESPLANE5);
    expectedBytePlanes.value()[6] = KHR_DFDVAL(bdfd, BYTESPLANE6);
    expectedBytePlanes.value()[7] = KHR_DFDVAL(bdfd, BYTESPLANE7);
    expectedBlockDimension0 = KHR_DFDVAL(bdfd, TEXELBLOCKDIMENSION0);
    expectedBlockDimension1 = KHR_DFDVAL(bdfd, TEXELBLOCKDIMENSION1);
    expectedBlockDimension2 = KHR_DFDVAL(bdfd, TEXELBLOCKDIMENSION2);
    expectedBlockDimension3 = KHR_DFDVAL(bdfd, TEXELBLOCKDIMENSION3);
    expectedBlockCompressedColorModel = *expectedColorModel >= KHR_DF_MODEL_DXT1A;

    if (!*expectedBlockCompressedColorModel) {
        InterpretedDFDChannel r, g, b, a;
        InterpretDFDResult result;
        uint32_t componentByteLength;

        result = interpretDFD(dfd.get(), &r, &g, &b, &a, &componentByteLength);

        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT)
            // Reset the "false" positive error in interpretDFD with VK_FORMAT_D32_SFLOAT_S8_UINT
            result = InterpretDFDResult(result & ~i_UNSUPPORTED_MIXED_CHANNELS);

        if (result > i_UNSUPPORTED_ERROR_BIT)
            error(ValidatorError::CreateDFDRoundtripFailed, toString(format));
        else
            expectedTypeSize = componentByteLength;
    }
}

void ValidationContext::validateDFD() {
    if (header.dataFormatDescriptor.byteOffset == 0 || header.dataFormatDescriptor.byteLength == 0)
        return; // There is no DFD block

    const auto buffer = std::make_unique<uint8_t[]>(header.dataFormatDescriptor.byteLength);
    read(header.dataFormatDescriptor.byteOffset, buffer.get(), header.dataFormatDescriptor.byteLength, "the DFD");
    const auto* ptrDFD = buffer.get();
    const auto* ptrDFDEnd = ptrDFD + header.dataFormatDescriptor.byteLength;
    const auto* ptrDFDIt = ptrDFD;

    uint32_t dfdTotalSize;
    std::memcpy(&dfdTotalSize, ptrDFDIt, sizeof(uint32_t));
    ptrDFDIt += sizeof(uint32_t);

    if (header.dataFormatDescriptor.byteLength != dfdTotalSize)
        error(DFD::SizeMismatch, header.dataFormatDescriptor.byteLength, dfdTotalSize);

    int numBlocks = 0;
    bool foundBDFD = false;

    while (ptrDFDIt < ptrDFDEnd) {
        const auto remainingDFDBytes = static_cast<std::size_t>(ptrDFDEnd - ptrDFDIt);

        if (++numBlocks > MAX_NUM_DFD_BLOCK) {
            error(DFD::TooManyDFDBlocks, numBlocks, remainingDFDBytes);
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

                    const auto numSample = (block.descriptorBlockSize - 24) / 16;

                    // Samples are located at the end of the block
                    std::vector<SampleType> samples(numSample);
                    std::memcpy(samples.data(), ptrDFDIt + sizeof(BDFD), numSample * sizeof(SampleType));

                    validateDFDBasic(numBlocks, reinterpret_cast<uint32_t*>(buffer.get()), block, samples);
                }

            } else if (blockHeader.vendorId == KHR_DF_VENDORID_KHRONOS && blockHeader.descriptorType == KHR_DF_KHR_DESCRIPTORTYPE_ADDITIONAL_DIMENSIONS) {
                // TODO Tools P5: Implement DFD validation for ADDITIONAL_DIMENSIONS

            } else if (blockHeader.vendorId == KHR_DF_VENDORID_KHRONOS && blockHeader.descriptorType == KHR_DF_KHR_DESCRIPTORTYPE_ADDITIONAL_PLANES) {
                // TODO Tools P5: Implement DFD validation for ADDITIONAL_PLANES

            } else {
                warning(DFD::UnknownDFDBlock,
                        numBlocks,
                        toString(khr_df_vendorid_e(blockHeader.vendorId)),
                        toString(khr_df_vendorid_e(blockHeader.vendorId), khr_df_khr_descriptortype_e(blockHeader.descriptorType)));
            }
        }

        ptrDFDIt += std::max(blockHeader.descriptorBlockSize, 8u);
    }

    // validatePaddingZeros(ptrDFDIt, ptrDFDEnd, 4, Metadata::PaddingNotZero, "after the last DFD block");

    if (!foundBDFD)
        error(DFD::MissingBDFD);
}

void ValidationContext::validateDFDBasic(uint32_t blockIndex, const uint32_t* dfd, const BDFD& block, const std::vector<SampleType>& samples) {

    parsedColorModel = khr_df_model_e(block.model);
    parsedTransferFunction = khr_df_transfer_e(block.transfer);

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
        khr_df_model_e expectedColorModel = getColorModelForBlockCompressedFormat(VkFormat(header.vkFormat));
        if (block.model != expectedColorModel)
            error(DFD::IncorrectModelForBlock, blockIndex, toString(khr_df_model_e(block.model)), toString(VkFormat(header.vkFormat)), toString(expectedColorModel));

    } else if (header.vkFormat != VK_FORMAT_UNDEFINED) {
        if (block.model != KHR_DF_MODEL_RGBSDA)
            error(DFD::IncorrectModelForRGB, blockIndex, toString(khr_df_model_e(block.model)), toString(VkFormat(header.vkFormat)));
    }

    if (header.supercompressionScheme == KTX_SS_BASIS_LZ)
        if (block.model != KHR_DF_MODEL_ETC1S)
            error(DFD::IncorrectModelForBLZE, blockIndex, toString(khr_df_model_e(block.model)));

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
                        error(DFD::FormatMismatch, blockIndex, i + 1, "bitOffset", parsed.bitOffset, expected.bitOffset, toString(VkFormat(header.vkFormat)));
                    if (parsed.bitLength != expected.bitLength)
                        error(DFD::FormatMismatch, blockIndex, i + 1, "bitLength", parsed.bitLength, expected.bitLength, toString(VkFormat(header.vkFormat)));
                    if (parsed.channelType != expected.channelType)
                        error(DFD::FormatMismatch, blockIndex, i + 1, "channelType",
                                toString(khr_df_model_e(block.model), khr_df_model_channels_e(parsed.channelType)),
                                toString(expectedColorModel.value_or(KHR_DF_MODEL_UNSPECIFIED), khr_df_model_channels_e(expected.channelType)),
                                toString(VkFormat(header.vkFormat)));
                    if (parsed.qualifierLinear != expected.qualifierLinear)
                        error(DFD::FormatMismatch, blockIndex, i + 1, "qualifierLinear", parsed.qualifierLinear, expected.qualifierLinear, toString(VkFormat(header.vkFormat)));
                    if (parsed.qualifierExponent != expected.qualifierExponent)
                        error(DFD::FormatMismatch, blockIndex, i + 1, "qualifierExponent", parsed.qualifierExponent, expected.qualifierExponent, toString(VkFormat(header.vkFormat)));
                    if (parsed.qualifierSigned != expected.qualifierSigned)
                        error(DFD::FormatMismatch, blockIndex, i + 1, "qualifierSigned", parsed.qualifierSigned, expected.qualifierSigned, toString(VkFormat(header.vkFormat)));
                    if (parsed.qualifierFloat != expected.qualifierFloat)
                        error(DFD::FormatMismatch, blockIndex, i + 1, "qualifierFloat", parsed.qualifierFloat, expected.qualifierFloat, toString(VkFormat(header.vkFormat)));
                    if (parsed.samplePosition0 != expected.samplePosition0)
                        error(DFD::FormatMismatch, blockIndex, i + 1, "samplePosition0", parsed.samplePosition0, expected.samplePosition0, toString(VkFormat(header.vkFormat)));
                    if (parsed.samplePosition1 != expected.samplePosition1)
                        error(DFD::FormatMismatch, blockIndex, i + 1, "samplePosition1", parsed.samplePosition1, expected.samplePosition1, toString(VkFormat(header.vkFormat)));
                    if (parsed.samplePosition2 != expected.samplePosition2)
                        error(DFD::FormatMismatch, blockIndex, i + 1, "samplePosition2", parsed.samplePosition2, expected.samplePosition2, toString(VkFormat(header.vkFormat)));
                    if (parsed.samplePosition3 != expected.samplePosition3)
                        error(DFD::FormatMismatch, blockIndex, i + 1, "samplePosition3", parsed.samplePosition3, expected.samplePosition3, toString(VkFormat(header.vkFormat)));
                    if (parsed.lower != expected.lower)
                        error(DFD::FormatMismatch, blockIndex, i + 1, "sampleLower", parsed.lower, expected.lower, toString(VkFormat(header.vkFormat)));
                    if (parsed.upper != expected.upper)
                        error(DFD::FormatMismatch, blockIndex, i + 1, "sampleUpper", parsed.upper, expected.upper, toString(VkFormat(header.vkFormat)));
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
                uint32_t componentByteLength;

                result = interpretDFD(dfd, &r, &g, &b, &a, &componentByteLength);

                if (header.vkFormat == VK_FORMAT_D32_SFLOAT_S8_UINT)
                    // Reset the "false" positive error in interpretDFD with VK_FORMAT_D32_SFLOAT_S8_UINT
                    result = InterpretDFDResult(result & ~i_UNSUPPORTED_MIXED_CHANNELS);

                if (result >= i_UNSUPPORTED_ERROR_BIT) {
                    switch (result) {
                    case i_UNSUPPORTED_CHANNEL_TYPES:
                        // TODO Tools P4: We intentionally ignore this error from interpretDFD().
                        // We already check channel types elsewhere and interpretDFD() currently doesn't
                        // handle anything else but the RGBSDA color model which is insufficient to handle
                        // cases like the allowed 422 YCbCr formats.
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
    // Check that were deferred during header parsing until the BDFD is available

    if (header.vkFormat == VK_FORMAT_UNDEFINED && !isSupercompressionBlockCompressed(ktxSupercmpScheme(header.supercompressionScheme))) {
        // Not VK_FORMAT_UNDEFINED and BlockCompressed Supercompressions were already checked before

        if (isColorModelBlockCompressed(khr_df_model_e(block.model))) {
            if (header.pixelHeight == 0)
                error(HeaderData::BlockCompressedNoHeight, toString(khr_df_model_e(block.model)));

            if (header.levelCount == 0)
                error(HeaderData::BlockCompressedNoLevel, toString(khr_df_model_e(block.model)));
        }
    }

    // TODO Tools P4: Verify these error condition
    // if (header.supercompressionScheme != KTX_SS_BASIS_LZ) {
    //     if (isColorModelBlockCompressed(khr_df_model_e(block.model))) {
    //         if (header.typeSize != 1)
    //             error(HeaderData::TypeSizeNotOne);
    //     } else {
    //         if (expectedTypeSize && header.typeSize != expectedTypeSize)
    //             error(HeaderData::TypeSizeMismatch, header.typeSize);
    //     }
    // }
}

void ValidationContext::validateKVD() {
    if (header.keyValueData.byteOffset == 0 || header.keyValueData.byteLength == 0)
        return; // There is no KVD block

    const auto buffer = std::make_unique<uint8_t[]>(header.keyValueData.byteLength);
    read(header.keyValueData.byteOffset, buffer.get(), header.keyValueData.byteLength, "the Key/Value Data");
    const auto* ptrKVD = buffer.get();
    const auto* ptrKVDEnd = ptrKVD + header.keyValueData.byteLength;

    struct KeyValueEntry {
        std::string key;
        const uint8_t* data;
        uint32_t size;

        KeyValueEntry(std::string_view key, const uint8_t* data, uint32_t size) :
                key(key), data(data), size(size) {}
    };
    std::vector<KeyValueEntry> entries;
    std::unordered_set<std::string_view> keys;

    int numKVEntry = 0;
    // Process Key-Value entries {size, key, \0, value} until the end of the KVD block
    // Where size is an uint32_t, and it equals to: sizeof(key) + 1 + sizeof(value)
    const auto* ptrEntry = ptrKVD;
    while (ptrEntry < ptrKVDEnd) {
        const auto remainingKVDBytes = ptrKVDEnd - ptrEntry;

        if (++numKVEntry > MAX_NUM_KV_ENTRY) {
            error(Metadata::TooManyEntries, numKVEntry - 1, remainingKVDBytes);
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
        validatePaddingZeros(ptrEntry, ptrKVDEnd, 4, Metadata::PaddingNotZero, "after a Key-Value entry");
        ptrEntry = align(ptrEntry, 4);
    }

    if (ptrEntry != ptrKVDEnd)
        // Being super explicit about the specs. This check might be overkill as other checks often cover this case
        error(Metadata::SizesDontAddUp, ptrEntry - ptrKVD, header.keyValueData.byteLength);

    if (header.supercompressionGlobalData.byteLength != 0)
        validatePaddingZeros(ptrEntry, ptrKVDEnd, 8, Metadata::PaddingNotZero, "between KVD and SGD");

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

// =================================================================================================
// TODO Tools P2: Validate SGD
//
// void ktxValidator::validateSgd(validationContext& ctx) {
//     uint64_t sgdByteLength = ctx.header.supercompressionGlobalData.byteLength;
//     if (ctx.header.supercompressionScheme == KTX_SS_BASIS_LZ) {
//         if (sgdByteLength == 0) {
//             addIssue(logger::eError, SGD.MissingSupercompressionGlobalData);
//             return;
//         }
//     } else {
//         if (sgdByteLength > 0)
//             addIssue(logger::eError, SGD.UnexpectedSupercompressionGlobalData);
//         return;
//     }
//
//     uint8_t* sgd = new uint8_t[sgdByteLength];
//     ctx.inp->read((char*) sgd, sgdByteLength);
//     if (ctx.inp->fail())
//         addIssue(logger::eFatal, IOError.FileRead, strerror(errno));
//     else if (ctx.inp->eof())
//         addIssue(logger::eFatal, IOError.UnexpectedEOF);
//
//     // firstImages contains the indices of the first images for each level.
//     // The last array entry contains the total number of images which is what
//     // we need here.
//     uint32_t* firstImages = new uint32_t[ctx.levelCount + 1];
//     // Temporary invariant value
//     uint32_t layersFaces = ctx.layerCount * ctx.header.faceCount;
//     firstImages[0] = 0;
//     for (uint32_t level = 1; level <= ctx.levelCount; level++) {
//         // NOTA BENE: numFaces * depth is only reasonable because they can't
//         // both be > 1. I.e there are no 3d cubemaps.
//         firstImages[level] = firstImages[level - 1]
//                 + layersFaces * MAX(ctx.header.pixelDepth >> (level - 1), 1);
//     }
//     uint32_t& imageCount = firstImages[ctx.levelCount];
//
//     ktxBasisLzGlobalHeader& bgdh = *reinterpret_cast<ktxBasisLzGlobalHeader*>(sgd);
//     uint32_t numSamples = KHR_DFDSAMPLECOUNT(ctx.pActualDfd + 1);
//
//     uint64_t expectedBgdByteLength = sizeof(ktxBasisLzGlobalHeader)
//             + sizeof(ktxBasisLzEtc1sImageDesc) * imageCount
//             + bgdh.endpointsByteLength
//             + bgdh.selectorsByteLength
//             + bgdh.tablesByteLength;
//
//     ktxBasisLzEtc1sImageDesc* imageDescs = BGD_ETC1S_IMAGE_DESCS(sgd);
//     ktxBasisLzEtc1sImageDesc* image = imageDescs;
//     for (; image < imageDescs + imageCount; image++) {
//         if (image->imageFlags & ~ETC1S_P_FRAME)
//             addIssue(logger::eError, SGD.InvalidImageFlagBit);
//         // Crosscheck the DFD.
//         if (image->alphaSliceByteOffset == 0 && numSamples == 2)
//             addIssue(logger::eError, SGD.DfdMismatchAlpha);
//         if (image->alphaSliceByteOffset > 0 && numSamples == 1)
//             addIssue(logger::eError, SGD.DfdMismatchNoAlpha);
//     }
//
//     if (sgdByteLength != expectedBgdByteLength)
//         addIssue(logger::eError, SGD.IncorrectGlobalDataSize);
//
//     if (bgdh.extendedByteLength != 0)
//         addIssue(logger::eError, SGD.ExtendedByteLengthNotZero);
//
//     // Can't do anymore as we have no idea how many endpoints, etc there
//     // should be.
//     // TODO: attempt transcode
// }
//
// void ktxValidator::validateDataSize(validationContext& ctx) {
//     // Expects to be called after validateSgd so current file offset is at
//     // the start of the data.
//     uint64_t dataSizeInFile;
//     off_t dataStart = (off_t) (ctx.inp->tellg());
//
//     ctx.inp->seekg(0, ios_base::end);
//     if (ctx.inp->fail())
//         addIssue(logger::eFatal, IOError.FileSeekEndFailure, strerror(errno));
//     off_t dataEnd = (off_t) (ctx.inp->tellg());
//     if (dataEnd < 0)
//         addIssue(logger::eFatal, IOError.FileTellFailure, strerror(errno));
//     dataSizeInFile = dataEnd - dataStart;
//     if (dataSizeInFile != ctx.dataSizeFromLevelIndex)
//         addIssue(logger::eError, FileError.IncorrectDataSize);
// }

// void ValidationContext::validateCreateAndTranscode() {
//     KTXTexture<ktxTexture2> texture{nullptr};
//     ktx_error_code_e result = createKTXTexture(KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, texture.pHandle());
//
//     if (result != KTX_SUCCESS)
//         fatal(ValidatorError::CreateFailure, ktxErrorString(result));
//
//     if (parsedColorModel == KHR_DF_MODEL_UASTC || parsedColorModel == KHR_DF_MODEL_ETC1S) {
//         ktx_error_code_e result = KTX_SUCCESS;
//
//         if (parsedColorModel == KHR_DF_MODEL_ETC1S)
//             result = ktxTexture2_TranscodeBasis(texture.handle(), KTX_TTF_ETC2_RGBA, 0);
//         else if (parsedColorModel == KHR_DF_MODEL_UASTC)
//             result = ktxTexture2_TranscodeBasis(texture.handle(), KTX_TTF_ASTC_4x4_RGBA, 0);
//
//         if (result != KTX_SUCCESS)
//             error(ValidatorError::TranscodeFailure, ktxErrorString(result));
//     }
// }

// =================================================================================================

} // namespace ktx

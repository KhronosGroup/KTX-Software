// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include "ktx.h"
#include "ktxint.h"
#include "texture2.h"
#include "vkformat_enum.h"
#include "platform_utils.h"

#include "astc-encoder/Source/astcenc.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

#include <fmt/os.h>
#include <fmt/ostream.h>
#include <fmt/printf.h>


template <typename T>
[[nodiscard]] constexpr inline T ceil_div(const T x, const T y) noexcept {
    assert(y != 0);
	return (x + y - 1) / y;
}

// C++20 - std::bit_cast
template <class To, class From>
[[nodiscard]] constexpr inline To bit_cast(const From& src) noexcept {
    static_assert(sizeof(To) == sizeof(From));
    static_assert(std::is_trivially_copyable_v<From>);
    static_assert(std::is_trivially_copyable_v<To>);
    static_assert(std::is_trivially_constructible_v<To>);
    To dst;
    std::memcpy(&dst, &src, sizeof(To));
    return dst;
}

[[nodiscard]] constexpr inline bool isFormatAstc(VkFormat format) noexcept {
    switch (format) {
    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_3x3x3_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_3x3x3_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_3x3x3_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x3x3_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x3x3_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x3x3_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x3_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x3_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x3_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x4_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x4_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x4_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x4x4_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x4x4_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x4x4_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x4_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x4_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x4_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x5_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x5_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x5_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x5x5_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x5x5_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x5x5_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x5_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x5_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x5_SFLOAT_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x6_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x6_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x6_SFLOAT_BLOCK_EXT:
        return true;
    default:
        return false;
    }
}

// -------------------------------------------------------------------------------------------------

int EXIT_CODE_ERROR = 2;
int EXIT_CODE_MISMATCH = 1;
int EXIT_CODE_MATCH = 0;

template <typename... Args>
void error(int return_code, Args&&... args) {
    fmt::print(std::cerr, std::forward<Args>(args)...);
    std::exit(return_code);
}

[[nodiscard]] inline std::string errnoMessage() {
    return std::make_error_code(static_cast<std::errc>(errno)).message();
}

struct Texture {
    std::string filepath;
    std::vector<std::byte> rawData;

    KTX_header2 header;
    std::vector<ktxLevelIndexEntry> levelIndices;
    const std::byte* levelIndexData = nullptr;
    size_t levelIndexSize = 0;
    const std::byte* dfdData = nullptr;
    size_t dfdSize = 0;
    const std::byte* kvdData = nullptr;
    size_t kvdSize = 0;
    const std::byte* sgdData = nullptr;
    size_t sgdSize = 0;

    ktxTexture2* handle = nullptr;
    bool transcoded = false;

public:
    explicit Texture(std::string filepath) :
        filepath(filepath) {
        std::memset(&header, 0, sizeof(header));

        loadFile();
        loadKTX();
        loadMetadata();
    }
    ~Texture() {
        std::free(handle);
    }
    void loadFile();
    void loadKTX();
    void loadMetadata();
    inline ktxTexture2* operator->() const {
        return handle;
    }
};

void Texture::loadFile() {
    auto file = std::ifstream(DecodeUTF8Path(filepath).c_str(), std::ios::binary | std::ios::in | std::ios::ate);
    if (!file)
        error(EXIT_CODE_ERROR, "ktxdiff error \"{}\": Failed to open file: {}\n", filepath, errnoMessage());

    const auto fileSize = file.tellg();
    file.seekg(0);
    if (file.fail())
        error(EXIT_CODE_ERROR, "ktxdiff error \"{}\": Failed to seek file: {}\n", filepath, errnoMessage());

    rawData.resize(fileSize);
    file.read(reinterpret_cast<char*>(rawData.data()), fileSize);
    if (file.fail())
        error(EXIT_CODE_ERROR, "ktxdiff error \"{}\": Failed to read file: {}\n", filepath, errnoMessage());
}

void Texture::loadKTX() {
    KTX_error_code ec = KTX_SUCCESS;
    ec = ktxTexture2_CreateFromMemory(
            reinterpret_cast<const ktx_uint8_t*>(rawData.data()),
            rawData.size(),
            KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
            &handle);
    if (ec != KTX_SUCCESS)
        error(EXIT_CODE_ERROR, "ktxdiff error \"{}\": ktxTexture2_CreateFromNamedFile: {}\n", filepath, ktxErrorString(ec));

    if (ktxTexture2_NeedsTranscoding(handle)) {
        ec = ktxTexture2_TranscodeBasis(handle, KTX_TTF_RGBA32, 0);
        if (ec != KTX_SUCCESS)
            error(EXIT_CODE_ERROR, "ktxdiff error \"{}\": ktxTexture2_TranscodeBasis: {}\n", filepath, ktxErrorString(ec));
        transcoded = true;
    }
}

void Texture::loadMetadata() {
    const auto headerData = rawData.data();
    const auto headerSize = sizeof(KTX_header2);
    std::memcpy(&header, headerData, headerSize);

    const auto numLevels = std::max(header.levelCount, 1u);
    levelIndexData = rawData.data() + sizeof(KTX_header2);
    levelIndexSize = sizeof(ktxLevelIndexEntry) * numLevels;
    levelIndices.resize(numLevels);
    std::memcpy(levelIndices.data(), levelIndexData, levelIndexSize);

    if (header.dataFormatDescriptor.byteLength != 0) {
        dfdData = rawData.data() + header.dataFormatDescriptor.byteOffset;
        dfdSize = header.dataFormatDescriptor.byteLength;
    }
    if (header.keyValueData.byteLength != 0) {
        kvdData = rawData.data() + header.keyValueData.byteOffset;
        kvdSize = header.keyValueData.byteLength;
    }
    if (header.supercompressionGlobalData.byteLength != 0) {
        sgdData = rawData.data() + header.dataFormatDescriptor.byteOffset;
        sgdSize = header.dataFormatDescriptor.byteLength;
    }
}

// -------------------------------------------------------------------------------------------------

struct CompareResult {
    bool match = true;
    float difference = 0.f;
    std::size_t elementIndex = 0;
    std::size_t byteOffset = 0;
};

CompareResult compareUnorm8(const char* rawLhs, const char* rawRhs, std::size_t rawSize, float tolerance) {
    const auto* lhs = reinterpret_cast<const uint8_t*>(rawLhs);
    const auto* rhs = reinterpret_cast<const uint8_t*>(rawRhs);
    const auto element_size = sizeof(uint8_t);
    const auto count = rawSize / element_size;

    for (std::size_t i = 0; i < count; ++i) {
        const auto diff = std::abs(static_cast<float>(lhs[i]) / 255.f - static_cast<float>(rhs[i]) / 255.f);
        if (diff > tolerance)
            return CompareResult{false, diff, i, i * element_size};
    }

    return CompareResult{};
}

CompareResult compareSFloat32(const char* rawLhs, const char* rawRhs, std::size_t rawSize, float tolerance) {
    const auto* lhs = reinterpret_cast<const float*>(rawLhs);
    const auto* rhs = reinterpret_cast<const float*>(rawRhs);
    const auto element_size = sizeof(float);
    const auto count = rawSize / element_size;

    for (std::size_t i = 0; i < count; ++i) {
        const auto diff = std::abs(lhs[i] - rhs[i]);
        if (diff > tolerance)
            return CompareResult{false, diff, i, i * element_size};
    }

    return CompareResult{};
}

auto decodeASTC(const char* compressedData, std::size_t compressedSize, uint32_t width, uint32_t height,
        const std::string& filepath, bool isFormatSRGB, uint32_t blockSizeX, uint32_t blockSizeY, uint32_t blockSizeZ) {

    const auto threadCount = 1u;
    static constexpr astcenc_swizzle swizzle{ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A};

    astcenc_error ec = ASTCENC_SUCCESS;

    const astcenc_profile profile = isFormatSRGB ? ASTCENC_PRF_LDR_SRGB : ASTCENC_PRF_LDR;
    astcenc_config config{};
    ec = astcenc_config_init(profile, blockSizeX, blockSizeY, blockSizeZ, ASTCENC_PRE_MEDIUM, ASTCENC_FLG_DECOMPRESS_ONLY, &config);
    if (ec != ASTCENC_SUCCESS)
        error(EXIT_CODE_ERROR, "ktxdiff error \"{}\": astcenc_config_init: {}\n", filepath, astcenc_get_error_string(ec));

    struct ASTCencStruct {
        astcenc_context* context = nullptr;
        ~ASTCencStruct() {
            astcenc_context_free(context);
        }
    } astcenc;
    astcenc_context*& context = astcenc.context;

    ec = astcenc_context_alloc(&config, threadCount, &context);
    if (ec != ASTCENC_SUCCESS)
        error(EXIT_CODE_ERROR, "ktxdiff error \"{}\": astcenc_context_alloc: {}\n", filepath, astcenc_get_error_string(ec));

    astcenc_image image{};
    image.dim_x = width;
    image.dim_y = height;
    image.dim_z = 1; // 3D ASTC formats are currently not supported
    const auto uncompressedSize = width * height * 4 * sizeof(uint8_t);
    auto uncompressedBuffer = std::make_unique<uint8_t[]>(uncompressedSize);
    auto* bufferPtr = uncompressedBuffer.get();
    image.data = reinterpret_cast<void**>(&bufferPtr);
    image.data_type = ASTCENC_TYPE_U8;

    ec = astcenc_decompress_image(context, reinterpret_cast<const uint8_t*>(compressedData), compressedSize, &image, &swizzle, 0);
    if (ec != ASTCENC_SUCCESS)
        error(EXIT_CODE_ERROR, "ktxdiff error \"{}\": astcenc_decompress_image: {}\n", filepath, astcenc_get_error_string(ec));

    astcenc_decompress_reset(context);

    struct Result {
        std::unique_ptr<uint8_t[]> data;
        std::size_t size;
    };
    return Result{std::move(uncompressedBuffer), uncompressedSize};
}

CompareResult compareAstc(const char* lhs, const char* rhs, std::size_t size, uint32_t width, uint32_t height,
        const std::string& filepathLhs, const std::string& filepathRhs,
        bool isFormatSRGB, uint32_t blockSizeX, uint32_t blockSizeY, uint32_t blockSizeZ,
        float tolerance) {
    const auto uncompressedLhs = decodeASTC(lhs, size, width, height, filepathLhs, isFormatSRGB, blockSizeX, blockSizeY, blockSizeZ);
    const auto uncompressedRhs = decodeASTC(rhs, size, width, height, filepathRhs, isFormatSRGB, blockSizeX, blockSizeY, blockSizeZ);

    return compareUnorm8(
            reinterpret_cast<const char*>(uncompressedLhs.data.get()),
            reinterpret_cast<const char*>(uncompressedRhs.data.get()),
            uncompressedLhs.size,
            tolerance);
}

bool compare(Texture& lhs, Texture& rhs, float tolerance) {
    const auto vkFormat = static_cast<VkFormat>(lhs.header.vkFormat);
    const auto* bdfd = reinterpret_cast<const uint32_t*>(lhs.dfdData) + 1;
    const auto componentCount = KHR_DFDSAMPLECOUNT(bdfd);
    const auto texelBlockDimension0 = static_cast<uint8_t>(KHR_DFDVAL(bdfd, TEXELBLOCKDIMENSION0));
    const auto texelBlockDimension1 = static_cast<uint8_t>(KHR_DFDVAL(bdfd, TEXELBLOCKDIMENSION1));
    const auto texelBlockDimension2 = static_cast<uint8_t>(KHR_DFDVAL(bdfd, TEXELBLOCKDIMENSION2));
    const auto blockSizeX = texelBlockDimension0 + 1u;
    const auto blockSizeY = texelBlockDimension1 + 1u;
    const auto blockSizeZ = texelBlockDimension2 + 1u;
    const bool isFormatSRGB = KHR_DFDVAL(bdfd, TRANSFER) == KHR_DF_TRANSFER_SRGB;

    const bool isSigned = (KHR_DFDSVAL(bdfd, 0, QUALIFIERS) & KHR_DF_SAMPLE_DATATYPE_SIGNED) != 0;
    const bool isFloat = (KHR_DFDSVAL(bdfd, 0, QUALIFIERS) & KHR_DF_SAMPLE_DATATYPE_FLOAT) != 0;
    const bool isNormalized = KHR_DFDSVAL(bdfd, 0, SAMPLEUPPER) == (isFloat ? bit_cast<uint32_t>(1.0f) : 1u);
    const bool is32Bit = KHR_DFDSVAL(bdfd, 0, BITLENGTH) + 1 == 32;
    const bool is8Bit = KHR_DFDSVAL(bdfd, 0, BITLENGTH) + 1 == 8;
    const bool isFormatSFloat32 = isSigned && isFloat && is32Bit && vkFormat != VK_FORMAT_D32_SFLOAT_S8_UINT;
    const bool isFormatUNORM8 = !isSigned && !isFloat && is8Bit && isNormalized;

    const auto mismatch = [&](auto&&... args) {
        fmt::print("ktxdiff: ");
        fmt::print(std::forward<decltype(args)>(args)...);
        fmt::print(" between\n");
        fmt::print("          Expected: {} and\n", lhs.filepath);
        fmt::print("          Received: {}\n", rhs.filepath);
        return false;
    };

    if (lhs.transcoded) {
        // For encoded images the compressed data sizes can differ.
        // Skip the related checks for header.supercompressionGlobalData and levelIndex
        if (std::memcmp(&lhs.header, &rhs.header, sizeof(lhs.header) - sizeof(ktxIndexEntry64)) != 0)
            return mismatch("Mismatching header");
    } else {
        if (std::memcmp(&lhs.header, &rhs.header, sizeof(lhs.header)) != 0)
            return mismatch("Mismatching header");
        if (lhs.levelIndexSize != rhs.levelIndexSize)
            return mismatch("Mismatching levelIndices");
        for (uint32_t i = 0; i < lhs.levelIndices.size(); ++i)
            // Offsets and (compressed) sizes can differ, but uncompressedByteLength must match
            if (lhs.levelIndices[i].uncompressedByteLength != rhs.levelIndices[i].uncompressedByteLength)
                return mismatch("Mismatching levelIndices[{}].uncompressedByteLength", i);
    }
    if (lhs.dfdSize != rhs.dfdSize || std::memcmp(lhs.dfdData, rhs.dfdData, lhs.dfdSize) != 0)
        return mismatch("Mismatching DFD");

    if (lhs.kvdSize != rhs.kvdSize || std::memcmp(lhs.kvdData, rhs.kvdData, lhs.kvdSize) != 0)
        return mismatch("Mismatching KVD");

    if (!lhs.transcoded)
        if (lhs.sgdSize != rhs.sgdSize || std::memcmp(lhs.sgdData, rhs.sgdData, lhs.sgdSize) != 0)
            return mismatch("Mismatching SGD");

    // If the tolerance is 1 or above accept every image data as matching
    if (tolerance >= 1.0f)
        return true;

    for (uint32_t levelIndex = 0; levelIndex < lhs->numLevels; ++levelIndex) {
        const auto imageSize = ktxTexture_GetImageSize(ktxTexture(lhs.handle), levelIndex);
        const auto imageWidth = std::max(1u, lhs->baseWidth >> levelIndex);
        const auto imageHeight = std::max(1u, lhs->baseHeight >> levelIndex);
        const auto imageDepth = std::max(1u, lhs->baseDepth >> levelIndex);

        for (uint32_t faceIndex = 0; faceIndex < lhs->numFaces; ++faceIndex) {
            for (uint32_t layerIndex = 0; layerIndex < lhs->numLayers; ++layerIndex) {
                for (uint32_t depthIndex = 0; depthIndex < ceil_div(imageDepth, blockSizeZ); ++depthIndex) {

                    ktx_size_t imageOffset;
                    ktxTexture2_GetImageOffset(lhs.handle, levelIndex, layerIndex, faceIndex + depthIndex, &imageOffset);
                    const char* imageDataLhs = reinterpret_cast<const char*>(lhs->pData) + imageOffset;
                    const char* imageDataRhs = reinterpret_cast<const char*>(rhs->pData) + imageOffset;

                    CompareResult result;
                    if (lhs.transcoded || isFormatUNORM8) {
                        result = compareUnorm8(imageDataLhs, imageDataRhs, imageSize, tolerance);
                    } else if (isFormatAstc(vkFormat)) {
                        result = compareAstc(imageDataLhs, imageDataRhs, imageSize, imageWidth, imageHeight,
                                lhs.filepath, rhs.filepath,
                                isFormatSRGB, blockSizeX, blockSizeY, blockSizeZ,
                                tolerance);
                    } else if (isFormatSFloat32) {
                        result = compareSFloat32(imageDataLhs, imageDataRhs, imageSize, tolerance);
                    } else {
                        for (std::size_t i = 0; i < imageSize; ++i) {
                            if (imageDataLhs[i] != imageDataRhs[i])
                                return mismatch("Mismatching image data: level {}, face {}, layer {}, depth {}, image byte {}",
                                        levelIndex, faceIndex, layerIndex, depthIndex, i);
                        }
                    }

                    if (!result.match) {
                        return mismatch("Mismatching image data (diff: {}): level {}, face {}, layer {}, depth {}, pixel {}, component {}",
                                result.difference, levelIndex, faceIndex, layerIndex, depthIndex,
                                result.elementIndex / componentCount, result.elementIndex % componentCount);
                    }
                }
            }
        }
    }

    return true;
}

/// EXIT CODES:
///     0 - Matching files
///     1 - Mismatching files
///     2 - Error while loading, decoding or processing an input file
int main(int argc, char* argv[]) {
    InitUTF8CLI(argc, argv);

    if (argc < 3) {
        fmt::print("Missing input file arguments\n");
        fmt::print("Usage: ktxdiff <expected-ktx2> <received-ktx2> [tolerance]\n");
        return EXIT_FAILURE;
    }

    const float tolerance = argc > 3 ? std::stof(argv[3]) : 0.05f;

    Texture lhs(argv[1]);
    Texture rhs(argv[2]);
    const auto match = compare(lhs, rhs, tolerance);

    return match ? 0 : 1;
}

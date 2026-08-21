// Copyright 2022-2026 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include "ktx.h"
#include "ktxint.h"
#include "vkformat_enum.h"
#include "platform_utils.h"
#include "imageio_utility.h"

#include "astc-encoder/Source/astcenc.h"

#include <cassert>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <vector>

#include <fmt/os.h>
#include <fmt/ostream.h>
#include <fmt/printf.h>
#include <filesystem>

#define CXXOPTS_NO_EXCEPTIONS
#include <cxxopts.hpp>

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

// -------------------------------------------------------------------------------------------------

int EXIT_CODE_ERROR = 2;
int EXIT_CODE_MISMATCH = 1;
int EXIT_CODE_MATCH = 0;

template <typename... Args>
void error(int return_code, fmt::format_string<Args...> fmt, Args&&... args) {
    fmt::print(std::cerr, fmt, std::forward<Args>(args)...);
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

    std::unique_ptr<ktxTexture2, decltype(ktxTexture2_Destroy)*> handle{nullptr, ktxTexture2_Destroy};
    bool transcoded = false;

public:
    explicit Texture(std::string filepath) :
        filepath(filepath) {
        std::memset(&header, 0, sizeof(header));

        loadFile();
        loadKTX();
        loadMetadata();
    }
    void loadFile();
    void loadKTX();
    void loadMetadata();
    inline ktxTexture2* operator->() const {
        return handle.get();
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
    ktxTexture2* pTexture = nullptr;
    ec = ktxTexture2_CreateFromMemory(
            reinterpret_cast<const ktx_uint8_t*>(rawData.data()),
            rawData.size(),
            KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
            &pTexture);
    handle.reset(pTexture);
    if (ec != KTX_SUCCESS)
        error(EXIT_CODE_ERROR, "ktxdiff error \"{}\": ktxTexture2_CreateFromNamedFile: {}\n", filepath, ktxErrorString(ec));

    if (ktxTexture2_IsTranscodable(handle.get())) {
        ktx_transcode_fmt_e outputFmt = ktxTexture_IsHDR(ktxTexture(handle.get())) ?
                                        KTX_TTF_RGBA_HALF : KTX_TTF_RGBA32;
        ec = ktxTexture2_TranscodeBasis(handle.get(), outputFmt, 0);
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
    float lhsElementValue = 0.f;
    float rhsElementValue = 0.f;
};

CompareResult compareUnorm8(const char* rawLhs, const char* rawRhs, std::size_t rawSize, float tolerance) {
    const auto* lhs = reinterpret_cast<const uint8_t*>(rawLhs);
    const auto* rhs = reinterpret_cast<const uint8_t*>(rawRhs);
    const auto element_size = sizeof(uint8_t);
    const auto count = rawSize / element_size;

    for (std::size_t i = 0; i < count; ++i) {
        const auto lhsFloat = static_cast<float>(lhs[i]) / 255.f;
        const auto rhsFloat = static_cast<float>(rhs[i]) / 255.f;
        const auto diff = std::abs(lhsFloat - rhsFloat);
        if (diff > tolerance)
            return CompareResult{false, diff, i, i * element_size, lhsFloat, rhsFloat};
    }

    return CompareResult{};
}

CompareResult compareUnorm16(const char* rawLhs, const char* rawRhs, std::size_t rawSize,
                            float tolerance) {
    const auto* lhs = reinterpret_cast<const uint16_t*>(rawLhs);
    const auto* rhs = reinterpret_cast<const uint16_t*>(rawRhs);
    const auto element_size = sizeof(uint16_t);
    const auto count = rawSize / element_size;

    for (std::size_t i = 0; i < count; ++i) {
        const auto lhsFloat = static_cast<float>(lhs[i]) / 65535.f;
        const auto rhsFloat = static_cast<float>(rhs[i]) / 65535.f;
        const auto diff = std::abs(lhsFloat - rhsFloat);
        if (diff > tolerance)
            return CompareResult{false, diff, i, i * element_size, lhsFloat, rhsFloat};
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
        const auto absMin = std::min(std::abs(lhs[1]), std::abs(rhs[1]));
        if (diff > tolerance * absMin)
            return CompareResult{false, diff, i, i * element_size, lhs[i], rhs[i]};
    }

    return CompareResult{};
}

CompareResult compareSFloat16(const char* rawLhs, const char* rawRhs, std::size_t rawSize, float tolerance) {
    const auto* lhs = reinterpret_cast<const uint16_t*>(rawLhs);
    const auto* rhs = reinterpret_cast<const uint16_t*>(rawRhs);
    const auto element_size = sizeof(uint16_t);
    const auto count = rawSize / element_size;
    const auto baseline = (std::numeric_limits<float>::epsilon() * 100000) * tolerance;

    for (std::size_t i = 0; i < count; ++i) {
        const auto lhsFloat = imageio::half_to_float(lhs[i]);
        const auto rhsFloat = imageio::half_to_float(rhs[i]);
        const auto diff = std::abs(lhsFloat - rhsFloat);
        const auto absMin = std::min(std::abs(lhsFloat), std::abs(rhsFloat));
        // Some encoders don't encode 0 values as 0 but rather as a very small number (e.g., BC6HU encodes 0 into circa 1e-06).
        // This is why a baseline is introduced so the difference doesn't have to be extremely small for 0 values or
        // for very small values.
        if (diff > std::max(tolerance * absMin, baseline))
            return CompareResult{false, diff, i, i * element_size, lhsFloat, rhsFloat};
    }

    return CompareResult{};
}

auto decodeASTC(const char* compressedData, std::size_t compressedSize, uint32_t width, uint32_t height,
        const std::string& filepath, bool isFloat, bool isFormatSRGB,
        uint32_t blockSizeX, uint32_t blockSizeY, uint32_t blockSizeZ) {

    const auto threadCount = 1u;
    static constexpr astcenc_swizzle swizzle{ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A};

    astcenc_error ec = ASTCENC_SUCCESS;

    const astcenc_profile profile = isFloat ? ASTCENC_PRF_HDR : isFormatSRGB ? ASTCENC_PRF_LDR_SRGB : ASTCENC_PRF_LDR;
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

    ec = astcenc_context_alloc(&config, threadCount, &context, nullptr);
    if (ec != ASTCENC_SUCCESS)
        error(EXIT_CODE_ERROR, "ktxdiff error \"{}\": astcenc_context_alloc: {}\n", filepath, astcenc_get_error_string(ec));

    astcenc_image image{};
    image.dim_x = width;
    image.dim_y = height;
    image.dim_z = 1; // 3D ASTC formats are currently not supported
    const auto uncompressedSize = width * height * 4 * (isFloat ? sizeof(uint16_t) : sizeof(uint8_t));
    auto uncompressedBuffer = std::make_unique<uint8_t[]>(uncompressedSize);
    auto* bufferPtr = uncompressedBuffer.get();
    image.data = reinterpret_cast<void**>(&bufferPtr);
    // F16 is also the target transcode format for HDR transcodable textures.
    image.data_type = isFloat ? ASTCENC_TYPE_F16 : ASTCENC_TYPE_U8;

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
        bool isFloat, bool isFormatSRGB, uint32_t blockSizeX, uint32_t blockSizeY, uint32_t blockSizeZ,
        float tolerance) {
    const auto uncompressedLhs = decodeASTC(lhs, size, width, height, filepathLhs, isFloat, isFormatSRGB, blockSizeX, blockSizeY, blockSizeZ);
    const auto uncompressedRhs = decodeASTC(rhs, size, width, height, filepathRhs, isFloat, isFormatSRGB, blockSizeX, blockSizeY, blockSizeZ);

    if (isFloat) {
        return compareSFloat16(
                reinterpret_cast<const char*>(uncompressedLhs.data.get()),
                reinterpret_cast<const char*>(uncompressedRhs.data.get()),
                uncompressedLhs.size,
                tolerance);
    } else {
        return compareUnorm8(
                reinterpret_cast<const char*>(uncompressedLhs.data.get()),
                reinterpret_cast<const char*>(uncompressedRhs.data.get()),
                uncompressedLhs.size,
                tolerance);
    }
}

bool compare(Texture& lhs, Texture& rhs, float tolerance, bool skip_kvd) {
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
    const bool isFormatAstc = KHR_DFDVAL(bdfd, MODEL) == KHR_DF_MODEL_ASTC;

    const bool isSigned = (KHR_DFDSVAL(bdfd, 0, QUALIFIERS) & KHR_DF_SAMPLE_DATATYPE_SIGNED) != 0;
    const bool isFloat = (KHR_DFDSVAL(bdfd, 0, QUALIFIERS) & KHR_DF_SAMPLE_DATATYPE_FLOAT) != 0;
    const bool isNormalized = KHR_DFDSVAL(bdfd, 0, SAMPLEUPPER) != (isFloat ? bit_cast<uint32_t>(1.0f) : 1u);
    const bool is32Bit = KHR_DFDSVAL(bdfd, 0, BITLENGTH) + 1 == 32;
    const bool is16Bit = KHR_DFDSVAL(bdfd, 0, BITLENGTH) + 1 == 16;
    const bool is8Bit = KHR_DFDSVAL(bdfd, 0, BITLENGTH) + 1 == 8;
    const bool isFormatSFloat32 = isSigned && isFloat && is32Bit && vkFormat != VK_FORMAT_D32_SFLOAT_S8_UINT;
    const bool isFormatSFloat16 = isSigned && isFloat && is16Bit;
    const bool isFormatUNORM8 = !isSigned && !isFloat && is8Bit && isNormalized;
    const bool isFormatUNORM16 = !isSigned && !isFloat && is16Bit && isNormalized;

    const auto mismatch = [&](const auto& fmt, auto&&... args) {
        fmt::print("ktxdiff: ");
        fmt::print(fmt::runtime(fmt), std::forward<decltype(args)>(args)...);
        fmt::print(" between\n");
        fmt::print("          Expected: {} and\n", lhs.filepath);
        fmt::print("          Received: {}\n", rhs.filepath);
        return false;
    };

    if (lhs.transcoded) {
        // For encoded images the compressed data sizes can differ.
        // Skip the related checks for header.supercompressionGlobalData and levelIndex
        if (std::memcmp(&lhs.header, &rhs.header,
                        sizeof(lhs.header) -
                            (sizeof(ktxIndexEntry64) + (skip_kvd ? sizeof(ktxIndexEntry32) : 0))) != 0)
            return mismatch("Mismatching header");
    } else {
        if (skip_kvd) {
            // First compare up-to keyValueData member exclusive
            if (std::memcmp(
                    &lhs.header, &rhs.header,
                    sizeof(lhs.header) - (sizeof(ktxIndexEntry64) + sizeof(ktxIndexEntry32))) != 0)
                return mismatch("Mismatching header");
            // Then only compare supercompressionGlobalData
            if (std::memcmp(&lhs.header.supercompressionGlobalData, &rhs.header.supercompressionGlobalData, sizeof(lhs.header.supercompressionGlobalData)) != 0)
                return mismatch("Mismatching header");
        } else {
            if (std::memcmp(&lhs.header, &rhs.header, sizeof(lhs.header)) != 0)
                return mismatch("Mismatching header");
        }
        if (lhs.levelIndexSize != rhs.levelIndexSize)
            return mismatch("Mismatching levelIndices");
        for (uint32_t i = 0; i < lhs.levelIndices.size(); ++i)
            // Offsets and (compressed) sizes can differ, but uncompressedByteLength must match
            if (lhs.levelIndices[i].uncompressedByteLength != rhs.levelIndices[i].uncompressedByteLength)
                return mismatch("Mismatching levelIndices[{}].uncompressedByteLength", i);
    }
    if (lhs.dfdSize != rhs.dfdSize || std::memcmp(lhs.dfdData, rhs.dfdData, lhs.dfdSize) != 0)
        return mismatch("Mismatching DFD");

    if (!skip_kvd)
        if (lhs.kvdSize != rhs.kvdSize || std::memcmp(lhs.kvdData, rhs.kvdData, lhs.kvdSize) != 0)
            return mismatch("Mismatching KVD");

    if (!lhs.transcoded)
        if (lhs.sgdSize != rhs.sgdSize || std::memcmp(lhs.sgdData, rhs.sgdData, lhs.sgdSize) != 0)
            return mismatch("Mismatching SGD");

    // If the tolerance is 1 or above and data is normalized, accept every image data as matching
    if (isNormalized && tolerance >= 1.0f)
        return true;

    for (uint32_t levelIndex = 0; levelIndex < lhs->numLevels; ++levelIndex) {
        const auto imageSize = ktxTexture_GetImageSize(ktxTexture(lhs.handle.get()), levelIndex);
        const auto imageWidth = std::max(1u, lhs->baseWidth >> levelIndex);
        const auto imageHeight = std::max(1u, lhs->baseHeight >> levelIndex);
        const auto imageDepth = std::max(1u, lhs->baseDepth >> levelIndex);

        for (uint32_t faceIndex = 0; faceIndex < lhs->numFaces; ++faceIndex) {
            for (uint32_t layerIndex = 0; layerIndex < lhs->numLayers; ++layerIndex) {
                for (uint32_t depthIndex = 0; depthIndex < ceil_div(imageDepth, blockSizeZ); ++depthIndex) {

                    ktx_size_t imageOffset;
                    ktxTexture2_GetImageOffset(lhs.handle.get(), levelIndex, layerIndex, faceIndex + depthIndex, &imageOffset);
                    const char* imageDataLhs = reinterpret_cast<const char*>(lhs->pData) + imageOffset;
                    const char* imageDataRhs = reinterpret_cast<const char*>(rhs->pData) + imageOffset;

                    CompareResult result;
                    if ((lhs.transcoded && !isFloat) || isFormatUNORM8) {
                        result = compareUnorm8(imageDataLhs, imageDataRhs, imageSize, tolerance);
                    } else if ((lhs.transcoded && isFloat) || isFormatSFloat16) {
                        result = compareSFloat16(imageDataLhs, imageDataRhs, imageSize, tolerance);
                    } else if (isFormatUNORM16) {
                        result = compareUnorm16(imageDataLhs, imageDataRhs, imageSize, tolerance);
                    } else if (isFormatAstc) {
                        result = compareAstc(imageDataLhs, imageDataRhs, imageSize, imageWidth,
                                             imageHeight, lhs.filepath, rhs.filepath, isFloat,
                                             isFormatSRGB, blockSizeX, blockSizeY, blockSizeZ,
                                             tolerance);
                    } else if (isFormatSFloat32) {
                        result = compareSFloat32(imageDataLhs, imageDataRhs, imageSize, tolerance);
                    } else {
                        for (std::size_t i = 0; i < imageSize; ++i) {
                            if (imageDataLhs[i] != imageDataRhs[i])
                                return mismatch("Mismatching image data (lhs[{}]={} != rhs[{}]={}): level {}, face {}, layer {}, depth {}, image byte {}",
                                        i, imageDataLhs[i], i, imageDataRhs[i], levelIndex, faceIndex, layerIndex, depthIndex, i);
                        }
                    }

                    if (!result.match) {
                        return mismatch("Mismatching image data (diff: {}; lhs[{}]={}; rhs[{}]={}): level {}, face {}, layer {}, depth {}, pixel {}, component {}",
                                result.difference, result.elementIndex, result.lhsElementValue, result.elementIndex, result.rhsElementValue, levelIndex, faceIndex, layerIndex, depthIndex,
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
///     3 - Missing arguments, incorrect options, and other CLI errors.
int main(int argc, char* argv[]) {
    namespace fs = std::filesystem;

    float tolerance = 0.05f;
    bool skip_kvd = false;

    cxxopts::Options opts("ktxdiff", "diff two KTX2 files");
    opts.add_options()("expected-ktx2", "Expected KTX2 file", cxxopts::value<std::string>())(
        "received-ktx2", "Received KTX2 file", cxxopts::value<std::string>())(
        "tolerance",
        "For normalized formats tolerance is the normalized absolute value of the acceptable "
        "difference (inclusive). For unnormalized formats it is the fraction of the minimum of the "
        "values being compared that is acceptable. Default is 0.05",
        cxxopts::value<float>())("skip-kvd", "Ignore key-value metadata (KVD)")(
        "help,h", "Show this help message and exit");
    opts.parse_positional({"expected-ktx2", "received-ktx2", "tolerance"});
    opts.positional_help("<expected-ktx2> <received-ktx2> [tolerance]");
    opts.show_positional_help();

    auto result = opts.parse(argc, argv);
    if (result.count("help")) {
        fmt::print(opts.help());
        std::exit(0);
    }

    // Mandatory arguments
    if (!result.count("expected-ktx2")) {
        fmt::println(stderr,
                     "Missing input (expected) KTX2 file. <expected-ktx2> must be specified.");
        fmt::println(stderr, opts.help());
        std::exit(3);
    }
    if (!result.count("received-ktx2")) {
        fmt::println(stderr,
                     "Missing input (received) KTX2 file. <received-ktx2> must be specified.");
        fmt::println(stderr, opts.help());
        std::exit(3);
    }

    // Parse options
    if (result.count("tolerance")) tolerance = result["tolerance"].as<float>();
    if (result.count("skip-kvd")) skip_kvd = true;

    InitUTF8CLI(argc, argv);

    auto lhs_path = result["expected-ktx2"].as<std::string>();
    auto rhs_path = result["received-ktx2"].as<std::string>();

    // Make sure provided paths are paths to regular files (i.e., not directories) otherwise we get
    // all sort of issues (e.g., bad_alloc if a directory is supplied)
    if ((fs::status(lhs_path)).type() != fs::file_type::regular) {
        fmt::println(
            stderr,
            "Profided expected-ktx2 filepath \"{}\" either does not exist or is not a regular file.",
            lhs_path);
        std::exit(3);
    }
    if ((fs::status(rhs_path)).type() != fs::file_type::regular) {
        fmt::println(
            stderr,
            "Profided received-ktx2 filepath \"{}\" either does not exist or is not a regular file.",
            lhs_path);
        std::exit(3);
    }

    Texture lhs(lhs_path);
    Texture rhs(rhs_path);
    const auto match = compare(lhs, rhs, tolerance, skip_kvd);

    return match ? 0 : 1;
}

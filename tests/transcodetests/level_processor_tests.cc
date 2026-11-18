// Copyright 2026 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

// ktxLevelProcessor coverage: creation validation, resolved output format
// and target-layout queries, verified against the actual output layout of
// ktxTexture2_TranscodeBasis on the same source; ProcessLevel argument
// validation; and ProcessLevel output verified level by level against
// ktxTexture2_TranscodeBasis across source families (BasisLZ/ETC1S, UASTC
// LDR raw/Zstd/ZLIB, UASTC HDR 4x4, UASTC HDR 6x6 intermediate), texture
// shapes and targets, with levels processed out of order and reprocessed.
// All sources are generated in-process.

#include <string.h>
#include "gl_format.h"
#include "ktx.h"
#include "ktxint.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <vector>

namespace {

using TextureRaii = std::unique_ptr<ktxTexture2, void (*)(ktxTexture2*)>;

TextureRaii makeRaii(ktxTexture2* texture) {
    return TextureRaii(texture,
                       [](ktxTexture2* t) { ktxTexture_Destroy(ktxTexture(t)); });
}

using ProcessorRaii =
    std::unique_ptr<ktxLevelProcessor, void (*)(ktxLevelProcessor*)>;

ProcessorRaii makeRaii(ktxLevelProcessor* processor) {
    return ProcessorRaii(processor, ktxLevelProcessor_Destroy);
}

enum class Etc1sLayout { Array, Video, NonArray };

// 2D array with a full mip chain, two-frame video with one level, or 2D
// texture that is not an array with a full mip chain. Encoded to ETC1S so
// SGD handling is included.
std::vector<ktx_uint8_t> encodeEtc1s(Etc1sLayout layout) {
    const bool video = layout == Etc1sLayout::Video;
    const bool isArray = layout != Etc1sLayout::NonArray;
    ktxTextureCreateInfo createInfo = {};
    createInfo.vkFormat = 43;  // VK_FORMAT_R8G8B8A8_SRGB
    createInfo.baseWidth = 32;
    createInfo.baseHeight = 32;
    createInfo.baseDepth = 1;
    createInfo.numDimensions = 2;
    createInfo.numLevels = video ? 1 : 6;
    createInfo.numLayers = isArray ? 2 : 1;
    createInfo.numFaces = 1;
    createInfo.isArray = isArray ? KTX_TRUE : KTX_FALSE;
    createInfo.generateMipmaps = KTX_FALSE;

    ktxTexture2* texture = nullptr;
    KTX_error_code result = ktxTexture2_Create(&createInfo,
                                               KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                               &texture);
    EXPECT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    if (result != KTX_SUCCESS)
        return {};
    TextureRaii texture_raii = makeRaii(texture);

    if (video) {
        texture->isVideo = KTX_TRUE;
        const ktx_uint32_t animData[] = {1, 30, 0};
        result = ktxHashList_AddKVPair(&texture->kvDataHead, KTX_ANIMDATA_KEY,
                                      sizeof(animData), animData);
        EXPECT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
        if (result != KTX_SUCCESS)
            return {};
    }

    for (ktx_uint32_t level = 0; level < createInfo.numLevels; level++) {
        const ktx_uint32_t width = std::max(1u, createInfo.baseWidth >> level);
        const ktx_uint32_t height = std::max(1u, createInfo.baseHeight >> level);
        for (ktx_uint32_t layer = 0; layer < createInfo.numLayers; layer++) {
            std::vector<ktx_uint8_t> pixels((size_t)width * height * 4);
            for (size_t i = 0; i < pixels.size(); i += 4) {
                pixels[i + 0] = (ktx_uint8_t)(i * 7 + level * 31 + layer * 101);
                pixels[i + 1] = (ktx_uint8_t)(i * 13 + level * 17);
                pixels[i + 2] = (ktx_uint8_t)(i * 3 + layer * 53);
                pixels[i + 3] = 255;
            }
            result = ktxTexture_SetImageFromMemory(ktxTexture(texture), level,
                                                   layer, 0, pixels.data(),
                                                   pixels.size());
            EXPECT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
        }
    }

    ktxBasisParams cparams = {};
    cparams.structSize = sizeof(cparams);
    cparams.threadCount = 1;
    cparams.codec = KTX_BASIS_CODEC_ETC1S;
    cparams.etc1sCompressionLevel = KTX_ETC1S_DEFAULT_COMPRESSION_LEVEL;
    cparams.qualityLevel = 128;
    result = ktxTexture2_CompressBasisEx(texture, &cparams);
    EXPECT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);

    ktx_uint8_t* bytes = nullptr;
    ktx_size_t size = 0;
    result = ktxTexture_WriteToMemory(ktxTexture(texture), &bytes, &size);
    EXPECT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    std::vector<ktx_uint8_t> file(bytes, bytes + size);
    free(bytes);
    return file;
}

const std::vector<ktx_uint8_t>& etc1sArrayFile() {
    static const std::vector<ktx_uint8_t> file =
        encodeEtc1s(Etc1sLayout::Array);
    return file;
}

const std::vector<ktx_uint8_t>& etc1sVideoFile() {
    static const std::vector<ktx_uint8_t> file =
        encodeEtc1s(Etc1sLayout::Video);
    return file;
}

const std::vector<ktx_uint8_t>& etc1sNonArrayFile() {
    static const std::vector<ktx_uint8_t> file =
        encodeEtc1s(Etc1sLayout::NonArray);
    return file;
}

class LevelProcessorKvd
    : public ::testing::TestWithParam<ktxTextureCreateFlags> {};

TEST_P(LevelProcessorKvd, RejectsVideoWithEveryKvdMode) {
    const std::vector<ktx_uint8_t>& file = etc1sVideoFile();
    ASSERT_FALSE(file.empty());

    const ktxTextureCreateFlags flags = GetParam();
    ktxTexture2* source = nullptr;
    ASSERT_EQ(ktxTexture2_CreateFromMemory(file.data(), file.size(), flags,
                                          &source),
              KTX_SUCCESS);
    TextureRaii source_raii = makeRaii(source);
    // Only parsed KVD initializes isVideo; the other modes cannot establish
    // whether this source requires inter-frame transcoder state.
    EXPECT_EQ(source->isVideo, flags == 0 ? KTX_TRUE : KTX_FALSE);

    ktxLevelProcessor* processor = nullptr;
    EXPECT_EQ(ktxLevelProcessor_CreateBasis(source, KTX_TTF_BC7_RGBA, 0,
                                           &processor),
              flags == 0 ? KTX_UNSUPPORTED_FEATURE : KTX_INVALID_OPERATION);
    ProcessorRaii processor_raii = makeRaii(processor);
    EXPECT_EQ(processor, nullptr);
}

TEST_P(LevelProcessorKvd, RequiresParsedNonemptyKvd) {
    const std::vector<ktx_uint8_t>& file = etc1sArrayFile();
    ASSERT_FALSE(file.empty());

    const ktxTextureCreateFlags flags = GetParam();
    ktxTexture2* source = nullptr;
    ASSERT_EQ(ktxTexture2_CreateFromMemory(file.data(), file.size(), flags,
                                          &source),
              KTX_SUCCESS);
    TextureRaii source_raii = makeRaii(source);
    EXPECT_FALSE(source->isVideo);

    ktxLevelProcessor* processor = nullptr;
    EXPECT_EQ(ktxLevelProcessor_CreateBasis(source, KTX_TTF_BC7_RGBA, 0,
                                           &processor),
              flags == 0 ? KTX_SUCCESS : KTX_INVALID_OPERATION);
    ProcessorRaii processor_raii = makeRaii(processor);
    EXPECT_EQ(processor != nullptr, flags == 0);
}

// The writer adds KTXwriter metadata automatically. Remove the KVD and its
// padding, then relocate the SGD and level offsets to make a metadata-free
// version of the same encoded array.
void removeKvd(std::vector<ktx_uint8_t>& file) {
    KTX_header2 header;
    ASSERT_GE(file.size(), sizeof(header));
    std::memcpy(&header, file.data(), sizeof(header));
    const size_t kvdOffset = header.keyValueData.byteOffset;
    const size_t oldSgdOffset = header.supercompressionGlobalData.byteOffset;
    const size_t newSgdOffset = (kvdOffset + 7) & ~size_t(7);
    ASSERT_GT(header.keyValueData.byteLength, 0u);
    ASSERT_GE(kvdOffset, sizeof(header)
                           + header.levelCount * sizeof(ktxLevelIndexEntry));
    ASSERT_GE(oldSgdOffset, newSgdOffset);
    ASSERT_LE(oldSgdOffset, file.size());
    const size_t removed = oldSgdOffset - newSgdOffset;
    file.erase(file.begin() + newSgdOffset, file.begin() + oldSgdOffset);
    std::fill(file.begin() + kvdOffset, file.begin() + newSgdOffset, 0);
    header.keyValueData = {};
    header.supercompressionGlobalData.byteOffset = newSgdOffset;
    std::memcpy(file.data(), &header, sizeof(header));
    for (ktx_uint32_t level = 0; level < header.levelCount; ++level) {
        const size_t indexOffset = sizeof(header)
                                 + level * sizeof(ktxLevelIndexEntry);
        ktxLevelIndexEntry entry;
        std::memcpy(&entry, file.data() + indexOffset, sizeof(entry));
        ASSERT_GE(entry.byteOffset, oldSgdOffset);
        entry.byteOffset -= removed;
        std::memcpy(file.data() + indexOffset, &entry, sizeof(entry));
    }
}

TEST_P(LevelProcessorKvd, AcceptsSourceWithoutKvd) {
    std::vector<ktx_uint8_t> file = etc1sArrayFile();
    ASSERT_FALSE(file.empty());
    ASSERT_NO_FATAL_FAILURE(removeKvd(file));

    ktxTexture2* source = nullptr;
    ASSERT_EQ(ktxTexture2_CreateFromMemory(file.data(), file.size(), GetParam(),
                                          &source),
              KTX_SUCCESS);
    TextureRaii source_raii = makeRaii(source);
    EXPECT_FALSE(source->isVideo);

    ktxLevelProcessor* processor = nullptr;
    ASSERT_EQ(ktxLevelProcessor_CreateBasis(source, KTX_TTF_BC7_RGBA, 0,
                                           &processor),
              KTX_SUCCESS);
    ProcessorRaii processor_raii = makeRaii(processor);
    EXPECT_NE(processor, nullptr);

    ktxTexture2* reference = nullptr;
    ASSERT_EQ(ktxTexture2_CreateFromMemory(
                  file.data(), file.size(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                  &reference),
              KTX_SUCCESS);
    TextureRaii reference_raii = makeRaii(reference);
    EXPECT_EQ(ktxTexture2_TranscodeBasis(reference, KTX_TTF_BC7_RGBA, 0),
              KTX_SUCCESS);
}

// Only an array can be a video, so whether a source that is not an array
// is a video is known whatever the KVD mode. Such a source written by
// libktx, which always adds KTXwriter, is accepted in every mode.
TEST_P(LevelProcessorKvd, AcceptsNonArraySourceWithEveryKvdMode) {
    const std::vector<ktx_uint8_t>& file = etc1sNonArrayFile();
    ASSERT_FALSE(file.empty());
    KTX_header2 header;
    ASSERT_GE(file.size(), sizeof(header));
    std::memcpy(&header, file.data(), sizeof(header));
    ASSERT_GT(header.keyValueData.byteLength, 0u);

    ktxTexture2* source = nullptr;
    ASSERT_EQ(ktxTexture2_CreateFromMemory(file.data(), file.size(), GetParam(),
                                          &source),
              KTX_SUCCESS);
    TextureRaii source_raii = makeRaii(source);
    ASSERT_FALSE(source->isArray);
    EXPECT_FALSE(source->isVideo);

    ktxLevelProcessor* processor = nullptr;
    ASSERT_EQ(ktxLevelProcessor_CreateBasis(source, KTX_TTF_BC7_RGBA, 0,
                                           &processor),
              KTX_SUCCESS);
    ProcessorRaii processor_raii = makeRaii(processor);
    ASSERT_NE(processor, nullptr);

    // The layout queries answer as for any other source.
    ktxTexture2* reference = nullptr;
    ASSERT_EQ(ktxTexture2_CreateFromMemory(
                  file.data(), file.size(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                  &reference),
              KTX_SUCCESS);
    TextureRaii reference_raii = makeRaii(reference);
    ASSERT_EQ(ktxTexture2_TranscodeBasis(reference, KTX_TTF_BC7_RGBA, 0),
              KTX_SUCCESS);
    EXPECT_EQ(ktxLevelProcessor_GetOutputVkFormat(processor),
              reference->vkFormat);
    for (ktx_uint32_t level = 0; level < source->numLevels; level++) {
        SCOPED_TRACE(level);
        ktx_size_t size = 0;
        ASSERT_EQ(ktxLevelProcessor_GetLevelSize(processor, level, &size),
                  KTX_SUCCESS);
        EXPECT_EQ(size, ktxTexture2_GetImageSize(reference, level));
        ASSERT_EQ(ktxLevelProcessor_GetImageSize(processor, level, &size),
                  KTX_SUCCESS);
        EXPECT_EQ(size, ktxTexture2_GetImageSize(reference, level));
        ktx_size_t offset = 1;
        ASSERT_EQ(ktxLevelProcessor_GetImageOffset(processor, level, 0, 0,
                                                   &offset),
                  KTX_SUCCESS);
        EXPECT_EQ(offset, 0u);
    }
}

INSTANTIATE_TEST_SUITE_P(
    KvdModes, LevelProcessorKvd,
    ::testing::Values(0u, KTX_TEXTURE_CREATE_SKIP_KVDATA_BIT,
                      KTX_TEXTURE_CREATE_RAW_KVDATA_BIT,
                      KTX_TEXTURE_CREATE_SKIP_KVDATA_BIT
                        | KTX_TEXTURE_CREATE_RAW_KVDATA_BIT));

TEST(LevelProcessor, LayoutQueriesMatchTranscodeBasisOutput) {
    const std::vector<ktx_uint8_t>& file = etc1sArrayFile();
    ASSERT_FALSE(file.empty());

    // Source for the processor: constructed without loading image data,
    // as a streaming consumer would.
    ktxTexture2* source = nullptr;
    KTX_error_code result =
        ktxTexture2_CreateFromMemory(file.data(), file.size(), 0, &source);
    ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    TextureRaii source_raii = makeRaii(source);

    ktxLevelProcessor* processor = nullptr;
    result = ktxLevelProcessor_CreateBasis(source, KTX_TTF_BC7_RGBA, 0,
                                           &processor);
    ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    ProcessorRaii processor_raii = makeRaii(processor);

    // Reference: the same file fully loaded and transcoded whole.
    ktxTexture2* reference = nullptr;
    result = ktxTexture2_CreateFromMemory(
        file.data(), file.size(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        &reference);
    ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    TextureRaii reference_raii = makeRaii(reference);
    result = ktxTexture2_TranscodeBasis(reference, KTX_TTF_BC7_RGBA, 0);
    ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);

    // The source is sRGB, so BC7 must resolve to the SRGB variant, same
    // as the whole-texture transcode.
    EXPECT_EQ(ktxLevelProcessor_GetOutputVkFormat(processor),
              reference->vkFormat);

    for (ktx_uint32_t level = 0; level < source->numLevels; level++) {
        ktx_size_t imageSize = 0;
        ASSERT_EQ(ktxLevelProcessor_GetImageSize(processor, level, &imageSize),
                  KTX_SUCCESS);
        EXPECT_EQ(imageSize, ktxTexture2_GetImageSize(reference, level))
            << "level " << level;

        // A complete level holds all layers; the first image starts at 0
        // and the second layer's image starts one image size in
        // (block-compressed images have no intra-level padding).
        ktx_size_t levelSize = 0;
        ASSERT_EQ(ktxLevelProcessor_GetLevelSize(processor, level, &levelSize),
                  KTX_SUCCESS);
        EXPECT_EQ(levelSize, imageSize * source->numLayers) << "level " << level;

        ktx_size_t offset0 = 1, offset1 = 0;
        ASSERT_EQ(ktxLevelProcessor_GetImageOffset(processor, level, 0, 0,
                                                   &offset0),
                  KTX_SUCCESS);
        EXPECT_EQ(offset0, 0u) << "level " << level;
        ASSERT_EQ(ktxLevelProcessor_GetImageOffset(processor, level, 1, 0,
                                                   &offset1),
                  KTX_SUCCESS);
        // Cross-check the level-relative offset against the reference's
        // mip-chain-relative offsets.
        ktx_size_t refOffset0 = 0, refOffset1 = 0;
        ASSERT_EQ(ktxTexture2_GetImageOffset(reference, level, 0, 0,
                                             &refOffset0),
                  KTX_SUCCESS);
        ASSERT_EQ(ktxTexture2_GetImageOffset(reference, level, 1, 0,
                                             &refOffset1),
                  KTX_SUCCESS);
        EXPECT_EQ(offset1, refOffset1 - refOffset0) << "level " << level;
    }

    // The three layout queries report an out-of-range argument the same
    // way, so a binding needs one rule for all of them.
    ktx_size_t value = 0;
    EXPECT_EQ(ktxLevelProcessor_GetLevelSize(processor, source->numLevels,
                                             &value),
              KTX_INVALID_VALUE);
    EXPECT_EQ(ktxLevelProcessor_GetImageSize(processor, source->numLevels,
                                             &value),
              KTX_INVALID_VALUE);
    EXPECT_EQ(ktxLevelProcessor_GetImageOffset(processor, source->numLevels,
                                               0, 0, &value),
              KTX_INVALID_VALUE);
    for (ktx_uint32_t level : {32u, UINT32_MAX}) {
        SCOPED_TRACE(level);
        EXPECT_EQ(ktxLevelProcessor_GetLevelSize(processor, level, &value),
                  KTX_INVALID_VALUE);
        EXPECT_EQ(ktxLevelProcessor_GetImageSize(processor, level, &value),
                  KTX_INVALID_VALUE);
        EXPECT_EQ(ktxLevelProcessor_GetImageOffset(processor, level, 0, 0,
                                                   &value),
                  KTX_INVALID_VALUE);
    }
    EXPECT_EQ(ktxLevelProcessor_GetImageOffset(processor, 0,
                                               source->numLayers, 0, &value),
              KTX_INVALID_VALUE);
    EXPECT_EQ(ktxLevelProcessor_GetImageOffset(processor, 0, 0, 1, &value),
              KTX_INVALID_VALUE);
}

TEST(LevelProcessor, ProcessLevelValidatesArguments) {
    const std::vector<ktx_uint8_t>& file = etc1sArrayFile();
    ASSERT_FALSE(file.empty());

    ktxTexture2* source = nullptr;
    KTX_error_code result =
        ktxTexture2_CreateFromMemory(file.data(), file.size(), 0, &source);
    ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    TextureRaii source_raii = makeRaii(source);

    ktxLevelProcessor* processor = nullptr;
    result = ktxLevelProcessor_CreateBasis(source, KTX_TTF_BC7_RGBA, 0,
                                           &processor);
    ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    ProcessorRaii processor_raii = makeRaii(processor);

    ktxLevelFileInfo info = {};
    ASSERT_EQ(ktxTexture2_GetLevelFileInfo(source, 0, &info), KTX_SUCCESS);
    std::vector<ktx_uint8_t> src((size_t)info.byteLength);
    std::memcpy(src.data(), file.data() + info.byteOffset, src.size());
    ktx_size_t levelSize = 0;
    ASSERT_EQ(ktxLevelProcessor_GetLevelSize(processor, 0, &levelSize),
              KTX_SUCCESS);
    std::vector<ktx_uint8_t> dst(levelSize);

    EXPECT_EQ(ktxLevelProcessor_ProcessLevel(nullptr, 0, src.data(),
                                             src.size(), dst.data(),
                                             dst.size()),
              KTX_INVALID_VALUE);
    EXPECT_EQ(ktxLevelProcessor_ProcessLevel(processor, source->numLevels,
                                             src.data(), src.size(),
                                             dst.data(), dst.size()),
              KTX_INVALID_VALUE);
    EXPECT_EQ(ktxLevelProcessor_ProcessLevel(processor, 0, src.data(),
                                             src.size() - 1, dst.data(),
                                             dst.size()),
              KTX_INVALID_VALUE);
    EXPECT_EQ(ktxLevelProcessor_ProcessLevel(processor, 0, src.data(),
                                             src.size(), dst.data(),
                                             dst.size() - 1),
              KTX_INVALID_VALUE);
    EXPECT_EQ(ktxLevelProcessor_ProcessLevel(processor, 0, src.data(),
                                             src.size(), dst.data(),
                                             dst.size()),
              KTX_SUCCESS);

    // Loading the source's image data after the processor was created
    // discards the serialized layout ProcessLevel consumes (for Zstd/Zlib
    // sources it also rewrites the level index to the inflated sizes);
    // the call reports that instead of comparing against the new index.
    ASSERT_EQ(ktxTexture2_LoadImageData(source, nullptr, 0), KTX_SUCCESS);
    ASSERT_EQ(ktxTexture2_GetLevelFileInfo(source, 0, &info),
              KTX_INVALID_OPERATION);
    EXPECT_EQ(ktxLevelProcessor_ProcessLevel(processor, 0, src.data(),
                                             src.size(), dst.data(),
                                             dst.size()),
              KTX_INVALID_OPERATION);
}

TEST(LevelProcessor, CreateValidation) {
    const std::vector<ktx_uint8_t>& file = etc1sArrayFile();
    ASSERT_FALSE(file.empty());

    ktxTexture2* source = nullptr;
    KTX_error_code result =
        ktxTexture2_CreateFromMemory(file.data(), file.size(), 0, &source);
    ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    TextureRaii source_raii = makeRaii(source);

    ktxLevelProcessor* processor = nullptr;
    EXPECT_EQ(ktxLevelProcessor_CreateBasis(nullptr, KTX_TTF_BC7_RGBA, 0,
                                            &processor),
              KTX_INVALID_VALUE);
    EXPECT_EQ(ktxLevelProcessor_CreateBasis(source, KTX_TTF_BC7_RGBA, 0,
                                            nullptr),
              KTX_INVALID_VALUE);
    EXPECT_EQ(ktxLevelProcessor_CreateBasis(source, (ktx_transcode_fmt_e)9999,
                                            0, &processor),
              KTX_INVALID_VALUE);

    // A texture that is not Basis-compressed is not a valid source.
    ktxTextureCreateInfo createInfo = {};
    createInfo.vkFormat = 43;  // VK_FORMAT_R8G8B8A8_SRGB
    createInfo.baseWidth = 4;
    createInfo.baseHeight = 4;
    createInfo.baseDepth = 1;
    createInfo.numDimensions = 2;
    createInfo.numLevels = 1;
    createInfo.numLayers = 1;
    createInfo.numFaces = 1;
    createInfo.isArray = KTX_FALSE;
    createInfo.generateMipmaps = KTX_FALSE;
    ktxTexture2* plain = nullptr;
    result = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                &plain);
    ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    TextureRaii plain_raii = makeRaii(plain);
    EXPECT_EQ(ktxLevelProcessor_CreateBasis(plain, KTX_TTF_BC7_RGBA, 0,
                                            &processor),
              KTX_INVALID_OPERATION);

    // A source whose image data has been loaded no longer describes the
    // serialized level payloads ProcessLevel consumes.
    ktxTexture2* loaded = nullptr;
    result = ktxTexture2_CreateFromMemory(
        file.data(), file.size(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        &loaded);
    ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    TextureRaii loaded_raii = makeRaii(loaded);
    EXPECT_EQ(ktxLevelProcessor_CreateBasis(loaded, KTX_TTF_BC7_RGBA, 0,
                                            &processor),
              KTX_INVALID_OPERATION);

    // Destroy tolerates NULL.
    ktxLevelProcessor_Destroy(nullptr);
}

// ---------------------------------------------------------------------------
// ProcessLevel output against ktxTexture2_TranscodeBasis.

enum class Shape { Npot2D, Array2D, Cubemap, Volume, SingleLevel };

struct SourceSpec {
    const char* name;
    ktx_basis_codec_e codec;
    ktx_uint32_t vkFormat;  // input format handed to the encoder
    ktxSupercmpScheme deflate;
    Shape shape;
};

ktx_uint16_t floatToHalf(float f) {
    ktx_uint32_t x;
    std::memcpy(&x, &f, sizeof(x));
    const ktx_uint32_t sign = (x >> 16) & 0x8000u;
    const ktx_uint32_t mantissa = x & 0x7fffffu;
    const int exponent = (int)((x >> 23) & 0xffu) - 127 + 15;
    if (exponent <= 0)
        return (ktx_uint16_t)sign;
    if (exponent >= 31)
        return (ktx_uint16_t)(sign | 0x7c00u);
    return (ktx_uint16_t)(sign | ((ktx_uint32_t)exponent << 10) | (mantissa >> 13));
}

// Every image gets distinct content so a level or image written to the
// wrong place cannot go unnoticed.
std::vector<ktx_uint8_t> encodeSource(const SourceSpec& spec) {
    ktxTextureCreateInfo createInfo = {};
    createInfo.vkFormat = spec.vkFormat;
    createInfo.baseWidth = 32;
    createInfo.baseHeight = 32;
    createInfo.baseDepth = 1;
    createInfo.numDimensions = 2;
    createInfo.numLayers = 1;
    createInfo.numFaces = 1;
    createInfo.isArray = KTX_FALSE;
    createInfo.generateMipmaps = KTX_FALSE;
    switch (spec.shape) {
      case Shape::Npot2D:
        createInfo.baseWidth = 37;
        createInfo.baseHeight = 23;
        break;
      case Shape::Array2D:
        createInfo.numLayers = 3;
        createInfo.isArray = KTX_TRUE;
        break;
      case Shape::Cubemap:
        createInfo.baseWidth = createInfo.baseHeight = 16;
        createInfo.numFaces = 6;
        break;
      case Shape::Volume:
        createInfo.baseWidth = createInfo.baseHeight = 16;
        createInfo.baseDepth = 8;
        createInfo.numDimensions = 3;
        break;
      case Shape::SingleLevel:
        createInfo.baseWidth = 64;
        createInfo.baseHeight = 48;
        break;
    }
    ktx_uint32_t largest = std::max({createInfo.baseWidth,
                                     createInfo.baseHeight,
                                     createInfo.baseDepth});
    createInfo.numLevels = 1;
    if (spec.shape != Shape::SingleLevel)
        while (largest >>= 1) createInfo.numLevels++;

    ktxTexture2* texture = nullptr;
    KTX_error_code result = ktxTexture2_Create(&createInfo,
                                               KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                               &texture);
    EXPECT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    if (result != KTX_SUCCESS)
        return {};
    TextureRaii texture_raii = makeRaii(texture);

    const bool hdr = spec.vkFormat == 97;  // VK_FORMAT_R16G16B16A16_SFLOAT
    const ktx_uint32_t components = spec.vkFormat == 29 ? 3 : 4;
    for (ktx_uint32_t level = 0; level < createInfo.numLevels; level++) {
        const ktx_uint32_t width = std::max(1u, createInfo.baseWidth >> level);
        const ktx_uint32_t height = std::max(1u, createInfo.baseHeight >> level);
        const ktx_uint32_t depth = std::max(1u, createInfo.baseDepth >> level);
        const ktx_uint32_t faceSlices = createInfo.numFaces > 1
                                      ? createInfo.numFaces : depth;
        for (ktx_uint32_t layer = 0; layer < createInfo.numLayers; layer++) {
            for (ktx_uint32_t faceSlice = 0; faceSlice < faceSlices; faceSlice++) {
                const ktx_uint32_t seed = level * 131 + layer * 17 + faceSlice * 7 + 1;
                std::vector<ktx_uint8_t> image;
                if (hdr) {
                    std::vector<ktx_uint16_t> texels((size_t)width * height * 4);
                    for (ktx_uint32_t y = 0; y < height; y++) {
                        for (ktx_uint32_t x = 0; x < width; x++) {
                            const size_t i = ((size_t)y * width + x) * 4;
                            const float fx = (float)x / (float)width;
                            const float fy = (float)y / (float)height;
                            texels[i + 0] = floatToHalf(0.05f + 8.0f * fx * fx
                                                        + 0.3f * (float)(seed % 5));
                            texels[i + 1] = floatToHalf(0.02f + 3.0f * fy
                                                        + 0.1f * (float)(seed % 7));
                            texels[i + 2] = floatToHalf(0.5f + 4.0f * fx * fy
                                                        + (float)(seed % 3));
                            texels[i + 3] = floatToHalf(1.0f);
                        }
                    }
                    image.resize(texels.size() * sizeof(ktx_uint16_t));
                    std::memcpy(image.data(), texels.data(), image.size());
                } else {
                    image.resize((size_t)width * height * components);
                    for (ktx_uint32_t y = 0; y < height; y++) {
                        for (ktx_uint32_t x = 0; x < width; x++) {
                            const size_t i = ((size_t)y * width + x) * components;
                            image[i + 0] = (ktx_uint8_t)(x * 255 / std::max(1u, width - 1) + seed * 3);
                            image[i + 1] = (ktx_uint8_t)(y * 255 / std::max(1u, height - 1) + seed * 11);
                            image[i + 2] = (ktx_uint8_t)(((x ^ y) * 29 + seed * 5) & 0xffu);
                            if (components == 4)
                                image[i + 3] = (ktx_uint8_t)((x * 7 + y * 13 + seed * 23) & 0xffu);
                        }
                    }
                }
                result = ktxTexture_SetImageFromMemory(ktxTexture(texture), level,
                                                       layer, faceSlice,
                                                       image.data(), image.size());
                EXPECT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
                if (result != KTX_SUCCESS)
                    return {};
            }
        }
    }

    ktxBasisParams params = {};
    params.structSize = sizeof(params);
    params.threadCount = 1;
    params.codec = spec.codec;
    if (spec.codec == KTX_BASIS_CODEC_ETC1S) {
        params.etc1sCompressionLevel = KTX_ETC1S_DEFAULT_COMPRESSION_LEVEL;
        params.qualityLevel = 128;
    } else if (spec.codec == KTX_BASIS_CODEC_UASTC_LDR_4x4) {
        params.uastcFlags = KTX_PACK_UASTC_LEVEL_FASTEST;
    }
    result = ktxTexture2_CompressBasisEx(texture, &params);
    EXPECT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    if (result != KTX_SUCCESS)
        return {};
    if (spec.deflate == KTX_SS_ZSTD)
        result = ktxTexture2_DeflateZstd(texture, 5);
    else if (spec.deflate == KTX_SS_ZLIB)
        result = ktxTexture2_DeflateZLIB(texture, 5);
    EXPECT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    if (result != KTX_SUCCESS)
        return {};

    ktx_uint8_t* bytes = nullptr;
    ktx_size_t size = 0;
    result = ktxTexture_WriteToMemory(ktxTexture(texture), &bytes, &size);
    EXPECT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    if (result != KTX_SUCCESS)
        return {};
    std::vector<ktx_uint8_t> file(bytes, bytes + size);
    free(bytes);
    return file;
}

struct EquivalenceCase {
    SourceSpec source;
    ktx_transcode_fmt_e target;
    ktx_transcode_flags flags;
};

std::ostream& operator<<(std::ostream& os, const EquivalenceCase& c) {
    return os << c.source.name << " -> " << ktxTranscodeFormatString(c.target)
              << " flags " << c.flags;
}

class LevelProcessorEquivalence
    : public ::testing::TestWithParam<EquivalenceCase> {};

// Every level processed from its serialized payload, in a scrambled order
// and one level again at the end, must equal the corresponding level of a
// whole-texture ktxTexture2_TranscodeBasis of the fully loaded file.
TEST_P(LevelProcessorEquivalence, MatchesTranscodeBasis) {
    const EquivalenceCase& c = GetParam();
    const std::vector<ktx_uint8_t> file = encodeSource(c.source);
    ASSERT_FALSE(file.empty());

    ktxTexture2* reference = nullptr;
    KTX_error_code result = ktxTexture2_CreateFromMemory(
        file.data(), file.size(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        &reference);
    ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    TextureRaii reference_raii = makeRaii(reference);
    result = ktxTexture2_TranscodeBasis(reference, c.target, c.flags);
    ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);

    ktxTexture2* source = nullptr;
    result = ktxTexture2_CreateFromMemory(file.data(), file.size(), 0, &source);
    ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    TextureRaii source_raii = makeRaii(source);
    if (c.source.deflate != KTX_SS_NONE) {
        ASSERT_EQ(source->supercompressionScheme, c.source.deflate);
    }

    ktxLevelProcessor* processor = nullptr;
    result = ktxLevelProcessor_CreateBasis(source, c.target, c.flags,
                                           &processor);
    ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    ProcessorRaii processor_raii = makeRaii(processor);
    EXPECT_EQ(ktxLevelProcessor_GetOutputVkFormat(processor),
              reference->vkFormat);

    std::vector<ktx_uint32_t> order(source->numLevels);
    for (ktx_uint32_t i = 0; i < source->numLevels; i++)
        order[i] = i;
    // Start from the middle level so the order is neither the file's
    // order nor its reverse, then process the base level a second time.
    std::rotate(order.begin(), order.begin() + order.size() / 2, order.end());
    order.push_back(0);

    for (ktx_uint32_t level : order) {
        ktxLevelFileInfo info = {};
        ASSERT_EQ(ktxTexture2_GetLevelFileInfo(source, level, &info),
                  KTX_SUCCESS);
        ASSERT_LE(info.byteOffset + info.byteLength, (ktx_uint64_t)file.size());
        ktx_size_t levelSize = 0;
        ASSERT_EQ(ktxLevelProcessor_GetLevelSize(processor, level, &levelSize),
                  KTX_SUCCESS);

        std::vector<ktx_uint8_t> dst(levelSize, 0xa5);
        result = ktxLevelProcessor_ProcessLevel(
            processor, level, file.data() + (size_t)info.byteOffset,
            (ktx_size_t)info.byteLength, dst.data(), dst.size());
        ASSERT_EQ(result, KTX_SUCCESS)
            << "level " << level << ": " << ktxErrorString(result);

        ktx_size_t referenceOffset = 0;
        ASSERT_EQ(ktxTexture_GetImageOffset(ktxTexture(reference), level, 0, 0,
                                            &referenceOffset),
                  KTX_SUCCESS);
        ASSERT_LE(referenceOffset + levelSize, reference->dataSize);
        EXPECT_TRUE(std::equal(dst.begin(), dst.end(),
                               reference->pData + referenceOffset))
            << "level " << level << " differs from TranscodeBasis";
    }
}

// Source families x shapes x targets. The PVRTC1 cases include levels
// smaller than its 8x8 minimum, which both paths fill out to 2x2 blocks.
const SourceSpec kEtc1sAlphaArray =
    {"etc1s-alpha-array", KTX_BASIS_CODEC_ETC1S, 43, KTX_SS_NONE, Shape::Array2D};
const SourceSpec kEtc1sOpaqueVolume =
    {"etc1s-opaque-volume", KTX_BASIS_CODEC_ETC1S, 29, KTX_SS_NONE, Shape::Volume};
const SourceSpec kEtc1sAlphaCubemap =
    {"etc1s-alpha-cubemap", KTX_BASIS_CODEC_ETC1S, 43, KTX_SS_NONE, Shape::Cubemap};
const SourceSpec kUastcRawSingle =
    {"uastc-raw-single", KTX_BASIS_CODEC_UASTC_LDR_4x4, 43, KTX_SS_NONE, Shape::SingleLevel};
const SourceSpec kUastcZstdCubemap =
    {"uastc-zstd-cubemap", KTX_BASIS_CODEC_UASTC_LDR_4x4, 43, KTX_SS_ZSTD, Shape::Cubemap};
const SourceSpec kUastcZlibNpot =
    {"uastc-zlib-npot", KTX_BASIS_CODEC_UASTC_LDR_4x4, 43, KTX_SS_ZLIB, Shape::Npot2D};
const SourceSpec kUastcZstdVolume =
    {"uastc-zstd-volume", KTX_BASIS_CODEC_UASTC_LDR_4x4, 29, KTX_SS_ZSTD, Shape::Volume};
const SourceSpec kHdr4x4ZstdVolume =
    {"hdr4x4-zstd-volume", KTX_BASIS_CODEC_UASTC_HDR_4x4, 97, KTX_SS_ZSTD, Shape::Volume};
const SourceSpec kHdr4x4RawNpot =
    {"hdr4x4-raw-npot", KTX_BASIS_CODEC_UASTC_HDR_4x4, 97, KTX_SS_NONE, Shape::Npot2D};
const SourceSpec kHdr6x6iArray =
    {"hdr6x6i-array", KTX_BASIS_CODEC_UASTC_HDR_6x6_INTERMEDIATE, 97, KTX_SS_NONE, Shape::Array2D};
const SourceSpec kHdr6x6iVolume =
    {"hdr6x6i-volume", KTX_BASIS_CODEC_UASTC_HDR_6x6_INTERMEDIATE, 97, KTX_SS_NONE, Shape::Volume};

const ktx_transcode_flags kHqAlphaToOpaque =
    KTX_TF_HIGH_QUALITY | KTX_TF_TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS;

INSTANTIATE_TEST_SUITE_P(
    Transcode, LevelProcessorEquivalence,
    ::testing::Values(
        EquivalenceCase{kEtc1sAlphaArray, KTX_TTF_BC7_RGBA, 0},
        EquivalenceCase{kEtc1sAlphaArray, KTX_TTF_BC1_OR_3, 0},
        EquivalenceCase{kEtc1sAlphaArray, KTX_TTF_ETC, 0},
        EquivalenceCase{kEtc1sAlphaArray, KTX_TTF_RGBA32, 0},
        EquivalenceCase{kEtc1sAlphaArray, KTX_TTF_BC4_R, kHqAlphaToOpaque},
        EquivalenceCase{kEtc1sAlphaArray, KTX_TTF_PVRTC1_4_RGBA, 0},
        EquivalenceCase{kEtc1sOpaqueVolume, KTX_TTF_BC1_RGB, 0},
        EquivalenceCase{kEtc1sOpaqueVolume, KTX_TTF_ETC2_EAC_R11, 0},
        EquivalenceCase{kEtc1sAlphaCubemap, KTX_TTF_ASTC_4x4_RGBA, 0},
        EquivalenceCase{kEtc1sAlphaCubemap, KTX_TTF_PVRTC2_4_RGBA, 0},
        EquivalenceCase{kUastcRawSingle, KTX_TTF_ASTC_4x4_RGBA, 0},
        EquivalenceCase{kUastcRawSingle, KTX_TTF_RGBA4444, 0},
        EquivalenceCase{kUastcZstdCubemap, KTX_TTF_BC7_RGBA, 0},
        EquivalenceCase{kUastcZstdCubemap, KTX_TTF_BC1_OR_3, kHqAlphaToOpaque},
        EquivalenceCase{kUastcZstdCubemap, KTX_TTF_PVRTC1_4_RGB, 0},
        EquivalenceCase{kUastcZlibNpot, KTX_TTF_ETC2_RGBA, 0},
        EquivalenceCase{kUastcZlibNpot, KTX_TTF_BC5_RG, 0},
        EquivalenceCase{kUastcZstdVolume, KTX_TTF_ETC, 0},
        EquivalenceCase{kUastcZstdVolume, KTX_TTF_RGB565, 0},
        EquivalenceCase{kHdr4x4ZstdVolume, KTX_TTF_BC6HU_RGB, 0},
        // Passthrough: the blocks are copied unchanged after inflation.
        EquivalenceCase{kHdr4x4ZstdVolume, KTX_TTF_ASTC_HDR_4x4_RGBA, 0},
        EquivalenceCase{kHdr4x4RawNpot, KTX_TTF_RGBA_HALF, 0},
        EquivalenceCase{kHdr4x4RawNpot, KTX_TTF_ASTC_HDR_4x4_RGBA, 0},
        EquivalenceCase{kHdr6x6iArray, KTX_TTF_ASTC_HDR_6x6_RGBA, 0},
        EquivalenceCase{kHdr6x6iArray, KTX_TTF_BC6HU_RGB, 0},
        EquivalenceCase{kHdr6x6iVolume, KTX_TTF_RGB_9E5, 0},
        EquivalenceCase{kHdr6x6iVolume, KTX_TTF_RGB_HALF, 0}));

// ---------------------------------------------------------------------------
// Corrupt input.

// A payload that is not a valid Zstd or ZLIB stream for the level is
// reported as an error, never as a successfully processed level. A damaged
// stream header is detected for both schemes. Damage inside the stream is
// detected for ZLIB, whose streams end with an Adler-32 checksum; libktx
// writes Zstd frames without a content checksum, so a Zstd frame damaged
// inside a literals section can inflate to data of the right length.
TEST(LevelProcessor, ProcessLevelRejectsCorruptSupercompressedPayload) {
    const SourceSpec specs[] = {
        {"uastc-zstd", KTX_BASIS_CODEC_UASTC_LDR_4x4, 43, KTX_SS_ZSTD, Shape::Npot2D},
        {"uastc-zlib", KTX_BASIS_CODEC_UASTC_LDR_4x4, 43, KTX_SS_ZLIB, Shape::Npot2D},
    };
    for (const SourceSpec& spec : specs) {
        SCOPED_TRACE(spec.name);
        const std::vector<ktx_uint8_t> file = encodeSource(spec);
        ASSERT_FALSE(file.empty());

        ktxTexture2* source = nullptr;
        KTX_error_code result =
            ktxTexture2_CreateFromMemory(file.data(), file.size(), 0, &source);
        ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
        TextureRaii source_raii = makeRaii(source);

        ktxLevelProcessor* processor = nullptr;
        result = ktxLevelProcessor_CreateBasis(source, KTX_TTF_RGBA32, 0,
                                               &processor);
        ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
        ProcessorRaii processor_raii = makeRaii(processor);

        ktxLevelFileInfo info = {};
        ASSERT_EQ(ktxTexture2_GetLevelFileInfo(source, 0, &info), KTX_SUCCESS);
        std::vector<ktx_uint8_t> payload(file.begin() + (ptrdiff_t)info.byteOffset,
                                         file.begin() + (ptrdiff_t)(info.byteOffset
                                                                    + info.byteLength));
        ktx_size_t levelSize = 0;
        ASSERT_EQ(ktxLevelProcessor_GetLevelSize(processor, 0, &levelSize),
                  KTX_SUCCESS);
        std::vector<ktx_uint8_t> dst(levelSize);

        // Intact payload first, to show the corruption is what fails.
        ASSERT_EQ(ktxLevelProcessor_ProcessLevel(processor, 0, payload.data(),
                                                 payload.size(), dst.data(),
                                                 dst.size()),
                  KTX_SUCCESS);

        std::vector<std::vector<ktx_uint8_t>> damaged;
        damaged.push_back(payload);  // Stream header overwritten.
        std::fill(damaged.back().begin(), damaged.back().begin() + 4,
                  (ktx_uint8_t)0xff);
        if (spec.deflate == KTX_SS_ZLIB) {
            damaged.push_back(payload);  // Middle of the stream zeroed.
            std::fill(damaged.back().begin() + (ptrdiff_t)(payload.size() / 2),
                      damaged.back().begin() + (ptrdiff_t)(payload.size() / 2 + 8),
                      (ktx_uint8_t)0x00);
        }
        for (const std::vector<ktx_uint8_t>& bad : damaged) {
            result = ktxLevelProcessor_ProcessLevel(processor, 0, bad.data(),
                                                    bad.size(), dst.data(),
                                                    dst.size());
            EXPECT_TRUE(result == KTX_FILE_DATA_ERROR
                        || result == KTX_DECOMPRESS_LENGTH_ERROR
                        || result == KTX_DECOMPRESS_CHECKSUM_ERROR)
                << ktxErrorString(result);
        }

        // The processor is still usable after a failed level.
        EXPECT_EQ(ktxLevelProcessor_ProcessLevel(processor, 0, payload.data(),
                                                 payload.size(), dst.data(),
                                                 dst.size()),
                  KTX_SUCCESS);
    }
}

// BasisLZ global data with an invalid layout is reported when the
// processor is created, with the same code ktxTexture2_TranscodeBasis
// returns for it.
TEST(LevelProcessor, CreateRejectsCorruptBasisLzGlobalData) {
    std::vector<ktx_uint8_t> file = etc1sArrayFile();
    ASSERT_FALSE(file.empty());

    // KTX2 header: sgdByteOffset is the uint64 at byte 64. In the BasisLZ
    // global header tablesByteLength is the uint32 at byte 12.
    ktx_uint64_t sgdByteOffset = 0;
    std::memcpy(&sgdByteOffset, file.data() + 64, sizeof(sgdByteOffset));
    ASSERT_GT(sgdByteOffset, 0u);
    ASSERT_LE(sgdByteOffset + 16, (ktx_uint64_t)file.size());
    const ktx_uint32_t zero = 0;
    std::memcpy(file.data() + (size_t)sgdByteOffset + 12, &zero, sizeof(zero));

    ktxTexture2* source = nullptr;
    KTX_error_code result =
        ktxTexture2_CreateFromMemory(file.data(), file.size(), 0, &source);
    ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    TextureRaii source_raii = makeRaii(source);

    ktxLevelProcessor* processor = nullptr;
    EXPECT_EQ(ktxLevelProcessor_CreateBasis(source, KTX_TTF_BC7_RGBA, 0,
                                            &processor),
              KTX_FILE_DATA_ERROR);

    ktxTexture2* loaded = nullptr;
    result = ktxTexture2_CreateFromMemory(
        file.data(), file.size(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        &loaded);
    ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    TextureRaii loaded_raii = makeRaii(loaded);
    EXPECT_EQ(ktxTexture2_TranscodeBasis(loaded, KTX_TTF_BC7_RGBA, 0),
              KTX_FILE_DATA_ERROR);
}

// BasisLZ global data shorter than its header is rejected before the
// header is read, by the processor and by ktxTexture2_TranscodeBasis.
TEST(LevelProcessor, CreateRejectsShortBasisLzGlobalData) {
    std::vector<ktx_uint8_t> file = etc1sArrayFile();
    ASSERT_FALSE(file.empty());

    // KTX2 header: sgdByteLength is the uint64 at byte 72. The BasisLZ
    // global header is 20 bytes.
    const ktx_uint64_t shortLength = 8;
    std::memcpy(file.data() + 72, &shortLength, sizeof(shortLength));

    ktxTexture2* source = nullptr;
    KTX_error_code result =
        ktxTexture2_CreateFromMemory(file.data(), file.size(), 0, &source);
    ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    TextureRaii source_raii = makeRaii(source);

    ktxLevelProcessor* processor = nullptr;
    EXPECT_EQ(ktxLevelProcessor_CreateBasis(source, KTX_TTF_BC7_RGBA, 0,
                                            &processor),
              KTX_FILE_DATA_ERROR);

    ktxTexture2* loaded = nullptr;
    result = ktxTexture2_CreateFromMemory(
        file.data(), file.size(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        &loaded);
    ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    TextureRaii loaded_raii = makeRaii(loaded);
    EXPECT_EQ(ktxTexture2_TranscodeBasis(loaded, KTX_TTF_BC7_RGBA, 0),
              KTX_FILE_DATA_ERROR);
}

// BasisLZ palettes or tables that fail to decode are reported by the level
// transcodes as KTX_TRANSCODE_FAILED, by ktxTexture2_TranscodeBasis and by
// the processor alike. @p part is 0, 1 or 2 for the endpoints, selectors or
// tables, whose first bytes are overwritten with @p junk, repeated over
// @p junkLength bytes or the whole part if it is 0.
static void
checkUndecodableBasisLzGlobalData(int part, ktx_uint32_t junk,
                                  ktx_uint32_t junkLength) {
    std::vector<ktx_uint8_t> file = etc1sArrayFile();
    ASSERT_FALSE(file.empty());

    // KTX2 header: sgdByteOffset and sgdByteLength are the uint64s at bytes
    // 64 and 72. The BasisLZ global data ends with the endpoints, selectors,
    // tables and extended data, whose byte lengths are the uint32s at bytes
    // 4 to 16 of its header.
    ktx_uint64_t sgdByteOffset = 0, sgdByteLength = 0;
    std::memcpy(&sgdByteOffset, file.data() + 64, sizeof(sgdByteOffset));
    std::memcpy(&sgdByteLength, file.data() + 72, sizeof(sgdByteLength));
    ASSERT_GT(sgdByteOffset, 0u);
    ASSERT_LE(sgdByteOffset + sgdByteLength, (ktx_uint64_t)file.size());
    ktx_uint32_t lengths[4];
    std::memcpy(lengths, file.data() + (size_t)sgdByteOffset + 4,
                sizeof(lengths));
    ktx_uint64_t offset = sgdByteOffset + sgdByteLength
        - lengths[0] - lengths[1] - lengths[2] - lengths[3];
    for (int i = 0; i < part; i++)
        offset += lengths[i];
    const ktx_uint32_t length = junkLength ? junkLength : lengths[part];
    ASSERT_LE(length, lengths[part]);
    for (ktx_uint32_t i = 0; i < length; i++)
        file[(size_t)offset + i] = (ktx_uint8_t)(junk >> (8 * (i % 4)));

    ktxTexture2* loaded = nullptr;
    KTX_error_code result = ktxTexture2_CreateFromMemory(
        file.data(), file.size(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        &loaded);
    ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    TextureRaii loaded_raii = makeRaii(loaded);
    EXPECT_EQ(ktxTexture2_TranscodeBasis(loaded, KTX_TTF_BC7_RGBA, 0),
              KTX_TRANSCODE_FAILED);

    ktxTexture2* source = nullptr;
    result = ktxTexture2_CreateFromMemory(file.data(), file.size(), 0, &source);
    ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    TextureRaii source_raii = makeRaii(source);

    ktxLevelProcessor* processor = nullptr;
    result = ktxLevelProcessor_CreateBasis(source, KTX_TTF_BC7_RGBA, 0,
                                           &processor);
    ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    ProcessorRaii processor_raii = makeRaii(processor);

    for (ktx_uint32_t level = 0; level < source->numLevels; level++) {
        ktxLevelFileInfo info = {};
        ASSERT_EQ(ktxTexture2_GetLevelFileInfo(source, level, &info),
                  KTX_SUCCESS);
        ktx_size_t levelSize = 0;
        ASSERT_EQ(ktxLevelProcessor_GetLevelSize(processor, level, &levelSize),
                  KTX_SUCCESS);
        std::vector<ktx_uint8_t> dst(levelSize);
        EXPECT_EQ(ktxLevelProcessor_ProcessLevel(
                      processor, level, file.data() + (size_t)info.byteOffset,
                      (ktx_size_t)info.byteLength, dst.data(), dst.size()),
                  KTX_TRANSCODE_FAILED)
            << "level " << level;
    }
}

// Palettes damaged as in the CTS's patched BasisLZ files.
TEST(LevelProcessor, UndecodableBasisLzPalettesFailTheTranscode) {
    checkUndecodableBasisLzGlobalData(0, 0xdeadbeef, 4);
}

TEST(LevelProcessor, UndecodableBasisLzTablesFailTheTranscode) {
    checkUndecodableBasisLzGlobalData(2, 0xffffffff, 0);
}

}  // namespace

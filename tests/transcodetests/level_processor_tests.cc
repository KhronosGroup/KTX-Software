// Copyright 2026 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

// ktxLevelProcessor skeleton coverage: creation validation, resolved
// output format and target-layout queries, verified against the actual
// output layout of ktxTexture2_TranscodeBasis on the same source.
// ProcessLevel is a stub in this commit (returns KTX_UNSUPPORTED_FEATURE
// after argument validation); its codec tests land with the
// implementation.

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

// 2D array with a full mip chain, or a two-frame video with one level.
// Encoded to ETC1S so SGD handling is included.
std::vector<ktx_uint8_t> encodeEtc1sArray(bool video = false) {
    ktxTextureCreateInfo createInfo = {};
    createInfo.vkFormat = 43;  // VK_FORMAT_R8G8B8A8_SRGB
    createInfo.baseWidth = 32;
    createInfo.baseHeight = 32;
    createInfo.baseDepth = 1;
    createInfo.numDimensions = 2;
    createInfo.numLevels = video ? 1 : 6;
    createInfo.numLayers = 2;
    createInfo.numFaces = 1;
    createInfo.isArray = KTX_TRUE;
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
    static const std::vector<ktx_uint8_t> file = encodeEtc1sArray();
    return file;
}

const std::vector<ktx_uint8_t>& etc1sVideoFile() {
    static const std::vector<ktx_uint8_t> file = encodeEtc1sArray(true);
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

TEST(LevelProcessor, ProcessLevelValidatesThenReportsUnimplemented) {
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

    // Argument validation precedes the not-yet-implemented report.
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
    // Correct arguments: the stub reports the codec paths as pending.
    EXPECT_EQ(ktxLevelProcessor_ProcessLevel(processor, 0, src.data(),
                                             src.size(), dst.data(),
                                             dst.size()),
              KTX_UNSUPPORTED_FEATURE);

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

}  // namespace

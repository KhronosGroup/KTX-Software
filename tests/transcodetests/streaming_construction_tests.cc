// Copyright 2026 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

// Metadata-only ("shell") construction coverage for the per-level streaming
// work discussed in https://github.com/KhronosGroup/KTX-Software/issues/1224.
//
// A streaming consumer fetches the serialized file prefix — identifier,
// header, level index, DFD, KVD and SGD, i.e. every byte before the first
// level's byteOffset — and constructs a ktxTexture2 from it without
// KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT. These tests pin down the existing
// constructor behavior that construction relies on:
//
//  1. Construction from exactly that prefix succeeds for BasisLZ (scheme 1,
//     SGD present), Zstd-supercompressed UASTC (scheme 2) and plain UASTC
//     (scheme 0, with alignment padding between metadata and level data).
//  2. The constructor never reads past the metadata prefix. Proven two ways:
//     an instrumented custom ktxStream whose read/skip/setpos refuse to
//     cross the prefix boundary (all platforms), and, on POSIX, the prefix
//     placed flush against a PROT_NONE guard page so any over-read faults
//     deterministically.
//  3. Every truncation fails cleanly, not with a crash: the LOAD bit on a
//     metadata-only buffer, a later ktxTexture2_LoadImageData on the shell,
//     and metadata cut one byte into the last section (mid-SGD for BasisLZ,
//     mid-KVD otherwise).
//
// Test files are generated in-process with the write API; no new binary
// resources are required.

#include <string.h>
#include "ktx.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <memory>
#include <vector>

#if !defined(_WIN32)
  #include <sys/mman.h>
  #include <unistd.h>
#endif

namespace {

//////////////////////////////
// Serialized-file map: the level index and section extents as written,
// parsed with the container layout from the KTX File Format Specification.
//////////////////////////////

struct LevelEntry {
    ktx_uint64_t byteOffset;
    ktx_uint64_t byteLength;
    ktx_uint64_t uncompressedByteLength;
};

struct FileMap {
    ktx_uint32_t levelCount = 0;
    ktx_uint32_t supercompressionScheme = 0;
    std::vector<LevelEntry> levels;
    // First byte of image data == min level byteOffset. The safe amount for
    // a streamer to fetch.
    ktx_uint64_t prefixLen = 0;
    // End of the last metadata section actually read by the constructor
    // (DFD/KVD/SGD). With scheme 0 alignment padding may follow before
    // prefixLen.
    ktx_uint64_t lastMetaByte = 0;
};

FileMap parseFileMap(const std::vector<ktx_uint8_t>& file) {
    FileMap m;
    EXPECT_GE(file.size(), 80u);
    ktx_uint32_t dfdOffset, dfdLength, kvdOffset, kvdLength;
    ktx_uint64_t sgdOffset, sgdLength;
    std::memcpy(&m.levelCount, file.data() + 40, 4);
    std::memcpy(&m.supercompressionScheme, file.data() + 44, 4);
    std::memcpy(&dfdOffset, file.data() + 48, 4);
    std::memcpy(&dfdLength, file.data() + 52, 4);
    std::memcpy(&kvdOffset, file.data() + 56, 4);
    std::memcpy(&kvdLength, file.data() + 60, 4);
    std::memcpy(&sgdOffset, file.data() + 64, 8);
    std::memcpy(&sgdLength, file.data() + 72, 8);

    ktx_uint64_t minOffset = UINT64_MAX;
    m.levels.resize(m.levelCount);
    for (ktx_uint32_t l = 0; l < m.levelCount; l++) {
        std::memcpy(&m.levels[l], file.data() + 80 + 24 * (size_t)l, 24);
        minOffset = std::min(minOffset, m.levels[l].byteOffset);
    }
    m.prefixLen = minOffset;

    ktx_uint64_t last = (ktx_uint64_t)dfdOffset + dfdLength;
    if (kvdLength)
        last = std::max(last, (ktx_uint64_t)kvdOffset + kvdLength);
    if (sgdLength)
        last = std::max(last, sgdOffset + sgdLength);
    m.lastMetaByte = last;
    EXPECT_GE(m.prefixLen, m.lastMetaByte);
    return m;
}

//////////////////////////////
// Instrumented bounded stream: serves at most `size` bytes and records
// whether the library ever tried to touch anything beyond them.
//////////////////////////////

struct BoundedStreamState {
    const ktx_uint8_t* data;
    ktx_size_t size;
    ktx_off_t pos = 0;
    bool overRead = false;
};

BoundedStreamState* boundedState(ktxStream* str) {
    return static_cast<BoundedStreamState*>(str->data.custom_ptr.address);
}

KTX_error_code boundedRead(ktxStream* str, void* dst, const ktx_size_t count) {
    BoundedStreamState* s = boundedState(str);
    if ((ktx_size_t)s->pos + count > s->size) {
        s->overRead = true;
        return KTX_FILE_UNEXPECTED_EOF;
    }
    std::memcpy(dst, s->data + s->pos, count);
    s->pos += (ktx_off_t)count;
    return KTX_SUCCESS;
}

KTX_error_code boundedSkip(ktxStream* str, const ktx_size_t count) {
    BoundedStreamState* s = boundedState(str);
    if ((ktx_size_t)s->pos + count > s->size) {
        s->overRead = true;
        return KTX_FILE_UNEXPECTED_EOF;
    }
    s->pos += (ktx_off_t)count;
    return KTX_SUCCESS;
}

KTX_error_code boundedGetpos(ktxStream* str, ktx_off_t* const offset) {
    *offset = boundedState(str)->pos;
    return KTX_SUCCESS;
}

KTX_error_code boundedSetpos(ktxStream* str, const ktx_off_t offset) {
    BoundedStreamState* s = boundedState(str);
    if ((ktx_size_t)offset > s->size) {
        s->overRead = true;
        return KTX_FILE_UNEXPECTED_EOF;
    }
    s->pos = offset;
    return KTX_SUCCESS;
}

KTX_error_code boundedGetsize(ktxStream* str, ktx_size_t* const size) {
    *size = boundedState(str)->size;
    return KTX_SUCCESS;
}

void boundedDestruct(ktxStream*) {}

ktxStream makeBoundedStream(BoundedStreamState* state) {
    ktxStream stream = {};
    stream.read = boundedRead;
    stream.skip = boundedSkip;
    stream.write = nullptr;
    stream.getpos = boundedGetpos;
    stream.setpos = boundedSetpos;
    stream.getsize = boundedGetsize;
    stream.destruct = boundedDestruct;
    stream.type = eStreamTypeCustom;
    stream.data.custom_ptr.address = state;
    stream.data.custom_ptr.size = state->size;
    stream.closeOnDestruct = KTX_FALSE;
    return stream;
}

//////////////////////////////
// In-process encoded test files, one per supercompression variant.
//////////////////////////////

enum class Variant { Etc1sBasisLz, UastcZstd, UastcRaw };

const char* variantName(Variant v) {
    switch (v) {
      case Variant::Etc1sBasisLz: return "Etc1sBasisLz";
      case Variant::UastcZstd: return "UastcZstd";
      case Variant::UastcRaw: return "UastcRaw";
    }
    return "?";
}

std::vector<ktx_uint8_t> encodeVariant(Variant variant) {
    ktxTextureCreateInfo createInfo = {};
    createInfo.vkFormat = 43;  // VK_FORMAT_R8G8B8A8_SRGB
    createInfo.baseWidth = 64;
    createInfo.baseHeight = 64;
    createInfo.baseDepth = 1;
    createInfo.numDimensions = 2;
    createInfo.numLevels = 7;
    createInfo.numLayers = 1;
    createInfo.numFaces = 1;
    createInfo.isArray = KTX_FALSE;
    createInfo.generateMipmaps = KTX_FALSE;

    ktxTexture2* texture = nullptr;
    KTX_error_code result = ktxTexture2_Create(&createInfo,
                                               KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                               &texture);
    EXPECT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    if (result != KTX_SUCCESS)
        return {};
    std::unique_ptr<ktxTexture2, void(*)(ktxTexture2*)> texture_raii(
        texture, [](ktxTexture2* t) { ktxTexture_Destroy(ktxTexture(t)); });

    for (ktx_uint32_t level = 0; level < createInfo.numLevels; level++) {
        const ktx_uint32_t width = std::max(1u, createInfo.baseWidth >> level);
        const ktx_uint32_t height = std::max(1u, createInfo.baseHeight >> level);
        std::vector<ktx_uint8_t> pixels((size_t)width * height * 4);
        for (size_t i = 0; i < pixels.size(); i += 4) {
            pixels[i + 0] = (ktx_uint8_t)(i * 7 + level * 31);
            pixels[i + 1] = (ktx_uint8_t)(i * 13 + level * 17);
            pixels[i + 2] = (ktx_uint8_t)(i * 3 + level * 53);
            pixels[i + 3] = 255;
        }
        result = ktxTexture_SetImageFromMemory(ktxTexture(texture), level, 0, 0,
                                               pixels.data(), pixels.size());
        EXPECT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    }

    ktxBasisParams cparams = {};
    cparams.structSize = sizeof(cparams);
    cparams.threadCount = 1;
    if (variant == Variant::Etc1sBasisLz) {
        cparams.codec = KTX_BASIS_CODEC_ETC1S;
        cparams.etc1sCompressionLevel = KTX_ETC1S_DEFAULT_COMPRESSION_LEVEL;
        cparams.qualityLevel = 128;
    } else {
        cparams.codec = KTX_BASIS_CODEC_UASTC_LDR_4x4;
    }
    result = ktxTexture2_CompressBasisEx(texture, &cparams);
    EXPECT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    if (variant == Variant::UastcZstd) {
        result = ktxTexture2_DeflateZstd(texture, 5);
        EXPECT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    }

    ktx_uint8_t* bytes = nullptr;
    ktx_size_t size = 0;
    result = ktxTexture_WriteToMemory(ktxTexture(texture), &bytes, &size);
    EXPECT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    std::vector<ktx_uint8_t> file(bytes, bytes + size);
    free(bytes);
    return file;
}

const std::vector<ktx_uint8_t>& fileFor(Variant variant) {
    // Encoding (particularly ETC1S) is the expensive part; do it once per
    // variant for the whole suite.
    static std::map<Variant, std::vector<ktx_uint8_t>> cache;
    auto it = cache.find(variant);
    if (it == cache.end())
        it = cache.emplace(variant, encodeVariant(variant)).first;
    return it->second;
}

class StreamingShellTest : public ::testing::TestWithParam<Variant> {};

//////////////////////////////
// 1+2. Constructing from exactly the metadata prefix succeeds and never
// touches a byte beyond it.
//////////////////////////////

TEST_P(StreamingShellTest, MetadataPrefixConstructsWithoutOverRead) {
    const std::vector<ktx_uint8_t>& file = fileFor(GetParam());
    ASSERT_FALSE(file.empty());
    const FileMap m = parseFileMap(file);

    BoundedStreamState state = {file.data(), (ktx_size_t)m.prefixLen, 0, false};
    ktxStream stream = makeBoundedStream(&state);

    ktxTexture2* shell = nullptr;
    KTX_error_code result = ktxTexture2_CreateFromStream(&stream, 0, &shell);
    ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    EXPECT_FALSE(state.overRead)
        << "constructor touched bytes past the metadata prefix";

    EXPECT_EQ(shell->numLevels, m.levelCount);
    EXPECT_EQ(shell->supercompressionScheme, m.supercompressionScheme);
    EXPECT_EQ(shell->pData, nullptr) << "shell should carry no image data";

    ktxTexture_Destroy(ktxTexture(shell));
}

#if !defined(_WIN32)
// The strongest form of the same statement: the prefix ends flush against a
// PROT_NONE page, so an over-read does not return an error — it faults.
TEST_P(StreamingShellTest, MetadataPrefixConstructsAgainstGuardPage) {
    const std::vector<ktx_uint8_t>& file = fileFor(GetParam());
    ASSERT_FALSE(file.empty());
    const FileMap m = parseFileMap(file);

    const size_t page = (size_t)sysconf(_SC_PAGESIZE);
    const size_t dataPages = ((size_t)m.prefixLen + page - 1) / page;
    ktx_uint8_t* base = static_cast<ktx_uint8_t*>(
        mmap(nullptr, (dataPages + 1) * page, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    ASSERT_NE(base, MAP_FAILED);
    ASSERT_EQ(mprotect(base + dataPages * page, page, PROT_NONE), 0);
    ktx_uint8_t* prefix = base + dataPages * page - (size_t)m.prefixLen;
    std::memcpy(prefix, file.data(), (size_t)m.prefixLen);

    ktxTexture2* shell = nullptr;
    KTX_error_code result =
        ktxTexture2_CreateFromMemory(prefix, (ktx_size_t)m.prefixLen, 0, &shell);
    EXPECT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);
    if (shell)
        ktxTexture_Destroy(ktxTexture(shell));
    munmap(base, (dataPages + 1) * page);
}
#endif

//////////////////////////////
// 3. Truncations fail cleanly.
//////////////////////////////

TEST_P(StreamingShellTest, LoadBitOnMetadataPrefixFailsCleanly) {
    const std::vector<ktx_uint8_t>& file = fileFor(GetParam());
    ASSERT_FALSE(file.empty());
    const FileMap m = parseFileMap(file);

    BoundedStreamState state = {file.data(), (ktx_size_t)m.prefixLen, 0, false};
    ktxStream stream = makeBoundedStream(&state);

    ktxTexture2* texture = nullptr;
    KTX_error_code result = ktxTexture2_CreateFromStream(
        &stream, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);
    EXPECT_EQ(result, KTX_FILE_UNEXPECTED_EOF) << ktxErrorString(result);
    EXPECT_EQ(texture, nullptr);
}

TEST_P(StreamingShellTest, LateLoadImageDataOnShellFailsCleanly) {
    const std::vector<ktx_uint8_t>& file = fileFor(GetParam());
    ASSERT_FALSE(file.empty());
    const FileMap m = parseFileMap(file);

    BoundedStreamState state = {file.data(), (ktx_size_t)m.prefixLen, 0, false};
    ktxStream stream = makeBoundedStream(&state);

    ktxTexture2* shell = nullptr;
    KTX_error_code result = ktxTexture2_CreateFromStream(&stream, 0, &shell);
    ASSERT_EQ(result, KTX_SUCCESS) << ktxErrorString(result);

    // The caller buffer must hold the inflated size: Zstd inflates on load.
    const ktx_size_t size = ktxTexture2_GetDataSizeUncompressed(shell);
    std::vector<ktx_uint8_t> buffer(size);
    result = ktxTexture2_LoadImageData(shell, buffer.data(), size);
    EXPECT_EQ(result, KTX_FILE_UNEXPECTED_EOF) << ktxErrorString(result);

    ktxTexture_Destroy(ktxTexture(shell));
}

TEST_P(StreamingShellTest, TruncatedMetadataFailsCleanly) {
    const std::vector<ktx_uint8_t>& file = fileFor(GetParam());
    ASSERT_FALSE(file.empty());
    const FileMap m = parseFileMap(file);

    // One byte into the last metadata section: mid-SGD for BasisLZ,
    // mid-KVD for the UASTC variants.
    BoundedStreamState state = {file.data(), (ktx_size_t)m.lastMetaByte - 1, 0,
                                false};
    ktxStream stream = makeBoundedStream(&state);

    ktxTexture2* texture = nullptr;
    KTX_error_code result = ktxTexture2_CreateFromStream(&stream, 0, &texture);
    EXPECT_NE(result, KTX_SUCCESS);
    EXPECT_EQ(texture, nullptr);
}

TEST_P(StreamingShellTest, PrefixShortByOneByte) {
    const std::vector<ktx_uint8_t>& file = fileFor(GetParam());
    ASSERT_FALSE(file.empty());
    const FileMap m = parseFileMap(file);

    // With scheme 0, alignment padding sits between the last metadata
    // section and the first level, so a prefix one byte short may still
    // construct — the padding is never read. Only cleanliness is asserted:
    // either success without over-read, or a clean error.
    BoundedStreamState state = {file.data(), (ktx_size_t)m.prefixLen - 1, 0,
                                false};
    ktxStream stream = makeBoundedStream(&state);

    ktxTexture2* texture = nullptr;
    KTX_error_code result = ktxTexture2_CreateFromStream(&stream, 0, &texture);
    if (result == KTX_SUCCESS) {
        EXPECT_FALSE(state.overRead);
        ktxTexture_Destroy(ktxTexture(texture));
    } else {
        EXPECT_EQ(texture, nullptr);
    }
}

INSTANTIATE_TEST_SUITE_P(
    Transcode, StreamingShellTest,
    ::testing::Values(Variant::Etc1sBasisLz, Variant::UastcZstd,
                      Variant::UastcRaw),
    [](const ::testing::TestParamInfo<Variant>& info) {
        return variantName(info.param);
    });

}  // namespace

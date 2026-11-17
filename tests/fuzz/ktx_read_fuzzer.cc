// Copyright 2026 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

// libFuzzer target for reading KTX and KTX2 files with libktx.
//
// Each input is loaded the way an application would load an untrusted file:
// create the texture, load the image data, transcode Basis Universal data and
// read every byte libktx reports. Build it with Clang and the address and
// undefined behavior sanitizers; see README.md.

#include <cstddef>
#include <cstdint>

#include <ktx.h>

namespace {

// Textures larger than this are rejected before their image data is loaded,
// so the fuzzer finds bugs rather than out-of-memory reports on headers
// that validly declare huge textures.
constexpr ktx_size_t kMaxDataSize = 64 * 1024 * 1024;
constexpr ktx_uint64_t kMaxTexels = 16 * 1024 * 1024;

// Transcode targets. One is picked per input so that the block encoders of
// the transcoder are all reached without transcoding every input many times.
constexpr ktx_transcode_fmt_e kTranscodeTargets[] = {
    KTX_TTF_ETC1_RGB,     KTX_TTF_ETC2_RGBA,
    KTX_TTF_BC1_RGB,      KTX_TTF_BC3_RGBA,
    KTX_TTF_BC4_R,        KTX_TTF_BC5_RG,
    KTX_TTF_BC7_RGBA,     KTX_TTF_ASTC_4x4_RGBA,
    KTX_TTF_RGBA32,       KTX_TTF_RGB565,
    KTX_TTF_ETC2_EAC_R11, KTX_TTF_PVRTC1_4_RGB,
    KTX_TTF_BC6HU,        KTX_TTF_ASTC_HDR_4x4_RGBA,
    KTX_TTF_RGBA_HALF,
};
constexpr size_t kTranscodeTargetCount = sizeof(kTranscodeTargets) / sizeof(kTranscodeTargets[0]);

bool IsTooLarge(ktxTexture* texture) {
    const ktx_uint64_t texels =
        ktx_uint64_t{texture->baseWidth} * (texture->baseHeight ? texture->baseHeight : 1) *
        (texture->baseDepth ? texture->baseDepth : 1) * texture->numLayers * texture->numFaces;
    // Both sizes come from the header: the size of the data as stored, which
    // libktx allocates to read it, and its size after inflation.
    return texels > kMaxTexels || ktxTexture_GetDataSize(texture) > kMaxDataSize ||
           ktxTexture_GetDataSizeUncompressed(texture) > kMaxDataSize;
}

// Reads every byte of the image data, so that a data size larger than the
// buffer it describes is reported by the address sanitizer here.
void ReadAllData(ktxTexture* texture) {
    const ktx_uint8_t* data = ktxTexture_GetData(texture);
    const ktx_size_t size = ktxTexture_GetDataSize(texture);
    if (data == nullptr) {
        return;
    }
    unsigned sum = 0;
    for (ktx_size_t i = 0; i < size; ++i) {
        sum += data[i];
    }
    volatile unsigned sink = sum;
    (void)sink;
}

KTX_error_code ReadImage(int, int, int, int, int, ktx_uint64_t size, void* pixels, void*) {
    const ktx_uint8_t* bytes = static_cast<const ktx_uint8_t*>(pixels);
    unsigned sum = 0;
    for (ktx_uint64_t i = 0; i < size; ++i) {
        sum += bytes[i];
    }
    volatile unsigned sink = sum;
    (void)sink;
    return KTX_SUCCESS;
}

// Reads the image data level by level, as an uploader would. Only for data
// that is not supercompressed: the iteration sizes each level from the format.
void ReadLevels(ktxTexture* texture) {
    (void)ktxTexture_IterateLevelFaces(texture, ReadImage, nullptr);
}

void ExerciseKtx2(ktxTexture2* texture, ktx_transcode_fmt_e target) {
    (void)ktxTexture2_GetNumComponents(texture);
    (void)ktxTexture2_GetOETF_e(texture);
    (void)ktxTexture2_GetPremultipliedAlpha(texture);
    if (ktxTexture2_NeedsTranscoding(texture)) {
        if (ktxTexture2_TranscodeBasis(texture, target, 0) != KTX_SUCCESS) {
            return;
        }
        ReadAllData(ktxTexture(texture));
    }
    if (texture->supercompressionScheme == KTX_SS_NONE) {
        ReadLevels(ktxTexture(texture));
    }
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    ktxTexture* texture = nullptr;
    if (ktxTexture_CreateFromMemory(data, size, KTX_TEXTURE_CREATE_NO_FLAGS, &texture) !=
        KTX_SUCCESS) {
        return 0;
    }
    if (!IsTooLarge(texture) && ktxTexture_LoadImageData(texture, nullptr, 0) == KTX_SUCCESS) {
        ReadAllData(texture);
        if (texture->classId == ktxTexture2_c) {
            ExerciseKtx2(reinterpret_cast<ktxTexture2*>(texture),
                         kTranscodeTargets[size % kTranscodeTargetCount]);
        } else {
            ReadLevels(texture);
        }
    }
    ktxTexture_Destroy(texture);
    return 0;
}

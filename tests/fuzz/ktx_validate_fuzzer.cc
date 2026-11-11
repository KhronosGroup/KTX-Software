// Copyright 2026 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

// libFuzzer target for the KTX2 validator of the ktx tool.
//
// validateMemory runs the checks of ktx validate, and of ktx info and the
// other commands that validate their input, on each input. The validator
// reads the header and the blocks it describes itself before creating the
// texture with libktx, so ktx_read_fuzzer does not reach it. See README.md.

#include <cstddef>
#include <cstdint>

#include <ktx.h>

#include "validate.h"

namespace {

// After its own checks the validator creates the texture with libktx, which
// allocates the image data the header declares, and transcodes it. As in
// ktx_read_fuzzer, inputs that libktx accepts and that declare more data or
// texels than this are skipped, so that the fuzzer finds bugs rather than
// out-of-memory reports on headers that validly declare huge textures.
constexpr ktx_size_t kMaxDataSize = 64 * 1024 * 1024;
constexpr ktx_uint64_t kMaxTexels = 16 * 1024 * 1024;

bool IsTooLarge(const uint8_t* data, size_t size) {
    ktxTexture* texture = nullptr;
    if (ktxTexture_CreateFromMemory(data, size, KTX_TEXTURE_CREATE_NO_FLAGS, &texture) !=
        KTX_SUCCESS) {
        return false;
    }
    const ktx_uint64_t texels =
        ktx_uint64_t{texture->baseWidth} * (texture->baseHeight ? texture->baseHeight : 1) *
        (texture->baseDepth ? texture->baseDepth : 1) * texture->numLayers * texture->numFaces;
    const bool tooLarge = texels > kMaxTexels || ktxTexture_GetDataSize(texture) > kMaxDataSize ||
                          ktxTexture_GetDataSizeUncompressed(texture) > kMaxDataSize;
    ktxTexture_Destroy(texture);
    return tooLarge;
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (IsTooLarge(data, size)) {
        return 0;
    }
    // The glTF checks of --gltf-basisu add to the others, so run them for
    // every other input.
    const bool gltfBasisU = size % 2 != 0;
    (void)ktx::validateMemory(reinterpret_cast<const char*>(data), size,
                              false, gltfBasisU,
                              [](const ktx::ValidationReport&) {});
    return 0;
}

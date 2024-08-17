// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 expandtab:

// Copyright 2023-2024 The Khronos Group Inc.
// Copyright 2023-2024 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

//!
//! @internal
//! @~English
//! @file
//!
//! @brief Internal Image Codec class
//!

#pragma once

#include <algorithm>
#include <functional>
#include <limits>
#include <type_traits>
#include <map>
#include <vector>
#include <assert.h>
#ifdef _MSC_VER
  #pragma warning(push)
  #pragma warning(disable : 4201)
#endif
#include <glm/gtc/packing.hpp>
#ifdef _MSC_VER
  #pragma warning(pop)
#endif
#include "imageio_utility.h"
#include "vkformat_enum.h"

class ImageCodec {
public:
    struct TexelBlockCodec {
    };

    ImageCodec() {
        flags.valid = false;
        flags.isBlockCompressed = false;
        flags.isPacked = false;
        flags.isFloat = false;
        flags.isFloatHalf = false;
        flags.isSigned = false;
        flags.isNormalized = false;
    }

    ImageCodec(VkFormat vkFormat, uint32_t typeSize, const uint32_t* dfd) {
        flags.valid = true;
        flags.isBlockCompressed = false;
        flags.isPacked = false;
        flags.isFloat = false;
        flags.isFloatHalf = false;
        flags.isSigned = false;
        flags.isNormalized = false;

        const auto* bdfd = dfd + 1;
        const auto model = khr_df_model_e(KHR_DFDVAL(bdfd, MODEL));
        texelBlockDimensions[0] = KHR_DFDVAL(bdfd, TEXELBLOCKDIMENSION0) + 1;
        texelBlockDimensions[1] = KHR_DFDVAL(bdfd, TEXELBLOCKDIMENSION1) + 1;
        texelBlockDimensions[2] = KHR_DFDVAL(bdfd, TEXELBLOCKDIMENSION2) + 1;
        texelBlockDimensions[3] = KHR_DFDVAL(bdfd, TEXELBLOCKDIMENSION3) + 1;

        packedElementByteSize = typeSize;

        // Packed element size must always be 1, 2, or 4 currently
        // (for block compressed formats the element size is considered 1 by convention)
        switch (packedElementByteSize) {
        case 1: codec.getPackedElement = getPackedElement<uint8_t>; break;
        case 2: codec.getPackedElement = getPackedElement<uint16_t>; break;
        case 4: codec.getPackedElement = getPackedElement<uint32_t>; break;
        default: flags.valid = false; return;
        }

        // We initialize the packed element count here based on the first plane's size and the element size
        const auto firstPlaneBytes = KHR_DFDVAL(bdfd, BYTESPLANE0);
        packedElementCount = firstPlaneBytes / packedElementByteSize;
        // We can do the above because we do not currently support multiple planes
        if (KHR_DFDVAL(bdfd, BYTESPLANE1) != 0) {
            flags.valid = false;
            return;
        }

        // If packedElementCount is zero, then there's something wrong with bytesPlane0 being zero
        if (packedElementCount == 0) {
            flags.valid = false;
            return;
        }

        // By default we do not have directly accessible channels
        // (e.g. for block compressed we can only access packed data)
        channels = 0;

        switch (model) {
        case KHR_DF_MODEL_RGBSDA: [[fallthrough]];
        case KHR_DF_MODEL_YUVSDA: [[fallthrough]];
        case KHR_DF_MODEL_YIQSDA: [[fallthrough]];
        case KHR_DF_MODEL_LABSDA: [[fallthrough]];
        case KHR_DF_MODEL_CMYKA: [[fallthrough]];
        case KHR_DF_MODEL_XYZW: [[fallthrough]];
        case KHR_DF_MODEL_HSVA_ANG: [[fallthrough]];
        case KHR_DF_MODEL_HSLA_ANG: [[fallthrough]];
        case KHR_DF_MODEL_HSVA_HEX: [[fallthrough]];
        case KHR_DF_MODEL_HSLA_HEX: [[fallthrough]];
        case KHR_DF_MODEL_YCGCOA: [[fallthrough]];
        case KHR_DF_MODEL_YCCBCCRC: [[fallthrough]];
        case KHR_DF_MODEL_ICTCP: [[fallthrough]];
        case KHR_DF_MODEL_CIEXYZ: [[fallthrough]];
        case KHR_DF_MODEL_CIEXYY:
            // These color models are handled as simple per-channel texel blocks
            switch (vkFormat) {
            case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
                flags.isFloat = true;
                channels = 3;
                codec.decodeFLOAT = decodeFLOAT_E9B5G5R5;
                break;

            case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
                channels = 3;
                codec.decodeFLOAT = decodeFLOAT_B10G11R11;
                break;

            case VK_FORMAT_R16G16_SFIXED5_NV:
                channels = 2;
                codec.decodeFLOAT = decodeFLOAT_SFIXED5_NV<2>;
                break;

            case VK_FORMAT_D16_UNORM_S8_UINT:
                flags.isNormalized = true;
                channels = 2;
                codec.decodeUINT = decodeUINT_D16_S8;
                codec.decodeFLOAT = decodeFLOAT_D16_S8;
                break;

            case VK_FORMAT_X8_D24_UNORM_PACK32:
                flags.isNormalized = true;
                channels = 1;
                codec.decodeUINT = decodeUINT_D24;
                codec.decodeFLOAT = decodeFLOAT_D24;
                break;

            case VK_FORMAT_D24_UNORM_S8_UINT:
                flags.isNormalized = true;
                channels = 2;
                codec.decodeUINT = decodeUINT_D24_S8;
                codec.decodeFLOAT = decodeFLOAT_D24_S8;
                break;

            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                channels = 2;
                codec.decodeFLOAT = decodeFLOAT_D32_S8;
                break;

            default: {
                // For other formats we only support cases where the number formats match across the samples
                const auto sampleCount = KHR_DFDSAMPLECOUNT(bdfd);
                const auto firstDataType = KHR_DFDSVAL(bdfd, 0, QUALIFIERS) & ~KHR_DF_SAMPLE_DATATYPE_LINEAR;
                const auto firstBitLength = KHR_DFDSVAL(bdfd, 0, BITLENGTH) + 1;
                const auto sampleUpper = KHR_DFDSVAL(bdfd, 0, SAMPLEUPPER);
                flags.isFloat = (firstDataType & KHR_DF_SAMPLE_DATATYPE_FLOAT) != 0;
                flags.isFloatHalf = flags.isFloat && (firstBitLength == 16);
                flags.isSigned = (firstDataType & KHR_DF_SAMPLE_DATATYPE_SIGNED) != 0;
                flags.isNormalized = sampleUpper != (flags.isFloat ? imageio::bit_cast<uint32_t>(1.f) : 1u);

                // Channel count matches sample count for these cases
                channels = sampleCount;

                if (firstDataType & KHR_DF_SAMPLE_DATATYPE_EXPONENT) {
                    // No other shared exponent formats are supported other than the ones handled explicitly
                    flags.valid = false;
                    return;
                }
                for (uint32_t i = 0; i < sampleCount; ++i) {
                    const auto dataType = KHR_DFDSVAL(bdfd, i, QUALIFIERS) & ~KHR_DF_SAMPLE_DATATYPE_LINEAR;
                    const auto bitLength = KHR_DFDSVAL(bdfd, i, BITLENGTH) + 1;
                    // If not all elements match the packed element byte size then this is a packed format
                    if (bitLength != firstBitLength || bitLength != packedElementByteSize * 8) {
                        flags.isPacked = true;
                    }
                    // We do not support mixed component types here as all special cases are handled outside
                    if (dataType != firstDataType) {
                        flags.valid = false;
                        return;
                    }
                    // For floats we also require either 32-bit or 16-bit float as all other
                    // special cases are currently handled outside
                    if (flags.isFloat && (bitLength != 16) && (bitLength != 32)) {
                        flags.valid = false;
                        return;
                    }
                }

                if (flags.isFloatHalf) {
                    // Data is a vector of FP16 values
                    switch (sampleCount) {
                    case 1: codec.decodeFLOAT = decodeFLOAT_FP16Vec<1>; break;
                    case 2: codec.decodeFLOAT = decodeFLOAT_FP16Vec<2>; break;
                    case 3: codec.decodeFLOAT = decodeFLOAT_FP16Vec<3>; break;
                    case 4: codec.decodeFLOAT = decodeFLOAT_FP16Vec<4>; break;
                    default: flags.valid = false; return;
                    }
                } else if (flags.isFloat) {
                    // Data is a vector of FP32 values
                    switch (sampleCount) {
                    case 1: codec.decodeFLOAT = decodeFLOAT_FP32Vec<1>; break;
                    case 2: codec.decodeFLOAT = decodeFLOAT_FP32Vec<2>; break;
                    case 3: codec.decodeFLOAT = decodeFLOAT_FP32Vec<3>; break;
                    case 4: codec.decodeFLOAT = decodeFLOAT_FP32Vec<4>; break;
                    default: flags.valid = false; return;
                    }
                } else if (flags.isPacked) {
                    // Data is packed so use the more general decoders
                    for (uint32_t i = 0; i < sampleCount; ++i) {
                        const auto bitOffset = KHR_DFDSVAL(bdfd, i, BITOFFSET);
                        const auto bitLength = KHR_DFDSVAL(bdfd, i, BITLENGTH) + 1;

                        SampleInfo sampleInfo{};
                        sampleInfo.elementIndex = (bitOffset >> 3) / packedElementByteSize;
                        sampleInfo.bitOffset = bitOffset % (packedElementByteSize << 3);
                        sampleInfo.bitLength = bitLength;

                        // If any of the samples straddle the packed elements then there's something wrong
                        if (sampleInfo.bitOffset + sampleInfo.bitLength > (packedElementByteSize << 3)) {
                            flags.valid = false;
                            return;
                        }

                        packedSampleInfo.emplace_back(sampleInfo);
                    }

                    switch (packedElementByteSize) {
                    case 1:
                        // 8-bit packed elements
                        if (flags.isSigned) {
                            codec.decodeSINT = decodeSINT_SINTPacked<int8_t>;
                            if (flags.isNormalized)
                                codec.decodeFLOAT = decodeFLOAT_SINTPacked<int8_t>;
                        } else {
                            codec.decodeUINT = decodeUINT_UINTPacked<uint8_t>;
                            if (flags.isNormalized)
                                codec.decodeFLOAT = decodeFLOAT_UINTPacked<uint8_t>;
                        }
                        break;
                    case 2:
                        // 16-bit packed elements
                        if (flags.isSigned) {
                            codec.decodeSINT = decodeSINT_SINTPacked<int16_t>;
                            if (flags.isNormalized)
                                codec.decodeFLOAT = decodeFLOAT_SINTPacked<int16_t>;
                        } else {
                            codec.decodeUINT = decodeUINT_UINTPacked<uint16_t>;
                            if (flags.isNormalized)
                                codec.decodeFLOAT = decodeFLOAT_UINTPacked<uint16_t>;
                        }
                        break;
                    case 4:
                        // 32-bit packed elements
                        if (flags.isSigned) {
                            codec.decodeSINT = decodeSINT_SINTPacked<int32_t>;
                            if (flags.isNormalized)
                                codec.decodeFLOAT = decodeFLOAT_SINTPacked<int32_t>;
                        } else {
                            codec.decodeUINT = decodeUINT_UINTPacked<uint32_t>;
                            if (flags.isNormalized)
                                codec.decodeFLOAT = decodeFLOAT_UINTPacked<uint32_t>;
                        }
                        break;
                    default:
                        flags.valid = false;
                        return;
                    }
                } else {
                    // Data is not packed so we can use the optimized decoders
                    if (flags.isSigned) {
                        switch (packedElementByteSize) {
                        case 1:
                            // 8-bit signed integer
                            switch (sampleCount) {
                            case 1: codec.decodeSINT = decodeSINT_SINTVec<int8_t, 1>; break;
                            case 2: codec.decodeSINT = decodeSINT_SINTVec<int8_t, 2>; break;
                            case 3: codec.decodeSINT = decodeSINT_SINTVec<int8_t, 3>; break;
                            case 4: codec.decodeSINT = decodeSINT_SINTVec<int8_t, 4>; break;
                            default: flags.valid = false; return;
                            }
                            if (flags.isNormalized) {
                                switch (sampleCount) {
                                case 1: codec.decodeFLOAT = decodeFLOAT_SINTVec<int8_t, 1>; break;
                                case 2: codec.decodeFLOAT = decodeFLOAT_SINTVec<int8_t, 2>; break;
                                case 3: codec.decodeFLOAT = decodeFLOAT_SINTVec<int8_t, 3>; break;
                                case 4: codec.decodeFLOAT = decodeFLOAT_SINTVec<int8_t, 4>; break;
                                default: break;
                                }
                            }
                            break;
                        case 2:
                            // 16-bit signed integer
                            switch (sampleCount) {
                            case 1: codec.decodeSINT = decodeSINT_SINTVec<int16_t, 1>; break;
                            case 2: codec.decodeSINT = decodeSINT_SINTVec<int16_t, 2>; break;
                            case 3: codec.decodeSINT = decodeSINT_SINTVec<int16_t, 3>; break;
                            case 4: codec.decodeSINT = decodeSINT_SINTVec<int16_t, 4>; break;
                            default: flags.valid = false; return;
                            }
                            if (flags.isNormalized) {
                                switch (sampleCount) {
                                case 1: codec.decodeFLOAT = decodeFLOAT_SINTVec<int16_t, 1>; break;
                                case 2: codec.decodeFLOAT = decodeFLOAT_SINTVec<int16_t, 2>; break;
                                case 3: codec.decodeFLOAT = decodeFLOAT_SINTVec<int16_t, 3>; break;
                                case 4: codec.decodeFLOAT = decodeFLOAT_SINTVec<int16_t, 4>; break;
                                default: break;
                                }
                            }
                            break;
                        case 4:
                            // 32-bit signed integer
                            switch (sampleCount) {
                            case 1: codec.decodeSINT = decodeSINT_SINTVec<int32_t, 1>; break;
                            case 2: codec.decodeSINT = decodeSINT_SINTVec<int32_t, 2>; break;
                            case 3: codec.decodeSINT = decodeSINT_SINTVec<int32_t, 3>; break;
                            case 4: codec.decodeSINT = decodeSINT_SINTVec<int32_t, 4>; break;
                            default: flags.valid = false; return;
                            }
                            if (flags.isNormalized) {
                                switch (sampleCount) {
                                case 1: codec.decodeFLOAT = decodeFLOAT_SINTVec<int32_t, 1>; break;
                                case 2: codec.decodeFLOAT = decodeFLOAT_SINTVec<int32_t, 2>; break;
                                case 3: codec.decodeFLOAT = decodeFLOAT_SINTVec<int32_t, 3>; break;
                                case 4: codec.decodeFLOAT = decodeFLOAT_SINTVec<int32_t, 4>; break;
                                default: break;
                                }
                            }
                            break;
                        default:
                            flags.valid = false;
                            return;
                        }
                    } else {
                        switch (packedElementByteSize) {
                        case 1:
                            // 8-bit unsigned integer
                            switch (sampleCount) {
                            case 1: codec.decodeUINT = decodeUINT_UINTVec<uint8_t, 1>; break;
                            case 2: codec.decodeUINT = decodeUINT_UINTVec<uint8_t, 2>; break;
                            case 3: codec.decodeUINT = decodeUINT_UINTVec<uint8_t, 3>; break;
                            case 4: codec.decodeUINT = decodeUINT_UINTVec<uint8_t, 4>; break;
                            default: flags.valid = false; return;
                            }
                            if (flags.isNormalized) {
                                switch (sampleCount) {
                                case 1: codec.decodeFLOAT = decodeFLOAT_UINTVec<uint8_t, 1>; break;
                                case 2: codec.decodeFLOAT = decodeFLOAT_UINTVec<uint8_t, 2>; break;
                                case 3: codec.decodeFLOAT = decodeFLOAT_UINTVec<uint8_t, 3>; break;
                                case 4: codec.decodeFLOAT = decodeFLOAT_UINTVec<uint8_t, 4>; break;
                                default: break;
                                }
                            }
                            break;
                        case 2:
                            // 16-bit unsigned integer
                            switch (sampleCount) {
                            case 1: codec.decodeUINT = decodeUINT_UINTVec<uint16_t, 1>; break;
                            case 2: codec.decodeUINT = decodeUINT_UINTVec<uint16_t, 2>; break;
                            case 3: codec.decodeUINT = decodeUINT_UINTVec<uint16_t, 3>; break;
                            case 4: codec.decodeUINT = decodeUINT_UINTVec<uint16_t, 4>; break;
                            default: flags.valid = false; return;
                            }
                            if (flags.isNormalized) {
                                switch (sampleCount) {
                                case 1: codec.decodeFLOAT = decodeFLOAT_UINTVec<uint16_t, 1>; break;
                                case 2: codec.decodeFLOAT = decodeFLOAT_UINTVec<uint16_t, 2>; break;
                                case 3: codec.decodeFLOAT = decodeFLOAT_UINTVec<uint16_t, 3>; break;
                                case 4: codec.decodeFLOAT = decodeFLOAT_UINTVec<uint16_t, 4>; break;
                                default: break;
                                }
                            }
                            break;
                        case 4:
                            // 32-bit unsigned integer
                            switch (sampleCount) {
                            case 1: codec.decodeUINT = decodeUINT_UINTVec<uint32_t, 1>; break;
                            case 2: codec.decodeUINT = decodeUINT_UINTVec<uint32_t, 2>; break;
                            case 3: codec.decodeUINT = decodeUINT_UINTVec<uint32_t, 3>; break;
                            case 4: codec.decodeUINT = decodeUINT_UINTVec<uint32_t, 4>; break;
                            default: flags.valid = false; return;
                            }
                            if (flags.isNormalized) {
                                switch (sampleCount) {
                                case 1: codec.decodeFLOAT = decodeFLOAT_UINTVec<uint32_t, 1>; break;
                                case 2: codec.decodeFLOAT = decodeFLOAT_UINTVec<uint32_t, 2>; break;
                                case 3: codec.decodeFLOAT = decodeFLOAT_UINTVec<uint32_t, 3>; break;
                                case 4: codec.decodeFLOAT = decodeFLOAT_UINTVec<uint32_t, 4>; break;
                                default: break;
                                }
                            }
                            break;
                        default:
                            flags.valid = false;
                            return;
                        }
                    }
                }
                break;
            }
            }
            break;

        case KHR_DF_MODEL_BC1A: [[fallthrough]];
        case KHR_DF_MODEL_BC2: [[fallthrough]];
        case KHR_DF_MODEL_BC3: [[fallthrough]];
        case KHR_DF_MODEL_BC4: [[fallthrough]];
        case KHR_DF_MODEL_BC5: [[fallthrough]];
        case KHR_DF_MODEL_BC6H: [[fallthrough]];
        case KHR_DF_MODEL_BC7: [[fallthrough]];
        case KHR_DF_MODEL_ETC1: [[fallthrough]];
        case KHR_DF_MODEL_ETC2: [[fallthrough]];
        case KHR_DF_MODEL_ASTC: [[fallthrough]];
        case KHR_DF_MODEL_PVRTC: [[fallthrough]];
        case KHR_DF_MODEL_PVRTC2:
            // These color models are handled is raw compressed blocks
            flags.isBlockCompressed = true;
            break;

        case KHR_DF_MODEL_UASTC:
            // UASTC needs special handling
            packedElementByteSize = 1;
            packedElementCount = 16;
            flags.isBlockCompressed = true;
            break;

        case KHR_DF_MODEL_ETC1S:
            // ETC1S (as used by BasisLZ) is not supported directly
            flags.valid = false;
            return;

        default:
            flags.valid = false;
            return;
        }

        texelBlockByteSize = packedElementByteSize * packedElementCount;

        // If we couldn't determine the texel block size then something went wrong.
        if (texelBlockByteSize == 0) {
            flags.valid = false;
            return;
        }
    }

    operator bool() const { return flags.valid; }

    constexpr bool isBlockCompressed() const { return flags.isBlockCompressed; }
    constexpr bool isPacked() const { return flags.isPacked; }
    constexpr bool isFloat() const { return flags.isFloat; }
    constexpr bool isFloatHalf() const { return flags.isFloatHalf; }
    constexpr bool isSigned() const { return flags.isSigned; }
    constexpr bool isNormalized() const { return flags.isNormalized; }
    constexpr bool canDecodeUINT() const { return codec.decodeUINT != nullptr; }
    constexpr bool canDecodeSINT() const { return codec.decodeSINT != nullptr; }
    constexpr bool canDecodeFLOAT() const { return codec.decodeFLOAT != nullptr; }

    glm::uvec4 getTexelBlockDimensions() const { return texelBlockDimensions; }
    constexpr uint32_t getPackedElementByteSize() const { return packedElementByteSize; }
    constexpr uint32_t getPackedElementCount() const { return packedElementCount; }
    constexpr uint32_t getTexelBlockByteSize() const { return texelBlockByteSize; }
    constexpr uint32_t getChannelCount() const { return channels; }

    glm::uvec4 pixelToTexelBlockSize(glm::uvec4 pixelSize) const {
        return (pixelSize + texelBlockDimensions - glm::uvec4(1, 1, 1, 1)) / texelBlockDimensions;
    }

    uint32_t getPackedElement(const void* ptr, uint32_t index) const { return codec.getPackedElement(this, ptr, index); }
    glm::uvec4 decodeUINT(const void* ptr) const { return codec.decodeUINT(this, ptr); }
    glm::ivec4 decodeSINT(const void* ptr) const { return codec.decodeSINT(this, ptr); }
    glm::vec4 decodeFLOAT(const void* ptr) const { return codec.decodeFLOAT(this, ptr); }

private:
    struct {
        uint32_t valid : 1;
        uint32_t isBlockCompressed : 1;
        uint32_t isPacked : 1;
        uint32_t isFloat : 1;
        uint32_t isFloatHalf : 1;
        uint32_t isSigned : 1;
        uint32_t isNormalized : 1;
    } flags;

    glm::uvec4 texelBlockDimensions = glm::uvec4(0, 0, 0, 0);
    uint32_t packedElementByteSize = 0;
    uint32_t packedElementCount = 0;
    uint32_t texelBlockByteSize = 0;
    uint32_t channels = 0;

    struct SampleInfo {
        uint32_t elementIndex;
        uint32_t bitOffset;
        uint32_t bitLength;
    };
    std::vector<SampleInfo> packedSampleInfo = {};

    typedef uint32_t (*GetPackedElement)(const ImageCodec*, const void*, uint32_t);
    typedef glm::uvec4 (*DecodeUINT)(const ImageCodec*, const void*);
    typedef glm::ivec4 (*DecodeSINT)(const ImageCodec*, const void*);
    typedef glm::vec4 (*DecodeFLOAT)(const ImageCodec*, const void*);

    struct {
        GetPackedElement getPackedElement = nullptr;
        DecodeUINT  decodeUINT = nullptr;
        DecodeSINT  decodeSINT = nullptr;
        DecodeFLOAT decodeFLOAT = nullptr;
    } codec = {};

    template <typename TYPE>
    static uint32_t getPackedElement(const ImageCodec* codec, const void* ptr, uint32_t index) {
        static_assert(std::is_unsigned_v<TYPE>);
        (void)codec; // silences unused parameter warnings in release builds
        assert(sizeof(TYPE) == codec->getPackedElementByteSize());
        auto data = reinterpret_cast<const TYPE*>(ptr);
        return data[index];
    }

    static glm::vec4 decodeFLOAT_E9B5G5R5(const ImageCodec*, const void* ptr) {
        auto data = reinterpret_cast<const uint32_t*>(ptr);
        return glm::vec4(glm::unpackF3x9_E1x5(data[0]), 1.f);
    }

    static glm::vec4 decodeFLOAT_B10G11R11(const ImageCodec*, const void* ptr) {
        auto data = reinterpret_cast<const uint32_t*>(ptr);
        auto value = glm::unpackF2x11_1x10(data[0]);
        // Need to handle NaN and infinity as special cases, because GLM "swallows" them
        const uint32_t exponentShifts[] = { 6, 11 + 6, 22 + 5 };
        const uint32_t exponentMask = 0x1F;
        const uint32_t mantissaShifts[] = { 0, 11, 22 };
        const uint32_t mantissaMasks[] = { 0x3F, 0x3F, 0x1F };
        for (uint32_t channel = 0; channel < 3; ++channel) {
            const uint32_t exponent = (data[0] >> exponentShifts[channel]) & exponentMask;
            const uint32_t mantissa = (data[0] >> mantissaShifts[channel]) & mantissaMasks[channel];
            if (exponent == 31) {
                if (mantissa == 0) {
                    value[channel] = std::numeric_limits<float>::infinity();
                } else {
                    value[channel] = std::numeric_limits<float>::quiet_NaN();
                }
            }
        }
        return glm::vec4(value, 1.f);
    }

    static glm::uvec4 decodeUINT_D16_S8(const ImageCodec*, const void* ptr) {
        auto data = reinterpret_cast<const uint16_t*>(ptr);
        return glm::uvec4(data[0], data[1] & 0xFF, 0, 0);
    }

    static glm::vec4 decodeFLOAT_D16_S8(const ImageCodec*, const void* ptr) {
        auto data = reinterpret_cast<const uint16_t*>(ptr);
        return glm::vec4(imageio::convertUNORMToFloat(data[0], 16), static_cast<float>(data[1] & 0xFF), 0.f, 1.f);
    }

    static glm::uvec4 decodeUINT_D24(const ImageCodec*, const void* ptr) {
        auto data = reinterpret_cast<const uint32_t*>(ptr);
        return glm::uvec4(data[0] & 0xFFFFFF, 0, 0, 0);
    }

    static glm::vec4 decodeFLOAT_D24(const ImageCodec*, const void* ptr) {
        auto data = reinterpret_cast<const uint32_t*>(ptr);
        return glm::vec4(imageio::convertUNORMToFloat(data[0] & 0xFFFFFF, 24), 0.f, 0.f, 1.f);
    }

    static glm::uvec4 decodeUINT_D24_S8(const ImageCodec*, const void* ptr) {
        auto data = reinterpret_cast<const uint32_t*>(ptr);
        return glm::uvec4(data[0] >> 8, data[0] & 0xFF, 0, 0);
    }

    static glm::vec4 decodeFLOAT_D24_S8(const ImageCodec*, const void* ptr) {
        auto data = reinterpret_cast<const uint32_t*>(ptr);
        return glm::vec4(imageio::convertUNORMToFloat(data[0] >> 8, 24), static_cast<float>(data[0] & 0xFF), 0.f, 1.f);
    }

    static glm::vec4 decodeFLOAT_D32_S8(const ImageCodec*, const void* ptr) {
        auto data_float = reinterpret_cast<const float*>(ptr);
        auto data_uint = reinterpret_cast<const uint32_t*>(ptr);
        return glm::vec4(data_float[0], static_cast<float>(data_uint[1] & 0xFF), 0.f, 1.f);
    }

    template <int COMPONENTS>
    static glm::vec4 decodeFLOAT_FP32Vec(const ImageCodec*, const void* ptr) {
        static_assert((COMPONENTS > 0) && (COMPONENTS <= 4));
        auto data = reinterpret_cast<const float*>(ptr);
        glm::vec4 result(0.f, 0.f, 0.f, 1.f);
        for (int i = 0; i < COMPONENTS; ++i)
            result[i] = data[i];
        return result;
    }

    template <int COMPONENTS>
    static glm::vec4 decodeFLOAT_FP16Vec(const ImageCodec*, const void* ptr) {
        static_assert((COMPONENTS > 0) && (COMPONENTS <= 4));
        auto data = reinterpret_cast<const uint16_t*>(ptr);
        glm::vec4 result(0.f, 0.f, 0.f, 1.f);
        for (int i = 0; i < COMPONENTS; ++i)
            result[i] = imageio::half_to_float(data[i]);
        return result;
    }

    template <int COMPONENTS>
    static glm::vec4 decodeFLOAT_SFIXED5_NV(const ImageCodec*, const void* ptr) {
        static_assert((COMPONENTS > 0) && (COMPONENTS <= 4));
        auto data = reinterpret_cast<const int16_t*>(ptr);
        glm::vec4 result(0.f, 0.f, 0.f, 1.f);
        for (int i = 0; i < COMPONENTS; ++i)
            result[i] = data[i] / 32.f;
        return result;
    }

    template <typename TYPE, int COMPONENTS>
    static glm::uvec4 decodeUINT_UINTVec(const ImageCodec*, const void* ptr) {
        static_assert(std::is_unsigned_v<TYPE>);
        static_assert((COMPONENTS > 0) && (COMPONENTS <= 4));
        auto data = reinterpret_cast<const TYPE*>(ptr);
        glm::uvec4 result(0, 0, 0, 0);
        for (int i = 0; i < COMPONENTS; ++i)
            result[i] = data[i];
        return result;
    }

    template <typename TYPE, int COMPONENTS>
    static glm::vec4 decodeFLOAT_UINTVec(const ImageCodec*, const void* ptr) {
        static_assert(std::is_unsigned_v<TYPE>);
        static_assert((COMPONENTS > 0) && (COMPONENTS <= 4));
        auto data = reinterpret_cast<const TYPE*>(ptr);
        const auto upper = static_cast<float>((256u << sizeof(TYPE)) - 1u);
        glm::vec4 result(0.f, 0.f, 0.f, 1.f);
        for (int i = 0; i < COMPONENTS; ++i)
            result[i] = static_cast<float>(data[i]) / upper;
        return result;
    }

    template <typename TYPE, int COMPONENTS>
    static glm::ivec4 decodeSINT_SINTVec(const ImageCodec*, const void* ptr) {
        static_assert(std::is_signed_v<TYPE>);
        static_assert((COMPONENTS > 0) && (COMPONENTS <= 4));
        auto data = reinterpret_cast<const TYPE*>(ptr);
        glm::ivec4 result(0, 0, 0, 0);
        for (int i = 0; i < COMPONENTS; ++i)
            result[i] = data[i];
        return result;
    }

    template <typename TYPE, int COMPONENTS>
    static glm::vec4 decodeFLOAT_SINTVec(const ImageCodec*, const void* ptr) {
        static_assert(std::is_signed_v<TYPE>);
        static_assert((COMPONENTS > 0) && (COMPONENTS <= 4));
        auto data = reinterpret_cast<const TYPE*>(ptr);
        const auto upper = static_cast<float>((128u << sizeof(TYPE)) - 1u);
        glm::vec4 result(0.f, 0.f, 0.f, 1.f);
        for (int i = 0; i < COMPONENTS; ++i)
            result[i] = std::max(static_cast<float>(data[i]) / upper, -1.f);
        return result;
    }

    template <typename TYPE>
    static glm::uvec4 decodeUINT_UINTPacked(const ImageCodec* codec, const void* ptr) {
        static_assert(std::is_unsigned_v<TYPE>);
        auto data = reinterpret_cast<const TYPE*>(ptr);
        glm::uvec4 result(0, 0, 0, 0);
        for (std::size_t i = 0; i < codec->packedSampleInfo.size(); ++i) {
            const auto& info = codec->packedSampleInfo[i];
            result[static_cast<glm::length_t>(i)] = (data[info.elementIndex] >> info.bitOffset) & ((1u << info.bitLength) - 1u);
        }
        return result;
    }

    template <typename TYPE>
    static glm::vec4 decodeFLOAT_UINTPacked(const ImageCodec* codec, const void* ptr) {
        static_assert(std::is_unsigned_v<TYPE>);
        auto data = reinterpret_cast<const TYPE*>(ptr);
        glm::vec4 result(0.f, 0.f, 0.f, 1.f);
        for (std::size_t i = 0; i < codec->packedSampleInfo.size(); ++i) {
            const auto& info = codec->packedSampleInfo[i];
            const auto upper = static_cast<float>((1u << info.bitLength) - 1u);
            uint32_t rawValue = (data[info.elementIndex] >> info.bitOffset) & ((1u << info.bitLength) - 1u);
            result[static_cast<glm::length_t>(i)] = static_cast<float>(rawValue) / upper;
        }
        return result;
    }

    template <typename TYPE>
    static glm::ivec4 decodeSINT_SINTPacked(const ImageCodec* codec, const void* ptr) {
        static_assert(std::is_signed_v<TYPE>);
        auto data = reinterpret_cast<const std::make_unsigned_t<TYPE>*>(ptr);
        glm::ivec4 result(0, 0, 0, 0);
        for (std::size_t i = 0; i < codec->packedSampleInfo.size(); ++i) {
            const auto& info = codec->packedSampleInfo[i];
            uint32_t rawValue = (data[info.elementIndex] >> info.bitOffset) & ((1u << info.bitLength) - 1u);
            result[static_cast<glm::length_t>(i)] = imageio::sign_extend(rawValue, info.bitLength);
        }
        return result;
    }

    template <typename TYPE>
    static glm::vec4 decodeFLOAT_SINTPacked(const ImageCodec* codec, const void* ptr) {
        static_assert(std::is_signed_v<TYPE>);
        auto data = reinterpret_cast<const std::make_unsigned_t<TYPE>*>(ptr);
        glm::vec4 result(0.f, 0.f, 0.f, 1.f);
        for (std::size_t i = 0; i < codec->packedSampleInfo.size(); ++i) {
            const auto& info = codec->packedSampleInfo[i];
            const auto upper = static_cast<float>((1u << (info.bitLength - 1)) - 1u);
            uint32_t rawValue = (data[info.elementIndex] >> info.bitOffset) & ((1u << info.bitLength) - 1u);
            result[static_cast<glm::length_t>(i)] = static_cast<float>(imageio::sign_extend(rawValue, info.bitLength)) / upper;
        }
        return result;
    }
};

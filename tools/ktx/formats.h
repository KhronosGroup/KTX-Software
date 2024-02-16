// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "vkformat_enum.h"
#include <fmt/format.h>
#include "ktx.h"
#define LIBKTX // To stop dfdutils including vulkan_core.h.
#include "dfdutils/dfd.h"
#include <string>
#include <string_view>
#include "utility.h"


// -------------------------------------------------------------------------------------------------
// External Functions
// These are part of libktx but not part of its public API.

extern "C" {
    bool isProhibitedFormat(VkFormat format);
    bool isValidFormat(VkFormat format);
    uint32_t vkFormatTypeSize(VkFormat format);
    const char* vkFormatString(VkFormat format);
    VkFormat stringToVkFormat(const char* str);
}

// -------------------------------------------------------------------------------------------------

namespace ktx {

[[nodiscard]] inline std::string toString(VkFormat format) noexcept {
    std::string str = vkFormatString(format);

    if (str != "VK_UNKNOWN_FORMAT")
        return str;

    return fmt::format("(0x{:08X})", static_cast<uint32_t>(format));
}

[[nodiscard]] inline std::string toString(ktxSupercmpScheme scheme) noexcept {
    std::string str = ktxSupercompressionSchemeString(scheme);

    if (str == "Invalid scheme value")
        str = fmt::format("(0x{:08X})", static_cast<uint32_t>(scheme));

    else if (str == "Vendor or reserved scheme")
        str = fmt::format("Vendor or reserved scheme (0x{:08X})", static_cast<uint32_t>(scheme));

    return str;
}

[[nodiscard]] inline std::string toString(khr_df_vendorid_e vendorId) noexcept {
    const auto str = dfdToStringVendorID(vendorId);
    return str ? std::string(str) : fmt::format("(0x{:05X})", static_cast<uint32_t>(vendorId));
}

[[nodiscard]] inline std::string toString(khr_df_vendorid_e vendorId, khr_df_khr_descriptortype_e descType) noexcept {
    if (vendorId == KHR_DF_VENDORID_KHRONOS) {
        const auto str = dfdToStringDescriptorType(descType);
        return str ? std::string(str) : fmt::format("(0x{:04X})", static_cast<uint32_t>(descType));
    } else {
        return fmt::format("(0x{:04X})", static_cast<uint32_t>(descType));
    }
}

[[nodiscard]] inline std::string toString(khr_df_versionnumber_e version) noexcept {
    const auto str = dfdToStringVersionNumber(version);
    return str ? std::string(str) : fmt::format("(0x{:04X})", static_cast<uint32_t>(version));
}

[[nodiscard]] inline std::string toString(khr_df_model_e model) noexcept {
    const auto str = dfdToStringColorModel(model);
    return str ? std::string(str) : fmt::format("(0x{:02X})", static_cast<uint32_t>(model));
}

[[nodiscard]] inline std::string toString(khr_df_primaries_e primaries) noexcept {
    const auto str = dfdToStringColorPrimaries(primaries);
    return str ? std::string(str) : fmt::format("(0x{:02X})", static_cast<uint32_t>(primaries));
}

[[nodiscard]] inline std::string toString(khr_df_transfer_e transfer) noexcept {
    const auto str = dfdToStringTransferFunction(transfer);
    return str ? std::string(str) : fmt::format("(0x{:02X})", static_cast<uint32_t>(transfer));
}

[[nodiscard]] inline std::string toString(khr_df_model_e colorModel, khr_df_model_channels_e channelType) noexcept {
    const auto str = dfdToStringChannelId(colorModel, channelType);
    return str ? std::string(str) : fmt::format("(0x{:01X})", static_cast<uint32_t>(channelType));
}

// -------------------------------------------------------------------------------------------------

/// Parses a VkFormat. VK_FORMAT_ prefix is optional. Case insensitive.
[[nodiscard]] inline std::optional<VkFormat> parseVkFormat(const std::string& str) noexcept {
    const auto vkFormat = stringToVkFormat(str.c_str());
    if (vkFormat == VK_FORMAT_UNDEFINED)
        return std::nullopt;
    return vkFormat;
}

// -------------------------------------------------------------------------------------------------

[[nodiscard]] constexpr inline bool isSupercompressionWithGlobalData(ktxSupercmpScheme scheme) noexcept {
    switch (scheme) {
    case KTX_SS_BASIS_LZ:
        return true;
    default:
        return false;
    }
}

[[nodiscard]] constexpr inline bool isSupercompressionWithNoGlobalData(ktxSupercmpScheme scheme) noexcept {
    switch (scheme) {
    case KTX_SS_ZSTD:
    case KTX_SS_ZLIB:
        return true;
    default:
        return false;
    }
}

// -------------------------------------------------------------------------------------------------

[[nodiscard]] constexpr inline bool isColorModelBlockCompressed(khr_df_model_e colorModel) noexcept {
    return colorModel >= KHR_DF_MODEL_DXT1A;
}

[[nodiscard]] inline bool isColorPrimariesValid(khr_df_primaries_e primaries) noexcept {
    // dfdToStringColorPrimaries yields a nullptr if the value is invalid
    return dfdToStringColorPrimaries(primaries) != nullptr;
}

[[nodiscard]] inline bool isChannelTypeValid(khr_df_model_e colorModel, khr_df_model_channels_e channelType) noexcept {
    // dfdToStringChannelId yields a nullptr if the model and channel combination is invalid
    return dfdToStringChannelId(colorModel, channelType) != nullptr;
}

[[nodiscard]] inline khr_df_model_e getColorModelForBlockCompressedFormat(VkFormat format) noexcept {
    switch (format) {
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
        return KHR_DF_MODEL_BC1A;
    case VK_FORMAT_BC2_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC2_SRGB_BLOCK:
        return KHR_DF_MODEL_BC2;
    case VK_FORMAT_BC3_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC3_SRGB_BLOCK:
        return KHR_DF_MODEL_BC3;
    case VK_FORMAT_BC4_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC4_SNORM_BLOCK:
        return KHR_DF_MODEL_BC4;
    case VK_FORMAT_BC5_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC5_SNORM_BLOCK:
        return KHR_DF_MODEL_BC5;
    case VK_FORMAT_BC6H_UFLOAT_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:
        return KHR_DF_MODEL_BC6H;
    case VK_FORMAT_BC7_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC7_SRGB_BLOCK:
        return KHR_DF_MODEL_BC7;
    case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_EAC_R11_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_EAC_R11_SNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_EAC_R11G11_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
        return KHR_DF_MODEL_ETC2;
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
        return KHR_DF_MODEL_ASTC;
    case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG: [[fallthrough]];
    case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG: [[fallthrough]];
    case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG: [[fallthrough]];
    case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
        return KHR_DF_MODEL_PVRTC;
    case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG: [[fallthrough]];
    case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG: [[fallthrough]];
    case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG: [[fallthrough]];
    case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
        return KHR_DF_MODEL_PVRTC2;
    default:
        assert(false);
        return KHR_DF_MODEL_UNSPECIFIED;
    }
}

// -------------------------------------------------------------------------------------------------

[[nodiscard]] inline bool isFormatValid(VkFormat format) noexcept {
    // isValidFormat would accept negative values, handle it separately
    return isValidFormat(format) && format >= 0;
}

[[nodiscard]] inline bool isFormatKnown(VkFormat format) noexcept {
    const std::string_view str = vkFormatString(format);
    return str != "VK_UNKNOWN_FORMAT";
}

[[nodiscard]] constexpr inline bool isFormatStencil(VkFormat format) noexcept {
    switch (format) {
    // Stencil only formats:
    case VK_FORMAT_S8_UINT: [[fallthrough]];
    // Depth and Stencil mixed formats:
    case VK_FORMAT_D16_UNORM_S8_UINT: [[fallthrough]];
    case VK_FORMAT_D24_UNORM_S8_UINT: [[fallthrough]];
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return true;
    default:
        return false;
    }
}

[[nodiscard]] constexpr inline bool isFormatDepth(VkFormat format) noexcept {
    switch (format) {
    // Depth only formats:
    case VK_FORMAT_D16_UNORM: [[fallthrough]];
    case VK_FORMAT_X8_D24_UNORM_PACK32: [[fallthrough]];
    case VK_FORMAT_D32_SFLOAT: [[fallthrough]];
    // Depth and Stencil mixed formats:
    case VK_FORMAT_D16_UNORM_S8_UINT: [[fallthrough]];
    case VK_FORMAT_D24_UNORM_S8_UINT: [[fallthrough]];
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return true;
    default:
        return false;
    }
}

[[nodiscard]] constexpr inline bool isFormatDepthStencil(VkFormat format) noexcept {
    return isFormatDepth(format) || isFormatStencil(format);
}

[[nodiscard]] constexpr inline bool isSupercompressionBlockCompressed(ktxSupercmpScheme scheme) noexcept {
    switch (scheme) {
    case KTX_SS_BASIS_LZ:
        return true;
    default:
        return false;
    }
}

/// SINT or UINT
[[nodiscard]] constexpr inline bool isFormatINT(VkFormat format) noexcept {
    switch (format) {
    case VK_FORMAT_R8_UINT: [[fallthrough]];
    case VK_FORMAT_R8_SINT: [[fallthrough]];
    case VK_FORMAT_R8G8_UINT: [[fallthrough]];
    case VK_FORMAT_R8G8_SINT: [[fallthrough]];
    case VK_FORMAT_R8G8B8_UINT: [[fallthrough]];
    case VK_FORMAT_R8G8B8_SINT: [[fallthrough]];
    case VK_FORMAT_B8G8R8_UINT: [[fallthrough]];
    case VK_FORMAT_B8G8R8_SINT: [[fallthrough]];
    case VK_FORMAT_R8G8B8A8_UINT: [[fallthrough]];
    case VK_FORMAT_R8G8B8A8_SINT: [[fallthrough]];
    case VK_FORMAT_B8G8R8A8_UINT: [[fallthrough]];
    case VK_FORMAT_B8G8R8A8_SINT: [[fallthrough]];
    case VK_FORMAT_A8B8G8R8_UINT_PACK32: [[fallthrough]];
    case VK_FORMAT_A8B8G8R8_SINT_PACK32: [[fallthrough]];
    case VK_FORMAT_A2R10G10B10_UINT_PACK32: [[fallthrough]];
    case VK_FORMAT_A2R10G10B10_SINT_PACK32: [[fallthrough]];
    case VK_FORMAT_A2B10G10R10_UINT_PACK32: [[fallthrough]];
    case VK_FORMAT_A2B10G10R10_SINT_PACK32: [[fallthrough]];
    case VK_FORMAT_R16_UINT: [[fallthrough]];
    case VK_FORMAT_R16_SINT: [[fallthrough]];
    case VK_FORMAT_R16G16_UINT: [[fallthrough]];
    case VK_FORMAT_R16G16_SINT: [[fallthrough]];
    case VK_FORMAT_R16G16B16_UINT: [[fallthrough]];
    case VK_FORMAT_R16G16B16_SINT: [[fallthrough]];
    case VK_FORMAT_R16G16B16A16_UINT: [[fallthrough]];
    case VK_FORMAT_R16G16B16A16_SINT: [[fallthrough]];
    case VK_FORMAT_R32_UINT: [[fallthrough]];
    case VK_FORMAT_R32_SINT: [[fallthrough]];
    case VK_FORMAT_R32G32_UINT: [[fallthrough]];
    case VK_FORMAT_R32G32_SINT: [[fallthrough]];
    case VK_FORMAT_R32G32B32_UINT: [[fallthrough]];
    case VK_FORMAT_R32G32B32_SINT: [[fallthrough]];
    case VK_FORMAT_R32G32B32A32_UINT: [[fallthrough]];
    case VK_FORMAT_R32G32B32A32_SINT: [[fallthrough]];
    case VK_FORMAT_R64_UINT: [[fallthrough]];
    case VK_FORMAT_R64_SINT: [[fallthrough]];
    case VK_FORMAT_R64G64_UINT: [[fallthrough]];
    case VK_FORMAT_R64G64_SINT: [[fallthrough]];
    case VK_FORMAT_R64G64B64_UINT: [[fallthrough]];
    case VK_FORMAT_R64G64B64_SINT: [[fallthrough]];
    case VK_FORMAT_R64G64B64A64_UINT: [[fallthrough]];
    case VK_FORMAT_R64G64B64A64_SINT: [[fallthrough]];
    case VK_FORMAT_S8_UINT: [[fallthrough]];
    case VK_FORMAT_D16_UNORM_S8_UINT: [[fallthrough]];
    case VK_FORMAT_D24_UNORM_S8_UINT: [[fallthrough]];
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return true;
    default:
        return false;
    }
}

[[nodiscard]] constexpr inline bool isFormatBlockCompressed(VkFormat format) noexcept {
    switch (format) {
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC2_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC2_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC3_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC3_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC4_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC4_SNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC5_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC5_SNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC6H_UFLOAT_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC6H_SFLOAT_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC7_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC7_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_EAC_R11_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_EAC_R11_SNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_EAC_R11G11_UNORM_BLOCK: [[fallthrough]];
    case VK_FORMAT_EAC_R11G11_SNORM_BLOCK: [[fallthrough]];
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
    case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG: [[fallthrough]];
    case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG: [[fallthrough]];
    case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG: [[fallthrough]];
    case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG: [[fallthrough]];
    case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG: [[fallthrough]];
    case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG: [[fallthrough]];
    case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG: [[fallthrough]];
    case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG: [[fallthrough]];
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

[[nodiscard]] constexpr inline bool isFormat3DBlockCompressed(VkFormat format) noexcept {
    switch (format) {
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

[[nodiscard]] constexpr inline bool isFormatSRGB(VkFormat format) noexcept {
    switch (format) {
    case VK_FORMAT_R8_SRGB: [[fallthrough]];
    case VK_FORMAT_R8G8_SRGB: [[fallthrough]];
    case VK_FORMAT_R8G8B8_SRGB: [[fallthrough]];
    case VK_FORMAT_B8G8R8_SRGB: [[fallthrough]];
    case VK_FORMAT_R8G8B8A8_SRGB: [[fallthrough]];
    case VK_FORMAT_B8G8R8A8_SRGB: [[fallthrough]];
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32: [[fallthrough]];
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC2_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC3_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_BC7_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK: [[fallthrough]];
    case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG: [[fallthrough]];
    case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG: [[fallthrough]];
    case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG: [[fallthrough]];
    case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG: [[fallthrough]];
    case VK_FORMAT_ASTC_3x3x3_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x3x3_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x3_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x4_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x4x4_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x4_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x5_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x5x5_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x5_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x6_SRGB_BLOCK_EXT:
        return true;
    default:
        return false;
    }
}

[[nodiscard]] constexpr inline bool isFormatNotSRGBButHasSRGBVariant(VkFormat format) noexcept {
    switch (format) {
    //   VK_FORMAT_R8_SRGB
    case VK_FORMAT_R8_UNORM: [[fallthrough]];
    case VK_FORMAT_R8_SNORM: [[fallthrough]];
    case VK_FORMAT_R8_USCALED: [[fallthrough]];
    case VK_FORMAT_R8_SSCALED: [[fallthrough]];
    case VK_FORMAT_R8_UINT: [[fallthrough]];
    case VK_FORMAT_R8_SINT: [[fallthrough]];

    //   VK_FORMAT_R8G8_SRGB
    case VK_FORMAT_R8G8_UNORM: [[fallthrough]];
    case VK_FORMAT_R8G8_SNORM: [[fallthrough]];
    case VK_FORMAT_R8G8_USCALED: [[fallthrough]];
    case VK_FORMAT_R8G8_SSCALED: [[fallthrough]];
    case VK_FORMAT_R8G8_UINT: [[fallthrough]];
    case VK_FORMAT_R8G8_SINT: [[fallthrough]];

    //   VK_FORMAT_R8G8B8_SRGB
    case VK_FORMAT_R8G8B8_UNORM: [[fallthrough]];
    case VK_FORMAT_R8G8B8_SNORM: [[fallthrough]];
    case VK_FORMAT_R8G8B8_USCALED: [[fallthrough]];
    case VK_FORMAT_R8G8B8_SSCALED: [[fallthrough]];
    case VK_FORMAT_R8G8B8_UINT: [[fallthrough]];
    case VK_FORMAT_R8G8B8_SINT: [[fallthrough]];

    //   VK_FORMAT_B8G8R8_SRGB
    case VK_FORMAT_B8G8R8_UNORM: [[fallthrough]];
    case VK_FORMAT_B8G8R8_SNORM: [[fallthrough]];
    case VK_FORMAT_B8G8R8_USCALED: [[fallthrough]];
    case VK_FORMAT_B8G8R8_SSCALED: [[fallthrough]];
    case VK_FORMAT_B8G8R8_UINT: [[fallthrough]];
    case VK_FORMAT_B8G8R8_SINT: [[fallthrough]];

    //   VK_FORMAT_R8G8B8A8_SRGB
    case VK_FORMAT_R8G8B8A8_UNORM: [[fallthrough]];
    case VK_FORMAT_R8G8B8A8_SNORM: [[fallthrough]];
    case VK_FORMAT_R8G8B8A8_USCALED: [[fallthrough]];
    case VK_FORMAT_R8G8B8A8_SSCALED: [[fallthrough]];
    case VK_FORMAT_R8G8B8A8_UINT: [[fallthrough]];
    case VK_FORMAT_R8G8B8A8_SINT: [[fallthrough]];

    //   VK_FORMAT_B8G8R8A8_SRGB
    case VK_FORMAT_B8G8R8A8_UNORM: [[fallthrough]];
    case VK_FORMAT_B8G8R8A8_SNORM: [[fallthrough]];
    case VK_FORMAT_B8G8R8A8_USCALED: [[fallthrough]];
    case VK_FORMAT_B8G8R8A8_SSCALED: [[fallthrough]];
    case VK_FORMAT_B8G8R8A8_UINT: [[fallthrough]];
    case VK_FORMAT_B8G8R8A8_SINT: [[fallthrough]];

    //   VK_FORMAT_A8B8G8R8_SRGB_PACK32
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32: [[fallthrough]];
    case VK_FORMAT_A8B8G8R8_SNORM_PACK32: [[fallthrough]];
    case VK_FORMAT_A8B8G8R8_USCALED_PACK32: [[fallthrough]];
    case VK_FORMAT_A8B8G8R8_SSCALED_PACK32: [[fallthrough]];
    case VK_FORMAT_A8B8G8R8_UINT_PACK32: [[fallthrough]];
    case VK_FORMAT_A8B8G8R8_SINT_PACK32: [[fallthrough]];

    //   VK_FORMAT_BC1_RGB_SRGB_BLOCK
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK: [[fallthrough]];

    //   VK_FORMAT_BC1_RGBA_SRGB_BLOCK
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK: [[fallthrough]];

    //   VK_FORMAT_BC2_SRGB_BLOCK
    case VK_FORMAT_BC2_UNORM_BLOCK: [[fallthrough]];

    //   VK_FORMAT_BC3_SRGB_BLOCK
    case VK_FORMAT_BC3_UNORM_BLOCK: [[fallthrough]];

    //   VK_FORMAT_BC7_SRGB_BLOCK
    case VK_FORMAT_BC7_UNORM_BLOCK: [[fallthrough]];

    //   VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK
    case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK: [[fallthrough]];

    //   VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK
    case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK: [[fallthrough]];

    //   VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK
    case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: [[fallthrough]];

    //   VK_FORMAT_ASTC_4x4_SRGB_BLOCK
    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK: [[fallthrough]];

    //   VK_FORMAT_ASTC_5x4_SRGB_BLOCK
    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK: [[fallthrough]];

    //   VK_FORMAT_ASTC_5x5_SRGB_BLOCK
    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK: [[fallthrough]];

    //   VK_FORMAT_ASTC_6x5_SRGB_BLOCK
    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK: [[fallthrough]];

    //   VK_FORMAT_ASTC_6x6_SRGB_BLOCK
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK: [[fallthrough]];

    //   VK_FORMAT_ASTC_8x5_SRGB_BLOCK
    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK: [[fallthrough]];

    //   VK_FORMAT_ASTC_8x6_SRGB_BLOCK
    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK: [[fallthrough]];

    //   VK_FORMAT_ASTC_8x8_SRGB_BLOCK
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK: [[fallthrough]];

    //   VK_FORMAT_ASTC_10x5_SRGB_BLOCK
    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK: [[fallthrough]];

    //   VK_FORMAT_ASTC_10x6_SRGB_BLOCK
    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK: [[fallthrough]];

    //   VK_FORMAT_ASTC_10x8_SRGB_BLOCK
    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK: [[fallthrough]];

    //   VK_FORMAT_ASTC_10x10_SRGB_BLOCK
    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK: [[fallthrough]];

    //   VK_FORMAT_ASTC_12x10_SRGB_BLOCK
    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK: [[fallthrough]];

    //   VK_FORMAT_ASTC_12x12_SRGB_BLOCK
    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK: [[fallthrough]];

    //   VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG
    case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG: [[fallthrough]];

    //   VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG
    case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG: [[fallthrough]];

    //   VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG
    case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG: [[fallthrough]];

    //   VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG
    case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG: [[fallthrough]];

    //   VK_FORMAT_ASTC_3x3x3_SRGB_BLOCK_EXT
    case VK_FORMAT_ASTC_3x3x3_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_3x3x3_SFLOAT_BLOCK_EXT: [[fallthrough]];

    //   VK_FORMAT_ASTC_4x3x3_SRGB_BLOCK_EXT
    case VK_FORMAT_ASTC_4x3x3_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x3x3_SFLOAT_BLOCK_EXT: [[fallthrough]];

    //   VK_FORMAT_ASTC_4x4x3_SRGB_BLOCK_EXT
    case VK_FORMAT_ASTC_4x4x3_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x3_SFLOAT_BLOCK_EXT: [[fallthrough]];

    //   VK_FORMAT_ASTC_4x4x4_SRGB_BLOCK_EXT
    case VK_FORMAT_ASTC_4x4x4_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x4_SFLOAT_BLOCK_EXT: [[fallthrough]];

    //   VK_FORMAT_ASTC_5x4x4_SRGB_BLOCK_EXT
    case VK_FORMAT_ASTC_5x4x4_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x4x4_SFLOAT_BLOCK_EXT: [[fallthrough]];

    //   VK_FORMAT_ASTC_5x5x4_SRGB_BLOCK_EXT
    case VK_FORMAT_ASTC_5x5x4_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x4_SFLOAT_BLOCK_EXT: [[fallthrough]];

    //   VK_FORMAT_ASTC_5x5x5_SRGB_BLOCK_EXT
    case VK_FORMAT_ASTC_5x5x5_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x5_SFLOAT_BLOCK_EXT: [[fallthrough]];

    //   VK_FORMAT_ASTC_6x5x5_SRGB_BLOCK_EXT
    case VK_FORMAT_ASTC_6x5x5_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x5x5_SFLOAT_BLOCK_EXT: [[fallthrough]];

    //   VK_FORMAT_ASTC_6x6x5_SRGB_BLOCK_EXT
    case VK_FORMAT_ASTC_6x6x5_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x5_SFLOAT_BLOCK_EXT: [[fallthrough]];

    //   VK_FORMAT_ASTC_6x6x6_SRGB_BLOCK_EXT
    case VK_FORMAT_ASTC_6x6x6_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x6_SFLOAT_BLOCK_EXT:
        return true;
    default:
        return false;
    }
}

[[nodiscard]] constexpr inline bool isFormat422(VkFormat format) noexcept {
    switch (format) {
    case VK_FORMAT_G8B8G8R8_422_UNORM: [[fallthrough]];
    case VK_FORMAT_B8G8R8G8_422_UNORM: [[fallthrough]];
    case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM: [[fallthrough]];
    case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM: [[fallthrough]];
    case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16: [[fallthrough]];
    case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16: [[fallthrough]];
    case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16: [[fallthrough]];
    case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16: [[fallthrough]];
    case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16: [[fallthrough]];
    case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16: [[fallthrough]];
    case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16: [[fallthrough]];
    case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16: [[fallthrough]];
    case VK_FORMAT_G16B16G16R16_422_UNORM: [[fallthrough]];
    case VK_FORMAT_B16G16R16G16_422_UNORM: [[fallthrough]];
    case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM: [[fallthrough]];
    case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
        return true;
    default:
        return false;
    }
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

[[nodiscard]] constexpr inline bool isFormatAstcLDR(VkFormat format) noexcept {
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
    case VK_FORMAT_ASTC_3x3x3_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_3x3x3_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x3x3_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x3x3_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x3_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x3_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x4_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_4x4x4_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x4x4_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x4x4_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x4_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x4_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x5_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_5x5x5_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x5x5_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x5x5_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x5_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x5_SRGB_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x6_UNORM_BLOCK_EXT: [[fallthrough]];
    case VK_FORMAT_ASTC_6x6x6_SRGB_BLOCK_EXT:
        return true;
    default:
        return false;
    }
}

} // namespace ktx

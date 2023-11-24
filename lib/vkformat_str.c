
/***************************** Do not edit.  *****************************
 Automatically generated from vulkan_core.h version 267 by mkvkformatfiles.
 *************************************************************************/

/*
** Copyright 2015-2023 The Khronos Group Inc.
**
** SPDX-License-Identifier: Apache-2.0
*/


#include <stdint.h>
#include <ctype.h>

#include "vkformat_enum.h"

const char*
vkFormatString(VkFormat format)
{
    switch (format) {
      case VK_FORMAT_UNDEFINED:
        return "VK_FORMAT_UNDEFINED";
      case VK_FORMAT_R4G4_UNORM_PACK8:
        return "VK_FORMAT_R4G4_UNORM_PACK8";
      case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
        return "VK_FORMAT_R4G4B4A4_UNORM_PACK16";
      case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
        return "VK_FORMAT_B4G4R4A4_UNORM_PACK16";
      case VK_FORMAT_R5G6B5_UNORM_PACK16:
        return "VK_FORMAT_R5G6B5_UNORM_PACK16";
      case VK_FORMAT_B5G6R5_UNORM_PACK16:
        return "VK_FORMAT_B5G6R5_UNORM_PACK16";
      case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
        return "VK_FORMAT_R5G5B5A1_UNORM_PACK16";
      case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
        return "VK_FORMAT_B5G5R5A1_UNORM_PACK16";
      case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
        return "VK_FORMAT_A1R5G5B5_UNORM_PACK16";
      case VK_FORMAT_R8_UNORM:
        return "VK_FORMAT_R8_UNORM";
      case VK_FORMAT_R8_SNORM:
        return "VK_FORMAT_R8_SNORM";
      case VK_FORMAT_R8_USCALED:
        return "VK_FORMAT_R8_USCALED";
      case VK_FORMAT_R8_SSCALED:
        return "VK_FORMAT_R8_SSCALED";
      case VK_FORMAT_R8_UINT:
        return "VK_FORMAT_R8_UINT";
      case VK_FORMAT_R8_SINT:
        return "VK_FORMAT_R8_SINT";
      case VK_FORMAT_R8_SRGB:
        return "VK_FORMAT_R8_SRGB";
      case VK_FORMAT_R8G8_UNORM:
        return "VK_FORMAT_R8G8_UNORM";
      case VK_FORMAT_R8G8_SNORM:
        return "VK_FORMAT_R8G8_SNORM";
      case VK_FORMAT_R8G8_USCALED:
        return "VK_FORMAT_R8G8_USCALED";
      case VK_FORMAT_R8G8_SSCALED:
        return "VK_FORMAT_R8G8_SSCALED";
      case VK_FORMAT_R8G8_UINT:
        return "VK_FORMAT_R8G8_UINT";
      case VK_FORMAT_R8G8_SINT:
        return "VK_FORMAT_R8G8_SINT";
      case VK_FORMAT_R8G8_SRGB:
        return "VK_FORMAT_R8G8_SRGB";
      case VK_FORMAT_R8G8B8_UNORM:
        return "VK_FORMAT_R8G8B8_UNORM";
      case VK_FORMAT_R8G8B8_SNORM:
        return "VK_FORMAT_R8G8B8_SNORM";
      case VK_FORMAT_R8G8B8_USCALED:
        return "VK_FORMAT_R8G8B8_USCALED";
      case VK_FORMAT_R8G8B8_SSCALED:
        return "VK_FORMAT_R8G8B8_SSCALED";
      case VK_FORMAT_R8G8B8_UINT:
        return "VK_FORMAT_R8G8B8_UINT";
      case VK_FORMAT_R8G8B8_SINT:
        return "VK_FORMAT_R8G8B8_SINT";
      case VK_FORMAT_R8G8B8_SRGB:
        return "VK_FORMAT_R8G8B8_SRGB";
      case VK_FORMAT_B8G8R8_UNORM:
        return "VK_FORMAT_B8G8R8_UNORM";
      case VK_FORMAT_B8G8R8_SNORM:
        return "VK_FORMAT_B8G8R8_SNORM";
      case VK_FORMAT_B8G8R8_USCALED:
        return "VK_FORMAT_B8G8R8_USCALED";
      case VK_FORMAT_B8G8R8_SSCALED:
        return "VK_FORMAT_B8G8R8_SSCALED";
      case VK_FORMAT_B8G8R8_UINT:
        return "VK_FORMAT_B8G8R8_UINT";
      case VK_FORMAT_B8G8R8_SINT:
        return "VK_FORMAT_B8G8R8_SINT";
      case VK_FORMAT_B8G8R8_SRGB:
        return "VK_FORMAT_B8G8R8_SRGB";
      case VK_FORMAT_R8G8B8A8_UNORM:
        return "VK_FORMAT_R8G8B8A8_UNORM";
      case VK_FORMAT_R8G8B8A8_SNORM:
        return "VK_FORMAT_R8G8B8A8_SNORM";
      case VK_FORMAT_R8G8B8A8_USCALED:
        return "VK_FORMAT_R8G8B8A8_USCALED";
      case VK_FORMAT_R8G8B8A8_SSCALED:
        return "VK_FORMAT_R8G8B8A8_SSCALED";
      case VK_FORMAT_R8G8B8A8_UINT:
        return "VK_FORMAT_R8G8B8A8_UINT";
      case VK_FORMAT_R8G8B8A8_SINT:
        return "VK_FORMAT_R8G8B8A8_SINT";
      case VK_FORMAT_R8G8B8A8_SRGB:
        return "VK_FORMAT_R8G8B8A8_SRGB";
      case VK_FORMAT_B8G8R8A8_UNORM:
        return "VK_FORMAT_B8G8R8A8_UNORM";
      case VK_FORMAT_B8G8R8A8_SNORM:
        return "VK_FORMAT_B8G8R8A8_SNORM";
      case VK_FORMAT_B8G8R8A8_USCALED:
        return "VK_FORMAT_B8G8R8A8_USCALED";
      case VK_FORMAT_B8G8R8A8_SSCALED:
        return "VK_FORMAT_B8G8R8A8_SSCALED";
      case VK_FORMAT_B8G8R8A8_UINT:
        return "VK_FORMAT_B8G8R8A8_UINT";
      case VK_FORMAT_B8G8R8A8_SINT:
        return "VK_FORMAT_B8G8R8A8_SINT";
      case VK_FORMAT_B8G8R8A8_SRGB:
        return "VK_FORMAT_B8G8R8A8_SRGB";
      case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
        return "VK_FORMAT_A8B8G8R8_UNORM_PACK32";
      case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
        return "VK_FORMAT_A8B8G8R8_SNORM_PACK32";
      case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
        return "VK_FORMAT_A8B8G8R8_USCALED_PACK32";
      case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
        return "VK_FORMAT_A8B8G8R8_SSCALED_PACK32";
      case VK_FORMAT_A8B8G8R8_UINT_PACK32:
        return "VK_FORMAT_A8B8G8R8_UINT_PACK32";
      case VK_FORMAT_A8B8G8R8_SINT_PACK32:
        return "VK_FORMAT_A8B8G8R8_SINT_PACK32";
      case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
        return "VK_FORMAT_A8B8G8R8_SRGB_PACK32";
      case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
        return "VK_FORMAT_A2R10G10B10_UNORM_PACK32";
      case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
        return "VK_FORMAT_A2R10G10B10_SNORM_PACK32";
      case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
        return "VK_FORMAT_A2R10G10B10_USCALED_PACK32";
      case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
        return "VK_FORMAT_A2R10G10B10_SSCALED_PACK32";
      case VK_FORMAT_A2R10G10B10_UINT_PACK32:
        return "VK_FORMAT_A2R10G10B10_UINT_PACK32";
      case VK_FORMAT_A2R10G10B10_SINT_PACK32:
        return "VK_FORMAT_A2R10G10B10_SINT_PACK32";
      case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        return "VK_FORMAT_A2B10G10R10_UNORM_PACK32";
      case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
        return "VK_FORMAT_A2B10G10R10_SNORM_PACK32";
      case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
        return "VK_FORMAT_A2B10G10R10_USCALED_PACK32";
      case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
        return "VK_FORMAT_A2B10G10R10_SSCALED_PACK32";
      case VK_FORMAT_A2B10G10R10_UINT_PACK32:
        return "VK_FORMAT_A2B10G10R10_UINT_PACK32";
      case VK_FORMAT_A2B10G10R10_SINT_PACK32:
        return "VK_FORMAT_A2B10G10R10_SINT_PACK32";
      case VK_FORMAT_R16_UNORM:
        return "VK_FORMAT_R16_UNORM";
      case VK_FORMAT_R16_SNORM:
        return "VK_FORMAT_R16_SNORM";
      case VK_FORMAT_R16_USCALED:
        return "VK_FORMAT_R16_USCALED";
      case VK_FORMAT_R16_SSCALED:
        return "VK_FORMAT_R16_SSCALED";
      case VK_FORMAT_R16_UINT:
        return "VK_FORMAT_R16_UINT";
      case VK_FORMAT_R16_SINT:
        return "VK_FORMAT_R16_SINT";
      case VK_FORMAT_R16_SFLOAT:
        return "VK_FORMAT_R16_SFLOAT";
      case VK_FORMAT_R16G16_UNORM:
        return "VK_FORMAT_R16G16_UNORM";
      case VK_FORMAT_R16G16_SNORM:
        return "VK_FORMAT_R16G16_SNORM";
      case VK_FORMAT_R16G16_USCALED:
        return "VK_FORMAT_R16G16_USCALED";
      case VK_FORMAT_R16G16_SSCALED:
        return "VK_FORMAT_R16G16_SSCALED";
      case VK_FORMAT_R16G16_UINT:
        return "VK_FORMAT_R16G16_UINT";
      case VK_FORMAT_R16G16_SINT:
        return "VK_FORMAT_R16G16_SINT";
      case VK_FORMAT_R16G16_SFLOAT:
        return "VK_FORMAT_R16G16_SFLOAT";
      case VK_FORMAT_R16G16B16_UNORM:
        return "VK_FORMAT_R16G16B16_UNORM";
      case VK_FORMAT_R16G16B16_SNORM:
        return "VK_FORMAT_R16G16B16_SNORM";
      case VK_FORMAT_R16G16B16_USCALED:
        return "VK_FORMAT_R16G16B16_USCALED";
      case VK_FORMAT_R16G16B16_SSCALED:
        return "VK_FORMAT_R16G16B16_SSCALED";
      case VK_FORMAT_R16G16B16_UINT:
        return "VK_FORMAT_R16G16B16_UINT";
      case VK_FORMAT_R16G16B16_SINT:
        return "VK_FORMAT_R16G16B16_SINT";
      case VK_FORMAT_R16G16B16_SFLOAT:
        return "VK_FORMAT_R16G16B16_SFLOAT";
      case VK_FORMAT_R16G16B16A16_UNORM:
        return "VK_FORMAT_R16G16B16A16_UNORM";
      case VK_FORMAT_R16G16B16A16_SNORM:
        return "VK_FORMAT_R16G16B16A16_SNORM";
      case VK_FORMAT_R16G16B16A16_USCALED:
        return "VK_FORMAT_R16G16B16A16_USCALED";
      case VK_FORMAT_R16G16B16A16_SSCALED:
        return "VK_FORMAT_R16G16B16A16_SSCALED";
      case VK_FORMAT_R16G16B16A16_UINT:
        return "VK_FORMAT_R16G16B16A16_UINT";
      case VK_FORMAT_R16G16B16A16_SINT:
        return "VK_FORMAT_R16G16B16A16_SINT";
      case VK_FORMAT_R16G16B16A16_SFLOAT:
        return "VK_FORMAT_R16G16B16A16_SFLOAT";
      case VK_FORMAT_R32_UINT:
        return "VK_FORMAT_R32_UINT";
      case VK_FORMAT_R32_SINT:
        return "VK_FORMAT_R32_SINT";
      case VK_FORMAT_R32_SFLOAT:
        return "VK_FORMAT_R32_SFLOAT";
      case VK_FORMAT_R32G32_UINT:
        return "VK_FORMAT_R32G32_UINT";
      case VK_FORMAT_R32G32_SINT:
        return "VK_FORMAT_R32G32_SINT";
      case VK_FORMAT_R32G32_SFLOAT:
        return "VK_FORMAT_R32G32_SFLOAT";
      case VK_FORMAT_R32G32B32_UINT:
        return "VK_FORMAT_R32G32B32_UINT";
      case VK_FORMAT_R32G32B32_SINT:
        return "VK_FORMAT_R32G32B32_SINT";
      case VK_FORMAT_R32G32B32_SFLOAT:
        return "VK_FORMAT_R32G32B32_SFLOAT";
      case VK_FORMAT_R32G32B32A32_UINT:
        return "VK_FORMAT_R32G32B32A32_UINT";
      case VK_FORMAT_R32G32B32A32_SINT:
        return "VK_FORMAT_R32G32B32A32_SINT";
      case VK_FORMAT_R32G32B32A32_SFLOAT:
        return "VK_FORMAT_R32G32B32A32_SFLOAT";
      case VK_FORMAT_R64_UINT:
        return "VK_FORMAT_R64_UINT";
      case VK_FORMAT_R64_SINT:
        return "VK_FORMAT_R64_SINT";
      case VK_FORMAT_R64_SFLOAT:
        return "VK_FORMAT_R64_SFLOAT";
      case VK_FORMAT_R64G64_UINT:
        return "VK_FORMAT_R64G64_UINT";
      case VK_FORMAT_R64G64_SINT:
        return "VK_FORMAT_R64G64_SINT";
      case VK_FORMAT_R64G64_SFLOAT:
        return "VK_FORMAT_R64G64_SFLOAT";
      case VK_FORMAT_R64G64B64_UINT:
        return "VK_FORMAT_R64G64B64_UINT";
      case VK_FORMAT_R64G64B64_SINT:
        return "VK_FORMAT_R64G64B64_SINT";
      case VK_FORMAT_R64G64B64_SFLOAT:
        return "VK_FORMAT_R64G64B64_SFLOAT";
      case VK_FORMAT_R64G64B64A64_UINT:
        return "VK_FORMAT_R64G64B64A64_UINT";
      case VK_FORMAT_R64G64B64A64_SINT:
        return "VK_FORMAT_R64G64B64A64_SINT";
      case VK_FORMAT_R64G64B64A64_SFLOAT:
        return "VK_FORMAT_R64G64B64A64_SFLOAT";
      case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
        return "VK_FORMAT_B10G11R11_UFLOAT_PACK32";
      case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
        return "VK_FORMAT_E5B9G9R9_UFLOAT_PACK32";
      case VK_FORMAT_D16_UNORM:
        return "VK_FORMAT_D16_UNORM";
      case VK_FORMAT_X8_D24_UNORM_PACK32:
        return "VK_FORMAT_X8_D24_UNORM_PACK32";
      case VK_FORMAT_D32_SFLOAT:
        return "VK_FORMAT_D32_SFLOAT";
      case VK_FORMAT_S8_UINT:
        return "VK_FORMAT_S8_UINT";
      case VK_FORMAT_D16_UNORM_S8_UINT:
        return "VK_FORMAT_D16_UNORM_S8_UINT";
      case VK_FORMAT_D24_UNORM_S8_UINT:
        return "VK_FORMAT_D24_UNORM_S8_UINT";
      case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return "VK_FORMAT_D32_SFLOAT_S8_UINT";
      case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
        return "VK_FORMAT_BC1_RGB_UNORM_BLOCK";
      case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
        return "VK_FORMAT_BC1_RGB_SRGB_BLOCK";
      case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        return "VK_FORMAT_BC1_RGBA_UNORM_BLOCK";
      case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
        return "VK_FORMAT_BC1_RGBA_SRGB_BLOCK";
      case VK_FORMAT_BC2_UNORM_BLOCK:
        return "VK_FORMAT_BC2_UNORM_BLOCK";
      case VK_FORMAT_BC2_SRGB_BLOCK:
        return "VK_FORMAT_BC2_SRGB_BLOCK";
      case VK_FORMAT_BC3_UNORM_BLOCK:
        return "VK_FORMAT_BC3_UNORM_BLOCK";
      case VK_FORMAT_BC3_SRGB_BLOCK:
        return "VK_FORMAT_BC3_SRGB_BLOCK";
      case VK_FORMAT_BC4_UNORM_BLOCK:
        return "VK_FORMAT_BC4_UNORM_BLOCK";
      case VK_FORMAT_BC4_SNORM_BLOCK:
        return "VK_FORMAT_BC4_SNORM_BLOCK";
      case VK_FORMAT_BC5_UNORM_BLOCK:
        return "VK_FORMAT_BC5_UNORM_BLOCK";
      case VK_FORMAT_BC5_SNORM_BLOCK:
        return "VK_FORMAT_BC5_SNORM_BLOCK";
      case VK_FORMAT_BC6H_UFLOAT_BLOCK:
        return "VK_FORMAT_BC6H_UFLOAT_BLOCK";
      case VK_FORMAT_BC6H_SFLOAT_BLOCK:
        return "VK_FORMAT_BC6H_SFLOAT_BLOCK";
      case VK_FORMAT_BC7_UNORM_BLOCK:
        return "VK_FORMAT_BC7_UNORM_BLOCK";
      case VK_FORMAT_BC7_SRGB_BLOCK:
        return "VK_FORMAT_BC7_SRGB_BLOCK";
      case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
        return "VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK";
      case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
        return "VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK";
      case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
        return "VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK";
      case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
        return "VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK";
      case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
        return "VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK";
      case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
        return "VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK";
      case VK_FORMAT_EAC_R11_UNORM_BLOCK:
        return "VK_FORMAT_EAC_R11_UNORM_BLOCK";
      case VK_FORMAT_EAC_R11_SNORM_BLOCK:
        return "VK_FORMAT_EAC_R11_SNORM_BLOCK";
      case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
        return "VK_FORMAT_EAC_R11G11_UNORM_BLOCK";
      case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
        return "VK_FORMAT_EAC_R11G11_SNORM_BLOCK";
      case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
        return "VK_FORMAT_ASTC_4x4_UNORM_BLOCK";
      case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
        return "VK_FORMAT_ASTC_4x4_SRGB_BLOCK";
      case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
        return "VK_FORMAT_ASTC_5x4_UNORM_BLOCK";
      case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
        return "VK_FORMAT_ASTC_5x4_SRGB_BLOCK";
      case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
        return "VK_FORMAT_ASTC_5x5_UNORM_BLOCK";
      case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
        return "VK_FORMAT_ASTC_5x5_SRGB_BLOCK";
      case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
        return "VK_FORMAT_ASTC_6x5_UNORM_BLOCK";
      case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
        return "VK_FORMAT_ASTC_6x5_SRGB_BLOCK";
      case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
        return "VK_FORMAT_ASTC_6x6_UNORM_BLOCK";
      case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
        return "VK_FORMAT_ASTC_6x6_SRGB_BLOCK";
      case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
        return "VK_FORMAT_ASTC_8x5_UNORM_BLOCK";
      case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
        return "VK_FORMAT_ASTC_8x5_SRGB_BLOCK";
      case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
        return "VK_FORMAT_ASTC_8x6_UNORM_BLOCK";
      case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
        return "VK_FORMAT_ASTC_8x6_SRGB_BLOCK";
      case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
        return "VK_FORMAT_ASTC_8x8_UNORM_BLOCK";
      case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
        return "VK_FORMAT_ASTC_8x8_SRGB_BLOCK";
      case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
        return "VK_FORMAT_ASTC_10x5_UNORM_BLOCK";
      case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
        return "VK_FORMAT_ASTC_10x5_SRGB_BLOCK";
      case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
        return "VK_FORMAT_ASTC_10x6_UNORM_BLOCK";
      case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
        return "VK_FORMAT_ASTC_10x6_SRGB_BLOCK";
      case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
        return "VK_FORMAT_ASTC_10x8_UNORM_BLOCK";
      case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
        return "VK_FORMAT_ASTC_10x8_SRGB_BLOCK";
      case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
        return "VK_FORMAT_ASTC_10x10_UNORM_BLOCK";
      case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
        return "VK_FORMAT_ASTC_10x10_SRGB_BLOCK";
      case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
        return "VK_FORMAT_ASTC_12x10_UNORM_BLOCK";
      case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
        return "VK_FORMAT_ASTC_12x10_SRGB_BLOCK";
      case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
        return "VK_FORMAT_ASTC_12x12_UNORM_BLOCK";
      case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
        return "VK_FORMAT_ASTC_12x12_SRGB_BLOCK";
      case VK_FORMAT_G8B8G8R8_422_UNORM:
        return "VK_FORMAT_G8B8G8R8_422_UNORM";
      case VK_FORMAT_B8G8R8G8_422_UNORM:
        return "VK_FORMAT_B8G8R8G8_422_UNORM";
      case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
        return "VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM";
      case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
        return "VK_FORMAT_G8_B8R8_2PLANE_420_UNORM";
      case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
        return "VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM";
      case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
        return "VK_FORMAT_G8_B8R8_2PLANE_422_UNORM";
      case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
        return "VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM";
      case VK_FORMAT_R10X6_UNORM_PACK16:
        return "VK_FORMAT_R10X6_UNORM_PACK16";
      case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:
        return "VK_FORMAT_R10X6G10X6_UNORM_2PACK16";
      case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:
        return "VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16";
      case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
        return "VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16";
      case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
        return "VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16";
      case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
        return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16";
      case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
        return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16";
      case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
        return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16";
      case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
        return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16";
      case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
        return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16";
      case VK_FORMAT_R12X4_UNORM_PACK16:
        return "VK_FORMAT_R12X4_UNORM_PACK16";
      case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:
        return "VK_FORMAT_R12X4G12X4_UNORM_2PACK16";
      case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:
        return "VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16";
      case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
        return "VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16";
      case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
        return "VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16";
      case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
        return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16";
      case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
        return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16";
      case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
        return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16";
      case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
        return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16";
      case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
        return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16";
      case VK_FORMAT_G16B16G16R16_422_UNORM:
        return "VK_FORMAT_G16B16G16R16_422_UNORM";
      case VK_FORMAT_B16G16R16G16_422_UNORM:
        return "VK_FORMAT_B16G16R16G16_422_UNORM";
      case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
        return "VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM";
      case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
        return "VK_FORMAT_G16_B16R16_2PLANE_420_UNORM";
      case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
        return "VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM";
      case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
        return "VK_FORMAT_G16_B16R16_2PLANE_422_UNORM";
      case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
        return "VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM";
      case VK_FORMAT_G8_B8R8_2PLANE_444_UNORM:
        return "VK_FORMAT_G8_B8R8_2PLANE_444_UNORM";
      case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16:
        return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16";
      case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16:
        return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16";
      case VK_FORMAT_G16_B16R16_2PLANE_444_UNORM:
        return "VK_FORMAT_G16_B16R16_2PLANE_444_UNORM";
      case VK_FORMAT_A4R4G4B4_UNORM_PACK16:
        return "VK_FORMAT_A4R4G4B4_UNORM_PACK16";
      case VK_FORMAT_A4B4G4R4_UNORM_PACK16:
        return "VK_FORMAT_A4B4G4R4_UNORM_PACK16";
      case VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK:
        return "VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK";
      case VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK:
        return "VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK";
      case VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK:
        return "VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK";
      case VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK:
        return "VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK";
      case VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK:
        return "VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK";
      case VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK:
        return "VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK";
      case VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK:
        return "VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK";
      case VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK:
        return "VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK";
      case VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK:
        return "VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK";
      case VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK:
        return "VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK";
      case VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK:
        return "VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK";
      case VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK:
        return "VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK";
      case VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK:
        return "VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK";
      case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK:
        return "VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK";
      case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
        return "VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG";
      case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
        return "VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG";
      case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
        return "VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG";
      case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
        return "VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG";
      case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
        return "VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG";
      case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
        return "VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG";
      case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
        return "VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG";
      case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
        return "VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG";
      case VK_FORMAT_ASTC_3x3x3_UNORM_BLOCK_EXT:
        return "VK_FORMAT_ASTC_3x3x3_UNORM_BLOCK_EXT";
      case VK_FORMAT_ASTC_3x3x3_SRGB_BLOCK_EXT:
        return "VK_FORMAT_ASTC_3x3x3_SRGB_BLOCK_EXT";
      case VK_FORMAT_ASTC_3x3x3_SFLOAT_BLOCK_EXT:
        return "VK_FORMAT_ASTC_3x3x3_SFLOAT_BLOCK_EXT";
      case VK_FORMAT_ASTC_4x3x3_UNORM_BLOCK_EXT:
        return "VK_FORMAT_ASTC_4x3x3_UNORM_BLOCK_EXT";
      case VK_FORMAT_ASTC_4x3x3_SRGB_BLOCK_EXT:
        return "VK_FORMAT_ASTC_4x3x3_SRGB_BLOCK_EXT";
      case VK_FORMAT_ASTC_4x3x3_SFLOAT_BLOCK_EXT:
        return "VK_FORMAT_ASTC_4x3x3_SFLOAT_BLOCK_EXT";
      case VK_FORMAT_ASTC_4x4x3_UNORM_BLOCK_EXT:
        return "VK_FORMAT_ASTC_4x4x3_UNORM_BLOCK_EXT";
      case VK_FORMAT_ASTC_4x4x3_SRGB_BLOCK_EXT:
        return "VK_FORMAT_ASTC_4x4x3_SRGB_BLOCK_EXT";
      case VK_FORMAT_ASTC_4x4x3_SFLOAT_BLOCK_EXT:
        return "VK_FORMAT_ASTC_4x4x3_SFLOAT_BLOCK_EXT";
      case VK_FORMAT_ASTC_4x4x4_UNORM_BLOCK_EXT:
        return "VK_FORMAT_ASTC_4x4x4_UNORM_BLOCK_EXT";
      case VK_FORMAT_ASTC_4x4x4_SRGB_BLOCK_EXT:
        return "VK_FORMAT_ASTC_4x4x4_SRGB_BLOCK_EXT";
      case VK_FORMAT_ASTC_4x4x4_SFLOAT_BLOCK_EXT:
        return "VK_FORMAT_ASTC_4x4x4_SFLOAT_BLOCK_EXT";
      case VK_FORMAT_ASTC_5x4x4_UNORM_BLOCK_EXT:
        return "VK_FORMAT_ASTC_5x4x4_UNORM_BLOCK_EXT";
      case VK_FORMAT_ASTC_5x4x4_SRGB_BLOCK_EXT:
        return "VK_FORMAT_ASTC_5x4x4_SRGB_BLOCK_EXT";
      case VK_FORMAT_ASTC_5x4x4_SFLOAT_BLOCK_EXT:
        return "VK_FORMAT_ASTC_5x4x4_SFLOAT_BLOCK_EXT";
      case VK_FORMAT_ASTC_5x5x4_UNORM_BLOCK_EXT:
        return "VK_FORMAT_ASTC_5x5x4_UNORM_BLOCK_EXT";
      case VK_FORMAT_ASTC_5x5x4_SRGB_BLOCK_EXT:
        return "VK_FORMAT_ASTC_5x5x4_SRGB_BLOCK_EXT";
      case VK_FORMAT_ASTC_5x5x4_SFLOAT_BLOCK_EXT:
        return "VK_FORMAT_ASTC_5x5x4_SFLOAT_BLOCK_EXT";
      case VK_FORMAT_ASTC_5x5x5_UNORM_BLOCK_EXT:
        return "VK_FORMAT_ASTC_5x5x5_UNORM_BLOCK_EXT";
      case VK_FORMAT_ASTC_5x5x5_SRGB_BLOCK_EXT:
        return "VK_FORMAT_ASTC_5x5x5_SRGB_BLOCK_EXT";
      case VK_FORMAT_ASTC_5x5x5_SFLOAT_BLOCK_EXT:
        return "VK_FORMAT_ASTC_5x5x5_SFLOAT_BLOCK_EXT";
      case VK_FORMAT_ASTC_6x5x5_UNORM_BLOCK_EXT:
        return "VK_FORMAT_ASTC_6x5x5_UNORM_BLOCK_EXT";
      case VK_FORMAT_ASTC_6x5x5_SRGB_BLOCK_EXT:
        return "VK_FORMAT_ASTC_6x5x5_SRGB_BLOCK_EXT";
      case VK_FORMAT_ASTC_6x5x5_SFLOAT_BLOCK_EXT:
        return "VK_FORMAT_ASTC_6x5x5_SFLOAT_BLOCK_EXT";
      case VK_FORMAT_ASTC_6x6x5_UNORM_BLOCK_EXT:
        return "VK_FORMAT_ASTC_6x6x5_UNORM_BLOCK_EXT";
      case VK_FORMAT_ASTC_6x6x5_SRGB_BLOCK_EXT:
        return "VK_FORMAT_ASTC_6x6x5_SRGB_BLOCK_EXT";
      case VK_FORMAT_ASTC_6x6x5_SFLOAT_BLOCK_EXT:
        return "VK_FORMAT_ASTC_6x6x5_SFLOAT_BLOCK_EXT";
      case VK_FORMAT_ASTC_6x6x6_UNORM_BLOCK_EXT:
        return "VK_FORMAT_ASTC_6x6x6_UNORM_BLOCK_EXT";
      case VK_FORMAT_ASTC_6x6x6_SRGB_BLOCK_EXT:
        return "VK_FORMAT_ASTC_6x6x6_SRGB_BLOCK_EXT";
      case VK_FORMAT_ASTC_6x6x6_SFLOAT_BLOCK_EXT:
        return "VK_FORMAT_ASTC_6x6x6_SFLOAT_BLOCK_EXT";
      case VK_FORMAT_R16G16_S10_5_NV:
        return "VK_FORMAT_R16G16_S10_5_NV";
      case VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR:
        return "VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR";
      case VK_FORMAT_A8_UNORM_KHR:
        return "VK_FORMAT_A8_UNORM_KHR";
      default:
        return "VK_UNKNOWN_FORMAT";
    }
}

static int ktx_strcasecmp(const char* s1, const char* s2) {
    const unsigned char* us1 = (const unsigned char*) s1;
    const unsigned char* us2 = (const unsigned char*) s2;

    while (tolower(*us1) == tolower(*us2)) {
        if (*us1 == '\0')
            return 0;
        ++us1;
        ++us2;
    }
    return tolower(*us1) - tolower(*us2);
}

static int ktx_strncasecmp(const char* s1, const char* s2, int length) {
    const unsigned char* us1 = (const unsigned char*) s1;
    const unsigned char* us2 = (const unsigned char*) s2;

    while (length > 0 && tolower(*us1) == tolower(*us2)) {
        if (*us1 == '\0')
            return 0;
        ++us1;
        ++us2;
        --length;
    }
    if (length == 0)
        return 0;
    return tolower(*us1) - tolower(*us2);
}

/// Parses a VkFormat. VK_FORMAT_ prefix is optional. Case insensitive.
VkFormat
stringToVkFormat(const char* str)
{
    if (ktx_strncasecmp(str, "VK_FORMAT_", sizeof("VK_FORMAT_") - 1) == 0)
        str += sizeof("VK_FORMAT_") - 1;

    if (ktx_strcasecmp(str, "UNDEFINED") == 0)
        return VK_FORMAT_UNDEFINED;
    if (ktx_strcasecmp(str, "R4G4_UNORM_PACK8") == 0)
        return VK_FORMAT_R4G4_UNORM_PACK8;
    if (ktx_strcasecmp(str, "R4G4B4A4_UNORM_PACK16") == 0)
        return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
    if (ktx_strcasecmp(str, "B4G4R4A4_UNORM_PACK16") == 0)
        return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
    if (ktx_strcasecmp(str, "R5G6B5_UNORM_PACK16") == 0)
        return VK_FORMAT_R5G6B5_UNORM_PACK16;
    if (ktx_strcasecmp(str, "B5G6R5_UNORM_PACK16") == 0)
        return VK_FORMAT_B5G6R5_UNORM_PACK16;
    if (ktx_strcasecmp(str, "R5G5B5A1_UNORM_PACK16") == 0)
        return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
    if (ktx_strcasecmp(str, "B5G5R5A1_UNORM_PACK16") == 0)
        return VK_FORMAT_B5G5R5A1_UNORM_PACK16;
    if (ktx_strcasecmp(str, "A1R5G5B5_UNORM_PACK16") == 0)
        return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
    if (ktx_strcasecmp(str, "R8_UNORM") == 0)
        return VK_FORMAT_R8_UNORM;
    if (ktx_strcasecmp(str, "R8_SNORM") == 0)
        return VK_FORMAT_R8_SNORM;
    if (ktx_strcasecmp(str, "R8_USCALED") == 0)
        return VK_FORMAT_R8_USCALED;
    if (ktx_strcasecmp(str, "R8_SSCALED") == 0)
        return VK_FORMAT_R8_SSCALED;
    if (ktx_strcasecmp(str, "R8_UINT") == 0)
        return VK_FORMAT_R8_UINT;
    if (ktx_strcasecmp(str, "R8_SINT") == 0)
        return VK_FORMAT_R8_SINT;
    if (ktx_strcasecmp(str, "R8_SRGB") == 0)
        return VK_FORMAT_R8_SRGB;
    if (ktx_strcasecmp(str, "R8G8_UNORM") == 0)
        return VK_FORMAT_R8G8_UNORM;
    if (ktx_strcasecmp(str, "R8G8_SNORM") == 0)
        return VK_FORMAT_R8G8_SNORM;
    if (ktx_strcasecmp(str, "R8G8_USCALED") == 0)
        return VK_FORMAT_R8G8_USCALED;
    if (ktx_strcasecmp(str, "R8G8_SSCALED") == 0)
        return VK_FORMAT_R8G8_SSCALED;
    if (ktx_strcasecmp(str, "R8G8_UINT") == 0)
        return VK_FORMAT_R8G8_UINT;
    if (ktx_strcasecmp(str, "R8G8_SINT") == 0)
        return VK_FORMAT_R8G8_SINT;
    if (ktx_strcasecmp(str, "R8G8_SRGB") == 0)
        return VK_FORMAT_R8G8_SRGB;
    if (ktx_strcasecmp(str, "R8G8B8_UNORM") == 0)
        return VK_FORMAT_R8G8B8_UNORM;
    if (ktx_strcasecmp(str, "R8G8B8_SNORM") == 0)
        return VK_FORMAT_R8G8B8_SNORM;
    if (ktx_strcasecmp(str, "R8G8B8_USCALED") == 0)
        return VK_FORMAT_R8G8B8_USCALED;
    if (ktx_strcasecmp(str, "R8G8B8_SSCALED") == 0)
        return VK_FORMAT_R8G8B8_SSCALED;
    if (ktx_strcasecmp(str, "R8G8B8_UINT") == 0)
        return VK_FORMAT_R8G8B8_UINT;
    if (ktx_strcasecmp(str, "R8G8B8_SINT") == 0)
        return VK_FORMAT_R8G8B8_SINT;
    if (ktx_strcasecmp(str, "R8G8B8_SRGB") == 0)
        return VK_FORMAT_R8G8B8_SRGB;
    if (ktx_strcasecmp(str, "B8G8R8_UNORM") == 0)
        return VK_FORMAT_B8G8R8_UNORM;
    if (ktx_strcasecmp(str, "B8G8R8_SNORM") == 0)
        return VK_FORMAT_B8G8R8_SNORM;
    if (ktx_strcasecmp(str, "B8G8R8_USCALED") == 0)
        return VK_FORMAT_B8G8R8_USCALED;
    if (ktx_strcasecmp(str, "B8G8R8_SSCALED") == 0)
        return VK_FORMAT_B8G8R8_SSCALED;
    if (ktx_strcasecmp(str, "B8G8R8_UINT") == 0)
        return VK_FORMAT_B8G8R8_UINT;
    if (ktx_strcasecmp(str, "B8G8R8_SINT") == 0)
        return VK_FORMAT_B8G8R8_SINT;
    if (ktx_strcasecmp(str, "B8G8R8_SRGB") == 0)
        return VK_FORMAT_B8G8R8_SRGB;
    if (ktx_strcasecmp(str, "R8G8B8A8_UNORM") == 0)
        return VK_FORMAT_R8G8B8A8_UNORM;
    if (ktx_strcasecmp(str, "R8G8B8A8_SNORM") == 0)
        return VK_FORMAT_R8G8B8A8_SNORM;
    if (ktx_strcasecmp(str, "R8G8B8A8_USCALED") == 0)
        return VK_FORMAT_R8G8B8A8_USCALED;
    if (ktx_strcasecmp(str, "R8G8B8A8_SSCALED") == 0)
        return VK_FORMAT_R8G8B8A8_SSCALED;
    if (ktx_strcasecmp(str, "R8G8B8A8_UINT") == 0)
        return VK_FORMAT_R8G8B8A8_UINT;
    if (ktx_strcasecmp(str, "R8G8B8A8_SINT") == 0)
        return VK_FORMAT_R8G8B8A8_SINT;
    if (ktx_strcasecmp(str, "R8G8B8A8_SRGB") == 0)
        return VK_FORMAT_R8G8B8A8_SRGB;
    if (ktx_strcasecmp(str, "B8G8R8A8_UNORM") == 0)
        return VK_FORMAT_B8G8R8A8_UNORM;
    if (ktx_strcasecmp(str, "B8G8R8A8_SNORM") == 0)
        return VK_FORMAT_B8G8R8A8_SNORM;
    if (ktx_strcasecmp(str, "B8G8R8A8_USCALED") == 0)
        return VK_FORMAT_B8G8R8A8_USCALED;
    if (ktx_strcasecmp(str, "B8G8R8A8_SSCALED") == 0)
        return VK_FORMAT_B8G8R8A8_SSCALED;
    if (ktx_strcasecmp(str, "B8G8R8A8_UINT") == 0)
        return VK_FORMAT_B8G8R8A8_UINT;
    if (ktx_strcasecmp(str, "B8G8R8A8_SINT") == 0)
        return VK_FORMAT_B8G8R8A8_SINT;
    if (ktx_strcasecmp(str, "B8G8R8A8_SRGB") == 0)
        return VK_FORMAT_B8G8R8A8_SRGB;
    if (ktx_strcasecmp(str, "A8B8G8R8_UNORM_PACK32") == 0)
        return VK_FORMAT_A8B8G8R8_UNORM_PACK32;
    if (ktx_strcasecmp(str, "A8B8G8R8_SNORM_PACK32") == 0)
        return VK_FORMAT_A8B8G8R8_SNORM_PACK32;
    if (ktx_strcasecmp(str, "A8B8G8R8_USCALED_PACK32") == 0)
        return VK_FORMAT_A8B8G8R8_USCALED_PACK32;
    if (ktx_strcasecmp(str, "A8B8G8R8_SSCALED_PACK32") == 0)
        return VK_FORMAT_A8B8G8R8_SSCALED_PACK32;
    if (ktx_strcasecmp(str, "A8B8G8R8_UINT_PACK32") == 0)
        return VK_FORMAT_A8B8G8R8_UINT_PACK32;
    if (ktx_strcasecmp(str, "A8B8G8R8_SINT_PACK32") == 0)
        return VK_FORMAT_A8B8G8R8_SINT_PACK32;
    if (ktx_strcasecmp(str, "A8B8G8R8_SRGB_PACK32") == 0)
        return VK_FORMAT_A8B8G8R8_SRGB_PACK32;
    if (ktx_strcasecmp(str, "A2R10G10B10_UNORM_PACK32") == 0)
        return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
    if (ktx_strcasecmp(str, "A2R10G10B10_SNORM_PACK32") == 0)
        return VK_FORMAT_A2R10G10B10_SNORM_PACK32;
    if (ktx_strcasecmp(str, "A2R10G10B10_USCALED_PACK32") == 0)
        return VK_FORMAT_A2R10G10B10_USCALED_PACK32;
    if (ktx_strcasecmp(str, "A2R10G10B10_SSCALED_PACK32") == 0)
        return VK_FORMAT_A2R10G10B10_SSCALED_PACK32;
    if (ktx_strcasecmp(str, "A2R10G10B10_UINT_PACK32") == 0)
        return VK_FORMAT_A2R10G10B10_UINT_PACK32;
    if (ktx_strcasecmp(str, "A2R10G10B10_SINT_PACK32") == 0)
        return VK_FORMAT_A2R10G10B10_SINT_PACK32;
    if (ktx_strcasecmp(str, "A2B10G10R10_UNORM_PACK32") == 0)
        return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
    if (ktx_strcasecmp(str, "A2B10G10R10_SNORM_PACK32") == 0)
        return VK_FORMAT_A2B10G10R10_SNORM_PACK32;
    if (ktx_strcasecmp(str, "A2B10G10R10_USCALED_PACK32") == 0)
        return VK_FORMAT_A2B10G10R10_USCALED_PACK32;
    if (ktx_strcasecmp(str, "A2B10G10R10_SSCALED_PACK32") == 0)
        return VK_FORMAT_A2B10G10R10_SSCALED_PACK32;
    if (ktx_strcasecmp(str, "A2B10G10R10_UINT_PACK32") == 0)
        return VK_FORMAT_A2B10G10R10_UINT_PACK32;
    if (ktx_strcasecmp(str, "A2B10G10R10_SINT_PACK32") == 0)
        return VK_FORMAT_A2B10G10R10_SINT_PACK32;
    if (ktx_strcasecmp(str, "R16_UNORM") == 0)
        return VK_FORMAT_R16_UNORM;
    if (ktx_strcasecmp(str, "R16_SNORM") == 0)
        return VK_FORMAT_R16_SNORM;
    if (ktx_strcasecmp(str, "R16_USCALED") == 0)
        return VK_FORMAT_R16_USCALED;
    if (ktx_strcasecmp(str, "R16_SSCALED") == 0)
        return VK_FORMAT_R16_SSCALED;
    if (ktx_strcasecmp(str, "R16_UINT") == 0)
        return VK_FORMAT_R16_UINT;
    if (ktx_strcasecmp(str, "R16_SINT") == 0)
        return VK_FORMAT_R16_SINT;
    if (ktx_strcasecmp(str, "R16_SFLOAT") == 0)
        return VK_FORMAT_R16_SFLOAT;
    if (ktx_strcasecmp(str, "R16G16_UNORM") == 0)
        return VK_FORMAT_R16G16_UNORM;
    if (ktx_strcasecmp(str, "R16G16_SNORM") == 0)
        return VK_FORMAT_R16G16_SNORM;
    if (ktx_strcasecmp(str, "R16G16_USCALED") == 0)
        return VK_FORMAT_R16G16_USCALED;
    if (ktx_strcasecmp(str, "R16G16_SSCALED") == 0)
        return VK_FORMAT_R16G16_SSCALED;
    if (ktx_strcasecmp(str, "R16G16_UINT") == 0)
        return VK_FORMAT_R16G16_UINT;
    if (ktx_strcasecmp(str, "R16G16_SINT") == 0)
        return VK_FORMAT_R16G16_SINT;
    if (ktx_strcasecmp(str, "R16G16_SFLOAT") == 0)
        return VK_FORMAT_R16G16_SFLOAT;
    if (ktx_strcasecmp(str, "R16G16B16_UNORM") == 0)
        return VK_FORMAT_R16G16B16_UNORM;
    if (ktx_strcasecmp(str, "R16G16B16_SNORM") == 0)
        return VK_FORMAT_R16G16B16_SNORM;
    if (ktx_strcasecmp(str, "R16G16B16_USCALED") == 0)
        return VK_FORMAT_R16G16B16_USCALED;
    if (ktx_strcasecmp(str, "R16G16B16_SSCALED") == 0)
        return VK_FORMAT_R16G16B16_SSCALED;
    if (ktx_strcasecmp(str, "R16G16B16_UINT") == 0)
        return VK_FORMAT_R16G16B16_UINT;
    if (ktx_strcasecmp(str, "R16G16B16_SINT") == 0)
        return VK_FORMAT_R16G16B16_SINT;
    if (ktx_strcasecmp(str, "R16G16B16_SFLOAT") == 0)
        return VK_FORMAT_R16G16B16_SFLOAT;
    if (ktx_strcasecmp(str, "R16G16B16A16_UNORM") == 0)
        return VK_FORMAT_R16G16B16A16_UNORM;
    if (ktx_strcasecmp(str, "R16G16B16A16_SNORM") == 0)
        return VK_FORMAT_R16G16B16A16_SNORM;
    if (ktx_strcasecmp(str, "R16G16B16A16_USCALED") == 0)
        return VK_FORMAT_R16G16B16A16_USCALED;
    if (ktx_strcasecmp(str, "R16G16B16A16_SSCALED") == 0)
        return VK_FORMAT_R16G16B16A16_SSCALED;
    if (ktx_strcasecmp(str, "R16G16B16A16_UINT") == 0)
        return VK_FORMAT_R16G16B16A16_UINT;
    if (ktx_strcasecmp(str, "R16G16B16A16_SINT") == 0)
        return VK_FORMAT_R16G16B16A16_SINT;
    if (ktx_strcasecmp(str, "R16G16B16A16_SFLOAT") == 0)
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    if (ktx_strcasecmp(str, "R32_UINT") == 0)
        return VK_FORMAT_R32_UINT;
    if (ktx_strcasecmp(str, "R32_SINT") == 0)
        return VK_FORMAT_R32_SINT;
    if (ktx_strcasecmp(str, "R32_SFLOAT") == 0)
        return VK_FORMAT_R32_SFLOAT;
    if (ktx_strcasecmp(str, "R32G32_UINT") == 0)
        return VK_FORMAT_R32G32_UINT;
    if (ktx_strcasecmp(str, "R32G32_SINT") == 0)
        return VK_FORMAT_R32G32_SINT;
    if (ktx_strcasecmp(str, "R32G32_SFLOAT") == 0)
        return VK_FORMAT_R32G32_SFLOAT;
    if (ktx_strcasecmp(str, "R32G32B32_UINT") == 0)
        return VK_FORMAT_R32G32B32_UINT;
    if (ktx_strcasecmp(str, "R32G32B32_SINT") == 0)
        return VK_FORMAT_R32G32B32_SINT;
    if (ktx_strcasecmp(str, "R32G32B32_SFLOAT") == 0)
        return VK_FORMAT_R32G32B32_SFLOAT;
    if (ktx_strcasecmp(str, "R32G32B32A32_UINT") == 0)
        return VK_FORMAT_R32G32B32A32_UINT;
    if (ktx_strcasecmp(str, "R32G32B32A32_SINT") == 0)
        return VK_FORMAT_R32G32B32A32_SINT;
    if (ktx_strcasecmp(str, "R32G32B32A32_SFLOAT") == 0)
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    if (ktx_strcasecmp(str, "R64_UINT") == 0)
        return VK_FORMAT_R64_UINT;
    if (ktx_strcasecmp(str, "R64_SINT") == 0)
        return VK_FORMAT_R64_SINT;
    if (ktx_strcasecmp(str, "R64_SFLOAT") == 0)
        return VK_FORMAT_R64_SFLOAT;
    if (ktx_strcasecmp(str, "R64G64_UINT") == 0)
        return VK_FORMAT_R64G64_UINT;
    if (ktx_strcasecmp(str, "R64G64_SINT") == 0)
        return VK_FORMAT_R64G64_SINT;
    if (ktx_strcasecmp(str, "R64G64_SFLOAT") == 0)
        return VK_FORMAT_R64G64_SFLOAT;
    if (ktx_strcasecmp(str, "R64G64B64_UINT") == 0)
        return VK_FORMAT_R64G64B64_UINT;
    if (ktx_strcasecmp(str, "R64G64B64_SINT") == 0)
        return VK_FORMAT_R64G64B64_SINT;
    if (ktx_strcasecmp(str, "R64G64B64_SFLOAT") == 0)
        return VK_FORMAT_R64G64B64_SFLOAT;
    if (ktx_strcasecmp(str, "R64G64B64A64_UINT") == 0)
        return VK_FORMAT_R64G64B64A64_UINT;
    if (ktx_strcasecmp(str, "R64G64B64A64_SINT") == 0)
        return VK_FORMAT_R64G64B64A64_SINT;
    if (ktx_strcasecmp(str, "R64G64B64A64_SFLOAT") == 0)
        return VK_FORMAT_R64G64B64A64_SFLOAT;
    if (ktx_strcasecmp(str, "B10G11R11_UFLOAT_PACK32") == 0)
        return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
    if (ktx_strcasecmp(str, "E5B9G9R9_UFLOAT_PACK32") == 0)
        return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
    if (ktx_strcasecmp(str, "D16_UNORM") == 0)
        return VK_FORMAT_D16_UNORM;
    if (ktx_strcasecmp(str, "X8_D24_UNORM_PACK32") == 0)
        return VK_FORMAT_X8_D24_UNORM_PACK32;
    if (ktx_strcasecmp(str, "D32_SFLOAT") == 0)
        return VK_FORMAT_D32_SFLOAT;
    if (ktx_strcasecmp(str, "S8_UINT") == 0)
        return VK_FORMAT_S8_UINT;
    if (ktx_strcasecmp(str, "D16_UNORM_S8_UINT") == 0)
        return VK_FORMAT_D16_UNORM_S8_UINT;
    if (ktx_strcasecmp(str, "D24_UNORM_S8_UINT") == 0)
        return VK_FORMAT_D24_UNORM_S8_UINT;
    if (ktx_strcasecmp(str, "D32_SFLOAT_S8_UINT") == 0)
        return VK_FORMAT_D32_SFLOAT_S8_UINT;
    if (ktx_strcasecmp(str, "BC1_RGB_UNORM_BLOCK") == 0)
        return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "BC1_RGB_SRGB_BLOCK") == 0)
        return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
    if (ktx_strcasecmp(str, "BC1_RGBA_UNORM_BLOCK") == 0)
        return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "BC1_RGBA_SRGB_BLOCK") == 0)
        return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
    if (ktx_strcasecmp(str, "BC2_UNORM_BLOCK") == 0)
        return VK_FORMAT_BC2_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "BC2_SRGB_BLOCK") == 0)
        return VK_FORMAT_BC2_SRGB_BLOCK;
    if (ktx_strcasecmp(str, "BC3_UNORM_BLOCK") == 0)
        return VK_FORMAT_BC3_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "BC3_SRGB_BLOCK") == 0)
        return VK_FORMAT_BC3_SRGB_BLOCK;
    if (ktx_strcasecmp(str, "BC4_UNORM_BLOCK") == 0)
        return VK_FORMAT_BC4_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "BC4_SNORM_BLOCK") == 0)
        return VK_FORMAT_BC4_SNORM_BLOCK;
    if (ktx_strcasecmp(str, "BC5_UNORM_BLOCK") == 0)
        return VK_FORMAT_BC5_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "BC5_SNORM_BLOCK") == 0)
        return VK_FORMAT_BC5_SNORM_BLOCK;
    if (ktx_strcasecmp(str, "BC6H_UFLOAT_BLOCK") == 0)
        return VK_FORMAT_BC6H_UFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "BC6H_SFLOAT_BLOCK") == 0)
        return VK_FORMAT_BC6H_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "BC7_UNORM_BLOCK") == 0)
        return VK_FORMAT_BC7_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "BC7_SRGB_BLOCK") == 0)
        return VK_FORMAT_BC7_SRGB_BLOCK;
    if (ktx_strcasecmp(str, "ETC2_R8G8B8_UNORM_BLOCK") == 0)
        return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "ETC2_R8G8B8_SRGB_BLOCK") == 0)
        return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
    if (ktx_strcasecmp(str, "ETC2_R8G8B8A1_UNORM_BLOCK") == 0)
        return VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "ETC2_R8G8B8A1_SRGB_BLOCK") == 0)
        return VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;
    if (ktx_strcasecmp(str, "ETC2_R8G8B8A8_UNORM_BLOCK") == 0)
        return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "ETC2_R8G8B8A8_SRGB_BLOCK") == 0)
        return VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;
    if (ktx_strcasecmp(str, "EAC_R11_UNORM_BLOCK") == 0)
        return VK_FORMAT_EAC_R11_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "EAC_R11_SNORM_BLOCK") == 0)
        return VK_FORMAT_EAC_R11_SNORM_BLOCK;
    if (ktx_strcasecmp(str, "EAC_R11G11_UNORM_BLOCK") == 0)
        return VK_FORMAT_EAC_R11G11_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "EAC_R11G11_SNORM_BLOCK") == 0)
        return VK_FORMAT_EAC_R11G11_SNORM_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_4x4_UNORM_BLOCK") == 0)
        return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_4x4_SRGB_BLOCK") == 0)
        return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_5x4_UNORM_BLOCK") == 0)
        return VK_FORMAT_ASTC_5x4_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_5x4_SRGB_BLOCK") == 0)
        return VK_FORMAT_ASTC_5x4_SRGB_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_5x5_UNORM_BLOCK") == 0)
        return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_5x5_SRGB_BLOCK") == 0)
        return VK_FORMAT_ASTC_5x5_SRGB_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_6x5_UNORM_BLOCK") == 0)
        return VK_FORMAT_ASTC_6x5_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_6x5_SRGB_BLOCK") == 0)
        return VK_FORMAT_ASTC_6x5_SRGB_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_6x6_UNORM_BLOCK") == 0)
        return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_6x6_SRGB_BLOCK") == 0)
        return VK_FORMAT_ASTC_6x6_SRGB_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_8x5_UNORM_BLOCK") == 0)
        return VK_FORMAT_ASTC_8x5_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_8x5_SRGB_BLOCK") == 0)
        return VK_FORMAT_ASTC_8x5_SRGB_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_8x6_UNORM_BLOCK") == 0)
        return VK_FORMAT_ASTC_8x6_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_8x6_SRGB_BLOCK") == 0)
        return VK_FORMAT_ASTC_8x6_SRGB_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_8x8_UNORM_BLOCK") == 0)
        return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_8x8_SRGB_BLOCK") == 0)
        return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_10x5_UNORM_BLOCK") == 0)
        return VK_FORMAT_ASTC_10x5_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_10x5_SRGB_BLOCK") == 0)
        return VK_FORMAT_ASTC_10x5_SRGB_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_10x6_UNORM_BLOCK") == 0)
        return VK_FORMAT_ASTC_10x6_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_10x6_SRGB_BLOCK") == 0)
        return VK_FORMAT_ASTC_10x6_SRGB_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_10x8_UNORM_BLOCK") == 0)
        return VK_FORMAT_ASTC_10x8_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_10x8_SRGB_BLOCK") == 0)
        return VK_FORMAT_ASTC_10x8_SRGB_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_10x10_UNORM_BLOCK") == 0)
        return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_10x10_SRGB_BLOCK") == 0)
        return VK_FORMAT_ASTC_10x10_SRGB_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_12x10_UNORM_BLOCK") == 0)
        return VK_FORMAT_ASTC_12x10_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_12x10_SRGB_BLOCK") == 0)
        return VK_FORMAT_ASTC_12x10_SRGB_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_12x12_UNORM_BLOCK") == 0)
        return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_12x12_SRGB_BLOCK") == 0)
        return VK_FORMAT_ASTC_12x12_SRGB_BLOCK;
    if (ktx_strcasecmp(str, "G8B8G8R8_422_UNORM") == 0)
        return VK_FORMAT_G8B8G8R8_422_UNORM;
    if (ktx_strcasecmp(str, "B8G8R8G8_422_UNORM") == 0)
        return VK_FORMAT_B8G8R8G8_422_UNORM;
    if (ktx_strcasecmp(str, "G8_B8_R8_3PLANE_420_UNORM") == 0)
        return VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM;
    if (ktx_strcasecmp(str, "G8_B8R8_2PLANE_420_UNORM") == 0)
        return VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
    if (ktx_strcasecmp(str, "G8_B8_R8_3PLANE_422_UNORM") == 0)
        return VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM;
    if (ktx_strcasecmp(str, "G8_B8R8_2PLANE_422_UNORM") == 0)
        return VK_FORMAT_G8_B8R8_2PLANE_422_UNORM;
    if (ktx_strcasecmp(str, "G8_B8_R8_3PLANE_444_UNORM") == 0)
        return VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM;
    if (ktx_strcasecmp(str, "R10X6_UNORM_PACK16") == 0)
        return VK_FORMAT_R10X6_UNORM_PACK16;
    if (ktx_strcasecmp(str, "R10X6G10X6_UNORM_2PACK16") == 0)
        return VK_FORMAT_R10X6G10X6_UNORM_2PACK16;
    if (ktx_strcasecmp(str, "R10X6G10X6B10X6A10X6_UNORM_4PACK16") == 0)
        return VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16;
    if (ktx_strcasecmp(str, "G10X6B10X6G10X6R10X6_422_UNORM_4PACK16") == 0)
        return VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16;
    if (ktx_strcasecmp(str, "B10X6G10X6R10X6G10X6_422_UNORM_4PACK16") == 0)
        return VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16;
    if (ktx_strcasecmp(str, "G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16") == 0)
        return VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16;
    if (ktx_strcasecmp(str, "G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16") == 0)
        return VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16;
    if (ktx_strcasecmp(str, "G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16") == 0)
        return VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16;
    if (ktx_strcasecmp(str, "G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16") == 0)
        return VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16;
    if (ktx_strcasecmp(str, "G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16") == 0)
        return VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16;
    if (ktx_strcasecmp(str, "R12X4_UNORM_PACK16") == 0)
        return VK_FORMAT_R12X4_UNORM_PACK16;
    if (ktx_strcasecmp(str, "R12X4G12X4_UNORM_2PACK16") == 0)
        return VK_FORMAT_R12X4G12X4_UNORM_2PACK16;
    if (ktx_strcasecmp(str, "R12X4G12X4B12X4A12X4_UNORM_4PACK16") == 0)
        return VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16;
    if (ktx_strcasecmp(str, "G12X4B12X4G12X4R12X4_422_UNORM_4PACK16") == 0)
        return VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16;
    if (ktx_strcasecmp(str, "B12X4G12X4R12X4G12X4_422_UNORM_4PACK16") == 0)
        return VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16;
    if (ktx_strcasecmp(str, "G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16") == 0)
        return VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16;
    if (ktx_strcasecmp(str, "G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16") == 0)
        return VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16;
    if (ktx_strcasecmp(str, "G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16") == 0)
        return VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16;
    if (ktx_strcasecmp(str, "G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16") == 0)
        return VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16;
    if (ktx_strcasecmp(str, "G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16") == 0)
        return VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16;
    if (ktx_strcasecmp(str, "G16B16G16R16_422_UNORM") == 0)
        return VK_FORMAT_G16B16G16R16_422_UNORM;
    if (ktx_strcasecmp(str, "B16G16R16G16_422_UNORM") == 0)
        return VK_FORMAT_B16G16R16G16_422_UNORM;
    if (ktx_strcasecmp(str, "G16_B16_R16_3PLANE_420_UNORM") == 0)
        return VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM;
    if (ktx_strcasecmp(str, "G16_B16R16_2PLANE_420_UNORM") == 0)
        return VK_FORMAT_G16_B16R16_2PLANE_420_UNORM;
    if (ktx_strcasecmp(str, "G16_B16_R16_3PLANE_422_UNORM") == 0)
        return VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM;
    if (ktx_strcasecmp(str, "G16_B16R16_2PLANE_422_UNORM") == 0)
        return VK_FORMAT_G16_B16R16_2PLANE_422_UNORM;
    if (ktx_strcasecmp(str, "G16_B16_R16_3PLANE_444_UNORM") == 0)
        return VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM;
    if (ktx_strcasecmp(str, "G8_B8R8_2PLANE_444_UNORM") == 0)
        return VK_FORMAT_G8_B8R8_2PLANE_444_UNORM;
    if (ktx_strcasecmp(str, "G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16") == 0)
        return VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16;
    if (ktx_strcasecmp(str, "G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16") == 0)
        return VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16;
    if (ktx_strcasecmp(str, "G16_B16R16_2PLANE_444_UNORM") == 0)
        return VK_FORMAT_G16_B16R16_2PLANE_444_UNORM;
    if (ktx_strcasecmp(str, "A4R4G4B4_UNORM_PACK16") == 0)
        return VK_FORMAT_A4R4G4B4_UNORM_PACK16;
    if (ktx_strcasecmp(str, "A4B4G4R4_UNORM_PACK16") == 0)
        return VK_FORMAT_A4B4G4R4_UNORM_PACK16;
    if (ktx_strcasecmp(str, "ASTC_4x4_SFLOAT_BLOCK") == 0)
        return VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_5x4_SFLOAT_BLOCK") == 0)
        return VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_5x5_SFLOAT_BLOCK") == 0)
        return VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_6x5_SFLOAT_BLOCK") == 0)
        return VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_6x6_SFLOAT_BLOCK") == 0)
        return VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_8x5_SFLOAT_BLOCK") == 0)
        return VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_8x6_SFLOAT_BLOCK") == 0)
        return VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_8x8_SFLOAT_BLOCK") == 0)
        return VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_10x5_SFLOAT_BLOCK") == 0)
        return VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_10x6_SFLOAT_BLOCK") == 0)
        return VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_10x8_SFLOAT_BLOCK") == 0)
        return VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_10x10_SFLOAT_BLOCK") == 0)
        return VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_12x10_SFLOAT_BLOCK") == 0)
        return VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_12x12_SFLOAT_BLOCK") == 0)
        return VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "PVRTC1_2BPP_UNORM_BLOCK_IMG") == 0)
        return VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG;
    if (ktx_strcasecmp(str, "PVRTC1_4BPP_UNORM_BLOCK_IMG") == 0)
        return VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG;
    if (ktx_strcasecmp(str, "PVRTC2_2BPP_UNORM_BLOCK_IMG") == 0)
        return VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG;
    if (ktx_strcasecmp(str, "PVRTC2_4BPP_UNORM_BLOCK_IMG") == 0)
        return VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG;
    if (ktx_strcasecmp(str, "PVRTC1_2BPP_SRGB_BLOCK_IMG") == 0)
        return VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG;
    if (ktx_strcasecmp(str, "PVRTC1_4BPP_SRGB_BLOCK_IMG") == 0)
        return VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG;
    if (ktx_strcasecmp(str, "PVRTC2_2BPP_SRGB_BLOCK_IMG") == 0)
        return VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG;
    if (ktx_strcasecmp(str, "PVRTC2_4BPP_SRGB_BLOCK_IMG") == 0)
        return VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG;
    if (ktx_strcasecmp(str, "ASTC_3x3x3_UNORM_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_3x3x3_UNORM_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_3x3x3_SRGB_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_3x3x3_SRGB_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_3x3x3_SFLOAT_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_3x3x3_SFLOAT_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_4x3x3_UNORM_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_4x3x3_UNORM_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_4x3x3_SRGB_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_4x3x3_SRGB_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_4x3x3_SFLOAT_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_4x3x3_SFLOAT_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_4x4x3_UNORM_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_4x4x3_UNORM_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_4x4x3_SRGB_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_4x4x3_SRGB_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_4x4x3_SFLOAT_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_4x4x3_SFLOAT_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_4x4x4_UNORM_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_4x4x4_UNORM_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_4x4x4_SRGB_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_4x4x4_SRGB_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_4x4x4_SFLOAT_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_4x4x4_SFLOAT_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_5x4x4_UNORM_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_5x4x4_UNORM_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_5x4x4_SRGB_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_5x4x4_SRGB_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_5x4x4_SFLOAT_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_5x4x4_SFLOAT_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_5x5x4_UNORM_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_5x5x4_UNORM_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_5x5x4_SRGB_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_5x5x4_SRGB_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_5x5x4_SFLOAT_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_5x5x4_SFLOAT_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_5x5x5_UNORM_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_5x5x5_UNORM_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_5x5x5_SRGB_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_5x5x5_SRGB_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_5x5x5_SFLOAT_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_5x5x5_SFLOAT_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_6x5x5_UNORM_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_6x5x5_UNORM_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_6x5x5_SRGB_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_6x5x5_SRGB_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_6x5x5_SFLOAT_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_6x5x5_SFLOAT_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_6x6x5_UNORM_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_6x6x5_UNORM_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_6x6x5_SRGB_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_6x6x5_SRGB_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_6x6x5_SFLOAT_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_6x6x5_SFLOAT_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_6x6x6_UNORM_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_6x6x6_UNORM_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_6x6x6_SRGB_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_6x6x6_SRGB_BLOCK_EXT;
    if (ktx_strcasecmp(str, "ASTC_6x6x6_SFLOAT_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_6x6x6_SFLOAT_BLOCK_EXT;
    if (ktx_strcasecmp(str, "R16G16_S10_5_NV") == 0)
        return VK_FORMAT_R16G16_S10_5_NV;
    if (ktx_strcasecmp(str, "A1B5G5R5_UNORM_PACK16_KHR") == 0)
        return VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR;
    if (ktx_strcasecmp(str, "A8_UNORM_KHR") == 0)
        return VK_FORMAT_A8_UNORM_KHR;
    if (ktx_strcasecmp(str, "ASTC_4x4_SFLOAT_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_5x4_SFLOAT_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_5x5_SFLOAT_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_6x5_SFLOAT_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_6x6_SFLOAT_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_8x5_SFLOAT_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_8x6_SFLOAT_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_8x8_SFLOAT_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_10x5_SFLOAT_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_10x6_SFLOAT_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_10x8_SFLOAT_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_10x10_SFLOAT_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_12x10_SFLOAT_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "ASTC_12x12_SFLOAT_BLOCK_EXT") == 0)
        return VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK;
    if (ktx_strcasecmp(str, "G8B8G8R8_422_UNORM_KHR") == 0)
        return VK_FORMAT_G8B8G8R8_422_UNORM;
    if (ktx_strcasecmp(str, "B8G8R8G8_422_UNORM_KHR") == 0)
        return VK_FORMAT_B8G8R8G8_422_UNORM;
    if (ktx_strcasecmp(str, "G8_B8_R8_3PLANE_420_UNORM_KHR") == 0)
        return VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM;
    if (ktx_strcasecmp(str, "G8_B8R8_2PLANE_420_UNORM_KHR") == 0)
        return VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
    if (ktx_strcasecmp(str, "G8_B8_R8_3PLANE_422_UNORM_KHR") == 0)
        return VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM;
    if (ktx_strcasecmp(str, "G8_B8R8_2PLANE_422_UNORM_KHR") == 0)
        return VK_FORMAT_G8_B8R8_2PLANE_422_UNORM;
    if (ktx_strcasecmp(str, "G8_B8_R8_3PLANE_444_UNORM_KHR") == 0)
        return VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM;
    if (ktx_strcasecmp(str, "R10X6_UNORM_PACK16_KHR") == 0)
        return VK_FORMAT_R10X6_UNORM_PACK16;
    if (ktx_strcasecmp(str, "R10X6G10X6_UNORM_2PACK16_KHR") == 0)
        return VK_FORMAT_R10X6G10X6_UNORM_2PACK16;
    if (ktx_strcasecmp(str, "R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR") == 0)
        return VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16;
    if (ktx_strcasecmp(str, "G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR") == 0)
        return VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16;
    if (ktx_strcasecmp(str, "B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR") == 0)
        return VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16;
    if (ktx_strcasecmp(str, "G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR") == 0)
        return VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16;
    if (ktx_strcasecmp(str, "G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR") == 0)
        return VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16;
    if (ktx_strcasecmp(str, "G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR") == 0)
        return VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16;
    if (ktx_strcasecmp(str, "G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR") == 0)
        return VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16;
    if (ktx_strcasecmp(str, "G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR") == 0)
        return VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16;
    if (ktx_strcasecmp(str, "R12X4_UNORM_PACK16_KHR") == 0)
        return VK_FORMAT_R12X4_UNORM_PACK16;
    if (ktx_strcasecmp(str, "R12X4G12X4_UNORM_2PACK16_KHR") == 0)
        return VK_FORMAT_R12X4G12X4_UNORM_2PACK16;
    if (ktx_strcasecmp(str, "R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR") == 0)
        return VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16;
    if (ktx_strcasecmp(str, "G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR") == 0)
        return VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16;
    if (ktx_strcasecmp(str, "B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR") == 0)
        return VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16;
    if (ktx_strcasecmp(str, "G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR") == 0)
        return VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16;
    if (ktx_strcasecmp(str, "G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR") == 0)
        return VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16;
    if (ktx_strcasecmp(str, "G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR") == 0)
        return VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16;
    if (ktx_strcasecmp(str, "G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR") == 0)
        return VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16;
    if (ktx_strcasecmp(str, "G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR") == 0)
        return VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16;
    if (ktx_strcasecmp(str, "G16B16G16R16_422_UNORM_KHR") == 0)
        return VK_FORMAT_G16B16G16R16_422_UNORM;
    if (ktx_strcasecmp(str, "B16G16R16G16_422_UNORM_KHR") == 0)
        return VK_FORMAT_B16G16R16G16_422_UNORM;
    if (ktx_strcasecmp(str, "G16_B16_R16_3PLANE_420_UNORM_KHR") == 0)
        return VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM;
    if (ktx_strcasecmp(str, "G16_B16R16_2PLANE_420_UNORM_KHR") == 0)
        return VK_FORMAT_G16_B16R16_2PLANE_420_UNORM;
    if (ktx_strcasecmp(str, "G16_B16_R16_3PLANE_422_UNORM_KHR") == 0)
        return VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM;
    if (ktx_strcasecmp(str, "G16_B16R16_2PLANE_422_UNORM_KHR") == 0)
        return VK_FORMAT_G16_B16R16_2PLANE_422_UNORM;
    if (ktx_strcasecmp(str, "G16_B16_R16_3PLANE_444_UNORM_KHR") == 0)
        return VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM;
    if (ktx_strcasecmp(str, "G8_B8R8_2PLANE_444_UNORM_EXT") == 0)
        return VK_FORMAT_G8_B8R8_2PLANE_444_UNORM;
    if (ktx_strcasecmp(str, "G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16_EXT") == 0)
        return VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16;
    if (ktx_strcasecmp(str, "G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16_EXT") == 0)
        return VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16;
    if (ktx_strcasecmp(str, "G16_B16R16_2PLANE_444_UNORM_EXT") == 0)
        return VK_FORMAT_G16_B16R16_2PLANE_444_UNORM;
    if (ktx_strcasecmp(str, "A4R4G4B4_UNORM_PACK16_EXT") == 0)
        return VK_FORMAT_A4R4G4B4_UNORM_PACK16;
    if (ktx_strcasecmp(str, "A4B4G4R4_UNORM_PACK16_EXT") == 0)
        return VK_FORMAT_A4B4G4R4_UNORM_PACK16;
    return VK_FORMAT_UNDEFINED;
}


/***************************** Do not edit.  *****************************
 Automatically generated from vulkan_core.h version 85 by mkvkformatfiles.
 *************************************************************************/

/*
** Copyright (c) 2015-2018 The Khronos Group Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <stdint.h>
#include <stdbool.h>

#include "vkformat_enum.h"

bool
isProhibitedFormat(VkFormat format)
{
    switch (format) {
      case VK_FORMAT_R8_USCALED:
      case VK_FORMAT_R8_SSCALED:
      case VK_FORMAT_R8G8_USCALED:
      case VK_FORMAT_R8G8_SSCALED:
      case VK_FORMAT_R8G8B8_USCALED:
      case VK_FORMAT_R8G8B8_SSCALED:
      case VK_FORMAT_B8G8R8_USCALED:
      case VK_FORMAT_B8G8R8_SSCALED:
      case VK_FORMAT_R8G8B8A8_USCALED:
      case VK_FORMAT_R8G8B8A8_SSCALED:
      case VK_FORMAT_B8G8R8A8_USCALED:
      case VK_FORMAT_B8G8R8A8_SSCALED:
      case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
      case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
      case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
      case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
      case VK_FORMAT_A8B8G8R8_UINT_PACK32:
      case VK_FORMAT_A8B8G8R8_SINT_PACK32:
      case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
      case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
      case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
      case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
      case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
      case VK_FORMAT_R16_USCALED:
      case VK_FORMAT_R16_SSCALED:
      case VK_FORMAT_R16G16_USCALED:
      case VK_FORMAT_R16G16_SSCALED:
      case VK_FORMAT_R16G16B16_USCALED:
      case VK_FORMAT_R16G16B16_SSCALED:
      case VK_FORMAT_R16G16B16A16_USCALED:
      case VK_FORMAT_R16G16B16A16_SSCALED:
        return true;
      default:
        return false;
    }
}


/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2019 Khronos Group, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @internal
 * @file texture1_vvtbl.c
 * @~English
 *
 * @brief Define the virtual function table for ktxTexture1 Vulkan functions.
 *
 * These are in a separate file to avoid including vulkan headers in texture.h.
 *
 * @author Mark Callow, www.edgewise-consulting.com
 */

#if defined(KTX_USE_FUNCPTRS_FOR_VULKAN)
#include "vk_funcs.h"   // Must be included before ktxvulkan.h.
#endif
#include "ktxvulkan.h"
#include "texture.h"

KTX_error_code ktxTexture2_VkUploadEx(ktxTexture1* This,
                                      ktxVulkanDeviceInfo* vdi,
                                      ktxVulkanTexture* vkTexture,
                                      VkImageTiling tiling,
                                      VkImageUsageFlags usageFlags,
                                      VkImageLayout layout);

KTX_error_code ktxTexture2_VkUpload(ktxTexture1* This,
                                    ktxVulkanDeviceInfo* vdi,
                                    ktxVulkanTexture *vkTexture);

VkFormat ktxTexture2_GetVkFormat(ktxTexture1* This);

struct ktxTexture_vvtbl ktxTexture2_vvtbl = {
    (PFNKTEXVKUPLOADEX)ktxTexture2_VkUploadEx,
    (PFNKTEXVKUPLOAD)ktxTexture2_VkUpload,
    (PFNKTEXGETVKFORMAT) ktxTexture2_GetVkFormat
};

struct ktxTexture_vvtbl* pKtxTexture2_vvtbl = &ktxTexture2_vvtbl;


/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab textwidth=70: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file vk_funcs.h
 * @~English
 *
 * @brief Declare pointers for Vulkan functions.
 *
 * Dynamically retrieving pointers avoids apps having to make sure a
 * Vulkan library is availablei when using a shared libktx, even if
 * not using libktx's Vulkan loader.
 */

#ifndef _VK_FUNCS_H_
#define _VK_FUNCS_H_

#if !defined(KTX_USE_FUNCPTRS_FOR_VULKAN)
#define KTX_USE_FUNCPTRS_FOR_VULKAN 1
#endif

#if defined(KTX_USE_FUNCPTRS_FOR_VULKAN)
#define VK_NO_PROTOTYPES
#endif

#include "vulkan/vk_platform.h"
#include "vulkan/vulkan_core.h"
#include "ktx.h"

#if defined(KTX_USE_FUNCPTRS_FOR_VULKAN)

#if WINDOWS
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
extern HMODULE ktxVulkanModuleHandle;
#else
extern void* ktxVulkanModuleHandle;
#endif

extern ktx_bool_t ktxLoadVulkanLibrary(void);

/* Declare pointers for functions libktx is using. */
#define VK_FUNCTION(fun) extern PFN_##fun ktx_##fun;

#include "vk_funclist.inl"

#undef VK_FUNCTION

/*
 * Define prefixed names to prevent collisions with other libraries or apps
 * finding our pointers when searching the module for function addresses.
 */
#define vkAllocateCommandBuffers ktx_vkAllocateCommandBuffers
#define vkAllocateMemory ktx_vkAllocateMemory
#define vkBeginCommandBuffer ktx_vkBeginCommandBuffer
#define vkBindBufferMemory ktx_vkBindBufferMemory
#define vkBindImageMemory ktx_vkBindImageMemory
#define vkCmdBlitImage ktx_vkCmdBlitImage
#define vkCmdCopyBufferToImage ktx_vkCmdCopyBufferToImage
#define vkCmdPipelineBarrier ktx_vkCmdPipelineBarrier
#define vkCreateBuffer ktx_vkCreateBuffer
#define vkCreateFence ktx_vkCreateFence
#define vkCreateImage ktx_vkCreateImage
#define vkDestroyBuffer ktx_vkDestroyBuffer
#define vkDestroyFence ktx_vkDestroyFence
#define vkDestroyImage ktx_vkDestroyImage
#define vkEndCommandBuffer ktx_vkEndCommandBuffer
#define vkFreeCommandBuffers ktx_vkFreeCommandBuffers
#define vkFreeMemory ktx_vkFreeMemory
#define vkGetBufferMemoryRequirements ktx_vkGetBufferMemoryRequirements
#define vkGetImageMemoryRequirements ktx_vkGetImageMemoryRequirements
#define vkGetImageSubresourceLayout ktx_vkGetImageSubresourceLayout
#define vkGetPhysicalDeviceImageFormatProperties ktx_vkGetPhysicalDeviceImageFormatProperties
#define vkGetPhysicalDeviceFormatProperties ktx_vkGetPhysicalDeviceFormatProperties
#define vkGetPhysicalDeviceMemoryProperties ktx_vkGetPhysicalDeviceMemoryProperties
#define vkMapMemory ktx_vkMapMemory
#define vkQueueSubmit ktx_vkQueueSubmit
#define vkQueueWaitIdle ktx_vkQueueWaitIdle
#define vkUnmapMemory ktx_vkUnmapMemory
#define vkWaitForFences ktx_vkWaitForFences

#undef VK_FUNCTION

#endif /* KTX_USE_FUNCPTRS_FOR_VULKAN */

#endif /* _VK_FUNCS_H_ */


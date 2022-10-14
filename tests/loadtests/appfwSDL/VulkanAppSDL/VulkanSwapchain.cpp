/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @class VulkanSwapchain
 * @~English
 *
 * @brief Manage the swapchain for a Vulkan app.
 *
 * A swap chain is a collection of image buffers used for rendering
 * The images can then be presented to the windowing system for display.
 *
 * @author Mark Callow, www.edgewise-consulting.com.
 */

#include <stdlib.h>
#include <string>
#include <fstream>
#include <assert.h>
#include <stdio.h>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#else
#endif

#include "VulkanSwapchain.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include "AppBaseSDL.h"
#include "unused.h"

#define ERROR_RETURN(msg)                                                     \
      (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, theApp->name(),    \
                                     msg, NULL);                              \
      return false;

#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                              \
  {                                                                           \
    pfn##entrypoint =                                                         \
        (PFN_vk##entrypoint)vkGetInstanceProcAddr(inst, "vk"#entrypoint);     \
    if (pfn##entrypoint == NULL) {                                            \
        ERROR_RETURN("vkGetInstanceProcAddr: unable to find vk"#entrypoint);  \
    }                                                                         \
  }

#define GET_DEVICE_PROC_ADDR(device, entrypoint)                            \
  {                                                                         \
    pfn##entrypoint =                                                       \
        (PFN_vk##entrypoint)vkGetDeviceProcAddr(device, "vk"#entrypoint);   \
    if (pfn##entrypoint == NULL) {                                          \
        ERROR_RETURN("vkGetDeviceProcAddr: unable to find vk"#entrypoint);  \
    }                                                                       \
  }

// Creates an os specific surface
// Tries to find a graphics and a present queue
bool
VulkanSwapchain::initSurface(SDL_Window* window)
{
    U_ASSERT_ONLY VkResult err;

    if (!SDL_Vulkan_CreateSurface(window, instance, &surface)) {
        std::string msg = "SDL_CreateVulkanSurface failed: ";
        msg += SDL_GetError();
        ERROR_RETURN(msg.c_str());
    }

    // Get available queue family properties
    uint32_t queueCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, NULL);
    assert(queueCount >= 1);

    std::vector<VkQueueFamilyProperties> queueProps(queueCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount,
                                             queueProps.data());

    // Iterate over the queues looking for ones which support presenting.
    std::vector<VkBool32> supportsPresent(queueCount);
    for (uint32_t i = 0; i < queueCount; i++) {
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface,
                                             &supportsPresent[i]);
    }

    // Search for a graphics- and present-capable queue.
    uint32_t graphicsQueueIndex = UINT32_MAX;
    uint32_t presentQueueIndex = UINT32_MAX;
    for (uint32_t i = 0; i < queueCount; i++) {
        if ((queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            if (graphicsQueueIndex == UINT32_MAX)
            {
                graphicsQueueIndex = i;
            }

            if (supportsPresent[i] == VK_TRUE)
            {
                graphicsQueueIndex = i;
                presentQueueIndex = i;
                break;
            }
        }
    }
    if (presentQueueIndex == UINT32_MAX) {
        // If there's no queue that supports both present and graphics
        // try to find a separate present queue
        for (uint32_t i = 0; i < queueCount; ++i)
        {
            if (supportsPresent[i] == VK_TRUE)
            {
                presentQueueIndex = i;
                break;
            }
        }
    }

    // Exit if either a graphics or a presenting queue hasn't been found
    if (graphicsQueueIndex == UINT32_MAX
        || presentQueueIndex == UINT32_MAX)
    {
        ERROR_RETURN("Could not find a graphics or presenting queue!");
    }

    // TODO: Add support for separate graphics and presenting queue
    if (graphicsQueueIndex != presentQueueIndex)
    {
        ERROR_RETURN("Separate graphics and present queues not yet supported!");
    }

    queueIndex = graphicsQueueIndex;

    // Get list of supported surface formats
    uint32_t formatCount;
    err = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface,
                                               &formatCount, NULL);
    assert(err == VK_SUCCESS);
    assert(formatCount > 0);

    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    err = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface,
                                               &formatCount,
                                               surfaceFormats.data());
    assert(err == VK_SUCCESS);

    // If the surface format list only includes one entry with
    // VK_FORMAT_UNDEFINED, there is no preferred format.
    // Assume VK_FORMAT_B8G8R8A8_RGB.
    // TODO: Consider passing in desired format from app.
    if ((formatCount == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED))
    {
        colorFormat = VK_FORMAT_B8G8R8A8_SRGB;
    }
    else
    {
        assert(formatCount >= 1);
        uint32_t i;
        for (i = 0; i < formatCount; i++) {
            if (surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB) {
                break;
            }
        }
        if (i == formatCount) {
            // Pick the first available, if no SRGB.
            // FIXME probably should raise an error...
            i = 0;
        }
        colorFormat = surfaceFormats[i].format;
    }
    colorSpace = surfaceFormats[0].colorSpace;
    return true;
}


// Connect to the instance and device and get all required function pointers
bool
VulkanSwapchain::connectDevice(VkDevice targetDevice)
{
    this->device = targetDevice;
#if USE_FUNCPTRS_FOR_KHR_EXTS
    GET_DEVICE_PROC_ADDR(device, CreateSwapchainKHR);
    GET_DEVICE_PROC_ADDR(device, DestroySwapchainKHR);
    GET_DEVICE_PROC_ADDR(device, GetSwapchainImagesKHR);
    GET_DEVICE_PROC_ADDR(device, AcquireNextImageKHR);
    GET_DEVICE_PROC_ADDR(device, QueuePresentKHR);
#endif
    return true;
}


// Connect to the instance and device and get all required function pointers
bool
VulkanSwapchain::connectInstance(VkInstance targetInstance,
                                 VkPhysicalDevice targetPhysicalDevice)
{
    this->instance = targetInstance;
    this->physicalDevice = targetPhysicalDevice;
#if USE_FUNCPTRS_FOR_KHR_EXTS
    GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfaceSupportKHR);
    GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfaceCapabilitiesKHR);
    GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfaceFormatsKHR);
    GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfacePresentModesKHR);
#endif
    return true;
}


// Create the swap chain and get images with given width and height
void
VulkanSwapchain::create(uint32_t *width, uint32_t *height,
                        bool vsync)
{
    U_ASSERT_ONLY VkResult err;
    VkSwapchainKHR oldSwapchain = swapchain;

    // Get physical device surface properties and formats
    VkSurfaceCapabilitiesKHR surfCaps;
    err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                                    &surfCaps);
    assert(err == VK_SUCCESS);

    // Get available present modes
    uint32_t presentModeCount;
    err = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                                    &presentModeCount, NULL);
    assert(err == VK_SUCCESS);
    assert(presentModeCount > 0);

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);

    err = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                                    &presentModeCount,
                                                    presentModes.data());
    assert(err == VK_SUCCESS);

    VkExtent2D swapchainExtent = {};
    // width and height are either both 0xFFFFFFFF, or both not 0xFFFFFFFF.
    if (surfCaps.currentExtent.width == UINT32_MAX)
    {
        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        swapchainExtent.width = *width;
        swapchainExtent.height = *height;
    }
    else
    {
        swapchainExtent = surfCaps.currentExtent;
        *width = surfCaps.currentExtent.width;
        *height = surfCaps.currentExtent.height;
    }


    // Select a present mode for the swapchain

    // The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
    // This mode waits for the vertical blank ("v-sync")
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

    // If v-sync is not requested, try to find a mailbox mode if present
    // It's the lowest latency non-tearing present mode available
    if (!vsync)
    {
        for (size_t i = 0; i < presentModeCount; i++)
        {
            if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
            if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR)
                 && (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR))
            {
                swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }
    }

    // Determine the number of images
    uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
    if ((surfCaps.maxImageCount > 0)
         && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount))
    {
        desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
    }

    VkSurfaceTransformFlagsKHR preTransform;
    if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        preTransform = surfCaps.currentTransform;
    }

    VkSwapchainCreateInfoKHR swapchainCI = {};
    swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCI.pNext = NULL;
    swapchainCI.surface = surface;
    swapchainCI.minImageCount = desiredNumberOfSwapchainImages;
    swapchainCI.imageFormat = colorFormat;
    swapchainCI.imageColorSpace = colorSpace;
    swapchainCI.imageExtent = { swapchainExtent.width, swapchainExtent.height };
    swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCI.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
    swapchainCI.imageArrayLayers = 1;
    swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCI.queueFamilyIndexCount = 0;
    swapchainCI.pQueueFamilyIndices = NULL;
    swapchainCI.presentMode = swapchainPresentMode;
    swapchainCI.oldSwapchain = oldSwapchain;
    swapchainCI.clipped = true;
    swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    err = vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapchain);
    assert(err == VK_SUCCESS);

    // If an existing swap chain is re-created, destroy the old swap chain
    // This also cleans up all the presentable images
    if (oldSwapchain != VK_NULL_HANDLE)
    {
        for (uint32_t i = 0; i < imageCount; i++)
        {
            vkDestroyImageView(device, buffers[i].view, nullptr);
        }
        vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
    }

    err = vkGetSwapchainImagesKHR(device, swapchain, &imageCount, NULL);
    assert(err == VK_SUCCESS);

    // Get the swap chain images
    images.resize(imageCount);
    err = vkGetSwapchainImagesKHR(device, swapchain,
                                  &imageCount, images.data());
    assert(err == VK_SUCCESS);

    // Get the swap chain buffers containing the image and imageview
    buffers.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++)
    {
        VkImageViewCreateInfo colorAttachmentView = {};
        colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        colorAttachmentView.pNext = NULL;
        colorAttachmentView.format = colorFormat;
        colorAttachmentView.components = {
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY
        };
        colorAttachmentView.subresourceRange.aspectMask
                                                  = VK_IMAGE_ASPECT_COLOR_BIT;
        colorAttachmentView.subresourceRange.baseMipLevel = 0;
        colorAttachmentView.subresourceRange.levelCount = 1;
        colorAttachmentView.subresourceRange.baseArrayLayer = 0;
        colorAttachmentView.subresourceRange.layerCount = 1;
        colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        colorAttachmentView.flags = 0;

        buffers[i].image = images[i];

        colorAttachmentView.image = buffers[i].image;

        err = vkCreateImageView(device, &colorAttachmentView, nullptr,
                                &buffers[i].view);
        assert(err == VK_SUCCESS);
    }
}


// Acquires the next image in the swap chain
VkResult
VulkanSwapchain::acquireNextImage(VkSemaphore presentCompleteSemaphore,
                                  uint32_t *currentBuffer)
{
    return vkAcquireNextImageKHR(device, swapchain, UINT64_MAX,
                                 presentCompleteSemaphore,
                                 (VkFence)nullptr,
                                 currentBuffer);
}


// Present the current image to the queue
VkResult
VulkanSwapchain::queuePresent(VkQueue queue, uint32_t currentBuffer)
{
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = NULL;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &currentBuffer;
    return vkQueuePresentKHR(queue, &presentInfo);
}


// Present the current image to the queue when semaphore signaled.
VkResult
VulkanSwapchain::queuePresent(VkQueue queue, uint32_t currentBuffer,
                              VkSemaphore waitSemaphore)
{
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = NULL;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &currentBuffer;
    if (waitSemaphore != VK_NULL_HANDLE)
    {
        presentInfo.pWaitSemaphores = &waitSemaphore;
        presentInfo.waitSemaphoreCount = 1;
    }
    return vkQueuePresentKHR(queue, &presentInfo);
}


// Free all Vulkan resources used by the swap chain
void
VulkanSwapchain::cleanup()
{
    for (uint32_t i = 0; i < imageCount; i++)
    {
        vkDestroyImageView(device, buffers[i].view, nullptr);
    }
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
}


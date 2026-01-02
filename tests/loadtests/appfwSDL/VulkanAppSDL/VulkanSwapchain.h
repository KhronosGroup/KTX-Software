/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#if !defined(USE_FUNCPTRS_FOR_KHR_EXTS)
#define USE_FUNCPTRS_FOR_KHR_EXTS 0
#endif

typedef struct _SwapchainBuffers {
    VkImage image;
    VkImageView view;
} SwapchainBuffer;

class VulkanSwapchain
{
  public:
    VkFormat colorFormat;
    VkColorSpaceKHR colorSpace;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;

    uint32_t imageCount;
    std::vector<VkImage> images;
    std::vector<SwapchainBuffer> buffers;

    // Index of the detected graphics- and present-capable device queue.
    uint32_t queueIndex = UINT32_MAX;

    // Creates an OS specific surface.
    // Looks for a graphics and a present queue
    bool initSurface(struct SDL_Window* window);

    // Connect to device and get required device function pointers.
    bool connectDevice(VkDevice device);

    // Connect to instance and get required instance function pointers.
    bool connectInstance(VkInstance instance,
                         VkPhysicalDevice physicalDevice);

    // Create the swap chain and get images with given width and height
    void create(uint32_t *width, uint32_t *height,
                bool vsync = false);

    // Acquires the next image in the swap chain
    VkResult acquireNextImage(VkSemaphore presentCompleteSemaphore,
                              uint32_t *currentBuffer);

    // Present the current image to the queue
    VkResult queuePresent(VkQueue queue, uint32_t currentBuffer);

    // Present the current image to the queue
    VkResult queuePresent(VkQueue queue, uint32_t currentBuffer,
                          VkSemaphore waitSemaphore);


    // Free all Vulkan resources used by the swap chain
    void cleanup();

  private:
    VkInstance instance;
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR surface;
#if USE_FUNCPTRS_FOR_KHR_EXTS
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR
        pfnGetPhysicalDeviceSurfaceSupportKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR
        pfnGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR
        pfnGetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR
        pfnGetPhysicalDeviceSurfacePresentModesKHR;

    PFN_vkCreateSwapchainKHR pfnCreateSwapchainKHR;
    PFN_vkDestroySwapchainKHR pfnDestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR pfnGetSwapchainImagesKHR;
    PFN_vkAcquireNextImageKHR pfnAcquireNextImageKHR;
    PFN_vkQueuePresentKHR pfnQueuePresentKHR;

#define vkGetPhysicalDeviceSurfaceSupportKHR \
            pfnGetPhysicalDeviceSurfaceSupportKHR
#define vkGetPhysicalDeviceSurfaceCapabilitiesKHR \
            pfnGetPhysicalDeviceSurfaceCapabilitiesKHR
#define vkGetPhysicalDeviceSurfaceFormatsKHR \
            pfnGetPhysicalDeviceSurfaceFormatsKHR
#define vkGetPhysicalDeviceSurfacePresentModesKHR \
            pfnGetPhysicalDeviceSurfacePresentModesKHR

#define vkCreateSwapchainKHR pfnCreateSwapchainKHR
#define vkDestroySwapchainKHR pfnDestroySwapchainKHR
#define vkGetSwapchainImagesKHR pfnGetSwapchainImagesKHR
#define vkAcquireNextImageKHR pfnAcquireNextImageKHR
#define vkQueuePresentKHR pfnQueuePresentKHR
#endif
};

/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include <vulkan/vulkan.hpp>

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
    struct csInfo {
        vk::ColorSpaceKHR cs;
        bool isHDR;
        bool isLinear;

        csInfo() {
            set(vk::ColorSpaceKHR::eSrgbNonlinear, false, false);
        }
        csInfo(vk::ColorSpaceKHR _cs, bool hdr, bool linear) {
            set(_cs, hdr, linear);
        }
        void set(vk::ColorSpaceKHR _cs, bool hdr, bool linear) {
            cs = _cs;
            isHDR = hdr;
            isLinear = linear;
        }
    };
    csInfo colorSpace;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;

    uint32_t imageCount;
    std::vector<VkImage> images;
    std::vector<SwapchainBuffer> buffers;

    // Index of the detected graphics- and present-capable device queue.
    uint32_t queueIndex = UINT32_MAX;

    // Create the swap chain and get images with given width and height
    void create(uint32_t& width, uint32_t& height,
                bool vsync = false);

    // Connect to device and get required device function pointers.
    bool connectDevice(VkDevice device);

    // Connect to instance and get required instance function pointers.
    bool connectInstance(VkInstance instance,
                         VkPhysicalDevice physicalDevice);


    // Creates an OS specific surface.
    void createSurface(struct SDL_Window* window);
    // Destroys the surface.
    void destroySurface();
   // Looks for a graphics and a present queue.
    void findGraphicsPresentQueue();
    // Initializes the surface.
    void initSurface(VkFormat format,
                     csInfo& colorSpace,
                     bool destroySurfaceOnFailure = true);


    // Acquires the next image in the swap chain
    VkResult acquireNextImage(VkSemaphore presentCompleteSemaphore,
                              uint32_t *imageIndex);

    // Present the current image to the queue
    VkResult queuePresent(VkQueue queue, uint32_t currentBuffer);

    // Present the current image to the queue
    VkResult queuePresent(VkQueue queue, uint32_t currentBuffer,
                          VkSemaphore waitSemaphore);

    VkSurfaceKHR getSurface() { return surface; }

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

/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

#ifndef VK_APP_SDL_H_1456211188
#define VK_APP_SDL_H_1456211188

/* $Id: ac63511da134f2c25a9e1da86a36bc27b6198ae3 $ */

/**
 * @internal
 * @file VkAppSDL.h
 * @~English
 *
 * @brief Declaration of VkAppSDL base class for Vulkan apps.
 *
 * @author Mark Callow
 * @copyright (c) 2016, Mark Callow.
 */

/*
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * unaltered in all copies or substantial portions of the Materials.
 * Any additions, deletions, or changes to the original source files
 * must be clearly indicated in accompanying documentation.
 *
 * If only executable code is distributed, then the accompanying
 * documentation must state that "this software is based in part on the
 * work of Mark Callow."
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

#include "AppBaseSDL.h"
#include <vulkan/vulkan.h>

#if !defined(USE_FUNCPTRS_FOR_KHR_EXTS)
#define USE_FUNCPTRS_FOR_KHR_EXTS 0
#endif

class VkAppSDL : public AppBaseSDL {
  public:
	VkAppSDL(const char* const name,
             int width, int height,
             const uint32_t version)
            : vkVersion(version),
              AppBaseSDL(name)
	{
	    w_width = width;
	    w_height = height;

	    pswMainWindow = NULL;

	    vdDevice = VK_NULL_HANDLE;
	    vpdGpu = VK_NULL_HANDLE;
	    viInstance = VK_NULL_HANDLE;
	    vqQueue = VK_NULL_HANDLE;
	    vsSurface = VK_NULL_HANDLE;
	    vscSwapchain = VK_NULL_HANDLE;
	};
    virtual int doEvent(SDL_Event* event);
    virtual void drawFrame(int ticks);
    virtual void finalize();
    virtual bool initialize(int argc, char* argv[]);
    virtual void onFPSUpdate();
    virtual void resize(int width, int height);

  protected:
    bool createDevice();
    bool createInstance();
    bool createSurface();
    bool createSwapchain();
    bool findGpu();
    bool findQueue();
    void flushInitialCommands();
    bool initializeVulkan();
    bool prepareColorBuffers();
    bool prepareDepthBuffer();
    bool prepareDescriptorLayout();
    bool prepareRenderPass();
    bool preparePipeline();
    bool prepareDescriptorPool();
    bool prepareDescriptorSet();
    bool prepareFramebuffers();

    bool setupDebugReporting();
    void setWindowTitle(const char* const szExtra);

    bool memoryTypeFromProperties(uint32_t typeBits,
                                  VkFlags requirementsMask,
                                  uint32_t *typeIndex);
    void setImageLayout(VkImage image, VkImageAspectFlags aspectMask,
                        VkImageLayout old_image_layout,
                        VkImageLayout new_image_layout,
                        VkAccessFlagBits srcAccessMask);

    VKAPI_ATTR VkBool32 VKAPI_CALL
    debugFunc(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
              uint64_t srcObject, size_t location, int32_t msgCode,
              const char *pLayerPrefix, const char *pMsg);

    static bool checkLayers(uint32_t nameCount, const char **names,
                            uint32_t layerCount, VkLayerProperties *layers);
    static VKAPI_ATTR VkBool32 VKAPI_CALL
    debugFunc(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
              uint64_t srcObject, size_t location, int32_t msgCode,
              const char *pLayerPrefix, const char *pMsg, void *pUserData);

    typedef struct _SwapchainBuffers {
        VkImage image;
        VkCommandBuffer cmd;
        VkImageView view;
        VkFramebuffer fb;
    } SwapchainBuffers;

    typedef struct _DepthBuffer {
        VkFormat format;
        VkImage image;
        VkMemoryAllocateInfo memAlloc;
        VkDeviceMemory mem;
        VkImageView view;
    } DepthBuffer;

    SDL_Window* pswMainWindow;

	int w_width;
	int w_height;

	uint32_t enabledExtensionCount;
	uint32_t enabledLayerCount;
	bool subOptimalPresentWarned;
	bool validate;

    const char* extensionNames[64];
    const char* deviceValidationLayers[64];

    uint32_t vkQueueFamilyIndex;

    VkColorSpaceKHR vcsColorSpace;
    VkCommandBuffer vcbCommandBuffer;
    VkCommandPool vcpCommandPool;
    VkDevice vdDevice;
    VkFormat vfFormat;
    VkPhysicalDevice vpdGpu;
    //VkPhysicalDeviceProperties vpdpGpuProperties;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    VkInstance viInstance;
    VkQueue vqQueue;
    VkRenderPass vrpRenderPass;
    VkSurfaceKHR vsSurface;

    uint32_t swapchainImageCount;
    uint32_t currentBuffer;
    VkSwapchainKHR vscSwapchain;
    SwapchainBuffers* scBuffers;
    VkExtent2D ve2SwapchainExtent;

    DepthBuffer depth;

    VkDebugReportCallbackEXT msgCallback;

    const uint32_t vkVersion;

    PFN_vkCreateDebugReportCallbackEXT pfnCreateDebugReportCallbackEXT;
    PFN_vkDestroyDebugReportCallbackEXT pfnDestroyDebugReportCallbackEXT;
    PFN_vkDebugReportMessageEXT pfnDebugReportMessageEXT;

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

#endif /* VK_APP_SDL_H_1456211188 */

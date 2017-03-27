/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

#ifndef VULKAN_APP_SDL_H_1456211188
#define VULKAN_APP_SDL_H_1456211188

/*
 * Copyright (c) 2017, Mark Callow, www.edgewise-consulting.com.
 *
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

#include <array>
#include <cstring>
#include <new>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "AppBaseSDL.h"
#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "vulkantextoverlay.hpp"

#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#else
#define U_ASSERT_ONLY
#endif

class VulkanAppSDL : public AppBaseSDL {
  public:
	VulkanAppSDL(const char* const name,
             int width, int height,
             const uint32_t version,
             bool enableTextOverlay)
            : w_width(width), w_height(height), vkVersion(version),
              enableTextOverlay(enableTextOverlay), AppBaseSDL(name)
	{
	    // The overridden new below will zero the storage. Thus
	    // we avoid a long list of initializers.
	    appTitle = name;
	};
	virtual ~VulkanAppSDL();
    virtual int doEvent(SDL_Event* event);
    virtual void drawFrame(uint32_t msTicks);
    virtual void finalize();
    virtual bool initialize(int argc, char* argv[]);
    virtual void onFPSUpdate();
    virtual void resizeWindow();
    virtual void windowResized();

    static void* operator new(size_t size) {
        void* storage = new char[size];
        memset(storage, 0, size);
        return storage;
    }

    void updateTextOverlay();

    // Called when the text overlay is updating
    // Can be overridden in derived class to add custom text to the overlay
    virtual void getOverlayText(VulkanTextOverlay * textOverlay);

  protected:
    bool createDevice();
    bool createInstance();
    bool createPipelineCache();
    bool createSemaphores();
    bool createSurface();
    bool createSwapchain();
    bool findGpu();
    void flushInitialCommands();
    bool initializeVulkan();
    bool prepareColorBuffers();
    bool prepareCommandBuffers();
    bool prepareDepthBuffer();
    bool prepareDescriptorLayout();
    void prepareFrame();
    bool preparePresentCommandBuffers();
    bool prepareRenderPass();
    bool preparePipeline();
    bool prepareDescriptorSet();
    bool prepareFramebuffers();
    void prepareTextOverlay();
    void submitFrame();

    bool setupDebugReporting();

    // Sets title to be used on window title bar.
    void setAppTitle(const char* const szExtra);
    // Sets text on window title bar.
    void setWindowTitle();

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

    std::string appTitle;

    bool prepared = false;
    // Set true if want presents v-sync'ed.
    bool enableVSync = false;

	uint32_t w_width;
	uint32_t w_height;

	bool subOptimalPresentWarned;
	bool validate;

    std::vector<const char*> extensionNames;
    std::vector<const char*> deviceValidationLayers;

    uint32_t vkQueueFamilyIndex;

    VkCommandBuffer setupCmdBuffer;
    VkSurfaceKHR vsSurface;

    VulkanContext vkctx;

    // Index of active framebuffer.
    uint32_t currentBuffer;

    // Synchronization semaphores
    struct {
        // Swap chain image presentation
        VkSemaphore presentComplete;
        // Command buffer submission and execution
        VkSemaphore renderComplete;
        // Text overlay submission and execution
        VkSemaphore textOverlayComplete;
    } semaphores;

    const uint32_t vkVersion;

    // Saved for clean-up
    std::vector<vk::ShaderModule> shaderModules;

    bool enableTextOverlay = false;
    VulkanTextOverlay *textOverlay;
    // List of shader modules created (stored for cleanup)

    VkDebugReportCallbackEXT msgCallback;

    PFN_vkCreateDebugReportCallbackEXT pfnCreateDebugReportCallbackEXT;
    PFN_vkDestroyDebugReportCallbackEXT pfnDestroyDebugReportCallbackEXT;
    PFN_vkDebugReportMessageEXT pfnDebugReportMessageEXT;
};

#endif /* VULKAN_APP_SDL_H_1456211188 */

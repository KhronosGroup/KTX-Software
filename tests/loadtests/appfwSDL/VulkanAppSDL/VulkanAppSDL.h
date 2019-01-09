/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

#ifndef VULKAN_APP_SDL_H_1456211188
#define VULKAN_APP_SDL_H_1456211188

/*
 * ©2017-2018 Mark Callow.
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
              enableTextOverlay(enableTextOverlay), AppBaseSDL(name),
              subOptimalPresentWarned(false), textOverlay(nullptr), validate(false)
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
    virtual void getOverlayText(VulkanTextOverlay * textOverlay, float yOffset);

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

    enum stencilRequirement { eNoStencil = 0, eStencil = 1 };
    enum depthRequirement { e16bits = 0, e24bits = 1, e32bits = 2 };
    bool getSupportedDepthFormat(vk::PhysicalDevice gpu,
                                 stencilRequirement requiredStencil,
                                 depthRequirement requiredDepth,
                                 vk::Format& pFormat,
                                 vk::ImageAspectFlags& pAspectMask);

    // Sets text on window title bar.
    void setWindowTitle();

    void setImageLayout(VkImage image, VkImageAspectFlags aspectMask,
                        VkImageLayout old_image_layout,
                        VkImageLayout new_image_layout,
                        VkAccessFlags srcAccessMask);

    VKAPI_ATTR VkBool32 VKAPI_CALL
    debugFunc(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
              uint64_t srcObject, size_t location, int32_t msgCode,
              const char *pLayerPrefix, const char *pMsg);
    std::string& wrapText(std::string& source, size_t width = 70,
                          const std::string& whitespace = " \t\r");
    uint32_t showDebugReport(uint32_t mbFlags, const std::string title,
                             std::string message, bool enableAbort);

    static bool checkLayers(uint32_t nameCount, const char **names,
                            uint32_t layerCount, VkLayerProperties *layers);
    static VKAPI_ATTR VkBool32 VKAPI_CALL
    debugFunc(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
              uint64_t srcObject, size_t location, int32_t msgCode,
              const char *pLayerPrefix, const char *pMsg, void *pUserData);

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

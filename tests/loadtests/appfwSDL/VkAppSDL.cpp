/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/* $Id: ac63511da134f2c25a9e1da86a36bc27b6198ae3 $ */

/**
 * @internal
 * @file VkAppSDL.cpp
 * @~English
 *
 * @brief VkAppSDL app class.
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

#if defined(_WIN32)
  #if _MSC_VER < 1900
    #define snprintf _snprintf
  #endif
  #define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sstream>

#include "VkAppSDL.h"
#include <SDL_vulkan.h>

#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#else
#define U_ASSERT_ONLY
#endif

#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))

#define ERROR_RETURN(msg) \
      (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, szName, msg, NULL); \
      return false;

#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                                 \
    {                                                                            \
        pfn##entrypoint =                                                        \
            (PFN_vk##entrypoint)vkGetInstanceProcAddr(inst, "vk" #entrypoint);   \
        if (pfn##entrypoint == NULL) {                                           \
            ERROR_RETURN("vkGetInstanceProcAddr: unable to find vk" #entrypoint); \
        }                                                                        \
    }

#define GET_DEVICE_PROC_ADDR(device, entrypoint)                                 \
    {                                                                            \
        pfn##entrypoint =                                                        \
            (PFN_vk##entrypoint)vkGetDeviceProcAddr(device, "vk" #entrypoint);   \
        if (pfn##entrypoint == NULL) {                                           \
            ERROR_RETURN("vkGetDeviceProcAddr: unable to find vk" #entrypoint); \
        }                                                                        \
    }


bool
VkAppSDL::initialize(int argc, char* argv[])
{
    char** argv2 = new char*[argc];
    int argc2 = argc;
    validate = false;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--validate") == 0) {
			validate = true;
			argc2--;
        } else {
            argv2[i] = argv[i];
        }
	}
	if (!AppBaseSDL::initialize(argc2, argv2))
		return false;
	delete[] argv2;

#if defined(DEBUG)
	validate = true;
	// Enable debug layers?
#endif
    
	// Create window.
	// Vulkan samples do not pass any information from Vulkan initialization
	// to window creation so this order should be okay...
    pswMainWindow = SDL_CreateWindow(
                        szName,
                        SDL_WINDOWPOS_UNDEFINED,
                        SDL_WINDOWPOS_UNDEFINED,
                        w_width, w_height,
                        SDL_WINDOW_RESIZABLE
                    );

    if (pswMainWindow == NULL) {
        (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, szName, SDL_GetError(), NULL);
        return false;
    }

	if (!initializeVulkan()) {
	    return false;
	}

	currentBuffer = 0;
	subOptimalPresentWarned = false;

    // Not getting an initial resize event, at least on Mac OS X.
    // Therefore call resize directly.
    
    resize(w_width, w_height);

    initializeFPSTimer();
    return true;
}


void
VkAppSDL::finalize()
{
    if (vscSwapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(vdDevice, vscSwapchain, NULL);
    }
    // Destroy vcbCommand and vcpCommandPool
    if (vqQueue != VK_NULL_HANDLE) {
        // Destroy
    }
    if (vdDevice != VK_NULL_HANDLE) {
        // Destroy
    }
    if (vpdGpu != VK_NULL_HANDLE) {
        // Destroy
    }
    if (vsSurface != VK_NULL_HANDLE) {
        // Destroy
    }
    if (viInstance != VK_NULL_HANDLE) {
        if (validate) {
            pfnDestroyDebugReportCallbackEXT(viInstance, msgCallback, NULL);
        }
        // Destroy
    }
}


int
VkAppSDL::doEvent(SDL_Event* event)
{
    switch (event->type) {
      case SDL_WINDOWEVENT:
        switch (event->window.event) {
          case SDL_WINDOWEVENT_SIZE_CHANGED:
            resize(event->window.data1, event->window.data2);
		    return 0;
        }
        break;
            
    }
    return AppBaseSDL::doEvent(event);
}


void
VkAppSDL::drawFrame(int ticks)
{
    AppBaseSDL::drawFrame(ticks);
    VkResult U_ASSERT_ONLY err;

    // Wait for work to finish before updating uniforms.
    // XXX Is this really necessary? Doesn't it stall the pipeline?
    vkDeviceWaitIdle(vdDevice);

    VkSemaphore presentCompleteSemaphore;
    VkSemaphoreCreateInfo scInfo = {
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        NULL,
        0,
    };
    //VkFence nullFence = VK_NULL_HANDLE;

    err = vkCreateSemaphore(vdDevice, &scInfo,
                            NULL, &presentCompleteSemaphore);
    assert(!err);

    // Get the index of the next available swapchain image:
    err = vkAcquireNextImageKHR(vdDevice, vscSwapchain, UINT64_MAX,
                                presentCompleteSemaphore,
                                (VkFence)0,
                                &currentBuffer);
    if (err == VK_ERROR_OUT_OF_DATE_KHR) {
        // Swap chain is out of date (e.g. the window was resized).
        // Re-create it.
        //resize();
        //draw(demo);
        vkDestroySemaphore(vdDevice, presentCompleteSemaphore, NULL);
        return;
    } else if (err == VK_SUBOPTIMAL_KHR) {
        // demo->swapchain is not as optimal as it could be, but the platform's
        // presentation engine will still present the image correctly.
    } else {
        assert(!err);
    }

    // XXX This should undoubtedly be moved somewhere else.
    // Is a command buffer per swap chain buffer necessary? Let's try without.
    if (vcbCommandBuffer == VK_NULL_HANDLE) {
        VkCommandBufferAllocateInfo aInfo;
        aInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        aInfo.pNext = NULL;
        aInfo.commandPool = vcpCommandPool;
        aInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        aInfo.commandBufferCount = 1;

        err = vkAllocateCommandBuffers(vdDevice, &aInfo, &vcbCommandBuffer);
        assert(!err);
    }

    const VkCommandBufferBeginInfo cmd_buf_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL, 0, NULL
    };

    VkClearValue clear_values[2] = {
       { currentBuffer * 1.f, 0.2f, 0.2f, 1.0f },
       { 0.0f, 0 }
    };

    const VkRenderPassBeginInfo rp_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        NULL,
        vrpRenderPass,
        scBuffers[currentBuffer].fb,
        { 0, 0, ve2SwapchainExtent.width, ve2SwapchainExtent.height },
        2,
        clear_values,
    };

    err = vkBeginCommandBuffer(vcbCommandBuffer, &cmd_buf_info);
    assert(!err);

    // We can use LAYOUT_UNDEFINED as a wildcard here because we don't care what
    // happens to the previous contents of the image
    VkImageMemoryBarrier image_memory_barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        NULL,
        0,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        scBuffers[currentBuffer].image,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };
    vkCmdPipelineBarrier(vcbCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0, 0, NULL, 0,
                         NULL, 1, &image_memory_barrier);

    vkCmdBeginRenderPass(vcbCommandBuffer, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(vcbCommandBuffer);

    VkImageMemoryBarrier present_barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        NULL,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_MEMORY_READ_BIT,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        scBuffers[currentBuffer].image,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
    };
    present_barrier.image = scBuffers[currentBuffer].image;

    vkCmdPipelineBarrier(
        vcbCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0, 0, NULL, 0, NULL, 1, &present_barrier);

    err = vkEndCommandBuffer(vcbCommandBuffer);
    assert(!err);


    VkPipelineStageFlags pipe_stage_flags =
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    VkSubmitInfo submit_info;
      submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submit_info.pNext = NULL;
      submit_info.waitSemaphoreCount = 1;
      submit_info.pWaitSemaphores = &presentCompleteSemaphore,
      submit_info.pWaitDstStageMask = &pipe_stage_flags,
      submit_info.commandBufferCount = 1;
      submit_info.pCommandBuffers = &vcbCommandBuffer;
      submit_info.pWaitDstStageMask = &pipe_stage_flags,
      submit_info.signalSemaphoreCount = 0;
      submit_info.pSignalSemaphores = NULL;

    err = vkQueueSubmit(vqQueue, 1, &submit_info, VK_NULL_HANDLE);
    assert(!err);

    VkPresentInfoKHR present = { };
      present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
      present.swapchainCount = 1;
      present.pSwapchains = &vscSwapchain;
      present.pImageIndices = &currentBuffer;

    err = vkQueuePresentKHR(vqQueue, &present);
    if (err == VK_ERROR_OUT_OF_DATE_KHR) {
        // demo->swapchain is out of date (e.g. the window was resized) and
        // must be recreated:
        //resize();
    } else if (err == VK_SUBOPTIMAL_KHR) {
        if (!subOptimalPresentWarned) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, szName,
                                     "Suboptimal present of framebuffer.", NULL);
        }
    } else
        assert(!err);

    err = vkQueueWaitIdle(vqQueue);
    assert(err == VK_SUCCESS);
    vkDestroySemaphore(vdDevice, presentCompleteSemaphore, NULL);
    //currentBuffer = (currentBuffer + 1) % swapchainImageCount;
}


void
VkAppSDL::resize(int width, int height)
{
}


void
VkAppSDL::onFPSUpdate()
{
    // Using onFPSUpdate avoids rewriting the title every frame.
    setWindowTitle("");
}


void
VkAppSDL::setWindowTitle(const char* const szExtra)
{
    char szTitle[100];

    // Using this verbose way to avoid compiler warnings that occur when using
    // the obvious way of a ?: to select a format string.
    if (szExtra[0] == '\0') {
        snprintf(szTitle, sizeof(szTitle),
                 "%#.2f fps. %s",
                 fFPS, szName);
    } else {
        snprintf(szTitle, sizeof(szTitle),
                 "%#.2f fps. %s: %s",
                 fFPS, szName, szExtra);
    }
    SDL_SetWindowTitle(pswMainWindow, szTitle);
}


bool
VkAppSDL::initializeVulkan()
{
    if (createInstance()
            && findGpu()
            && setupDebugReporting()
            && createSurface()
            && findQueue()
            && createDevice()
            && createSwapchain()
            && prepareColorBuffers()
            && prepareDepthBuffer()
            && prepareRenderPass()
            && prepareFramebuffers()) {
        // Functions above most likely generate pipeline commands
        // that need to be flushed before beginning the render loop.
        flushInitialCommands();
        return true;
    } else {
        return false;
    }
}


bool
VkAppSDL::createInstance()
{
    VkResult err;
    uint32_t instanceLayerCount = 0;
    uint32_t deviceValidationLayerCount = 0;
    const char **instanceValidationLayers = NULL;

    const char *instanceValidationLayers_alt1[] = {
        "VK_LAYER_LUNARG_standard_validation"
    };

    const char *instanceValidationLayers_alt2[] = {
        "VK_LAYER_GOOGLE_threading",     "VK_LAYER_LUNARG_parameter_validation",
        "VK_LAYER_LUNARG_device_limits", "VK_LAYER_LUNARG_object_tracker",
        "VK_LAYER_LUNARG_image",         "VK_LAYER_LUNARG_core_validation",
        "VK_LAYER_LUNARG_swapchain",     "VK_LAYER_GOOGLE_unique_objects"
    };

    enabledExtensionCount = 0;
    enabledLayerCount = 0;

    // Look for validation layers.
    bool validationFound = 0;
    memset(deviceValidationLayers, 0, sizeof(deviceValidationLayers));
    if (validate) {

        err = vkEnumerateInstanceLayerProperties(&instanceLayerCount, NULL);
        assert(err == VK_SUCCESS);

        instanceValidationLayers = instanceValidationLayers_alt1;
        if (instanceLayerCount > 0) {
            VkLayerProperties* instanceLayers
                = new VkLayerProperties[instanceLayerCount];
            err = vkEnumerateInstanceLayerProperties(&instanceLayerCount,
                                                     instanceLayers);
            assert(err == VK_SUCCESS);

            validationFound = checkLayers(ARRAY_LEN(instanceValidationLayers_alt1),
                                          instanceValidationLayers,
                                          instanceLayerCount,
                                          instanceLayers);
            if (validationFound) {
                enabledLayerCount = ARRAY_LEN(instanceValidationLayers_alt1);
                deviceValidationLayers[0] = "VK_LAYER_LUNARG_standard_validation";
                deviceValidationLayerCount = 1;
            } else {
                // Use alternative set of validation layers.
                instanceValidationLayers = instanceValidationLayers_alt2;
                enabledLayerCount = ARRAY_LEN(instanceValidationLayers_alt2);
                validationFound = checkLayers(
                    ARRAY_LEN(instanceValidationLayers_alt2),
                    instanceValidationLayers, instanceLayerCount,
                    instanceLayers);
                deviceValidationLayerCount =
                        ARRAY_LEN(instanceValidationLayers_alt2);
                for (uint32_t i = 0; i < deviceValidationLayerCount; i++) {
                    deviceValidationLayers[i] = instanceValidationLayers[i];
                }
            }
            delete [] instanceLayers;
        }

        if (!validationFound) {
            ERROR_RETURN("vkEnumerateInstanceLayerProperties failed to find "
                         "required validation layer.\n");
        }
    }


    /* Build list of needed extensions */
    int extNamesArrayLen = ARRAY_LEN(extensionNames);
    memset(extensionNames, 0, sizeof(extensionNames));

    extensionNames[enabledExtensionCount++] = VK_KHR_SURFACE_EXTENSION_NAME;
    if (validate)
        extensionNames[enabledExtensionCount++] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;

    unsigned c = ARRAY_LEN(extensionNames) - enabledExtensionCount;
    if (!SDL_GetVulkanInstanceExtensions(&c, &extensionNames[enabledExtensionCount])) {
        std::string msg;
        msg = "SDL_GetVulkanInstanceExtensions failed: ";
        msg += SDL_GetError();
        ERROR_RETURN(msg.c_str());
    }
    enabledExtensionCount += c;

    const VkApplicationInfo app = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        NULL,
        szName,
        0,
        szName,
        0,
        vkVersion
    };

    VkInstanceCreateInfo instanceInfo = { };
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pNext = NULL;
    instanceInfo.pApplicationInfo = &app;
    instanceInfo.enabledLayerCount = enabledLayerCount;
    instanceInfo.ppEnabledLayerNames = (const char *const *)instanceValidationLayers;
    instanceInfo.enabledExtensionCount = enabledExtensionCount;
    instanceInfo.ppEnabledExtensionNames = (const char *const *)extensionNames;

    /*
     * This is info for a temp callback to use during CreateInstance.
     * After the instance is created, we use the instance-based
     * function to register the final callback.
     */
    VkDebugReportCallbackCreateInfoEXT dbgCreateInfo = { };
    if (validate) {
        dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgCreateInfo.pNext = NULL;
        dbgCreateInfo.pfnCallback = debugFunc;
        dbgCreateInfo.pUserData = this;
        dbgCreateInfo.flags =
            VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        instanceInfo.pNext = &dbgCreateInfo;
    }

    err = vkCreateInstance(&instanceInfo, NULL, &viInstance);

    if (err != VK_SUCCESS) {
        std::string title;
        std::stringstream msg;
        title = szName;
        title += ": vkCreateInstance Failure";
        if (err == VK_ERROR_INCOMPATIBLE_DRIVER) {
            msg << "Cannot find a compatible Vulkan installable client "
                   "driver (ICD).";
        } else if (err == VK_ERROR_EXTENSION_NOT_PRESENT) {
            // Figure out which extension(s) are missing.
            uint32_t instanceExtensionCount = 0;

            err = vkEnumerateInstanceExtensionProperties(NULL,
                                                         &instanceExtensionCount,
                                                         NULL);
            VkExtensionProperties* instanceExtensions
                = new VkExtensionProperties[instanceExtensionCount];

            assert(err == VK_SUCCESS);
            if (instanceExtensionCount > 0) {
                err = vkEnumerateInstanceExtensionProperties(NULL,
                                                             &instanceExtensionCount,
                                                             instanceExtensions);
                assert(err == VK_SUCCESS);
            }
            msg << "Cannot find the following extensions:\n";
            for (int i = 0; i < enabledExtensionCount; i++) {
                int j;
                for (j = 0; j < instanceExtensionCount; j++) {
                    if (!strcmp(extensionNames[i], instanceExtensions[j].extensionName))
                        break;
                }
                if (j == instanceExtensionCount) {
                    // Not found
                    msg << "    " << extensionNames[i] << "\n";
                }
            }
            msg << "\nMake sure your layers path is set appropriately.";
            delete [] instanceExtensions;
       } else {
            msg << "vkCreateInstance: unexpected failure, code = "
                << err << ".\n\nDo you have a compatible Vulkan "
                  "installable client driver (ICD) installed?";
       }
       (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.c_str(),
                                      msg.str().c_str(), NULL);
       return false;
    }

#if USE_FUNCPTRS_FOR_KHR_EXTS
    GET_INSTANCE_PROC_ADDR(viInstance, GetPhysicalDeviceSurfaceSupportKHR);
    GET_INSTANCE_PROC_ADDR(viInstance, GetPhysicalDeviceSurfaceCapabilitiesKHR);
    GET_INSTANCE_PROC_ADDR(viInstance, GetPhysicalDeviceSurfaceFormatsKHR);
    GET_INSTANCE_PROC_ADDR(viInstance, GetPhysicalDeviceSurfacePresentModesKHR);
    //GET_INSTANCE_PROC_ADDR(viInstance, GetSwapchainImagesKHR);
#endif

    return true;
} // createInstance


bool
VkAppSDL::findGpu()
{
    VkResult err;
    uint32_t gpuCount;

    // Make initial call to query gpu_count, then second call for gpu info.
    err = vkEnumeratePhysicalDevices(viInstance, &gpuCount, NULL);
    assert(err == VK_SUCCESS && gpuCount > 0);

    if (gpuCount > 0) {
        VkPhysicalDevice *gpus = new VkPhysicalDevice[gpuCount];
        err = vkEnumeratePhysicalDevices(viInstance, &gpuCount, gpus);
        assert(err == VK_SUCCESS);
        // For now just grab the first physical device */
        vpdGpu = gpus[0];
        //vkGetPhysicalDeviceProperties(vpdGpu, &vpdpGpuProperties);
        delete [] gpus;
    } else {
        vpdGpu = VK_NULL_HANDLE;

        std::string msg;
        ERROR_RETURN(
            "vkEnumeratePhysicalDevices reported zero accessible devices.\n\n"
            "Do you have a compatible Vulkan installable client driver (ICD) "
            "installed?");
    }

    // Query fine-grained feature support for this GPU. If the app has
    // specific feature requirements, it should check supported
    // features based on this query
    //VkPhysicalDeviceFeatures physDevFeatures;
    //vkGetPhysicalDeviceFeatures(vpdGpu, &physDevFeatures);

    // Get Memory information and properties
    vkGetPhysicalDeviceMemoryProperties(vpdGpu, &memoryProperties);

    return true;
} // findGpu


bool
VkAppSDL::setupDebugReporting()
{
    VkResult err;

    if (validate) {
        GET_INSTANCE_PROC_ADDR(viInstance, CreateDebugReportCallbackEXT);
        GET_INSTANCE_PROC_ADDR(viInstance, DestroyDebugReportCallbackEXT);
        GET_INSTANCE_PROC_ADDR(viInstance, DebugReportMessageEXT);
        VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
        dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgCreateInfo.pNext = NULL;
        dbgCreateInfo.pfnCallback = debugFunc;
        dbgCreateInfo.pUserData = this;
        dbgCreateInfo.flags =
            VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        VkResult err = pfnCreateDebugReportCallbackEXT(viInstance,
                                                       &dbgCreateInfo, NULL,
                                                       &msgCallback);
        switch (err) {
          case VK_SUCCESS:
            return true;
            break;
          case VK_ERROR_OUT_OF_HOST_MEMORY:
            ERROR_RETURN("CreateDebugReportCallback: out of host memory.");
            break;
          default:
          {
            std::stringstream msg;
            msg << "CreateDebugReportCallback: unexpected failure, result code "
                << err << ".";
            ERROR_RETURN(msg.str().c_str());
            break;
          }
        }
    }
    return true;
} // setupDebugReporting


bool
VkAppSDL::createSurface()
{
    if (!SDL_CreateVulkanSurface(pswMainWindow, viInstance, &vsSurface)) {
        std::string msg = "SDL_CreateVulkanSurface failed: ";
        msg += SDL_GetError();
        ERROR_RETURN(msg.c_str());
    }
    return true;
} // createSurface


bool
VkAppSDL::findQueue()
{
    uint32_t queueCount;

    vkQueueFamilyIndex = UINT32_MAX;
    // Retrieve no. of supported queues.
    vkGetPhysicalDeviceQueueFamilyProperties(vpdGpu, &queueCount, NULL);
    // Retrieve queue properties
    VkQueueFamilyProperties* queueProps
        = new VkQueueFamilyProperties[queueCount];
    vkGetPhysicalDeviceQueueFamilyProperties(vpdGpu, &queueCount, queueProps);
    assert(queueCount >= 1);

    // Find a queue that supports graphics and present
    for (uint32_t i = 0; i < queueCount; i++) {
        VkBool32 supportsPresent;
        vkGetPhysicalDeviceSurfaceSupportKHR(vpdGpu, i, vsSurface, &supportsPresent);
        if (supportsPresent && (queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            vkQueueFamilyIndex = i;
            break;
        }
    }
    delete [] queueProps;
    if (vkQueueFamilyIndex == UINT32_MAX) {
        ERROR_RETURN("Could not find a graphics- and a present-capable Vulkan queue.")
    }

    return true;
} // findQueue


bool
VkAppSDL::createDevice()
{
    const char *deviceExtensionNames[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    uint32_t enabledDeviceExtensionCount = ARRAY_LEN(deviceExtensionNames);
    VkResult err;

    float queue_priorities[1] = {0.0};
    const VkDeviceQueueCreateInfo queueInfo = {
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        NULL,
        0,
        vkQueueFamilyIndex,
        1,
        queue_priorities
    };

    VkDeviceCreateInfo deviceInfo = { };
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    //deviceInfo.pNext = NULL; // Zeroed by { } initialization.
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.enabledLayerCount = enabledLayerCount;
    deviceInfo.ppEnabledLayerNames =
        (const char *const *)((validate)
                                  ? deviceValidationLayers
                                  : NULL),
    deviceInfo.enabledExtensionCount = enabledDeviceExtensionCount;
    deviceInfo.ppEnabledExtensionNames = (const char *const *)deviceExtensionNames;
    // If specific features are required, pass them in here
    //deviceInfo.pEnabledFeatures = NULL;

    err = vkCreateDevice(vpdGpu, &deviceInfo, NULL, &vdDevice);
    if (err != VK_SUCCESS) {
        std::string title;
        std::stringstream msg;
        title = szName;
        title += ": vkCreateDevice Failure";
        if (err == VK_ERROR_EXTENSION_NOT_PRESENT) {
            // Figure out which extension(s) are missing.
            uint32_t deviceExtensionCount = 0;

            err = vkEnumerateDeviceExtensionProperties(vpdGpu,
                                                       NULL,
                                                       &deviceExtensionCount,
                                                       NULL);
            VkExtensionProperties* deviceExtensions
                = new VkExtensionProperties[deviceExtensionCount];

            assert(err == VK_SUCCESS);
            if (deviceExtensionCount > 0) {
                err = vkEnumerateDeviceExtensionProperties(vpdGpu,
                                                             NULL,
                                                             &deviceExtensionCount,
                                                             deviceExtensions);
                assert(err == VK_SUCCESS);
            }
            msg << "Cannot find the following device extensions:\n";
            for (int i = 0; i < enabledDeviceExtensionCount; i++) {
                int j;
                for (j = 0; j < deviceExtensionCount; j++) {
                    if (!strcmp(extensionNames[i], deviceExtensions[j].extensionName))
                        break;
                }
                if (j == deviceExtensionCount) {
                    // Not found
                    msg << "    " << extensionNames[i] << "\n";
                }
            }
            msg << "\n\nDo you have a compatible Vulkan "
                   "installable client driver (ICD) installed?";
            delete [] deviceExtensions;
       } else {
            msg << "vkCreateDevice: unexpected failure, result code = ";
            msg << err << ".";
       }
       (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.c_str(),
                                      msg.str().c_str(), NULL);
       return false;
    }

#if USE_FUNCPTRS_FOR_KHR_EXTS
    GET_DEVICE_PROC_ADDR(vdDevice, CreateSwapchainKHR);
    GET_DEVICE_PROC_ADDR(vdDevice, DestroySwapchainKHR);
    GET_DEVICE_PROC_ADDR(vdDevice, GetSwapchainImagesKHR);
    GET_DEVICE_PROC_ADDR(vdDevice, AcquireNextImageKHR);
    GET_DEVICE_PROC_ADDR(vdDevice, QueuePresentKHR);
#endif

    const VkCommandPoolCreateInfo cmdPoolInfo = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        NULL,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        vkQueueFamilyIndex,
    };
    err = vkCreateCommandPool(vdDevice, &cmdPoolInfo, NULL, &vcpCommandPool);
    assert(!err);

    vkGetDeviceQueue(vdDevice, vkQueueFamilyIndex, 0, &vqQueue);

    return true;
} // createDevice


bool
VkAppSDL::createSwapchain()
{
    // Get the list of supported formats.
    VkResult err;
    uint32_t formatCount, i;
    err = vkGetPhysicalDeviceSurfaceFormatsKHR(vpdGpu, vsSurface,
                                               &formatCount, NULL);
    assert(!err);

    VkSurfaceFormatKHR* formats = new VkSurfaceFormatKHR[formatCount];
    err = vkGetPhysicalDeviceSurfaceFormatsKHR(vpdGpu, vsSurface,
                                               &formatCount, formats);
    assert(!err);

    if (formatCount == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
        vfFormat = VK_FORMAT_B8G8R8A8_SRGB;
    } else {
        assert(formatCount >= 1);
        for (i = 0; i < formatCount; i++) {
            if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB) {
                vfFormat = formats[i].format;
                break;
            }
        }
        if (i == formatCount)
            i = 0;
        vfFormat = formats[i].format;
    }
    vcsColorSpace = formats[i].colorSpace;

    VkSurfaceCapabilitiesKHR surfCap;
    err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vpdGpu, vsSurface, &surfCap);
    assert(!err);

    if (surfCap.currentExtent.width == (uint32_t)-1) {
        ve2SwapchainExtent.width = w_width;
        ve2SwapchainExtent.height = w_height;
    } else {
        // If the surface size is defined, the swap chain size must match.
        ve2SwapchainExtent = surfCap.currentExtent;
        w_width = surfCap.currentExtent.width;
        w_height = surfCap.currentExtent.height;
    }

    uint32_t presentModeCount;
    err = vkGetPhysicalDeviceSurfacePresentModesKHR(vpdGpu, vsSurface,
                                                    &presentModeCount, NULL);
    assert(!err);
    VkPresentModeKHR* presentModes = new VkPresentModeKHR[presentModeCount];
    err = vkGetPhysicalDeviceSurfacePresentModesKHR(vpdGpu, vsSurface,
                                                    &presentModeCount,
                                                    presentModes);
    assert(!err);

    // If mailbox mode is available, use it, as is the lowest-latency non-
    // tearing mode. If not, fall back to FIFO which is always available.
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (size_t i = 0; i < presentModeCount; i++) {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    // Determine the number of VkImage's to use in the swap chain. Want to
    // own only 1 image at a time, besides the images being displayed and
    // queued for display).
    uint32_t desiredImageCount = surfCap.minImageCount + 1;
    if ((surfCap.maxImageCount > 0)
        && (desiredImageCount > surfCap.maxImageCount)) {
        // Application must settle for fewer images than desired:
        desiredImageCount = surfCap.maxImageCount;
    }

    const class MySwapChainInfo : public VkSwapchainCreateInfoKHR {
      public:
        MySwapChainInfo(VkAppSDL& app,
                        uint32_t minImageCount,
                        VkSurfaceTransformFlagBitsKHR preTransform,
                        VkPresentModeKHR presentMode)
        {
            sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            pNext = NULL;
            flags = 0;
            surface = app.vsSurface;
            this->minImageCount = minImageCount;
            imageFormat = app.vfFormat;
            imageColorSpace = app.vcsColorSpace;
            imageExtent = app.ve2SwapchainExtent;
            imageArrayLayers = 1;
            imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            queueFamilyIndexCount = 0;
            pQueueFamilyIndices = NULL;
            this->preTransform = preTransform;
            compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            this->presentMode = presentMode;
            oldSwapchain = NULL;
            clipped = true;
        }
    } swapchainInfo(*this, desiredImageCount,
                    surfCap.currentTransform, swapchainPresentMode);

    err = vkCreateSwapchainKHR(vdDevice, &swapchainInfo, NULL, &vscSwapchain);
    assert(!err);

    delete [] formats;
    delete [] presentModes;

    return true;
} // createSwapchain


bool
VkAppSDL::prepareColorBuffers()
{
    VkResult err;

    err = vkGetSwapchainImagesKHR(vdDevice, vscSwapchain,
                                  &swapchainImageCount,
                                  NULL);
    VkImage* swapchainImages = new VkImage[swapchainImageCount];
    err = vkGetSwapchainImagesKHR(vdDevice, vscSwapchain,
                                  &swapchainImageCount,
                                  swapchainImages);
    assert(!err);

    scBuffers = new SwapchainBuffers[swapchainImageCount];
    assert(scBuffers);

    for (uint32_t i = 0; i < swapchainImageCount; i++) {
        VkImageViewCreateInfo colorImageView = { };
        colorImageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        colorImageView.pNext = NULL;
        colorImageView.format = vfFormat;
        colorImageView.components.r = VK_COMPONENT_SWIZZLE_R;
        colorImageView.components.g = VK_COMPONENT_SWIZZLE_G;
        colorImageView.components.b = VK_COMPONENT_SWIZZLE_B;
        colorImageView.components.a = VK_COMPONENT_SWIZZLE_A;
        colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorImageView.subresourceRange.baseMipLevel = 0;
        colorImageView.subresourceRange.levelCount = 1;
        colorImageView.subresourceRange.baseArrayLayer = 0;
        colorImageView.subresourceRange.layerCount = 1;
        colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        colorImageView.flags = 0;

        scBuffers[i].image = swapchainImages[i];

        colorImageView.image = scBuffers[i].image;

        err = vkCreateImageView(vdDevice, &colorImageView, NULL,
                                &scBuffers[i].view);
        assert(!err);
    }

    delete [] swapchainImages;
    return true;
} // prepareColorBuffers


bool
VkAppSDL::prepareDepthBuffer()
{
    const VkFormat depth_format = VK_FORMAT_D16_UNORM;
    const class MyImageCreateInfo : public VkImageCreateInfo {
      public:
        MyImageCreateInfo(VkAppSDL& app) {
            sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            pNext = NULL;
            imageType = VK_IMAGE_TYPE_2D;
            format = depth_format;
            extent.width = app.w_width;
            extent.height = app.w_height;
            extent.depth = 1;
            mipLevels = 1;
            arrayLayers = 1;
            samples = VK_SAMPLE_COUNT_1_BIT;
            tiling = VK_IMAGE_TILING_OPTIMAL;
            usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            flags = 0;
        }
    } image(*this);

    VkImageViewCreateInfo view = { };
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.pNext = NULL;
    view.image = VK_NULL_HANDLE;
    view.format = depth_format;
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    view.subresourceRange.baseMipLevel = 0;
    view.subresourceRange.levelCount = 1;
    view.subresourceRange.baseArrayLayer = 0;
    view.subresourceRange.layerCount = 1;
    view.flags = 0;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;

    VkMemoryRequirements mem_reqs;
    VkResult U_ASSERT_ONLY err;
    bool U_ASSERT_ONLY pass;

    depth.format = depth_format;

    /* create image */
    err = vkCreateImage(vdDevice, &image, NULL, &depth.image);
    assert(!err);

    vkGetImageMemoryRequirements(vdDevice, depth.image, &mem_reqs);
    assert(!err);

    depth.memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    depth.memAlloc.pNext = NULL;
    depth.memAlloc.allocationSize = mem_reqs.size;
    depth.memAlloc.memoryTypeIndex = 0;

    pass = memoryTypeFromProperties(mem_reqs.memoryTypeBits,
                                    0, /* No requirements */
                                    &depth.memAlloc.memoryTypeIndex);
    assert(pass);

    /* allocate memory */
    err = vkAllocateMemory(vdDevice, &depth.memAlloc, NULL, &depth.mem);
    assert(!err);

    /* bind memory */
    err =
        vkBindImageMemory(vdDevice, depth.image, depth.mem, 0);
    assert(!err);

    setImageLayout(depth.image, VK_IMAGE_ASPECT_DEPTH_BIT,
                   VK_IMAGE_LAYOUT_UNDEFINED,
                   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                   (VkAccessFlagBits)0);

    /* create image view */
    view.image = depth.image;
    err = vkCreateImageView(vdDevice, &view, NULL, &depth.view);
    assert(!err);

    return true;
} // prepareDepthBuffer


bool
VkAppSDL::prepareDescriptorLayout()
{
    return true;
}


bool
VkAppSDL::prepareRenderPass()
{
    VkAttachmentDescription attachments[2] = { };
      attachments[0].format = vfFormat;
      attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
      attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      attachments[1].format = depth.format;
      attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
      attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachments[1].initialLayout =
                     VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      attachments[1].finalLayout =
                     VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    const VkAttachmentReference color_reference = {
        0,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const VkAttachmentReference depth_reference = {
        1,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    const VkSubpassDescription subpass = {
        0,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        0,
        NULL,
        1,
        &color_reference,
        NULL,
        &depth_reference,
        0,
        NULL,
    };
    const VkRenderPassCreateInfo rp_info = {
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        NULL,
        0,
        2,
        attachments,
        1,
        &subpass,
        0,
        NULL,
    };
    VkResult U_ASSERT_ONLY err;

    err = vkCreateRenderPass(vdDevice, &rp_info, NULL, &vrpRenderPass);
    assert(!err);
    return true;
}


bool
VkAppSDL::preparePipeline()
{
    return true;
}


bool
VkAppSDL::prepareDescriptorPool()
{
    return true;

}


bool
VkAppSDL::prepareDescriptorSet()
{
    return true;

}


bool
VkAppSDL::prepareFramebuffers()
{
    VkImageView attachments[2];
    attachments[1] = depth.view;

    const VkFramebufferCreateInfo fb_info = {
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        NULL,
        0,
        vrpRenderPass,
        2,
        attachments,
        static_cast<uint32_t>(w_width),
        static_cast<uint32_t>(w_height),
        1,
    };
    VkResult U_ASSERT_ONLY err;
    uint32_t i;

    for (i = 0; i < swapchainImageCount; i++) {
        attachments[0] = scBuffers[i].view;
        err = vkCreateFramebuffer(vdDevice, &fb_info, NULL,
                                  &scBuffers[i].fb);
        assert(!err);
    }

    return true;
}


void
VkAppSDL::flushInitialCommands()
{
    VkResult U_ASSERT_ONLY err;

    if (vcbCommandBuffer == VK_NULL_HANDLE)
        return;

    err = vkEndCommandBuffer(vcbCommandBuffer);
    assert(!err);

    const VkCommandBuffer cmd_bufs[] = { vcbCommandBuffer };
    VkFence nullFence = VK_NULL_HANDLE;
    VkSubmitInfo sInfo;
    sInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    sInfo.pNext = NULL;
    sInfo.waitSemaphoreCount = 0;
    sInfo.pWaitSemaphores = NULL;
    sInfo.pWaitDstStageMask = NULL;
    sInfo.commandBufferCount = 1;
    sInfo.pCommandBuffers = cmd_bufs;
    sInfo.signalSemaphoreCount = 0;
    sInfo.pSignalSemaphores = NULL;

    err = vkQueueSubmit(vqQueue, 1, &sInfo, nullFence);
    assert(!err);

    err = vkQueueWaitIdle(vqQueue);
    assert(!err);

    vkFreeCommandBuffers(vdDevice, vcpCommandPool, 1, cmd_bufs);
    vcbCommandBuffer = VK_NULL_HANDLE;

}


//----------------------------------------------------------------------
//  Utility functions
//----------------------------------------------------------------------


void
VkAppSDL::setImageLayout(VkImage image, VkImageAspectFlags aspectMask,
        VkImageLayout old_image_layout,
        VkImageLayout new_image_layout,
        VkAccessFlagBits srcAccessMask)
{
    VkResult U_ASSERT_ONLY err;

    if (vcbCommandBuffer == VK_NULL_HANDLE) {
        const VkCommandBufferAllocateInfo cbaInfo = {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            NULL,
            vcpCommandPool,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            1,
        };

        err = vkAllocateCommandBuffers(vdDevice, &cbaInfo, &vcbCommandBuffer);
        assert(!err);

        VkCommandBufferInheritanceInfo cmdBufHInfo = { };
        cmdBufHInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        cmdBufHInfo.pNext = NULL;
        cmdBufHInfo.renderPass = VK_NULL_HANDLE;
        cmdBufHInfo.subpass = 0;
        cmdBufHInfo.framebuffer = VK_NULL_HANDLE;
        cmdBufHInfo.occlusionQueryEnable = VK_FALSE;
        cmdBufHInfo.queryFlags = 0;
        cmdBufHInfo.pipelineStatistics = 0;

        VkCommandBufferBeginInfo cmdBufInfo = { };
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        cmdBufInfo.pNext = NULL,
        cmdBufInfo.flags = 0,
        cmdBufInfo.pInheritanceInfo = &cmdBufHInfo,

        err = vkBeginCommandBuffer(vcbCommandBuffer, &cmdBufInfo);
        assert(!err);
    }

    VkImageMemoryBarrier imageMemoryBarrier = { };
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext = NULL;
    imageMemoryBarrier.srcAccessMask = srcAccessMask;
    imageMemoryBarrier.dstAccessMask = 0;
    imageMemoryBarrier.oldLayout = old_image_layout;
    imageMemoryBarrier.newLayout = new_image_layout;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange.aspectMask = aspectMask;
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.levelCount = 1;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount = 1;

    if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        /* Make sure anything that was copying from this image has completed */
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    }

    if (new_image_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        imageMemoryBarrier.dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }

    if (new_image_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        imageMemoryBarrier.dstAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    if (new_image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        /* Make sure any Copy or CPU writes to image are flushed */
        imageMemoryBarrier.dstAccessMask =
            VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    }

    VkImageMemoryBarrier *pMemoryBarrier = &imageMemoryBarrier;

    VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    vkCmdPipelineBarrier(vcbCommandBuffer, src_stages, dest_stages, 0, 0, NULL, 0,
                         NULL, 1, pMemoryBarrier);
} // setImageLayout


/*
 * @internal
 * @~English
 * @brief Check if all layer names specified in @p names can be
 *        found in the given layer properties.
 *
 * @return true if all layer names can be found, false otherwise.
 */
bool
VkAppSDL::checkLayers(uint32_t nameCount, const char **names,
                      uint32_t layerCount,
                      VkLayerProperties *layers) {
    for (uint32_t i = 0; i < nameCount; i++) {
        bool found = 0;
        for (uint32_t j = 0; j < layerCount; j++) {
            if (!strcmp(names[i], layers[j].layerName)) {
                found = 1;
                break;
            }
        }
        if (!found) {
            fprintf(stderr, "Cannot find layer: %s\n", names[i]);
            return 0;
        }
    }
    return 1;
}


bool
VkAppSDL::memoryTypeFromProperties(uint32_t typeBits,
                                   VkFlags requirementsMask,
                                   uint32_t *typeIndex)
{
    // Search memtypes to find first index with desired properties
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
        if ((typeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if ((memoryProperties.memoryTypes[i].propertyFlags &
                 requirementsMask) == requirementsMask) {
                *typeIndex = i;
                return true;
            }
        }
        typeBits >>= 1;
    }
    // No memory types matched, return failure
    return false;
}



VKAPI_ATTR VkBool32 VKAPI_CALL
VkAppSDL::debugFunc(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
                    uint64_t srcObject, size_t location, int32_t msgCode,
                    const char *pLayerPrefix, const char *pMsg, void *pUserData) {
    VkAppSDL* app = (VkAppSDL*)pUserData;
    return app->debugFunc(msgFlags, objType, srcObject, location, msgCode,
                          pLayerPrefix, pMsg);
}


VKAPI_ATTR VkBool32 VKAPI_CALL
VkAppSDL::debugFunc(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
                    uint64_t srcObject, size_t location, int32_t msgCode,
                    const char *pLayerPrefix, const char *pMsg) {

    std::string title = szName;
    char* message = new char[strlen(pMsg) + 100];

    assert(message);

    if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        sprintf(message, "ERROR: [%s] Code %d : %s", pLayerPrefix, msgCode,
                pMsg);
    } else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        // We know that we're submitting queues without fences, ignore this
        // warning
        if (strstr(pMsg,
                   "vkQueueSubmit parameter, VkFence fence, is null pointer")) {
            return false;
        }
        sprintf(message, "WARNING: [%s] Code %d : %s", pLayerPrefix, msgCode,
                pMsg);
    } else {
        return false;
    }

    title += ": alert";
    (void)SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_WARNING, title.c_str(), message, NULL
    );
    delete[] message;

    /*
     * false indicates that layer should not bail-out of an
     * API call that had validation failures. This may mean that the
     * app dies inside the driver due to invalid parameter(s).
     * That's what would happen without validation layers, so we'll
     * keep that behavior here.
     */
    return false;
}


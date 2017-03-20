/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

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

/**
 * @internal
 * @class VulkanAppSDL
 * @~English
 *
 * @brief Framework for Vulkan apps using SDL windows.
 *
 * @author Mark Callow, www.edgewise-consulting.com.
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
#include <iomanip>
#include <string>
#include <sstream>

#include "VulkanAppSDL.h"
// Include this when vulkantools is removed.
//#include "vulkancheckres.h"
#include <SDL_vulkan.h>

#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))

#define ERROR_RETURN(msg) \
      (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, szName, msg, NULL); \
      return false;

#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                              \
  {                                                                           \
    pfn##entrypoint =                                                         \
        (PFN_vk##entrypoint)vkGetInstanceProcAddr(inst, "vk" #entrypoint);    \
    if (pfn##entrypoint == NULL) {                                            \
        ERROR_RETURN("vkGetInstanceProcAddr: unable to find vk" #entrypoint); \
    }                                                                         \
  }

#define GET_DEVICE_PROC_ADDR(device, entrypoint)                            \
  {                                                                         \
    pfn##entrypoint =                                                       \
        (PFN_vk##entrypoint)vkGetDeviceProcAddr(device, "vk" #entrypoint);  \
    if (pfn##entrypoint == NULL) {                                          \
        ERROR_RETURN("vkGetDeviceProcAddr: unable to find vk" #entrypoint); \
    }                                                                       \
  }


VulkanAppSDL::~VulkanAppSDL()
{

}


bool
VulkanAppSDL::initialize(int argc, char* argv[])
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
  #if !defined(__IPHONEOS__) && !defined(__MACOSX__)
	validate = true;
  #endif
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
        (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, szName,
                                       SDL_GetError(), NULL);
        return false;
    }

	if (!initializeVulkan()) {
	    return false;
	}

	subOptimalPresentWarned = false;

    // Not getting an initial resize event, at least on Mac OS X.
    // Therefore call resize directly.
    
    //resize(w_width, w_height);

    initializeFPSTimer();
    return true;
}


void
VulkanAppSDL::finalize()
{
    if (vkctx.descriptorPool)
    {
        vkDestroyDescriptorPool(vkctx.device, vkctx.descriptorPool, nullptr);
    }
    vkDestroyPipelineCache(vkctx.device, vkctx.pipelineCache, nullptr);

    vkctx.swapchain.cleanup();
    for (auto& shaderModule : shaderModules)
    {
        vkctx.device.destroyShaderModule(shaderModule);
    }
    if (enableTextOverlay)
    {
        delete textOverlay;
    }

    vkctx.device.destroyCommandPool(vkctx.commandPool);
    //vkctx.device.destroy();
    if (vsSurface != VK_NULL_HANDLE) {
        // Destroy
    }
    //vkctx.instance.destroy();
}


int
VulkanAppSDL::doEvent(SDL_Event* event)
{
    switch (event->type) {
      case SDL_WINDOWEVENT:
        switch (event->window.event) {
          case SDL_WINDOWEVENT_SIZE_CHANGED:
            resizeWindow(event->window.data1, event->window.data2);
		    return 0;
        }
        break;
            
    }
    return AppBaseSDL::doEvent(event);
}


void
VulkanAppSDL::drawFrame(uint32_t msTicks)
{
    if (!prepared)
        return;

    prepareFrame();

    vkctx.drawCmdSubmitInfo.commandBufferCount = 1;
    vkctx.drawCmdSubmitInfo.pCommandBuffers =
                    &vkctx.drawCmdBuffers[currentBuffer];

    // Submit to queue
    VK_CHECK_RESULT(vkQueueSubmit(vkctx.queue, 1,
                                  &vkctx.drawCmdSubmitInfo, VK_NULL_HANDLE));

    submitFrame();
}


void
VulkanAppSDL::windowResized()
{
    // Derived class can override as necessary.
}

void
VulkanAppSDL::resizeWindow(int width, int height)
{
    // XXX Necessary? Get out-of-date errors from vkAcquireNextImage regardless
    // of whether this guard is used. This guard doesn't seem to make them any
    // less likely.
#define GUARD 0
#if GUARD
    if (!prepared)
    {
        return;
    }
    prepared = false;
#endif

    // Recreate swap chain
    w_width = width;
    w_height = height;

    // This destroys any existing swapchain and makes a new one.
    createSwapchain();

    vkDestroyImageView(vkctx.device, vkctx.depthBuffer.view, nullptr);
    vkDestroyImage(vkctx.device, vkctx.depthBuffer.image, nullptr);
    vkFreeMemory(vkctx.device, vkctx.depthBuffer.mem, nullptr);

    // Recreate the frame buffers
    for (uint32_t i = 0; i < vkctx.framebuffers.size(); i++)
    {
        vkDestroyFramebuffer(vkctx.device, vkctx.framebuffers[i], nullptr);
    }

    // XXX Is this necessary? Is Willems doing this?
    vkDestroyRenderPass(vkctx.device, vkctx.renderPass, NULL);

    vkctx.destroyPresentCommandBuffers();
    (void)(prepareDepthBuffer() // XXX Call it DepthStencil?
        && vkctx.createPresentCommandBuffers()
        && preparePresentCommandBuffers()
        && prepareRenderPass()
        && prepareFramebuffers());

    flushInitialCommands();
    if (enableTextOverlay)
    {
        textOverlay->reallocateCommandBuffers();
        updateTextOverlay();
    }
    // Notify derived class.
    windowResized();
#if GUARD
    prepared = true;
#endif
}


void
VulkanAppSDL::onFPSUpdate()
{
    if (!enableTextOverlay) {
        setWindowTitle();
    }
    updateTextOverlay();
    // Using onFPSUpdate avoids rewriting the title every frame.
}


//----------------------------------------------------------------------
//  Frame draw utilities
//----------------------------------------------------------------------


void
VulkanAppSDL::prepareFrame()
{
    // Acquire the next image from the swap chain
    VkResult err;
    err = vkctx.swapchain.acquireNextImage(semaphores.presentComplete,
                                           &currentBuffer);

    if (err == VK_ERROR_OUT_OF_DATE_KHR) {
        // Swap chain is out of date (e.g. the window was resized).
        // Re-create it.
        //resize();
        //draw(demo);
        return;
    } else if (err == VK_SUBOPTIMAL_KHR) {
        // demo->swapchain is not as optimal as it could be, but the platform's
        // presentation engine will still present the image correctly.
    } else {
        assert(!err);
    }

    // Submit post present image barrier to transform the image back to a
    // color attachment that our render pass can write to
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = NULL;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkctx.postPresentCmdBuffers[currentBuffer];
    VK_CHECK_RESULT(vkQueueSubmit(vkctx.queue, 1,
                                  &submitInfo, VK_NULL_HANDLE));
}


void
VulkanAppSDL::submitFrame()
{
    bool submitTextOverlay = enableTextOverlay && textOverlay->visible;

    if (submitTextOverlay)
    {
        // Wait for color attachment output to finish before rendering the
        // text overlay
        VkPipelineStageFlags stageFlags
                               = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        vkctx.drawCmdSubmitInfo.pWaitDstStageMask = &stageFlags;

        // Set semaphores
        // Wait for render complete semaphore
        vkctx.drawCmdSubmitInfo.waitSemaphoreCount = 1;
        vkctx.drawCmdSubmitInfo.pWaitSemaphores = &semaphores.renderComplete;
        // Signal ready with text overlay complete semaphore
        vkctx.drawCmdSubmitInfo.signalSemaphoreCount = 1;
        vkctx.drawCmdSubmitInfo.pSignalSemaphores
                                            = &semaphores.textOverlayComplete;

        // Submit current text overlay command buffer
        vkctx.drawCmdSubmitInfo.commandBufferCount = 1;
        vkctx.drawCmdSubmitInfo.pCommandBuffers
                                     = &textOverlay->cmdBuffers[currentBuffer];
        VK_CHECK_RESULT(vkQueueSubmit(vkctx.queue, 1, &vkctx.drawCmdSubmitInfo,
                                      VK_NULL_HANDLE));

        // Reset stage mask
        vkctx.drawCmdSubmitInfo.pWaitDstStageMask = &vkctx.submitPipelineStages;
        // Reset wait and signal semaphores for rendering next frame
        // Wait for swap chain presentation to finish
        vkctx.drawCmdSubmitInfo.waitSemaphoreCount = 1;
        vkctx.drawCmdSubmitInfo.pWaitSemaphores = &semaphores.presentComplete;
        // Signal ready with offscreen semaphore
        vkctx.drawCmdSubmitInfo.signalSemaphoreCount = 1;
        vkctx.drawCmdSubmitInfo.pSignalSemaphores = &semaphores.renderComplete;
    }
    // Submit pre-present image barrier to transform the image from color
    // attachment to present(khr) for presenting to the swap chain.
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = NULL;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkctx.prePresentCmdBuffers[currentBuffer];
    VK_CHECK_RESULT(vkQueueSubmit(vkctx.queue, 1, &submitInfo, VK_NULL_HANDLE));

    VkResult err =
            vkctx.swapchain.queuePresent(vkctx.queue, currentBuffer,
               submitTextOverlay ?
                   semaphores.textOverlayComplete : semaphores.renderComplete);

    if (err == VK_ERROR_OUT_OF_DATE_KHR) {
        // swapchain is out of date (e.g. the window was resized) and
        // must be recreated:
        //resize();
    } else if (err == VK_SUBOPTIMAL_KHR) {
        if (!subOptimalPresentWarned) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, szName,
                                     "Suboptimal present of framebuffer.",
                                     NULL);
        }
    } else
        assert(!err);

    // This is necessary because we use the same command buffers every frame
    // and they aren't marked for simultaneous use.
    VK_CHECK_RESULT(vkQueueWaitIdle(vkctx.queue));
}


//----------------------------------------------------------------------
//  Vulkan Initialization
//----------------------------------------------------------------------


bool
VulkanAppSDL::initializeVulkan()
{
    if (createInstance()
            && findGpu()
            && setupDebugReporting()
            && vkctx.swapchain.connectInstance(vkctx.instance, vkctx.gpu)
            && createSurface()
            && createDevice()
            && vkctx.swapchain.connectDevice(vkctx.device)
            && createSemaphores()
            && createSwapchain()
            && prepareDepthBuffer()
            && vkctx.createPresentCommandBuffers()
            && preparePresentCommandBuffers()
            && prepareRenderPass()
            && createPipelineCache()
            && prepareFramebuffers()) {
        // Functions above most likely generate pipeline commands
        // that need to be flushed before beginning the render loop.
        flushInitialCommands();
        prepareTextOverlay();

        return true;
    } else {
        return false;
    }
}


bool
VulkanAppSDL::createInstance()
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

            validationFound = checkLayers(
                                     ARRAY_LEN(instanceValidationLayers_alt1),
                                     instanceValidationLayers,
                                     instanceLayerCount,
                                     instanceLayers);
            if (validationFound) {
                enabledLayerCount = ARRAY_LEN(instanceValidationLayers_alt1);
                deviceValidationLayers[0]
                                       = "VK_LAYER_LUNARG_standard_validation";
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
    memset(extensionNames, 0, sizeof(extensionNames));

    extensionNames[enabledExtensionCount++] = VK_KHR_SURFACE_EXTENSION_NAME;
    if (validate)
        extensionNames[enabledExtensionCount++] =
                                           VK_EXT_DEBUG_REPORT_EXTENSION_NAME;

    unsigned c = ARRAY_LEN(extensionNames) - enabledExtensionCount;
    if (!SDL_GetVulkanInstanceExtensions(&c,
                                     &extensionNames[enabledExtensionCount])) {
        std::string msg;
        msg = "SDL_GetVulkanInstanceExtensions failed: ";
        msg += SDL_GetError();
        ERROR_RETURN(msg.c_str());
    }
    enabledExtensionCount += c;

    const vk::ApplicationInfo app(szName, 0, szName, 0, vkVersion);

    vk::InstanceCreateInfo instanceInfo(
								{},
								&app,
								enabledLayerCount,
								(const char *const *)instanceValidationLayers,
								enabledExtensionCount,
								(const char *const *)extensionNames);

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
            VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT
            | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
            //| VK_DEBUG_REPORT_INFORMATION_BIT_EXT
            //| VK_DEBUG_REPORT_DEBUG_BIT_EXT,
        instanceInfo.pNext = &dbgCreateInfo;
    }

    //err = vkCreateInstance(&instanceInfo, NULL, &vkctx.instance);
    vk::Result cerr;
    cerr = vk::createInstance(&instanceInfo, nullptr, &vkctx.instance);

    if (cerr != vk::Result::eSuccess) {
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

            err = vkEnumerateInstanceExtensionProperties(
                                                        NULL,
                                                        &instanceExtensionCount,
                                                        NULL);
            VkExtensionProperties* instanceExtensions
                = new VkExtensionProperties[instanceExtensionCount];

            assert(err == VK_SUCCESS);
            if (instanceExtensionCount > 0) {
                err = vkEnumerateInstanceExtensionProperties(
                                                        NULL,
                                                        &instanceExtensionCount,
                                                        instanceExtensions);
                assert(err == VK_SUCCESS);
            }
            msg << "Cannot find the following extensions:\n";
            for (int i = 0; i < enabledExtensionCount; i++) {
                int j;
                for (j = 0; j < instanceExtensionCount; j++) {
                    if (!strcmp(extensionNames[i],
                                instanceExtensions[j].extensionName))
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

    return true;
} // createInstance


bool
VulkanAppSDL::findGpu()
{
    vk::Result err;
    uint32_t gpuCount;

    // Make initial call to query gpu_count, then second call for gpu info.
    err = vkctx.instance.enumeratePhysicalDevices(&gpuCount, NULL);
    assert(err == vk::Result::eSuccess);

    if (gpuCount > 0) {
    	std::vector<vk::PhysicalDevice> gpus;
    	gpus.resize(gpuCount);
        err = vkctx.instance.enumeratePhysicalDevices(&gpuCount, gpus.data());
        assert(err == vk::Result::eSuccess);
        // For now just grab the first physical device */
        vkctx.gpu = gpus[0];
        // Store properties and features so apps can query them.
        vkctx.gpu.getProperties(&vkctx.gpuProperties);
        vkctx.gpu.getFeatures(&vkctx.gpuFeatures);
    } else {
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
    //vkGetPhysicalDeviceFeatures(vkctx.gpu, &physDevFeatures);

    // Get Memory information and properties
    vkctx.gpu.getMemoryProperties(&vkctx.memoryProperties);

    return true;
} // findGpu


bool
VulkanAppSDL::setupDebugReporting()
{
    if (validate) {
        GET_INSTANCE_PROC_ADDR(vkctx.instance, CreateDebugReportCallbackEXT);
        GET_INSTANCE_PROC_ADDR(vkctx.instance, DestroyDebugReportCallbackEXT);
        GET_INSTANCE_PROC_ADDR(vkctx.instance, DebugReportMessageEXT);
        VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
        dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgCreateInfo.pNext = NULL;
        dbgCreateInfo.pfnCallback = debugFunc;
        dbgCreateInfo.pUserData = this;
        dbgCreateInfo.flags =
            VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        VkResult err = pfnCreateDebugReportCallbackEXT(vkctx.instance,
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
VulkanAppSDL::createSurface()
{
    vkctx.swapchain.initSurface(pswMainWindow);
    return true;
} // createSurface


bool
VulkanAppSDL::createDevice()
{
    const char *deviceExtensionNames[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    uint32_t enabledDeviceExtensionCount = ARRAY_LEN(deviceExtensionNames);
    vk::Result err;

    float queue_priorities[1] = {0.0};
    const vk::DeviceQueueCreateInfo queueInfo(
		{},
		vkQueueFamilyIndex,
		1,
		queue_priorities
	);

	vk::DeviceCreateInfo deviceInfo(
			{},
			1,
			&queueInfo,
			enabledLayerCount,
			(const char *const *)((validate)
								  ? deviceValidationLayers
								  : NULL),
			enabledDeviceExtensionCount,
			(const char *const *)deviceExtensionNames);
			// If specific features are required, pass them as a final
			// argument to deviceInfo.

	err = vkctx.gpu.createDevice(&deviceInfo, NULL, &vkctx.device);
	if (err != vk::Result::eSuccess) {
        std::string title;
        std::stringstream msg;
        title = szName;
        title += ": vkCreateDevice Failure";
        if (err == vk::Result::eErrorExtensionNotPresent) {
            // Figure out which extension(s) are missing.
            uint32_t deviceExtensionCount = 0;

            err = vkctx.gpu.enumerateDeviceExtensionProperties(
            										   NULL,
                                                       &deviceExtensionCount,
                                                       NULL);
            assert(err == vk::Result::eSuccess);

            std::vector<vk::ExtensionProperties> deviceExtensions;
            deviceExtensions.resize(deviceExtensionCount);

            if (deviceExtensionCount > 0) {
                err = vkctx.gpu.enumerateDeviceExtensionProperties(
                										NULL,
                                                        &deviceExtensionCount,
                                                        deviceExtensions.data());
                assert(err == vk::Result::eSuccess);
            }
            msg << "Cannot find the following device extensions:\n";
            for (int i = 0; i < enabledDeviceExtensionCount; i++) {
                int j;
                for (j = 0; j < deviceExtensionCount; j++) {
                    if (!strcmp(extensionNames[i],
                                deviceExtensions[j].extensionName))
                        break;
                }
                if (j == deviceExtensionCount) {
                    // Not found
                    msg << "    " << extensionNames[i] << "\n";
                }
            }
            msg << "\n\nDo you have a compatible Vulkan "
                   "installable client driver (ICD) installed?";
       } else {
            msg << "vkCreateDevice: unexpected failure, result code = ";
            msg << err << ".";
       }
       (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.c_str(),
                                      msg.str().c_str(), NULL);
       return false;
    }

    const vk::CommandPoolCreateInfo cmdPoolInfo(
    	vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        vkctx.swapchain.queueIndex
    );
    err = vkctx.device.createCommandPool(&cmdPoolInfo,
                              nullptr, &vkctx.commandPool);
    assert(err == vk::Result::eSuccess);

    vkctx.device.getQueue(vkctx.swapchain.queueIndex, 0, &vkctx.queue);

    return true;
} // createDevice


// Create synchronization objects
bool
VulkanAppSDL::createSemaphores()
{
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = NULL;
    semaphoreCreateInfo.flags = 0;

    // Semaphore used to synchronize image presentation.
    // Ensures that the image is displayed before we start submitting new
    // commands to the queue.
    VK_CHECK_RESULT(vkCreateSemaphore(vkctx.device,
                                      &semaphoreCreateInfo,
                                      nullptr,
                                      &semaphores.presentComplete));
    // Semaphore used to synchronize render command submission.
    // Ensures that the image is not presented until all render commands have
    // been submitted and executed.
    VK_CHECK_RESULT(vkCreateSemaphore(vkctx.device,
                                      &semaphoreCreateInfo,
                                      nullptr,
                                      &semaphores.renderComplete));
    // Semaphore used to synchronize text overlay command submission.
    // Ensures that the image is not presented until all commands for the
    // text overlay have been submitted and executed. Will be inserted after
    // the render complete semaphore if the text overlay is enabled.
    VK_CHECK_RESULT(vkCreateSemaphore(vkctx.device,
                                      &semaphoreCreateInfo,
                                      nullptr,
                                      &semaphores.textOverlayComplete));

    // Set up submit info structure
    // Semaphores will stay the same during application lifetime
    // Command buffer submission info is set by each example
    vkctx.drawCmdSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    vkctx.drawCmdSubmitInfo.pNext = NULL;
    vkctx.drawCmdSubmitInfo.pWaitDstStageMask = &vkctx.submitPipelineStages;
    vkctx.drawCmdSubmitInfo.waitSemaphoreCount = 1;
    vkctx.drawCmdSubmitInfo.pWaitSemaphores = &semaphores.presentComplete;
    vkctx.drawCmdSubmitInfo.signalSemaphoreCount = 1;
    vkctx.drawCmdSubmitInfo.pSignalSemaphores = &semaphores.renderComplete;
    return true;
}


bool
VulkanAppSDL::createSwapchain()
{
    vkctx.swapchain.create(&w_width, &w_height, enableVSync);
    return true;
} // createSwapchain


bool
VulkanAppSDL::prepareDepthBuffer()
{
    const VkFormat depthFormat = VK_FORMAT_D16_UNORM;
    vkctx.depthBuffer.format = depthFormat;

    VkImageCreateInfo image = {};
    image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image.pNext = NULL;
    image.imageType = VK_IMAGE_TYPE_2D;
    image.format = depthFormat;
    image.extent.width = w_width;
    image.extent.height = w_height;
    image.extent.depth = 1;
    image.mipLevels = 1;
    image.arrayLayers = 1;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                            | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    image.flags = 0;

    VkImageViewCreateInfo view = { };
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.pNext = NULL;
    view.image = VK_NULL_HANDLE;
    view.format = depthFormat;
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

    /* create image */
    err = vkCreateImage(vkctx.device, &image, NULL, &vkctx.depthBuffer.image);
    assert(!err);

    vkGetImageMemoryRequirements(vkctx.device, vkctx.depthBuffer.image, &mem_reqs);
    assert(!err);

    vkctx.depthBuffer.memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vkctx.depthBuffer.memAlloc.pNext = NULL;
    vkctx.depthBuffer.memAlloc.allocationSize = mem_reqs.size;
    vkctx.depthBuffer.memAlloc.memoryTypeIndex = 0;

    pass = vkctx.getMemoryType(mem_reqs.memoryTypeBits,
                               vk::MemoryPropertyFlagBits::eDeviceLocal,
                               &vkctx.depthBuffer.memAlloc.memoryTypeIndex);
    assert(pass);

    /* allocate memory */
    err = vkAllocateMemory(vkctx.device, &vkctx.depthBuffer.memAlloc, NULL,
                           &vkctx.depthBuffer.mem);
    assert(!err);

    /* bind memory */
    err =
        vkBindImageMemory(vkctx.device, vkctx.depthBuffer.image,
                          vkctx.depthBuffer.mem, 0);
    assert(!err);

    setImageLayout(vkctx.depthBuffer.image, VK_IMAGE_ASPECT_DEPTH_BIT,
                   VK_IMAGE_LAYOUT_UNDEFINED,
                   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                   (VkAccessFlagBits)0);

    /* create image view */
    view.image = vkctx.depthBuffer.image;
    err = vkCreateImageView(vkctx.device, &view, NULL, &vkctx.depthBuffer.view);
    assert(!err);

    return true;
} // prepareDepthBuffer


bool
VulkanAppSDL::preparePresentCommandBuffers()
{
    VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufferBeginInfo.pNext = NULL;

    for (uint32_t i = 0; i < vkctx.swapchain.imageCount; i++)
    {
        // Command buffer for post present barrier

        // Insert a post present image barrier to transform the image back to a
        // color attachment that our render pass can write to. We always use
        // undefined image layout as the source as it doesn't actually matter
        // what is done with the previous image contents

        VK_CHECK_RESULT(vkBeginCommandBuffer(vkctx.postPresentCmdBuffers[i],
                                             &cmdBufferBeginInfo));

        VkImageMemoryBarrier postPresentBarrier = { };
        postPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        postPresentBarrier.pNext = NULL;
        postPresentBarrier.srcAccessMask = 0;
        postPresentBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        postPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        postPresentBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        postPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        postPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        postPresentBarrier.subresourceRange =
                                     { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        postPresentBarrier.image = vkctx.swapchain.buffers[i].image;

        vkCmdPipelineBarrier(
            vkctx.postPresentCmdBuffers[i],
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &postPresentBarrier);

        VK_CHECK_RESULT(vkEndCommandBuffer(vkctx.postPresentCmdBuffers[i]));

        // Command buffer for pre present barrier

        // Submit a pre present image barrier to the queue.
        // Transforms the (framebuffer) image layout from color attachment to
        // present(khr) for presenting to the swap chain.

        VK_CHECK_RESULT(vkBeginCommandBuffer(vkctx.prePresentCmdBuffers[i],
                                             &cmdBufferBeginInfo));

        VkImageMemoryBarrier prePresentBarrier = { };
        prePresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        prePresentBarrier.pNext = NULL;
        prePresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        prePresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        prePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        prePresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        prePresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        prePresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        prePresentBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        prePresentBarrier.image = vkctx.swapchain.buffers[i].image;

        vkCmdPipelineBarrier(
            vkctx.prePresentCmdBuffers[i],
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0,
            0, nullptr, // No memory barriers,
            0, nullptr, // No buffer barriers,
            1, &prePresentBarrier);

        VK_CHECK_RESULT(vkEndCommandBuffer(vkctx.prePresentCmdBuffers[i]));
    }
    return true;
}


bool
VulkanAppSDL::prepareDescriptorLayout()
{
    return true;
}


bool
VulkanAppSDL::prepareRenderPass()
{
    VkAttachmentDescription attachments[2] = { };
      attachments[0].format = vkctx.swapchain.colorFormat;
      attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
      attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      attachments[1].format = vkctx.depthBuffer.format;
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

    err = vkCreateRenderPass(vkctx.device, &rp_info, NULL, &vkctx.renderPass);
    assert(!err);
    return true;
}


bool
VulkanAppSDL::createPipelineCache()
{
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VK_CHECK_RESULT(vkCreatePipelineCache(vkctx.device,
                                          &pipelineCacheCreateInfo,
                                          nullptr, &vkctx.pipelineCache));
    return true;
}


bool
VulkanAppSDL::preparePipeline()
{
    return true;
}


bool
VulkanAppSDL::prepareDescriptorSet()
{
    return true;
}


bool
VulkanAppSDL::prepareFramebuffers()
{
    VkImageView attachments[2];
    attachments[1] = vkctx.depthBuffer.view;

    const VkFramebufferCreateInfo fb_info = {
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        NULL,
        0,
        vkctx.renderPass,
        2,
        attachments,
        w_width,
        w_height,
        1,
    };
    VkResult U_ASSERT_ONLY err;
    uint32_t i;

    vkctx.framebuffers.resize(vkctx.swapchain.imageCount);
    for (i = 0; i < vkctx.framebuffers.size(); i++) {
        attachments[0] = vkctx.swapchain.buffers[i].view;
        err = vkCreateFramebuffer(vkctx.device, &fb_info, NULL,
                                  &vkctx.framebuffers[i]);
        assert(!err);
    }
    return true;
}


void
VulkanAppSDL::flushInitialCommands()
{
    VkResult U_ASSERT_ONLY err;

    if (setupCmdBuffer == VK_NULL_HANDLE)
        return;

    err = vkEndCommandBuffer(setupCmdBuffer);
    assert(!err);

    const VkCommandBuffer cmd_bufs[] = { setupCmdBuffer };
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

    err = vkQueueSubmit(vkctx.queue, 1, &sInfo, nullFence);
    assert(!err);

    err = vkQueueWaitIdle(vkctx.queue);
    assert(!err);

    vkFreeCommandBuffers(vkctx.device, vkctx.commandPool, 1, cmd_bufs);
    setupCmdBuffer = VK_NULL_HANDLE;

}


//----------------------------------------------------------------------
//  Window title and text overlay functions
//----------------------------------------------------------------------


void
VulkanAppSDL::setAppTitle(const char* const szExtra)
{
    appTitle = name();
    if (szExtra != NULL && szExtra[0] != '\0') {
        appTitle += ": ";
        appTitle += szExtra;
    }
    setWindowTitle();
}


void
VulkanAppSDL::setWindowTitle()
{
    std::stringstream ss;
    std::string wt;

    if (!enableTextOverlay) {
        ss << std::fixed << std::setprecision(2)
           << lastFrameTime << "ms (" << fpsCounter.lastFPS << " fps)" << " ";
        wt = ss.str();
        wt += appTitle;
        SDL_SetWindowTitle(pswMainWindow, wt.c_str());
    } else {
        SDL_SetWindowTitle(pswMainWindow, appTitle.c_str());
    }
}


void
VulkanAppSDL::prepareTextOverlay()
{
    if (enableTextOverlay) {
        // Load the text rendering shaders
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        VkPipelineShaderStageCreateInfo shaderStage;
        std::string filepath = getAssetPath() + "shaders/textoverlay.vert.spv";
        shaderStage = vkctx.loadShader(filepath, vk::ShaderStageFlagBits::eVertex);
        shaderStages.push_back(shaderStage);
        shaderModules.push_back(shaderStage.module);
        filepath = getAssetPath() + "shaders/textoverlay.frag.spv";
        shaderStage = vkctx.loadShader(filepath, vk::ShaderStageFlagBits::eFragment);
        shaderStages.push_back(shaderStage);
        shaderModules.push_back(shaderStage.module);

        textOverlay = new VulkanTextOverlay(
            vkctx.gpu,
            vkctx.device,
            vkctx.queue,
            vkctx.framebuffers,
            vkctx.swapchain.colorFormat,
            vkctx.depthBuffer.format,
            &w_width,
            &w_height,
            shaderStages
            );
        updateTextOverlay();
    }
}


void VulkanAppSDL::updateTextOverlay()
{
    if (!enableTextOverlay)
        return;

    textOverlay->beginTextUpdate();

    textOverlay->addText(appTitle, 5.0f, 5.0f, VulkanTextOverlay::alignLeft);

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2)
       << lastFrameTime << "ms (" << fpsCounter.lastFPS << " fps)";
    textOverlay->addText(ss.str(), 5.0f, 25.0f, VulkanTextOverlay::alignLeft);

    textOverlay->addText(vkctx.gpuProperties.deviceName, 5.0f, 45.0f,
                         VulkanTextOverlay::alignLeft);

    getOverlayText(textOverlay);

    textOverlay->endTextUpdate();
}


void VulkanAppSDL::getOverlayText(VulkanTextOverlay *textOverlay)
{
    // Can be overriden in derived class
}


//----------------------------------------------------------------------
//  Utility functions
//----------------------------------------------------------------------


void
VulkanAppSDL::setImageLayout(VkImage image, VkImageAspectFlags aspectMask,
        VkImageLayout old_image_layout,
        VkImageLayout new_image_layout,
        VkAccessFlagBits srcAccessMask)
{
    VkResult U_ASSERT_ONLY err;

    if (setupCmdBuffer == VK_NULL_HANDLE) {
        const VkCommandBufferAllocateInfo cbaInfo = {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            NULL,
            vkctx.commandPool,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            1,
        };

        err = vkAllocateCommandBuffers(vkctx.device, &cbaInfo,
                                       &setupCmdBuffer);
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

        err = vkBeginCommandBuffer(setupCmdBuffer, &cmdBufInfo);
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

    vkCmdPipelineBarrier(setupCmdBuffer, src_stages, dest_stages, 0, 0,
                         NULL, 0, NULL, 1, pMemoryBarrier);
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
VulkanAppSDL::checkLayers(uint32_t nameCount, const char **names,
                      uint32_t layerCount,
                      VkLayerProperties *layers)
{
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


VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanAppSDL::debugFunc(VkDebugReportFlagsEXT msgFlags,
                    VkDebugReportObjectTypeEXT objType,
                    uint64_t srcObject, size_t location, int32_t msgCode,
                    const char *pLayerPrefix, const char *pMsg,
                    void *pUserData)
{
    VulkanAppSDL* app = (VulkanAppSDL*)pUserData;
    return app->debugFunc(msgFlags, objType, srcObject, location, msgCode,
                          pLayerPrefix, pMsg);
}


VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanAppSDL::debugFunc(VkDebugReportFlagsEXT msgFlags,
                    VkDebugReportObjectTypeEXT objType,
                    uint64_t srcObject, size_t location, int32_t msgCode,
                    const char *pLayerPrefix, const char *pMsg)
{

    std::string title = szName;
    std::string text(pMsg);
    std::string prefix("");
    std::stringstream message;
    uint32_t mbFlags;

    // Errors may cause undefined behaviour or a crash.
    if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        mbFlags = SDL_MESSAGEBOX_ERROR;
        prefix += "ERROR:";
    }
    // Warnings indicate use of Vulkan that may expose an app bug
    if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        // We know that we're submitting queues without fences, ignore this
        // warning
        // XXX Check this
        if (strstr(pMsg,
                   "vkQueueSubmit parameter, VkFence fence, is null pointer")) {
            return false;
        }
        mbFlags = SDL_MESSAGEBOX_WARNING;
        prefix += "WARNING:";
    }
    // Performance warnings indicate sub-optimal usage of the API
    if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
    {
        mbFlags = SDL_MESSAGEBOX_WARNING;
        prefix += "PERFORMANCE:";
    };
    // Information that may be handy during debugging.
    if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
    {
        mbFlags = SDL_MESSAGEBOX_INFORMATION;
        prefix += "INFO:";
    }
    // Diagnostic info from the Vulkan loader and layers.
#if 0
    // Usually not helpful in terms of API usage, but may help to debug layer and loader problems
    if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
    {
        mbFlags = SDL_MESSAGEBOX_INFORMATION;
        prefix += "DEBUG:";
    }
#endif
    message << prefix << " [" << pLayerPrefix << "] Code " << msgCode
            << " : " << text;

    title += ": alert";
    (void)SDL_ShowSimpleMessageBox(
        mbFlags, title.c_str(), message.str().c_str(), NULL
    );

    /*
     * false indicates that layer should not bail-out of an
     * API call that had validation failures. This may mean that the
     * app dies inside the driver due to invalid parameter(s).
     * That's what would happen without validation layers, so we'll
     * keep that behavior here.
     */
    return false;
}


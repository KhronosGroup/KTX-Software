/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

#ifndef VULKAN_LOAD_TEST_SAMPLE_H
#define VULKAN_LOAD_TEST_SAMPLE_H

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "LoadTestSample.h"
#include "VulkanAppSDL.h"
#include "VulkanContext.h"
// MeshLoader needs vulkantools.h to be already included. It is included by
// vulkantextoverlay.hpp via VulkanAppSDL.h.
#include "utils/VulkanMeshLoader.hpp"

#include <ktxvulkan.h>

namespace VMA_CALLBACKS
{
    void InitVMA(VkPhysicalDevice& physicalDevice, VkDevice& device, VkInstance& instance, 
                 VkPhysicalDeviceMemoryProperties& devMemProps);
    void DestroyVMA();
    uint64_t AllocMemCWrapper(VkMemoryAllocateInfo* allocInfo, VkMemoryRequirements* memReq, uint64_t* numPages);
    VkResult BindBufferMemoryCWrapper(VkBuffer buffer, uint64_t allocId);
    VkResult BindImageMemoryCWrapper(VkImage image, uint64_t allocId);
    VkResult MapMemoryCWrapper(uint64_t allocId, uint64_t, VkDeviceSize* mapLength, void** dataPtr);
    void UnmapMemoryCWrapper(uint64_t allocId, uint64_t);
    void FreeMemCWrapper(uint64_t allocId);
}

#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))

extern "C" const char* vkFormatString(VkFormat format);

class VulkanLoadTestSample : public LoadTestSample {
  public:
    typedef uint64_t ticks_t;
    VulkanLoadTestSample(VulkanContext& vkctx,
                     uint32_t width, uint32_t height,
                     /* const char* const szArgs, */
                     const std::string sBasePath, int32_t yflip = -1)
           : LoadTestSample(width, height, /*szArgs,*/ sBasePath, yflip),
             vkctx(vkctx),
             defaultClearColor(std::array<float,4>({0.025f, 0.025f, 0.025f, 1.0f}))
    {
    }

    virtual ~VulkanLoadTestSample();
    //virtual int doEvent(SDL_Event* event);
    virtual void resize(uint32_t width, uint32_t height) = 0;
    virtual void run(uint32_t msTicks) = 0;

    virtual void getOverlayText(VulkanTextOverlay* /*textOverlay*/,
                                float /*yoffset*/) { }
    virtual const char* customizeTitle(const char* const title) {
        return title;
    }

    typedef VulkanLoadTestSample* (*PFN_create)(VulkanContext&,
                                    uint32_t width, uint32_t height,
                                    const char* const szArgs,
                                    const std::string sBasePath);

  protected:

    ktxVulkanTexture_subAllocatorCallbacks subAllocatorCallbacks = {
        VMA_CALLBACKS::AllocMemCWrapper,
        VMA_CALLBACKS::BindBufferMemoryCWrapper,
        VMA_CALLBACKS::BindImageMemoryCWrapper,
        VMA_CALLBACKS::MapMemoryCWrapper,
        VMA_CALLBACKS::UnmapMemoryCWrapper,
        VMA_CALLBACKS::FreeMemCWrapper
    };

    virtual void keyPressed(uint32_t /*keyCode*/) { }
    virtual void viewChanged() { }
    bool gpuSupportsSwizzle() {
        return vkctx.gpuSupportsSwizzle();
    }

    std::string ktxfilename;
    int externalFile = 0;

    vk::PipelineShaderStageCreateInfo
    loadShader(std::string filename,
               vk::ShaderStageFlagBits stage,
               const char* modname = "main");
    void loadMesh(std::string filename,
                  vkMeshLoader::MeshBuffer* meshBuffer,
                  std::vector<vkMeshLoader::VertexLayout> vertexLayout,
                  float scale);

    struct MeshBufferInfo
    {
        vk::Buffer buf;
        vk::DeviceMemory mem;
        vk::DeviceSize size = 0;

        void freeResources(vk::Device& device) {
            if (buf) device.destroyBuffer(buf);
            if (mem) device.freeMemory(mem);
        }
    };

    struct MeshBuffer
    {
        MeshBufferInfo vertices;
        MeshBufferInfo indices;
        uint32_t indexCount;
        glm::vec3 dim;

        void freeResources(vk::Device& device) {
            vertices.freeResources(device);
            indices.freeResources(device);
        }
    };

    struct UniformData
    {
        vk::Buffer buffer;
        vk::DeviceMemory memory;
        vk::DescriptorBufferInfo descriptor;
        uint32_t allocSize;
        void* mapped = nullptr;

        void freeResources(vk::Device& device) {
            device.destroyBuffer(buffer);
            device.freeMemory(memory);
        }
    };

    VulkanContext& vkctx;

    // Saved for clean-up
    std::vector<VkShaderModule> shaderModules;

    const vk::ClearColorValue defaultClearColor;
};

#endif /* VULKAN_LOAD_TEST_SAMPLE_H */

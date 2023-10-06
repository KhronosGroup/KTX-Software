/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @class VulkanLoadTestSample
 * @~English
 *
 * @brief Base class for Vulkan texture loading test samples.
 *
 * @author Mark Callow, www.edgewise-consulting.com.
 */

#include "VulkanLoadTestSample.h"

#include <random>
#include <unordered_map>

#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION 1000000
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable: 4100)
    #pragma warning(disable: 4189)
    #pragma warning(disable: 4324)
#endif

#include "vma/vk_mem_alloc.h"

#if defined(_MSC_VER)
    #pragma warning(pop)
#endif

namespace VMA_CALLBACKS
{
    VmaAllocator vmaAllocator;
    std::mt19937_64 mt64;
    VkPhysicalDeviceMemoryProperties cachedDevMemProps;

    void InitVMA(VkPhysicalDevice& physicalDevice, VkDevice& device, VkInstance& instance, 
                 VkPhysicalDeviceMemoryProperties& devMemProps)
    {
        cachedDevMemProps = devMemProps;

        VmaVulkanFunctions vulkanFunctions = {};
        vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_0;
        allocatorCreateInfo.physicalDevice = physicalDevice;
        allocatorCreateInfo.device = device;
        allocatorCreateInfo.instance = instance;
        allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

        vmaCreateAllocator(&allocatorCreateInfo, &vmaAllocator);
    }

    void DestroyVMA()
    {
        vmaDestroyAllocator(vmaAllocator);
    }

    struct AllocationInfo
    {
        VmaAllocation allocation;
        VkDeviceSize mapSize;
    };
    std::unordered_map<uint64_t, AllocationInfo> AllocMemCWrapperDirectory;
    uint64_t AllocMemCWrapper(VkMemoryAllocateInfo* allocInfo, VkMemoryRequirements* memReq, uint64_t* numPages)
    {
        uint64_t allocId = mt64();
        VmaAllocationCreateInfo pCreateInfo = {};
        if ((cachedDevMemProps.memoryTypes[allocInfo->memoryTypeIndex].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ||
            (cachedDevMemProps.memoryTypes[allocInfo->memoryTypeIndex].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        {
            pCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
            pCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        }
        else
        {
            pCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        }
        pCreateInfo.memoryTypeBits = memReq->memoryTypeBits;

        VmaAllocation allocation;
        VkResult result = vmaAllocateMemory(vmaAllocator, memReq, &pCreateInfo, &allocation, VMA_NULL);
        if (result != VK_SUCCESS)
        {
            return 0ull;
        }

        AllocMemCWrapperDirectory[allocId].allocation = allocation;
        AllocMemCWrapperDirectory[allocId].mapSize = memReq->size;
        *numPages = 1ull;

        return allocId;
    }

    VkResult BindBufferMemoryCWrapper(VkBuffer buffer, uint64_t allocId)
    {
        return vmaBindBufferMemory(vmaAllocator, AllocMemCWrapperDirectory[allocId].allocation, buffer);
    }

    VkResult BindImageMemoryCWrapper(VkImage image, uint64_t allocId)
    {
        return vmaBindImageMemory(vmaAllocator, AllocMemCWrapperDirectory[allocId].allocation, image);
    }

    VkResult MapMemoryCWrapper(uint64_t allocId, uint64_t, VkDeviceSize* mapLength, void** dataPtr)
    {
        *mapLength = AllocMemCWrapperDirectory[allocId].mapSize;
        return vmaMapMemory(vmaAllocator, AllocMemCWrapperDirectory[allocId].allocation, dataPtr);
    }

    void UnmapMemoryCWrapper(uint64_t allocId, uint64_t)
    {
        vmaUnmapMemory(vmaAllocator, AllocMemCWrapperDirectory[allocId].allocation);
    }

    void FreeMemCWrapper(uint64_t allocId)
    {
        vmaFreeMemory(vmaAllocator, AllocMemCWrapperDirectory[allocId].allocation);
        AllocMemCWrapperDirectory.erase(allocId);
    }
}

VulkanLoadTestSample::~VulkanLoadTestSample()
{
    for (auto& shaderModule : shaderModules)
    {
        vkctx.device.destroyShaderModule(shaderModule, nullptr);
    }
}

void
VulkanLoadTestSample::loadMesh(std::string filename,
                               vkMeshLoader::MeshBuffer* meshBuffer,
                               std::vector<vkMeshLoader::VertexLayout> vertexLayout,
                               float scale)
{
    VulkanMeshLoader *mesh = new VulkanMeshLoader();

    if (!mesh->LoadMesh(filename)) {
        std::stringstream message;
        message << "Error reading or parsing mesh file \"" << filename << "\"";
        throw std::runtime_error(message.str());
    }

    assert(mesh->m_Entries.size() > 0);

    vk::CommandBuffer copyCmd =
            vkctx.createCommandBuffer(vk::CommandBufferLevel::ePrimary, false);

    mesh->createBuffers(
        vkctx.device,
        static_cast<VkPhysicalDeviceMemoryProperties>(vkctx.memoryProperties),
        meshBuffer,
        vertexLayout,
        scale,
        true,
        copyCmd,
        vkctx.queue);

    vkctx.device.freeCommandBuffers(vkctx.commandPool, 1, &copyCmd);

    meshBuffer->dim = mesh->dim.size;

    delete(mesh);
}

vk::PipelineShaderStageCreateInfo
VulkanLoadTestSample::loadShader(std::string filename,
                                 vk::ShaderStageFlagBits stage,
                                 const char* modname)
{
    vk::PipelineShaderStageCreateInfo shaderStage = vkctx.loadShader(filename, stage, modname);
    assert(shaderStage.module);
    shaderModules.push_back(shaderStage.module);
    return shaderStage;
}

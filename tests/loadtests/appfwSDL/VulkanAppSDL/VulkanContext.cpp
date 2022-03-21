/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */


/**
 * @internal
 * @class VulkanContext
 * @~English
 *
 * @brief Class for holding and passing Vulkan context info to applications.
 *
 * @author Mark Callow, www.edgewise-consulting.com.
 */

#include <exception>
#include <iomanip>
#include <sstream>
#include <SDL2/SDL_rwops.h>
#include "VulkanContext.h"
// Until exceptions are used everywhere...
#include "vulkancheckres.h"
#include "ltexceptions.h"
#include "unused.h"

vk::CommandBuffer
VulkanContext::createCommandBuffer(vk::CommandBufferLevel level, bool begin)
{
    vk::CommandBuffer cmdBuffer;

    vk::CommandBufferAllocateInfo cmdBufAllocateInfo(
            commandPool,
            level,
            1);

    vk::Result res
      = device.allocateCommandBuffers(&cmdBufAllocateInfo, &cmdBuffer);

    if (res != vk::Result::eSuccess) {
        throw bad_vulkan_alloc((int)res, "device.allocateCommandBuffers");
    }

    // If requested, also start the new command buffer
    if (begin)
    {
        vk::CommandBufferBeginInfo cmdBufInfo;
        res = cmdBuffer.begin(&cmdBufInfo);
        if (res != vk::Result::eSuccess) {
            throw bad_vulkan_alloc((int)res, "cmdBuffer.begin");
        }
    }

    return cmdBuffer;
}


void
VulkanContext::flushCommandBuffer(vk::CommandBuffer& cmdBuffer,
                                  bool free)
{
    if (!cmdBuffer) {
        return;
    }

    cmdBuffer.end();

    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;

    vk::Result res = queue.submit(1, &submitInfo, nullptr);
    if (res != vk::Result::eSuccess) {
        if (res == vk::Result::eErrorDeviceLost) {
            throw std::runtime_error("Vulkan device lost.");
        } else {
            throw bad_vulkan_alloc((int)res, "queue.submit");
        }
    }

    queue.waitIdle();

    if (free) {
        device.freeCommandBuffers(commandPool, 1, &cmdBuffer);
    }
}

bool
VulkanContext::createDrawCommandBuffers()
{
    VkCommandBufferAllocateInfo aInfo;
    aInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    aInfo.pNext = NULL;
    aInfo.commandPool = commandPool;
    aInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    aInfo.commandBufferCount = 1;

    drawCmdBuffers.resize(swapchain.imageCount);
    for (uint32_t i = 0; i < swapchain.imageCount; i++) {
        VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &aInfo,
                        &drawCmdBuffers[i]));
    }
    return true;
}

bool
VulkanContext::createPresentCommandBuffers()
{
    VkCommandBufferAllocateInfo aInfo;
    aInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    aInfo.pNext = NULL;
    aInfo.commandPool = commandPool;
    aInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    aInfo.commandBufferCount = 1;

    prePresentCmdBuffers.resize(swapchain.imageCount);
    postPresentCmdBuffers.resize(swapchain.imageCount);
    for (uint32_t i = 0; i < swapchain.imageCount; i++) {
        VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &aInfo,
                        &prePresentCmdBuffers[i]));
        VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &aInfo,
                        &postPresentCmdBuffers[i]));
    }
    return true;
}

void
VulkanContext::destroyDrawCommandBuffers()
{
    if (drawCmdBuffers.size() > 0) {
        vkFreeCommandBuffers(device, commandPool,
                        static_cast<uint32_t>(drawCmdBuffers.size()),
                        drawCmdBuffers.data());
    }
    for (uint32_t i = 0; i < drawCmdBuffers.size(); ++i) {
        drawCmdBuffers[i] = nullptr;
    }

}

void
VulkanContext::destroyPresentCommandBuffers()
{
    vkFreeCommandBuffers(device, commandPool,
                    static_cast<uint32_t>(drawCmdBuffers.size()),
                    prePresentCmdBuffers.data());
    vkFreeCommandBuffers(device, commandPool,
                    static_cast<uint32_t>(drawCmdBuffers.size()),
                    postPresentCmdBuffers.data());
}

bool
VulkanContext::checkDrawCommandBuffers()
{
    for (auto& cmdBuffer : drawCmdBuffers)
    {
        if (cmdBuffer == VK_NULL_HANDLE)
        {
            return false;
        }
    }
    return true;
}

bool
VulkanContext::createBuffer(vk::BufferUsageFlags usageFlags,
                            vk::MemoryPropertyFlags memoryPropertyFlags,
                            vk::DeviceSize size,
                            void * data,
                            vk::Buffer* buffer,
                            vk::DeviceMemory* memory)
{
    vk::MemoryRequirements memReqs;
    vk::MemoryAllocateInfo memAlloc(0, 0);
    vk::BufferCreateInfo bufferCreateInfo({}, size, usageFlags);

    vk::Result res = device.createBuffer(&bufferCreateInfo, nullptr, buffer);
    if (res != vk::Result::eSuccess) {
        return false;
    }

    device.getBufferMemoryRequirements(*buffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits,
                                             memoryPropertyFlags);
    res = device.allocateMemory(&memAlloc, nullptr, memory);
    if (res == vk::Result::eSuccess) {
        if (data != nullptr)
        {
            void *mapped;
            mapped = device.mapMemory(*memory, 0, size, {});
            memcpy(mapped, data, (size_t)size);
            device.unmapMemory(*memory);
        }
        device.bindBufferMemory(*buffer, *memory, 0);

        return true;
    } else
        return false;
}

bool
VulkanContext::createBuffer(vk::BufferUsageFlags usage,
                            vk::DeviceSize size,
                            void* data,
                            vk::Buffer* buffer,
                            vk::DeviceMemory* memory)
{
    return createBuffer(usage, vk::MemoryPropertyFlagBits::eHostVisible,
                        size, data, buffer, memory);
}

bool
VulkanContext::createBuffer(vk::BufferUsageFlags usage,
                            vk::DeviceSize size,
                            void* data,
                            vk::Buffer* buffer,
                            vk::DeviceMemory* memory,
                            vk::DescriptorBufferInfo* descriptor)
{
    bool res = createBuffer(usage, size, data, buffer, memory);
    if (res)
    {
        descriptor->offset = 0;
        descriptor->buffer = *buffer;
        descriptor->range = size;
        return true;
    }
    else
    {
        return false;
    }
}

bool
VulkanContext::createBuffer(vk::BufferUsageFlags usage,
                            vk::MemoryPropertyFlags memoryPropertyFlags,
                            vk::DeviceSize size,
                            void* data,
                            vk::Buffer* buffer,
                            vk::DeviceMemory* memory,
                            vk::DescriptorBufferInfo* descriptor)
{
    bool res = createBuffer(usage, memoryPropertyFlags, size, data, buffer, memory);
    if (res)
    {
        descriptor->offset = 0;
        descriptor->buffer = *buffer;
        descriptor->range = size;
        return true;
    }
    else
    {
        return false;
    }
}

bool
VulkanContext::getMemoryType(uint32_t typeBits,
                             vk::MemoryPropertyFlags requirementsMask,
                             uint32_t *typeIndex) const
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

uint32_t
VulkanContext::getMemoryType(uint32_t typeBits,
                             vk::MemoryPropertyFlags requirementsMask) const
{
    // Search memtypes to find first index with desired properties
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
        if ((typeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if ((memoryProperties.memoryTypes[i].propertyFlags &
                 requirementsMask) == requirementsMask) {
                return i;
            }
        }
        typeBits >>= 1;
    }
    // No memory types matched, return failure
    // TODO throw error
    return 0;
}

vk::PipelineShaderStageCreateInfo
VulkanContext::loadShader(std::string filename,
                          vk::ShaderStageFlagBits stage,
                          const char* const modname)
{
    vk::PipelineShaderStageCreateInfo shaderStage({}, stage);
    shaderStage.module = loadShader(filename);
    shaderStage.pName = modname;
    return shaderStage;
}

vk::ShaderModule
VulkanContext::loadShader(std::string filename)
{
    size_t codeSize;
    uint32_t* shaderCode;

    shaderCode = readSpv(filename.c_str(), &codeSize);

    vk::ShaderModule shaderModule;
    vk::ShaderModuleCreateInfo moduleCreateInfo({}, codeSize, shaderCode);

    vk::Result res
          = device.createShaderModule(&moduleCreateInfo, NULL, &shaderModule);
    if (res != vk::Result::eSuccess) {
        throw bad_vulkan_alloc((int)res, "device.createShaderModule");
    }

    delete[] shaderCode;

    assert(shaderModule);
    return shaderModule;
}

uint32_t*
VulkanContext::readSpv(const char *filename, size_t *pSize) {
    size_t size;
    U_ASSERT_ONLY size_t retval;
    uint32_t* shader_code;

    SDL_RWops* rw = SDL_RWFromFile(filename, "rb");
    if (!rw) {
        std::stringstream message;

        // String returned by SDL_GetError() includes file name.
        message << "Open of shader failed: " << SDL_GetError();
        throw std::runtime_error(message.str());
    }

    size = (size_t)SDL_RWsize(rw);

    // Round-up to next 4-byte size.
    shader_code = new uint32_t[(size + 3)/4];
    retval = SDL_RWread(rw, shader_code, size, 1);
    assert(retval == 1);

    *pSize = size;

    SDL_RWclose(rw);
    return shader_code;
}


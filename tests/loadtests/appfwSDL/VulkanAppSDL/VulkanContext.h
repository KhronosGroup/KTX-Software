/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

#ifndef VULKAN_TEXTURE_H_229895365400979164311947449304284143508
#define VULKAN_TEXTURE_H_229895365400979164311947449304284143508

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

#include <vector>
#include <vulkan/vulkan.hpp>
#include "VulkanSwapchain.h"

struct VulkanDepthBuffer {
    VkFormat format;
    VkImage image;
    VkMemoryAllocateInfo memAlloc;
    VkDeviceMemory mem;
    VkImageView view;
};

struct VulkanContext {
    vk::Instance instance;
    vk::PhysicalDevice gpu;
    vk::PhysicalDeviceFeatures gpuFeatures;
    vk::PhysicalDeviceProperties gpuProperties;
    vk::PhysicalDeviceMemoryProperties memoryProperties;
    vk::Device device;
    vk::CommandPool commandPool;
    vk::Queue queue;
    std::vector<VkCommandBuffer> drawCmdBuffers;
    std::vector<VkCommandBuffer> postPresentCmdBuffers;
    std::vector<VkCommandBuffer> prePresentCmdBuffers;
    VkRenderPass renderPass;
    // Pipeline stage flags for the submit info structure
    const VkPipelineStageFlags submitPipelineStages =
                                      VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    // Contains command buffers and semaphores to be presented to the queue
    VkSubmitInfo drawCmdSubmitInfo;

    vk::DescriptorPool descriptorPool;
    VkPipelineCache pipelineCache;

    VulkanSwapchain swapchain;
    // List of frame buffers (same as number of swap chain images)
    std::vector<VkFramebuffer>framebuffers;
    VulkanDepthBuffer depthBuffer;

    // Create a new command buffer, opening it for command entry,
    // if requested.
    vk::CommandBuffer createCommandBuffer(vk::CommandBufferLevel level,
                                        bool begin);
    // End a command buffer, submit it to the queue and free, if requested.
    // Note : Waits for the queue to become idle
    void flushCommandBuffer(vk::CommandBuffer& commandBuffer, vk::Queue& queue,
                            bool free);

    // Create a command buffer for each image in the swap chain.
    bool createDrawCommandBuffers();
    bool createPresentCommandBuffers();
    bool checkDrawCommandBuffers();
    void destroyDrawCommandBuffers();
    void destroyPresentCommandBuffers();


    bool getMemoryType(uint32_t typeBits,
                       vk::MemoryPropertyFlags requirementsMask,
                       uint32_t *typeIndex) const;
    uint32_t getMemoryType(uint32_t typeBits,
                           vk::MemoryPropertyFlags requirementsMask) const;

    // Create a buffer, fill it with data (if != NULL) and bind buffer memory
    bool createBuffer(
        vk::BufferUsageFlags usageFlags,
        vk::MemoryPropertyFlags memoryPropertyFlags,
        vk::DeviceSize size,
        void *data,
        vk::Buffer *buffer,
        vk::DeviceMemory *memory);
    // This version always uses HOST_VISIBLE memory
    bool createBuffer(
        vk::BufferUsageFlags usage,
        vk::DeviceSize size,
        void *data,
        vk::Buffer *buffer,
        vk::DeviceMemory *memory);
    // Overload that assigns buffer info to descriptor
    bool createBuffer(
        vk::BufferUsageFlags usage,
        vk::DeviceSize size,
        void* data,
        vk::Buffer* buffer,
        vk::DeviceMemory* memory,
        vk::DescriptorBufferInfo* descriptor);
    // Overload to pass memory property flags
    bool createBuffer(
        vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags memoryPropertyFlags,
        vk::DeviceSize size,
        void* data,
        vk::Buffer* buffer,
        vk::DeviceMemory* memory,
        vk::DescriptorBufferInfo* descriptor);

    vk::PipelineShaderStageCreateInfo loadShader(std::string filename,
                                            vk::ShaderStageFlagBits stage,
                                            const char* const modname = "main");
    vk::ShaderModule loadShader(std::string filename);
    uint32_t* readSpv(const char *filename, size_t *pSize);
};

#endif /* VULKAN_TEXTURE_H_229895365400979164311947449304284143508 */

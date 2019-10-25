/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

#ifndef VULKAN_TEXTURE_H_229895365400979164311947449304284143508
#define VULKAN_TEXTURE_H_229895365400979164311947449304284143508

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

    struct {
       bool pvrtc = false;
       bool astc_hdr = false;
       bool astc_3d = false;
    } enabledDeviceExtensions;
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

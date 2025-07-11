/*
* Copyright 2016 Sascha Willems - www.saschawillems.de
* SPDX-License-Identifier: MIT
*
* Text overlay class for displaying debug information
*
*/

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <sstream>
#include <iomanip>

#include <vulkan/vulkan.h>
#include "vulkantools.h"
#include "vulkandebug.h"

#include "stb/stb_font_consolas_24_latin1.inl"

// Defines for the STB font used
// STB font files can be found at http://nothings.org/stb/font/
#define STB_FONT_NAME stb_font_consolas_24_latin1
#define STB_FONT_WIDTH STB_FONT_consolas_24_latin1_BITMAP_WIDTH
#define STB_FONT_HEIGHT STB_FONT_consolas_24_latin1_BITMAP_HEIGHT 
#define STB_FIRST_CHAR STB_FONT_consolas_24_latin1_FIRST_CHAR
#define STB_NUM_CHARS STB_FONT_consolas_24_latin1_NUM_CHARS
#define STB_MISSING_GLPYH 0x80 // Actually a control character.

// Max. number of chars the text overlay buffer can hold
#define MAX_CHAR_COUNT 1024


/**
 * @internal
 * @brief Given the lead byte of a UTF-8 sequence returns the expected length of the codepoint
 * @param[in] leadByte The lead byte of a UTF-8 sequence
 * @return The expected length of the codepoint */
[[nodiscard]] constexpr inline int sequenceLength(uint8_t leadByte) noexcept {
    if ((leadByte & 0b1000'0000u) == 0b0000'0000u)
        return 1;
    if ((leadByte & 0b1110'0000u) == 0b1100'0000u)
        return 2;
    if ((leadByte & 0b1111'0000u) == 0b1110'0000u)
        return 3;
    if ((leadByte & 0b1111'1000u) == 0b1111'0000u)
        return 4;

    return 0;
}

/**
 * @internal
 * @brief Checks if the codepoint was coded as a longer than required sequence
 * @param[in] codepoint The unicode codepoint
 * @param[in] length The UTF-8 sequence length
 * @return True if the sequence length was inappropriate for the given codepoint */
[[nodiscard]] constexpr inline bool isOverlongSequence(uint32_t codepoint, int length) noexcept {
    if (codepoint < 0x80)
        return length != 1;
    else if (codepoint < 0x800)
        return length != 2;
    else if (codepoint < 0x10000)
        return length != 3;
    else
        return false;
}

/**
 * @internal
 * @brief Checks if the codepoint is valid
 * @param[in] codepoint The unicode codepoint
 * @return True if the codepoint is a valid unicode codepoint */
[[nodiscard]] constexpr inline bool isCodepointValid(uint32_t codepoint) noexcept {
    return codepoint <= 0x0010FFFFu
            && !(0xD800u <= codepoint && codepoint <= 0xDBFFu);
}

/**
 * @internal
 * @brief Safely checks and advances a UTF-8 sequence iterator to the start of the next unicode codepoint
 * @param[in] it iterator to be advanced
 * @param[in] end iterator pointing to the end of the range
 * @return True if the advance operation was successful and the advanced codepoint was a valid UTF-8 sequence */
template <typename Iterator>
[[nodiscard]] constexpr bool advanceUTF8(Iterator& it, Iterator end,
                                         uint32_t& codepoint) noexcept {
    if (it == end)
        return false;

    const auto length = sequenceLength(*it);
    if (length == 0)
        return false;

    if (std::distance(it, end) < length)
        return false;

    for (int i = 1; i < length; ++i) {
        const auto trailByte = *(it + i);
        if ((static_cast<uint8_t>(trailByte) & 0b1100'0000u) != 0b1000'0000u)
            return false;
    }

    codepoint = 0;
    switch (length) {
    case 1:
        codepoint |= *it++;
        break;
    case 2:
        codepoint |= (*it++ & 0b0001'1111u) << 6u;
        codepoint |= (*it++ & 0b0011'1111u);
        break;
    case 3:
        codepoint |= (*it++ & 0b0000'1111u) << 12u;
        codepoint |= (*it++ & 0b0011'1111u) << 6u;
        codepoint |= (*it++ & 0b0011'1111u);
        break;
    case 4:
        codepoint |= (*it++ & 0b0000'0111u) << 18u;
        codepoint |= (*it++ & 0b0011'1111u) << 12u;
        codepoint |= (*it++ & 0b0011'1111u) << 6u;
        codepoint |= (*it++ & 0b0011'1111u);
        break;
    }

    if (!isCodepointValid(codepoint))
        return false;

    if (isOverlongSequence(codepoint, length))
        return false;

    return true;
}

// Mostly self-contained text overlay class
// todo : comment
class VulkanTextOverlay
{
private:
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
    VkQueue queue;
    VkFormat colorFormat;
    VkFormat depthFormat;

    uint32_t *frameBufferWidth;
    uint32_t *frameBufferHeight;

    VkSampler sampler;
    VkImage image;
    VkImageView view;
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDeviceMemory imageMemory;
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;
    VkPipelineLayout pipelineLayout;
    VkPipelineCache pipelineCache;
    VkPipeline pipeline;
    VkRenderPass renderPass;
    VkCommandPool commandPool;
    std::vector<VkFramebuffer*> frameBuffers;
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    // Pointer to mapped vertex buffer
    glm::vec4 *mapped = nullptr;
    // Used during text updates
    glm::vec4 *mappedLocal = nullptr;

    stb_fontchar stbFontData[STB_NUM_CHARS];
    uint32_t numLetters;

    // Try to find appropriate memory type for a memory allocation
    uint32_t getMemoryType(uint32_t typeBits, VkFlags properties)
    {
        for (uint32_t i = 0; i < 32; i++)
        {
            if ((typeBits & 1) == 1)
            {
                if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
                {
                    return i;
                }
            }
            typeBits >>= 1;
        }

        // todo : throw error
        return 0;
    }

public:

    enum TextAlign { alignLeft, alignCenter, alignRight };

    bool visible = true;
    bool invalidated = false;

    std::vector<VkCommandBuffer> cmdBuffers;

    VulkanTextOverlay(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkQueue queue,
        std::vector<VkFramebuffer> &framebuffers,
        VkFormat colorformat,
        VkFormat depthformat,
        uint32_t *framebufferwidth,
        uint32_t *framebufferheight,
        std::vector<VkPipelineShaderStageCreateInfo> shaderstages)
    {
        this->physicalDevice = physicalDevice;
        this->device = device;
        this->queue = queue;
        this->colorFormat = colorformat;
        this->depthFormat = depthformat;

        this->frameBuffers.resize(framebuffers.size());
        for (uint32_t i = 0; i < framebuffers.size(); i++)
        {
            this->frameBuffers[i] = &framebuffers[i];
        }

        this->shaderStages = shaderstages;

        this->frameBufferWidth = framebufferwidth;
        this->frameBufferHeight = framebufferheight;

        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);
        cmdBuffers.resize(framebuffers.size());
        prepareResources();
        prepareRenderPass();
        preparePipeline();
    }

    ~VulkanTextOverlay()
    {
        // Free up all Vulkan resources requested by the text overlay
        vkDestroySampler(device, sampler, nullptr);
        vkDestroyImage(device, image, nullptr);
        vkDestroyImageView(device, view, nullptr);
        vkDestroyBuffer(device, buffer, nullptr);
        vkFreeMemory(device, memory, nullptr);
        vkFreeMemory(device, imageMemory, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyPipelineCache(device, pipelineCache, nullptr);
        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);
        vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(cmdBuffers.size()), cmdBuffers.data());
        vkDestroyCommandPool(device, commandPool, nullptr);
    }

    // Prepare all vulkan resources required to render the font
    // The text overlay uses separate resources for descriptors (pool, sets, layouts), pipelines and command buffers
    void prepareResources()
    {
        static unsigned char font24pixels[STB_FONT_HEIGHT][STB_FONT_WIDTH];
        STB_FONT_NAME(stbFontData, font24pixels, STB_FONT_HEIGHT);

        // Command buffer

        // Pool
        VkCommandPoolCreateInfo cmdPoolInfo = {};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.queueFamilyIndex = 0; // todo : pass from example base / swap chain
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &commandPool));

        VkCommandBufferAllocateInfo cmdBufAllocateInfo =
            vkTools::initializers::commandBufferAllocateInfo(
                commandPool,
                VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                (uint32_t)cmdBuffers.size());

        VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, cmdBuffers.data()));

        // Vertex buffer, 4 per character.
        VkDeviceSize bufferSize = MAX_CHAR_COUNT * sizeof(glm::vec4) * 4;

        VkBufferCreateInfo bufferInfo = vkTools::initializers::bufferCreateInfo(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize);
        VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));

        VkMemoryRequirements memReqs;
        VkMemoryAllocateInfo allocInfo = vkTools::initializers::memoryAllocateInfo();

        vkGetBufferMemoryRequirements(device, buffer, &memReqs);
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &memory));
        VK_CHECK_RESULT(vkBindBufferMemory(device, buffer, memory, 0));

        // Map persistent
        VK_CHECK_RESULT(vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, (void **)&mapped));


        // Font texture
        VkImageCreateInfo imageInfo = vkTools::initializers::imageCreateInfo();
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_R8_UNORM;
        imageInfo.extent.width = STB_FONT_WIDTH;
        imageInfo.extent.height = STB_FONT_HEIGHT;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        VK_CHECK_RESULT(vkCreateImage(device, &imageInfo, nullptr, &image));

        vkGetImageMemoryRequirements(device, image, &memReqs);
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory));
        VK_CHECK_RESULT(vkBindImageMemory(device, image, imageMemory, 0));

        // Staging

        struct {
            VkDeviceMemory memory;
            VkBuffer buffer;
        } stagingBuffer;

        VkBufferCreateInfo bufferCreateInfo = vkTools::initializers::bufferCreateInfo();
        bufferCreateInfo.size = allocInfo.allocationSize;
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &stagingBuffer.buffer));

        // Get memory requirements for the staging buffer (alignment, memory type bits)
        vkGetBufferMemoryRequirements(device, stagingBuffer.buffer, &memReqs);

        allocInfo.allocationSize = memReqs.size;
        // Get memory type index for a host visible buffer
        allocInfo.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

        VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &stagingBuffer.memory));
        VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffer.buffer, stagingBuffer.memory, 0));

        uint8_t *data;
        VK_CHECK_RESULT(vkMapMemory(device, stagingBuffer.memory, 0, allocInfo.allocationSize, 0, (void **)&data));
        memcpy(data, &font24pixels[0][0], STB_FONT_WIDTH * STB_FONT_HEIGHT);
        vkUnmapMemory(device, stagingBuffer.memory);

        // Copy to image

        VkCommandBuffer copyCmd;
        cmdBufAllocateInfo.commandBufferCount = 1;
        VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &copyCmd));

        VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();
        VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));

        // Prepare for transfer
        vkTools::setImageLayout(
            copyCmd,
            image,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_PREINITIALIZED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy bufferCopyRegion = {};
        bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion.imageSubresource.mipLevel = 0;
        bufferCopyRegion.imageSubresource.layerCount = 1;
        bufferCopyRegion.imageExtent.width = STB_FONT_WIDTH;
        bufferCopyRegion.imageExtent.height = STB_FONT_HEIGHT;
        bufferCopyRegion.imageExtent.depth = 1;

        vkCmdCopyBufferToImage(
            copyCmd,
            stagingBuffer.buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &bufferCopyRegion
            );

        // Prepare for shader read
        vkTools::setImageLayout(
            copyCmd,
            image,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

        VkSubmitInfo submitInfo = vkTools::initializers::submitInfo();
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &copyCmd;

        VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
        VK_CHECK_RESULT(vkQueueWaitIdle(queue));

        vkFreeCommandBuffers(device, commandPool, 1, &copyCmd);
        vkFreeMemory(device, stagingBuffer.memory, nullptr);
        vkDestroyBuffer(device, stagingBuffer.buffer, nullptr);


        VkImageViewCreateInfo imageViewInfo = vkTools::initializers::imageViewCreateInfo();
        imageViewInfo.image = image;
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewInfo.format = imageInfo.format;
        imageViewInfo.components = {
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY
        };
        imageViewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        VK_CHECK_RESULT(vkCreateImageView(device, &imageViewInfo, nullptr, &view));

        // Sampler
        VkSamplerCreateInfo samplerInfo = vkTools::initializers::samplerCreateInfo();
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0;
        VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, &sampler));

        // Descriptor
        // Font uses a separate descriptor pool
        std::array<VkDescriptorPoolSize, 1> poolSizes;
        poolSizes[0] = vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1);

        VkDescriptorPoolCreateInfo descriptorPoolInfo =
            vkTools::initializers::descriptorPoolCreateInfo(
                static_cast<uint32_t>(poolSizes.size()),
                poolSizes.data(),
                1);

        VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

        // Descriptor set layout
        std::array<VkDescriptorSetLayoutBinding, 1> setLayoutBindings;
        setLayoutBindings[0] = vkTools::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo =
            vkTools::initializers::descriptorSetLayoutCreateInfo(
                setLayoutBindings.data(),
                static_cast<uint32_t>(setLayoutBindings.size()));

        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout));

        // Pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo =
            vkTools::initializers::pipelineLayoutCreateInfo(
                &descriptorSetLayout,
                1);

        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

        // Descriptor set
        VkDescriptorSetAllocateInfo descriptorSetAllocInfo =
            vkTools::initializers::descriptorSetAllocateInfo(
                descriptorPool,
                &descriptorSetLayout,
                1);

        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &descriptorSet));

        VkDescriptorImageInfo texDescriptor =
            vkTools::initializers::descriptorImageInfo(
                sampler,
                view,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        std::array<VkWriteDescriptorSet, 1> writeDescriptorSets;
        writeDescriptorSets[0] = vkTools::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &texDescriptor);
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);

        // Pipeline cache
        VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
        pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache));
    }

    // Prepare a separate pipeline for the font rendering decoupled from the main application
    void preparePipeline()
    {
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
            vkTools::initializers::pipelineInputAssemblyStateCreateInfo(
                VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
                0,
                // primmitiveRestartEnable not needed but disabling it results in a MoltenVK
                // feature not present warning.
                VK_TRUE);

        VkPipelineRasterizationStateCreateInfo rasterizationState =
            vkTools::initializers::pipelineRasterizationStateCreateInfo(
                VK_POLYGON_MODE_FILL,
                VK_CULL_MODE_BACK_BIT,
                VK_FRONT_FACE_CLOCKWISE,
                0);
        // Because we haven't enabled the depthClamp device feature.
        rasterizationState.depthClampEnable = VK_FALSE;

        // Enable blending
        VkPipelineColorBlendAttachmentState blendAttachmentState =
            vkTools::initializers::pipelineColorBlendAttachmentState(0xf, VK_TRUE);

        blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
        blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
        blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo colorBlendState =
            vkTools::initializers::pipelineColorBlendStateCreateInfo(
                1,
                &blendAttachmentState);

        VkPipelineDepthStencilStateCreateInfo depthStencilState =
            vkTools::initializers::pipelineDepthStencilStateCreateInfo(
                VK_FALSE,
                VK_FALSE,
                VK_COMPARE_OP_LESS_OR_EQUAL);

        VkPipelineViewportStateCreateInfo viewportState =
            vkTools::initializers::pipelineViewportStateCreateInfo(1, 1, 0);

        VkPipelineMultisampleStateCreateInfo multisampleState =
            vkTools::initializers::pipelineMultisampleStateCreateInfo(
                VK_SAMPLE_COUNT_1_BIT,
                0);

        std::vector<VkDynamicState> dynamicStateEnables = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState =
            vkTools::initializers::pipelineDynamicStateCreateInfo(
                dynamicStateEnables.data(),
                static_cast<uint32_t>(dynamicStateEnables.size()),
                0);

        std::array<VkVertexInputBindingDescription, 2> vertexBindings = {};
        vertexBindings[0] = vkTools::initializers::vertexInputBindingDescription(0, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX);
        vertexBindings[1] = vkTools::initializers::vertexInputBindingDescription(1, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX);

        std::array<VkVertexInputAttributeDescription, 2> vertexAttribs = {};
        // Position
        vertexAttribs[0] = vkTools::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, 0);
        // UV
        vertexAttribs[1] = vkTools::initializers::vertexInputAttributeDescription(1, 1, VK_FORMAT_R32G32_SFLOAT, sizeof(glm::vec2));

        VkPipelineVertexInputStateCreateInfo inputState = vkTools::initializers::pipelineVertexInputStateCreateInfo();
        inputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindings.size());
        inputState.pVertexBindingDescriptions = vertexBindings.data();
        inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttribs.size());
        inputState.pVertexAttributeDescriptions = vertexAttribs.data();

        VkGraphicsPipelineCreateInfo pipelineCreateInfo =
            vkTools::initializers::pipelineCreateInfo(
                pipelineLayout,
                renderPass,
                0);

        pipelineCreateInfo.pVertexInputState = &inputState;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
        pipelineCreateInfo.pRasterizationState = &rasterizationState;
        pipelineCreateInfo.pColorBlendState = &colorBlendState;
        pipelineCreateInfo.pMultisampleState = &multisampleState;
        pipelineCreateInfo.pViewportState = &viewportState;
        pipelineCreateInfo.pDepthStencilState = &depthStencilState;
        pipelineCreateInfo.pDynamicState = &dynamicState;
        pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineCreateInfo.pStages = shaderStages.data();

        VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
    }

    // Prepare a separate render pass for rendering the text as an overlay
    void prepareRenderPass()
    {
        VkAttachmentDescription attachments[2] = {};

        // Color attachment
        attachments[0].format = colorFormat;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        // Don't clear the framebuffer (like the renderpass from the example does)
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // Depth attachment
        attachments[1].format = depthFormat;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorReference = {};
        colorReference.attachment = 0;
        colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthReference = {};
        depthReference.attachment = 1;
        depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.flags = 0;
        subpass.inputAttachmentCount = 0;
        subpass.pInputAttachments = NULL;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorReference;
        subpass.pResolveAttachments = NULL;
        subpass.pDepthStencilAttachment = &depthReference;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments = NULL;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.pNext = NULL;
        renderPassInfo.attachmentCount = 2;
        renderPassInfo.pAttachments = attachments;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 0;
        renderPassInfo.pDependencies = NULL;

        VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
    }

    // Map buffer 
    void beginTextUpdate()
    {
        mappedLocal = mapped;
        numLetters = 0;
    }

    // Add text to the current buffer
    // todo : drop shadow? color attribute?
    void addText(std::string text, float x, float y, TextAlign align)
    {
        if (numLetters == MAX_CHAR_COUNT)
            return;

        assert(mapped != nullptr);

        const float charW = 1.5f / *frameBufferWidth;
        const float charH = 1.5f / *frameBufferHeight;

        float fbW = (float)*frameBufferWidth;
        float fbH = (float)*frameBufferHeight;
        x = (x / fbW * 2.0f) - 1.0f;
        y = (y / fbH * 2.0f) - 1.0f;

        // Calculate text width
        float textWidth = 0;
        uint32_t codepoint;
        auto it = text.begin();
        while (it != text.end())
        {
            if (!advanceUTF8(it, text.end(), codepoint))
                break;
            // TODO: Get a UTF8 font. Consider changing to Dear ImGUI
            // https://github.com/ocornut/imgui.
            // Placeholder to avoid crashing.
            if (codepoint > STB_NUM_CHARS + STB_FIRST_CHAR)
                codepoint = STB_MISSING_GLPYH;
            stb_fontchar *charData = &stbFontData[(uint32_t)codepoint - STB_FIRST_CHAR];
            textWidth += charData->advance * charW;
        }
        switch (align)
        {
        case alignRight:
            x -= textWidth;
            break;
        case alignCenter:
            x -= textWidth / 2.0f;
            break;
        case alignLeft:
            break;
        }

        // Generate a uv mapped quad per char in the new text
        it = text.begin();
        while (it != text.end())
        {
            if (!advanceUTF8(it, text.end(), codepoint))
                break;
            if (codepoint > STB_NUM_CHARS + STB_FIRST_CHAR)
                codepoint = STB_MISSING_GLPYH;

            stb_fontchar *charData = &stbFontData[(uint32_t)codepoint - STB_FIRST_CHAR];

            mappedLocal->x = (x + (float)charData->x0 * charW);
            mappedLocal->y = (y + (float)charData->y0 * charH);
            mappedLocal->z = charData->s0;
            mappedLocal->w = charData->t0;
            mappedLocal++;

            mappedLocal->x = (x + (float)charData->x1 * charW);
            mappedLocal->y = (y + (float)charData->y0 * charH);
            mappedLocal->z = charData->s1;
            mappedLocal->w = charData->t0;
            mappedLocal++;

            mappedLocal->x = (x + (float)charData->x0 * charW);
            mappedLocal->y = (y + (float)charData->y1 * charH);
            mappedLocal->z = charData->s0;
            mappedLocal->w = charData->t1;
            mappedLocal++;

            mappedLocal->x = (x + (float)charData->x1 * charW);
            mappedLocal->y = (y + (float)charData->y1 * charH);
            mappedLocal->z = charData->s1;
            mappedLocal->w = charData->t1;
            mappedLocal++;

            x += charData->advance * charW;

            numLetters++;

            if (numLetters == MAX_CHAR_COUNT)
                break; // Truncate the text.
        }
    }
    // Unmap buffer and update command buffers
    void endTextUpdate()
    {
        updateCommandBuffers();
    }

    // Needs to be called by the application
    void updateCommandBuffers()
    {
        VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

        VkRenderPassBeginInfo renderPassBeginInfo = vkTools::initializers::renderPassBeginInfo();
        renderPassBeginInfo.renderPass = renderPass;
        renderPassBeginInfo.renderArea.extent.width = *frameBufferWidth;
        renderPassBeginInfo.renderArea.extent.height = *frameBufferHeight;
        renderPassBeginInfo.clearValueCount = 0;
        renderPassBeginInfo.pClearValues = nullptr;

        for (uint32_t i = 0; i < cmdBuffers.size(); ++i)
        {
            renderPassBeginInfo.framebuffer = *frameBuffers[i];

            VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffers[i], &cmdBufInfo));

            if (vkDebug::DebugMarker::active)
            {
                vkDebug::DebugMarker::beginRegion(cmdBuffers[i], "Text overlay", glm::vec4(1.0f, 0.94f, 0.3f, 1.0f));
            }

            vkCmdBeginRenderPass(cmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            VkViewport viewport = vkTools::initializers::viewport((float)*frameBufferWidth, (float)*frameBufferHeight, 0.0f, 1.0f);
            vkCmdSetViewport(cmdBuffers[i], 0, 1, &viewport);

            VkRect2D scissor = vkTools::initializers::rect2D(*frameBufferWidth, *frameBufferHeight, 0, 0);
            vkCmdSetScissor(cmdBuffers[i], 0, 1, &scissor);
            
            vkCmdBindPipeline(cmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            vkCmdBindDescriptorSets(cmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

            VkDeviceSize offsets = 0;
            vkCmdBindVertexBuffers(cmdBuffers[i], 0, 1, &buffer, &offsets);
            vkCmdBindVertexBuffers(cmdBuffers[i], 1, 1, &buffer, &offsets);
            for (uint32_t j = 0; j < numLetters; j++)
            {
                vkCmdDraw(cmdBuffers[i], 4, 1, j * 4, 0);
            }

            vkCmdEndRenderPass(cmdBuffers[i]);

            if (vkDebug::DebugMarker::active)
            {
                vkDebug::DebugMarker::endRegion(cmdBuffers[i]);
            }

            VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffers[i]));
        }
    }

    // Submit the text command buffers to a queue
    void submit(VkQueue targetQueue, uint32_t bufferindex, VkSubmitInfo submitInfo)
    {
        if (!visible)
        {
            return;
        }

        submitInfo.pCommandBuffers = &cmdBuffers[bufferindex];
        submitInfo.commandBufferCount = 1;

        VK_CHECK_RESULT(vkQueueSubmit(targetQueue, 1, &submitInfo, VK_NULL_HANDLE));
    }

    void reallocateCommandBuffers()
    {
        vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(cmdBuffers.size()), cmdBuffers.data());

        VkCommandBufferAllocateInfo cmdBufAllocateInfo =
            vkTools::initializers::commandBufferAllocateInfo(
                commandPool,
                VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                static_cast<uint32_t>(cmdBuffers.size()));

        VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, cmdBuffers.data()));
    }

};

// vi: set sw=2 ts=4 expandtab: 

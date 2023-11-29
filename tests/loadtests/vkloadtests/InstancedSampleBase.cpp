/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @class InstancedSampleBase
 * @~English
 *
 * @brief Base for tests that need instanced drawing of textured quads.
 *
 * @author Mark Callow, www.edgewise-consulting.com.
 *
 * @par Acknowledgement
 * Thanks to Sascha Willems' - www.saschawillems.de - for the concept,
 * the VulkanTextOverlay class and the shaders used by this test.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <algorithm>
#include <time.h> 
#include <vector>

#if defined(_WIN32)
#define NOMINMAX    // Prevent windows.h min max defines from causing trouble.
#endif
#include "argparser.h"
#include "InstancedSampleBase.h"
#include "ltexceptions.h"
#include "VulkanTextureTranscoder.hpp"

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION false

// Vertex layout for this example
struct TAVertex {
    float pos[3];
    float uv[2];
};

InstancedSampleBase::InstancedSampleBase(VulkanContext& vkctx,
                 uint32_t width, uint32_t height,
                 const char* const szArgs, const std::string sBasePath)
        : VulkanLoadTestSample(vkctx, width, height, sBasePath)
{
    zoom = -15.0f;
    rotationSpeed = 0.25f;
    rotation = { -15.0f, 35.0f, 0.0f };
    tiling = vk::ImageTiling::eOptimal;
    uboVS.instance = nullptr;
    transcoded = false;

    ktxVulkanDeviceInfo vdi;
    ktxVulkanDeviceInfo_Construct(&vdi, vkctx.gpu, vkctx.device,
                                  vkctx.queue, vkctx.commandPool, nullptr);

    processArgs(szArgs);

    KTX_error_code ktxresult;
    ktxTexture* kTexture;
    std::string ktxfilepath = externalFile ? ktxfilename
                                           : getAssetPath() + ktxfilename;
    ktxresult = ktxTexture_CreateFromNamedFile(ktxfilepath.c_str(),
                                               KTX_TEXTURE_CREATE_NO_FLAGS,
                                               &kTexture);
    if (KTX_SUCCESS != ktxresult) {
        std::stringstream message;
        
        message << "Creation of ktxTexture from \"" << ktxfilepath
                << "\" failed: " << ktxErrorString(ktxresult);
        throw std::runtime_error(message.str());
    }

    if (ktxTexture_NeedsTranscoding(kTexture)) {
        TextureTranscoder tc(vkctx);
        tc.transcode((ktxTexture2*)kTexture);
        transcoded = true;
    }

    vk::Format vkFormat
                = static_cast<vk::Format>(ktxTexture_GetVkFormat(kTexture));
    transcodedFormat = vkFormat;
    vk::ImageType imageType;
    vk::ImageFormatProperties imageFormatProperties;
    vk::ImageCreateFlags createFlags;
    vk::ImageUsageFlags usageFlags = vk::ImageUsageFlagBits::eSampled;
    uint32_t numLevels;
    switch (kTexture->numDimensions) {
      case 1:
        imageType = vk::ImageType::e1D;
        break;
      case 2:
      default: // To keep compilers happy.
        imageType = vk::ImageType::e2D;
        break;
      case 3:
        if (kTexture->isArray) {
            std::stringstream message;

            message << "Texture in \"" << getAssetPath() << szArgs
            << "\" is a 3D array texture which are not supported by Vulkan.";
            ktxTexture_Destroy(kTexture);
            throw std::runtime_error(message.str());
        }
        imageType = vk::ImageType::e3D;
        break;
    }
    if (tiling == vk::ImageTiling::eOptimal) {
        // Ensure we can copy from staging buffer to image.
        usageFlags |= vk::ImageUsageFlagBits::eTransferDst;
    }
    if (kTexture->generateMipmaps) {
        // Ensure we can blit between levels.
        usageFlags |= (vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc);
    }
    vk::Result res
        = vkctx.gpu.getImageFormatProperties(vkFormat, imageType, tiling,
                                             usageFlags, createFlags,
                                             &imageFormatProperties);
    if (res != vk::Result::eSuccess) {
        if (res == vk::Result::eErrorFormatNotSupported) {
            throw unsupported_ttype();
        } else {
            throw bad_vulkan_alloc((int)res, "device.getImageFormatProperties");
        }
    }
    numLevels = kTexture->numLevels;
    if (kTexture->generateMipmaps) {
        uint32_t max_dim = std::max(std::max(kTexture->baseWidth, kTexture->baseHeight), kTexture->baseDepth);
        numLevels = (uint32_t)floor(log2(max_dim)) + 1;
    }
    if (numLevels > imageFormatProperties.maxMipLevels) {
        ktxTexture_Destroy(kTexture);
        throw unsupported_ttype();
    }
    if (kTexture->isArray
        && kTexture->numLevels > imageFormatProperties.maxArrayLayers)
    {
        ktxTexture_Destroy(kTexture);
        throw unsupported_ttype();
    }

    vk::FormatProperties properties;
    vkctx.gpu.getFormatProperties(vkFormat, &properties);
    vk::FormatFeatureFlags features =  tiling == vk::ImageTiling::eLinear ?
                                        properties.linearTilingFeatures :
                                        properties.optimalTilingFeatures;
    vk::FormatFeatureFlags neededFeatures =
             vk::FormatFeatureFlagBits::eSampledImage;
    if (kTexture->numLevels > 1) {
        neededFeatures |=
                vk::FormatFeatureFlagBits::eSampledImageFilterLinear;
    }
    if (kTexture->generateMipmaps) {
		neededFeatures |=  vk::FormatFeatureFlagBits::eBlitDst
			    | vk::FormatFeatureFlagBits::eBlitSrc
		       | vk::FormatFeatureFlagBits::eSampledImageFilterLinear;
    }

    if ((features & neededFeatures) != neededFeatures) {
        ktxTexture_Destroy(kTexture);
        throw unsupported_ttype();
    }

    if (features & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)
        filter = vk::Filter::eLinear;
    else
        filter = vk::Filter::eNearest;

    ktxresult =
      ktxTexture_VkUploadEx(kTexture, &vdi, &texture,
                            static_cast<VkImageTiling>(tiling),
                            VK_IMAGE_USAGE_SAMPLED_BIT,
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    if (KTX_SUCCESS != ktxresult) {
        std::stringstream message;
        
        message << "ktxTexture_VkUpload failed: " << ktxErrorString(ktxresult);
        throw std::runtime_error(message.str());
    }
    
    // Checking if KVData contains keys of interest would go here.
    
    ktxTexture_Destroy(kTexture);
    ktxVulkanDeviceInfo_Destruct(&vdi);
}

InstancedSampleBase::~InstancedSampleBase()
{
    cleanup();
}

void
InstancedSampleBase::resize(uint32_t width, uint32_t height)
{
    this->w_width = width;
    this->w_height = height;
    vkctx.destroyDrawCommandBuffers();
    vkctx.createDrawCommandBuffers();
    buildCommandBuffers();
    updateUniformBufferMatrices();
}

void
InstancedSampleBase::run(uint32_t /*msTicks*/)
{
    // Nothing to do since the scene is not animated.
    // VulkanLoadTests base class redraws from the command buffer we built.
}

//===================================================================

void
InstancedSampleBase::processArgs(std::string sArgs)
{
    // Options descriptor
    struct argparser::option longopts[] = {
      {"external",      argparser::option::no_argument, &externalFile, 1},
      {"linear-tiling", argparser::option::no_argument, (int*)&tiling, (int)vk::ImageTiling::eLinear},
      {NULL,            argparser::option::no_argument, NULL,          0}
    };

    argvector argv(sArgs);
    argparser ap(argv);

    int ch;
    while ((ch = ap.getopt(nullptr, longopts, nullptr)) != -1) {
        switch (ch) {
            case 0: break;
            default: assert(false); // Error in args in sample table.
        }
    }
    assert(ap.optind < argv.size());
    ktxfilename = argv[ap.optind];
}

/* ------------------------------------------------------------------------- */

void
InstancedSampleBase::cleanup()
{
    // Clean up used Vulkan resources

    // Clean up texture resources
    if (sampler)
        vkctx.device.destroySampler(sampler);
    if (imageView)
        vkctx.device.destroyImageView(imageView);
    ktxVulkanTexture_Destruct(&texture, vkctx.device, nullptr);

    if (pipelines.solid)
        vkctx.device.destroyPipeline(pipelines.solid);
    if (pipelineLayout)
        vkctx.device.destroyPipelineLayout(pipelineLayout);
    if (descriptorSetLayout)
        vkctx.device.destroyDescriptorSetLayout(descriptorSetLayout);

    vkctx.destroyDrawCommandBuffers();
    quad.freeResources(vkctx.device);
    uniformDataVS.freeResources(vkctx.device);

    delete[] uboVS.instance;
}

void
InstancedSampleBase::buildCommandBuffers()
{
    vk::CommandBufferBeginInfo cmdBufInfo({}, nullptr);

    vk::ClearValue clearValues[2];
    clearValues[0].color = defaultClearColor;
    clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

    vk::RenderPassBeginInfo renderPassBeginInfo(vkctx.renderPass,
            nullptr,
            {{0, 0}, {w_width, w_height}},
            2,
            clearValues);

    for (uint32_t i = 0; i < vkctx.drawCmdBuffers.size(); ++i)
    {
        // Set target frame buffer
        renderPassBeginInfo.framebuffer = vkctx.framebuffers[i];

        VK_CHECK_RESULT(vkBeginCommandBuffer(vkctx.drawCmdBuffers[i],
                &static_cast<const VkCommandBufferBeginInfo&>(cmdBufInfo)));

        vkCmdBeginRenderPass(vkctx.drawCmdBuffers[i],
                &static_cast<const VkRenderPassBeginInfo&>(renderPassBeginInfo),
                VK_SUBPASS_CONTENTS_INLINE);

        vk::Viewport viewport(0, 0,
                              (float)w_width, (float)w_height,
                              0.0f, 1.0f);
        vkCmdSetViewport(vkctx.drawCmdBuffers[i], 0, 1,
                &static_cast<const VkViewport&>(viewport));

        vk::Rect2D scissor({0, 0}, {w_width, w_height});
        vkCmdSetScissor(vkctx.drawCmdBuffers[i], 0, 1,
                &static_cast<const VkRect2D&>(scissor));

        setSubclassPushConstants(i);
        vkCmdBindDescriptorSets(vkctx.drawCmdBuffers[i],
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipelineLayout, 0, 1,
                        &static_cast<const VkDescriptorSet&>(descriptorSet),
                        0, NULL);

        VkDeviceSize offsets[1] = { 0 };
        vkCmdBindVertexBuffers(vkctx.drawCmdBuffers[i],
                        VERTEX_BUFFER_BIND_ID, 1,
                        &static_cast<const VkBuffer&>(quad.vertices.buf),
                        offsets);
        vkCmdBindIndexBuffer(vkctx.drawCmdBuffers[i], quad.indices.buf,
                             0, VK_INDEX_TYPE_UINT32);
        vkCmdBindPipeline(vkctx.drawCmdBuffers[i],
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipelines.solid);

        vkCmdDrawIndexed(vkctx.drawCmdBuffers[i], quad.indexCount,
                         instanceCount, 0, 0, 0);

        vkCmdEndRenderPass(vkctx.drawCmdBuffers[i]);

        VK_CHECK_RESULT(vkEndCommandBuffer(vkctx.drawCmdBuffers[i]));
    }
}

// Setup vertices for a single uv-mapped quad
void
InstancedSampleBase::generateQuad()
{
#define dim 2.5f
    std::vector<TAVertex> vertexBuffer =
    {
        { {  dim,  dim, 0.0f }, { 1.0f, 1.0f } },
        { { -dim,  dim, 0.0f }, { 0.0f, 1.0f } },
        { { -dim, -dim, 0.0f }, { 0.0f, 0.0f } },
        { {  dim, -dim, 0.0f }, { 1.0f, 0.0f } }
    };
#undef dim

    vkctx.createBuffer(
        vk::BufferUsageFlagBits::eVertexBuffer,
        vertexBuffer.size() * sizeof(TAVertex),
        vertexBuffer.data(),
        &quad.vertices.buf,
        &quad.vertices.mem);

    // Setup indices
    std::vector<uint32_t> indexBuffer = { 0,1,2, 2,3,0 };
    quad.indexCount = static_cast<uint32_t>(indexBuffer.size());

    vkctx.createBuffer(
        vk::BufferUsageFlagBits::eIndexBuffer,
        indexBuffer.size() * sizeof(uint32_t),
        indexBuffer.data(),
        &quad.indices.buf,
        &quad.indices.mem);
}

void
InstancedSampleBase::setupVertexDescriptions()
{
    // Binding description
    vertices.bindingDescriptions.resize(1);
    vertices.bindingDescriptions[0] =
        vk::VertexInputBindingDescription(
            VERTEX_BUFFER_BIND_ID,
            sizeof(TAVertex),
            vk::VertexInputRate::eVertex);

    // Attribute descriptions
    // Describes memory layout and shader positions
    vertices.attributeDescriptions.resize(2);
    // Location 0 : Position
    vertices.attributeDescriptions[0] =
        vk::VertexInputAttributeDescription(
            0,
            VERTEX_BUFFER_BIND_ID,
            vk::Format::eR32G32B32Sfloat,
            0);
    // Location 1 : Texture coordinates
    vertices.attributeDescriptions[1] =
        vk::VertexInputAttributeDescription(
            1,
            VERTEX_BUFFER_BIND_ID,
            vk::Format::eR32G32Sfloat,
            sizeof(float) * 3);

    vertices.inputState = vk::PipelineVertexInputStateCreateInfo();
    vertices.inputState.vertexBindingDescriptionCount =
                  static_cast<uint32_t>(vertices.bindingDescriptions.size());
    vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
    vertices.inputState.vertexAttributeDescriptionCount =
                  static_cast<uint32_t>(vertices.attributeDescriptions.size());
    vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
}

void
InstancedSampleBase::setupDescriptorPool()
{
    // Example uses one ubo and one image sampler
    std::vector<vk::DescriptorPoolSize> poolSizes =
    {
        {vk::DescriptorType::eUniformBuffer, 1},
        {vk::DescriptorType::eCombinedImageSampler, 1}
    };

    vk::DescriptorPoolCreateInfo descriptorPoolInfo(
                                        {},
                                        2,
                                        static_cast<uint32_t>(poolSizes.size()),
                                        poolSizes.data());
    vk::Result res = vkctx.device.createDescriptorPool(&descriptorPoolInfo,
                                                       nullptr,
                                                       &descriptorPool);
    if (res != vk::Result::eSuccess) {
        throw bad_vulkan_alloc((int)res, "createDescriptorPool");
    }
}

void
InstancedSampleBase::setupDescriptorSetLayout()
{
    DescriptorBindings descriptorBindings =
    {
        // Binding 0 : Vertex shader uniform buffer
        {0,
         vk::DescriptorType::eUniformBuffer,
         1,
         vk::ShaderStageFlagBits::eVertex},
        // Binding 1 : Fragment shader image sampler
        {1,
         vk::DescriptorType::eCombinedImageSampler,
         1,
         vk::ShaderStageFlagBits::eFragment},
    };

    //addSubclassDescriptors(descriptorBindings);

    vk::DescriptorSetLayoutCreateInfo descriptorLayout(
                              {},
                              static_cast<uint32_t>(descriptorBindings.size()),
                              descriptorBindings.data());

    vk::Result res
        = vkctx.device.createDescriptorSetLayout(&descriptorLayout,
                                                 nullptr,
                                                 &descriptorSetLayout);
    if (res != vk::Result::eSuccess) {
        throw bad_vulkan_alloc((int)res, "createDescriptorSetLayout");
    }

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo(
                                                    {},
                                                    1,
                                                    &descriptorSetLayout);

    std::vector<vk::PushConstantRange> pushConstantRanges;
    addSubclassPushConstantRanges(pushConstantRanges);
    if (pushConstantRanges.size() > 0) {
        pipelineLayoutCreateInfo.setPushConstantRanges(pushConstantRanges);
    }

    res = vkctx.device.createPipelineLayout(&pipelineLayoutCreateInfo,
                                            nullptr,
                                            &pipelineLayout);
    if (res != vk::Result::eSuccess) {
        throw bad_vulkan_alloc((int)res, "createPipelineLayout");
    }
}

void
InstancedSampleBase::setupDescriptorSet()
{
    vk::DescriptorSetAllocateInfo allocInfo(
            descriptorPool,
            1,
            &descriptorSetLayout);

    vk::Result res
        = vkctx.device.allocateDescriptorSets(&allocInfo, &descriptorSet);
    if (res != vk::Result::eSuccess) {
        throw bad_vulkan_alloc((int)res, "allocateDescriptorSets");
    }

    // Image descriptor for the color map texture
    vk::DescriptorImageInfo texDescriptor(
            sampler,
            imageView,
            vk::ImageLayout::eShaderReadOnlyOptimal);

    std::vector<vk::WriteDescriptorSet> writeDescriptorSets;
    // Binding 0 : Vertex shader uniform buffer
    writeDescriptorSets.push_back(vk::WriteDescriptorSet(
            descriptorSet,
            0,
            0,
            1,
            vk::DescriptorType::eUniformBuffer,
            nullptr,
            &uniformDataVS.descriptor)
        );
    // Binding 1 : Fragment shader texture sampler
    writeDescriptorSets.push_back(vk::WriteDescriptorSet(
            descriptorSet,
            1,
            0,
            1,
            vk::DescriptorType::eCombinedImageSampler,
            &texDescriptor)
    );

    vkctx.device.updateDescriptorSets(
                            static_cast<uint32_t>(writeDescriptorSets.size()),
                            writeDescriptorSets.data(),
                            0,
                            nullptr);
}

void
InstancedSampleBase::preparePipelines(const char* const fragShaderName,
                                      const char* const vertShaderName,
                                      uint32_t instanceCountConstId)
{
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState(
            {},
            vk::PrimitiveTopology::eTriangleList);

    vk::PipelineRasterizationStateCreateInfo rasterizationState;
    // Must be false because we haven't enabled the depthClamp device feature.
    rasterizationState.depthClampEnable = false;
    rasterizationState.rasterizerDiscardEnable = false;
    rasterizationState.polygonMode = vk::PolygonMode::eFill;
    rasterizationState.cullMode = vk::CullModeFlagBits::eNone;
    rasterizationState.frontFace = vk::FrontFace::eCounterClockwise;
    rasterizationState.lineWidth = 1.0f;

    vk::PipelineColorBlendAttachmentState blendAttachmentState;
    blendAttachmentState.blendEnable = false;
    //blendAttachmentState.colorWriteMask = 0xf;
    blendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR
                                          | vk::ColorComponentFlagBits::eG
                                          | vk::ColorComponentFlagBits::eB
                                          | vk::ColorComponentFlagBits::eA;

    vk::PipelineColorBlendStateCreateInfo colorBlendState;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &blendAttachmentState;

    vk::PipelineDepthStencilStateCreateInfo depthStencilState;
    depthStencilState.depthTestEnable = true;
    depthStencilState.depthWriteEnable = true;
    depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;

    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    vk::PipelineMultisampleStateCreateInfo multisampleState;
    multisampleState.rasterizationSamples = vk::SampleCountFlagBits::e1;

    std::vector<vk::DynamicState> dynamicStateEnables = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };
    vk::PipelineDynamicStateCreateInfo dynamicState(
            {},
            static_cast<uint32_t>(dynamicStateEnables.size()),
            dynamicStateEnables.data());

    // Load shaders
    std::array<vk::PipelineShaderStageCreateInfo,2> shaderStages;
    std::string filepath = getAssetPath();
    // What a lot of code to set a single constant value.
    vk::SpecializationInfo specializationInfo;
    vk::SpecializationMapEntry mapEntries[1];
    mapEntries[0].setConstantID(instanceCountConstId);
    mapEntries[0].setOffset(0);
    mapEntries[0].setSize(4);
    specializationInfo.setMapEntryCount(1);
    specializationInfo.setPMapEntries(mapEntries);
    specializationInfo.setPData(&instanceCount);
    specializationInfo.setDataSize(sizeof(instanceCount));
    shaderStages[0].pSpecializationInfo = &specializationInfo;
    shaderStages[0] = loadShader(filepath + vertShaderName,
                                vk::ShaderStageFlagBits::eVertex);
    shaderStages[1] = loadShader(filepath + fragShaderName,
                                vk::ShaderStageFlagBits::eFragment);

    vk::GraphicsPipelineCreateInfo pipelineCreateInfo;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.renderPass = vkctx.renderPass;
    pipelineCreateInfo.pVertexInputState = &vertices.inputState;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pColorBlendState = &colorBlendState;
    pipelineCreateInfo.pMultisampleState = &multisampleState;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pDepthStencilState = &depthStencilState;
    pipelineCreateInfo.pDynamicState = &dynamicState;
    pipelineCreateInfo.stageCount = (uint32_t)shaderStages.size();
    pipelineCreateInfo.pStages = shaderStages.data();

    vk::Result res
        = vkctx.device.createGraphicsPipelines(vkctx.pipelineCache, 1,
                                               &pipelineCreateInfo, nullptr,
                                               &pipelines.solid);
    if (res != vk::Result::eSuccess) {
        throw bad_vulkan_alloc((int)res, "createGraphicsPipelines");
    }
}

#define _PAD16(nbytes) (ktx_uint32_t)(16 * ceilf((float)(nbytes) / 16))

void
InstancedSampleBase::prepareUniformBuffers(uint32_t shaderDeclaredInstances)
{
    uboVS.instance = new UboInstanceData[instanceCount];

    // Elements of the array of UboInstanceData will be aligned on 16-byte
    // boundaries per the std140 rule for mat4/vec4. _PAD16 is unnecessary
    // right now but will become so if anything is added to the ubo before
    // the UboInstanceData. _PAD16 is put here as a warning.
    uint32_t uboSize = _PAD16(sizeof(uboVS.matrices))
             //+ instanceCount * sizeof(UboInstanceData);
             + shaderDeclaredInstances * sizeof(UboInstanceData);

    // Vertex shader uniform buffer block
    vkctx.createBuffer(
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        uboSize,
        nullptr,
        &uniformDataVS.buffer,
        &uniformDataVS.memory,
        &uniformDataVS.descriptor);

    // MoltenVK can't specialize array-length constants, an MSL limitation,
    // so we have to potentially modify instanceCount. We can't just
    // declare a very long array in the shaders because we get a MoltenVK
    // validation error when the allocation we make above is less than
    // the declared length. Making the array length 1, works on macOS
    // but not on iOS where only 1 instance is drawn correctly. See
    // MoltenVK issues 1420 and 1421.
    // https://github.com/KhronosGroup/MoltenVK/issues/1421.
    // Paren around std::min avoids a SNAFU that windef.h has a "min" macro.
    instanceCount = (std::min)(shaderDeclaredInstances, instanceCount);

    // Array indices and model matrices are fixed
    float offset = -1.5f;
    float center = (instanceCount * offset) / 2;
    for (uint32_t i = 0; i < instanceCount; i++)
    {
        // Instance model matrix
        uboVS.instance[i].model = glm::translate(glm::mat4(), glm::vec3(0.0f, i * offset - center, 0.0f));
        uboVS.instance[i].model = glm::rotate(uboVS.instance[i].model, glm::radians(60.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    }

    // Update instanced part of the uniform buffer
    uint8_t *pData;
    // N.B. See comment re _PAD16 before uboSize above.
    uint32_t dataOffset = _PAD16(sizeof(uboVS.matrices));
    uint32_t dataSize = instanceCount * sizeof(UboInstanceData);
    VK_CHECK_RESULT(vkMapMemory(vkctx.device, uniformDataVS.memory, dataOffset, dataSize, 0, (void **)&pData));
    memcpy(pData, uboVS.instance, dataSize);
    vkUnmapMemory(vkctx.device, uniformDataVS.memory);

    updateUniformBufferMatrices();
}

void
InstancedSampleBase::updateUniformBufferMatrices()
{
    // Only updates the uniform buffer block part containing the global matrices

    // Projection
    uboVS.matrices.projection = glm::perspective(glm::radians(60.0f), (float)w_width / (float)w_height, 0.001f, 256.0f);

    // View
    uboVS.matrices.view = glm::translate(glm::mat4(), glm::vec3(0.0f, -1.0f, zoom));
    uboVS.matrices.view *= glm::translate(glm::mat4(), cameraPos);
    uboVS.matrices.view = glm::rotate(uboVS.matrices.view, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    uboVS.matrices.view = glm::rotate(uboVS.matrices.view, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    uboVS.matrices.view = glm::rotate(uboVS.matrices.view, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    // Only update the matrices part of the uniform buffer
    uint8_t *pData;
    VK_CHECK_RESULT(vkMapMemory(vkctx.device, uniformDataVS.memory, 0, sizeof(uboVS.matrices), 0, (void **)&pData));
    memcpy(pData, &uboVS.matrices, sizeof(uboVS.matrices));
    vkUnmapMemory(vkctx.device, uniformDataVS.memory);
}

void
InstancedSampleBase::prepareSamplerAndView()
{
    // Create sampler.
    vk::SamplerCreateInfo samplerInfo;
    // Set the non-default values
    samplerInfo.magFilter = filter;
    samplerInfo.minFilter = filter;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.maxLod = (float)texture.levelCount;
    if (vkctx.gpuFeatures.samplerAnisotropy == VK_TRUE) {
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 8;
    } else {
        // vulkan.hpp needs fixing
        samplerInfo.maxAnisotropy = 1.0;
    }
    samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
    // To make viewer more useful in verifying the content of 3d textures.
    samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
    sampler = vkctx.device.createSampler(samplerInfo);
    
    // Create image view.
    // Textures are not directly accessed by the shaders and are abstracted
    // by image views containing additional information and sub resource
    // ranges.
    vk::ImageViewCreateInfo viewInfo;
    // Set the non-default values.
    viewInfo.image = texture.image;
    viewInfo.format = static_cast<vk::Format>(texture.imageFormat);
    viewInfo.viewType
    = static_cast<vk::ImageViewType>(texture.viewType);
    viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    viewInfo.subresourceRange.layerCount = texture.layerCount;
    viewInfo.subresourceRange.levelCount = texture.levelCount;
    imageView = vkctx.device.createImageView(viewInfo);
}

void
InstancedSampleBase::prepare(const char* const fragShaderName,
                             const char* const vertShaderName,
                             uint32_t instanceCountConstId,
                             uint32_t instanceCountIn,
                             uint32_t shaderDeclaredInstances)
{
    this->instanceCount = instanceCountIn;
    prepareSamplerAndView();
    setupVertexDescriptions();
    generateQuad();
    prepareUniformBuffers(shaderDeclaredInstances);
    setupDescriptorSetLayout();
    preparePipelines(fragShaderName, vertShaderName,
                     instanceCountConstId);
    setupDescriptorPool();
    setupDescriptorSet();
    vkctx.createDrawCommandBuffers();
    buildCommandBuffers();
}

const char*
InstancedSampleBase::customizeTitle(const char* const baseTitle)
{
    if (transcoded) {
        this->title = baseTitle;
        this->title += " Transcoded to ";
        this->title += vkFormatString((VkFormat)transcodedFormat);
        return this->title.c_str();
    }
    return baseTitle;
}

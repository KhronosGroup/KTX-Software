/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab : */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @class Texture
 * @~English
 *
 * @brief Test loading of 2D textures.
 *
 * @author Mark Callow, www.edgewise-consulting.com.
 *
 * @par Acknowledgement
 * Thanks to Sascha Willems' - www.saschawillems.de - for the concept,
 * the VulkanTextOverlay class and the shaders used by this test.
 */

#if defined(_WIN32)
  #define _CRT_SECURE_NO_WARNINGS // For sscanf
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <exception>
#include <vector>

#include "argparser.h"
#include "Texture.h"
#include "VulkanTextureTranscoder.hpp"
#include "ltexceptions.h"

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION false

// Vertex layout for this example
struct Vertex {
    std::array<float, 3> pos;
    std::array<float, 2> uv;
    std::array<float, 3> normal;
    std::array<float, 3> color;
};

VulkanLoadTestSample*
Texture::create(VulkanContext& vkctx,
                 uint32_t width, uint32_t height,
                 const char* const szArgs,
                 const std::string sBasePath)
{
    return new Texture(vkctx, width, height, szArgs, sBasePath);
}

Texture::Texture(VulkanContext& vkctx,
                 uint32_t width, uint32_t height,
                 const char* const szArgs,
                 const std::string sBasePath)
        : VulkanLoadTestSample(vkctx, width, height, sBasePath)
{
    zoom = -2.5f;
    rotation = { 0.0f, 15.0f, 0.0f };
    tiling = vk::ImageTiling::eOptimal;
    useSubAlloc = UseSuballocator::No;
    rgbcolor upperLeftColor{ 0.7f, 0.1f, 0.2f };
    rgbcolor lowerLeftColor{ 0.8f, 0.9f, 0.3f };
    rgbcolor upperRightColor{ 0.4f, 1.0f, 0.5f };
    rgbcolor lowerRightColor{ 0.0f, 0.6f, 0.1f };

    transcoded = false;

    quadColor = { upperLeftColor, lowerLeftColor,
                  upperRightColor, lowerRightColor };

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
    vk::FormatProperties properties;
    vkctx.gpu.getFormatProperties(vkFormat, &properties);
    vk::FormatFeatureFlags& features =  tiling == vk::ImageTiling::eLinear ?
                                        properties.linearTilingFeatures :
                                        properties.optimalTilingFeatures;
    vk::FormatFeatureFlags wantedFeatures =
             vk::FormatFeatureFlagBits::eSampledImage
           | vk::FormatFeatureFlagBits::eSampledImageFilterLinear;
    if (!(features & wantedFeatures)) {
        ktxTexture_Destroy(kTexture);
        throw unsupported_ttype();
    }

    if (useSubAlloc == UseSuballocator::Yes)
    {
        VkInstance vkInst = vkctx.instance;
        VMA_CALLBACKS::InitVMA(vdi.physicalDevice, vdi.device, vkInst, vdi.deviceMemoryProperties);

        ktxresult = ktxTexture_VkUploadEx_WithSuballocator(kTexture, &vdi, &texture,
                                                           static_cast<VkImageTiling>(tiling),
                                                           VK_IMAGE_USAGE_SAMPLED_BIT,
                                                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, &subAllocatorCallbacks);
    }
    else // Keep separate call so ktxTexture_VkUploadEx is also tested.
        ktxresult = ktxTexture_VkUploadEx(kTexture, &vdi, &texture,
                                          static_cast<VkImageTiling>(tiling),
                                          VK_IMAGE_USAGE_SAMPLED_BIT,
                                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    if (KTX_SUCCESS != ktxresult) {
        std::stringstream message;
        
        message << "ktxTexture_VkUpload failed: " << ktxErrorString(ktxresult);
        throw std::runtime_error(message.str());
    }

    if (kTexture->orientation.x == KTX_ORIENT_X_LEFT)
        sign_s = -1;
    if (kTexture->orientation.y == KTX_ORIENT_Y_UP)
        sign_t = -1;

    ktx_uint32_t swizzleLen;
    char* swizzleStr;
    ktxresult = ktxHashList_FindValue(&kTexture->kvDataHead, KTX_SWIZZLE_KEY,
                                      &swizzleLen, (void**)&swizzleStr);
    if (ktxresult == KTX_SUCCESS && swizzleLen == 5) {
        if (gpuSupportsSwizzle()) {
            swizzle.r = swizzleStr[0] == 'r' ? vk::ComponentSwizzle::eR
                      : swizzleStr[0] == 'g' ? vk::ComponentSwizzle::eG
                      : swizzleStr[0] == 'b' ? vk::ComponentSwizzle::eB
                      : swizzleStr[0] == 'a' ? vk::ComponentSwizzle::eA
                      : swizzleStr[0] == '0' ? vk::ComponentSwizzle::eZero
                      : vk::ComponentSwizzle::eOne;
            swizzle.g = swizzleStr[1] == 'r' ? vk::ComponentSwizzle::eR
                      : swizzleStr[1] == 'g' ? vk::ComponentSwizzle::eG
                      : swizzleStr[1] == 'b' ? vk::ComponentSwizzle::eB
                      : swizzleStr[1] == 'a' ? vk::ComponentSwizzle::eA
                      : swizzleStr[1] == '0' ? vk::ComponentSwizzle::eZero
                      : vk::ComponentSwizzle::eOne;
            swizzle.b = swizzleStr[2] == 'r' ? vk::ComponentSwizzle::eR
                      : swizzleStr[2] == 'g' ? vk::ComponentSwizzle::eG
                      : swizzleStr[2] == 'b' ? vk::ComponentSwizzle::eB
                      : swizzleStr[2] == 'a' ? vk::ComponentSwizzle::eA
                      : swizzleStr[2] == '0' ? vk::ComponentSwizzle::eZero
                      : vk::ComponentSwizzle::eOne;
            swizzle.a = swizzleStr[3] == 'r' ? vk::ComponentSwizzle::eR
                      : swizzleStr[3] == 'g' ? vk::ComponentSwizzle::eG
                      : swizzleStr[3] == 'b' ? vk::ComponentSwizzle::eB
                      : swizzleStr[3] == 'a' ? vk::ComponentSwizzle::eA
                      : swizzleStr[3] == '0' ? vk::ComponentSwizzle::eZero
                      : vk::ComponentSwizzle::eOne;
        } else {
            std::stringstream message;
            message << "Input file has swizzle metadata but "
                    << "app is running on a VK_KHR_portability_subset device "
                    << "that does not support swizzling.";
            // I have absolutely no idea why the following line makes clang
            // raise an error about no matching conversion from
            // std::__1::basic_stringstream to unsupported_ttype
            // so I've resorted to using a temporary variable.
            //throw(unsupported_ttype(message.str());
            std::string msg = message.str();
            throw(unsupported_ttype(msg));
        }
    }

    ktxTexture_Destroy(kTexture);
    ktxVulkanDeviceInfo_Destruct(&vdi);

    try {
        prepare();
    } catch (std::exception& e) {
        (void)e; // To quiet unused variable warnings from some compilers.
        cleanup();
        throw;
    }
}

Texture::~Texture()
{
    cleanup();
}

void
Texture::resize(uint32_t width, uint32_t height)
{
    this->w_width = width;
    this->w_height = height;
    vkctx.destroyDrawCommandBuffers();
    vkctx.createDrawCommandBuffers();
    buildCommandBuffers();
    updateUniformBuffers();
}

void
Texture::run(uint32_t /*msTicks*/)
{
    // Nothing to do since the scene is not animated.
    // VulkanLoadTests base class redraws from the command buffer we built.
}

//===================================================================

void
Texture::processArgs(std::string sArgs)
{
    // Options descriptor
    struct argparser::option longopts[] = {
      {"external",      argparser::option::no_argument,       &externalFile,        1},
      {"linear-tiling", argparser::option::no_argument,       (int*)&tiling,        (int)vk::ImageTiling::eLinear},
      {"use-vma",       argparser::option::no_argument,       (int*)&useSubAlloc,   (int)UseSuballocator::Yes},
      {"qcolor",        argparser::option::required_argument, NULL,                 1},
      {NULL,            argparser::option::no_argument,       NULL,                 0}
    };

    argvector argv(sArgs);
    argparser ap(argv);
    
    int ch;
    while ((ch = ap.getopt(nullptr, longopts, nullptr)) != -1) {
        switch (ch) {
            case 0: break;
            case 1:
            {
                std::istringstream in(ap.optarg);
                rgbcolor clr;
                int i;

                for (i = 0; i < 4 && !in.eof(); i++) {
                    in >> clr[0] >> skip(",") >> clr[1] >> skip(",") >> clr[2];
                    quadColor[i] = clr;
                    if (!in.eof())
                        in >> skip(",");
                }
                assert(!in.fail() && (i == 1 || i == 4));
                if (i == 1) {
                    for(; i < 4; i++)
                        quadColor[i] = quadColor[0];
                }
                break;
            }
            default: assert(false); // Error in args in sample table.
        }
    }
    assert(ap.optind < argv.size());
    ktxfilename = argv[ap.optind];
}

/* ------------------------------------------------------------------------- */

// It is difficult to have these members clean up up their own mess, hence
// this. Some of them are vulkan.hpp objects that have no destructors and
// no record of the device. We could add destructors for our own but each
// would have to remember the device.
void
Texture::cleanup()
{
    // Clean up used Vulkan resources

    vkctx.destroyDrawCommandBuffers();

    if (sampler)
        vkctx.device.destroySampler(sampler);
    if (imageView)
        vkctx.device.destroyImageView(imageView);


    if (useSubAlloc == UseSuballocator::Yes)
    {
        VkDevice vkDev = vkctx.device;
        (void)ktxVulkanTexture_Destruct_WithSuballocator(&texture, vkDev, VK_NULL_HANDLE, &subAllocatorCallbacks);
        VMA_CALLBACKS::DestroyVMA();
    }
    else // Keep separate call so ktxVulkanTexture_Destruct is also tested.
        ktxVulkanTexture_Destruct(&texture, vkctx.device, nullptr);

    if (pipelines.solid)
        vkctx.device.destroyPipeline(pipelines.solid);
    if (pipelineLayout)
        vkctx.device.destroyPipelineLayout(pipelineLayout);
    if (descriptorSetLayout)
        vkctx.device.destroyDescriptorSetLayout(descriptorSetLayout);

    quad.freeResources(vkctx.device);
    uniformDataVS.freeResources(vkctx.device);
}

void
Texture::buildCommandBuffers()
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

        vkCmdBindDescriptorSets(vkctx.drawCmdBuffers[i],
                VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                &static_cast<const VkDescriptorSet&>(descriptorSet), 0, NULL);
        vkCmdBindPipeline(vkctx.drawCmdBuffers[i],
                          VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.solid);

        VkDeviceSize offsets[1] = { 0 };
        vkCmdBindVertexBuffers(vkctx.drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID,
                1, &static_cast<const VkBuffer&>(quad.vertices.buf), offsets);
        vkCmdBindIndexBuffer(vkctx.drawCmdBuffers[i], quad.indices.buf, 0,
                             VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(vkctx.drawCmdBuffers[i], quad.indexCount, 1, 0, 0, 0);

        vkCmdEndRenderPass(vkctx.drawCmdBuffers[i]);

        VK_CHECK_RESULT(vkEndCommandBuffer(vkctx.drawCmdBuffers[i]));
    }
}

void
Texture::generateQuad()
{
    // Setup vertices for a single uv-mapped quad
#define DIM 1.0f
#define NORMAL { 0.0f, 0.0f, 1.0f }
    std::vector<Vertex> vertexBuffer = {
        { { -DIM, -DIM, 0.0f }, { 0.0f, 0.0f }, NORMAL, { quadColor[0] } },
        { { -DIM,  DIM, 0.0f }, { 0.0f, 1.0f }, NORMAL, { quadColor[1] } },
        { {  DIM, -DIM, 0.0f }, { 1.0f, 0.0f }, NORMAL, { quadColor[2] } },
        { {  DIM,  DIM, 0.0f }, { 1.0f, 1.0f }, NORMAL, { quadColor[3] } }
    };
#undef DIM
#undef NORMAL

    if (sign_s < 0 || sign_t < 0) {
        // Transform the texture coordinates to get correct image orientation.
        for (uint32_t i = 0; i < vertexBuffer.size(); i++) {
            if (sign_t < 1) {
                vertexBuffer[i].uv[1] = vertexBuffer[i].uv[1] * -1 + 1;
            }
            if (sign_s < 1) {
                vertexBuffer[i].uv[0] = vertexBuffer[i].uv[0] * -1 + 1;
            }
        }
    }
    vkctx.createBuffer(
        vk::BufferUsageFlagBits::eVertexBuffer,
        vertexBuffer.size() * sizeof(Vertex),
        vertexBuffer.data(),
        &quad.vertices.buf,
        &quad.vertices.mem);

    // Setup indices
    std::vector<uint32_t> indexBuffer = { 0,1,2,3 };
    quad.indexCount = static_cast<uint32_t>(indexBuffer.size());

    vkctx.createBuffer(
        vk::BufferUsageFlagBits::eIndexBuffer,
        indexBuffer.size() * sizeof(uint32_t),
        indexBuffer.data(),
        &quad.indices.buf,
        &quad.indices.mem);
}

void
Texture::setupVertexDescriptions()
{
    // Binding description
    vertices.bindingDescriptions.resize(1);
    vertices.bindingDescriptions[0] =
        vk::VertexInputBindingDescription(
            VERTEX_BUFFER_BIND_ID,
            sizeof(Vertex),
            vk::VertexInputRate::eVertex);
//#define OFFSET(f) (&(((struct Vertex*)0)->f) - &(struct Vertex*)0)
    // Attribute descriptions
    // Describes memory layout and shader positions
    vertices.attributeDescriptions.resize(4);
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
    // Location 2 : Vertex normal
    vertices.attributeDescriptions[2] =
        vk::VertexInputAttributeDescription(
            2,
            VERTEX_BUFFER_BIND_ID,
            vk::Format::eR32G32B32Sfloat,
            sizeof(float) * 5);
    // Location 3 : Color
    vertices.attributeDescriptions[3] =
        vk::VertexInputAttributeDescription(
            3,
            VERTEX_BUFFER_BIND_ID,
            vk::Format::eR32G32B32Sfloat,
            sizeof(float) * 8);


    vertices.inputState = vk::PipelineVertexInputStateCreateInfo();
    vertices.inputState.vertexBindingDescriptionCount =
                static_cast<uint32_t>(vertices.bindingDescriptions.size());
    vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
    vertices.inputState.vertexAttributeDescriptionCount =
                static_cast<uint32_t>(vertices.attributeDescriptions.size());
    vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
}

void
Texture::setupDescriptorPool()
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
Texture::setupDescriptorSetLayout()
{
    std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings =
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

    vk::DescriptorSetLayoutCreateInfo descriptorLayout(
                              {},
                              static_cast<uint32_t>(setLayoutBindings.size()),
                              setLayoutBindings.data());

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

    res = vkctx.device.createPipelineLayout(&pipelineLayoutCreateInfo,
                                            nullptr,
                                            &pipelineLayout);
    if (res != vk::Result::eSuccess) {
        throw bad_vulkan_alloc((int)res, "createPipelineLayout");
    }
}

void
Texture::setupDescriptorSet()
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
Texture::preparePipelines()
{
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState(
            {},
            vk::PrimitiveTopology::eTriangleStrip);

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
    shaderStages[0] = loadShader(filepath + "texture.vert.spv",
                                vk::ShaderStageFlagBits::eVertex);
    std::string fragShader = filepath;
    if (texture.viewType == VK_IMAGE_VIEW_TYPE_1D)
        fragShader += "texture1d";
    else
        fragShader += "texture2d";
    fragShader += ".frag.spv";
    shaderStages[1] = loadShader(fragShader,
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

// Prepare and initialize uniform buffer containing shader uniforms
void
Texture::prepareUniformBuffers()
{
    // Vertex shader uniform buffer block
    vkctx.createBuffer(
        vk::BufferUsageFlagBits::eUniformBuffer,
        sizeof(uboVS),
        &uboVS,
        &uniformDataVS.buffer,
        &uniformDataVS.memory,
        &uniformDataVS.descriptor);

    updateUniformBuffers();
}

void
Texture::updateUniformBuffers()
{
    if (w_width == 0 || w_height == 0)
        return;
    // Vertex shader
    uboVS.projection = glm::perspective(glm::radians(60.0f), (float)w_width / (float)w_height, 0.001f, 256.0f);
    glm::mat4 viewMatrix = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, zoom));

    uboVS.model = viewMatrix * glm::translate(glm::mat4(), cameraPos);
    uboVS.model = glm::rotate(uboVS.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    uboVS.model = glm::rotate(uboVS.model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    uboVS.model = glm::rotate(uboVS.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    // Because MetalSL does not have a matrix inverse function...
    // It looks like the glm::mat3(glm::mat4) does something different than
    // GLSL. If I convert to mat3 here, only half the quad is lit. Do it in
    // the shader.
    uboVS.normal = inverse(transpose(uboVS.model));

    uboVS.viewPos = glm::vec4(0.0f, 0.0f, -zoom, 0.0f);

    uint8_t *pData;
    VK_CHECK_RESULT(vkMapMemory(vkctx.device, uniformDataVS.memory, 0, sizeof(uboVS), 0, (void **)&pData));
    memcpy(pData, &uboVS, sizeof(uboVS));
    vkUnmapMemory(vkctx.device, uniformDataVS.memory);
}

void
Texture::prepareSamplerAndView()
{
    // Create sampler.
    vk::SamplerCreateInfo samplerInfo;
    // Set the non-default values
    samplerInfo.magFilter = vk::Filter::eLinear;
    samplerInfo.minFilter = vk::Filter::eLinear;
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
    sampler = vkctx.device.createSampler(samplerInfo);
    
    // Create image view.
    // Textures are not directly accessed by the shaders and are abstracted
    // by image views containing additional information and sub resource
    // ranges.
    vk::ImageViewCreateInfo viewInfo;
    // Set the non-default values.
    viewInfo.components = swizzle;
    viewInfo.image = texture.image;
    viewInfo.format = static_cast<vk::Format>(texture.imageFormat);
    viewInfo.viewType = static_cast<vk::ImageViewType>(texture.viewType);
    viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    viewInfo.subresourceRange.layerCount = texture.layerCount;
    viewInfo.subresourceRange.levelCount = texture.levelCount;
    imageView = vkctx.device.createImageView(viewInfo);
}

void
Texture::prepare()
{
    prepareSamplerAndView();
    generateQuad();
    setupVertexDescriptions();
    prepareUniformBuffers();
    setupDescriptorSetLayout();
    preparePipelines();
    setupDescriptorPool();
    setupDescriptorSet();
    vkctx.createDrawCommandBuffers();
    buildCommandBuffers();
}

void
Texture::changeLodBias(float delta)
{
    uboVS.lodBias += delta;
    if (uboVS.lodBias < 0.0f)
    {
        uboVS.lodBias = 0.0f;
    }
    if (uboVS.lodBias > texture.levelCount)
    {
        uboVS.lodBias = (float)texture.levelCount;
    }
    updateUniformBuffers();
    //updateTextOverlay();
}

void
Texture::keyPressed(uint32_t keyCode)
{
    switch (keyCode)
    {
    case SDLK_KP_PLUS:
        changeLodBias(0.1f);
        break;
    case SDLK_KP_MINUS:
        changeLodBias(-0.1f);
        break;
    }
}

void
Texture::getOverlayText(VulkanTextOverlay *textOverlay, float yOffset)
{
    std::stringstream ss;
    ss << std::setprecision(2) << std::fixed << uboVS.lodBias;
    textOverlay->addText("LOD bias: " + ss.str() + " (numpad +/- to change)",
                         5.0f, yOffset, VulkanTextOverlay::alignLeft);
}

const char*
Texture::customizeTitle(const char* const baseTitle)
{
    if (transcoded) {
        this->title = baseTitle;
        this->title += " Transcoded to ";
        this->title += vkFormatString((VkFormat)transcodedFormat);
        return this->title.c_str();
    }
    return baseTitle;
}


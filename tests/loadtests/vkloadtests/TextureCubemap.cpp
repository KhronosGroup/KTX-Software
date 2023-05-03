/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @class TextureCubemap
 * @~English
 *
 * @brief Test loading of cubemap textures.
 *
 * @author Mark Callow, www.edgewise-consulting.com.
 *
 * @par Acknowledgement
 * Thanks to Sascha Willems' - www.saschawillems.de - for the concept,
 * the VulkanTextOverlay  VulkanMeshLoader classes and the shaders used
 * by this test.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>

#include "TextureCubemap.h"
#include "VulkanTextureTranscoder.hpp"
#include "argparser.h"
#include "ltexceptions.h"

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION false
#define USE_GL_RH_NDC 0

// Vertex layout for this example
std::vector<vkMeshLoader::VertexLayout> vertexLayout =
{
    vkMeshLoader::VERTEX_LAYOUT_POSITION,
    vkMeshLoader::VERTEX_LAYOUT_NORMAL,
    vkMeshLoader::VERTEX_LAYOUT_UV
};

VulkanLoadTestSample*
TextureCubemap::create(VulkanContext& vkctx,
                 uint32_t width, uint32_t height,
                 const char* const szArgs, const std::string sBasePath)
{
    return new TextureCubemap(vkctx, width, height, szArgs, sBasePath,
                              USE_GL_RH_NDC ? 1 : -1);
}

TextureCubemap::TextureCubemap(VulkanContext& vkctx,
                 uint32_t width, uint32_t height,
                 const char* const szArgs, const std::string sBasePath,
                 int32_t yflip)
        : VulkanLoadTestSample(vkctx, width, height, sBasePath, yflip)
{
    zoom = -4.0f;
    rotationSpeed = 0.25f;
    rotation = { -7.25f, 120.0f, 0.0f };
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
                         preloadImages ? KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT
                                       : KTX_TEXTURE_CREATE_NO_FLAGS,
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

    vk::Format
        vkFormat = static_cast<vk::Format>(ktxTexture_GetVkFormat(kTexture));
    transcodedFormat = vkFormat;
    vk::FormatProperties properties;
    vkctx.gpu.getFormatProperties(vkFormat, &properties);
    vk::FormatFeatureFlags wantedFeatures =
             vk::FormatFeatureFlagBits::eSampledImage
           | vk::FormatFeatureFlagBits::eSampledImageFilterLinear;
    if (!(properties.optimalTilingFeatures & wantedFeatures)) {
         ktxTexture_Destroy(kTexture);
         throw unsupported_ttype();
    }

    ktxresult = ktxTexture_VkUpload(kTexture, &vdi, &cubeMap);
    
    if (KTX_SUCCESS != ktxresult) {
        std::stringstream message;
        
        message << "ktxTexture_VkUpload failed: " << ktxErrorString(ktxresult);
        throw std::runtime_error(message.str());
    }
    
    if (kTexture->orientation.y == KTX_ORIENT_Y_DOWN) {
        // Assume a KTX-compliant cubemap. That means the faces are in a
        // LH coord system with +y up. Multiply the cube's y and z by -1 to
        // put the +z face in front of the view and keep +y up. Alternatively
        // we could multiply the y and x coords by -1 to kepp the +y up while
        // placing the +z face in the +z direction.
#if !USE_GL_RH_NDC
        ubo.uvwTransform = glm::scale(glm::mat4(1.0f), glm::vec3(1, -1, -1));
#else
        // Scale the skybox cube's z by -1 to convert it to LH coords
        // with the +z face in front of the view.
        ubo.uvwTransform = glm::scale(glm::mat4(1.0f), glm::vec3(1, 1, -1));
#endif
    } else {
        std::stringstream message;

        message << "Cubemap faces have unsupported KTXorientation value.";
        throw std::runtime_error(message.str());
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

TextureCubemap::~TextureCubemap()
{
    cleanup();
}

void TextureCubemap::resize(uint32_t width, uint32_t height)
{
    this->w_width = width;
    this->w_height = height;
    rebuildCommandBuffers();
    updateUniformBuffers();
}

void
TextureCubemap::run(uint32_t /*msTicks*/)
{
    // Nothing to do since the scene is not animated.
    // VulkanLoadTests base class redraws from the command buffer we built.
}

//===================================================================

void
TextureCubemap::processArgs(std::string sArgs)
{
    // Options descriptor
    struct argparser::option longopts[] = {
      {"external", argparser::option::no_argument, &externalFile, 1},
      {"preload",  argparser::option::no_argument,  &preloadImages, 1},
      {NULL,       argparser::option::no_argument,  NULL,          0}
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
TextureCubemap::cleanup()
{
    // Clean up used Vulkan resources

    // Clean up texture resources
    if (sampler)
        vkctx.device.destroySampler(sampler);
    if (imageView)
        vkctx.device.destroyImageView(imageView);
    ktxVulkanTexture_Destruct(&cubeMap, vkctx.device, nullptr);

    if (pipelines.reflect)
        vkctx.device.destroyPipeline(pipelines.reflect);
    if (pipelines.skybox)
        vkctx.device.destroyPipeline(pipelines.skybox);
    if (pipelineLayout)
        vkctx.device.destroyPipelineLayout(pipelineLayout);
    if (descriptorSetLayout)
        vkctx.device.destroyDescriptorSetLayout(descriptorSetLayout);

    vkctx.destroyDrawCommandBuffers();

    for (size_t i = 0; i < meshes.objects.size(); i++)
    {
        vkMeshLoader::freeMeshBufferResources(vkctx.device, &meshes.objects[i]);
    }
    vkMeshLoader::freeMeshBufferResources(vkctx.device, &meshes.skybox);

    uniformData.object.freeResources(vkctx.device);
    uniformData.skybox.freeResources(vkctx.device);
}

void
TextureCubemap::rebuildCommandBuffers()
{
    if (!vkctx.checkDrawCommandBuffers())
    {
        vkctx.destroyDrawCommandBuffers();
        vkctx.createDrawCommandBuffers();
    }
    buildCommandBuffers();
}

void
TextureCubemap::buildCommandBuffers()
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

#if !USE_GL_RH_NDC
        vk::Viewport viewport(0, 0,
                              (float)w_width, (float)w_height,
                              0.0f, 1.0f);
#else
        // Make OpenGL style viewport
        vk::Viewport viewport(0, (float)w_height,
                              (float)w_width, -(float)w_height,
                              0.0f, 1.0f);
#endif
        vkCmdSetViewport(vkctx.drawCmdBuffers[i], 0, 1,
                &static_cast<const VkViewport&>(viewport));

        vk::Rect2D scissor({0, 0}, {w_width, w_height});
        vkCmdSetScissor(vkctx.drawCmdBuffers[i], 0, 1,
                &static_cast<const VkRect2D&>(scissor));

        VkDeviceSize offsets[1] = { 0 };

        // Skybox
        if (displaySkybox)
        {
            vkCmdBindDescriptorSets(vkctx.drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout, 0, 1,
                    &static_cast<const VkDescriptorSet&>(descriptorSets.skybox), 0, NULL);
            vkCmdBindVertexBuffers(vkctx.drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.skybox.vertices.buf, offsets);
            vkCmdBindIndexBuffer(vkctx.drawCmdBuffers[i], meshes.skybox.indices.buf, 0, VK_INDEX_TYPE_UINT32);
            vkCmdBindPipeline(vkctx.drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.skybox);
            vkCmdDrawIndexed(vkctx.drawCmdBuffers[i], meshes.skybox.indexCount, 1, 0, 0, 0);
        }

        // 3D object
        vkCmdBindDescriptorSets(vkctx.drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout, 0, 1,
                &static_cast<const VkDescriptorSet&>(descriptorSets.object), 0, NULL);
        vkCmdBindVertexBuffers(vkctx.drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.objects[meshes.objectIndex].vertices.buf, offsets);
        vkCmdBindIndexBuffer(vkctx.drawCmdBuffers[i], meshes.objects[meshes.objectIndex].indices.buf, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindPipeline(vkctx.drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.reflect);
        vkCmdDrawIndexed(vkctx.drawCmdBuffers[i], meshes.objects[meshes.objectIndex].indexCount, 1, 0, 0, 0);

        vkCmdEndRenderPass(vkctx.drawCmdBuffers[i]);

        VK_CHECK_RESULT(vkEndCommandBuffer(vkctx.drawCmdBuffers[i]));
    }
}

void
TextureCubemap::loadMeshes()
{
    std::string filepath = getAssetPath();

    // Skybox
    loadMesh(filepath + "cube.obj", &meshes.skybox, vertexLayout, 0.05f);
    // Objects
    meshes.objects.resize(3);
    loadMesh(filepath + "sphere.obj", &meshes.objects[0], vertexLayout, 0.05f);
    loadMesh(filepath + "teapot.dae", &meshes.objects[1], vertexLayout, 0.05f);
    loadMesh(filepath + "torusknot.obj", &meshes.objects[2], vertexLayout, 0.05f);
}

void
TextureCubemap::setupVertexDescriptions()
{
    // Binding description
    vertices.bindingDescriptions.resize(1);
    vertices.bindingDescriptions[0] =
        vk::VertexInputBindingDescription(
            VERTEX_BUFFER_BIND_ID,
            vkMeshLoader::vertexSize(vertexLayout),
            vk::VertexInputRate::eVertex);

    // Attribute descriptions
    // Describes memory layout and shader positions
    vertices.attributeDescriptions.resize(3);
    // Location 0 : Position
    vertices.attributeDescriptions[0] =
        vk::VertexInputAttributeDescription(
            0,
            VERTEX_BUFFER_BIND_ID,
            vk::Format::eR32G32B32Sfloat,
            0);
    // Location 1 : Vertex normal
    vertices.attributeDescriptions[1] =
        vk::VertexInputAttributeDescription(
            1,
            VERTEX_BUFFER_BIND_ID,
            vk::Format::eR32G32B32Sfloat,
            sizeof(float) * 3);
    // Location 2 : Texture coordinates
    vertices.attributeDescriptions[2] =
        vk::VertexInputAttributeDescription(
            2,
            VERTEX_BUFFER_BIND_ID,
            vk::Format::eR32G32Sfloat,
            sizeof(float) * 6);

    vertices.inputState = vk::PipelineVertexInputStateCreateInfo();
    vertices.inputState.vertexBindingDescriptionCount =
                static_cast<uint32_t>(vertices.bindingDescriptions.size());
    vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
    vertices.inputState.vertexAttributeDescriptionCount =
                static_cast<uint32_t>(vertices.attributeDescriptions.size());
    vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
}

void
TextureCubemap::setupDescriptorPool()
{
    // Example uses one ubo and one image sampler
    std::vector<vk::DescriptorPoolSize> poolSizes =
    {
        {vk::DescriptorType::eUniformBuffer, 2},
        {vk::DescriptorType::eCombinedImageSampler, 2}
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
TextureCubemap::setupDescriptorSetLayout()
{
    std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings =
    {
        // Binding 0 : Vertex shader uniform buffer
        {
            0,
            vk::DescriptorType::eUniformBuffer,
            1,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        },
        // Binding 1 : Fragment shader image sampler
        {
            1,
            vk::DescriptorType::eCombinedImageSampler,
            1,
            vk::ShaderStageFlagBits::eFragment
        },
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
TextureCubemap::setupDescriptorSets()
{
    vk::DescriptorSetAllocateInfo allocInfo(
            descriptorPool,
            1,
            &descriptorSetLayout);

    vk::Result res
         = vkctx.device.allocateDescriptorSets(&allocInfo,
                                               &descriptorSets.object);
    if (res != vk::Result::eSuccess) {
        throw bad_vulkan_alloc((int)res, "allocateDescriptorSets");
    }

    // Image descriptor for the cubemap texture
    vk::DescriptorImageInfo cubeMapDescriptor(
            sampler,
            imageView,
            vk::ImageLayout::eShaderReadOnlyOptimal);

    std::vector<vk::WriteDescriptorSet> writeDescriptorSets =
    {
        // Binding 0 : Vertex shader uniform buffer
        vk::WriteDescriptorSet(
                descriptorSets.object,
                0,
                0,
                1,
                vk::DescriptorType::eUniformBuffer,
                nullptr,
                &uniformData.object.descriptor),
        // Binding 1 : Fragment shader cubemap sampler
        vk::WriteDescriptorSet(
                descriptorSets.object,
                1,
                0,
                1,
                vk::DescriptorType::eCombinedImageSampler,
                &cubeMapDescriptor)
    };

    vkctx.device.updateDescriptorSets(
                             static_cast<uint32_t>(writeDescriptorSets.size()),
                             writeDescriptorSets.data(),
                             0,
                             nullptr);

    // Sky box descriptor set
    res = vkctx.device.allocateDescriptorSets(&allocInfo,
                                              &descriptorSets.skybox);
    if (res != vk::Result::eSuccess) {
        throw bad_vulkan_alloc((int)res, "allocateDescriptorSets");
    }

    writeDescriptorSets =
    {
        // Binding 0 : Vertex shader uniform buffer
        vk::WriteDescriptorSet(
                descriptorSets.skybox,
                0,
                0,
                1,
                vk::DescriptorType::eUniformBuffer,
                nullptr,
                &uniformData.skybox.descriptor),
        // Binding 1 : Fragment shader texture sampler
        vk::WriteDescriptorSet(
                descriptorSets.skybox,
                1,
                0,
                1,
                vk::DescriptorType::eCombinedImageSampler,
                &cubeMapDescriptor)
    };

    vkctx.device.updateDescriptorSets(
                              static_cast<uint32_t>(writeDescriptorSets.size()),
                              writeDescriptorSets.data(),
                              0,
                              nullptr);
}

void
TextureCubemap::preparePipelines()
{
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState(
            {},
            vk::PrimitiveTopology::eTriangleList);

    vk::PipelineRasterizationStateCreateInfo rasterizationState;
    // Must be false because we haven't enabled the depthClamp device feature.
    rasterizationState.depthClampEnable = false;
    rasterizationState.rasterizerDiscardEnable = VK_FALSE;
    rasterizationState.polygonMode = vk::PolygonMode::eFill;
    rasterizationState.cullMode = vk::CullModeFlagBits::eBack;
    // Make the faces on the inside of the cube the front faces. The mesh
    // was designed with the exterior faces as the front faces for OpenGL's
    // default of GL_CCW.
#if !USE_GL_RH_NDC
    rasterizationState.frontFace = vk::FrontFace::eCounterClockwise;
#else
    rasterizationState.frontFace = vk::FrontFace::eClockwise;
#endif
    rasterizationState.lineWidth = 1.0f;

    vk::PipelineColorBlendAttachmentState blendAttachmentState;
    blendAttachmentState.blendEnable = VK_FALSE;
    //blendAttachmentState.colorWriteMask = 0xf;
    blendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR
                                          | vk::ColorComponentFlagBits::eG
                                          | vk::ColorComponentFlagBits::eB
                                          | vk::ColorComponentFlagBits::eA;

    vk::PipelineColorBlendStateCreateInfo colorBlendState;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &blendAttachmentState;

    vk::PipelineDepthStencilStateCreateInfo depthStencilState;
    depthStencilState.depthTestEnable = VK_FALSE;
    depthStencilState.depthWriteEnable = VK_FALSE;
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
    shaderStages[0] = loadShader(filepath + "skybox.vert.spv",
                                vk::ShaderStageFlagBits::eVertex);
    shaderStages[1] = loadShader(filepath + "skybox.frag.spv",
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
                                               &pipelines.skybox);
    if (res != vk::Result::eSuccess) {
        throw bad_vulkan_alloc((int)res, "createGraphicsPipelines");
    }

    // Cube map reflect pipeline
    shaderStages[0] = loadShader(filepath + "reflect.vert.spv",
                                vk::ShaderStageFlagBits::eVertex);
    shaderStages[1] = loadShader(filepath + "reflect.frag.spv",
                                vk::ShaderStageFlagBits::eFragment);

    // Enable depth test and write
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthTestEnable = VK_TRUE;
    // Flip cull mode
    rasterizationState.cullMode = vk::CullModeFlagBits::eFront;

    res = vkctx.device.createGraphicsPipelines(vkctx.pipelineCache, 1,
                                               &pipelineCreateInfo, nullptr,
                                               &pipelines.reflect);
    if (res != vk::Result::eSuccess) {
        throw bad_vulkan_alloc((int)res, "createGraphicsPipelines");
    }
}

// Prepare and initialize uniform buffer containing shader uniforms
void
TextureCubemap::prepareUniformBuffers()
{
    // 3D objact
    vkctx.createBuffer(
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        sizeof(ubo),
        nullptr,
        &uniformData.object.buffer,
        &uniformData.object.memory,
        &uniformData.object.descriptor);

    // Skybox
    vkctx.createBuffer(
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        sizeof(ubo),
        nullptr,
        &uniformData.skybox.buffer,
        &uniformData.skybox.memory,
        &uniformData.skybox.descriptor);

    updateUniformBuffers();
}

void
TextureCubemap::updateUniformBuffers()
{
    // 3D object
    glm::mat4 viewMatrix = glm::mat4();
    ubo.projection = glm::perspective(glm::radians(60.0f),
                                      (float)w_width / (float)w_height,
                                      0.001f, 256.0f);
    viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, 0.0f, zoom));

    ubo.modelView = glm::mat4();
    ubo.modelView = viewMatrix * glm::translate(ubo.modelView, cameraPos);
    ubo.modelView = glm::rotate(ubo.modelView, glm::radians(rotation.x),
                                glm::vec3(1.0f, 0.0f, 0.0f));
    ubo.modelView = glm::rotate(ubo.modelView, glm::radians(rotation.y),
                                glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.modelView = glm::rotate(ubo.modelView, glm::radians(rotation.z),
                                glm::vec3(0.0f, 0.0f, 1.0f));
    // Do the inverse here because doing it in every fragment is a bit much.
    // Also MetalSL does not have inverse() and does not support passing
    // transforms between stages.
    ubo.invModelView = glm::inverse(ubo.modelView);

    uint8_t *pData;
    VK_CHECK_RESULT(vkMapMemory(vkctx.device, uniformData.object.memory, 0, sizeof(ubo), 0, (void **)&pData));
    memcpy(pData, &ubo, sizeof(ubo));
    vkUnmapMemory(vkctx.device, uniformData.object.memory);

    // Skybox
    // Remove translation from modelView so the skybox doesn't move.
    ubo.modelView = glm::mat4(glm::mat3(ubo.modelView));
    // Inverse not needed by skybox.

    VK_CHECK_RESULT(vkMapMemory(vkctx.device, uniformData.skybox.memory, 0, sizeof(ubo), 0, (void **)&pData));
    memcpy(pData, &ubo, sizeof(ubo));
    vkUnmapMemory(vkctx.device, uniformData.skybox.memory);
}

void
TextureCubemap::prepareSamplerAndView()
{
    // Create sampler.
    vk::SamplerCreateInfo samplerInfo;
    // Set the non-default values
    samplerInfo.magFilter = vk::Filter::eLinear;
    samplerInfo.minFilter = vk::Filter::eLinear;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.maxLod = (float)cubeMap.levelCount;
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
    viewInfo.image = cubeMap.image;
    viewInfo.format = static_cast<vk::Format>(cubeMap.imageFormat);
    viewInfo.viewType = static_cast<vk::ImageViewType>(cubeMap.viewType);
    viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    viewInfo.subresourceRange.layerCount = cubeMap.layerCount;
    viewInfo.subresourceRange.levelCount = cubeMap.levelCount;
    imageView = vkctx.device.createImageView(viewInfo);
}

void
TextureCubemap::prepare()
{
    prepareSamplerAndView();
    loadMeshes();
    setupVertexDescriptions();
    prepareUniformBuffers();
    setupDescriptorSetLayout();
    preparePipelines();
    setupDescriptorPool();
    setupDescriptorSets();
    vkctx.createDrawCommandBuffers();
    buildCommandBuffers();
}

void
TextureCubemap::toggleSkyBox()
{
    displaySkybox = !displaySkybox;
    rebuildCommandBuffers();
}

void
TextureCubemap::toggleObject()
{
    meshes.objectIndex++;
    if (meshes.objectIndex >= static_cast<uint32_t>(meshes.objects.size()))
    {
        meshes.objectIndex = 0;
    }
    rebuildCommandBuffers();
}

void
TextureCubemap::changeLodBias(float delta)
{
    ubo.lodBias += delta;
    if (ubo.lodBias < 0.0f)
    {
        ubo.lodBias = 0.0f;
    }
    if (ubo.lodBias > cubeMap.levelCount)
    {
        ubo.lodBias = (float)cubeMap.levelCount;
    }
    updateUniformBuffers();
    //updateTextOverlay();
}

void
TextureCubemap::keyPressed(uint32_t keyCode)
{
    switch (keyCode)
    {
    case 's':
        toggleSkyBox();
        break;
    case ' ':
        toggleObject();
        break;
    case SDLK_KP_PLUS:
        changeLodBias(0.1f);
        break;
    case SDLK_KP_MINUS:
        changeLodBias(-0.1f);
        break;
    }
}

void
TextureCubemap::getOverlayText(VulkanTextOverlay *textOverlay, float yOffset)
{
    std::stringstream ss;
    ss << std::setprecision(2) << std::fixed << ubo.lodBias;
    textOverlay->addText("Press \"s\" to toggle skybox", 5.0f,
                         yOffset, VulkanTextOverlay::alignLeft);
    textOverlay->addText("Press \"space\" to change object", 5.0f,
                         yOffset+20.0f, VulkanTextOverlay::alignLeft);
    textOverlay->addText("LOD bias: " + ss.str() + " (numpad +/- to change)",
                         5.0f, yOffset+40.0f, VulkanTextOverlay::alignLeft);
}

const char*
TextureCubemap::customizeTitle(const char* const baseTitle)
{
    if (transcoded) {
        this->title = baseTitle;
        this->title += " Transcoded to ";
        this->title += vkFormatString((VkFormat)transcodedFormat);
        return this->title.c_str();
    }
    return baseTitle;
}


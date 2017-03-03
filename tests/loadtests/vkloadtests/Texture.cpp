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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <exception>
#include <vector>

#include <ktxvulkan.h>
#include "Texture.h"

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION false

// Vertex layout for this example
struct Vertex {
	float pos[3];
	float uv[2];
	float normal[3];
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
        : VulkanLoadTestSample(vkctx, width, height, szArgs, sBasePath)
{
    zoom = -2.5f;
    rotation = { 0.0f, 15.0f, 0.0f };

    ktxVulkanDeviceInfo kvdi;
    ktxVulkanDeviceInfo_init(&kvdi, vkctx.gpu, vkctx.device,
                             vkctx.queue, vkctx.commandPool);


    uint8_t* pKvData;
    uint32_t  kvDataLen;
    KTX_error_code ktxresult;
    ktxresult = ktxLoadVkTextureN(&kvdi,
                          (getAssetPath() + szArgs).c_str(),
                          &texture, &kvDataLen, &pKvData);
    ktxVulkanDeviceInfo_deinit(&kvdi);
    if (KTX_SUCCESS != ktxresult) {
        std::stringstream message;

        message << "Load of texture \"" << getAssetPath() << szArgs
        		<< "\" failed: " << ktxErrorString(ktxresult);
        throw std::runtime_error(message.str());
    }

    KTX_hash_table kvtable;
    ktxresult = ktxHashTable_Deserialize(kvDataLen, pKvData, &kvtable);
    if (KTX_SUCCESS == ktxresult) {
        char* pValue;
        uint32_t valueLen;

        if (KTX_SUCCESS == ktxHashTable_FindValue(kvtable, KTX_ORIENTATION_KEY,
                                                  &valueLen, (void**)&pValue))
        {
            char s, t;

            if (sscanf(pValue, /*valueLen,*/ KTX_ORIENTATION2_FMT, &s, &t) == 2) {
                if (s == 'l') sign_s = -1;
                if (t == 'u') sign_t = -1;
            }
        }
        ktxHashTable_Destroy(kvtable);
        free(pKvData);
    }

    try {
    	prepare();
    } catch (std::exception& e) {
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
Texture::run(uint32_t msTicks)
{
    // Nothing to do since the scene is not animated.
    // VulkanLoadTests base class redraws from the command buffer we built.
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

	destroyTextureImage();

	if (pipelines.solid)
		vkctx.device.destroyPipeline(pipelines.solid);
	if (pipelineLayout)
		vkctx.device.destroyPipelineLayout(pipelineLayout);
	if (descriptorSetLayout)
		vkctx.device.destroyDescriptorSetLayout(descriptorSetLayout);

	vkctx.destroyDrawCommandBuffers();
	quad.freeResources(vkctx.device);
	uniformDataVS.freeResources(vkctx.device);
}

// Free staging resources used while creating a texture
void
Texture::destroyTextureImage()
{
    vkDestroyImageView(vkctx.device, texture.view, NULL);
    vkDestroyImage(vkctx.device, texture.image, NULL);
    vkDestroySampler(vkctx.device, texture.sampler, NULL);
    vkFreeMemory(vkctx.device, texture.deviceMemory, NULL);
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

    for (int32_t i = 0; i < vkctx.drawCmdBuffers.size(); ++i)
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

        vkCmdBindDescriptorSets(vkctx.drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
        		pipelineLayout, 0, 1, &static_cast<const VkDescriptorSet&>(descriptorSet), 0, NULL);
        vkCmdBindPipeline(vkctx.drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.solid);

        VkDeviceSize offsets[1] = { 0 };
        vkCmdBindVertexBuffers(vkctx.drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID,
        		1, &static_cast<const VkBuffer&>(quad.vertices.buf), offsets);
        vkCmdBindIndexBuffer(vkctx.drawCmdBuffers[i], quad.indices.buf, 0, VK_INDEX_TYPE_UINT32);

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
    std::vector<Vertex> vertexBuffer =
    {
        { {  DIM,  DIM, 0.0f }, { 1.0f, 1.0f }, NORMAL },
        { { -DIM,  DIM, 0.0f }, { 0.0f, 1.0f }, NORMAL },
        { { -DIM, -DIM, 0.0f }, { 0.0f, 0.0f }, NORMAL },
        { {  DIM, -DIM, 0.0f }, { 1.0f, 0.0f }, NORMAL }
    };
#undef dim
#undef normal
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
Texture::setupVertexDescriptions()
{
    // Binding description
    vertices.bindingDescriptions.resize(1);
    vertices.bindingDescriptions[0] =
        vk::VertexInputBindingDescription(
            VERTEX_BUFFER_BIND_ID,
            sizeof(Vertex),
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
    vkctx.device.createDescriptorPool(&descriptorPoolInfo, nullptr,
    		                          &descriptorPool);
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

    vkctx.device.createDescriptorSetLayout(&descriptorLayout, nullptr,
    									   &descriptorSetLayout);

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo(
    												{},
													1,
													&descriptorSetLayout);

    vkctx.device.createPipelineLayout(&pipelineLayoutCreateInfo,
    								  nullptr,
									  &pipelineLayout);
}

void
Texture::setupDescriptorSet()
{
    vk::DescriptorSetAllocateInfo allocInfo(
            descriptorPool,
			1,
			&descriptorSetLayout);

    vkctx.device.allocateDescriptorSets(&allocInfo, &descriptorSet);

    // Image descriptor for the color map texture
    vk::DescriptorImageInfo texDescriptor(
            texture.sampler,
            texture.view,
            vk::ImageLayout::eGeneral);

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
			vk::PrimitiveTopology::eTriangleList);

    vk::PipelineRasterizationStateCreateInfo rasterizationState;
    rasterizationState.depthClampEnable = VK_TRUE;
    rasterizationState.rasterizerDiscardEnable = VK_FALSE;
    rasterizationState.polygonMode = vk::PolygonMode::eFill;
    rasterizationState.cullMode = vk::CullModeFlagBits::eNone;
    rasterizationState.frontFace = vk::FrontFace::eCounterClockwise;
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
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
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
    std::string filepath = getAssetPath() + "shaders/";
    shaderStages[0] = loadShader(filepath + "texture.vert.spv",
    							vk::ShaderStageFlagBits::eVertex);
    shaderStages[1] = loadShader(filepath + "texture.frag.spv",
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
    pipelineCreateInfo.stageCount = shaderStages.size();
    pipelineCreateInfo.pStages = shaderStages.data();

    vkctx.device.createGraphicsPipelines(vkctx.pipelineCache, 1,
    		                             &pipelineCreateInfo, nullptr,
										 &pipelines.solid);
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

    uboVS.viewPos = glm::vec4(0.0f, 0.0f, -zoom, 0.0f);

    uint8_t *pData;
    VK_CHECK_RESULT(vkMapMemory(vkctx.device, uniformDataVS.memory, 0, sizeof(uboVS), 0, (void **)&pData));
    memcpy(pData, &uboVS, sizeof(uboVS));
    vkUnmapMemory(vkctx.device, uniformDataVS.memory);
}

void
Texture::prepare()
{
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
    if (uboVS.lodBias > texture.mipLevels)
    {
        uboVS.lodBias = texture.mipLevels;
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
Texture::getOverlayText(VulkanTextOverlay *textOverlay)
{
    std::stringstream ss;
    ss << std::setprecision(2) << std::fixed << uboVS.lodBias;
    textOverlay->addText("LOD bias: " + ss.str() + " (numpad +/- to change)",
                         5.0f, 85.0f, VulkanTextOverlay::alignLeft);
}



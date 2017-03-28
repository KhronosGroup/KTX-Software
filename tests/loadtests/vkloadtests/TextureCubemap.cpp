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

#include <vulkan/vulkan.h>
#include "TextureCubemap.h"

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION false

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
    return new TextureCubemap(vkctx, width, height, szArgs, sBasePath);
}

TextureCubemap::TextureCubemap(VulkanContext& vkctx,
                 uint32_t width, uint32_t height,
				 const char* const szArgs, const std::string sBasePath)
        : VulkanLoadTestSample(vkctx, width, height, szArgs, sBasePath)
{
    zoom = -4.0f;
    rotationSpeed = 0.25f;
    rotation = { -7.25f, 120.0f, 0.0f };

    ktxVulkanDeviceInfo kvdi;
    ktxVulkanDeviceInfo_construct(&kvdi, vkctx.gpu, vkctx.device,
                                  vkctx.queue, vkctx.commandPool, nullptr);
    KTX_error_code ktxresult;
    ktxresult = ktxLoadVkTextureN(&kvdi,
                          (getAssetPath() + szArgs).c_str(),
                          &cubeMap, 0, NULL);
    ktxVulkanDeviceInfo_destruct(&kvdi);
    if (ktxresult != KTX_SUCCESS) {
        std::stringstream message;

        message << "Load of texture \"" << getAssetPath() << szArgs
        		<< "\" failed: " << ktxErrorString(ktxresult);
        throw std::runtime_error(message.str());
    }

    try {
    	prepare();
    } catch (std::exception& e) {
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
TextureCubemap::run(uint32_t msTicks)
{
    // Nothing to do since the scene is not animated.
    // VulkanLoadTests base class redraws from the command buffer we built.
}

/* ------------------------------------------------------------------------- */

void
TextureCubemap::cleanup()
{
	// Clean up used Vulkan resources

	// Clean up texture resources
	ktxVulkanTexture_destruct(&cubeMap, vkctx.device, nullptr);

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
    std::string filepath = getAssetPath() + "models/";

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
    vkctx.device.createDescriptorPool(&descriptorPoolInfo, nullptr,
    		                          &descriptorPool);
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
TextureCubemap::setupDescriptorSets()
{
    vk::DescriptorSetAllocateInfo allocInfo(
            descriptorPool,
			1,
			&descriptorSetLayout);

    vkctx.device.allocateDescriptorSets(&allocInfo, &descriptorSets.object);

	// Image descriptor for the cubemap texture
	vk::DescriptorImageInfo cubeMapDescriptor(
			cubeMap.sampler,
			cubeMap.view,
			vk::ImageLayout::eGeneral);

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
    vkctx.device.allocateDescriptorSets(&allocInfo, &descriptorSets.skybox);

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
    rasterizationState.depthClampEnable = VK_TRUE;
    rasterizationState.rasterizerDiscardEnable = VK_FALSE;
    rasterizationState.polygonMode = vk::PolygonMode::eFill;
    rasterizationState.cullMode = vk::CullModeFlagBits::eBack;
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
    std::string filepath = getAssetPath() + "shaders/";
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
    pipelineCreateInfo.stageCount = shaderStages.size();
    pipelineCreateInfo.pStages = shaderStages.data();

    vkctx.device.createGraphicsPipelines(vkctx.pipelineCache, 1,
    		                             &pipelineCreateInfo, nullptr,
										 &pipelines.skybox);

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

    vkctx.device.createGraphicsPipelines(vkctx.pipelineCache, 1,
    									 &pipelineCreateInfo, nullptr,
										 &pipelines.reflect);
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
    ubo.projection = glm::perspective(glm::radians(60.0f), (float)w_width / (float)w_height, 0.001f, 256.0f);
    viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, 0.0f, zoom));

    ubo.modelView = glm::mat4();
    ubo.modelView = viewMatrix * glm::translate(ubo.modelView, cameraPos);
    ubo.modelView = glm::rotate(ubo.modelView, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    ubo.modelView = glm::rotate(ubo.modelView, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.modelView = glm::rotate(ubo.modelView, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    // Do the inverse here because doing it in every fragment is a bit much.
    // Also MetalSL does not have inverse() and does not support passing
    // transforms between stages.
    ubo.invModelView = glm::inverse(ubo.modelView);
    

    uint8_t *pData;
    VK_CHECK_RESULT(vkMapMemory(vkctx.device, uniformData.object.memory, 0, sizeof(ubo), 0, (void **)&pData));
    memcpy(pData, &ubo, sizeof(ubo));
    vkUnmapMemory(vkctx.device, uniformData.object.memory);

    // Skybox
    viewMatrix = glm::mat4();
    ubo.projection = glm::perspective(glm::radians(60.0f), (float)w_width / (float)w_height, 0.001f, 256.0f);

    ubo.modelView = glm::mat4();
    ubo.modelView = viewMatrix * glm::translate(ubo.modelView, glm::vec3(0, 0, 0));
    ubo.modelView = glm::rotate(ubo.modelView, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    ubo.modelView = glm::rotate(ubo.modelView, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.modelView = glm::rotate(ubo.modelView, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    // Inverse not needed by skybox.

    VK_CHECK_RESULT(vkMapMemory(vkctx.device, uniformData.skybox.memory, 0, sizeof(ubo), 0, (void **)&pData));
    memcpy(pData, &ubo, sizeof(ubo));
    vkUnmapMemory(vkctx.device, uniformData.skybox.memory);
}

void
TextureCubemap::prepare()
{
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
    if (ubo.lodBias > cubeMap.mipLevels)
    {
        ubo.lodBias = cubeMap.mipLevels;
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
TextureCubemap::getOverlayText(VulkanTextOverlay *textOverlay)
{
    std::stringstream ss;
    ss << std::setprecision(2) << std::fixed << ubo.lodBias;
    textOverlay->addText("Press \"s\" to toggle skybox", 5.0f, 85.0f, VulkanTextOverlay::alignLeft);
    textOverlay->addText("Press \"space\" to toggle object", 5.0f, 100.0f, VulkanTextOverlay::alignLeft);
    textOverlay->addText("LOD bias: " + ss.str() + " (numpad +/- to change)", 5.0f, 115.0f, VulkanTextOverlay::alignLeft);
}

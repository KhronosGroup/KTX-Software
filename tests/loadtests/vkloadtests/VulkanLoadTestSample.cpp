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
        vkctx.memoryProperties,
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
                                 std::string modname)
{
    vk::PipelineShaderStageCreateInfo shaderStage({}, stage);
    shaderStage.module = vkctx.loadShader(filename);
    shaderStage.pName = modname.c_str();
    assert(shaderStage.module);
    shaderModules.push_back(shaderStage.module);
    return shaderStage;
}

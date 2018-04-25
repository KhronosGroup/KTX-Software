/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

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



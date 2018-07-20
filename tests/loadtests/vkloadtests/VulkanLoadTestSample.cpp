/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2017 Mark Callow.
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



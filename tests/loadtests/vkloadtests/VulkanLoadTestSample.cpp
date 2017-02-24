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

int
VulkanLoadTestSample::doEvent(SDL_Event* event)
{
    switch (event->type) {
      case SDL_MOUSEMOTION:
      {
        SDL_MouseMotionEvent& motion = event->motion;
        if (mouseButtons.left)
        {
            rotation.x += (mousePos.y - (float)motion.y) * 1.25f;
            rotation.y -= (mousePos.x - (float)motion.x) * 1.25f;
            viewChanged();
        }
        if (mouseButtons.right)
        {
            zoom += (mousePos.y - (float)motion.y) * .005f;
            viewChanged();
        }
        if (mouseButtons.middle)
        {
            cameraPos.x -= (mousePos.x - (float)motion.x) * 0.01f;
            cameraPos.y -= (mousePos.y - (float)motion.y) * 0.01f;
            viewChanged();
            mousePos.x = (float)motion.x;
            mousePos.y = (float)motion.y;
        }
        mousePos = glm::vec2((float)motion.x, (float)motion.y);
        return 0;
      }
      case SDL_MOUSEBUTTONDOWN:
        switch (event->button.button) {
          case SDL_BUTTON_LEFT:
            mouseButtons.left = true;
            break;
          case SDL_BUTTON_MIDDLE:
            mouseButtons.middle = true;
            break;
          case SDL_BUTTON_RIGHT:
            mouseButtons.right = true;
            break;
          default:
            return 1;
        }
        return 0;
      case SDL_MOUSEBUTTONUP:
        switch (event->button.button) {
          case SDL_BUTTON_LEFT:
            mouseButtons.left = false;
            break;
          case SDL_BUTTON_MIDDLE:
            mouseButtons.middle = false;
            break;
          case SDL_BUTTON_RIGHT:
            mouseButtons.right = false;
            break;
          default:
            return 1;
        }
        return 0;
      case SDL_KEYUP:
        if (event->key.keysym.sym == 'q')
            quit = true;
        keyPressed(event->key.keysym.sym);
        return 0;
      default:
        break;
    }
    return 1;
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



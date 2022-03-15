/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @class Texture3d
 * @~English
 *
 * @brief Definition of test sample for loading and displaying the slices of a 3d texture.
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

#include "Texture3d.h"
#include "ltexceptions.h"

VulkanLoadTestSample*
Texture3d::create(VulkanContext& vkctx,
                 uint32_t width, uint32_t height,
                 const char* const szArgs, const std::string sBasePath)
{
    return new Texture3d(vkctx, width, height, szArgs, sBasePath);
}

#define INSTANCE_COUNT_CONST_ID 1
#define INSTANCES_DECLARED_IN_SHADER 30

Texture3d::Texture3d(VulkanContext& vkctx,
                 uint32_t width, uint32_t height,
                 const char* const szArgs, const std::string sBasePath)
        : InstancedSampleBase(vkctx, width, height, szArgs, sBasePath)
{
    zoom = -15.0f;

    if (texture.depth == 1) {
        std::stringstream message;

        message << "Texture3d requires a 3d texture.";
        throw std::runtime_error(message.str());
    }

    try {
        prepare("instancing3d.frag.spv", "instancing3d.vert.spv",
                INSTANCE_COUNT_CONST_ID, texture.depth,
                INSTANCES_DECLARED_IN_SHADER);
    } catch (std::exception& e) {
        (void)e; // To quiet unused variable warnings from some compilers.
        //cleanup(); // See explanation in TextureMipmap.cpp
        throw;
    }
}

#if 0
// Addition of extra uniform buffer for the instance count is to work around
// MoltenVK issue #issue 1421:
//     https://github.com/KhronosGroup/MoltenVK/issues/1421.
void
Texture3d::addSubclassDescriptors(DescriptorBindings& descriptorBindings)
{
    descriptorBindings.push_back(vk::DescriptorSetLayoutBinding(
      2, // Binding 2 : uniform buffer for instanceCount value.
      vk::DescriptorType::eUniformBuffer,
      1,
      vk::ShaderStageFlagBits::eVertex
    ));
}
#endif

// Providing instanceCount via a push constant is a workaround for
// MoltenVK issue #1421:
//     https://github.com/KhronosGroup/MoltenVK/issues/1421.
void
Texture3d::addSubclassPushConstantRanges(PushConstantRanges& ranges)
{
    ranges.push_back(vk::PushConstantRange(
      vk::ShaderStageFlagBits::eVertex,
      0, // offset
      sizeof(instanceCount)
    ));
}

void
Texture3d::setSubclassPushConstants(uint32_t bufferIndex)
{
    vkCmdPushConstants(
        vkctx.drawCmdBuffers[bufferIndex],
        pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(instanceCount),
        &instanceCount
    );
}


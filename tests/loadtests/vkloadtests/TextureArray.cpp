/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @class TextureArray
 * @~English
 *
 * @brief Test loading of 2D texture arrays.
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

#include <vulkan/vulkan.h>
#include <ktxvulkan.h>
#include "TextureArray.h"
#include "ltexceptions.h"

VulkanLoadTestSample*
TextureArray::create(VulkanContext& vkctx,
                 uint32_t width, uint32_t height,
                 const char* const szArgs, const std::string sBasePath)
{
    return new TextureArray(vkctx, width, height, szArgs, sBasePath);
}

TextureArray::TextureArray(VulkanContext& vkctx,
                 uint32_t width, uint32_t height,
                 const char* const szArgs, const std::string sBasePath)
        : InstancedSampleBase(vkctx, width, height, szArgs, sBasePath)
{
    zoom = -15.0f;


    if (texture.layerCount == 1) {
        std::stringstream message;

        message << "TextureArray requires an array texture.";
        throw std::runtime_error(message.str());
    }
    instanceCount = texture.layerCount;

    try {
        prepare("instancing.frag.spv", "instancing.vert.spv", 8U);
    } catch (std::exception& e) {
        (void)e; // To quiet unused variable warnings from some compilers.
        cleanup();
        throw;
    }
}



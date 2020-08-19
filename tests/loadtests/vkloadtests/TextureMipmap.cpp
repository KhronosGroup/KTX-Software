/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @class TextureMipmap
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

#include <assert.h>
#include <algorithm>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>

#include <vulkan/vulkan.h>
#include <ktxvulkan.h>
#include "TextureMipmap.h"
#include "ltexceptions.h"

VulkanLoadTestSample*
TextureMipmap::create(VulkanContext& vkctx,
                 uint32_t width, uint32_t height,
                 const char* const szArgs, const std::string sBasePath)
{
    return new TextureMipmap(vkctx, width, height, szArgs, sBasePath);
}

TextureMipmap::TextureMipmap(VulkanContext& vkctx,
                 uint32_t width, uint32_t height,
                 const char* const szArgs, const std::string sBasePath)
        : InstancedSampleBase(vkctx, width, height, szArgs, sBasePath)
{
    zoom = -18.0f;

    if (texture.levelCount == 1) {
        std::stringstream message;

        message << "TextureMipmap requires a mipmapped texture.";
        throw std::runtime_error(message.str());
    }
    instanceCount = texture.levelCount;

    try {
        prepare("instancinglod.frag.spv", "instancinglod.vert.spv", 20U);
    } catch (std::exception& e) {
        (void)e; // To quiet unused variable warnings from some compilers.
        cleanup();
        throw;
    }
}



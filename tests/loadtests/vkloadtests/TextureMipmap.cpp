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
 * @brief Definition of test sample for loading and displaying all the levels of a 2D mipmapped texture.
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

#include "TextureMipmap.h"
#include "ltexceptions.h"

#include <ktxvulkan.h>

VulkanLoadTestSample*
TextureMipmap::create(VulkanContext& vkctx,
                 uint32_t width, uint32_t height,
                 const char* const szArgs, const std::string sBasePath)
{
    return new TextureMipmap(vkctx, width, height, szArgs, sBasePath);
}

#define INSTANCE_COUNT_CONST_ID 1
#define INSTANCES_DECLARED_IN_SHADER 16

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

    try {
        prepare("instancinglod.frag.spv", "instancinglod.vert.spv",
                INSTANCE_COUNT_CONST_ID, texture.levelCount,
                INSTANCES_DECLARED_IN_SHADER);

    } catch (std::exception& e) {
        (void)e; // To quiet unused variable warnings from some compilers.
        // For reasons I don't understand ~InstancedSampleBase is called
        // during the throw before the exception is caught. ~InstancedSampleBase
        // also calls cleanup(). In Texture, an identically structured class
        // which does not have a base class between it and LoadTestSample, the
        // destructor is not called during throw.
        //cleanup();
        throw;
    }
}

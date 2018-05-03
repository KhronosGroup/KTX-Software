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

        cleanup();
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



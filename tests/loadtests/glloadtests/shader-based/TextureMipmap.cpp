/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2018-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file TextureMipmap.cpp
 * @~English
 *
 * @brief Definition of test sample for loading and displaying all the levels of a 2D mipmapped texture.
 *
 * @author Mark Callow, www.edgewise-consulting.com.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <algorithm>
#include <time.h>
#include <sstream>
#include <vector>
#include <ktx.h>

#include "argparser.h"
#include "TextureMipmap.h"
#include "ltexceptions.h"

#define member_size(type, member) sizeof(((type *)0)->member)

const GLchar* pszLodFsDeclarations =
    "precision mediump float;\n"

    "in vec2 UV;\n"
    "flat in float lambda;\n\n"

    "layout (location = 0) out vec4 outFragColor;\n\n"

    "uniform mediump sampler2D uSampler;\n\n";

const GLchar* pszLodFsMain =
   "void main()\n"
    "{\n"
    "    outFragColor = textureLod(uSampler, UV, lambda);\n"
    "}";

const GLchar* pszLodSrgbEncodeFsMain =
    "void main()\n"
    "{\n"
    "    vec4 t_color = textureLod(uSampler, UV, lambda);\n"
    "    outFragColor.rgb = srgb_encode(t_color.rgb);\n"
    "    outFragColor.a = t_color.a;\n"
    "}";

const GLchar* pszLodVsMain =
    "out vec2 UV;\n"
    "flat out float lambda;\n\n"

    "void main()\n"
    "{\n"
    "    UV = inUV;\n"
    "    lambda = gl_InstanceID + 0.5;\n"
    "    mat4 modelView = ubo.view * ubo.instance[gl_InstanceID].model;\n"
    "    gl_Position = ubo.projection * modelView * inPos;\n"
    "}";

/* ------------------------------------------------------------------------- */

LoadTestSample*
TextureMipmap::create(uint32_t width, uint32_t height,
                     const char* const szArgs, const std::string sBasePath)
{
    return new TextureMipmap(width, height, szArgs, sBasePath);
}

/**
 * @internal
 * @class TextureMipmap
 * @~English
 *
 * @brief Test loading of 2D texture arrays.
 */
TextureMipmap::TextureMipmap(uint32_t width, uint32_t height,
                           const char* const szArgs,
                           const std::string sBasePath)
        : InstancedSampleBase(width, height, szArgs, sBasePath)
{
    zoom = -15.0f;
    if (texTarget != GL_TEXTURE_2D || textureInfo.numLevels == 1) {
        std::stringstream message;
        
        message << "TextureMipmap requires a 2D mipmapped texture.";
        throw std::runtime_error(message.str());
    }

    // Checking if KVData contains keys of interest would go here.

    instanceCount = textureInfo.numLevels;

    InstancedSampleBase::ShaderSource fs;
    InstancedSampleBase::ShaderSource vs;

    fs.push_back(pszLodFsDeclarations);
    if (framebufferColorEncoding() == GL_LINEAR) {
        fs.push_back(pszSrgbEncodeFunc);
        fs.push_back(pszLodSrgbEncodeFsMain);
    } else {
        fs.push_back(pszLodFsMain);;
    }
    vs.push_back(pszInstancingVsDeclarations);
    vs.push_back(pszLodVsMain);

    try {
        prepare(fs, vs);
    } catch (std::exception& e) {
        (void)e; // To quiet unused variable warnings from some compilers.
        cleanup();
        throw;
    }
    bInitialized = true;
}

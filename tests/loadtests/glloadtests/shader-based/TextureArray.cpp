/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2018-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file
 * @~English
 *
 * @brief Definition of test sample for loading and displaying the layers of a 2D array texture.
 *
 * @author Mark Callow, github.com/MarkCallow.
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
#include "TextureArray.h"
#include "ltexceptions.h"

#define member_size(type, member) sizeof(((type *)0)->member)

const GLchar* pszFsArraySamplerDeclaration =
    "uniform mediump sampler2DArray uSampler;\n\n";

const GLchar* pszArrayVsMain =
    "void main()\n"
    "{\n"
    "    UVW = vec3(inUV, gl_InstanceID);\n"
    "    mat4 modelView = ubo.view * ubo.instance[gl_InstanceID].model;\n"
    "    gl_Position = ubo.projection * modelView * inPos;\n"
    "}";

/* ------------------------------------------------------------------------- */

LoadTestSample*
TextureArray::create(uint32_t width, uint32_t height,
                     const char* const szArgs, const std::string sBasePath)
{
    return new TextureArray(width, height, szArgs, sBasePath);
}

/**
 * @internal
 * @class TextureArray
 * @~English
 *
 * @brief Test loading of 2D texture arrays.
 */
TextureArray::TextureArray(uint32_t width, uint32_t height,
                           const char* const szArgs,
                           const std::string sBasePath)
        : InstancedSampleBase(width, height, szArgs, sBasePath)
{
    zoom = -15.0f;
    if (texTarget != GL_TEXTURE_2D_ARRAY) {
        std::stringstream message;
        
        message << "TextureArray requires an array texture.";
        throw std::runtime_error(message.str());
    }

    // Checking if KVData contains keys of interest would go here.

    instanceCount = textureInfo.numLayers;
    InstancedSampleBase::ShaderSource fs;
    InstancedSampleBase::ShaderSource vs;

    fs.push_back(pszInstancingFsDeclarations);
    fs.push_back(pszFsArraySamplerDeclaration);
    if (framebufferColorEncoding() == GL_LINEAR) {
        fs.push_back(pszSrgbEncodeFunc);
        fs.push_back(pszInstancingSrgbEncodeFsMain);
    } else {
        fs.push_back(pszInstancingFsMain);;
    }
    vs.push_back(pszInstancingVsDeclarations);
    vs.push_back(pszArrayVsMain);

    try {
        prepare(fs, vs);
    } catch (std::exception& e) {
        (void)e; // To quiet unused variable warnings from some compilers.
        cleanup();
        throw;
    }
    bInitialized = true;
}

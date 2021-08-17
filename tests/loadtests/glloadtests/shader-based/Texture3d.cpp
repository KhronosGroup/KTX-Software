/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2021 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file Texture3d.cpp
 * @~English
 *
 * @brief Definition of test sample for loading and displaying the slices of a 3d texture.
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
#include "Texture3d.h"
#include "ltexceptions.h"

#define member_size(type, member) sizeof(((type *)0)->member)

const GLchar* pszFsSampler3dDeclaration =
    "uniform mediump sampler3D uSampler;\n\n";

const GLchar* psz3dVsMain =
    "void main()\n"
    "{\n"
    "    UVW = vec3(inUV, float(gl_InstanceID) / float(INSTANCE_COUNT - 1U));\n"
    "    mat4 modelView = ubo.view * ubo.instance[gl_InstanceID].model;\n"
    "    gl_Position = ubo.projection * modelView * inPos;\n"
    "}";

/* ------------------------------------------------------------------------- */

LoadTestSample*
Texture3d::create(uint32_t width, uint32_t height,
                     const char* const szArgs, const std::string sBasePath)
{
    return new Texture3d(width, height, szArgs, sBasePath);
}

/**
 * @internal
 * @class Texture3d
 * @~English
 *
 * @brief Test loading of 2D texture arrays.
 */
Texture3d::Texture3d(uint32_t width, uint32_t height,
                           const char* const szArgs,
                           const std::string sBasePath)
        : InstancedSampleBase(width, height, szArgs, sBasePath)
{
    zoom = -15.0f;
    if (texTarget != GL_TEXTURE_3D) {
        std::stringstream message;
        
        message << "Texture3d requires an 3d texture.";
        throw std::runtime_error(message.str());
    }

    // Checking if KVData contains keys of interest would go here.

    instanceCount = textureInfo.baseDepth;

    InstancedSampleBase::ShaderSource fs;
    InstancedSampleBase::ShaderSource vs;

    fs.push_back(pszInstancingFsDeclarations);
    fs.push_back(pszFsSampler3dDeclaration);
    if (framebufferColorEncoding() == GL_LINEAR) {
        fs.push_back(pszSrgbEncodeFunc);
        fs.push_back(pszInstancingSrgbEncodeFsMain);
    } else {
        fs.push_back(pszInstancingFsMain);;
    }
    vs.push_back(pszInstancingVsDeclarations);
    vs.push_back(psz3dVsMain);

    try {
        prepare(fs, vs);
    } catch (std::exception& e) {
        (void)e; // To quiet unused variable warnings from some compilers.
        cleanup();
        throw;
    }

    // Texture was bound by prepare()
    // Set this so it is easier to recognize that the texture has the expected
    // slices.
    glTexParameteri(texTarget, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    bInitialized = true;
}

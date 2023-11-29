/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2018-2021 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file InstancedSampleBase.cpp
 * @~English
 *
 * @brief Base for samplesusing instancing such as array texture display.
 *
 * @author Mark Callow, www.edgewise-consulting.com.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <algorithm>
#include <time.h>
#include <sstream>
#include <vector>
#include <ktx.h>

#include "argparser.h"
#include "InstancedSampleBase.h"
#include "TranscodeTargetStrToFmt.h"
#include "GLTextureTranscoder.hpp"
#include "ltexceptions.h"

#define member_size(type, member) sizeof(((type *)0)->member)

using namespace std;

const GLchar* InstancedSampleBase::pszInstancingFsDeclarations =
    "precision mediump float;\n"

    "in vec3 UVW;\n\n"

    "layout (location = 0) out vec4 outFragColor;\n\n";

const GLchar* InstancedSampleBase::pszSrgbEncodeFunc =
    "vec3 srgb_encode(vec3 color) {\n"
    "   float r = color.r < 0.0031308 ? 12.92 * color.r : 1.055 * pow(color.r, 1.0/2.4) - 0.055;\n"
    "   float g = color.g < 0.0031308 ? 12.92 * color.g : 1.055 * pow(color.g, 1.0/2.4) - 0.055;\n"
    "   float b = color.b < 0.0031308 ? 12.92 * color.b : 1.055 * pow(color.b, 1.0/2.4) - 0.055;\n"
    "   return vec3(r, g, b);\n"
    "}\n\n";

const GLchar* InstancedSampleBase::pszInstancingFsMain =
   "void main()\n"
    "{\n"
    "    outFragColor = texture(uSampler, UVW);\n"
    "}";

const GLchar* InstancedSampleBase::pszInstancingSrgbEncodeFsMain =
    "void main()\n"
    "{\n"
    "    vec4 t_color = texture(uSampler, UVW);\n"
    "    outFragColor.rgb = srgb_encode(t_color.rgb);\n"
    "    outFragColor.a = t_color.a;\n"
    "}";

const GLchar* InstancedSampleBase::pszInstancingVsDeclarations =
    "layout (location = 0) in vec4 inPos;\n"
    "layout (location = 1) in vec2 inUV;\n\n"

    "struct Instance\n"
    "{\n"
    "    mat4 model;\n"
    "};\n\n"

    "//layout (binding = 0) uniform UBO\n"
    "layout(std140) uniform UBO\n"
    "{\n"
    "    mat4 projection;\n"
    "    mat4 view;\n"
    "    Instance instance[INSTANCE_COUNT];\n"
    "} ubo;\n\n"

    "out vec3 UVW;\n\n";


/* ------------------------------------------------------------------------- */


/**
 * @internal
 * @class InstancedSampleBase
 * @~English
 *
 * @brief Test loading of 2D texture arrays.
 */
InstancedSampleBase::InstancedSampleBase(uint32_t width, uint32_t height,
                           const char* const szArgs,
                           const string sBasePath)
        : GL3LoadTestSample(width, height, szArgs, sBasePath),
          texUnit(GL_TEXTURE0), uniformBufferBindId(0),
          bInitialized(false)
{
    zoom = -15.0f;
    rotationSpeed = 0.25f;
    //rotation = glm::vec3(-15.0f, 35.0f, 0.0f);
    rotation = glm::vec3(15.0f, 35.0f, 0.0f);
    gnTexture = 0;
    // Ensure we're using the desired unit
    glActiveTexture(texUnit);

    processArgs(szArgs);

    KTX_error_code ktxresult;
    ktxTexture* kTexture;
    GLenum glerror;
    string ktxfilepath = externalFile ? ktxfilename
                                           : getAssetPath() + ktxfilename;
    ktxresult =
           ktxTexture_CreateFromNamedFile(ktxfilepath.c_str(),
                                          KTX_TEXTURE_CREATE_NO_FLAGS,
                                          &kTexture);
    if (KTX_SUCCESS != ktxresult) {
        std::stringstream message;
        
        message << "Creation of ktxTexture from \"" << ktxfilepath
                << "\" failed: " << ktxErrorString(ktxresult);
        throw std::runtime_error(message.str());
    }

    if (ktxTexture_NeedsTranscoding(kTexture)) {
        transcodeTarget = KTX_TTF_NOSELECTION;
        TextureTranscoder tc;
        tc.transcode((ktxTexture2*)kTexture, transcodeTarget);
    }

    ktxresult = ktxTexture_GLUpload(kTexture, &gnTexture, &texTarget,
                                    &glerror);
    
    if (KTX_SUCCESS != ktxresult) {
        std::stringstream message;
        
        message << "ktxTexture_GLUpload failed: ";
        if (ktxresult != KTX_GL_ERROR) {
             message << ktxErrorString(ktxresult);
             throw std::runtime_error(message.str());
        } else if (kTexture->isCompressed && glerror == GL_INVALID_ENUM) {
             throw unsupported_ctype();
        } else {
             message << std::showbase << "GL error " << std::hex << glerror
                    << " occurred.";
             throw std::runtime_error(message.str());
        }
    }

    if (kTexture->generateMipmaps) {
        // GLUpload will have generated the mipmaps already.
        uint32_t maxDim = (std::max)(
                    (std::max)(kTexture->baseWidth, kTexture->baseHeight),
                    kTexture->baseDepth);
        textureInfo.numLevels = (uint32_t)floor(log2(maxDim)) + 1;
    } else {
        textureInfo.numLevels = kTexture->numLevels;
    }
    textureInfo.numLayers = kTexture->numLayers;
    textureInfo.baseDepth = kTexture->baseDepth;
    if (textureInfo.numLevels > 1)
        bIsMipmapped = true;
    else
        bIsMipmapped = false;

    // Checking if KVData contains keys of interest would go here.
    
    ktxTexture_Destroy(kTexture);
}

InstancedSampleBase::~InstancedSampleBase()
{
    cleanup();
}

void
InstancedSampleBase::resize(uint32_t width, uint32_t height)
{
    this->w_width = width;
    this->w_height = height;

    glViewport(0, 0, width, height);
    updateUniformBufferMatrices();
}

void
InstancedSampleBase::run(uint32_t /*msTicks*/)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Keep these permanently bound
    //glBindVertexArray(gnVao);
    // Must be done after the VAO is bound
    //glBindBuffer(GL_ARRAY_BUFFER, gnVbo[0]);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gnVbo[1]);
    glDrawElementsInstanced(GL_TRIANGLES, quad.indexCount,
                            GL_UNSIGNED_INT, (GLvoid*)quad.indicesOffset,
                            instanceCount);

    assert(GL_NO_ERROR == glGetError());
}

//===================================================================

void
InstancedSampleBase::processArgs(string sArgs)
{
    // Options descriptor
    struct argparser::option longopts[] = {
      {"external",         argparser::option::no_argument, &externalFile, 1},
      {"transcode-target", argparser::option::required_argument, nullptr, 2},
      {NULL,               argparser::option::no_argument, NULL,          0}
    };

    argvector argv(sArgs);
    argparser ap(argv);

    int ch;
    while ((ch = ap.getopt(nullptr, longopts, nullptr)) != -1) {
        switch (ch) {
            case 0: break;
          case 2:
            transcodeTarget = TranscodeTargetStrToFmt(ap.optarg);
            break;
          default: assert(false); // Error in args in sample table.
        }
    }
    assert(ap.optind < argv.size());
    ktxfilename = argv[ap.optind];
}

/* ------------------------------------------------------------------------- */

void
InstancedSampleBase::cleanup()
{
    glEnable(GL_DITHER);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glDisable(GL_DEPTH_TEST);
    if (bInitialized) {
        glUseProgram(0);
        glDeleteTextures(1, &gnTexture);
        glDeleteProgram(gnInstancingProg);
        glDeleteBuffers(2, quad.gnVbo);
        glDeleteVertexArrays(1, &quad.gnVao);
        delete uboVS.instance;
    }
    assert(GL_NO_ERROR == glGetError());
}

// Setup vertices for a single uv-mapped quad
void
InstancedSampleBase::generateQuad()
{
#define dim 2.5f
    //std::vector<TAVertex> vertices =
    TAVertex vertices[] =
    {
        { {  dim,  dim, 0.0f }, { 1.0f, 1.0f } },
        { { -dim,  dim, 0.0f }, { 0.0f, 1.0f } },
        { { -dim, -dim, 0.0f }, { 0.0f, 0.0f } },
        { {  dim, -dim, 0.0f }, { 1.0f, 0.0f } }
    };
#undef dim

    // Setup indices
    uint32_t indices[] =  { 0,1,2, 2,3,0 };
    quad.indexCount = static_cast<uint32_t>(ARRAY_LEN(indices));
    
    // Create a VAO and bind it.
    glGenVertexArrays(1, &quad.gnVao);
    glBindVertexArray(quad.gnVao);
    
    // Must have vertex data in buffer objects to use VAO's on ES3/GL Core
    glGenBuffers(2, quad.gnVbo);
    glBindBuffer(GL_ARRAY_BUFFER, quad.gnVbo[0]);
    // Must be done after the VAO is bound
    // WebGL requires different buffers for data and indices.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad.gnVbo[1]);

    // Create the buffer data store.
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), NULL, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    GLsizeiptr offset = 0;
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vertices), vertices);
    quad.verticesOffset = offset;
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TAVertex),
                          (GLvoid*)offset);
    offset += member_size(TAVertex, pos);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                          sizeof(TAVertex), (GLvoid*)offset);
    offset = sizeof(vertices);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices),
                 indices, GL_STATIC_DRAW);
    quad.indicesOffset = 0;
}

#define _PAD16(nbytes) (ktx_uint32_t)(16 * ceilf((float)(nbytes) / 16))

void
InstancedSampleBase::prepareUniformBuffers()
{
    uProgramUniforms = glGetUniformBlockIndex(gnInstancingProg, "UBO");
    if (uProgramUniforms == -1) {
        std::stringstream message;
        
        message << "prepareUniformBuffers: UBO not found in program";
        throw std::runtime_error(message.str());
    }

    // INSTANCE_COUNT is set in GLSL code via define set in prepareProgram.
    //
    // Elements of the array of UboInstanceData will be aligned on 16-byte
    // boundaries per the std140 rule for mat4/vec4. _PAD16 is unnecessary
    // right now but will become so if anything is added to the ubo before
    // the UboInstanceData. _PAD16 is put here as a warning.
    uint32_t uboSize = _PAD16(sizeof(uboVS.matrices))
             + instanceCount * sizeof(UboInstanceData);
    uboVS.instance = new UboInstanceData[instanceCount];

    glGenBuffers(1, &gnUbo);
    glBindBuffer(GL_UNIFORM_BUFFER, gnUbo);
    // Create the data store.
    glBufferData(GL_UNIFORM_BUFFER, uboSize, 0, GL_DYNAMIC_DRAW);

    float offset = 1.5f;
    float center = (instanceCount * offset) / 2;
    for (uint32_t i = 0; i < instanceCount; i++)
    {
        // Instance model matrix
        uboVS.instance[i].model
           = glm::translate(glm::mat4(), glm::vec3(0.0f,
                                                   i * offset - center, 0.0f));
        uboVS.instance[i].model = glm::rotate(uboVS.instance[i].model,
                                              glm::radians(120.0f),
                                              glm::vec3(1.0f, 0.0f, 0.0f));
    }

    // Update instanced part of the uniform buffer
    // N.B. See comment re _PAD16 before uboSize above.
    uint32_t dataOffset = _PAD16(sizeof(uboVS.matrices));
    uint32_t dataSize = instanceCount * sizeof(UboInstanceData);
    glBufferSubData(GL_UNIFORM_BUFFER, dataOffset, dataSize, uboVS.instance);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, uniformBufferBindId, gnUbo);

    glUseProgram(gnInstancingProg);
    glUniformBlockBinding(gnInstancingProg, uProgramUniforms,
                          uniformBufferBindId);
    updateUniformBufferMatrices();
    glUseProgram(0);

    assert(glGetError() == GL_NO_ERROR);
}

void
InstancedSampleBase::updateUniformBufferMatrices()
{
    // Only updates the uniform buffer block part containing the global matrices

    // Projection
    uboVS.matrices.projection = glm::perspective(glm::radians(60.0f),
                                                 (float)w_width / w_height,
                                                 .01f, 256.f);

    // View
    uboVS.matrices.view = glm::translate(glm::mat4(),
                                         glm::vec3(0.0f, 1.0f, zoom));
    uboVS.matrices.view *= glm::translate(glm::mat4(), cameraPos);
    uboVS.matrices.view = glm::rotate(uboVS.matrices.view,
                                      glm::radians(rotation.x),
                                      glm::vec3(1.0f, 0.0f, 0.0f));
    uboVS.matrices.view = glm::rotate(uboVS.matrices.view,
                                      glm::radians(rotation.y),
                                      glm::vec3(0.0f, 1.0f, 0.0f));
    uboVS.matrices.view = glm::rotate(uboVS.matrices.view,
                                      glm::radians(rotation.z),
                                      glm::vec3(0.0f, 0.0f, 1.0f));

#if !defined(EMSCRIPTEN)
    // Only update the matrices part of the uniform buffer
    uint8_t *pData = (uint8_t*)glMapBufferRange(GL_UNIFORM_BUFFER, 0,
                                                sizeof(uboVS.matrices),
                                                GL_MAP_WRITE_BIT);
    memcpy(pData, &uboVS.matrices, sizeof(uboVS.matrices));
    glUnmapBuffer(GL_UNIFORM_BUFFER);
#else
    glBufferSubData(GL_UNIFORM_BUFFER, 0,
                    sizeof(uboVS.matrices), &uboVS.matrices);
#endif
}

void
InstancedSampleBase::prepareSampler()
{
    glBindTexture(texTarget, gnTexture);
    if (bIsMipmapped)
        glTexParameteri(texTarget,
                        GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    else
        glTexParameteri(texTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(texTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(texTarget, 0);

    glUseProgram(gnInstancingProg);
    if ((uSampler = glGetUniformLocation(gnInstancingProg,
                                              "uSampler")) == -1) {
        std::stringstream message;
        
        message << "prepareSampler: uSampler not found in program";
        throw std::runtime_error(message.str());
    }
    glUniform1i(uSampler, texUnit - GL_TEXTURE0);
    glUseProgram(0);

}

void
InstancedSampleBase::prepareProgram(ShaderSource& fs, ShaderSource& vs)
{
    GLuint gnInstancingFs, gnInstancingVs;

    try {
        std::stringstream ssDefine;
        ssDefine << "#define INSTANCE_COUNT " << instanceCount << "U" << endl;
        // str().c_str() doesn't work because str goes outof scope immediately.
        // Hence this 2 step process.
        string sDefine = ssDefine.str();
        vs.insert(vs.begin(), sDefine.c_str());
        makeShader(GL_VERTEX_SHADER, vs, &gnInstancingVs);
        makeShader(GL_FRAGMENT_SHADER, fs, &gnInstancingFs);
        makeProgram(gnInstancingVs, gnInstancingFs, &gnInstancingProg);
    } catch (std::exception& e) {
        (void)e; // To quiet unused variable warnings from some compilers.
        throw;
    }
    glDeleteShader(gnInstancingVs);
    glDeleteShader(gnInstancingFs);
}

void
InstancedSampleBase::prepare(ShaderSource& fs, ShaderSource& vs)
{
    // By default dithering is enabled. Dithering does not provide visual
    // improvement in this sample so disable it to improve performance.
    glDisable(GL_DITHER);

    glFrontFace(GL_CW);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f,0.3f,0.4f,1.0f);


    //prepareSamplerAndView();
    //setupVertexDescriptions();
    generateQuad();
    prepareProgram(fs, vs);
    prepareUniformBuffers();
    prepareSampler();

    glUseProgram(gnInstancingProg);
    glBindTexture(texTarget, gnTexture);
}



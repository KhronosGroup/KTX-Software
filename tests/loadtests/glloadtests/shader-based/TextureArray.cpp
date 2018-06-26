/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2018 Mark Callow.
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
 * @file TextureArray.cpp
 * @~English
 *
 * @brief Test loading of 2D texture arrays.
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

#include "TextureArray.h"
#include "ltexceptions.h"

#define member_size(type, member) sizeof(((type *)0)->member)

const GLchar* pszInstancingFs =
    "precision mediump float;\n"

    "uniform mediump sampler2DArray uArraySampler;\n\n"

    "in vec3 UV;\n\n"

    "layout (location = 0) out vec4 outFragColor;\n\n"

    "void main()\n"
    "{\n"
    "    outFragColor = texture(uArraySampler, UV);\n"
    "}";

const GLchar* pszInstancingVs =
    "layout (location = 0) in vec4 inPos;\n"
    "layout (location = 1) in vec2 inUV;\n\n"

    "struct Instance\n"
    "{\n"
    "    mat4 model;\n"
    "    vec4 arrayIndex;\n"
    "};\n\n"

    "//layout (binding = 0) uniform UBO\n"
    "uniform UBO\n"
    "{\n"
    "    mat4 projection;\n"
    "    mat4 view;\n"
    "    Instance instance[8];\n"
    "} ubo;\n\n"

    "out vec3 UV;\n\n"

    "void main()\n"
    "{\n"
    "    UV = vec3(inUV, ubo.instance[gl_InstanceID].arrayIndex.x);\n"
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
        : GL3LoadTestSample(width, height, szArgs, sBasePath),
          arrayTexUnit(GL_TEXTURE0), uniformBufferBindId(0),
          bInitialized(false)
{
    zoom = -15.0f;
    rotationSpeed = 0.25f;
    //rotation = glm::vec3(-15.0f, 35.0f, 0.0f);
    rotation = glm::vec3(15.0f, 35.0f, 0.0f);
    gnArrayTexture = 0;
    // Ensure we're using the desired unit
    glActiveTexture(arrayTexUnit);

    KTX_error_code ktxresult;
    ktxTexture* kTexture;
    GLenum glerror;
    ktxresult =
           ktxTexture_CreateFromNamedFile((getAssetPath() + szArgs).c_str(),
                                           KTX_TEXTURE_CREATE_NO_FLAGS,
                                           &kTexture);
    if (KTX_SUCCESS != ktxresult) {
        std::stringstream message;
        
        message << "Creation of ktxTexture from \"" << getAssetPath() << szArgs
        << "\" failed: " << ktxErrorString(ktxresult);
        throw std::runtime_error(message.str());
    }
    ktxresult = ktxTexture_GLUpload(kTexture, &gnArrayTexture, &arrayTexTarget,
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
    if (arrayTexTarget != GL_TEXTURE_2D_ARRAY) {
        std::stringstream message;
        
        message << "Loaded texture is not a 2D array texture.";
        throw std::runtime_error(message.str());
    }
    
    numLayers = kTexture->numLayers;
    if (numLayers > 1 || kTexture->generateMipmaps)
        // GLUpload will have generated the mipmaps already.
        bIsMipmapped = true;
    else
        bIsMipmapped = false;

    // Checking if KVData contains keys of interest would go here.
    
    ktxTexture_Destroy(kTexture);

    try {
        prepare();
    } catch (std::exception& e) {
        (void)e; // To quiet unused variable warnings from some compilers.
        cleanup();
        throw;
    }
    bInitialized = true;
}

TextureArray::~TextureArray()
{
    cleanup();
}

void
TextureArray::resize(uint32_t width, uint32_t height)
{
    this->w_width = width;
    this->w_height = height;
    updateUniformBufferMatrices();
}

void
TextureArray::run(uint32_t msTicks)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Keep these permanently bound
    //glBindVertexArray(gnVao);
    //glBindBuffer(GL_ARRAY_BUFFER, gnVbo);
    // Must be done after the VAO is bound
    // Use the same buffer for vertex attributes and element indices.
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gnVbo);
    glDrawElementsInstanced(GL_TRIANGLES, quad.indexCount,
                            GL_UNSIGNED_INT, (GLvoid*)quad.indicesOffset,
                            numLayers);

    assert(GL_NO_ERROR == glGetError());
}

/* ------------------------------------------------------------------------- */

void
TextureArray::cleanup()
{
    glEnable(GL_DITHER);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glDisable(GL_DEPTH_TEST);
    if (bInitialized) {
        glUseProgram(0);
        glDeleteTextures(1, &gnArrayTexture);
        glDeleteProgram(gnInstancingProg);
        glDeleteBuffers(1, &quad.gnVbo);
        glDeleteVertexArrays(1, &quad.gnVao);
        if (uboVS.instance)
            delete uboVS.instance;
    }
    assert(GL_NO_ERROR == glGetError());
}

// Setup vertices for a single uv-mapped quad
void
TextureArray::generateQuad()
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
    glGenBuffers(1, &quad.gnVbo);
    glBindBuffer(GL_ARRAY_BUFFER, quad.gnVbo);
    // Must be done after the VAO is bound
    // Use the same buffer for vertex attributes and element indices.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad.gnVbo);

    // Create the buffer data store.
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(vertices) + sizeof(indices),
                 NULL, GL_STATIC_DRAW);

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

    offset = sizeof(vertices);;
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(indices), indices);
    //glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, sizeof(indices), indices);
    quad.indicesOffset = offset;
}

#define LAYERS_DECLARED_IN_SHADER 8U

void
TextureArray::prepareUniformBuffers()
{
    uProgramUniforms = glGetUniformBlockIndex(gnInstancingProg, "UBO");
    if (uProgramUniforms == -1) {
        std::stringstream message;
        
        message << "prepareUniformBuffers: UBO not found in program";
        throw std::runtime_error(message.str());
    }

    uint32_t uboSize = sizeof(uboVS.matrices)
             + LAYERS_DECLARED_IN_SHADER * sizeof(UboInstanceData);
    uboVS.instance = new UboInstanceData[numLayers];

    glGenBuffers(1, &gnUbo);
    glBindBuffer(GL_UNIFORM_BUFFER, gnUbo);
    // Create the data store.
    glBufferData(GL_UNIFORM_BUFFER, uboSize, 0, GL_DYNAMIC_DRAW);

    // Array indices and model matrices are fixed
    // Paren around std::min avoids a SNAFU that windef.h has a "min" macro.
    int32_t maxLayers = (std::min)(numLayers, LAYERS_DECLARED_IN_SHADER);
    float offset = 1.5f;
    float center = (maxLayers * offset) / 2;
    for (int32_t i = 0; i < maxLayers; i++)
    {
        // Instance model matrix
        uboVS.instance[i].model
           = glm::translate(glm::mat4(), glm::vec3(0.0f,
                                                   i * offset - center, 0.0f));
        uboVS.instance[i].model = glm::rotate(uboVS.instance[i].model,
                                              glm::radians(120.0f),
                                              glm::vec3(1.0f, 0.0f, 0.0f));
        // Instance texture array index
        uboVS.instance[i].arrayIndex.x = (float)i;
    }

    // Update instanced part of the uniform buffer
    uint32_t dataOffset = sizeof(uboVS.matrices);
    uint32_t dataSize = numLayers * sizeof(UboInstanceData);
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
TextureArray::updateUniformBufferMatrices()
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

    // Only update the matrices part of the uniform buffer
    uint8_t *pData = (uint8_t*)glMapBufferRange(GL_UNIFORM_BUFFER, 0,
                                                sizeof(uboVS.matrices),
                                                GL_MAP_WRITE_BIT);
    memcpy(pData, &uboVS.matrices, sizeof(uboVS.matrices));
    glUnmapBuffer(GL_UNIFORM_BUFFER);
}

void
TextureArray::prepareSampler()
{
    glBindTexture(arrayTexTarget, gnArrayTexture);
    if (bIsMipmapped)
        glTexParameteri(arrayTexTarget,
                        GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    else
        glTexParameteri(arrayTexTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(arrayTexTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(arrayTexTarget, 0);

    glUseProgram(gnInstancingProg);
    if ((uArraySampler = glGetUniformLocation(gnInstancingProg,
                                              "uArraySampler")) == -1) {
        std::stringstream message;
        
        message << "prepareSampler: uArraySampler not found in program";
        throw std::runtime_error(message.str());
    }
    glUniform1i(uArraySampler, arrayTexUnit - GL_TEXTURE0);
    glUseProgram(0);

}

void
TextureArray::prepareProgram()
{
    GLuint gnInstancingFs, gnInstancingVs;
    try {
        makeShader(GL_VERTEX_SHADER, pszInstancingVs, &gnInstancingVs);
        makeShader(GL_FRAGMENT_SHADER, pszInstancingFs, &gnInstancingFs);
        makeProgram(gnInstancingVs, gnInstancingFs, &gnInstancingProg);
    } catch (std::exception& e) {
        (void)e; // To quiet unused variable warnings from some compilers.
        throw;
    }
    glDeleteShader(gnInstancingVs);
    glDeleteShader(gnInstancingFs);
}

void
TextureArray::prepare()
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
    prepareProgram();
    prepareUniformBuffers();
    prepareSampler();

    glUseProgram(gnInstancingProg);
    glBindTexture(arrayTexTarget, gnArrayTexture);
}



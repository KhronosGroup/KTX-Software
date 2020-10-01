/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2018-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file TextureArray.h
 * @~English
 *
 * @brief Test loading of 2D texture arrays.
 *
 * @author Mark Callow, www.edgewise-consulting.com.
 */

#include <vector>

#include "GL3LoadTestSample.h"

#include <glm/gtc/matrix_transform.hpp>

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION false

class TextureArray : public GL3LoadTestSample
{
  public:
    TextureArray(uint32_t width, uint32_t height,
                 const char* const szArgs,
                 const std::string sBasePath);
    ~TextureArray();

    virtual void resize(uint32_t width, uint32_t height);
    virtual void run(uint32_t msTicks);

    //virtual void getOverlayText(VulkanTextOverlay *textOverlay);

    static LoadTestSample*
    create(uint32_t width, uint32_t height,
           const char* const szArgs, const std::string sBasePath);

  protected:
    const GLuint arrayTexUnit;
    const GLuint uniformBufferBindId;
    GLenum arrayTexTarget;
    GLuint gnArrayTexture;
    GLuint gnInstancingProg;
    GLuint gnUbo;
    
    bool bInitialized;
    bool bIsMipmapped;
    
    uint32_t numLayers;

    // Vertex layout for this example
    struct TAVertex {
        float pos[3];
        float uv[2];
    };
    
    struct MeshBuffer {
        uint32_t indexCount;
        glm::vec3 dim;
        GLuint gnVao;
        GLuint gnVbo[2];
        GLsizeiptr verticesOffset;
        GLsizeiptr indicesOffset;
    };

    MeshBuffer quad;

    struct UboInstanceData {
        // Model matrix
        glm::mat4 model;
        // Texture array index
        // Vec4 due to padding
        glm::vec4 arrayIndex;
    };

    struct {
        // Global matrices
        struct {
            glm::mat4 projection;
            glm::mat4 view;
        } matrices;
        // Separate data for each instance
        UboInstanceData *instance;
    } uboVS;
    
    GLint uProgramUniforms;
    GLint uArraySampler;

    void cleanup();

    // Setup vertices for a single uv-mapped quad
    void generateQuad();

    void prepareUniformBuffers();
    void updateUniformBufferMatrices();
    void prepareSampler();
    void prepareProgram();
    void prepare();

    void processArgs(std::string sArgs);

    virtual void viewChanged()
    {
        updateUniformBufferMatrices();
    }
};

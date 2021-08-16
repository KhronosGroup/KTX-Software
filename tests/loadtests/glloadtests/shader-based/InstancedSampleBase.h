/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2018-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _INSTANCE_SAMPLE_BASE_H_
#define _INSTANCE_SAMPLE_BASE_H_

#include <vector>

#include "GL3LoadTestSample.h"

#include <glm/gtc/matrix_transform.hpp>

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION false

class InstancedSampleBase : public GL3LoadTestSample
{
  public:
    InstancedSampleBase(uint32_t width, uint32_t height,
                 const char* const szArgs,
                 const std::string sBasePath);
    ~InstancedSampleBase();

    virtual void resize(uint32_t width, uint32_t height);
    virtual void run(uint32_t msTicks);

    //virtual void getOverlayText(VulkanTextOverlay *textOverlay);

  protected:
  using ShaderSource = GL3LoadTestSample::ShaderSource;

    const GLuint texUnit;
    const GLuint uniformBufferBindId;
    GLenum texTarget;
    GLuint gnTexture;
    GLuint gnInstancingProg;
    GLuint gnUbo;

    ktx_transcode_fmt_e transcodeTarget;

    bool bInitialized;
    bool bIsMipmapped;

    struct textureInfo {
        uint32_t numLayers;
        uint32_t numLevels;
        uint32_t baseDepth;
    } textureInfo;
    uint32_t instanceCount;

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
    };

    struct {
        // Global matrices
        struct {
            glm::mat4 projection;
            glm::mat4 view;
        } matrices;
        // N.B. The UBO structure declared in the shader has the array of
        // instance data inside the structure rather than pointed at from the
        // structure. The start of the array will be aligned on a 16-byte
        // boundary as it starts with a matrix.
        //
        // Separate data for each instance
        UboInstanceData *instance;
    } uboVS;
    
    GLint uProgramUniforms;
    GLint uSampler;

    static const GLchar* pszInstancingFsDeclarations;
    static const GLchar* pszSrgbEncodeFunc;
    static const GLchar* pszInstancingFsMain;
    static const GLchar* pszInstancingSrgbEncodeFsMain;
    static const GLchar* pszInstancingVsDeclarations;

    void cleanup();

    // Setup vertices for a single uv-mapped quad
    void generateQuad();

    void prepareUniformBuffers();
    void updateUniformBufferMatrices();
    void prepareSampler();
    void prepareProgram(ShaderSource& fs, ShaderSource& vs);
    void prepare(ShaderSource& fs, ShaderSource& vs);

    void processArgs(std::string sArgs);

    virtual void viewChanged()
    {
        updateUniformBufferMatrices();
    }
};

#endif /* _INSTANCE_SAMPLE_BASE_H_ */

/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file TextureCubemap.h
 * @~English
 *
 * @brief Test loading of a cubemap.
 *
 * @author Mark Callow, www.edgewise-consulting.com.
 */

#include <vector>

#include "GL3LoadTestSample.h"

#include <glm/gtc/matrix_transform.hpp>

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION false

class TextureCubemap : public GL3LoadTestSample
{
  public:
    TextureCubemap(uint32_t width, uint32_t height,
                 const char* const szArgs,
                 const std::string sBasePath);
    ~TextureCubemap();

    virtual void resize(uint32_t width, uint32_t height);
    virtual void run(uint32_t msTicks);

    //virtual void getOverlayText(VulkanTextOverlay *textOverlay);

    static LoadTestSample*
    create(uint32_t width, uint32_t height,
           const char* const szArgs, const std::string sBasePath);

  protected:
    const GLuint cubemapTexUnit;
    const GLuint uniformBufferBindId;
    GLuint levelCount = 0;
    GLenum cubemapTexTarget;
    GLuint gnCubemapTexture;
    GLuint gnReflectProg;
    GLuint gnSkyboxProg;
    GLuint gnUbo;
    
    bool bInitialized;
    bool bIsMipmapped;
    bool bDisplaySkybox = true;
    
    uint32_t numLayers;

    // Vertex layout for this example
    struct TAVertex {
        float pos[3];
        float uv[2];
    };
    
    struct {
        glMeshLoader::MeshBuffer skybox;
        std::vector<glMeshLoader::MeshBuffer> objects;
        uint32_t objectIndex = 0;
    } meshes;

    struct {
        // Global matrices
        glm::mat4 projection;
        glm::mat4 modelView;
        glm::mat4 skyboxView;
        glm::mat4 invModelView;
        glm::mat4 uvwTransform;
        float lodBias = 0.0f;
    } ubo;
    
    GLint uReflectProgramUniforms;
    GLint uSkyboxProgramUniforms;
    GLint uReflectCubemap;
    GLint uSkyboxCubemap;

    void cleanup();

    void loadMeshes();

    void prepareUniformBuffers();
    void updateUniformBuffers();
    void prepareSampler();
    void preparePrograms();
    void prepare();

    void toggleSkyBox();
    void toggleObject();
    void changeLodBias(float delta);

    void processArgs(std::string sArgs);

    virtual void keyPressed(uint32_t keyCode);
    virtual void viewChanged()
    {
        updateUniformBuffers();
    }

    GLuint skyboxVAO, skyboxVBO;
};

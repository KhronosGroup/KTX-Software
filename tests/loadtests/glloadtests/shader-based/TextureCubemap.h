/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2020 Mark Callow.
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
    const GLuint uniformBufferBindIds[2];
    GLuint levelCount = 0;
    GLenum cubemapTexTarget;
    GLuint gnCubemapTexture;
    GLuint gnReflectProg;
    GLuint gnSkyboxProg;
    GLuint gnUbo[2];
    
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

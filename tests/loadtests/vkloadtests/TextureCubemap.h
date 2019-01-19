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

#include <vector>

#include <ktxvulkan.h>
#include "VulkanLoadTestSample.h"

#include <glm/gtc/matrix_transform.hpp>

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION false

class TextureCubemap : public VulkanLoadTestSample
{
  public:
    TextureCubemap(VulkanContext& vkctx,
            uint32_t width, uint32_t height,
            const char* const szArgs,
            const std::string sBasePath);
    ~TextureCubemap();

    virtual void resize(uint32_t width, uint32_t height);
    virtual void run(uint32_t msTicks);

    virtual void getOverlayText(VulkanTextOverlay *textOverlay, float yOffset);

    static VulkanLoadTestSample*
    create(VulkanContext& vkctx,
           uint32_t width, uint32_t height,
           const char* const szArgs, const std::string sBasePath);

  protected:
    std::string filename;
    int preloadImages = 0;

    bool displaySkybox = true;

    ktxVulkanTexture cubeMap;
    vk::Sampler sampler;
    vk::ImageView imageView;

    struct {
        vk::PipelineVertexInputStateCreateInfo inputState;
        std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
        std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
    } vertices;

    struct {
        vkMeshLoader::MeshBuffer skybox;
        std::vector<vkMeshLoader::MeshBuffer> objects;
        uint32_t objectIndex = 0;
    } meshes;

    struct {
        UniformData object;
        UniformData skybox;
    } uniformData;
    
    struct {
        glm::mat4 projection;
        glm::mat4 modelView;
        glm::mat4  invModelView;
        float lodBias = 0.0f;
    } ubo;

    struct {
        vk::Pipeline skybox;
        vk::Pipeline reflect;
    } pipelines;

    struct {
        vk::DescriptorSet object;
        vk::DescriptorSet skybox;
    } descriptorSets;

    vk::PipelineLayout pipelineLayout;
    vk::DescriptorSetLayout descriptorSetLayout;
    vk::DescriptorPool descriptorPool;

    void cleanup();

    void rebuildCommandBuffers();
    void buildCommandBuffers();
    void loadMeshes();

    void setupVertexDescriptions();
    void setupDescriptorPool();
    void setupDescriptorSetLayout();
    void setupDescriptorSets();
    void preparePipelines();

    // Prepare and initialize uniform buffer containing shader uniforms
    void prepareUniformBuffers();
    void updateUniformBuffers();
    
    void prepareSamplerAndView();

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
};

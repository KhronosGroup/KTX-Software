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

class Texture : public VulkanLoadTestSample
{
  public:
    Texture(VulkanContext& vkctx,
            uint32_t width, uint32_t height,
            const char* const szArgs,
            const std::string sBasePath);
    ~Texture();

    virtual void resize(uint32_t width, uint32_t height);
    virtual void run(uint32_t msTicks);

    virtual void getOverlayText(VulkanTextOverlay *textOverlay, float yOffset);

    static VulkanLoadTestSample*
    create(VulkanContext& vkctx,
           uint32_t width, uint32_t height,
           const char* const szArgs, const std::string sBasePath);

  protected:
    std::string filename;
    ktxVulkanTexture texture;
    vk::Sampler sampler;
    vk::ImageView imageView;
    vk::ImageTiling tiling;

    struct {
        vk::PipelineVertexInputStateCreateInfo inputState;
        std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
        std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
    } vertices;

    MeshBuffer quad;
    typedef std::array<float, 3> rgbcolor;
    std::array<rgbcolor,4> quadColor;

    UniformData uniformDataVS;

    struct {
        glm::mat4 projection;
        glm::mat4 model;
        glm::mat4 normal;
        glm::vec4 viewPos;
        float lodBias = 0.0f;
    } uboVS;

    struct {
        vk::Pipeline solid;
    } pipelines;

    vk::PipelineLayout pipelineLayout;
    vk::DescriptorSet descriptorSet;
    vk::DescriptorSetLayout descriptorSetLayout;
    vk::DescriptorPool descriptorPool;

    int sign_s = 1;
    int sign_t = 1;

    void cleanup();
    void buildCommandBuffers();
    void generateQuad();
    void setupVertexDescriptions();
    void setupDescriptorPool();
    void setupDescriptorSetLayout();
    void setupDescriptorSet();
    void preparePipelines();
    // Prepare and initialize uniform buffer containing shader uniforms
    void prepareUniformBuffers();
    void updateUniformBuffers();
    void prepareSamplerAndView();
    void prepare();
    
    void processArgs(std::string sArgs);

    virtual void keyPressed(uint32_t keyCode);
    virtual void viewChanged()
    {
        updateUniformBuffers();
    }

    void changeLodBias(float delta);
};



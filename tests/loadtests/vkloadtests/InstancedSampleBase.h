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

#ifndef _INSTANCE_SAMPLE_BASE_H_
#define _INSTANCE_SAMPLE_BASE_H_

#include <vector>

#include <ktxvulkan.h>
#include "VulkanLoadTestSample.h"

#include <glm/gtc/matrix_transform.hpp>

class InstancedSampleBase : public VulkanLoadTestSample
{
  public:
    InstancedSampleBase(VulkanContext& vkctx,
                 uint32_t width, uint32_t height,
                 const char* const szArgs,
                 const std::string sBasePath);
    virtual ~InstancedSampleBase();

    virtual void resize(uint32_t width, uint32_t height);
    virtual void run(uint32_t msTicks);

    //virtual void getOverlayText(VulkanTextOverlay *textOverlay, float yOffset);

  protected:
    std::string filename;
    ktxVulkanTexture texture;
    vk::Sampler sampler;
    vk::ImageView imageView;
    vk::ImageTiling tiling;
    vk::Filter filter;


    uint32_t instanceCount;

    struct {
        vk::PipelineVertexInputStateCreateInfo inputState;
        std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
        std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
    } vertices;

    MeshBuffer quad;

    UniformData uniformDataVS;

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

    struct {
        vk::Pipeline solid;
    } pipelines;

    vk::PipelineLayout pipelineLayout;
    vk::DescriptorSet descriptorSet;
    vk::DescriptorSetLayout descriptorSetLayout;
    vk::DescriptorPool descriptorPool;

    void cleanup();

    void buildCommandBuffers();

    // Setup vertices for a single uv-mapped quad
    void generateQuad();

    void setupVertexDescriptions();
    void setupDescriptorPool();
    void setupDescriptorSetLayout();
    void setupDescriptorSet();
    void preparePipelines(const char* const fragShaderName,
                          const char* const vertShaderName);

    void prepareUniformBuffers(uint32_t shaderDeclaredInstances,
                               uint32_t instanceCount);
    void updateUniformBufferMatrices();

    void prepareSamplerAndView();
    
    void prepare(const char* const fragShaderName,
                 const char* const vertShaderName,
                 uint32_t shaderDeclaredInstances);

    void processArgs(std::string sArgs);

    virtual void viewChanged()
    {
        updateUniformBufferMatrices();
    }
};

#endif /* _INSTANCE_SAMPLE_BASE_H_ */

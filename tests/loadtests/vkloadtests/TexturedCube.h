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

#ifndef TEXTURED_CUBE_H
#define TEXTURED_CUBE_H

#include <vector>

#include "VulkanLoadTestSample.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

class TexturedCube : public VulkanLoadTestSample {
  public:
    TexturedCube(VulkanContext& vkctx,
                 uint32_t width, uint32_t height,
                 const char* const szArgs,
                 const std::string sBasePath);
    virtual ~TexturedCube();

    virtual void resize(uint32_t width, uint32_t height);
    virtual void run(uint32_t msTicks);

    //virtual void getOverlayText(VulkanTextOverlay *textOverlay);

    static VulkanLoadTestSample*
    create(VulkanContext& vkctx,
           uint32_t width, uint32_t height,
           const char* const szArgs, const std::string sBasePath);

  protected:
    void buildCommandBuffer(const int bufferIndex);
    void cleanup();
    void createDescriptorSetLayout();
    void prepareCubeDataBuffers();
    void prepareDescriptorPool();
    void prepareDescriptorSet();
    void preparePipeline();
    void prepareUniformBuffer();
#if defined(INCLUDE_SHADERS)
    VkShaderModule createShaderModule(uint32_t* spv, size_t size);
    VkShaderModule prepareFragShader();
    VkShaderModule prepareVertShader();
#endif
    void setupVertexDescriptions();
    void updateUniformBuffer();

    struct {
        vk::Buffer buf;
        vk::DeviceMemory mem;
        vk::PipelineVertexInputStateCreateInfo inputState;
        std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
        std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
    } vertices;

    struct {
        int count;
        vk::Buffer buf;
        vk::DeviceMemory mem;
    } indices;

    UniformData uniformData;

    struct Uniforms {
        mat4 projection;
        mat4 model;
        mat4 view;
        vec4 viewPos;
        float lodBias;
    } uniforms;

    float zoom;
    vec3 rotation;
    vec3 cameraPos;

#if 0
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 viewPos;
#endif

    const uint32_t numTextures;

    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipelineCache pipelineCache;
    VkPipeline pipeline;
    VkShaderModule vsModule;
    VkShaderModule fsModule;
};

#endif /* TEXTURED_CUBE_H */

/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright (c) 2017, Mark Callow, www.edgewise-consulting.com.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * unaltered in all copies or substantial portions of the Materials.
 * Any additions, deletions, or changes to the original source files
 * must be clearly indicated in accompanying documentation.
 *
 * If only executable code is distributed, then the accompanying
 * documentation must state that "this software is based in part on the
 * work of Mark Callow."
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

#ifndef TEXTURED_CUBE_H
#define TEXTURED_CUBE_H

#include <vector>

#include "VulkanLoadTestSample.h"
//#include "vecmath.hpp"
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

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

/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _INSTANCE_SAMPLE_BASE_H_
#define _INSTANCE_SAMPLE_BASE_H_

#include <vector>

#include "VulkanLoadTestSample.h"

#include <ktxvulkan.h>
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
    virtual const char* customizeTitle(const char* const title);

  protected:
    ktxVulkanTexture texture;
    vk::Sampler sampler;
    vk::ImageView imageView;
    vk::ImageTiling tiling;
    vk::Filter filter;

    uint32_t instanceCount;

    bool transcoded;
    vk::Format transcodedFormat;
    std::string title;

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

    using DescriptorBindings = std::vector<vk::DescriptorSetLayoutBinding>;
    using PushConstantRanges = std::vector<vk::PushConstantRange>;
    virtual void addSubclassDescriptors(DescriptorBindings&) { }
    virtual void addSubclassPushConstantRanges(PushConstantRanges&) { }
    virtual void setSubclassPushConstants(uint32_t) { }
    void setupVertexDescriptions();
    void setupDescriptorPool();
    void setupDescriptorSetLayout();
    void setupDescriptorSet();
    void preparePipelines(const char* const fragShaderName,
                          const char* const vertShaderName,
                          uint32_t instanceCountConstId);

    void prepareUniformBuffers(// See note in prepare declaration.
                               uint32_t shaderDeclaredInstances);
    void updateUniformBufferMatrices();

    void prepareSamplerAndView();
    
    void prepare(const char* const fragShaderName,
                 const char* const vertShaderName,
                 uint32_t instanceCountConstId,
                 uint32_t instanceCount,
                 // Solely because of MoltenVK issue #1420.
                 // It can't specialize array length constants.
                 uint32_t shaderDeclaredInstances);

    void processArgs(std::string sArgs);

    virtual void viewChanged()
    {
        updateUniformBufferMatrices();
    }
};

#endif /* _INSTANCE_SAMPLE_BASE_H_ */

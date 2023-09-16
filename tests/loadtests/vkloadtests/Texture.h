/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <vector>

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
    virtual const char* customizeTitle(const char* const title);

    static VulkanLoadTestSample*
    create(VulkanContext& vkctx,
           uint32_t width, uint32_t height,
           const char* const szArgs, const std::string sBasePath);

  protected:
    enum class UseSuballocator
    {
        No = 0,
        Yes
    };

    std::string filename;
    ktxVulkanTexture texture;
    vk::Sampler sampler;
    vk::ImageView imageView;
    vk::ImageTiling tiling;
    UseSuballocator useSubAlloc;
    vk::ComponentMapping swizzle;

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

    bool transcoded;
    vk::Format transcodedFormat;
    std::string title;

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



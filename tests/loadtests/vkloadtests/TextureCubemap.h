/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <vector>

#include "VulkanLoadTestSample.h"

#include <ktxvulkan.h>
#include <glm/gtc/matrix_transform.hpp>

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION false

class TextureCubemap : public VulkanLoadTestSample
{
  public:
    TextureCubemap(VulkanContext& vkctx,
            uint32_t width, uint32_t height,
            const char* const szArgs,
            const std::string sBasePath, int32_t yflip);
    ~TextureCubemap();

    virtual void resize(uint32_t width, uint32_t height);
    virtual void run(uint32_t msTicks);

    virtual void getOverlayText(VulkanTextOverlay *textOverlay, float yOffset);
    virtual const char* customizeTitle(const char* const title);

    static VulkanLoadTestSample*
    create(VulkanContext& vkctx,
           uint32_t width, uint32_t height,
           const char* const szArgs, const std::string sBasePath);

  protected:
    int preloadImages = 0;

    bool displaySkybox = true;

    bool transcoded;
    vk::Format transcodedFormat;
    std::string title;

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
        glm::mat4 invModelView;
        glm::mat4 uvwTransform;
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

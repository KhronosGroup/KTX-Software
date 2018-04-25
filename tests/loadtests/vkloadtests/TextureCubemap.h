/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

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

    virtual void getOverlayText(VulkanTextOverlay *textOverlay);

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

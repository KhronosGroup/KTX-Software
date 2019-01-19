/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

#ifndef VULKAN_LOAD_TEST_SAMPLE_H
#define VULKAN_LOAD_TEST_SAMPLE_H

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

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "LoadTestSample.h"
#include "VulkanAppSDL.h"
#include "VulkanContext.h"
// MeshLoader needs vulkantools.h to be already included. It is included by
// vulkantextoverlay.hpp via VulkanAppSDL.h.
#include "utils/VulkanMeshLoader.hpp"

#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))

class VulkanLoadTestSample : public LoadTestSample {
  public:
    typedef uint64_t ticks_t;
    VulkanLoadTestSample(VulkanContext& vkctx,
                     uint32_t width, uint32_t height,
                     /* const char* const szArgs, */
                     const std::string sBasePath)
           : LoadTestSample(width, height, /*szArgs,*/ sBasePath, -1),
             vkctx(vkctx),
             defaultClearColor(std::array<float,4>({0.025f, 0.025f, 0.025f, 1.0f}))
    {
    }

    virtual ~VulkanLoadTestSample();
    //virtual int doEvent(SDL_Event* event);
    virtual void resize(uint32_t width, uint32_t height) = 0;
    virtual void run(uint32_t msTicks) = 0;

    virtual void getOverlayText(VulkanTextOverlay *textOverlay,
                                float yoffset) { };

    typedef VulkanLoadTestSample* (*PFN_create)(VulkanContext&,
                                    uint32_t width, uint32_t height,
                                    const char* const szArgs,
                                    const std::string sBasePath);

  protected:
    virtual void keyPressed(uint32_t keyCode) { }
    virtual void viewChanged() { }
    
    vk::PipelineShaderStageCreateInfo
    loadShader(std::string filename,
               vk::ShaderStageFlagBits stage,
               std::string modname = "main");
    void loadMesh(std::string filename,
                  vkMeshLoader::MeshBuffer* meshBuffer,
                  std::vector<vkMeshLoader::VertexLayout> vertexLayout,
                  float scale);

    struct MeshBufferInfo
    {
        vk::Buffer buf;
        vk::DeviceMemory mem;
        vk::DeviceSize size = 0;

        void freeResources(vk::Device& device) {
            if (buf) device.destroyBuffer(buf);
            if (mem) device.freeMemory(mem);
        }
    };

    struct MeshBuffer
    {
        MeshBufferInfo vertices;
        MeshBufferInfo indices;
        uint32_t indexCount;
        glm::vec3 dim;

        void freeResources(vk::Device& device) {
            vertices.freeResources(device);
            indices.freeResources(device);
        }
    };

    struct UniformData
    {
        vk::Buffer buffer;
        vk::DeviceMemory memory;
        vk::DescriptorBufferInfo descriptor;
        uint32_t allocSize;
        void* mapped = nullptr;

        void freeResources(vk::Device& device) {
            device.destroyBuffer(buffer);
            device.freeMemory(memory);
        }
    };

    VulkanContext& vkctx;

    // Saved for clean-up
    std::vector<VkShaderModule> shaderModules;

    const vk::ClearColorValue defaultClearColor;
};

#endif /* VULKAN_LOAD_TEST_SAMPLE_H */

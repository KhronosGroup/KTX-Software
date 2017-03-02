/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

#ifndef VULKAN_LOAD_TEST_SAMPLE_H
#define VULKAN_LOAD_TEST_SAMPLE_H

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

#include "VulkanAppSDL.h"
#include "VulkanContext.h"
// MeshLoader needs vulkantools.h to be already included. It is included by
// vulkantextoverlay.hpp via VulkanAppSDL.h.
#include "utils/VulkanMeshLoader.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))

class VulkanLoadTestSample {
  public:
    typedef uint64_t ticks_t;
    VulkanLoadTestSample(VulkanContext& vkctx,
                     uint32_t width, uint32_t height,
					 const char* const szArgs,
                     const std::string sBasePath)
           : vkctx(vkctx), w_width(width), w_height(height),
             sBasePath(sBasePath),
             defaultClearColor(std::array<float,4>({0.025f, 0.025f, 0.025f, 1.0f}))
    {
    }

    virtual ~VulkanLoadTestSample();
    virtual int doEvent(SDL_Event* event);
    virtual void resize(uint32_t width, uint32_t height) = 0;
    virtual void run(uint32_t msTicks) = 0;

    virtual void getOverlayText(VulkanTextOverlay *textOverlay) { };

    typedef VulkanLoadTestSample* (*PFN_create)(VulkanContext&,
									uint32_t width, uint32_t height,
									const char* const szArgs,
									const std::string sBasePath);

  protected:
    virtual void keyPressed(uint32_t keyCode) { }
    virtual void viewChanged() { }

    const std::string getAssetPath() { return sBasePath; }

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
		size_t size = 0;

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
    //const std::array<float,4> defaultClearColor;

    glm::vec3 rotation = glm::vec3();
    glm::vec3 cameraPos = glm::vec3();
    glm::vec2 mousePos = glm::vec2();
    struct {
        bool left = false;
        bool right = false;
        bool middle = false;
    } mouseButtons;
    bool quit = false;

    float zoom = 0;

    uint32_t w_width;
    uint32_t w_height;

    // Defines a frame rate independent timer value clamped from -1.0...1.0
    // For use in animations, rotations, etc.
    float timer = 0.0f;
    // Multiplier for speeding up (or slowing down) the global timer
    float timerSpeed = 0.25f;

    bool paused = false;

    // Use to adjust mouse rotation speed
    float rotationSpeed = 1.0f;
    // Use to adjust mouse zoom speed
    float zoomSpeed = 1.0f;

    const std::string sBasePath;
};


#endif /* VULKAN_LOAD_TEST_SAMPLE_H */

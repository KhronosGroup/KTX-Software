/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @class TextureArray
 * @~English
 *
 * @brief Test loading of 2D textures.
 *
 * @author Mark Callow, www.edgewise-consulting.com.
 *
 * @par Acknowledgement
 * Thanks to Sascha Willems' - www.saschawillems.de - for VulkanTextOverlay.
 */

#if defined(_WIN32)
  #if _MSC_VER < 1900
    #define snprintf _snprintf
  #endif
  #define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <math.h>

#include <array>
#include <vector>

#include "TexturedCube.h"
#include "cube.h"

#define UNIFORM_OFFSET(f) (size_t)(&((Uniforms *)0)->mvp)

#define VERTEX_BUFFER_FIRST_BINDING_ID 0

VulkanLoadTestSample*
TexturedCube::create(VulkanContext& vkctx,
                                  uint32_t width, uint32_t height,
                                  const char* const szArgs,
                                  const std::string sBasePath)

{
    return new TexturedCube(vkctx, width, height, szArgs, sBasePath);
}


TexturedCube::TexturedCube(VulkanContext& vkctx,
                           uint32_t width, uint32_t height,
                           const char* const /*szArgs*/,
                           const std::string sBasePath)
        : VulkanLoadTestSample(vkctx, width, height, sBasePath),
          numTextures(1)
{
    zoom = 1.0f;
    rotation = { 0.0f, 0.0f, 0.0f };
    uniforms.lodBias = 0.0;

    try {
        prepareUniformBuffer();
        prepareCubeDataBuffers();
        setupVertexDescriptions();
        createDescriptorSetLayout();
        preparePipeline();
        prepareDescriptorPool();
        prepareDescriptorSet();

        vkctx.createDrawCommandBuffers();
        for (uint32_t i = 0; i < vkctx.swapchain.imageCount; i++) {
            buildCommandBuffer(i);
        }
    } catch (std::exception& e) {
        (void)e; // To quiet unused variable warnings from some compilers.
        cleanup();
        throw;
    }
}


TexturedCube::~TexturedCube()
{
    cleanup();
}


void
TexturedCube::resize(uint32_t width, uint32_t height)
{
    w_width = width;
    w_height = height;
    uniforms.projection = perspective(45.f, width / (float)height, 1.f, 100.f);

    vkctx.destroyDrawCommandBuffers();
    vkctx.createDrawCommandBuffers();
    for (uint32_t i = 0; i < vkctx.swapchain.imageCount; i++) {
        buildCommandBuffer(i);
    }
}


void
TexturedCube::run(uint32_t msTicks)
{
    /* Setup the view matrix : just turn around the cube. */
    vec3 up(0.f, 1.f, 0.f);
    const float fDistance = 5.0f;
    vec3 eye((float)cos( msTicks*0.001f ) * fDistance,
             (float)sin( msTicks*0.0007f ) * fDistance,
             (float)sin( msTicks*0.001f ) * fDistance);
    vec3 center = vec3(0.f, 0.f, 0.f);

    uniforms.viewPos = vec4(eye, 1.0);
    uniforms.view = lookAt(eye, center, up);

    updateUniformBuffer();
}

/* ------------------------------------------------------------------------- */

void
TexturedCube::cleanup()
{

}

void
TexturedCube::buildCommandBuffer(const int bufferIndex)
{
    const VkCommandBuffer& cmdBuf = vkctx.drawCmdBuffers[bufferIndex];

    const VkCommandBufferBeginInfo cmd_buf_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL, 0, NULL
    };

#if 0
typedef union VkClearColorValue {
    float       float32[4];
    int32_t     int32[4];
    uint32_t    uint32[4];
} VkClearColorValue;

typedef struct VkClearDepthStencilValue {
    float       depth;
    uint32_t    stencil;
} VkClearDepthStencilValue;

typedef union VkClearValue {
    VkClearColorValue           color;
    VkClearDepthStencilValue    depthStencil;
} VkClearValue;
#endif

    // clang suggested all these braces. I could not find documentation of
    // union initialization except using designated intializers which are not
    // in C++11.
    VkClearValue clear_values[2] = {
      { {{0.0f, 0.2f, 0.2f, 1.0f}} },
      { {{0.0f, 0}} }
    };

    const VkRenderPassBeginInfo rp_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        NULL,
        vkctx.renderPass,
        vkctx.framebuffers[bufferIndex],
        { {0, 0}, {w_width, w_height} },
        ARRAY_LEN(clear_values),
        clear_values,
    };

    VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuf, &cmd_buf_info));

    vkCmdBeginRenderPass(cmdBuf, &rp_begin,
                         VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {
         0,
         0,
         (float)w_width,
         (float)w_height,
         0.0f,
         1.0f
    };
    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

    VkRect2D scissor;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = w_width;
    scissor.extent.height = w_height;
    vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkDeviceSize offsets[1] = { 0 };
    vkCmdBindVertexBuffers(cmdBuf, VERTEX_BUFFER_FIRST_BINDING_ID, 1,
                           &static_cast<const VkBuffer&>(vertices.buf),
                           offsets);
    vkCmdBindIndexBuffer(cmdBuf, indices.buf, 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(cmdBuf, indices.count, 1, 0, 0, 0);

    vkCmdEndRenderPass(vkctx.drawCmdBuffers[bufferIndex]);

    VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuf));
}


void
TexturedCube::prepareUniformBuffer()
{
    vkctx.createBuffer(
        vk::BufferUsageFlagBits::eUniformBuffer,
        sizeof(Uniforms),
        &uniforms,
        &uniformData.buffer,
        &uniformData.memory,
        &uniformData.descriptor);
}


void
TexturedCube::updateUniformBuffer()
{
#if 0 /* Move to run? */
    mat4 viewMatrix = glm::translate(mat4(), vec3(0.0f, 0.0f, zoom));

    uniforms.model = viewMatrix * translate(mat4(), cameraPos);
    uniforms.model = rotate(uniforms.model, rotation.x, vec3(1.0f, 0.0f, 0.0f));
    uniforms.model = rotate(uniforms.model, rotation.y, vec3(0.0f, 1.0f, 0.0f));
    uniforms.model = rotate(uniforms.model, rotation.z, vec3(0.0f, 0.0f, 1.0f));

    uniforms.viewPos = glm::vec4(0.0f, 0.0f, -zoom, 0.0f);
#endif

    uint8_t *pData;
    VK_CHECK_RESULT(vkMapMemory(vkctx.device, uniformData.memory, 0,
                    sizeof(uniforms), 0, (void **)&pData));
    memcpy(pData, &uniforms, sizeof(uniforms));
    vkUnmapMemory(vkctx.device, uniformData.memory);
}


struct Vertex {
    float position[3];
    float normal[3];
    float uv[2];
    float color[4];
};

void
TexturedCube::prepareCubeDataBuffers()
{
#if 1
    std::vector<float> vertexBuffer;

    vertexBuffer.insert(vertexBuffer.end(),
                        &cube_face[0],
                        &cube_face[ARRAY_LEN(cube_face)]);
#if 0
    vertexBuffer.insert(vertexBuffer.end(),
                        &cube_normal[0],
                        &cube_normal[ARRAY_LEN(cube_normal)]);
    vertexBuffer.insert(vertexBuffer.end(),
                        &cube_texture[0],
                        &cube_texture[ARRAY_LEN(cube_texture)]);
    vertexBuffer.insert(vertexBuffer.end(),
                        &cube_color[0],
                        &cube_color[ARRAY_LEN(cube_color)]);
#endif

    vkctx.createBuffer(
        vk::BufferUsageFlagBits::eVertexBuffer,
        vertexBuffer.size() * sizeof(float),
        vertexBuffer.data(),
        &vertices.buf,
        &vertices.mem);
#else
    // It appears to be impossible in Vulkan to have multiple non-interleaved
    // attributes in 1 buffer object. So interleave the data.
    std::vector<Vertex> vertexBuffer;
    uint i, j, k;

    for (i = 0, j = 0, k = 0; i < ARRAY_LEN(cube_face); i++, j++, k++) {
        Vertex vertex;

        vertex.position[0] = cube_face[i];
        vertex.position[1] = cube_face[i+1];
        vertex.position[2] = cube_face[i+2];
        vertex.normal[0] = cube_normal[i];
        vertex.normal[1] = cube_normal[++i];
        vertex.normal[2] = cube_normal[++i];
        vertex.uv[0] = cube_texture[j];
        vertex.uv[1] = cube_texture[++j];
        vertex.color[0] = cube_color[k];
        vertex.color[1] = cube_color[++k];
        vertex.color[2] = cube_color[++k];
        vertex.color[3] = cube_color[++k];
        vertexBuffer.push_back(vertex);
    }

    createBuffer(
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        vertexBuffer.size() * sizeof(Vertex),
        vertexBuffer.data(),
        &vertices.buf,
        &vertices.mem);
#endif

    // Setup indices
    indices.count = ARRAY_LEN(cube_index_buffer);
    vkctx.createBuffer(
        vk::BufferUsageFlagBits::eIndexBuffer,
        sizeof(cube_index_buffer),
        (void*)cube_index_buffer,
        &indices.buf,
        &indices.mem);
}


static VkVertexInputBindingDescription
initVertexInputBindingDescription(uint32_t binding,
                                  uint32_t stride,
                                  VkVertexInputRate inputRate)
{
    VkVertexInputBindingDescription vInputBindDescription = {};
    vInputBindDescription.binding = binding;
    vInputBindDescription.stride = stride;
    vInputBindDescription.inputRate = inputRate;
    return vInputBindDescription;
}


static VkVertexInputAttributeDescription
initVertexInputAttributeDescription(uint32_t binding,
                                    uint32_t location,
                                    VkFormat format,
                                    uint32_t offset)
{
    VkVertexInputAttributeDescription vInputAttribDescription = {};
    vInputAttribDescription.location = location;
    vInputAttribDescription.binding = binding;
    vInputAttribDescription.format = format;
    vInputAttribDescription.offset = offset;
    return vInputAttribDescription;
}


static VkPipelineVertexInputStateCreateInfo
initPipelineVertexInputStateCreateInfo()
{
    VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {};
    pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipelineVertexInputStateCreateInfo.pNext = NULL;
    return pipelineVertexInputStateCreateInfo;
}


void
TexturedCube::setupVertexDescriptions()
{
    // Binding description
    vertices.bindingDescriptions.resize(1);
    vertices.bindingDescriptions[0] =
        initVertexInputBindingDescription(
            VERTEX_BUFFER_FIRST_BINDING_ID,
            /*sizeof(Vertex),*/CUBE_FACE_STRIDE,
            VK_VERTEX_INPUT_RATE_VERTEX);

    // Attribute descriptions
    // Describes memory layout and shader positions
    vertices.attributeDescriptions.resize(1);
    //vertices.attributeDescriptions.resize(4);
    // Location 0 : position
    vertices.attributeDescriptions[0] =
        initVertexInputAttributeDescription(
            VERTEX_BUFFER_FIRST_BINDING_ID,
            0,
            VK_FORMAT_R32G32B32_SFLOAT,
            0);
#if 0
    // Location 1 : vertex normal
    vertices.attributeDescriptions[1] =
        initVertexInputAttributeDescription(
            VERTEX_BUFFER_FIRST_BINDING_ID,
            1,
            VK_FORMAT_R32G32B32_SFLOAT,
            sizeof(float)*3);//sizeof(cube_face));
    // Location 2 : texture coordinates
    vertices.attributeDescriptions[2] =
        initVertexInputAttributeDescription(
            VERTEX_BUFFER_FIRST_BINDING_ID,
            2,
            VK_FORMAT_R32G32_SFLOAT,
            sizeof(float)*6);//sizeof(cube_face) + sizeof(cube_normal));
    // Location 3 : colors
    vertices.attributeDescriptions[3] =
        initVertexInputAttributeDescription(
            VERTEX_BUFFER_FIRST_BINDING_ID,
            3,
            VK_FORMAT_R32G32B32_SFLOAT,
            sizeof(float)*6 + sizeof(float)*2);//sizeof(cube_face) + sizeof(cube_normal) + sizeof(cube_texture));
#endif

    vertices.inputState = initPipelineVertexInputStateCreateInfo();
    vertices.inputState.vertexBindingDescriptionCount =
                  static_cast<uint32_t>(vertices.bindingDescriptions.size());
    vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
    vertices.inputState.vertexAttributeDescriptionCount =
                  static_cast<uint32_t>(vertices.attributeDescriptions.size());
    vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
}


void
TexturedCube::createDescriptorSetLayout()
{
    const VkDescriptorSetLayoutBinding layoutBindings[1] = {
        {
            0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
            VK_SHADER_STAGE_VERTEX_BIT,
            NULL,
        },
#if 0
        {
            1,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            numTextures,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            NULL,
        },
#endif
    };
    const VkDescriptorSetLayoutCreateInfo dslcInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        NULL,
        0,
        1,//2,
        layoutBindings,
    };

    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(vkctx.device,
                                                &dslcInfo, NULL,
                                                &descriptorSetLayout));
}


void
TexturedCube::preparePipeline()
{
    const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        NULL,
        0,
        1,
        &descriptorSetLayout,
        0,
        NULL,
    };

    VK_CHECK_RESULT(vkCreatePipelineLayout(vkctx.device,
                                           &pipelineLayoutCreateInfo,
                                           NULL,
                                           &pipelineLayout));

    VkPipelineInputAssemblyStateCreateInfo ias = { };
    ias.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ias.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineRasterizationStateCreateInfo rs = { };
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE; //VK_CULL_MODE_BACK_BIT;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    // Must be false because we haven't enabled the depthClamp device feature.
    rs.depthClampEnable = VK_FALSE;
     rs.rasterizerDiscardEnable = VK_FALSE;
    rs.depthBiasEnable = VK_FALSE;
    rs.lineWidth = 1.0f;

    VkPipelineColorBlendAttachmentState cbas = { };
    cbas.colorWriteMask = 0xf;
    cbas.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo cbs = { };
    cbs.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cbs.attachmentCount = 1;
    cbs.pAttachments = &cbas;

    VkPipelineDepthStencilStateCreateInfo dss = { };
    dss.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    dss.depthTestEnable = VK_TRUE;
    dss.depthWriteEnable = VK_TRUE;
    dss.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    dss.depthBoundsTestEnable = VK_FALSE;
    dss.back.failOp = VK_STENCIL_OP_KEEP;
    dss.back.passOp = VK_STENCIL_OP_KEEP;
    dss.back.compareOp = VK_COMPARE_OP_ALWAYS;
    dss.stencilTestEnable = VK_FALSE;
    dss.front = dss.back;

    VkPipelineViewportStateCreateInfo vps = { };
    vps.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vps.viewportCount = 1;
    vps.scissorCount = 1;

    std::vector<VkDynamicState> dynamicStateEnables = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo ds = { };
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    ds.pDynamicStates = dynamicStateEnables.data();
    ds.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

    VkPipelineMultisampleStateCreateInfo mss = { };
    mss.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    mss.pSampleMask = NULL;
    mss.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Load shaders
    // Two stages: vs and fs
    //std::array<vk::PipelineShaderStageCreateInfo,2> shaderStages;
    std::array<VkPipelineShaderStageCreateInfo,2> shaderStages;
#if defined(INCLUDE_SHADERS)
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    //shaderStages[0].pNext = NULL;
    //shaderStages[0].flags = 0;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = prepareVertShader();
    shaderStages[0].pName = "main";
    //shaderStages[0].pSpecializationInfo = nullptr;
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    //shaderStages[1].pNext = NULL;
    //shaderStages[1].flags = 0;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = prepareFragShader();
    shaderStages[1].pName = "main";
    //shaderStages[1].pSpecializationInfo = nullptr;
#else
    std::string filepath = getAssetPath();
    shaderStages[0] = static_cast<VkPipelineShaderStageCreateInfo>(loadShader(filepath + "cube.vert.spv",
                                vk::ShaderStageFlagBits::eVertex));
    shaderStages[1] = static_cast<VkPipelineShaderStageCreateInfo>(loadShader(filepath + "cube.frag.spv",
                                vk::ShaderStageFlagBits::eFragment));

#endif

    VkPipelineCacheCreateInfo pc  { };
    pc.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VK_CHECK_RESULT(vkCreatePipelineCache(vkctx.device, &pc, NULL,
                                          &pipelineCache));

    VkGraphicsPipelineCreateInfo pipelineCreateInfo { };
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.renderPass = vkctx.renderPass;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.pVertexInputState =
        &static_cast<const VkPipelineVertexInputStateCreateInfo&>(vertices.inputState);
    pipelineCreateInfo.pInputAssemblyState = &ias;
    pipelineCreateInfo.pRasterizationState = &rs;
    pipelineCreateInfo.pColorBlendState = &cbs;
    pipelineCreateInfo.pMultisampleState = &mss;
    pipelineCreateInfo.pViewportState = &vps;
    pipelineCreateInfo.pDepthStencilState = &dss;
    pipelineCreateInfo.pDynamicState = &ds;
    pipelineCreateInfo.stageCount = (uint32_t)shaderStages.size();
    pipelineCreateInfo.pStages = shaderStages.data();

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(vkctx.device,
                                              pipelineCache,
                                              1,
                                              &pipelineCreateInfo, nullptr,
                                              &pipeline));

}


#if defined(INCLUDE_SHADERS)
VkShaderModule
TexturedCube::prepareVertShader()
{
    size_t codeSize;

    // INCLUDE_SHADERS requires use of glslc or addition of its -mfmt=num
    // option to glslangValidator. In order not to add the additional
    // dependency on glslc we'll load the shaders from files.
    uint32_t vert[] = {
#include "cube.vert.spv"
    };
    codeSize = sizeof(vert);
    return createShaderModule(vert, codeSize);
}


VkShaderModule
TexturedCube::prepareFragShader()
{
    size_t codeSize;

    uint32_t frag[] = {
#include "cube.frag.spv"
    };
    codeSize = sizeof(frag);
    return createShaderModule(frag, codeSize);
}


VkShaderModule
TexturedCube::createShaderModule(uint32_t* spv, size_t size)
{
    VkShaderModule module;
    VkShaderModuleCreateInfo shCreateInfo = {};

    shCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shCreateInfo.codeSize = size;
    shCreateInfo.pCode = spv;
    shCreateInfo.flags = 0;
    VK_CHECK_RESULT(vkCreateShaderModule(vkctx.device, &shCreateInfo,
                                         NULL, &module));

    return module;

}
#endif

void
TexturedCube::prepareDescriptorPool() {
    const VkDescriptorPoolSize typeCounts[2] = {
        {
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
        },
        {
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            numTextures,
        },
    };
    const VkDescriptorPoolCreateInfo dpoolCreateInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        NULL,
        0,
        1,
        2,
        typeCounts,
    };
    U_ASSERT_ONLY VkResult err;

    err = vkCreateDescriptorPool(vkctx.device, &dpoolCreateInfo, NULL,
                                 &descriptorPool);
    assert(!err);
}


static
VkWriteDescriptorSet initWriteDescriptorSet(
    VkDescriptorSet dstSet,
    VkDescriptorType type,
    uint32_t binding,
    const VkDescriptorBufferInfo* bufferInfo)
{
    VkWriteDescriptorSet writeDescriptorSet = {};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.pNext = NULL;
    writeDescriptorSet.dstSet = dstSet;
    writeDescriptorSet.descriptorType = type;
    writeDescriptorSet.dstBinding = binding;
    writeDescriptorSet.pBufferInfo = bufferInfo;
    // Default value in all examples
    writeDescriptorSet.descriptorCount = 1;
    return writeDescriptorSet;
}


void
TexturedCube::prepareDescriptorSet()
{
    VkDescriptorSetAllocateInfo allocInfo = { };
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    VK_CHECK_RESULT(vkAllocateDescriptorSets(vkctx.device,
                                             &allocInfo,
                                             &descriptorSet));

#if 0
    // Image descriptor for the color map texture
    vk::DescriptorImageInfo texDescriptor(
            texture.sampler,
            texture.view,
            vk::ImageLayout::eShaderReadOnlyOptimal);

#endif

    std::vector<VkWriteDescriptorSet> writeDescriptorSets =
    {
        // Binding 0 : Vertex shader uniform buffer
        initWriteDescriptorSet(
            descriptorSet,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            0,
            &static_cast<const VkDescriptorBufferInfo&>(uniformData.descriptor)),
#if 0
        // Binding 1 : Fragment shader texture sampler
        initWriteDescriptorSet(
            descriptorSet,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1,
            &texDescriptor)
#endif
    };

    vkUpdateDescriptorSets(vkctx.device,
                           static_cast<uint32_t>(writeDescriptorSets.size()),
                           writeDescriptorSets.data(),
                           0, NULL);
}

// Copyright 2018-2020 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

#include <vulkan/vulkan.h>
#include <ktxvulkan.h>
// Use c++ to keep the example short. Not required.
#include <vulkan/vulkan.hpp>

class Texture {
  public:
    Texture(const std::string ktxfile);
    ~Texture();

  protected:
    ktxVulkanTexture texture;
    vk::PhysicalDevice gpu;
    vk::Device device;
    vk::Queue queue;
    vk::CommandPool commandPool;

    cleanup();
    prepareSamplerAndView();
};

Texture::Texture(const std::string ktxfile)
{
    ktxVulkanDeviceInfo kvdi;
    ktxTexture* kTexture;
    KTX_error_code ktxresult;

    createVulkanInstance();
    findVulkanGpu();         // Find a suitable physical device
    createVulkanSurface();
    createVulkanDevice();
    prepareVulkanSwapchain();
    /*
          .
          .
          .
    */

    // This structure is used to pass the Vulkan device information to the loader
    // with the expectation that app's will typically load many textures.
    ktxVulkanDeviceInfo_Construct(&kvdi, gpu, device, queue, commandPool, nullptr);

    ktxresult = ktxTexture_CreateFromNamedFile(
                                        (getAssetPath() + ktxfile).c_str(),
                                        KTX_TEXTURE_CREATE_NO_FLAGS,
                                        &kTexture);
    if (KTX_SUCCESS != ktxresult) {
        std::stringstream message;

        message << "Creation of ktxTexture from \"" << getAssetPath()
                << ktxfile << "\" failed: " << ktxErrorString(ktxresult);
        throw std::runtime_error(message.str());
    }
    ktxresult = ktxTexture_VkUploadEx(kTexture, &kvdi, &texture,
                                      VK_IMAGE_TILING_OPTIMAL,
                                      VK_IMAGE_USAGE_SAMPLED_BIT,
                                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    if (KTX_SUCCESS != ktxresult) {
        std::stringstream message;

        message << "ktxTexture_VkUpload failed: " << ktxErrorString(ktxresult);
        throw std::runtime_error(message.str());
    }

    char* pValue;
    uint32_t valueLen;
    if (KTX_SUCCESS == ktxHashTable_FindValue(&kTexture->kvDataHead,
                                              KTX_ORIENTATION_KEY,
                                              &valueLen, (void**)&pValue))
    {
        char s, t;

        if (sscanf(pValue, KTX_ORIENTATION2_FMT, &s, &t) == 2) {
            if (s == 'l') sign_s = -1;
            if (t == 'u') sign_t = -1;
        }
    }

    ktxTexture_Destroy(kTexture);
    ktxVulkanDeviceInfo_destruct(&kvdi);

    try {
        prepareSamplerAndView(); // See below  for implementation.
        // Setup a layout with, e.g., a binding for a combined image-sampler.
        setupDescriptorSetLayout();
        // Create a descriptor set and update it with the sampler and image view handles.
        setupDescriptorSet();
        preparePipelines();
        setupDescriptorPool();
        setupDescriptorSet();
        buildCommandBuffers();
    } catch (std::exception&) {
        cleanup();
        throw;
    }
}

Texture::~Texture()
{
    cleanup();
}

Texture::cleanup()
{
    destroyCommandBuffers();
    destroySampler();
    destroyImageview();
    ktxVulkanTexture_destruct(&texture, device, nullptr);
    /*
          .
          .
          .
    */
}

void
Texture::prepareSamplerAndView()
{
    // Create sampler.
    vk::SamplerCreateInfo samplerInfo;
    // Set the non-default values
    samplerInfo.magFilter = vk::Filter::eLinear;
    samplerInfo.minFilter = vk::Filter::eLinear;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.maxLod = texture.levelCount;
    samplerInfo.anisotropyEnable = false;
    samplerInfo.maxAnisotropy = 1.0;
    samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
    sampler = vkctx.device.createSampler(samplerInfo);

    // Create image view.
    // Textures are not directly accessed by the shaders and are abstracted
    // by image views containing additional information and sub resource
    // ranges.
    vk::ImageViewCreateInfo viewInfo;
    // Set the non-default values.
    viewInfo.image = texture.image;
    viewInfo.format = static_cast<vk::Format>(texture.imageFormat);
    viewInfo.viewType = static_cast<vk::ImageViewType>(texture.viewType);
    viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    viewInfo.subresourceRange.layerCount = texture.layerCount;
    viewInfo.subresourceRange.levelCount = texture.levelCount;
    imageView = vkctx.device.createImageView(viewInfo);
}

/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

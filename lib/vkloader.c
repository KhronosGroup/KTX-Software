/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2018 Mark Callow.
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
 
/**
 * @internal
 * @file
 * @~English
 *
 * @brief Functions for instantiating Vulkan textures from KTX files.
 *
 * @author Mark Callow, Edgewise Consulting
 */

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
/* For GL format tokens */
#include "GL/glcorearb.h"
#include "GL/glext.h"

#include "ktxvulkan.h"
#include "ktxint.h"
#include "vk_format.h"

// Macro to check and display Vulkan return results.
// Use when the only possible errors are caused by invalid usage by this loader.
#if defined(_DEBUG)
#define VK_CHECK_RESULT(f)                                                  \
{                                                                           \
    VkResult res = (f);                                                     \
    if (res != VK_SUCCESS)                                                  \
    {                                                                       \
        /* XXX Find an errorString function. */                             \
        fprintf(stderr, "Fatal error in ktxLoadVkTexture*: "                \
                "VkResult is \"%d\" in %s at line %d\n",                    \
                res, __FILE__, __LINE__);                                   \
        assert(res == VK_SUCCESS);                                          \
    }                                                                       \
}
#else
#define VK_CHECK_RESULT(f) ((void)f)
#endif

#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))

#define DEFAULT_FENCE_TIMEOUT 100000000000
#define VK_FLAGS_NONE 0

static void
setImageLayout(
    VkCommandBuffer cmdBuffer,
    VkImage image,
    VkImageAspectFlags aspectMask,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkImageSubresourceRange subresourceRange);

/**
 * @defgroup ktx_vkloader Vulkan Texture Image Loader
 * @brief Create texture images on a Vulkan device.
 * @{
 */

/**
 * @example vkload.cpp
 * This shows how to create and load a Vulkan image using the Vulkan texture
 * image loading functions.
 */

/**
 * @memberof ktxVulkanDeviceInfo
 * @~English
 * @brief Create a ktxVulkanDeviceInfo object.
 * 
 * Allocates CPU memory for a ktxVulkanDeviceInfo object then calls
 * ktxVulkanDeviceInfo_construct(). See it for documentation of the
 * parameters.
 *
 * @return a pointer to the constructed ktxVulkanDeviceInfo.
 *
 * @sa ktxVulkanDeviceInfo_construct(), ktxVulkanDeviceInfo_destroy()
 */
ktxVulkanDeviceInfo*
ktxVulkanDeviceInfo_Create(VkPhysicalDevice physicalDevice, VkDevice device,
                           VkQueue queue, VkCommandPool cmdPool,
                           const VkAllocationCallbacks* pAllocator)
{
    ktxVulkanDeviceInfo* newvdi;
    newvdi = (ktxVulkanDeviceInfo*)malloc(sizeof(ktxVulkanDeviceInfo));
    if (newvdi != NULL) {
        if (ktxVulkanDeviceInfo_Construct(newvdi, physicalDevice, device,
                                    queue, cmdPool, pAllocator) != KTX_SUCCESS)
        {
            free(newvdi);
            newvdi = 0;
        }
    }
    return newvdi;
}

/**
 * @memberof ktxVulkanDeviceInfo
 * @~English
 * @brief Construct a ktxVulkanDeviceInfo object.
 *
 * Records the device information, allocates a command buffer that will be
 * used to transfer image data to the Vulkan device and retrieves the physical
 * device memory properties for ease of use when allocating device memory for
 * the images.
 *
 * Pass a valid ktxVulkanDeviceInfo* to any Vulkan KTX image loading
 * function to provide it with the information.
 *
 * @param  This            pointer to the ktxVulkanDeviceInfo object to
 *                        initialize.
 * @param  physicalDevice handle of the Vulkan physical device.
 * @param  device         handle of the Vulkan logical device.
 * @param  queue          handle of the Vulkan queue.
 * @param  cmdPool        handle of the Vulkan command pool.
 * @param  pAllocator     pointer to the allocator to use for the image
 *                        memory. If NULL, the default allocator will be used.
 *
 * @returns KTX_SUCCESS on success, KTX_OUT_OF_MEMORY if a command buffer could
 *          not be allocated.
 *
 * @sa ktxVulkanDeviceInfo_destruct()
 */
KTX_error_code
ktxVulkanDeviceInfo_Construct(ktxVulkanDeviceInfo* This,
                              VkPhysicalDevice physicalDevice, VkDevice device,
                              VkQueue queue, VkCommandPool cmdPool,
                              const VkAllocationCallbacks* pAllocator)
{
    VkCommandBufferAllocateInfo cmdBufInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO
    };
    VkResult result;

    This->physicalDevice = physicalDevice;
    This->device = device;
    This->queue = queue;
    This->cmdPool = cmdPool;
    This->pAllocator = pAllocator;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice,
                                        &This->deviceMemoryProperties);

    // Use a separate command buffer for texture loading. Needed for
    // submitting image barriers and converting tilings.
    cmdBufInfo.commandPool = cmdPool;
    cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufInfo.commandBufferCount = 1;
    result = vkAllocateCommandBuffers(device, &cmdBufInfo, &This->cmdBuffer);
    if (result != VK_SUCCESS) {
        return KTX_OUT_OF_MEMORY; // XXX Consider an equivalent to pGlError
    }
    return KTX_SUCCESS;
}

/**
 * @memberof ktxVulkanDeviceInfo
 * @~English
 * @brief Destruct a ktxVulkanDeviceInfo object.
 *
 * Frees the command buffer.
 *
 * @param This pointer to the ktxVulkanDeviceInfo to destruct.
 */
void
ktxVulkanDeviceInfo_Destruct(ktxVulkanDeviceInfo* This)
{
    vkFreeCommandBuffers(This->device, This->cmdPool, 1,
                         &This->cmdBuffer);
}

/**
 * @memberof ktxVulkanDeviceInfo
 * @~English
 * @brief Destroy a ktxVulkanDeviceInfo object.
 *
 * Calls ktxVulkanDeviceInfo_destruct() then frees the ktxVulkanDeviceInfo.
 *
 * @param This pointer to the ktxVulkanDeviceInfo to destroy.
 */
void
ktxVulkanDeviceInfo_Destroy(ktxVulkanDeviceInfo* This)
{
    assert(This != NULL);
    ktxVulkanDeviceInfo_Destruct(This);
    free(This);
}

/* Get appropriate memory type index for a memory allocation. */
static uint32_t
ktxVulkanDeviceInfo_getMemoryType(ktxVulkanDeviceInfo* This,
                                  uint32_t typeBits, VkFlags properties)
{
    for (uint32_t i = 0; i < 32; i++)
    {
        if ((typeBits & 1) == 1)
        {
            if ((This->deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        typeBits >>= 1;
    }

    // XXX : throw error
    return 0;
}

//======================================================================
//  ReadImages callbacks
//======================================================================

typedef struct user_cbdata_optimal {
    VkBufferImageCopy* region; // specify destination region in final image.
    VkDeviceSize offset;       // Offset of current level in staging buffer
    ktx_uint32_t numFaces;
    ktx_uint32_t numLayers;
#if defined(_DEBUG)
    VkBufferImageCopy* regionsArrayEnd;   //  "
#endif
} user_cbdata_optimal;

/*
 * Callback for an optimally tiled texture. Set up a copy region for
 * the miplevel.
 */
KTX_error_code KTXAPIENTRY
optimalTilingCallback(int miplevel, int face,
                      int width, int height, int depth,
                      ktx_uint32_t faceLodSize,
                      void* pixels, void* userdata)
{
    user_cbdata_optimal* ud = (user_cbdata_optimal*)userdata;

    // Set up copy to destination region in final image
    assert(ud->region < ud->regionsArrayEnd);
    ud->region->bufferOffset = ud->offset;
    ud->offset += faceLodSize;
    // XXX Handle row padding for uncompressed textures. KTX specifies
    // GL_UNPACK_ALIGNMENT of 4 so need to pad this from actual width.
    // That means I need the element size and group size for the format
    // to calculate bufferRowLength.
    ud->region->bufferRowLength = 0;
    ud->region->bufferImageHeight = 0;
    ud->region->imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ud->region->imageSubresource.mipLevel = miplevel;
    ud->region->imageSubresource.baseArrayLayer = face;
    ud->region->imageSubresource.layerCount = ud->numLayers * ud->numFaces;
    ud->region->imageOffset.x = 0;
    ud->region->imageOffset.y = 0;
    ud->region->imageOffset.z = 0;
    ud->region->imageExtent.width = width;
    ud->region->imageExtent.height = height;
    ud->region->imageExtent.depth = depth;

    ud->region += 1; // XXX Probably need some check of the array length.

    return KTX_SUCCESS;
}

typedef struct user_cbdata_linear {
    VkImage destImage;
    VkDevice device;
    uint8_t* dest;   // Pointer to mapped Image memory
} user_cbdata_linear;


/*
 * Callback for linear tiled textures.
 * Copy the image data into the Vulkan image.
 */
KTX_error_code KTXAPIENTRY
linearTilingCallback(int miplevel, int face,
                      int width, int height, int depth,
                      ktx_uint32_t faceLodSize,
                      void* pixels, void* userdata)
{
    user_cbdata_linear* ud = (user_cbdata_linear*)userdata;
    VkSubresourceLayout subResLayout;
    VkImageSubresource subRes = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .mipLevel = miplevel,
      .arrayLayer = face
    };

    // Get sub resources layout
    // Includes row pitch, size offsets, etc.
    vkGetImageSubresourceLayout(ud->device, ud->destImage, &subRes, &subResLayout);

    // Copy image data to destImage via its mapped memory.
    // XXX How to handle subResLayout.{array,depth,row}Pitch?
    //     Problem if rowPitch is not a multiple of 4. Really don't want
    //     copy a row at a time. For now
    if ((subResLayout.rowPitch & 0x3) != 0)
        return KTX_INVALID_OPERATION;
    // XXX We receive all the array levels in one lump. Will this work?
    memcpy(ud->dest + subResLayout.offset, pixels, faceLodSize);
    return KTX_SUCCESS;
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Create a Vulkan image object from a ktxTexture object.
 *
 * Creates a VkImage with format etc. matching the KTX data and uploads the
 * images. Also creates VkSampler and VkImageView objects for accessing the
 * image. Returns the handles of the created objects and information about
 * the texture in the ktxVulkanTexture pointed at by @p vkTexture.
 *
 * Most Vulkan implementations support VK_IMAGE_TILING_LINEAR only for a very
 * limited number of formats and features. Generally VK_IMAGE_TILING_OPTIMAL is
 * preferred. The latter requires a staging buffer so will use more memory
 * during loading.
 *
 * @param[in] This          pointer to the ktxTexture from which to upload.
 * @anchor vdiparam
 * @param [in] vdi          pointer to a ktxVulkanDeviceInfo structure providing
 *                          information about the Vulkan device onto which to
 *                          load the texture.
 * @anchor ptextureparam
 * @param [in,out] vkTexture pointer to a ktxVulkanTexture structure into which
 *                           the function writes information about the created
 *                           VkImage.
 * @anchor tilingparam
 * @param [in] tiling       type of tiling to use in the destination image
 *                          on the Vulkan device.
 * @anchor usageflagsparam
 * @param [in] usageFlags   a set of VkImageUsageFlags bits indicating the
 *                          intended usage of the destination image.
 * @anchor layoutparam
 * @param [in] layout       a VkImageLayout value indicating the desired
 *                          layout the destination image.
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p This, @p vdi or @p vkTexture is @c NULL.
 * @exception KTX_INVALID_OPERATION The ktxTexture contains neither images nor
 *                                  an active stream from which to read them.
 * @exception KTX_OUT_OF_MEMORY Sufficient memory could not be allocated
 *                              on either the CPU or the Vulkan device.
 */
KTX_error_code
ktxTexture_VkUploadEx(ktxTexture* This, ktxVulkanDeviceInfo* vdi,
                      ktxVulkanTexture* vkTexture,
                      VkImageTiling tiling,
                      VkImageUsageFlags usageFlags,
                      VkImageLayout layout)
{
    KTX_error_code           kResult;
    VkFormat                 vkFormat;
    VkImageType              imageType;
    VkImageViewType          viewType;
    VkImageCreateFlags       createFlags = 0;
    VkImageFormatProperties  formatProperties;
    VkResult                 vResult;
    VkCommandBufferBeginInfo cmdBufBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL
    };
    VkImageCreateInfo        imageCreateInfo = {
         .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
         .pNext = NULL
    };
    VkMemoryAllocateInfo     memAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = 0,
        .memoryTypeIndex = 0
    };
    VkMemoryRequirements     memReqs;
    uint32_t                 arrayLayers;

    if (!vdi || !This || !vkTexture) {
        return KTX_INVALID_VALUE;
    }

    if (!This->pData && !ktxTexture_isActiveStream(This)) {
        /* Nothing to upload. */
        return KTX_INVALID_OPERATION;
    }

    /* _ktxCheckHeader should have caught this. */
    assert(This->numFaces == 6 ? This->numDimensions == 2 : VK_TRUE);

    arrayLayers = This->numLayers;
    if (This->isCubemap) {
        arrayLayers *= 6;
        createFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    vkTexture->width = This->baseWidth;
    vkTexture->height = This->baseHeight;
    vkTexture->depth = This->baseDepth;
    switch (This->numDimensions) {
      case 1:
        imageType = VK_IMAGE_TYPE_1D;
        viewType = This->isArray ?
                        VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
        break;
      case 2:
        imageType = VK_IMAGE_TYPE_2D;
        if (This->isCubemap)
            viewType = This->isArray ?
                        VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
        else
            viewType = This->isArray ?
                        VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
        break;
      case 3:
        imageType = VK_IMAGE_TYPE_3D;
        /* 3D array textures not supported in Vulkan. Attempts to create or
         * load them should have been trapped long before this.
         */
        assert(!This->isArray);
        viewType = VK_IMAGE_VIEW_TYPE_3D;
        break;
    }

    vkFormat = vkGetFormatFromOpenGLInternalFormat(This->glInternalformat);
    if (vkFormat == VK_FORMAT_UNDEFINED)
        vkFormat = vkGetFormatFromOpenGLFormat(This->glFormat, This->glType);
    if (vkFormat == VK_FORMAT_UNDEFINED) {
        return KTX_INVALID_OPERATION;
    }

    /* Get device properties for the requested texture format */
    vResult = vkGetPhysicalDeviceImageFormatProperties(vdi->physicalDevice,
                                                      vkFormat,
                                                      imageType,
                                                      tiling,
                                                      usageFlags,
                                                      createFlags,
                                                      &formatProperties);
    if (vResult == VK_ERROR_FORMAT_NOT_SUPPORTED) {
        return KTX_INVALID_OPERATION;
    }

    VK_CHECK_RESULT(vkBeginCommandBuffer(vdi->cmdBuffer, &cmdBufBeginInfo));

    if (tiling != VK_IMAGE_TILING_LINEAR)
    {
        // Create a host-visible staging buffer that contains the raw image data
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;
        VkBufferImageCopy* copyRegions;
        VkDeviceSize textureSize;
        VkBufferCreateInfo bufferCreateInfo = {
          .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
          .pNext = NULL
        };
        VkImageSubresourceRange subresourceRange;
        VkFence copyFence;
        VkFenceCreateInfo fenceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = NULL,
            .flags = VK_FLAGS_NONE
        };
        VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = NULL
        };
        ktx_uint8_t* pMappedStagingBuffer;

        /*
         * Because all array layers and faces are the same size they can be
         * copied in a single operation so there'll be 1 copy per mip level.
         */
        uint32_t numCopyRegions = This->numLevels;
        user_cbdata_optimal cbData;

        textureSize = ktxTexture_GetSize(This);

        copyRegions = (VkBufferImageCopy*)malloc(sizeof(VkBufferImageCopy)
                                                   * numCopyRegions);
        if (copyRegions == NULL) {
            return KTX_OUT_OF_MEMORY;
        }

        bufferCreateInfo.size = textureSize;
        // This buffer is used as a transfer source for the buffer copy
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VK_CHECK_RESULT(vkCreateBuffer(vdi->device, &bufferCreateInfo,
                                       vdi->pAllocator, &stagingBuffer));

        // Get memory requirements for the staging buffer (alignment,
        // memory type bits)
        vkGetBufferMemoryRequirements(vdi->device, stagingBuffer, &memReqs);

        memAllocInfo.allocationSize = memReqs.size;
        // Get memory type index for a host visible buffer
        memAllocInfo.memoryTypeIndex = ktxVulkanDeviceInfo_getMemoryType(
                vdi,
                memReqs.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        vResult = vkAllocateMemory(vdi->device, &memAllocInfo,
                                  vdi->pAllocator, &stagingMemory);
        if (vResult != VK_SUCCESS) {
            return KTX_OUT_OF_MEMORY;
        }
        VK_CHECK_RESULT(vkBindBufferMemory(vdi->device, stagingBuffer,
                                           stagingMemory, 0));

        VK_CHECK_RESULT(vkMapMemory(vdi->device, stagingMemory, 0,
                                    memReqs.size, 0,
                                    (void **)&pMappedStagingBuffer));

        cbData.offset = 0;
        cbData.region = copyRegions;
        cbData.numFaces = This->numFaces;
        cbData.numLayers = This->numLayers;
#if defined(_DEBUG)
        cbData.regionsArrayEnd = copyRegions + numCopyRegions;
#endif

        if (This->pData) {
            /* Image data has already been loaded. Copy to staging buffer. */
            assert(This->dataSize <= memAllocInfo.allocationSize);
            memcpy(pMappedStagingBuffer, This->pData, This->dataSize);
        } else {
            /* Load the image data directly into the staging buffer. */
            kResult = ktxTexture_LoadImageData(This, pMappedStagingBuffer,
                                               memAllocInfo.allocationSize);
            if (kResult != KTX_SUCCESS)
                return kResult;
        }
        
        // Iterate over mip levels to set up the copy regions.
        kResult = ktxTexture_IterateLevels(This,
                                           optimalTilingCallback,
                                           &cbData);
        // XXX Check for possible errors

        vkUnmapMemory(vdi->device, stagingMemory);

        // Create optimal tiled target image
        imageCreateInfo.imageType = imageType;
        imageCreateInfo.flags = createFlags;
        imageCreateInfo.format = vkFormat;
        imageCreateInfo.mipLevels = This->numLevels;
        imageCreateInfo.arrayLayers = arrayLayers;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = usageFlags;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.extent.width = vkTexture->width;
        imageCreateInfo.extent.height = vkTexture->height;
        imageCreateInfo.extent.depth = vkTexture->depth;
        imageCreateInfo.usage
                = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        VK_CHECK_RESULT(vkCreateImage(vdi->device, &imageCreateInfo,
                                      vdi->pAllocator, &vkTexture->image));

        vkGetImageMemoryRequirements(vdi->device, vkTexture->image, &memReqs);

        memAllocInfo.allocationSize = memReqs.size;

        memAllocInfo.memoryTypeIndex = ktxVulkanDeviceInfo_getMemoryType(
                                          vdi, memReqs.memoryTypeBits,
                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VK_CHECK_RESULT(vkAllocateMemory(vdi->device, &memAllocInfo,
                                         vdi->pAllocator,
                                         &vkTexture->deviceMemory));
        VK_CHECK_RESULT(vkBindImageMemory(vdi->device, vkTexture->image,
                                          vkTexture->deviceMemory, 0));

        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = This->numLevels;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = arrayLayers;

        // Image barrier for optimal image (target)
        // Optimal image will be used as destination for the copy
        setImageLayout(
            vdi->cmdBuffer,
            vkTexture->image,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            subresourceRange);

        // Copy mip levels from staging buffer
        vkCmdCopyBufferToImage(
            vdi->cmdBuffer, stagingBuffer,
            vkTexture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            numCopyRegions, copyRegions
            );

        // Change texture image layout to shader read after all mip levels
        // have been copied
        setImageLayout(
            vdi->cmdBuffer,
            vkTexture->image,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            layout,
            subresourceRange);

        // Submit command buffer containing copy and image layout commands
        VK_CHECK_RESULT(vkEndCommandBuffer(vdi->cmdBuffer));

        // Create a fence to make sure that the copies have finished before
        // continuing
        VK_CHECK_RESULT(vkCreateFence(vdi->device, &fenceCreateInfo,
                                      vdi->pAllocator, &copyFence));

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &vdi->cmdBuffer;

        VK_CHECK_RESULT(vkQueueSubmit(vdi->queue, 1, &submitInfo, copyFence));

        VK_CHECK_RESULT(vkWaitForFences(vdi->device, 1, &copyFence,
                                        VK_TRUE, DEFAULT_FENCE_TIMEOUT));

        vkDestroyFence(vdi->device, copyFence, vdi->pAllocator);

        // Clean up staging resources
        vkFreeMemory(vdi->device, stagingMemory, vdi->pAllocator);
        vkDestroyBuffer(vdi->device, stagingBuffer, vdi->pAllocator);
    }
    else
    {
        VkImage mappableImage;
        VkDeviceMemory mappableMemory;
        VkFence nullFence = { VK_NULL_HANDLE };
        VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = NULL
        };
        VkImageSubresourceRange subresourceRange;
        user_cbdata_linear cbData;

        imageCreateInfo.imageType = imageType;
        imageCreateInfo.flags = createFlags;
        imageCreateInfo.format = vkFormat;
        imageCreateInfo.extent.width = vkTexture->width;
        imageCreateInfo.extent.height = vkTexture->height;
        imageCreateInfo.extent.depth = vkTexture->depth;
        imageCreateInfo.mipLevels = This->numLevels;
        imageCreateInfo.arrayLayers = arrayLayers;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
        imageCreateInfo.usage = usageFlags;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

        // Load mip map level 0 to linear tiling image
        VK_CHECK_RESULT(vkCreateImage(vdi->device, &imageCreateInfo,
                                      vdi->pAllocator, &mappableImage));

        // Get memory requirements for this image
        // like size and alignment
        vkGetImageMemoryRequirements(vdi->device, mappableImage, &memReqs);
        // Set memory allocation size to required memory size
        memAllocInfo.allocationSize = memReqs.size;

        // Get memory type that can be mapped to host memory
        memAllocInfo.memoryTypeIndex = ktxVulkanDeviceInfo_getMemoryType(
                vdi,
                memReqs.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        // Allocate host memory
        vResult = vkAllocateMemory(vdi->device, &memAllocInfo, vdi->pAllocator,
                                  &mappableMemory);
        if (vResult != VK_SUCCESS) {
            return KTX_OUT_OF_MEMORY;
        }
        VK_CHECK_RESULT(vkBindImageMemory(vdi->device, mappableImage,
                                          mappableMemory, 0));

        cbData.destImage = mappableImage;
        cbData.device = vdi->device;

        // Map image memory
        VK_CHECK_RESULT(vkMapMemory(vdi->device, mappableMemory, 0,
                        memReqs.size, 0, (void **)&cbData.dest));

        // Iterate over images to copy texture data into mapped image memory.
        if (ktxTexture_isActiveStream(This)) {
            kResult = ktxTexture_IterateLoadLevelFaces(This,
                                                       linearTilingCallback,
                                                       &cbData);
        } else {
            kResult = ktxTexture_IterateLevelFaces(This,
                                                   linearTilingCallback,
                                                   &cbData);
        }
        // XXX Check for possible errors

        vkUnmapMemory(vdi->device, mappableMemory);

        // Linear tiled images don't need to be staged
        // and can be directly used as textures
        vkTexture->image = mappableImage;
        vkTexture->deviceMemory = mappableMemory;

        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = This->numLevels;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = arrayLayers;

        // Setup image memory barrier
        setImageLayout(
            vdi->cmdBuffer,
            vkTexture->image,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_PREINITIALIZED,
            layout,
            subresourceRange);

        // Submit command buffer containing image layout commands
        VK_CHECK_RESULT(vkEndCommandBuffer(vdi->cmdBuffer));

        submitInfo.waitSemaphoreCount = 0;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &vdi->cmdBuffer;

        VK_CHECK_RESULT(vkQueueSubmit(vdi->queue, 1, &submitInfo, nullFence));
        VK_CHECK_RESULT(vkQueueWaitIdle(vdi->queue));
    }

    vkTexture->imageFormat = vkFormat;
    vkTexture->imageLayout = layout;
    vkTexture->levelCount = This->numLevels;
    vkTexture->layerCount = arrayLayers;
    vkTexture->viewType = viewType;

    return KTX_SUCCESS;
}

/** @memberof ktxTexture
 * @~English
 * @brief Create a Vulkan image object from a ktxTexture object.
 *
 * Calls ktxReader_LoadVkTextureEx() with the most commonly used options:
 * VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT and
 * VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
 * 
 * @sa ktxReader_LoadVkTextureEx() for details and use that for complete
 *     control.
 */
KTX_error_code
ktxTexture_VkUpload(ktxTexture* texture, ktxVulkanDeviceInfo* vdi,
                    ktxVulkanTexture *vkTexture)
{
    return ktxTexture_VkUploadEx(texture, vdi, vkTexture,
                                 VK_IMAGE_TILING_OPTIMAL,
                                 VK_IMAGE_USAGE_SAMPLED_BIT,
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

//======================================================================
//  Utilities
//======================================================================

/**
 * @internal
 * @~English
 * @brief Create an image memory barrier for changing the layout of an image.
 *
 * The barrier is placed in the passed command buffer. See the Vulkan spec.
 * chapter 11.4 "Image Layout" for details.
 */
static void
setImageLayout(
    VkCommandBuffer cmdBuffer,
    VkImage image,
    VkImageAspectFlags aspectMask,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkImageSubresourceRange subresourceRange)
{
    // Create an image barrier object
    VkImageMemoryBarrier imageMemoryBarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = NULL,
         // Some default values
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED
    };

    imageMemoryBarrier.oldLayout = oldLayout;
    imageMemoryBarrier.newLayout = newLayout;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = subresourceRange;

    // Source layouts (old)
    // The source access mask controls actions to be finished on the old
    // layout before it will be transitioned to the new layout.
    switch (oldLayout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        // Image layout is undefined (or does not matter).
        // Only valid as initial layout. No flags required.
        imageMemoryBarrier.srcAccessMask = 0;
        break;

    case VK_IMAGE_LAYOUT_PREINITIALIZED:
        // Image is preinitialized.
        // Only valid as initial layout for linear images; preserves memory
        // contents. Make sure host writes have finished.
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image is a color attachment.
        // Make sure writes to the color buffer have finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image is a depth/stencil attachment.
        // Make sure any writes to the depth/stencil buffer have finished.
        imageMemoryBarrier.srcAccessMask
                                = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image is a transfer source.
        // Make sure any reads from the image have finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image is a transfer destination.
        // Make sure any writes to the image have finished.
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image is read by a shader.
        // Make sure any shader reads from the image have finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;

    default:
        /* Value not used by callers, so not supported. */
        assert(KTX_FALSE);
    }

    // Target layouts (new)
    // The destination access mask controls the dependency for the new image
    // layout.
    switch (newLayout)
    {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image will be used as a transfer destination.
        // Make sure any writes to the image have finished.
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image will be used as a transfer source.
        // Make sure any reads from and writes to the image have finished.
        imageMemoryBarrier.srcAccessMask |= VK_ACCESS_TRANSFER_READ_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image will be used as a color attachment.
        // Make sure any writes to the color buffer have finished.
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image layout will be used as a depth/stencil attachment.
        // Make sure any writes to depth/stencil buffer have finished.
        imageMemoryBarrier.dstAccessMask
                                = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image will be read in a shader (sampler, input attachment).
        // Make sure any writes to the image have finished.
        if (imageMemoryBarrier.srcAccessMask == 0)
        {
            imageMemoryBarrier.srcAccessMask
                    = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        /* Value not used by callers, so not supported. */
        assert(KTX_FALSE);
    }

    // Put barrier on top of pipeline.
    VkPipelineStageFlags srcStageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkPipelineStageFlags destStageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    // Add the barrier to the passed command buffer
    vkCmdPipelineBarrier(
        cmdBuffer,
        srcStageFlags,
        destStageFlags,
        0,
        0, NULL,
        0, NULL,
        1, &imageMemoryBarrier);
}

//======================================================================
//  ktxVulkanTexture utilities
//======================================================================

/**
 * @memberof ktxVulkanTexture
 * @~English
 * @brief Destructor for the object returned when loading a texture image.
 *
 * Frees the Vulkan resources created when the texture image was loaded.
 *
 * @param vkTexture  pointer to the ktxVulkanTexture to be destructed.
 * @param device     handle to the Vulkan logical device to which the texture was
 *                   loaded.
 * @param pAllocator pointer to the allocator used during loading.
 */
void
ktxVulkanTexture_Destruct(ktxVulkanTexture* vkTexture, VkDevice device,
                          const VkAllocationCallbacks* pAllocator)
{
    vkDestroyImage(device, vkTexture->image, pAllocator);
    vkFreeMemory(device, vkTexture->deviceMemory, pAllocator);
}

/** @} */

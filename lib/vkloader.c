/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

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
#include "ktxreader.h"
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
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    VkImageSubresourceRange subresourceRange);

/**
 * @defgroup vkhelper Vulkan Texture Image Loader Helpers
 * @{
 */

/**
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
ktxVulkanDeviceInfo_create(VkPhysicalDevice physicalDevice, VkDevice device,
                           VkQueue queue, VkCommandPool cmdPool,
						   const VkAllocationCallbacks* pAllocator)
{
    ktxVulkanDeviceInfo* vdi;
    vdi = (ktxVulkanDeviceInfo*)malloc(sizeof(ktxVulkanDeviceInfo));
    if (vdi != NULL) {
        if (ktxVulkanDeviceInfo_construct(vdi, physicalDevice, device,
                                    queue, cmdPool, pAllocator) != KTX_SUCCESS)
        {
            free(vdi);
            vdi = 0;
        }
    }
    return vdi;
}

/**
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
 * @param  vdi            pointer to the ktxVulkanDeviceInfo object to
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
ktxVulkanDeviceInfo_construct(ktxVulkanDeviceInfo* vdi,
                              VkPhysicalDevice physicalDevice, VkDevice device,
                              VkQueue queue, VkCommandPool cmdPool,
							  const VkAllocationCallbacks* pAllocator)
{
    VkCommandBufferAllocateInfo cmdBufInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO
    };
    VkResult result;

    vdi->physicalDevice = physicalDevice;
    vdi->device = device;
    vdi->queue = queue;
    vdi->cmdPool = cmdPool;
    vdi->pAllocator = pAllocator;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice,
                                        &vdi->deviceMemoryProperties);

    // Use a separate command buffer for texture loading. Needed for
    // submitting image barriers and converting tilings.
    cmdBufInfo.commandPool = cmdPool;
    cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufInfo.commandBufferCount = 1;
    result = vkAllocateCommandBuffers(device, &cmdBufInfo, &vdi->cmdBuffer);
    if (result != VK_SUCCESS) {
        return KTX_OUT_OF_MEMORY; // XXX Consider an equivalent to pGlError
    }
    return KTX_SUCCESS;
}

/**
 * @~English
 * @brief Destruct a ktxVulkanDeviceInfo object.
 *
 * Frees the command buffer.
 *
 * @param vdi pointer to the ktxVulkanDeviceInfo to destruct.
 */
void
ktxVulkanDeviceInfo_destruct(ktxVulkanDeviceInfo* vdi)
{
    vkFreeCommandBuffers(vdi->device, vdi->cmdPool, 1,
                         &vdi->cmdBuffer);
}

/**
 * @~English
 * @brief Destroy a ktxVulkanDeviceInfo object.
 *
 * Calls ktxVulkanDeviceInfo_destruct() then frees the ktxVulkanDeviceInfo.
 *
 * @param vdi pointer to the ktxVulkanDeviceInfo to destroy.
 */
void
ktxVulkanDeviceInfo_destroy(ktxVulkanDeviceInfo* vdi)
{
    assert(vdi != NULL);
    ktxVulkanDeviceInfo_destruct(vdi);
    free(vdi);
}

/* Get appropriate memory type index for a memory allocation. */
static uint32_t
ktxVulkanDeviceInfo_getMemoryType(ktxVulkanDeviceInfo* vdi,
                                  uint32_t typeBits, VkFlags properties)
{
    for (uint32_t i = 0; i < 32; i++)
    {
        if ((typeBits & 1) == 1)
        {
            if ((vdi->deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
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
    VkDeviceSize offset;       // Offset of current image in staging buffer
    uint8_t* dest;             // Pointer to staging buffer
#if defined(_DEBUG)
    size_t stagingBufferSize;  // Bytes.
    VkBufferImageCopy* regionsArrayEnd;   //  "
#endif
} user_cbdata_optimal;

/*
 * Callback for an optimally tiled texture.
 * Copy the image data to the staging buffer.
 */
KTX_error_code KTXAPIENTRY
optimalTilingCallback(int miplevel, int face,
                      int width, int height, int depth,
                      int layers,
                      ktx_uint32_t faceLodSize,
                      void* pixels, void* userdata)
{
    user_cbdata_optimal* ud = (user_cbdata_optimal*)userdata;

    // Copy image to staging buffer
    assert((size_t)ud->offset < ud->stagingBufferSize);
    memcpy(ud->dest + ud->offset, pixels, faceLodSize);

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
    ud->region->imageSubresource.layerCount = layers;
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
                      int layers,
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

/** @ingroup reader
 * @~English
 * @brief Create a Vulkan image object from KTX data.
 *
 * Creates a VkImage with format etc. matching the KTX data and uploads the
 * images. Also creates VkSampler and VkImageView objects for accessing the
 * image. Returns the
 * handles of the created objects and information about the texture in the
 * ktxVulkanTexture pointed at by @p pTexture.
 *
 * Most Vulkan implementations support VK_IMAGE_TILING_LINEAR only for a very
 * limited number of formats and features. Generally VK_IMAGE_TILING_OPTIMAL is
 * preferred. The latter requires a staging buffer so will use more memory
 * during loading.
 *
 * @param [in] reader		handle of the ktxReader opened on the data.
 * @anchor vdiparam
 * @param [in] vdi          pointer to a ktxVulkanDeviceInfo structure providing
 *                          information about the Vulkan device onto which to
 *                          load the texture.
 * @anchor ptextureparam
 * @param [in,out] pTexture pointer to a ktxVulkanTexture structure into which
 *                          the function writes information about the created
 *                          VkImage.
 * @anchor tilingparam
 * @param [in] tiling       type of tiling to use in the destination image
 *                          on the Vulkan device.
 * @anchor usageflagsparam
 * @param [in] usageFlags   a set of VkImageUsageFlagsBits indicating the
 *                          intended usage of the destination image.
 * @anchor pkvdlenparam
 * @param [in,out] pKvdLen	if not NULL, @p *pKvdLen is set to the number of
 *                          bytes of key-value data pointed at by @p *ppKvd.
 *                          Must not be NULL, if @p ppKvd is not NULL.
 * @anchor ppkvdparam
 * @param [in,out] ppKvd	if not NULL, @p *ppKvd is set to the point to a
 *                          block of memory containing key-value data read from
 *                          the file. The application is responsible for
 *                          freeing the memory.
 *
 * @return	KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p reader, @p vdi or @p pTexture is @c NULL or
 *                              @p reader is not a valid reader handle.
 * @exception KTX_INVALID_OPERATION There is no VkFormat that matches the KTX
 *                                  data or the Vulkan device does not support
 *                                  one or more of the requested @p usageFlags
 *                                  or requested @p tiling with this format.
 * @exception KTX_OUT_OF_MEMORY Sufficient memory could not be allocated
 *                              on either the CPU or the Vulkan device.
 *
 * Error conditions from ktxReader_readHeader() and ktxReader_readKVData()
 * may also be returned.
 */
KTX_error_code
ktxReader_loadVkTextureEx(KTX_reader reader, ktxVulkanDeviceInfo* vdi,
                          ktxVulkanTexture* pTexture,
                          VkImageTiling tiling, VkImageUsageFlags usageFlags,
                          unsigned int* pKvdLen, unsigned char** ppKvd)
{
    ktxReader*               This = (ktxReader*)reader;
    KTX_header               header;
    KTX_supplemental_info    texinfo;
    KTX_error_code           errorCode;
    VkFormat				 vkFormat;
    VkImageType              imageType;
    VkImageViewType          viewType;
    VkImageCreateFlags       createFlags = 0;
    VkImageFormatProperties  formatProperties;
    VkResult                 result;
    VkCommandBufferBeginInfo cmdBufBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL
    };
    VkImageCreateInfo        imageCreateInfo = {
         .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
         .pNext = NULL
    };
    VkSamplerCreateInfo sampler;
    VkImageViewCreateInfo view;
    VkMemoryAllocateInfo     memAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = 0,
        .memoryTypeIndex = 0
    };
    VkMemoryRequirements     memReqs;
    VkBool32                 isArray;
    VkBool32                 isCube = VK_FALSE;
    uint32_t                 arrayLayers;

    if (ppKvd) {
        *ppKvd = NULL;
    }

    if (!vdi) {
        return KTX_INVALID_VALUE;
    }

    if (!This || !This->stream.read || !This->stream.skip) {
        return KTX_INVALID_VALUE;
    }

    if (!pTexture) {
        return KTX_INVALID_VALUE;
    }

    errorCode = ktxReader_readHeader(This, &header, &texinfo);
    if (errorCode != KTX_SUCCESS)
        return errorCode;

    ktxReader_readKVData(This, pKvdLen, ppKvd);
    if (errorCode != KTX_SUCCESS) {
        return errorCode;
    }

    /* _ktxCheckHeader should have caught this. */
    assert(header.numberOfFaces == 6 ? texinfo.textureDimension == 2 : VK_TRUE);

    if (header.numberOfArrayElements > 0) {
        arrayLayers = header.numberOfArrayElements;
        isArray = VK_TRUE;
    } else {
        arrayLayers = 1;
        isArray = VK_FALSE;
    }
    if (header.numberOfFaces == 6) {
        arrayLayers *= 6;
        createFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        isCube = VK_TRUE;
    }

    pTexture->width = header.pixelWidth;
    switch (texinfo.textureDimension) {
      case 1:
        imageType = VK_IMAGE_TYPE_1D;
        viewType = isArray ?
                        VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
        pTexture->height = pTexture->depth = 1;
        break;
      case 2:
        imageType = VK_IMAGE_TYPE_2D;
        pTexture->height = header.pixelHeight;
        pTexture->depth = 1;
        if (isCube)
            viewType = isArray ?
                        VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
        else
            viewType = isArray ?
                        VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
        break;
      case 3:
        imageType = VK_IMAGE_TYPE_3D;
        pTexture->height = header.pixelHeight;
        pTexture->depth = header.pixelWidth;
        assert(!isArray); // 3D array textures not yet supported in Vulkan.
        viewType = VK_IMAGE_VIEW_TYPE_3D;
        break;
    }

    pTexture->mipLevels = header.numberOfMipmapLevels;
    pTexture->layerCount = arrayLayers;

    vkFormat = vkGetFormatFromOpenGLInternalFormat(header.glInternalFormat);
    if (vkFormat == VK_FORMAT_UNDEFINED)
    	vkFormat = vkGetFormatFromOpenGLFormat(header.glFormat, header.glType);
    if (vkFormat == VK_FORMAT_UNDEFINED) {
        return KTX_INVALID_OPERATION;
    }

    // Get device properties for the requested texture format
    result = vkGetPhysicalDeviceImageFormatProperties(vdi->physicalDevice,
    												  vkFormat,
                                                      imageType,
                                                      tiling,
                                                      usageFlags,
                                                      createFlags,
                                                      &formatProperties);
    if (result == VK_ERROR_FORMAT_NOT_SUPPORTED) {
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

        // Because all array layers are the same size they can be copied
        // in a single operation so there'll be 1 copy per mip level.
        uint32_t numCopyRegions = header.numberOfMipmapLevels * header.numberOfFaces;
        user_cbdata_optimal cbData;

        (void)ktxReader_getDataSize(reader, (size_t*)&textureSize);

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

        result = vkAllocateMemory(vdi->device, &memAllocInfo,
                                  vdi->pAllocator, &stagingMemory);
        if (result != VK_SUCCESS) {
            return KTX_OUT_OF_MEMORY;
        }
        VK_CHECK_RESULT(vkBindBufferMemory(vdi->device, stagingBuffer,
                                           stagingMemory, 0));

        VK_CHECK_RESULT(vkMapMemory(vdi->device, stagingMemory, 0,
                                    memReqs.size, 0, (void **)&cbData.dest));

        cbData.offset = 0;
        cbData.region = copyRegions;
#if defined(_DEBUG)
        cbData.stagingBufferSize = (size_t)memAllocInfo.allocationSize;
        cbData.regionsArrayEnd = copyRegions + numCopyRegions;
#endif

        // Call ReadImages to copy texture data into staging buffer
        errorCode = ktxReader_readImages((void*)This, optimalTilingCallback, &cbData);
        // XXX Check for possible errors

        vkUnmapMemory(vdi->device, stagingMemory);

        // Create optimal tiled target image
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = vkFormat;
        imageCreateInfo.mipLevels = pTexture->mipLevels;
        imageCreateInfo.arrayLayers = arrayLayers;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = usageFlags;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.extent.width = pTexture->width;
        imageCreateInfo.extent.height = pTexture->height;
        imageCreateInfo.extent.depth = pTexture->depth;
        imageCreateInfo.usage
                = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        VK_CHECK_RESULT(vkCreateImage(vdi->device, &imageCreateInfo,
                                      vdi->pAllocator, &pTexture->image));

        vkGetImageMemoryRequirements(vdi->device, pTexture->image, &memReqs);

        memAllocInfo.allocationSize = memReqs.size;

        memAllocInfo.memoryTypeIndex = ktxVulkanDeviceInfo_getMemoryType(
                                          vdi, memReqs.memoryTypeBits,
                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VK_CHECK_RESULT(vkAllocateMemory(vdi->device, &memAllocInfo, vdi->pAllocator,
                                         &pTexture->deviceMemory));
        VK_CHECK_RESULT(vkBindImageMemory(vdi->device, pTexture->image,
                                          pTexture->deviceMemory, 0));

        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = pTexture->mipLevels;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = arrayLayers;

        // Image barrier for optimal image (target)
        // Optimal image will be used as destination for the copy
        setImageLayout(
            vdi->cmdBuffer,
            pTexture->image,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            subresourceRange);

        // Copy mip levels from staging buffer
        vkCmdCopyBufferToImage(
            vdi->cmdBuffer, stagingBuffer,
            pTexture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            numCopyRegions, copyRegions
            );

        // Change texture image layout to shader read after all mip levels have been copied
        pTexture->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        setImageLayout(
            vdi->cmdBuffer,
            pTexture->image,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            pTexture->imageLayout,
            subresourceRange);

        // Submit command buffer containing copy and image layout commands
        VK_CHECK_RESULT(vkEndCommandBuffer(vdi->cmdBuffer));

        // Create a fence to make sure that the copies have finished before continuing
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

        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = vkFormat;
        imageCreateInfo.extent.width = pTexture->width;
        imageCreateInfo.extent.height = pTexture->height;
        imageCreateInfo.extent.depth = pTexture->depth;
        imageCreateInfo.mipLevels = pTexture->mipLevels;
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
        result = vkAllocateMemory(vdi->device, &memAllocInfo, vdi->pAllocator,
                                  &mappableMemory);
        if (result != VK_SUCCESS) {
            return KTX_OUT_OF_MEMORY;
        }
        VK_CHECK_RESULT(vkBindImageMemory(vdi->device, mappableImage, mappableMemory, 0));

        cbData.destImage = mappableImage;
        cbData.device = vdi->device;

        // Map image memory
        VK_CHECK_RESULT(vkMapMemory(vdi->device, mappableMemory, 0,
                        memReqs.size, 0, (void **)&cbData.dest));

        // Call ReadImages to copy texture data into mapped image memory.
        errorCode = ktxReader_readImages((void*)This, linearTilingCallback, &cbData);
        // XXX Check for possible errors

        vkUnmapMemory(vdi->device, mappableMemory);

        // Linear tiled images don't need to be staged
        // and can be directly used as textures
        pTexture->image = mappableImage;
        pTexture->deviceMemory = mappableMemory;
        pTexture->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = pTexture->mipLevels;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = arrayLayers;

        // Setup image memory barrier
        setImageLayout(
            vdi->cmdBuffer,
            pTexture->image,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_PREINITIALIZED,
            pTexture->imageLayout,
            subresourceRange);

        // Submit command buffer containing image layout commands
        VK_CHECK_RESULT(vkEndCommandBuffer(vdi->cmdBuffer));

        submitInfo.waitSemaphoreCount = 0;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &vdi->cmdBuffer;

        VK_CHECK_RESULT(vkQueueSubmit(vdi->queue, 1, &submitInfo, nullFence));
        VK_CHECK_RESULT(vkQueueWaitIdle(vdi->queue));
    }

    // Create sampler.
    sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler.magFilter = VK_FILTER_LINEAR;
    sampler.minFilter = VK_FILTER_LINEAR;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.mipLodBias = 0.0f;
    sampler.unnormalizedCoordinates = VK_FALSE;
    sampler.compareEnable = VK_FALSE;
    sampler.compareOp = VK_COMPARE_OP_NEVER;
    sampler.minLod = 0.0f;
    // Max level-of-detail should match mip level count
    sampler.maxLod = (float)pTexture->mipLevels;
    // Enable anisotropic filtering
    sampler.maxAnisotropy = 8;
    sampler.anisotropyEnable = VK_TRUE;
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    VK_CHECK_RESULT(vkCreateSampler(vdi->device, &sampler,
                                    vdi->pAllocator, &pTexture->sampler));

    // Create image view.
    // Textures are not directly accessed by the shaders and are abstracted
    // by image views containing additional information and sub resource
    // ranges.
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.pNext = NULL;
    view.image = VK_NULL_HANDLE;
    view.viewType = viewType;
    view.format = vkFormat;
    view.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    view.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    view.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    view.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view.subresourceRange.baseMipLevel = 0;
    view.subresourceRange.levelCount = pTexture->mipLevels;
    view.subresourceRange.baseArrayLayer = 0;
    view.subresourceRange.layerCount = arrayLayers;
    view.image = pTexture->image;
    VK_CHECK_RESULT(vkCreateImageView(vdi->device, &view,
                                      vdi->pAllocator, &pTexture->view));

    // Fill descriptor image info that can be used for setting up descriptor sets
    pTexture->descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    pTexture->descriptor.imageView = pTexture->view;
    pTexture->descriptor.sampler = pTexture->sampler;

    return KTX_SUCCESS;
}

/** @ingroup reader
 * @~English
 * @brief Create a Vulkan image object from KTX data.
 *
 * Calls ktxReader_LoadVkTextureEx() with the most commonly used options:
 * VK_IMAGE_TILING_OPTIMAL and VK_IMAGE_USAGE_SAMPLED_BIT.
 * 
 * @sa ktxReader_LoadVkTextureEx() for details and use that for complete
 *     control.
 */
KTX_error_code
ktxReader_loadVkTexture(KTX_reader reader, ktxVulkanDeviceInfo* vdi,
                 ktxVulkanTexture *texture,
                 unsigned int* pKvdLen, unsigned char** ppKvd)
{
    return ktxReader_loadVkTextureEx(reader, vdi, texture,
                                     VK_IMAGE_TILING_OPTIMAL,
                                     VK_IMAGE_USAGE_SAMPLED_BIT,
                                     pKvdLen, ppKvd);
}

/**
 * @~English
 * @brief Create a Vulkan image object from KTX data in a stdio FILE.
 *
 * Calls ktxReader_LoadVkTextureEx() to do the bulk of the work.
 *
 * @param [in] file       stdio FILE pointer
 * @param [in] vdi        see @ref vdiparam "same parameter" of
 *                        ktxReader_LoadVkTextureEx().
 * @param [in,out] pTexture see @ref ptextureparam "same parameter" of
 *                          ktxReader_LoadVkTextureEx().
 * @param [in] tiling     see @ref tilingparam "same parameter" of
 *                        ktxReader_LoadVkTextureEx().
 * @param [in] usageFlags see @ref usageflagsparam "same parameter" of
 *                        ktxReader_LoadVkTextureEx().
 * @param [in,out] pKvdLen  see @ref pkvdlenparam "same parameter" of
 *                          ktxReader_LoadVkTextureEx().
 * @param [in,out] ppKvd    see @ref ppkvdparam "same parameter" of
 *                          ktxReader_LoadVkTextureEx().
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @sa ktxReader_LoadVkTextureEx() for description of parameters
 *     and errors.
 */
KTX_error_code
ktxLoadVkTextureExF(ktxVulkanDeviceInfo* vdi, FILE* file,
                    ktxVulkanTexture *pTexture,
                    VkImageTiling tiling,
                    VkImageUsageFlags usageFlags,
                    unsigned int* pKvdLen, unsigned char** ppKvd)
{
    KTX_reader reader;
    KTX_error_code errorCode = KTX_SUCCESS;

    errorCode = ktxOpenKTXF(file, &reader);
    if (errorCode != KTX_SUCCESS)
        return errorCode;

    errorCode = ktxReader_loadVkTextureEx(reader, vdi, pTexture,
                                          tiling, usageFlags,
                                          pKvdLen, ppKvd);
    ktxReader_close(reader);

    return errorCode;
}

/**
 * @~English
 * @brief Create a Vulkan image object from KTX data in a stdio FILE.
 *
 * Calls ktxLoadVkTextureExF() with the most commonly used options:
 * VK_IMAGE_TILING_OPTIMAL and VK_IMAGE_USAGE_SAMPLED_BIT.
 *
 * @sa ktxLoadVkTextureExF()
 * @sa ktxReader_LoadVkTextureEx() for parameter and error details
 */
KTX_error_code
ktxLoadVkTextureF(ktxVulkanDeviceInfo* vdi, FILE* file,
                         ktxVulkanTexture *pTexture,
                         unsigned int* pKvdLen, unsigned char** ppKvd)
{
    return ktxLoadVkTextureExF(vdi, file, pTexture,
                               VK_IMAGE_TILING_OPTIMAL,
                               VK_IMAGE_USAGE_SAMPLED_BIT,
                               pKvdLen, ppKvd);
}

/**
 * @~English
 * @brief Create a Vulkan image object from KTX data in a named file on disk.
 *
 * ktxReader_LoadVkTextureEx() does the bulk of the work.
 *
 * @param [in] filename   pointer to a C string that contains the path of
 *                        the file to load
 * @param [in] vdi        see @ref vdiparam "same parameter" of
 *                        ktxReader_LoadVkTextureEx().
 * @param [in,out] pTexture see @ref ptextureparam "same parameter" of
 *                          ktxReader_LoadVkTextureEx().
 * @param [in] tiling     see @ref tilingparam "same parameter" of
 *                        ktxReader_LoadVkTextureEx().
 * @param [in] usageFlags see @ref usageflagsparam "same parameter" of
 *                        ktxReader_LoadVkTextureEx().
 * @param [in,out] pKvdLen  see @ref pkvdlenparam "same parameter" of
 *                          ktxReader_LoadVkTextureEx().
 * @param [in,out] ppKvd    see @ref ppkvdparam "same parameter" of
 *                          ktxReader_LoadVkTextureEx().
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_FILE_OPEN_FAILED  The specified file could not be opened.
 *
 * @sa ktxReader_LoadVkTextureEx() for description of parameters
 *     and errors.
 */
KTX_error_code
ktxLoadVkTextureExN(ktxVulkanDeviceInfo* vdi, const char* const filename,
                    ktxVulkanTexture *pTexture,
                    VkImageTiling tiling,
                    VkImageUsageFlags usageFlags,
                    unsigned int* pKvdLen, unsigned char** ppKvd)
{
    KTX_error_code errorCode;
    FILE* file = fopen(filename, "rb");

    if (file) {
        errorCode = ktxLoadVkTextureExF(vdi, file, pTexture,
                                        tiling, usageFlags,
                                        pKvdLen, ppKvd);
        fclose(file);
    } else
        errorCode = KTX_FILE_OPEN_FAILED;

    return errorCode;
}

/**
 * @~English
 * @brief Create a Vulkan image object from KTX data in a named file on disk.
 *
 * Calls ktxLoadVkTextureExN() with the most commonly used options:
 * VK_IMAGE_TILING_OPTIMAL and VK_IMAGE_USAGE_SAMPLED_BIT.
 *
 * @sa ktxLoadVkTextureExN()
 * @sa ktxReader_LoadVkTextureEx() for parameter and error details
 */
KTX_error_code
ktxLoadVkTextureN(ktxVulkanDeviceInfo* vdi, const char* const filename,
                  ktxVulkanTexture *pTexture,
                  unsigned int* pKvdLen, unsigned char** ppKvd)
{
    return ktxLoadVkTextureExN(vdi, filename, pTexture,
                               VK_IMAGE_TILING_OPTIMAL,
                               VK_IMAGE_USAGE_SAMPLED_BIT,
                               pKvdLen, ppKvd);
}

/**
 * @~English
 * @brief Create a Vulkan image object from KTX formatted data in memory.
 *
 * ktxReader_LoadVkTextureEx() does the bulk of the work.
 *
 * @param [in] bytes      pointer to the location of the KTX data.
 * @param [in] size       length of the KTX data.
 * @param [in] vdi        see @ref vdiparam "same parameter" of
 *                        ktxReader_LoadVkTextureEx().
 * @param [in,out] pTexture see @ref ptextureparam "same parameter" of
 *                          ktxReader_LoadVkTextureEx().
 * @param [in] tiling     see @ref tilingparam "same parameter" of
 *                        ktxReader_LoadVkTextureEx().
 * @param [in] usageFlags see @ref usageflagsparam "same parameter" of
 *                        ktxReader_LoadVkTextureEx().
 * @param [in,out] pKvdLen  see @ref pkvdlenparam "same parameter" of
 *                          ktxReader_LoadVkTextureEx().
 * @param [in,out] ppKvd    see @ref ppkvdparam "same parameter" of
 *                          ktxReader_LoadVkTextureEx().
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @sa ktxReader_LoadVkTextureEx() for description of parameters
 *     and errors.
 */
KTX_error_code
ktxLoadVkTextureExM(ktxVulkanDeviceInfo* vdi,
                    const void* bytes, GLsizei size,
                    ktxVulkanTexture* pTexture,
                    VkImageTiling tiling,
                    VkImageUsageFlags usageFlags,
                    unsigned int* pKvdLen, unsigned char** ppKvd)
{
    KTX_reader reader;
    KTX_error_code errorCode = KTX_SUCCESS;

    errorCode = ktxOpenKTXM(bytes, size, &reader);
    if (errorCode != KTX_SUCCESS)
        return errorCode;

    errorCode = ktxReader_loadVkTextureEx(reader, vdi, pTexture,
                                          tiling, usageFlags,
                                          pKvdLen, ppKvd);
    ktxReader_close(reader);

    return errorCode;
}

/**
 * @~English
 * @brief Create a Vulkan image object from KTX formattted data in memory.
 *
 * Calls ktxLoadVkTextureExM() with the most commonly used options:
 * VK_IMAGE_TILING_OPTIMAL and VK_IMAGE_USAGE_SAMPLED_BIT.
 *
 * @sa ktxLoadVkTextureExM()
 * @sa ktxReader_LoadVkTextureEx() for parameter and error details
 */
KTX_error_code
ktxLoadVkTextureM(ktxVulkanDeviceInfo* vdi,
                  const void* bytes, GLsizei size,
                  ktxVulkanTexture* texture,
                  unsigned int* pKvdLen, unsigned char** ppKvd)
{
    return ktxLoadVkTextureExM(vdi, bytes, size, texture,
                               VK_IMAGE_TILING_OPTIMAL,
                               VK_IMAGE_USAGE_SAMPLED_BIT,
                               pKvdLen, ppKvd);
}

/**
 * @}
 */

//======================================================================
//  Utilities
//======================================================================

/*
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
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
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

    imageMemoryBarrier.oldLayout = oldImageLayout;
    imageMemoryBarrier.newLayout = newImageLayout;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = subresourceRange;

    // Source layouts (old)
    // The source access mask controls actions to be finished on the old
    // layout before it will be transitioned to the new layout.
    switch (oldImageLayout)
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
    switch (newImageLayout)
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
    VkPipelineStageFlags srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags destStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

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
 * @~English
 * @brief Destructor for the object returned when loading a texture image.
 *
 * Frees the Vulkan resources created when the texture image was loaded.
 *
 * @param texture    pointer to the ktxVulkanTexture to be destructed.
 * @param device     handle to the Vulkan logical device to which the texture was
 *                   loaded.
 * @param pAllocator pointer to the allocator used during loading.
 */
void
ktxVulkanTexture_destruct(ktxVulkanTexture* texture, VkDevice device,
						  const VkAllocationCallbacks* pAllocator)
{
    vkDestroyImageView(device, texture->view, pAllocator);
    vkDestroyImage(device, texture->image, pAllocator);
    vkDestroySampler(device, texture->sampler, pAllocator);
    vkFreeMemory(device, texture->deviceMemory, pAllocator);
}

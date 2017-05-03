/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

#ifndef KTX_H_C54B42AEE39611E68E1E4FF8C51D1C66
#define KTX_H_C54B42AEE39611E68E1E4FF8C51D1C66

/**
 * @internal
 * @file
 * @~English
 *
 * @brief Declares the public functions and structures of the
 *        KTX Vulkan texture loading API.
 *
 * A separate header file is used to avoid extra dependencies for those not
 * using Vulkan. The nature of the Vulkan API, rampant structures and enums,
 * means that vulkan.h must be included. The alternative is duplicating
 * unattractively large parts of it.
 *
 * @author Mark Callow, Edgewise Consulting
 *
 * $Date$
 */

#include <ktx.h>
#include <vulkan/vulkan.h>

#if 0
/* Avoid Vulkan include file */
#define VK_DEFINE_HANDLE(object) typedef struct object##_T* object;

#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
        #define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef struct object##_T *object;
#else
        #define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef uint64_t object;
#endif

VK_DEFINE_HANDLE(VkPhysicalDevice)
VK_DEFINE_HANDLE(VkDevice)
VK_DEFINE_HANDLE(VkQueue)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkCommandPool)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDeviceMemory)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkImage)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkImageView)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSampler)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Struct for receiving information about the Vulkan image created
 *        by the texture image loading functions.
 */
typedef struct ktxVulkanTexture
{
    /** Handle to the sampler created if @c VK_IMAGE_USAGE_SAMPLED_BIT is
      * passed to the loading function. */
    VkSampler sampler;
    VkImage image; /*!< Handle to the Vulkan image created by the loader. */
    // This is only ever set to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL so
    // perhaps it is not needed. It's an enum. Is "enum VkImageView;" sufficient?
    VkImageLayout imageLayout; /*!< The layout of the created image. */
    /** The memory allocated for the image on the Vulkan device. */
    VkDeviceMemory deviceMemory;
    VkImageView view; /*!< Handle to the VkImageView created for the image. */
    uint32_t width; /*!< The width of the image. */
    uint32_t height; /*!< The height of the image. */
    uint32_t depth; /*!< The depth of the image. */
    uint32_t mipLevels; /*!< The number of MIP levels in the image. */
    uint32_t layerCount; /*!< The number of array layers in the image. */
    // The information this is duplicated above so perhaps this can be removed
    // too.
    VkDescriptorImageInfo descriptor; /*<! Descriptor image info. */
} ktxVulkanTexture;

void
ktxVulkanTexture_destruct(ktxVulkanTexture* texture, VkDevice device,
						  const VkAllocationCallbacks* pAllocator);

/**
 * @brief Struct for passing information about the Vulkan device on which
 *        to create images to the texture image loading functions.
 *
 * Avoids passing a large number of parameters to each loading function.
 * Use of ktxVulkanDeviceInfo_create() or ktxVulkanDeviceInfo_construct() to
 * populate this structure is highly recommended.
 *
 * @code
    ktxVulkanDeviceInfo vdi;
    ktxVulkanTexture texture;
 
    vdi = ktxVulkanDeviceInfo_create(physicalDevice,
                                     device,
                                     queue,
                                     cmdPool,
                                     &allocator);
    ktxLoadVkTextureN("texture_1.ktx", vdi, &texture, NULL, NULL);
    // ...
    ktxLoadVkTextureN("texture_n.ktx", vdi, &texture, NULL, NULL);
    ktxVulkanDeviceInfo_destroy(vdi);
 * @endcode
 */
typedef struct ktxVulkanDeviceInfo {
    VkPhysicalDevice physicalDevice; /*!< Handle of the physical device. */
    VkDevice device; /*!< Handle of the logical device. */
    VkQueue queue; /*!< Handle to the queue to which to submit commands. */
    VkCommandBuffer cmdBuffer; /*!< Handle of the cmdBuffer to use. */
    /** Handle of the command pool from which to allocate the command buffer. */
    VkCommandPool cmdPool;
    /** Pointer to the allocator to use for the command buffer and created
     * images.
     */
    const VkAllocationCallbacks* pAllocator;
    /** Memory properties of the Vulkan physical device. */
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
} ktxVulkanDeviceInfo;

ktxVulkanDeviceInfo*
ktxVulkanDeviceInfo_create(VkPhysicalDevice physicalDevice, VkDevice device,
                           VkQueue queue, VkCommandPool cmdPool,
						   const VkAllocationCallbacks* pAllocator);
KTX_error_code
ktxVulkanDeviceInfo_construct(ktxVulkanDeviceInfo* vdi,
                         VkPhysicalDevice physicalDevice, VkDevice device,
                         VkQueue queue, VkCommandPool cmdPool,
						 const VkAllocationCallbacks* pAllocator);
void
ktxVulkanDeviceInfo_destruct(ktxVulkanDeviceInfo* vdi);
void
ktxVulkanDeviceInfo_destroy(ktxVulkanDeviceInfo* vdi);


KTX_error_code
ktxReader_LoadVkTextureEx(KTX_reader This, ktxVulkanDeviceInfo* vdi,
                          ktxVulkanTexture* pTexture,
                          VkImageTiling tiling, VkImageUsageFlags usageFlags,
                          unsigned int* pKvdLen, unsigned char** ppKvd);

KTX_error_code
ktxReader_LoadVkTexture(KTX_reader This, ktxVulkanDeviceInfo* vdi,
                        ktxVulkanTexture *texture,
                        unsigned int* pKvdLen, unsigned char** ppKvd);

KTX_error_code
ktxLoadVkTextureExF(FILE* file, ktxVulkanDeviceInfo* vdi,
                    ktxVulkanTexture *texture,
                    VkImageTiling tiling,
                    VkImageUsageFlags imageUsageFlags,
                    unsigned int* pKvdLen, unsigned char** ppKvd);

KTX_error_code
ktxLoadVkTextureF(FILE* file, ktxVulkanDeviceInfo* vdi,
                  ktxVulkanTexture *texture,
                  unsigned int* pKvdLen, unsigned char** ppKvd);

KTX_error_code
ktxLoadVkTextureExN(const char* const filename, ktxVulkanDeviceInfo* vdi,
                    ktxVulkanTexture *texture,
                    VkImageTiling tiling,
                    VkImageUsageFlags imageUsageFlags,
                    unsigned int* pKvdLen, unsigned char** ppKvd);

KTX_error_code
ktxLoadVkTextureN(const char* const filename, ktxVulkanDeviceInfo* vdi,
                  ktxVulkanTexture *texture,
                  unsigned int* pKvdLen, unsigned char** ppKvd);

KTX_error_code
ktxLoadVkTextureExM(const void* bytes, GLsizei size,
					ktxVulkanDeviceInfo* vdi,
		            ktxVulkanTexture* texture,
                    VkImageTiling tiling,
                    VkImageUsageFlags imageUsageFlags,
                    unsigned int* pKvdLen, unsigned char** ppKvd);

KTX_error_code
ktxLoadVkTextureM(const void* bytes, GLsizei size,
				  ktxVulkanDeviceInfo* vdi,
		          ktxVulkanTexture* texture,
                  unsigned int* pKvdLen, unsigned char** ppKvd);
  
#ifdef __cplusplus
}
#endif
#endif /* KTX_H_A55A6F00956F42F3A137C11929827FE1 */

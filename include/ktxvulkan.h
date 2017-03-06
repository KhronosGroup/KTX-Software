/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

#ifndef KTX_H_C54B42AEE39611E68E1E4FF8C51D1C66
#define KTX_H_C54B42AEE39611E68E1E4FF8C51D1C66

/**
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
 * The loader fills this in with information about the loaded texture.
 */
typedef struct ktxVulkanTexture
{
    VkSampler sampler;
    VkImage image;
    // This is only ever set to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL so
    // perhaps it is not needed. It's an enum. Is "enum VkImageView;" sufficient?
    VkImageLayout imageLayout;
    VkDeviceMemory deviceMemory;
    VkImageView view;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t mipLevels;
    uint32_t layerCount;
    // The information this is duplicated above so perhaps this can be removed
    // too.
    VkDescriptorImageInfo descriptor;
} ktxVulkanTexture;

void
ktxVulkanTexture_destruct(ktxVulkanTexture* texture, VkDevice device,
						  const VkAllocationCallbacks* pAllocator);

typedef struct ktxVulkanDeviceInfo {
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue queue;
    VkCommandBuffer cmdBuffer;
    VkCommandPool cmdPool;
    const VkAllocationCallbacks* pAllocator;
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
ktxLoadVkTextureExF(ktxVulkanDeviceInfo* vdi, FILE* file,
                    ktxVulkanTexture *texture,
                    VkImageTiling tiling,
                    VkImageUsageFlags imageUsageFlags,
                    unsigned int* pKvdLen, unsigned char** ppKvd);

KTX_error_code
ktxLoadVkTextureF(ktxVulkanDeviceInfo* vdi, FILE* file,
                  ktxVulkanTexture *texture,
                  unsigned int* pKvdLen, unsigned char** ppKvd);

KTX_error_code
ktxLoadVkTextureExN(ktxVulkanDeviceInfo* vdi, const char* const filename,
                    ktxVulkanTexture *texture,
                    VkImageTiling tiling,
                    VkImageUsageFlags imageUsageFlags,
                    unsigned int* pKvdLen, unsigned char** ppKvd);

KTX_error_code
ktxLoadVkTextureN(ktxVulkanDeviceInfo* vdi, const char* const filename,
                  ktxVulkanTexture *texture,
                  unsigned int* pKvdLen, unsigned char** ppKvd);

KTX_error_code
ktxLoadVkTextureExM(ktxVulkanDeviceInfo* vdi,
                    const void* bytes, GLsizei size,
                    ktxVulkanTexture* texture,
                    VkImageTiling tiling,
                    VkImageUsageFlags imageUsageFlags,
                    unsigned int* pKvdLen, unsigned char** ppKvd);

KTX_error_code
ktxLoadVkTextureM(ktxVulkanDeviceInfo* vdi,
                  const void* bytes, GLsizei size,
                  ktxVulkanTexture* texture,
                  unsigned int* pKvdLen, unsigned char** ppKvd);
  
#ifdef __cplusplus
}
#endif
#endif /* KTX_H_A55A6F00956F42F3A137C11929827FE1 */

/*
 Simple DirectMedia Layer
 Copyright (C) 2017, Mark Callow.
 
 This software is provided 'as-is', without any express or implied
 warranty.  In no event will the authors be held liable for any damages
 arising from the use of this software.
 
 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:
 
 1. The origin of this software must not be misrepresented; you must not
 claim that you wrote the original software. If you use this software
 in a product, an acknowledgment in the product documentation would be
 appreciated but is not required.
 2. Altered source versions must be plainly marked as such, and must not be
 misrepresented as being the original software.
 3. This notice may not be removed or altered from any source distribution.
 */

/**
 *  \file SDL_vulkan.h
 *
 *  Header file for functions to creating Vulkan surfaces on SDL windows.
 */

#ifndef _SDL_vulkan_h
#define _SDL_vulkan_h

#include "SDL_video.h"
#if defined(USE_VULKAN_HEADER_IN_SDL_VULKAN_H)
#include <vulkan/vulkan.h>
#else
/* Avoid including vulkan.h */
#define VK_DEFINE_HANDLE(object) typedef struct object##_T* object;

#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef struct object##_T *object;
#else
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef uint64_t object;
#endif

VK_DEFINE_HANDLE(VkInstance)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSurfaceKHR)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  \brief Get the names of the Vulkan instance extensions needed to create
 *         a surface on the current video driver.
 *
 *  \param [in]  length   the length of the array pointed to by \a names
 *  \param [out] names    an array of char* into which the names of the
 *                        extensions are written. The length of the array needed
 *                        can be queried by passing NULL.
 *
 *  \return the number of extensions, or 0 on error.
 *
 *  \note The extension names queried here must be passed along when calling
 *        VkCreateInstance, otherwise surface creation will fail.
 *
 *
 *  \sa SDL_CreateVulkanSurface()
 */
extern DECLSPEC int SDLCALL SDL_GetVulkanInstanceExtensions(unsigned int length,
                                                            const char** names);

/**
 *  \brief Create a Vulkan rendering surface attached to the passed window.
 *
 *  \param [in]  window   SDL_Window to which to attach the rendering surface.
 *  \param [in]  instance handle to the Vulkan instance to use.
 *  \param [out] surface  a pointer to a VkSurfaceKHR handle to receive the
 *                        handle of the newly created surface.
 *
 *  \return 0 on success, or -1 on error.
 *
 *  \note Before calling this, the application must call
 *        SDL_GetVulkanInstanceExtensions() and pass the needed extensions along
 *        when creating the Vulkan instance \a instance.
 *
 *  \sa SDL_GetVulkanInstanceExtensions()
 */
extern DECLSPEC int SDLCALL SDL_CreateVulkanSurface(SDL_Window* window,
                                                    VkInstance instance,
                                                    VkSurfaceKHR* surface);

#ifdef __cplusplus
}
#endif

#endif /* _SDL_vulkan_h */

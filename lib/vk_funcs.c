/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab textwidth=70: */

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

/**
 * @internal
 * @file vk_funcs.c
 * @~English
 *
 * @brief Retrieve Vulkan function pointers needed by libktx
 */

#if KTX_USE_FUNCPTRS_FOR_VULKAN

#define UNUX 0
#define MACOS 0
#define WINDOWS 0

#if defined(_WIN32)
#undef WINDOWS
#define WINDOWS 1
#endif
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__bsdi__) || defined(__DragonFly__)
#undef UNIX
#define UNIX 1
#endif
#if defined(linux) || defined(__linux) || defined(__linux__)
#undef UNIX
#define UNIX 1
#endif
#if defined(__APPLE__) && defined(__x86_64__)
#undef MACOS
#define MACOS 1
#endif

#if (IOS + MACOS + UNIX + WINDOWS) > 1
#error Multiple OS's defined
#endif 

#if WINDOWS
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include "ktx.h"
#include "vk_funcs.h"

#if WINDOWS
HMODULE ktxVulkanLibary;
#define LoadLibrary LoadLibrary
#define LoadProcAddr GetProcAddress
#elif MACOS || UNIX
#define LoadLibrary dlopen
#define LoadProcAddr dlsym
void* ktxVulkanLibrary;
#else
#error Don't know how to load symbols on this OS.
#endif

#if WINDOWS
#define VULKANLIB "vulkan-1.dll"
#elif MACOS
#define VULKANLIB "vulkan.framework/vulkan"
#elif UNIX
#define VULKANLIB "libvulkan.so"
#endif

static PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;

/* Define pointers for functions libktx is using. */
#define VK_FUNCTION(fun) PFN_##fun ktx_##fun;

#include "vk_funclist.inl"

#undef VK_FUNCTION

#define VK_FUNCTION(fun)                                                   \
  if ( !(ktx_##fun = (PFN_##fun)vkGetInstanceProcAddr(ktxVulkanLibrary, #fun )) ) {      \
    fprintf(stderr, "Could not load vulkan function: %s!\n", #fun);        \
    return KTX_FILE_OPEN_FAILED;                                           \
  }

ktxResult
ktxVulkanLoadLibrary()
{
    if (ktxVulkanLibrary)
        return KTX_SUCCESS;

    ktxVulkanLibrary = LoadLibrary(VULKANLIB, RTLD_LAZY);
    if (ktxVulkanLibrary == NULL)
        return(KTX_FILE_OPEN_FAILED);

    vkGetInstanceProcAddr =
            (PFN_vkGetInstanceProcAddr)LoadProcAddr(ktxVulkanLibrary,
                                                    "vkGetInstanceProcAddr");
    if (!vkGetInstanceProcAddr)
        return(KTX_FILE_OPEN_FAILED);

#include "vk_funclist.inl"

    return KTX_SUCCESS;
}

#undef VK_FUNCTION

#else

extern int keepISOCompilerHappy;

#endif /* KTX_USE_FUNCPTRS_FOR_VULKAN */

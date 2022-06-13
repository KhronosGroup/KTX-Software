/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab textwidth=70: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file vk_funcs.c
 * @~English
 *
 * @brief Retrieve Vulkan function pointers needed by libktx
 */

#define UNIX 0
#define MACOS 0
#define WINDOWS 0
#define IOS 0

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
#if defined(__APPLE__) && (defined(__arm64__) || defined (__arm__))
#undef IOS
#define IOS 1
#endif
#if (IOS + MACOS + UNIX + WINDOWS) > 1
#error "Multiple OS\'s defined"
#endif

#include "vk_funcs.h"


#if WINDOWS
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#include <stdlib.h>
#endif
#include "ktx.h"

#if WINDOWS
HMODULE ktxVulkanModuleHandle;
#define GetVulkanModuleHandle(flags) ktxGetVulkanModuleHandle()
#define LoadProcAddr GetProcAddress
#elif MACOS || UNIX || IOS
// Using NULL returns a handle that can be used to search the process that
// loaded us and any other libraries it has loaded. That's all we need to
// search as the app is responsible for creating the GL context so it must
// be there.
#define GetVulkanModuleHandle(flags) dlopen(NULL, flags)
#define LoadProcAddr dlsym
void* ktxVulkanModuleHandle;
#else
#error "Don\'t know how to load symbols on this OS."
#endif

#if WINDOWS
#define VULKANLIB "vulkan-1.dll"
static HMODULE
ktxGetVulkanModuleHandle()
{
    HMODULE module = NULL;
    GetModuleHandleExA(
		0,
		VULKANLIB,
		&module
	);
	return module;
}
#endif

ktx_error_code_e
ktxLoadVulkanLibrary(void)
{
    if (ktxVulkanModuleHandle)
        return KTX_SUCCESS;

    ktxVulkanModuleHandle = GetVulkanModuleHandle(RTLD_LAZY);
    if (ktxVulkanModuleHandle == NULL) {
        fprintf(stderr, "Vulkan lib not linked or loaded by application.\n");
        // Normal use is for this constructor to be called by an
        // application that has completed OpenGL initialization. In that
        // case the only cause for failure would be a coding error in our
        // library loading. The only other cause would be an application
        // calling GLUpload without having initialized OpenGL.
#if defined(DEBUG)
        abort();
#else
        return KTX_LIBRARY_NOT_LINKED; // So release version doesn't crash.
#endif
    }

    return KTX_SUCCESS;
}

PFN_vkVoidFunction
ktxLoadVulkanFunction(const char* pName) {
    ktx_error_code_e rc = ktxLoadVulkanLibrary();
    if (rc != KTX_SUCCESS) {
        return NULL;
    }

    PFN_vkVoidFunction pfn
           = (PFN_vkVoidFunction)LoadProcAddr(ktxVulkanModuleHandle, pName);
    if (pfn == NULL) {
        fprintf(stderr, "Couldn't load Vulkan command: %s\n", pName);
        return NULL;
    }
    return pfn;
}




/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab textwidth=70: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file vk_funcs.h
 * @~English
 *
 * @brief Declare pointers for Vulkan functions.
 *
 * Dynamically retrieving pointers avoids apps having to make sure a
 * Vulkan library is available when using a shared libktx, even if
 * not using libktx's Vulkan loader.
 */

#ifndef _VK_FUNCS_H_
#define _VK_FUNCS_H_

#if !defined(VK_NO_PROTOTYPES)
#define VK_NO_PROTOTYPES
#endif

#include "vulkan/vk_platform.h"
#include "vulkan/vulkan_core.h"
#include "ktx.h"


#if WINDOWS
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
extern HMODULE ktxVulkanModuleHandle;
#else
extern void* ktxVulkanModuleHandle;
#endif

ktx_error_code_e ktxLoadVulkanLibrary(void);

// This is used to load instance functions through libktx's methods.
PFN_vkVoidFunction ktxLoadVulkanFunction(const char* pName);


#endif /* _VK_FUNCS_H_ */


/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

#ifndef VULKAN_CHECK_RES_H_1456211188
#define VULKAN_CHECK_RES_H_1456211188

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <iomanip>
#include <sstream>

//#include <SDL2/SDL.h>
#include <SDL2/SDL_messagebox.h>

/**
 * @internal
 * @file vulkancheckres.h
 * @~English
 *
 * @brief Check result of a Vulkan command.
 *
 * Use for commands that will always succeed unless usage is invalid.
 */
#if defined(DEBUG)
#define VK_CHECK_RESULT(f)                                                    \
{                                                                             \
    VkResult res = (f);                                                       \
    if (res != VK_SUCCESS)                                                    \
    {                                                                         \
        std::stringstream msg;                                                \
        msg << "Fatal : VkResult is \"" << res << "\" in " << __FILE__        \
            << " at line " << __LINE__ << std::endl;                          \
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,                        \
                                 "VkSample_02_cube_textured",                 \
                                 msg.str().c_str(),                           \
                                 NULL);                                       \
        assert(res == VK_SUCCESS);                                            \
    }                                                                         \
}
#else
#define VK_CHECK_RESULT(f) (void)f
#endif

#endif

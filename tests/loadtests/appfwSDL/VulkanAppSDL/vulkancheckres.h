/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

#ifndef VULKAN_CHECK_RES_H_1456211188
#define VULKAN_CHECK_RES_H_1456211188

/*
 * Copyright (c) 2017, Mark Callow, www.edgewise-consulting.com.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * unaltered in all copies or substantial portions of the Materials.
 * Any additions, deletions, or changes to the original source files
 * must be clearly indicated in accompanying documentation.
 *
 * If only executable code is distributed, then the accompanying
 * documentation must state that "this software is based in part on the
 * work of Mark Callow."
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
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

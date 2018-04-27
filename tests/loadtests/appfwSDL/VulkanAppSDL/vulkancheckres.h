/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

#ifndef VULKAN_CHECK_RES_H_1456211188
#define VULKAN_CHECK_RES_H_1456211188

/*
 * ©2017-2018 Mark Callow.
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

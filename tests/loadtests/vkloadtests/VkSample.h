/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

#ifndef VK_SAMPLE_H
#define VK_SAMPLE_H

/* $Id: ac63511da134f2c25a9e1da86a36bc27b6198ae3 $ */

/**
 * @internal
 * @file VkLoadTests.h
 * @~English
 *
 * @brief Declaration of VkLoadTests app class.
 *
 * @author Mark Callow
 * @copyright (c) 2016, Mark Callow.
 */

/*
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


#include "VkAppSDL.h"

class VkSample {
  public:
    VkSample(const VkCommandPool commandPool, const VkDevice device,
             const VkRenderPass renderPass, VkAppSDL::Swapchain& swapchain)
       : commandPool(commandPool), device(device), renderPass(renderPass),
         swapchain(swapchain) { }

    virtual ~VkSample() { };
    virtual void initialize(const char* const szArgs,
                            const char* const szBasePath) = 0;
    virtual void finalize() = 0;
    virtual void resize(int iWidth, int iHeight) = 0;
    virtual void run(int iTimeMS) = 0;

    typedef VkSample* (*PFN_create)(const VkCommandPool commandPool,
                                    const VkDevice device,
                                    const VkRenderPass renderPass,
                                    VkAppSDL::Swapchain& swapchain,
                                    const char* const szArgs,
                                    const char* const szBasePath);

  protected:
    const VkCommandPool commandPool;
    const VkDevice device;
    const VkRenderPass renderPass;
    VkAppSDL::Swapchain& swapchain;
};

#endif /* VK_SAMPLE_H */

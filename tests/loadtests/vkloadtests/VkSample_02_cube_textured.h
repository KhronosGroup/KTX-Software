/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/* $Id$ */

/**
 * @file	VkSample_02_cube_textured.c
 * @brief	Draw a textured cube.
 */

/*
 * Copyright (c) 2016 Mark Callow
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

#ifndef VK_SAMPLE_02_CUBE_TEXTURED_H
#define VK_SAMPLE_02_CUBE_TEXTURED_H

#include "VkSample.h"

class VkSample_02_cube_textured : public VkSample {
  public:
    VkSample_02_cube_textured(const VkCommandPool commandPool,
                              const VkDevice device,
                              const VkRenderPass renderPass,
                              VkAppSDL::Swapchain& swapchain)
                         : VkSample(commandPool, device, renderPass, swapchain) { }
    virtual ~VkSample_02_cube_textured();

    // Keep separate initialize for now in case we need to return errors
    virtual void initialize(const char* const szArgs,
                            const char* const szBasePath);
    virtual void finalize();
    virtual void resize(int iWidth, int iHeight);
    virtual void run(int iTimeMS);

    static VkSample*
    create(const VkCommandPool commandPool, const VkDevice device,
           const VkRenderPass renderPass, VkAppSDL::Swapchain& swapchain,
           const char* const szArgs, const char* const szBasePath);

  protected:
    void buildCommandBuffer(int bufferIndex);
};

#endif /* VK_SAMPLE_02_CUBE_TEXTURED_H */

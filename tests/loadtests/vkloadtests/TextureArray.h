/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TEXTURE_ARRAY_H_
#define _TEXTURE_ARRAY_H_

#include <vector>

#include <ktxvulkan.h>
#include "InstancedSampleBase.h"

#include <glm/gtc/matrix_transform.hpp>

class TextureArray : public InstancedSampleBase
{
  public:
    TextureArray(VulkanContext& vkctx,
                 uint32_t width, uint32_t height,
                 const char* const szArgs,
                 const std::string sBasePath);

    static VulkanLoadTestSample*
    create(VulkanContext& vkctx,
           uint32_t width, uint32_t height,
           const char* const szArgs, const std::string sBasePath);
};

#endif /* _TEXTURE_ARRAY_H_ */

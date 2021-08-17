/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TEXTURE_3D_H_
#define _TEXTURE_3D_H_

/**
 * @internal
 * @file Texture3d.h
 * @~English
 *
 * @brief Declaration of test sample for loading and displaying the slices of a 3d texture..
 *
 * @author Mark Callow, www.edgewise-consulting.com.
 */

#include <vector>

#include "InstancedSampleBase.h"

#include <glm/gtc/matrix_transform.hpp>

class Texture3d : public InstancedSampleBase
{
  public:
    Texture3d(VulkanContext& vkctx,
                 uint32_t width, uint32_t height,
                 const char* const szArgs,
                 const std::string sBasePath);

    static VulkanLoadTestSample*
    create(VulkanContext& vkctx,
           uint32_t width, uint32_t height,
           const char* const szArgs, const std::string sBasePath);

  protected:
    virtual void addSubclassPushConstantRanges(PushConstantRanges&);
    virtual void setSubclassPushConstants(uint32_t bufferIndex);
};

#endif /* _TEXTURE_3D_H_ */

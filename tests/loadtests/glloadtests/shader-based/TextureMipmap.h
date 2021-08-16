/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2021 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file TextureMipmap.h
 * @~English
 *
 * @brief Declaration of test sample for loading and displaying all the levels of a 2D mipmapped texture.
 *
 * @author Mark Callow, www.edgewise-consulting.com.
 */

#include <vector>

#include "InstancedSampleBase.h"

#include <glm/gtc/matrix_transform.hpp>

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION false

class TextureMipmap : public InstancedSampleBase
{
  public:
    TextureMipmap(uint32_t width, uint32_t height,
                 const char* const szArgs,
                 const std::string sBasePath);

    static LoadTestSample*
    create(uint32_t width, uint32_t height,
           const char* const szArgs, const std::string sBasePath);

  protected:
    using ShaderSource = GL3LoadTestSample::ShaderSource;
};

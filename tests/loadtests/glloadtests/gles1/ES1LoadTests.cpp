/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2018 Mark Callow.
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

/**
 * @file    ES1LoadTests.cpp
 * @brief  List of tests of the KTX loader for OpenGL ES 1.1.
 *
 * The loader is tested by loading and drawing KTX textures in various formats
 * using the DrawTexture and TexturedCube samples.
 */

#include "GLLoadTests.h"
#include "DrawTexture.h"
#include "TexturedCube.h"

const GLLoadTests::sampleInvocation siSamples[] = {
    { DrawTexture::create,
      "--npot testimages/hi_mark.ktx",
      GLLoadTests::CompressionType::eNone,
      "RGB8 NPOT HI Logo"
    },
    { DrawTexture::create,
      "--npot testimages/luminance_reference.ktx",
      GLLoadTests::CompressionType::eNone,
      "LUMINANCE8 NPOT"
    },
    { DrawTexture::create,
      "testimages/orient-up.ktx",
      GLLoadTests::CompressionType::eNone,
      "RGB8 (no metadata)"
    },
    { DrawTexture::create,
      "testimages/orient-up-metadata.ktx",
      GLLoadTests::CompressionType::eNone,
      "RGB8 + KTXOrientation up"
    },
    { DrawTexture::create,
      "testimages/orient-down-metadata.ktx",
      GLLoadTests::CompressionType::eNone,
      "RGB8 + KTXOrientation down"
    },
    { DrawTexture::create,
      "testimages/etc1.ktx",
      GLLoadTests::CompressionType::eNone,
      "ETC1 RGB8"
    },
    { DrawTexture::create,
      "testimages/etc2-rgb.ktx",
      GLLoadTests::CompressionType::eNone,
      "ETC2 RGB8"
    },
    { DrawTexture::create,
      "testimages/etc2-rgba1.ktx",
      GLLoadTests::CompressionType::eNone,
      "ETC2 RGB8A1"
    },
    { DrawTexture::create,
      "testimages/etc2-rgba8.ktx",
      GLLoadTests::CompressionType::eNone,
      "ETC2 RGB8A8"
    },
    { DrawTexture::create,
      "testimages/rgba-reference.ktx",
      GLLoadTests::CompressionType::eNone,
      "RGBA8"
    },
    { TexturedCube::create,
      "testimages/rgb-reference.ktx",
      GLLoadTests::CompressionType::eNone,
      "RGB8"
    },
    { TexturedCube::create,
      "testimages/rgb-amg-reference.ktx",
      GLLoadTests::CompressionType::eNone,
      "RGB8 + Auto Mipmap"
    },
    { TexturedCube::create,
      "testimages/rgb-mipmap-reference.ktx",
      GLLoadTests::CompressionType::eNone,
      "RGB8 Color/level mipmap"
    },
    { TexturedCube::create,
      "--npot testimages/hi_mark_sq.ktx",
      GLLoadTests::CompressionType::eNone,
      "RGB8 NPOT HI Logo"
    },
};

const int iNumSamples = sizeof(siSamples) / sizeof(GLLoadTests::sampleInvocation);

AppBaseSDL* theApp = new GLLoadTests(siSamples, iNumSamples,
                                         "KTX Loader Tests for OpenGL ES 1",
                                         SDL_GL_CONTEXT_PROFILE_ES, 1, 1);



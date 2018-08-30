/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2015 - 2018 Mark Callow.
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
 * @internal
 * @file LoadTestsGL3.cpp
 * @~English
 *
 * @brief Instantiate GLLoadTests app with set of tests for OpenGL 3.3+ and
 *        OpenGL ES 3.x
 *
 * @author Mark Callow
 * @copyright (c) 2015-2018, Mark Callow.
 */

#include "GLLoadTests.h"
#include "DrawTexture.h"
#include "TexturedCube.h"
#include "TextureArray.h"

const GLLoadTests::sampleInvocation siSamples[] = {
    { DrawTexture::create,
      "testimages/hi_mark.ktx",
      "RGB8 NPOT HI Logo"
    },
    { DrawTexture::create,
      "testimages/luminance-reference-metadata.ktx",
      "LUMINANCE8 NPOT + KTXOrientation down"
    },
    { DrawTexture::create,
      "testimages/orient-up.ktx",
      "RGB8 + KTXOrientation up"
    },
    { DrawTexture::create,
      "testimages/orient-down-metadata.ktx",
      "RGB8 + KTXOrientation down"
    },
    { DrawTexture::create,
      "testimages/not4_rgb888_srgb.ktx",
      "RGB8 2D, Row length not Multiple of 4"
    },
    { DrawTexture::create,
      "testimages/etc1.ktx",
      "ETC1 RGB8"
    },
    { DrawTexture::create,
      "testimages/etc2-rgb.ktx",
      "ETC2 RGB8"
    },
    { DrawTexture::create,
      "testimages/etc2-rgba1.ktx",
      "ETC2 RGB8A1"
    },
    { DrawTexture::create,
      "testimages/etc2-rgba8.ktx",
      "ETC2 RGB8A8"
    },
    { DrawTexture::create,
      "testimages/etc2-sRGB.ktx",
      "ETC2 sRGB8"
    },
    { DrawTexture::create,
      "testimages/etc2-sRGBa1.ktx",
      "ETC2 sRGB8A1"
    },
    { DrawTexture::create,
      "testimages/etc2-sRGBa8.ktx",
      "ETC2 sRGB8A8"
    },
    { DrawTexture::create,
      "testimages/rgba-reference.ktx",
      "RGBA8"
    },
    { DrawTexture::create,
      "testimages/rgb-reference.ktx",
      "RGB8"
    },
    { DrawTexture::create,
      "testimages/conftestimage_R11_EAC.ktx",
      "ETC2 R11"
    },
    { DrawTexture::create,
      "testimages/conftestimage_SIGNED_R11_EAC.ktx",
      "ETC2 Signed R11"
    },
    { DrawTexture::create,
      "testimages/conftestimage_RG11_EAC.ktx",
      "ETC2 RG11"
    },
    { DrawTexture::create,
      "testimages/conftestimage_SIGNED_RG11_EAC.ktx",
      "ETC2 Signed RG11"
    },
    { TextureArray::create,
      "testimages/texturearray_bc3_unorm.ktx",
      "BC3 (S3TC DXT5) Compressed Texture Array"
    },
    { TextureArray::create,
      "testimages/texturearray_astc_8x8_unorm.ktx",
      "ASTC 8x8 Compressed Texture Array"
    },
    { TextureArray::create,
      "testimages/texturearray_etc2_unorm.ktx",
      "ETC2 Compressed Texture Array"
    },
    { TexturedCube::create,
      "testimages/rgb-amg-reference.ktx",
      "RGB8 + Auto Mipmap"
    },
    { TexturedCube::create,
      "testimages/rgb-mipmap-reference.ktx",
      "RGB8 Color/level mipmap"
    },
    { TexturedCube::create,
      "testimages/hi_mark_sq.ktx",
      "RGB8 NPOT HI Logo"
    },
};

const int iNumSamples = sizeof(siSamples) / sizeof(GLLoadTests::sampleInvocation);

#if !(defined(GL_CONTEXT_PROFILE) && defined(GL_CONTEXT_MAJOR_VERSION) && defined(GL_CONTEXT_MINOR_VERSION))
  #error GL_CONTEXT_PROFILE, GL_CONTEXT_MAJOR_VERSION & GL_CONTEXT_MINOR_VERSION must be defined.
#endif

AppBaseSDL* theApp = new GLLoadTests(siSamples, iNumSamples,
                                         "KTX Loader Tests for GL3 & ES3",
                                         GL_CONTEXT_PROFILE,
                                         GL_CONTEXT_MAJOR_VERSION,
                                         GL_CONTEXT_MINOR_VERSION);


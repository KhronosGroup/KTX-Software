/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2018-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file    ES1LoadTests.cpp
 * @brief  List of tests of the KTX loader for OpenGL ES 1.1.
 *
 * The loader is tested by loading and drawing KTX textures in various formats
 * using the DrawTexture and TexturedCube samples.
 */

#include <sstream>
#include <ktx.h>

#include "GLLoadTests.h"
#include "DrawTexture.h"
#include "TexturedCube.h"

LoadTestSample*
GLLoadTests::showFile(const std::string& filename)
{
    KTX_error_code ktxresult;
    ktxTexture* kTexture;
    ktxresult = ktxTexture_CreateFromNamedFile(filename.c_str(),
                                        KTX_TEXTURE_CREATE_NO_FLAGS,
                                        &kTexture);
    if (KTX_SUCCESS != ktxresult) {
        std::stringstream message;

        message << "Creation of ktxTexture from \"" << getAssetPath()
                << filename << "\" failed: " << ktxErrorString(ktxresult);
        throw std::runtime_error(message.str());
    }

    LoadTestSample::PFN_create createViewer;
    LoadTestSample* pViewer;
    createViewer = DrawTexture::create;
    ktxTexture_Destroy(kTexture);
    pViewer = createViewer(w_width, w_height, filename.c_str(), "");
    return pViewer;
}

const GLLoadTests::sampleInvocation siSamples[] = {
    { DrawTexture::create,
      "--npot hi_mark.ktx",
      "KTX1: RGB8 NPOT HI Logo"
    },
    { DrawTexture::create,
      "--npot l8_unorm_metadata.ktx",
      "KTX1: LUMINANCE8 NPOT"
    },
    { DrawTexture::create,
      "orient_up_metadata.ktx",
      "KTX1: RGB8 + KTXOrientation up"
    },
    { DrawTexture::create,
      "orient_down_metadata.ktx",
      "KTX1: RGB8 + KTXOrientation down"
    },
    { DrawTexture::create,
      "etc1.ktx",
      "KTX1: ETC1 RGB8"
    },
    { DrawTexture::create,
      "etc2_rgb.ktx",
      "KTX1: ETC2 RGB8"
    },
    { DrawTexture::create,
      "etc2_rgba1.ktx",
      "KTX1: ETC2 RGB8A1"
    },
    { DrawTexture::create,
      "etc2_rgba8.ktx",
      "KTX1: ETC2 RGB8A8"
    },
    { DrawTexture::create,
      "r8g8b8a8_srgb.ktx",
      "KTX1: RGBA8 No KTXOrientation"
    },
    { TexturedCube::create,
      "r8g8b8_srgb.ktx",
      "KTX1: RGB8"
    },
    { TexturedCube::create,
      "r8g8b8_unorm_amg.ktx",
      "KTX1: RGB8 + Auto Mipmap"
    },
    { TexturedCube::create,
      "r8g8b8_srgb_mip.ktx",
      "KTX1: RGB8 Color/level mipmap"
    },
    { TexturedCube::create,
      "--npot hi_mark_sq.ktx",
      "KTX1: RGB8 NPOT HI Logo"
    },
};

const int iNumSamples = sizeof(siSamples) / sizeof(GLLoadTests::sampleInvocation);

AppBaseSDL* theApp = new GLLoadTests(siSamples, iNumSamples,
                                         "KTX Loader Tests for OpenGL ES 1",
                                         SDL_GL_CONTEXT_PROFILE_ES, 1, 1);

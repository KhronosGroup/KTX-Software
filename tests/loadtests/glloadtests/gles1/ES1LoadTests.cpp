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
GLLoadTests::showFile(std::string& filename)
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
      "RGB8 NPOT HI Logo"
    },
    { DrawTexture::create,
      "--npot luminance-reference-metadata.ktx",
      "LUMINANCE8 NPOT"
    },
    { DrawTexture::create,
      "orient-up-metadata.ktx",
      "RGB8 + KTXOrientation up"
    },
    { DrawTexture::create,
      "orient-down-metadata.ktx",
      "RGB8 + KTXOrientation down"
    },
    { DrawTexture::create,
      "etc1.ktx",
      "ETC1 RGB8"
    },
    { DrawTexture::create,
      "etc2-rgb.ktx",
      "ETC2 RGB8"
    },
    { DrawTexture::create,
      "etc2-rgba1.ktx",
      "ETC2 RGB8A1"
    },
    { DrawTexture::create,
      "etc2-rgba8.ktx",
      "ETC2 RGB8A8"
    },
    { DrawTexture::create,
      "rgba-reference.ktx",
      "RGBA8"
    },
    { TexturedCube::create,
      "rgb-reference.ktx",
      "RGB8"
    },
    { TexturedCube::create,
      "rgb-amg-reference.ktx",
      "RGB8 + Auto Mipmap"
    },
    { TexturedCube::create,
      "rgb-mipmap-reference.ktx",
      "RGB8 Color/level mipmap"
    },
    { TexturedCube::create,
      "--npot hi_mark_sq.ktx",
      "RGB8 NPOT HI Logo"
    },
};

const int iNumSamples = sizeof(siSamples) / sizeof(GLLoadTests::sampleInvocation);

AppBaseSDL* theApp = new GLLoadTests(siSamples, iNumSamples,
                                         "KTX Loader Tests for OpenGL ES 1",
                                         SDL_GL_CONTEXT_PROFILE_ES, 1, 1);



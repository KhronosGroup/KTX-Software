/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2015-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file LoadTestsGL3.cpp
 * @~English
 *
 * @brief Instantiate GLLoadTests app with set of tests for OpenGL 3.3+ and
 *        OpenGL ES 3.x
 */

#include "GLLoadTests.h"
#include "EncodeTexture.h"
#include "DrawTexture.h"
#include "TexturedCube.h"
#include "Texture3d.h"
#include "TextureCubemap.h"
#include "TextureArray.h"
#include "TextureMipmap.h"

#if !defined TEST_COMPRESSION
#define TEST_COMPRESSION 1
#endif

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

        message << "Creation of ktxTexture from \"" << filename
                << "\" failed: " << ktxErrorString(ktxresult);
        throw std::runtime_error(message.str());
    }

    LoadTestSample::PFN_create createViewer;
    LoadTestSample* pViewer;
    if (kTexture->numDimensions == 3)
       createViewer = Texture3d::create;
    else if (kTexture->isArray && kTexture->isCubemap) {
        // TODO: Add cubemap array app.
        std::stringstream message;
        message << "Display of cubemap array textures not yet implemented.";
        throw std::runtime_error(message.str());
    } else if (kTexture->isArray) {
        createViewer = TextureArray::create;
    } else if (kTexture->isCubemap) {
#if !defined(__EMSCRIPTEN__)
        createViewer = TextureCubemap::create;
#else
        throw std::runtime_error("Emscripten viewer can't display cube maps"
                                 " because there is no libassimp support.");
#endif
    } else if (kTexture->numLevels > 1 || kTexture->generateMipmaps) {
        // TODO: Add option to choose to display showing the individual
        // mipmaps vs. DrawTexture that displays a single rect using the
        // mipmaps, if present.
        createViewer = TextureMipmap::create;
    } else {
        createViewer = DrawTexture::create;
    }
    ktxTexture_Destroy(kTexture);
    std::string args = "--external " + filename;
    pViewer = createViewer(w_width, w_height, args.c_str(), sBasePath);
    return pViewer;
}

const GLLoadTests::sampleInvocation siSamples[] = {
    { DrawTexture::create,
      "etc1s_Iron_Bars_001_normal.ktx2",
      "Transcode of ETC1S+BasisLZ Compressed KTX2 XY normal map mipmapped"
    },
    { DrawTexture::create,
      "uastc_Iron_Bars_001_normal.ktx2",
      "Transcode of UASTC+zstd Compressed KTX2 XY normal map mipmapped"
    },
    { DrawTexture::create,
      "color_grid_uastc_zstd.ktx2",
      "Transcode of UASTC+Zstd Compressed KTX2 RGB not mipmapped "
    },
    { DrawTexture::create,
      "color_grid_zstd.ktx2",
      "Zstd Compressed KTX2 RGB not mipmapped"
    },
    { DrawTexture::create,
      "color_grid_uastc.ktx2",
      "Transcode of UASTC Compressed KTX2 RGB not mipmapped"
    },
    { DrawTexture::create,
      "color_grid_basis.ktx2",
      "Transcode of ETC1S+BasisLZ Compressed KTX2 RGB not mipmapped"
    },
    { DrawTexture::create,
      "kodim17_basis.ktx2",
      "Transcode of ETC1S+BasisLZ Compressed KTX2 RGB not mipmapped"
    },
    { DrawTexture::create,
      "--transcode-target RGBA4444 kodim17_basis.ktx2",
      "Transcode of ETC1S+BasisLZ Compressed KTX2 RGB not mipmapped to RGBA4444"
    },
    { EncodeTexture::create,
      "FlightHelmet_baseColor_basis.ktx2",
      "Transcode of ETC1S+BasisLZ Compressed KTX2 RGBA not mipmapped"
    },
#if TEST_COMPRESSION
    { EncodeTexture::create,
      "--encode etc1s rgba-reference-u.ktx2",
      "Encode to ETC1S+BasisLZ then Transcode of Compressed KTX2 RGBA not mipmapped"
    },
    { EncodeTexture::create,
      "--encode uastc rgba-reference-u.ktx2",
      "Encode to UASTC then Transcode of Compressed KTX2 RGBA not mipmapped"
    },
    { EncodeTexture::create,
      "--encode astc rgba-reference-u.ktx2",
      "Encode to ASTC then display RGBA not mipmapped"
    },
#endif
#if !defined(__EMSCRIPTEN__)
    { TextureCubemap::create,
      "cubemap_goldengate_uastc_rdo4_zstd5_rd.ktx2",
      "Transcode of UASTC+rdo+zstd Compressed KTX2 Cube Map Transcoded"
    },
    { TextureCubemap::create,
      "cubemap_yokohama_basis_rd.ktx2",
      "Transcode of ETC1S/BasisLZ Compressed KTX2 mipmapped cube map",
    },
#endif
    { DrawTexture::create,
      "orient-down-metadata-u.ktx2",
      "KTX2: RGB8 + KTXOrientation down"
    },
    { DrawTexture::create,
      "--preload orient-down-metadata-u.ktx2",
      "KTX2: RGB8 + KTXOrientation down with pre-loaded images"
    },
    { TextureArray::create,
      "texturearray_bc3_unorm.ktx2",
      "KTX2: BC3 (S3TC DXT5) Compressed Texture Array"
    },
    { TextureArray::create,
      "texturearray_astc_8x8_unorm.ktx2",
      "KTX2: ASTC 8x8 Compressed Texture Array"
    },
    { TextureArray::create,
      "texturearray_etc2_unorm.ktx2",
      "KTX2: ETC2 Compressed Texture Array"
    },
    { Texture3d::create,
      "3dtex_7_reference_u.ktx2",
      "RGBA8 3d Texture, Depth == 7"
    },
    { TexturedCube::create,
      "rgb-mipmap-reference-u.ktx2",
      "KTX2: RGB8 Color/level mipmap"
    },
    { DrawTexture::create,
      "hi_mark.ktx",
      "RGB8 NPOT HI Logo"
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
      "not4_rgb888_srgb.ktx",
      "RGB8 2D, Row length not Multiple of 4"
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
      "etc2-sRGB.ktx",
      "ETC2 sRGB8"
    },
    { DrawTexture::create,
      "etc2-sRGBa1.ktx",
      "ETC2 sRGB8A1"
    },
    { DrawTexture::create,
      "etc2-sRGBa8.ktx",
      "ETC2 sRGB8A8"
    },
    { DrawTexture::create,
      "rgba-reference.ktx",
      "RGBA8"
    },
    { DrawTexture::create,
      "rgb-reference.ktx",
      "RGB8"
    },
    { DrawTexture::create,
      "conftestimage_R11_EAC.ktx",
      "ETC2 R11"
    },
    { DrawTexture::create,
      "conftestimage_SIGNED_R11_EAC.ktx",
      "ETC2 Signed R11"
    },
    { DrawTexture::create,
      "conftestimage_RG11_EAC.ktx",
      "ETC2 RG11"
    },
    { DrawTexture::create,
      "conftestimage_SIGNED_RG11_EAC.ktx",
      "ETC2 Signed RG11"
    },
    { TextureArray::create,
      "texturearray_bc3_unorm.ktx",
      "BC3 (S3TC DXT5) Compressed Texture Array"
    },
    { TextureArray::create,
      "texturearray_astc_8x8_unorm.ktx",
      "ASTC 8x8 Compressed Texture Array"
    },
    { TextureArray::create,
      "texturearray_etc2_unorm.ktx",
      "ETC2 Compressed Texture Array"
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
      "hi_mark_sq.ktx",
      "RGB8 NPOT HI Logo"
    },
};

const uint32_t uNumSamples = sizeof(siSamples) / sizeof(GLLoadTests::sampleInvocation);

#if !(defined(GL_CONTEXT_PROFILE) && defined(GL_CONTEXT_MAJOR_VERSION) && defined(GL_CONTEXT_MINOR_VERSION))
  #error GL_CONTEXT_PROFILE, GL_CONTEXT_MAJOR_VERSION & GL_CONTEXT_MINOR_VERSION must be defined.
#endif

AppBaseSDL* theApp = new GLLoadTests(siSamples, uNumSamples,
                                         "KTX Loader Tests for GL3 & ES3",
                                         GL_CONTEXT_PROFILE,
                                         GL_CONTEXT_MAJOR_VERSION,
                                         GL_CONTEXT_MINOR_VERSION);


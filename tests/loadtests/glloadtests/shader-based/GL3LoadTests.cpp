/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2015-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file
 * @~English
 *
 * @brief Instantiate GLLoadTests app with set of tests for OpenGL 3.3+ and
 *        OpenGL ES 3.x
 */

#include <string>
#include <regex>

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
GLLoadTests::showFile(const std::string& filename)
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

    // Escape any spaces in filename.
    std::string args = "--external " + std::regex_replace( filename, std::regex(" "), "\\ " );
    pViewer = createViewer(w_width, w_height, args.c_str(), sBasePath);
    return pViewer;
}

const GLLoadTests::sampleInvocation siSamples[] = {
    { DrawTexture::create,
      "Iron_Bars_001_normal_blze.ktx2",
      "KTX2: Transcode of ETC1S+BasisLZ Compressed XY normal map mipmapped"
    },
    { DrawTexture::create,
      "Iron_Bars_001_normal_uastc_zstd_10.ktx2",
      "KTX2: Transcode of UASTC+zstd Compressed XY normal map mipmapped"
    },
    { DrawTexture::create,
      "color_grid_zstd_5.ktx2",
      "KTX2: Zstd Compressed RGB not mipmapped"
    },
    { DrawTexture::create,
      "color_grid_uastc_zstd_5.ktx2",
      "KTX2: Transcode of UASTC+Zstd Compressed RGB not mipmapped "
    },
    { DrawTexture::create,
      "color_grid_blze.ktx2",
      "KTX2: Transcode of ETC1S+BasisLZ Compressed RGB not mipmapped"
    },
    { DrawTexture::create,
      "kodim17_blze.ktx2",
      "KTX2: Transcode of ETC1S+BasisLZ Compressed RGB not mipmapped"
    },
    { DrawTexture::create,
      "--transcode-target RGBA4444 kodim17_blze.ktx2",
      "KTX2: Transcode of ETC1S+BasisLZ Compressed RGB not mipmapped to RGBA4444"
    },
    { EncodeTexture::create,
      "FlightHelmet_baseColor_blze.ktx2",
      "KTX2: Transcode of ETC1S+BasisLZ Compressed RGBA not mipmapped"
    },
#if TEST_COMPRESSION
    { EncodeTexture::create,
      "--encode etc1s r8g8b8a8_srgb.ktx2",
      "KTX2: Encode to ETC1S+BasisLZ then Transcode of Compressed RGBA not mipmapped"
    },
    { EncodeTexture::create,
      "--encode uastc r8g8b8a8_srgb.ktx2",
      "KTX2: Encode to UASTC then Transcode of Compressed KTX2 RGBA not mipmapped"
    },
    { EncodeTexture::create,
      "--encode astc r8g8b8a8_srgb.ktx2",
      "KTX2: Encode to ASTC then display RGBA not mipmapped"
    },
#endif
#if !defined(__EMSCRIPTEN__)
    { TextureCubemap::create,
      "cubemap_goldengate_uastc_rdo_4_zstd_5.ktx2",
      "KTX2: Transcode of UASTC+rdo+zstd Compressed KTX2 Cube Map Transcoded"
    },
    { TextureCubemap::create,
      "cubemap_yokohama_blze.ktx2",
      "KTX2: Transcode of ETC1S/BasisLZ Compressed KTX2 mipmapped cube map",
    },
#endif

    { DrawTexture::create,
      "orient_down_metadata.ktx2",
      "KTX2: RGBA8 2D + KTXOrientation down"
    },
    { DrawTexture::create,
      "orient_up_metadata.ktx2",
      "KTX2: RGBA8 2D + KTXOrientation up"
    },
    { DrawTexture::create,
      "--preload orient_down_metadata.ktx2",
      "KTX2: RGBA8 + KTXOrientation down with pre-loaded images"
    },
    { DrawTexture::create,
      "orient_up_metadata.ktx",
      "KTX1: RGB8 + KTXOrientation up"
    },
    { DrawTexture::create,
      "orient_down_metadata.ktx",
      "KTX1: RGB8 + KTXOrientation down"
    },
    { TextureArray::create,
      "bc3_unorm_array_7.ktx2",
      "KTX2: BC3 (S3TC DXT5) Compressed Texture Array"
    },
    { TextureArray::create,
      "astc_8x8_unorm_array_7.ktx2",
      "KTX2: ASTC 8x8 Compressed Texture Array"
    },
    { TextureArray::create,
      "etc2_unorm_array_7.ktx2",
      "KTX2: ETC2 Compressed Texture Array"
    },
    { Texture3d::create,
      "r8g8b8a8_srgb_3d_7.ktx2",
      "KTX2: RGBA8 3d Texture, Depth == 7"
    },
    { TexturedCube::create,
      "r8g8b8_srgb_mip.ktx2",
      "KTX2: RGB8 Color/level mipmap"
    },
    { DrawTexture::create,
      "hi_mark.ktx",
      "KTX1: RGB8 NPOT HI Logo"
    },
    { DrawTexture::create,
      "not4_r8g8b8_srgb.ktx",
      "KTX1: RGB8 2D, Row length not Multiple of 4"
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
      "etc2_srgb.ktx",
      "KTX1: ETC2 sRGB8"
    },
    { DrawTexture::create,
      "etc2_srgba1.ktx",
      "KTX1: ETC2 sRGB8A1"
    },
    { DrawTexture::create,
      "etc2_srgba8.ktx",
      "KTX1: ETC2 sRGB8A8"
    },
    { DrawTexture::create,
      "r8g8b8a8_srgb.ktx",
      "KTX1: RGBA8"
    },
    { DrawTexture::create,
      "r8g8b8_srgb.ktx",
      "KTX1: RGB8"
    },
    { DrawTexture::create,
      "conftestimage_R11_EAC.ktx",
      "KTX1: ETC2 R11"
    },
    { DrawTexture::create,
      "conftestimage_SIGNED_R11_EAC.ktx",
      "KTX1: ETC2 Signed R11"
    },
    { DrawTexture::create,
      "conftestimage_RG11_EAC.ktx",
      "KTX1: ETC2 RG11"
    },
    { DrawTexture::create,
      "conftestimage_SIGNED_RG11_EAC.ktx",
      "KTX1: ETC2 Signed RG11"
    },
    { TextureArray::create,
      "bc3_unorm_array_7.ktx",
      "KTX1: BC3 (S3TC DXT5) Compressed Texture Array"
    },
    { TextureArray::create,
      "astc_8x8_unorm_array_7.ktx",
      "KTX1: ASTC 8x8 Compressed Texture Array"
    },
    { TextureArray::create,
      "etc2_unorm_array_7.ktx",
      "KTX1: ETC2 Compressed Texture Array"
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
      "hi_mark_sq.ktx",
      "KTX1: RGB8 NPOT HI Logo"
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


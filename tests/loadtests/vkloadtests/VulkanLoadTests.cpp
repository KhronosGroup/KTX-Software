/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @class VulkanLoadTests
 * @~English
 *
 * @brief Framework for Vulkan texture loading test samples.
 *
 * @author Mark Callow, www.edgewise-consulting.com.
 */

#include <exception>

#define _USE_MATH_DEFINES
#include <math.h>

#include <SDL2/SDL_vulkan.h>

#include "VulkanLoadTests.h"
#include "Texture.h"
#include "Texture3d.h"
#include "TextureArray.h"
#include "TextureCubemap.h"
#include "TexturedCube.h"
#include "TextureMipmap.h"
#include "ltexceptions.h"

#define LT_VK_MAJOR_VERSION 1
#define LT_VK_MINOR_VERSION 0
#define LT_VK_PATCH_VERSION 0
#define LT_VK_VERSION VK_MAKE_VERSION(1, 0, 0)

VulkanLoadTests::VulkanLoadTests(const sampleInvocation samples[],
                                 const uint32_t numSamples,
                                 const char* const name)
                  : VulkanAppSDL(name, 1280, 720, LT_VK_VERSION, true),
                    siSamples(samples), sampleIndex(numSamples)
{
    pCurSample = nullptr;
}

VulkanLoadTests::~VulkanLoadTests()
{
    delete pCurSample;
}

bool
VulkanLoadTests::initialize(Args& args)
{
    if (!VulkanAppSDL::initialize(args))
        return false;

    for (auto it = args.begin() + 1; it != args.end(); it++) {
        infiles.push_back(*it);
    }
    if (infiles.size() > 0) {
        sampleIndex.setNumSamples((uint32_t)infiles.size());
    }
    // Launch the first sample.
    invokeSample(Direction::eForward);
    return true;
}

void
VulkanLoadTests::finalize()
{
    delete pCurSample;
    VulkanAppSDL::finalize();
}

int
VulkanLoadTests::doEvent(SDL_Event* event)
{
    int result = 0;

    switch (event->type) {
      case SDL_KEYUP:
        switch (event->key.keysym.sym) {
          case 'q':
            quit = true;
            break;
          case 'n':
            ++sampleIndex;
            invokeSample(Direction::eForward);
            break;
          case 'p':
            --sampleIndex;
            invokeSample(Direction::eBack);
            break;
          default:
            result = 1;
        }
        break;

      default:
        switch(swipeDetector.doEvent(event)) {
          case SwipeDetector::eSwipeUp:
          case SwipeDetector::eSwipeDown:
          case SwipeDetector::eEventConsumed:
            break;
          case SwipeDetector::eSwipeLeft:
            ++sampleIndex;
            invokeSample(Direction::eForward);
            break;
          case SwipeDetector::eSwipeRight:
            --sampleIndex;
            invokeSample(Direction::eBack);
            break;
          case SwipeDetector::eEventNotConsumed:
            result = 1;
          }
    }
    
    if (result == 1) {
        // Further processing required.
        if (pCurSample != nullptr)
            result = pCurSample->doEvent(event);  // Give sample a chance.
        if (result == 1)
            return VulkanAppSDL::doEvent(event);  // Pass to base class.
    }
    return result;
}


void
VulkanLoadTests::windowResized()
{
    if (pCurSample != nullptr)
        pCurSample->resize(w_width, w_height);
}


void
VulkanLoadTests::drawFrame(uint32_t msTicks)
{
    pCurSample->run(msTicks);

    VulkanAppSDL::drawFrame(msTicks);
}


void
VulkanLoadTests::getOverlayText(float yOffset)
{
    if (enableTextOverlay) {
        textOverlay->addText("Press \"n\" or 2-finger swipe left for next "
                             "sample, \"p\" or swipe right for previous.",
                             5.0f, yOffset, VulkanTextOverlay::alignLeft);
        yOffset += 20;
        textOverlay->addText("2-finger rotate or left mouse + drag to rotate.",
                             5.0f, yOffset, VulkanTextOverlay::alignLeft);
        yOffset += 20;
        textOverlay->addText("Pinch/zoom or right mouse + drag to change "
                             "object size.",
                             5.0f, yOffset, VulkanTextOverlay::alignLeft);
        yOffset += 20;
        if (pCurSample != nullptr) {
            pCurSample->getOverlayText(textOverlay, yOffset);
        }
    }
}

void
VulkanLoadTests::invokeSample(Direction dir)
{
    const sampleInvocation* sampleInv
            = &siSamples[sampleIndex];

    prepared = false;  // Prevent any more rendering.
    if (pCurSample != nullptr) {
        vkctx.queue.waitIdle(); // Wait for current rendering to finish.
        delete pCurSample;
        // Certain events can be triggered during new sample initialization
        // while pCurSample is not valid, e.g. FOCUS_LOST caused by a Vulkan
        // validation failure raising a message box. Protect against problems
        // from this by indicating there is no current sample.
        pCurSample = nullptr;
    }

    uint32_t unsupportedTypeExceptions = 0;
    std::string fileTitle;
    for (;;) {
        try {
            if (infiles.size() > 0) {
                fileTitle = "Viewing file ";
                fileTitle += infiles[sampleIndex];
                pCurSample = showFile(infiles[sampleIndex]);
            } else {
                sampleInv = &siSamples[sampleIndex];
                pCurSample = sampleInv->createSample(vkctx, w_width, w_height,
                                                     sampleInv->args,
                                                     sBasePath);
            }
            break;
        } catch (unsupported_ttype& e) {
            (void)e; // To quiet unused variable warnings from some compilers.
            unsupportedTypeExceptions++;
            if (unsupportedTypeExceptions == sampleIndex.getNumSamples()) {
                std::string manyFailMessage =
                    "All of the specified samples or files use texture types "
                    "or features not supported on this platform.";
                const std::string& message = unsupportedTypeExceptions == 1 ?
                                       e.what() : manyFailMessage;
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                    infiles.size() > 0 ? fileTitle.c_str() : sampleInv->title,
                    message.c_str(),
                    NULL);
                exit(0);
            } else {
                dir == Direction::eForward ? ++sampleIndex : --sampleIndex;
            }
        } catch (bad_vulkan_alloc& e) {
            const SDL_MessageBoxButtonData buttons[] = {
                /* .flags, .buttonid, .text */
                { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Abort" },
            };
            const SDL_MessageBoxData messageboxdata = {
                SDL_MESSAGEBOX_ERROR,                           // .flags
                NULL,                                           // .window
                infiles.size() > 0 ?
                  fileTitle.c_str() : sampleInv->title,         // .title
                e.what(),                                       // .message
                SDL_arraysize(buttons),                         // .numbuttons
                buttons,                                        // .buttons
                NULL //&colorScheme                             // .colorScheme
            };
            int buttonid;
            if (SDL_ShowMessageBox(&messageboxdata, &buttonid) < 0) {
                SDL_Log("error displaying error message box");
                exit(1);
            }
            if (buttonid == 0) {
                // We've been told to quit or no button was pressed.
                exit(1);
            }
        } catch (std::exception& e) {
            const SDL_MessageBoxButtonData buttons[] = {
                /* .flags, .buttonid, .text */
                { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Continue" },
                { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "Abort" },
            };
#if 0
            const SDL_MessageBoxColorScheme colorScheme = {
                { /* .colors (.r, .g, .b) */
                    /* [SDL_MESSAGEBOX_COLOR_BACKGROUND] */
                    { 255,   0,   0 },
                    /* [SDL_MESSAGEBOX_COLOR_TEXT] */
                    {   0, 255,   0 },
                    /* [SDL_MESSAGEBOX_COLOR_BUTTON_BORDER] */
                    { 255, 255,   0 },
                    /* [SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND] */
                    {   0,   0, 255 },
                    /* [SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED] */
                    { 255,   0, 255 }
                }
            };
#endif
            const SDL_MessageBoxData messageboxdata = {
                SDL_MESSAGEBOX_ERROR,                               // .flags
                NULL,                                               // .window
                infiles.size() > 0 ?
                 fileTitle.c_str() : sampleInv->title,              // .title
                e.what(),                                           // .message
                SDL_arraysize(buttons),                             // .numbuttons
                buttons,                                            // .buttons
                NULL //&colorScheme                                 // .colorScheme
            };
            int buttonid;
            if (SDL_ShowMessageBox(&messageboxdata, &buttonid) < 0) {
                SDL_Log("error displaying error message box");
                exit(1);
            }
            if (buttonid == 0) {
                // Continue
                dir == Direction::eForward ? ++sampleIndex : --sampleIndex;
            } else {
                // We've been told to quit or no button was pressed.
                exit(1);
            }
        }
    }
    prepared = true;
    setAppTitle(pCurSample->customizeTitle(infiles.size() > 0 ?
                                                            fileTitle.c_str()
                                                          : sampleInv->title));
}


void
VulkanLoadTests::onFPSUpdate()
{
    VulkanAppSDL::onFPSUpdate();
}

VulkanLoadTestSample*
VulkanLoadTests::showFile(std::string& filename)
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

    VulkanLoadTestSample::PFN_create createViewer;
    VulkanLoadTestSample* pViewer;
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
        createViewer = TextureCubemap::create;
    } else if (kTexture->numLevels > 1) {
        createViewer = TextureMipmap::create;
    } else {
        createViewer = Texture::create;
    }
    ktxTexture_Destroy(kTexture);
    std::string args = "--external " + filename;
    pViewer = createViewer(vkctx, w_width, w_height, args.c_str(), sBasePath);
    return pViewer;
}

/* ------------------------------------------------------------------------ */

const VulkanLoadTests::sampleInvocation siSamples[] = {
    { Texture::create,
      "etc1s_Iron_Bars_001_normal.ktx2",
      "Transcode of ETC1S+BasisLZ Compressed KTX2 XY normal map mipmapped"
    },
    { Texture::create,
      "uastc_Iron_Bars_001_normal.ktx2",
      "Transcode of UASTC+zstd Compressed KTX2 XY normal map mipmapped"
    },
    { Texture::create,
      "--use-vma uastc_Iron_Bars_001_normal.ktx2",
      "Transcode of UASTC+zstd Compressed KTX2 XY normal map mipmapped, using VMA"
    },
    { Texture::create,
      "ktx_document_uastc_rdo4_zstd5.ktx2",
      "UASTC+rdo+zstd compressed KTX2 RGBA8 mipmapped"
    },
    { Texture::create,
      "color_grid_uastc_zstd.ktx2",
      "UASTC+zstd Compressed KTX2 RGB non-mipmapped"
    },
    { Texture::create,
      "color_grid_zstd.ktx2",
      "Zstd Compressed KTX2 RGB non-mipmapped"
    },
    { Texture::create,
      "color_grid_uastc.ktx2",
      "UASTC Compressed KTX2 RGB non-mipmapped"
    },
    { Texture::create,
      "color_grid_basis.ktx2",
      "ETC1S+BasisLZ Compressed KTX2 RGB non-mipmapped"
    },
    { Texture::create,
      "kodim17_basis.ktx2",
      "ETC1S+BasisLZ Compressed KTX2 RGB non-mipmapped"
    },
    { Texture::create,
        "--qcolor 0.0,0.0,0.0 pattern_02_bc2.ktx2",
        "KTX2: BC2 (S3TC DXT3) Compressed 2D"
    },
    { TextureMipmap::create,
      "ktx_document_basis.ktx2",
      "ETC1S+BasisLZ  compressed RGBA + Mipmap"
    },
    { TextureMipmap::create,
      "rgba-mipmap-reference-basis.ktx2",
      // ETC1S encoder removed all-1 alpha from rgba-mipmap-reference.
      "ETC1S+BasisLZ Compressed RGB + Mipmap"
    },
    { Texture3d::create,
      "3dtex_7_reference_u.ktx2",
      "RGBA8 3d Texture, Depth == 7"
    },
    { TextureArray::create,
      "arraytex_7_mipmap_reference_u.ktx2",
      "RGBA8 Array Texture, Layers = 7"
    },
    { TextureCubemap::create,
      "cubemap_goldengate_uastc_rdo4_zstd5_rd.ktx2",
      "UASTC+rdo+zstd Compressed RGB Cube Map"
    },
    { TextureCubemap::create,
      "--preload cubemap_goldengate_uastc_rdo4_zstd5_rd.ktx2",
      "UASTC+rdo+zstd Compressed RGB Cube Map from pre-loaded images"
    },
    { TextureCubemap::create,
        "cubemap_yokohama_basis_rd.ktx2",
        "ETC1S+BasisLZ Compressed RGB Cube Map"
    },
    { TextureCubemap::create,
        "--preload cubemap_yokohama_basis_rd.ktx2",
        "ETC1S+BasisLZ Compressed RGB Cube Map from pre-loaded images"
    },
    { TextureCubemap::create,
        "skybox_zstd.ktx2",
        "Zstd Compressed B10G11R11_UFLOAT Cube Map. Tests for correct blockSizeInBits after inflation"
    },
    { Texture::create,
      "orient-down-metadata.ktx",
      "RGB8 2D + KTXOrientation down"
    },
    { Texture::create,
      "orient-up-metadata.ktx",
      "RGB8 2D + KTXOrientation up"
    },
    { Texture::create,
      "--linear-tiling orient-up-metadata.ktx",
      "RGB8 2D + KTXOrientation up with Linear Tiling"
    },
    { Texture::create,
      "rgba-reference.ktx",
      "RGBA8 2D"
    },
    { Texture::create,
        "--linear-tiling rgba-reference.ktx",
        "RGBA8 2D using Linear Tiling"
    },
    { Texture::create,
      "etc2-rgb.ktx",
      "ETC2 RGB8"
    },
    { Texture::create,
      "etc2-rgba8.ktx",
      "ETC2 RGB8A8"
    },
    { Texture::create,
      "etc2-sRGB.ktx",
      "ETC2 sRGB8"
    },
    { Texture::create,
        "etc2-sRGBa8.ktx",
        "ETC2 sRGB8a8"
    },
    { Texture::create,
        "--qcolor 0.0,0.0,0.0 pattern_02_bc2.ktx",
        "BC2 (S3TC DXT3) Compressed 2D"
    },
    { TextureMipmap::create,
      "rgb-amg-reference.ktx",
      "RGB8 + Auto Mipmap"
    },
    { TextureMipmap::create,
      "--linear-tiling rgb-amg-reference.ktx",
      "RGB8 + Auto Mipmap using Linear Tiling"
    },
    { TextureMipmap::create,
      "metalplate-amg-rgba8.ktx",
      "RGBA8 2D + Auto Mipmap"
    },
    { TextureMipmap::create,
      "--linear-tiling metalplate-amg-rgba8.ktx",
      "RGBA8 2D + Auto Mipmap using Linear Tiling"
    },
    { TextureMipmap::create,
      "not4_rgb888_srgb.ktx",
      "RGB8 2D, Row length not Multiple of 4"
    },
    { TextureMipmap::create,
      "--linear-tiling not4_rgb888_srgb.ktx",
      "RGB8 2D, Row length not Multiple of 4 using Linear Tiling"
    },
    { TextureArray::create,
        "texturearray_bc3_unorm.ktx",
        "BC2 (S3TC DXT3) Compressed Texture Array"
    },
    { TextureArray::create,
        "--linear-tiling texturearray_bc3_unorm.ktx",
        "BC2 (S3TC DXT3) Compressed Texture Array using Linear Tiling"
    },
    { TextureArray::create,
        "texturearray_astc_8x8_unorm.ktx",
        "ASTC 8x8 Compressed Texture Array"
    },
    { TextureArray::create,
        "texturearray_etc2_unorm.ktx",
        "ETC2 Compressed Texture Array"
    },
#if 0
    { TexturedCube::create,
      "rgb-amg-reference.ktx",
      "RGB8 + Auto Mipmap"
    },
    { TexturedCube::create,
      "rgb-amg-reference.ktx",
      "RGB8 + Auto Mipmap"
    },
#endif
};

const uint32_t uNumSamples = sizeof(siSamples) / sizeof(VulkanLoadTests::sampleInvocation);

AppBaseSDL* theApp = new VulkanLoadTests(siSamples, uNumSamples,
                                     "KTX Loader Tests for Vulkan");

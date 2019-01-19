/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2017 Mark Callow.
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
                                 const int numSamples,
                                 const char* const name)
                  : siSamples(samples), sampleIndex(numSamples),
                    VulkanAppSDL(name, 1280, 720, LT_VK_VERSION, true)
{
    pCurSample = nullptr;
}

VulkanLoadTests::~VulkanLoadTests()
{
    if (pCurSample != nullptr) {
        delete pCurSample;
    }
}

bool
VulkanLoadTests::initialize(int argc, char* argv[])
{
    if (!VulkanAppSDL::initialize(argc, argv))
        return false;

    // Launch the first sample.
    invokeSample(Direction::eForward);
    return true;
}

void
VulkanLoadTests::finalize()
{
    if (pCurSample != nullptr) {
        delete pCurSample;
    }
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
VulkanLoadTests::getOverlayText(VulkanTextOverlay * textOverlay, float yOffset)
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
    const sampleInvocation* sampleInv;

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
    sampleInv = &siSamples[sampleIndex];

    for (;;) {
        try {
            pCurSample = sampleInv->createSample(vkctx, w_width, w_height,
                                    sampleInv->args, sBasePath);
            break;
        } catch (unsupported_ttype& e) {
            (void)e; // To quiet unused variable warnings from some compilers.
            dir == Direction::eForward ? ++sampleIndex : --sampleIndex;
            sampleInv = &siSamples[sampleIndex];
        } catch (std::exception& e) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                    sampleInv->title,
                    e.what(), NULL);
            dir == Direction::eForward ? ++sampleIndex : --sampleIndex;
            sampleInv = &siSamples[sampleIndex];
        }
    }
    prepared = true;
    setAppTitle(sampleInv->title);
}


void
VulkanLoadTests::onFPSUpdate()
{
    VulkanAppSDL::onFPSUpdate();
}

/* ------------------------------------------------------------------------ */

const VulkanLoadTests::sampleInvocation siSamples[] = {
    { Texture::create,
      "testimages/orient-down-metadata.ktx",
      "RGB8 2D + KTXOrientation down"
    },
    { Texture::create,
      "testimages/orient-up-metadata.ktx",
      "RGB8 2D + KTXOrientation up"
    },
    { Texture::create,
      "--linear-tiling testimages/orient-up-metadata.ktx",
      "RGB8 2D + KTXOrientation up with Linear Tiling"
    },
    { Texture::create,
      "testimages/rgba-reference.ktx",
      "RGBA8 2D"
    },
    { Texture::create,
        "--linear-tiling testimages/rgba-reference.ktx",
        "RGBA8 2D using Linear Tiling"
    },
    { Texture::create,
      "testimages/etc2-rgb.ktx",
      "ETC2 RGB8"
    },
    { Texture::create,
      "testimages/etc2-rgba8.ktx",
      "ETC2 RGB8A8"
    },
    { Texture::create,
      "testimages/etc2-sRGB.ktx",
      "ETC2 sRGB8"
    },
    { Texture::create,
        "testimages/etc2-sRGBa8.ktx",
        "ETC2 sRGB8a8"
    },
    { Texture::create,
        "--qcolor 0.0,0.0,0.0 testimages/pattern_02_bc2.ktx",
        "BC2 (S3TC DXT3) Compressed 2D"
    },
    { TextureMipmap::create,
      "testimages/rgb-amg-reference.ktx",
      "RGB8 + Auto Mipmap"
    },
    { TextureMipmap::create,
      "--linear-tiling testimages/rgb-amg-reference.ktx",
      "RGB8 + Auto Mipmap using Linear Tiling"
    },
    { TextureMipmap::create,
      "testimages/metalplate-amg-rgba8.ktx",
      "RGBA8 2D + Auto Mipmap"
    },
    { TextureMipmap::create,
      "--linear-tiling testimages/metalplate-amg-rgba8.ktx",
      "RGBA8 2D + Auto Mipmap using Linear Tiling"
    },
    { TextureMipmap::create,
      "testimages/not4_rgb888_srgb.ktx",
      "RGB8 2D, Row length not Multiple of 4"
    },
    { TextureMipmap::create,
      "--linear-tiling testimages/not4_rgb888_srgb.ktx",
      "RGB8 2D, Row length not Multiple of 4 using Linear Tiling"
    },
    { TextureArray::create,
        "testimages/texturearray_bc3_unorm.ktx",
        "BC2 (S3TC DXT3) Compressed Texture Array"
    },
    { TextureArray::create,
        "--linear-tiling testimages/texturearray_bc3_unorm.ktx",
        "BC2 (S3TC DXT3) Compressed Texture Array using Linear Tiling"
    },
    { TextureArray::create,
        "testimages/texturearray_astc_8x8_unorm.ktx",
        "ASTC 8x8 Compressed Texture Array"
    },
    { TextureArray::create,
        "testimages/texturearray_etc2_unorm.ktx",
        "ETC2 Compressed Texture Array"
    },
    { TextureCubemap::create,
        "testimages/cubemap_yokohama_bc3_unorm.ktx",
        "BC2 (S3TC DXT3) Compressed Cube Map"
    },
    { TextureCubemap::create,
        "testimages/cubemap_yokohama_astc_8x8_unorm.ktx",
        "ASTC Compressed Cube Map"
    },
    { TextureCubemap::create,
        "testimages/cubemap_yokohama_etc2_unorm.ktx",
        "ETC2 Compressed Cube Map"
    },
    { TextureCubemap::create,
        "--preload testimages/cubemap_yokohama_bc3_unorm.ktx",
        "BC2 (S3TC DXT3) Compressed Cube Map from Preloaded Images."
    },
    { TextureCubemap::create,
        "--preload testimages/cubemap_yokohama_astc_8x8_unorm.ktx",
        "ASTC Compressed Cube Map from Preloaded Images."
    },
    { TextureCubemap::create,
        "--preload testimages/cubemap_yokohama_etc2_unorm.ktx",
        "ETC2 Compressed Cube Map from Preloaded Images."
    },
#if 0
    { TexturedCube::create,
      "testimages/rgb-amg-reference.ktx",
      "RGB8 + Auto Mipmap"
    },
    { TexturedCube::create,
      "testimages/rgb-amg-reference.ktx",
      "RGB8 + Auto Mipmap"
    },
#endif
};

const int iNumSamples = sizeof(siSamples) / sizeof(VulkanLoadTests::sampleInvocation);

AppBaseSDL* theApp = new VulkanLoadTests(siSamples, iNumSamples,
                                     "KTX Loader Tests for Vulkan");

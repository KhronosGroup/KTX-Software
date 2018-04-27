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

#include <SDL2/SDL_vulkan.h>

#include "VulkanLoadTests.h"
#include "Texture.h"
#include "TextureArray.h"
#include "TextureCubemap.h"
#include "TexturedCube.h"
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
      case SDL_MOUSEBUTTONDOWN:
        // Forward to sample in case this is the start of motion.
        result = 1;
        switch (event->button.button) {
          case SDL_BUTTON_LEFT:
            buttonDown.x = event->button.x;
            buttonDown.y = event->button.y;
            buttonDown.timestamp = event->button.timestamp;
            break;
          default:
            break;
        }
        break;
      case SDL_MOUSEBUTTONUP:
        // Forward to sample so it doesn't get stuck in button down state.
        result = 1;
        switch (event->button.button) {
          case SDL_BUTTON_LEFT:
            if (SDL_abs(event->button.x - buttonDown.x) < 5
                && SDL_abs(event->button.y - buttonDown.y) < 5
                && (event->button.timestamp - buttonDown.timestamp) < 100) {
                // Advance to the next sample.
                ++sampleIndex;
                invokeSample(Direction::eForward);
            }
            break;
          default:
            break;
        }
        break;
      default:
          result = 1;
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
VulkanLoadTests::getOverlayText(VulkanTextOverlay * textOverlay)
{
    if (enableTextOverlay && pCurSample != nullptr) {
        pCurSample->getOverlayText(textOverlay);
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
            switch (sampleInv->ctype) {
              case CompressionType::eNone:
                break;
              case CompressionType::eBC:
                if (!vkctx.gpuFeatures.textureCompressionBC)
                    throw unsupported_ctype();
                break;
              case CompressionType::eASTC_LDR:
                if (!vkctx.gpuFeatures.textureCompressionASTC_LDR)
                    throw unsupported_ctype();
                break;
              case CompressionType::eETC2:
                if (!vkctx.gpuFeatures.textureCompressionETC2)
                    throw unsupported_ctype();
                break;
            }
            pCurSample = sampleInv->createSample(vkctx, w_width, w_height,
                                    sampleInv->args, sBasePath);
            break;
        } catch (unsupported_ctype& e) {
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
      "testimages/rgba-reference.ktx",
      VulkanLoadTests::CompressionType::eNone,
      "RGBA8 2D"
    },
    { Texture::create,
        "--linear-tiling testimages/rgba-reference.ktx",
        VulkanLoadTests::CompressionType::eNone,
        "RGBA8 2D using Linear Tiling"
    },
#if !defined(__IPHONEOS__) && !defined(__MACOSX__)
    // Uncompressed RGB formats not supported by Metal.
    { Texture::create,
      "testimages/orient-down-metadata.ktx",
      VulkanLoadTests::CompressionType::eNone,
     "RGB8 2D + KTXOrientation down"
    },
    { Texture::create,
      "testimages/orient-up-metadata.ktx",
      VulkanLoadTests::CompressionType::eNone,
      "RGB8 2D + KTXOrientation up"
    },
#endif
    { Texture::create,
      "testimages/etc2-rgb.ktx",
      VulkanLoadTests::CompressionType::eETC2,
      "ETC2 RGB8"
    },
    { Texture::create,
      "testimages/etc2-rgba8.ktx",
      VulkanLoadTests::CompressionType::eETC2,
      "ETC2 RGB8A8"
    },
    { Texture::create,
      "testimages/etc2-sRGB.ktx",
      VulkanLoadTests::CompressionType::eETC2,
      "ETC2 sRGB8"
    },
    { Texture::create,
        "testimages/etc2-sRGBa8.ktx",
        VulkanLoadTests::CompressionType::eETC2,
        "ETC2 sRGB8a8"
    },
    { Texture::create,
        "--qcolor 0.0,0.0,0.0 testimages/pattern_02_bc2.ktx",
        VulkanLoadTests::CompressionType::eBC,
        "BC2 (S3TC DXT3) Compressed 2D"
    },
    { TextureArray::create,
        "testimages/texturearray_bc3_unorm.ktx",
        VulkanLoadTests::CompressionType::eBC,
        "BC3 (S3TC DXT5) Compressed Texture Array"
    },
    { TextureArray::create,
        "testimages/texturearray_astc_8x8_unorm.ktx",
        VulkanLoadTests::CompressionType::eASTC_LDR,
        "ASTC 8x8 Compressed Texture Array"
    },
    { TextureArray::create,
        "testimages/texturearray_etc2_unorm.ktx",
        VulkanLoadTests::CompressionType::eETC2,
        "ETC2 Compressed Texture Array"
    },
    { TextureCubemap::create,
        "testimages/cubemap_yokohama_bc3_unorm.ktx",
        VulkanLoadTests::CompressionType::eBC,
        "BC2 (S3TC DXT3) Compressed Cube Map"
    },
    { TextureCubemap::create,
        "testimages/cubemap_yokohama_astc_8x8_unorm.ktx",
        VulkanLoadTests::CompressionType::eASTC_LDR,
        "ASTC Compressed Cube Map"
    },
    { TextureCubemap::create,
        "testimages/cubemap_yokohama_etc2_unorm.ktx",
        VulkanLoadTests::CompressionType::eETC2,
        "ETC2 Compressed Cube Map"
    },
    { TextureCubemap::create,
        "--preload testimages/cubemap_yokohama_bc3_unorm.ktx",
        VulkanLoadTests::CompressionType::eBC,
        "BC2 (S3TC DXT3) Compressed Cube Map from Preloaded Images."
    },
    { TextureCubemap::create,
        "--preload testimages/cubemap_yokohama_astc_8x8_unorm.ktx",
        VulkanLoadTests::CompressionType::eASTC_LDR,
        "ASTC Compressed Cube Map from Preloaded Images."
    },
    { TextureCubemap::create,
        "--preload testimages/cubemap_yokohama_etc2_unorm.ktx",
        VulkanLoadTests::CompressionType::eETC2,
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

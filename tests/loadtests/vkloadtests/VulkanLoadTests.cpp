/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/*
 * Copyright (c) 2017, Mark Callow, www.edgewise-consulting.com.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * unaltered in all copies or substantial portions of the Materials.
 * Any additions, deletions, or changes to the original source files
 * must be clearly indicated in accompanying documentation.
 *
 * If only executable code is distributed, then the accompanying
 * documentation must state that "this software is based in part on the
 * work of Mark Callow."
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
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

#define LT_VK_MAJOR_VERSION 1
#define LT_VK_MINOR_VERSION 0
#define LT_VK_PATCH_VERSION 0
#define LT_VK_VERSION VK_MAKE_VERSION(1, 0, 0)

VulkanLoadTests::VulkanLoadTests(const sampleInvocation samples[],
								 const int numSamples,
								 const char* const name)
				  : siSamples(samples), iNumSamples(numSamples),
					VulkanAppSDL(name, 1280, 720, LT_VK_VERSION, true)
{
    iCurSampleNum = 0;
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

	// Not getting an initialize resize event, at least on Mac OS X.
	// Therefore use invokeSample which calls the sample's resize func.
	invokeSample(iCurSampleNum, Direction::eForward);
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
    bool done = true;
    switch (event->type) {
      case SDL_KEYUP:
        switch (event->key.keysym.sym) {
          case 'q':
            quit = true;
            break;
          case 'n':
            if (++iCurSampleNum >= iNumSamples)
              iCurSampleNum = 0;
            invokeSample(iCurSampleNum, Direction::eForward);
            break;
          case 'p':
            if (--iCurSampleNum < 0)
              iCurSampleNum = iNumSamples-1;
            invokeSample(iCurSampleNum, Direction::eBack);
            break;

          default:
            done = false;
        }
        break;
      case SDL_MOUSEBUTTONDOWN:
        // Forward to sample in case this is the start of motion.
        done = false;
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
        done = false;
        switch (event->button.button) {
          case SDL_BUTTON_LEFT:
            if (SDL_abs(event->button.x - buttonDown.x) < 5
                && SDL_abs(event->button.y - buttonDown.y) < 5
				&& (event->button.timestamp - buttonDown.timestamp) < 40) {
                // Advance to the next sample.
                if (++iCurSampleNum >= iNumSamples)
                    iCurSampleNum = 0;
                invokeSample(iCurSampleNum, Direction::eForward);
            }
            break;
          default:
            break;
        }
        break;
      default:
          done = false;
    }
    if (!done && pCurSample->doEvent(event) != 0)
        return VulkanAppSDL::doEvent(event);
    else
        return 0;
}


void
VulkanLoadTests::windowResized()
{
    if (pCurSample != NULL)
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
VulkanLoadTests::invokeSample(int& iSampleNum, Direction dir)
{
    const sampleInvocation* sampleInv;
    class unsupported_ctype : public std::runtime_error {
      public:
       unsupported_ctype()
            : std::runtime_error("Unsupported compression type") { }
    };

    prepared = false;  // Prevent any more rendering.
    if (pCurSample != nullptr) {
        vkctx.queue.waitIdle(); // Wait for current rendering to finish.
        delete pCurSample;
    }
    sampleInv = &siSamples[iSampleNum];

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
        	dir == Direction::eForward ? ++iSampleNum : --iSampleNum;
            sampleInv = &siSamples[iSampleNum];
		} catch (std::exception& e) {
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
					sampleInv->title,
					e.what(), NULL);
        	dir == Direction::eForward ? ++iSampleNum : --iSampleNum;
			sampleInv = &siSamples[iSampleNum];
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
#if 0
    { &sc_Sample01, "testimages/hi_mark.ktx", "RGB8 NPOT HI Logo" },
    { &sc_Sample01, "testimages/luminance_unsized_reference.ktx", "Luminance (Unsized)" },
    { &sc_Sample01, "testimages/luminance_sized_reference.ktx", "Luminance (Sized)" },
    { &sc_Sample01, "testimages/up-reference.ktx", "RGB8" },
    { &sc_Sample01, "testimages/down-reference.ktx", "RGB8 + KTXOrientation"},
    { &sc_Sample01, "testimages/etc1.ktx", "ETC1 RGB8" },
    { &sc_Sample01, "testimages/etc2-rgb.ktx", "ETC2 RGB8"},
    { &sc_Sample01, "testimages/etc2-rgba1.ktx", "ETC2 RGB8A1"},
    { &sc_Sample01, "testimages/etc2-rgba8.ktx", "ETC2 RGB8A8" },
    { &sc_Sample01, "testimages/etc2-sRGB.ktx", "ETC2 sRGB8"},
    { &sc_Sample01, "testimages/etc2-sRGBa1.ktx", "ETC2 sRGB8A1"},
    { &sc_Sample01, "testimages/etc2-sRGBa8.ktx", "ETC2 sRGB8A8" },
    { &sc_Sample01, "testimages/rgba-reference.ktx", "RGBA8"},
    { &sc_Sample01, "testimages/rgb-reference.ktx", "RGB8" },
    { &sc_Sample01, "testimages/conftestimage_R11_EAC.ktx", "ETC2 R11"},
    { &sc_Sample01, "testimages/conftestimage_SIGNED_R11_EAC.ktx", "ETC2 Signed R11" },
    { &sc_Sample01, "testimages/conftestimage_RG11_EAC.ktx", "ETC2 RG11" },
    { &sc_Sample01, "testimages/conftestimage_SIGNED_RG11_EAC.ktx", "ETC2 Signed RG11" },
    { &sc_Sample02, "testimages/rgb-amg-reference.ktx", "RGB8 + Auto Mipmap" },
    { &sc_Sample02, "testimages/rgb-mipmap-reference.ktx", "Color/level mipmap" },
    { &sc_Sample02, "testimages/hi_mark_sq.ktx", "RGB8 NPOT HI Logo" }
#endif
#if 1
    { Texture::create,
      "testimages/rgba-reference.ktx",
      VulkanLoadTests::CompressionType::eNone,
      "RGBA unsized 2D"
    },
#if !defined(__IPHONEOS__) && !defined(__MACOS__)
    // Uncompressed RGB formats not supported by Metal.
    { Texture::create,
      "testimages/orient-down-metadata-sized.ktx",
      VulkanLoadTests::CompressionType::eNone,
     "RGB8 2D + KTXOrientation down"
    },
    { Texture::create,
      "testimages/orient-up-metadata-sized.ktx",
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
        "testimages/pattern_02_bc2.ktx",
        VulkanLoadTests::CompressionType::eBC,
        "BC2 (S3TC DXT3) Compressed 2D"
    },
#endif
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

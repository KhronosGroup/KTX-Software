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
	invokeSample(iCurSampleNum);
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
            // TODO advance to next sample when no keyboard
            if (++iCurSampleNum >= iNumSamples)
              iCurSampleNum = 0;
            invokeSample(iCurSampleNum);
            break;
          case 'p':
            // TODO advance to next sample when no keyboard
            if (--iCurSampleNum < 0)
              iCurSampleNum = iNumSamples-1;
            invokeSample(iCurSampleNum);
            break;

          default:
            done = false;
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
VulkanLoadTests::invokeSample(int& iSampleNum)
{
    int width, height;
    const sampleInvocation* sampleInv;

    if (pCurSample != nullptr) {
        delete pCurSample;
    }
    SDL_GetWindowSize(pswMainWindow, &width, & height);
    prepared = false;
    sampleInv = &siSamples[iSampleNum];

	for (;;) {
		try {
			pCurSample = sampleInv->createSample(vkctx, width, height,
									sampleInv->args, sBasePath);
			break;
		} catch (std::exception& e) {
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
					sampleInv->title,
					e.what(), NULL);
			sampleInv = &siSamples[++iSampleNum];
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
    { Texture::create,
      "testimages/pattern_02_bc2.ktx",
      "BC2 (S3TC DXT3) Compressed 2D"
    },
    { Texture::create,
      "testimages/orient-down-metadata-sized.ktx",
      "RGB8 2D + KTXOrientation down"
    },
    { Texture::create,
      "testimages/orient-up-metadata-sized.ktx",
      "RGB8 2D + KTXOrientation up"
    },
    { TextureArray::create,
      "testimages/texturearray_bc3.ktx",
      "BC3 (S3TC DXT5) Texture Array"
    },
    { TextureCubemap::create,
      "testimages/cubemap_yokohama.ktx",
      "Cube Map"
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

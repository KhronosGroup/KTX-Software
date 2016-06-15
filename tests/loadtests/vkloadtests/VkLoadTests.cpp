/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/* $Id: ac63511da134f2c25a9e1da86a36bc27b6198ae3 $ */

/**
 * @internal
 * @file VkLoadTests.cpp
 * @~English
 *
 * @brief VkLoadTests app class for Vulkan.
 *
 * @author Mark Callow
 * @copyright (c) 2016, Mark Callow.
 */

/*
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


#include "VkLoadTests.h"
#include "VkSample_02_cube_textured.h"

#define LT_VK_MAJOR_VERSION 1
#define LT_VK_MINOR_VERSION 0
#define LT_VK_PATCH_VERSION 0
#define LT_VK_VERSION VK_MAKE_VERSION(1, 0, 0)

VkLoadTests::VkLoadTests(const sampleInvocation samples[],
                         const int numSamples,
                         const char* const name)
          : siSamples(samples), iNumSamples(numSamples),
            VkAppSDL(name, 640, 480, LT_VK_VERSION)
{
    iCurSampleNum = 0;
    pCurSample = NULL;
}

VkLoadTests::~VkLoadTests()
{
    if (pCurSample != NULL) {
        pCurSample->finalize();
        delete pCurSample;
    }
}

bool
VkLoadTests::initialize(int argc, char* argv[])
{
	if (!VkAppSDL::initialize(argc, argv))
		return false;
    
    szBasePath = SDL_GetBasePath();
    if (szBasePath == NULL)
        szBasePath = SDL_strdup("./");
    
    // Not getting an initialize resize event, at least on Mac OS X.
    // Therefore use invokeSample which calls the sample's resize func.
    invokeSample(iCurSampleNum);
    
    return AppBaseSDL::initialize(argc, argv);
}


void
VkLoadTests::finalize()
{
    pCurSample->finalize();
	VkAppSDL::finalize();
}


int
VkLoadTests::doEvent(SDL_Event* event)
{
    switch (event->type) {
      case SDL_MOUSEBUTTONUP:
        switch (event->button.button) {
          case SDL_BUTTON_LEFT:
            pCurSample->finalize();
            delete pCurSample;
            if (++iCurSampleNum >= iNumSamples)
              iCurSampleNum = 0;
            invokeSample(iCurSampleNum);
            return 0;
          default:
            ;
        }
        break;
      default:
        ;
    }
    return VkAppSDL::doEvent(event);
}


void
VkLoadTests::resize(int width, int height)
{
    VkAppSDL::resize(width, height);
    if (pCurSample != NULL)
        pCurSample->resize(width, height);
}


void
VkLoadTests::drawFrame(int ticks)
{
    pCurSample->run(ticks);

    VkAppSDL::drawFrame(ticks);
}


void
VkLoadTests::invokeSample(int iSampleNum)
{
    int width, height;
    const sampleInvocation* sampleInv;

    sampleInv = &siSamples[iSampleNum];
    pCurSample = sampleInv->createSample(vcpCommandPool, vdDevice,
                                         vrpRenderPass, swapchain,
                                         sampleInv->args, szBasePath);
    
    SDL_GetWindowSize(pswMainWindow, &width, & height);
    setWindowTitle(sampleInv->title);
    //pCurSample->resize(width, height);
}


void
VkLoadTests::onFPSUpdate()
{
    // Using onFPSUpdate avoids rewriting the title every frame.
    setWindowTitle(siSamples[iCurSampleNum].title);
}

extern "C" {
/* ----------------------------------------------------------------------------- */
#if 0
/* SAMPLE 01 */
void atInitialize_01_draw_texture(void** ppAppData, const char* const szArgs,
                                  const char* const szBasePath);
void atRelease_01_draw_texture(void* pAppData);
void atResize_01_draw_texture(void* pAppData, int iWidth, int iHeight);
void atRun_01_draw_texture(void* pAppData, int iTimeMS);

static const atSample sc_Sample01 = {
    atInitialize_01_draw_texture,
    atRelease_01_draw_texture,
    atResize_01_draw_texture,
    atRun_01_draw_texture,
};
#endif

/* ----------------------------------------------------------------------------- */
#if 0
/* SAMPLE 02 */
void atInitialize_02_cube(void** ppAppData, const char* const szArgs,
                          const char* const szBasePath);
void atRelease_02_cube(void* pAppData);
void atResize_02_cube(void* pAppData, int iWidth, int iHeight);
void atRun_02_cube(void* pAppData, int iTimeMS);

static const atSample sc_Sample02 = {
    atInitialize_02_cube,
    atRelease_02_cube,
    atResize_02_cube,
    atRun_02_cube,
};
#endif
};

/* ----------------------------------------------------------------------------- */

const VkLoadTests::sampleInvocation siSamples[] = {
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
    { VkSample_02_cube_textured::create,
      "testimages/rgb-amg-reference.ktx",
      "RGB8 + Auto Mipmap"
    },
};

const int iNumSamples = sizeof(siSamples) / sizeof(VkLoadTests::sampleInvocation);

AppBaseSDL* theApp = new VkLoadTests(siSamples, iNumSamples,
                                     "KTX Loader Tests for Vulkan");

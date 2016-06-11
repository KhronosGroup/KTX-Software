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
    pCurSampleInv = &siSamples[0];
	pCurSampleData = NULL;
}

bool
VkLoadTests::initialize(int argc, char* argv[])
{
	if (!VkAppSDL::initialize(argc, argv))
		return false;
    
    szBasePath = SDL_GetBasePath();
    if (szBasePath == NULL)
        szBasePath = SDL_strdup("./");
    
    VkCommandBufferAllocateInfo aInfo;
    aInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    aInfo.pNext = NULL;
    aInfo.commandPool = vcpCommandPool;
    aInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    aInfo.commandBufferCount = 1;

    for (int i = 0; i < swapchain.imageCount; i++) {
        VkResult U_ASSERT_ONLY err =
                          vkAllocateCommandBuffers(vdDevice, &aInfo,
                                                   &swapchain.buffers[i].cmd);
        assert(!err);
        buildCommandBuffer(i);
    }


    // Not getting an initialize resize event, at least on Mac OS X.
    // Therefore use invokeSample which calls the sample's resize func.
    invokeSample(iCurSampleNum);
    
    return AppBaseSDL::initialize(argc, argv);
}


void
VkLoadTests::finalize()
{
    pCurSampleInv->sample->pfRelease(pCurSampleData);
	VkAppSDL::finalize();
}


int
VkLoadTests::doEvent(SDL_Event* event)
{
    switch (event->type) {
      case SDL_MOUSEBUTTONUP:
        switch (event->button.button) {
          case SDL_BUTTON_LEFT:
            pCurSampleInv->sample->pfRelease(pCurSampleData);
            if (++iCurSampleNum >= iNumSamples)
              iCurSampleNum = 0;
            pCurSampleInv = &siSamples[iCurSampleNum];
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
    if (pCurSampleData != NULL)
        pCurSampleInv->sample->pfResize(pCurSampleData, width, height);
}


void
VkLoadTests::drawFrame(int ticks)
{
    pCurSampleInv->sample->pfRun(pCurSampleData, ticks);

    VkAppSDL::drawFrame(ticks);
}


void
VkLoadTests::buildCommandBuffer(int bufferIndex)
{
    // XXX This should be moved into sample's initialize function.
    // Doing so requires changes to sample framework that I don't want
    // to do right now.

    VkResult U_ASSERT_ONLY err;

    const VkCommandBufferBeginInfo cmd_buf_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL, 0, NULL
    };

    VkClearValue clear_values[2] = {
       { bufferIndex * 1.f, 0.2f, 0.2f, 1.0f },
       { 0.0f, 0 }
    };

    const VkRenderPassBeginInfo rp_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        NULL,
        vrpRenderPass,
        swapchain.buffers[bufferIndex].fb,
        { 0, 0, swapchain.extent.width, swapchain.extent.height },
        2,
        clear_values,
    };

    err = vkBeginCommandBuffer(swapchain.buffers[bufferIndex].cmd,
                               &cmd_buf_info);
    assert(!err);

    // We can use LAYOUT_UNDEFINED as a wildcard here because we don't care what
    // happens to the previous contents of the image
    VkImageMemoryBarrier image_memory_barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        NULL,
        0,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        swapchain.buffers[bufferIndex].image,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };
    vkCmdPipelineBarrier(swapchain.buffers[bufferIndex].cmd,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0, 0, NULL, 0,
                         NULL, 1, &image_memory_barrier);

    vkCmdBeginRenderPass(swapchain.buffers[bufferIndex].cmd, &rp_begin,
                         VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(swapchain.buffers[bufferIndex].cmd);

    VkImageMemoryBarrier present_barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        NULL,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_MEMORY_READ_BIT,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        swapchain.buffers[bufferIndex].image,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
    };
    present_barrier.image = swapchain.buffers[bufferIndex].image;

    vkCmdPipelineBarrier(
        swapchain.buffers[bufferIndex].cmd,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0, 0, NULL, 0, NULL, 1, &present_barrier);

    err = vkEndCommandBuffer(swapchain.buffers[bufferIndex].cmd);
    assert(!err);

}

void
VkLoadTests::invokeSample(int iSampleNum)
{
    int width, height;

    pCurSampleInv = &siSamples[iSampleNum];
    pCurSampleInv->sample->pfInitialize(
                            &pCurSampleData,
                            pCurSampleInv->args,
                            szBasePath);
    
    SDL_GetWindowSize(pswMainWindow, &width, & height);
    setWindowTitle(pCurSampleInv->title);
    pCurSampleInv->sample->pfResize(pCurSampleData, width, height);
}


void
VkLoadTests::onFPSUpdate()
{
    // Using onFPSUpdate avoids rewriting the title every frame.
    setWindowTitle(pCurSampleInv->title);
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
#endif
    { &sc_Sample02, "testimages/rgb-amg-reference.ktx", "RGB8 + Auto Mipmap" },
#if 0
    { &sc_Sample02, "testimages/rgb-mipmap-reference.ktx", "Color/level mipmap" },
    { &sc_Sample02, "testimages/hi_mark_sq.ktx", "RGB8 NPOT HI Logo" }
#endif
};

const int iNumSamples = sizeof(siSamples) / sizeof(VkLoadTests::sampleInvocation);

AppBaseSDL* theApp = new VkLoadTests(siSamples, iNumSamples,
                                     "KTX Loader Tests for Vulkan");

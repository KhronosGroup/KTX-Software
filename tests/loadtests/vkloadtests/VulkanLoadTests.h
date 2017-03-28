/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

#ifndef VULKAN_LOAD_TESTS_H
#define VULKAN_LOAD_TESTS_H

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

#include "VulkanAppSDL.h"
#include "utils/VulkanMeshLoader.hpp"
#include "VulkanLoadTestSample.h"
#include <string>

class VulkanLoadTests : public VulkanAppSDL {
  public:
    enum class CompressionType {
        eNone,
        eBC,
        eASTC_LDR,
        eETC2
    };
    /** A table of samples and arguments */
    typedef struct sampleInvocation_ {
        const VulkanLoadTestSample::PFN_create createSample;
        const char* const args;
        const CompressionType ctype;
        const char* const title;
    } sampleInvocation;
    
    VulkanLoadTests(const sampleInvocation samples[],
                const int numSamples,
                const char* const name);
    virtual ~VulkanLoadTests();
    virtual int doEvent(SDL_Event* event);
    virtual void drawFrame(uint32_t msTicks);
    virtual void finalize();
    virtual void getOverlayText(VulkanTextOverlay * textOverlay);
    virtual bool initialize(int argc, char* argv[]);
    virtual void onFPSUpdate();
    virtual void windowResized();

  protected:
    void invokeSample(int& iSampleNum);
    int iCurSampleNum;
    VulkanLoadTestSample* pCurSample;

    bool quit = false;

    const sampleInvocation* const siSamples;
    const int iNumSamples;
    
    struct {
        int32_t x;
        int32_t y;
        uint32_t timestamp;
    } buttonDown;
};

#endif /* VULKAN_LOAD_TESTS_H */

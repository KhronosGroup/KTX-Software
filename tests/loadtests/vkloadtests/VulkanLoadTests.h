/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

#ifndef VULKAN_LOAD_TESTS_H
#define VULKAN_LOAD_TESTS_H

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "VulkanAppSDL.h"
#include "utils/VulkanMeshLoader.hpp"
#include "SwipeDetector.h"
#include "VulkanLoadTestSample.h"
#include <string>

class VulkanLoadTests : public VulkanAppSDL {
  public:
    /** A table of samples and arguments */
    typedef struct sampleInvocation_ {
        const VulkanLoadTestSample::PFN_create createSample;
        const char* const args;
        const char* const title;
    } sampleInvocation;
    
    VulkanLoadTests(const sampleInvocation samples[],
                const uint32_t numSamples,
                const char* const name);
    virtual ~VulkanLoadTests();
    virtual int doEvent(SDL_Event* event);
    virtual void drawFrame(uint32_t msTicks);
    virtual void finalize();
    virtual void getOverlayText(float yOffset);
    virtual bool initialize(Args& args);
    virtual void onFPSUpdate();
    virtual void windowResized();

  protected:
    enum class Direction {
        eForward,
        eBack
    };
    void invokeSample(Direction dir);
    VulkanLoadTestSample* showFile(std::string& filename);
    VulkanLoadTestSample* pCurSample;

    bool quit = false;

    const sampleInvocation* const siSamples;
    class sampleIndex {
      public:
        sampleIndex(const int32_t numSamples) : numSamples(numSamples) {
            index = 0;
        }
        sampleIndex& operator++() {
            if (++index >= numSamples)
                index = 0;
            return *this;
        }
        sampleIndex& operator--() {
            if (--index > numSamples /* underflow */)
                index = numSamples-1;
            return *this;
        }
        operator int32_t() {
            return index;
        }
        uint32_t getNumSamples() { return numSamples; }
        void setNumSamples(uint32_t ns) { numSamples = ns; }

      protected:
        uint32_t numSamples;
        uint32_t index;
    } sampleIndex;

    std::vector<std::string> infiles;
    
    struct {
        int32_t x;
        int32_t y;
        uint32_t timestamp;
    } buttonDown;

    SwipeDetector swipeDetector;
};

#endif /* VULKAN_LOAD_TESTS_H */

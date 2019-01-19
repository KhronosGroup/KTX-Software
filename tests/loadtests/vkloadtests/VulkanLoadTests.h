/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

#ifndef VULKAN_LOAD_TESTS_H
#define VULKAN_LOAD_TESTS_H

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
                const int numSamples,
                const char* const name);
    virtual ~VulkanLoadTests();
    virtual int doEvent(SDL_Event* event);
    virtual void drawFrame(uint32_t msTicks);
    virtual void finalize();
    virtual void getOverlayText(VulkanTextOverlay * textOverlay, float yOffset);
    virtual bool initialize(int argc, char* argv[]);
    virtual void onFPSUpdate();
    virtual void windowResized();

  protected:
    enum class Direction {
        eForward,
        eBack
    };
    void invokeSample(Direction dir);
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
            if (--index < 0)
                index = numSamples-1;
            return *this;
        }
        operator int32_t() {
            return index;
        }
      protected:
        const int32_t numSamples;
        int32_t index;
    } sampleIndex;
    
    struct {
        int32_t x;
        int32_t y;
        uint32_t timestamp;
    } buttonDown;

    SwipeDetector swipeDetector;
};

#endif /* VULKAN_LOAD_TESTS_H */

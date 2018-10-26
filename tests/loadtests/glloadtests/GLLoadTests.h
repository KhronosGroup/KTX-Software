/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

#ifndef GL_LOAD_TESTS_H
#define GL_LOAD_TESTS_H

/*
 * ©2015-2018 Mark Callow.
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
 * @file LoadTests.h
 * @~English
 *
 * @brief Definition of app for running a set of OpenGL load tests.
 *
 * @author Mark Callow, www.edgewise-consulting.com
 * @copyright © 2015-2018, Mark Callow.
 */

#include <string>

#include "GLAppSDL.h"
#include "LoadTestSample.h"
#include "SwipeDetector.h"

class GLLoadTests : public GLAppSDL {
  public:
    /** A table of samples and arguments */
    typedef struct sampleInvocation_ {
        const LoadTestSample::PFN_create createSample;
        const char* const args;
        const char* const title;
    } sampleInvocation;
    
    GLLoadTests(const sampleInvocation samples[],
                const int numSamples,
                const char* const name,
                const SDL_GLprofile profile,
                const int majorVersion,
                const int minorVersion);
    virtual ~GLLoadTests();
    virtual int doEvent(SDL_Event* event);
    virtual void drawFrame(uint32_t msTicks);
    virtual void finalize();
    //virtual void getOverlayText(TextOverlay* textOverlay, float yOffset);
    virtual bool initialize(int argc, char* argv[]);
    virtual void onFPSUpdate();
    virtual void windowResized();

  protected:
    enum class Direction {
        eForward,
        eBack
    };
    void invokeSample(Direction dir);
    LoadTestSample* pCurSample;

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

#endif /* GL_LOAD_TESTS_H */

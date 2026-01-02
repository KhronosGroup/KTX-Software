/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

#ifndef GL_LOAD_TESTS_H
#define GL_LOAD_TESTS_H

/*
 * Copyright 2015-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file
 * @~English
 *
 * @brief Definition of app for running a set of OpenGL load tests.
 */

#include <string>
#include <vector>

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
                const uint32_t numSamples,
                const char* const name,
                const SDL_GLProfile profile,
                const int majorVersion,
                const int minorVersion);
    virtual ~GLLoadTests();
    virtual bool doEvent(SDL_Event* event);
    virtual void drawFrame(uint32_t msTicks);
    virtual void finalize();
    //virtual void getOverlayText(TextOverlay* textOverlay, float yOffset);
    virtual bool initialize(Args& args);
    virtual void onFPSUpdate();
    virtual void windowResized();

  protected:
    enum class Direction {
        eForward,
        eBack
    };
    void invokeSample(Direction dir);
    LoadTestSample* showFile(const std::string& filename);
    LoadTestSample* pCurSample;

    bool quit = false;

    const sampleInvocation* const siSamples;
    class sampleIndex {
      public:
        sampleIndex(const uint32_t numSamples) : numSamples(numSamples) {
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
    
    Sint64 dropCompleteTime = 0;
    struct {
        float x;
        float y;
        Sint64 timestamp;
    } buttonDown;

    SwipeDetector swipeDetector;
};

#endif /* GL_LOAD_TESTS_H */

/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

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
 * @file GLLoadTests.cpp
 * @~English
 *
 * @brief Implementation of app for running a set of OpenGL load tests.
 *
 * @author Mark Callow, www.edgewise-consulting.com
 * @copyright © 2015-2018, Mark Callow.
 */

#include <exception>

#include "GLLoadTests.h"
#include "ltexceptions.h"

GLLoadTests::GLLoadTests(const sampleInvocation samples[],
                         const int numSamples,
                         const char* const name,
                         const SDL_GLprofile profile,
                         const int majorVersion,
                         const int minorVersion)
                : siSamples(samples), sampleIndex(numSamples),
                  GLAppSDL(name, 640, 480, profile, majorVersion, minorVersion)
{
    pCurSample = nullptr;
}

GLLoadTests::~GLLoadTests()
{
    if (pCurSample != nullptr) {
        delete pCurSample;
    }
}

bool
GLLoadTests::initialize(int argc, char* argv[])
{
    if (!GLAppSDL::initialize(argc, argv))
        return false;

    // Launch the first sample.
    invokeSample(Direction::eForward);
    return AppBaseSDL::initialize(argc, argv);
}


void
GLLoadTests::finalize()
{
    if (pCurSample != nullptr) {
        delete pCurSample;
    }
    GLAppSDL::finalize();
}


int
GLLoadTests::doEvent(SDL_Event* event)
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
        switch(swipeDetector.doEvent(event)) {
          case SwipeDetector::eSwipeUp:
          case SwipeDetector::eSwipeDown:
          case SwipeDetector::eEventConsumed:
            break;
          case SwipeDetector::eSwipeLeft:
            ++sampleIndex;
            invokeSample(Direction::eForward);
            break;
          case SwipeDetector::eSwipeRight:
            --sampleIndex;
            invokeSample(Direction::eBack);
            break;
          case SwipeDetector::eEventNotConsumed:
            result = 1;
          }
    }
    
    if (result == 1) {
        // Further processing required.
        if (pCurSample != nullptr)
            result = pCurSample->doEvent(event);  // Give sample a chance.
        if (result == 1)
            return GLAppSDL::doEvent(event);  // Pass to base class.
    }
    return result;
}


void
GLLoadTests::windowResized()
{
    if (pCurSample != nullptr)
        pCurSample->resize(w_width, w_height);
}


void
GLLoadTests::drawFrame(uint32_t msTicks)
{
    pCurSample->run(msTicks);

    GLAppSDL::drawFrame(msTicks);
}

#if 0
void
GLLoadTests::getOverlayText(GLTextOverlay * textOverlay)
{
    if (enableTextOverlay && pCurSample != nullptr) {
        pCurSample->getOverlayText(textOverlay);
    }
}
#endif

void
GLLoadTests::invokeSample(Direction dir)
{
    const sampleInvocation* sampleInv;

    if (pCurSample != nullptr) {
        delete pCurSample;
        // Certain events can be triggered during new sample initialization
        // while pCurSample is not valid, e.g. FOCUS_LOST. Protect against
        // problems from this by indicating there is no current sample.
        pCurSample = nullptr;
    }
    sampleInv = &siSamples[sampleIndex];

    for (;;) {
        try {
            pCurSample = sampleInv->createSample(w_width, w_height,
                                                 sampleInv->args, sBasePath);
            break;
        } catch (unsupported_ctype& e) {
            (void)e; // To quiet unused variable warnings from some compilers.
            dir == Direction::eForward ? ++sampleIndex : --sampleIndex;
            sampleInv = &siSamples[sampleIndex];
        } catch (std::exception& e) {
            printf("**** %s\n", e.what());
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                    sampleInv->title,
                    e.what(), NULL);
            dir == Direction::eForward ? ++sampleIndex : --sampleIndex;
            sampleInv = &siSamples[sampleIndex];
        }
    }
    setAppTitle(sampleInv->title);
    pCurSample->resize(w_width, w_height);
}


void
GLLoadTests::onFPSUpdate()
{
    // Using onFPSUpdate avoids rewriting the title every frame.
    //setWindowTitle(pCurSample->title);
    GLAppSDL::onFPSUpdate();
}


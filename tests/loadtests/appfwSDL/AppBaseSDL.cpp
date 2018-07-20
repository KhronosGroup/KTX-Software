/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/* $Id: f63e0a9e6eed51ed84a8eea1eff0708c8a6af22b $ */

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
 * @file AppBaseSDL.cpp
 * @~English
 *
 * @brief Base class for SDL applications.
 *
 * @author Mark Callow, www.edgewise-consulting.com
 * @copyright © 2015-2018, Mark Callow.
 */

#include "AppBaseSDL.h"
#include <iomanip>
#include <sstream>

bool
AppBaseSDL::initialize(int argc, char* argv[])
{
    const char* basePath = SDL_GetBasePath();
    if (basePath == NULL)
        basePath = SDL_strdup("./");
    sBasePath = basePath;
    SDL_free((void *)basePath);
    return true;
}


void
AppBaseSDL::initializeFPSTimer()
{
    lastFrameTime = 0;
    fpsCounter.numFrames = 0;
    fpsCounter.lastFPS = 0;
    startTicks = fpsCounter.startTicks = SDL_GetPerformanceCounter();
}


void
AppBaseSDL::finalize() {
    
}


int
AppBaseSDL::doEvent(SDL_Event* event)
{
    switch (event->type) {
      case SDL_QUIT:
        finalize();
        exit(0);
    }
    return 1;    
}


void
AppBaseSDL::onFPSUpdate()
{
    
}


//  Protected, non-virtual
void
AppBaseSDL::drawFrame()
{
    ticks_t ticks = SDL_GetPerformanceCounter();
    Uint64 tps = SDL_GetPerformanceFrequency();
    uint32_t msTicksSinceStart = (uint32_t)((ticks - startTicks) * 1000 / tps);

    drawFrame(msTicksSinceStart);

    ticks_t endTicks = SDL_GetPerformanceCounter();
    lastFrameTime = (1000.0f * (endTicks - ticks)) / tps;
    fpsCounter.numFrames++;
    if (endTicks - fpsCounter.startTicks > tps) {
        fpsCounter.lastFPS = (float)(fpsCounter.numFrames * tps)
                             / (endTicks - fpsCounter.startTicks);
        onFPSUpdate(); // Notify listeners that fps value has been updated.
        fpsCounter.startTicks = endTicks;
        fpsCounter.numFrames = 0;
    }
}

//----------------------------------------------------------------------
//  Window title and text overlay functions
//----------------------------------------------------------------------


void
AppBaseSDL::setAppTitle(const char* const szExtra)
{
    appTitle = name();
    if (szExtra != NULL && szExtra[0] != '\0') {
        appTitle += ": ";
        appTitle += szExtra;
    }
    setWindowTitle();
}


void
AppBaseSDL::setWindowTitle()
{
    std::stringstream ss;
    std::string wt;

    ss << std::fixed << std::setprecision(2)
       << lastFrameTime << "ms (" << fpsCounter.lastFPS << " fps)" << " ";
    wt = ss.str();
    wt += appTitle;
    SDL_SetWindowTitle(pswMainWindow, wt.c_str());
}


const char* const appName()
{
    return theApp->name();
}


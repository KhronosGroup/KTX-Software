/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/* $Id: ac63511da134f2c25a9e1da86a36bc27b6198ae3 $ */

/**
 * @internal
 * @file LoadTestsGL.cpp
 * @~English
 *
 * @brief LoadTests app class.
 *
 * @author Mark Callow
 * @copyright (c) 2015, Mark Callow.
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
 * work of HI Corporation."
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */


#include "LoadTests.h"


LoadTests::LoadTests(const sampleInvocation samples[],
                     const int numSamples,
                     const char* const name)
          : siSamples(samples), iNumSamples(numSamples),
            GLAppSDL(name, 640, 480, GL_CONTEXT_PROFILE,
                     GL_CONTEXT_MAJOR_VERSION, GL_CONTEXT_MINOR_VERSION)
{
    iCurSampleNum = 0;
    pCurSampleInv = &siSamples[0];
	pCurSampleData = NULL;
}

bool
LoadTests::initialize(int argc, char* argv[])
{
	if (!GLAppSDL::initialize(argc, argv))
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
LoadTests::finalize()
{
    pCurSampleInv->sample->pfRelease(pCurSampleData);
}


int
LoadTests::doEvent(SDL_Event* event)
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
    return GLAppSDL::doEvent(event);
}


void
LoadTests::resize(int width, int height)
{
    if (pCurSampleData != NULL)
        pCurSampleInv->sample->pfResize(pCurSampleData, width, height);
}


void
LoadTests::drawFrame(int ticks)
{
    pCurSampleInv->sample->pfRun(pCurSampleData, ticks);
    GLAppSDL::drawFrame(ticks);
}


void
LoadTests::invokeSample(int iSampleNum)
{
    int width, height;

    pCurSampleInv = &siSamples[iSampleNum];
    pCurSampleInv->sample->pfInitialize(
                            &pCurSampleData,
                            (szBasePath + pCurSampleInv->args).c_str()
                                       );
    
    SDL_GL_GetDrawableSize(pswMainWindow, &width, & height);
    setWindowTitle(pCurSampleInv->title);
    pCurSampleInv->sample->pfResize(pCurSampleData, width, height);
}


void
LoadTests::onFPSUpdate()
{
    // Using onFPSUpdate avoids rewriting the title every frame.
    setWindowTitle(pCurSampleInv->title);
}

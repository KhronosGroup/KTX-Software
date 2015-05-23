/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/* $Id: f63e0a9e6eed51ed84a8eea1eff0708c8a6af22b $ */

/**
 * @internal
 * @file AppBaseSDL.cpp
 * @~English
 *
 * @brief Base class for SDL applications.
 *
 * @author Mark Callow
 * @copyright (c) 2015, Mark Callow.
 */

/*
Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and/or associated documentation files (the
"Materials"), to deal in the Materials without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Materials, and to
permit persons to whom the Materials are furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
unaltered in all copies or substantial portions of the Materials.
Any additions, deletions, or changes to the original source files
must be clearly indicated in accompanying documentation.

If only executable code is distributed, then the accompanying
documentation must state that "this software is based in part on the
work of Mark Callow."

THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

#include "AppBaseSDL.h"

bool
AppBaseSDL::initialize(int argc, char* argv[])
{
    lFPSTimeStart = SDL_GetTicks();
    iFPSFrames = 0;
    return true;
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


void
AppBaseSDL::update(int ticks)
{
    iFPSFrames++;
    if (ticks - lFPSTimeStart > 1000) {
        fFPS = (float)(iFPSFrames * 1000) / (ticks - lFPSTimeStart);
        onFPSUpdate(); // Notify listeners that fps value has been updated.
        lFPSTimeStart = ticks;
        iFPSFrames = 0;
    }
}

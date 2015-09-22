/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/* $Id: f63e0a9e6eed51ed84a8eea1eff0708c8a6af22b $ */

/**
 * @internal
 * @file AppBaseSDL.h
 * @~English
 *
 * @brief Declarations for App framework using SDL.
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

#include <SDL2/SDL.h>
#include <string>

extern class AppBaseSDL* theApp;

class AppBaseSDL {
  public:
    AppBaseSDL(const char* const name) : szName(name) { }
    virtual bool initialize(int argc, char* argv[]);
    virtual void finalize();
    virtual void drawFrame(int ticks);
    virtual int doEvent(SDL_Event* event);
    virtual void onFPSUpdate();
    
    void initializeFPSTimer();
    const char* const name() { return szName; }
    std::string basePath() { return sBasePath; }
    
    static int onEvent(void* userdata, SDL_Event* event) {
        return ((AppBaseSDL *)userdata)->doEvent(event);
    }
    
    static void onDrawFrame(void* userdata) {
        ((AppBaseSDL *)userdata)->drawFrame(SDL_GetTicks());
    }

protected:
    long lFPSTimeStart;
    int iFPSFrames;
    float fFPS;
    
    const char* const szName;
    std::string sBasePath;

};







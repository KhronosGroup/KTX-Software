/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

#ifndef APP_BASE_SDL_H_1456211087
#define APP_BASE_SDL_H_1456211087

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

class AppBaseSDL {
  public:
    typedef Uint64 ticks_t;
    AppBaseSDL(const char* const name) : szName(name), appTitle(name) { }
    virtual bool initialize(int argc, char* argv[]);
    virtual void finalize();
    // Ticks in milliseconds since start.
    virtual void drawFrame(uint32_t msTicks) { }
    // When used with SDL_SetEventWatch, return value is ignored. When used
    // with SDL_SetEventFilter, 1 causes event to be added to SDL's internal
    // event queue, 0 causes it to be dropped.
    virtual int doEvent(SDL_Event* event);
    virtual void onFPSUpdate();
    virtual SDL_Window* getMainWindow() { return pswMainWindow; }
    
    void drawFrame();
    void initializeFPSTimer();
    const char* const name() { return szName; }
    const std::string getAssetPath() { return sBasePath; }

    // Sets title to be used on window title bar. Content of szExtra ia
    // appended to the app name.
    virtual void setAppTitle(const char* const szExtra);

    static int onEvent(void* userdata, SDL_Event* event) {
        return ((AppBaseSDL *)userdata)->doEvent(event);
    }
    
    static void onDrawFrame(void* userdata) {
        ((AppBaseSDL *)userdata)->drawFrame();
    }

  protected:
    // Sets text on window title bar. Fps value is preprended to appTitle.
    virtual void setWindowTitle();

    ticks_t startTicks;
    float lastFrameTime;  // ms
    struct fpsCounter {
        ticks_t startTicks;
        int numFrames;
        float lastFPS;
    } fpsCounter;
    
    SDL_Window* pswMainWindow;
    
    const char* const szName;
    std::string appTitle;
    std::string sBasePath;

};

extern class AppBaseSDL* theApp;

#endif /* APP_BASE_SDL_H_1456211087 */





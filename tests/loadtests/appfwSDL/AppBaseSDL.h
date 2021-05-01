/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

#ifndef APP_BASE_SDL_H_1456211087
#define APP_BASE_SDL_H_1456211087

/* $Id: f63e0a9e6eed51ed84a8eea1eff0708c8a6af22b $ */

/*
 * Copyright 2015-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file AppBaseSDL.h
 * @~English
 *
 * @brief Declarations for App framework using SDL.
 */

#include <SDL2/SDL.h>
#include <string>
#include <vector>

class AppBaseSDL {
  public:
    typedef Uint64 ticks_t;
    typedef std::vector<std::string> Args;
    AppBaseSDL(const char* const name) : szName(name), appTitle(name) { }
    virtual bool initialize(Args& args);
    virtual void finalize();
    // Ticks in milliseconds since start.
    virtual void drawFrame(uint32_t) { }
    // When used with SDL_SetEventWatch, return value is ignored. When used
    // with SDL_SetEventFilter, 1 causes event to be added to SDL's internal
    // event queue, 0 causes it to be dropped.
    virtual int doEvent(SDL_Event* event);
    virtual void onFPSUpdate();
    virtual SDL_Window* getMainWindow() { return pswMainWindow; }
    
    void drawFrame();
    void initializeFPSTimer();
    const char* name() { return szName; }
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





/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

#ifndef GL_APP_SDL_H_1456211188
#define GL_APP_SDL_H_1456211188

/* $Id: ac63511da134f2c25a9e1da86a36bc27b6198ae3 $ */

/*
 * Copyright 2015-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file GLAppSDL.h
 * @~English
 *
 * @brief Declaration of GLAppSDL base class for GL apps.
 */

#include "AppBaseSDL.h"


class GLAppSDL : public AppBaseSDL {
  public:
      GLAppSDL(const char* const name,
               int width, int height,
               const SDL_GLprofile profile,
               const int majorVersion,
               const int minorVersion)
            : AppBaseSDL(name),
              profile(profile),
              majorVersion(majorVersion), minorVersion(minorVersion)
    {
        appTitle = name;
        w_width = width;
        w_height = height;
    };
    virtual int doEvent(SDL_Event* event);
    virtual void drawFrame(uint32_t msTicks);
    virtual void finalize();
    virtual bool initialize(Args& args);
    virtual void onFPSUpdate();
    virtual void resizeWindow();
    virtual void windowResized();

  protected:
    SDL_GLContext sgcGLContext;

    int w_width;
    int w_height;

    const SDL_GLprofile profile;
    const int majorVersion;
    const int minorVersion;
};

#endif /* GL_APP_SDL_H_1456211188 */

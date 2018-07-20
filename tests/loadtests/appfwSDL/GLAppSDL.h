/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

#ifndef GL_APP_SDL_H_1456211188
#define GL_APP_SDL_H_1456211188

/* $Id: ac63511da134f2c25a9e1da86a36bc27b6198ae3 $ */

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
 * @file GLAppSDL.h
 * @~English
 *
 * @brief Declaration of GLAppSDL base class for GL apps.
 *
 * @author Mark Callow, www.edgewise-consulting.com
 * @copyright © 2015-2018, Mark Callow.
 */

#include "AppBaseSDL.h"


class GLAppSDL : public AppBaseSDL {
  public:
      GLAppSDL(const char* const name,
               int width, int height,
               const SDL_GLprofile profile,
               const int majorVersion,
               const int minorVersion)
            : profile(profile),
              majorVersion(majorVersion), minorVersion(minorVersion),
              AppBaseSDL(name)
    {
        appTitle = name;
        w_width = width;
        w_height = height;
    };
    virtual int doEvent(SDL_Event* event);
    virtual void drawFrame(uint32_t msTicks);
    virtual void finalize();
    virtual bool initialize(int argc, char* argv[]);
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

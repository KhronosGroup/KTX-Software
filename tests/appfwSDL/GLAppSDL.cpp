/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/* $Id: ac63511da134f2c25a9e1da86a36bc27b6198ae3 $ */

/**
 * @internal
 * @file GLAppSDLGL.cpp
 * @~English
 *
 * @brief GLAppSDL app class.
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
 * work of Mark Callow."
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

#if defined(_WIN32)
  #if _MSC_VER < 1900
    #define snprintf _snprintf
  #endif
  #define _CRT_SECURE_NO_WARNINGS
  #include "GL/glew.h"
  #include "SDL2/SDL_loadso.h"
#endif

#include <stdio.h>
#include <iomanip>
#include <sstream>

#include "GLAppSDL.h"

bool
GLAppSDL::initialize(int argc, char* argv[])
{
	if (!AppBaseSDL::initialize(argc, argv))
		return false;

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profile);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, majorVersion);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minorVersion);
#if defined(DEBUG)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

	if (profile == SDL_GL_CONTEXT_PROFILE_ES) {
#if 0
		int numVideoDrivers = SDL_GetNumVideoDrivers();
		int i;
		const char** drivers;

		drivers = (const char**)SDL_malloc(sizeof(const char*) * numVideoDrivers);
		for (i = 0; i < numVideoDrivers; i++) {
			drivers[i] = SDL_GetVideoDriver(i);
		}
#endif

		// Only the indicated platforms pay attention to these hints
		// but they could be set on any platform.
#if __WINDOWS__ || __LINUX__
		SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
#endif

#if __WINDOWS__
		// If using ANGLE copied from Chrome should set to "d3dcompiler_46.dll"
		// Should set value via compiler -D definition from gyp file.
		SDL_SetHint(SDL_HINT_VIDEO_WIN_D3DCOMPILER, "none");
#endif
    }

    
    pswMainWindow = SDL_CreateWindow(
                        szName,
                        SDL_WINDOWPOS_UNDEFINED,
                        SDL_WINDOWPOS_UNDEFINED,
                        w_width, w_height,
                        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
                    );

    if (pswMainWindow == NULL) {
        (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, szName, SDL_GetError(), NULL);
        return false;
    }


    sgcGLContext = SDL_GL_CreateContext(pswMainWindow);
	// Work around bug in SDL. It returns a 2.x context when 3.x is requested.
	// It does though internally record an error.
	const char* error = SDL_GetError();
    if (sgcGLContext == NULL
		|| (error[0] != '\0'
		    && majorVersion >= 3
	        && (profile == SDL_GL_CONTEXT_PROFILE_CORE
			    || profile == SDL_GL_CONTEXT_PROFILE_COMPATIBILITY))
		) {
        (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, szName, SDL_GetError(), NULL);
        return false;
    }

#if __WINDOWS__
	if (profile != SDL_GL_CONTEXT_PROFILE_ES)
    {
        // No choice but to use GLEW for GL on Windows; there is no .lib with static
        // bindings. For ES we use one of the hardware vendor SDKs all of which have
        // static bindings.
		// TODO: Figure out how to support {GLX,WGL}_EXT_create_context_es2_profile
        //       were there are no static bindings. Need a GLEW equivalent for ES and
        //       different compile options. Perhaps can borrow function loading stuff
        //       from SDL's testgles2.c.

		// So one build of this library can be linked in to applications using GLEW and
		// applications not using GLEW, do not call any GLEW functions directly.
		// Call via queried function pointers.
		void* glewdll = SDL_LoadObject("glew32.dll");
		if (glewdll == NULL) {
			std::string sName(szName);

			(void)SDL_ShowSimpleMessageBox(
				SDL_MESSAGEBOX_ERROR,
				szName,
				(sName + ": " + (const char*)SDL_GetError()).c_str(),
				NULL);
			return false;
		}
		
		typedef GLenum(GLEWAPIENTRY PFNGLEWINIT)(void);
		typedef const GLubyte * GLEWAPIENTRY PFNGLEWGETERRORSTRING(GLenum error);
		PFNGLEWINIT* pGlewInit;
		PFNGLEWGETERRORSTRING* pGlewGetErrorString;
		bool error = true;
		pGlewInit = (PFNGLEWINIT*)SDL_LoadFunction(glewdll, "glewInit");
		if (pGlewInit != NULL) {
			pGlewGetErrorString = (PFNGLEWGETERRORSTRING*)SDL_LoadFunction(glewdll, "glewGetErrorString");
			if (pGlewGetErrorString != NULL) {
				error = false;
			}
		}

		if (error) {
			std::string sName(szName);

			(void)SDL_ShowSimpleMessageBox(
				SDL_MESSAGEBOX_ERROR,
				szName,
				(sName + ": " + (const char*)SDL_GetError()).c_str(),
				NULL);
			return false;
		}

        int iResult = pGlewInit();
        if (iResult != GLEW_OK) {
			std::string sName(szName);

            (void)SDL_ShowSimpleMessageBox(
                          SDL_MESSAGEBOX_ERROR,
                          szName,
						  (sName + (const char*)pGlewGetErrorString(iResult)).c_str(),
                          NULL);
            return false;
        }
    }
#endif

    // Not getting an initial resize event, at least on Mac OS X.
    // Therefore call resize directly.
    
    resize(w_width, w_height);

    initializeFPSTimer();
    return true;
}


void
GLAppSDL::finalize()
{
	SDL_GL_DeleteContext(sgcGLContext);
}


int
GLAppSDL::doEvent(SDL_Event* event)
{
    switch (event->type) {
      case SDL_WINDOWEVENT:
        switch (event->window.event) {
          case SDL_WINDOWEVENT_SIZE_CHANGED:
            resize(event->window.data1, event->window.data2);
		    return 0;
        }
        break;
            
    }
    return AppBaseSDL::doEvent(event);
}


void
GLAppSDL::drawFrame(ticks_t ticks)
{
    AppBaseSDL::drawFrame(ticks);
    SDL_GL_SwapWindow(pswMainWindow);
}


void
GLAppSDL::resize(int width, int height)
{
}


void
GLAppSDL::onFPSUpdate()
{
    // Using onFPSUpdate avoids rewriting the title every frame.
    setWindowTitle("");
}


void
GLAppSDL::setWindowTitle(const char* const szExtra)
{
    std::stringstream ss;

    ss << std::fixed << std::setprecision(2)
       << lastFrameTime << "ms (" << fpsCounter.lastFPS << " fps)"
       << " - " << szName;

    if (szExtra != NULL && szExtra[0] != '\0') {
        ss << ": " << szExtra;
    }
    SDL_SetWindowTitle(pswMainWindow, ss.str().c_str());
}

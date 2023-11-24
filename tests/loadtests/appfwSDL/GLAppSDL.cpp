/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/* $Id: ac63511da134f2c25a9e1da86a36bc27b6198ae3 $ */

/*
 * Copyright 2015-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file GLAppSDLGL.cpp
 * @~English
 *
 * @brief GLAppSDL app class.
 */

#if defined(_WIN32)
  #if _MSC_VER < 1900
    #define snprintf _snprintf
  #endif
  #define _CRT_SECURE_NO_WARNINGS
#include "windows.h"
  #include "GL/glew.h"
  #include "SDL2/SDL_loadso.h"
#else
  #define GL_GLEXT_PROTOTYPES 1
  #include "GL/glcorearb.h"   // for glEnable and FRAMEBUFFER_RGB
#endif

#include <stdio.h>
#include <iomanip>
#include <sstream>

#include "GLAppSDL.h"

#if __WINDOWS__
void setWindowsIcon(SDL_Window *sdlWindow);
#endif

bool
GLAppSDL::initialize(Args& args)
{
    if (!AppBaseSDL::initialize(args))
        return false;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profile);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, majorVersion);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minorVersion);
#if !defined(EMSCRIPTEN)
    if (majorVersion >= 3)
      SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
#endif
#if defined(DEBUG) && !defined(EMSCRIPTEN)
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

#if __MACOSX__
    SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "1");
#endif
    
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

#if __WINDOWS__
    // Set the applications own icon in place of the Windows default set by SDL.
    // Needs to be done here to avoid change being visible.
    setWindowsIcon(pswMainWindow);
#endif

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
        void* glewdll;
#if defined(_DEBUG)
        glewdll = SDL_LoadObject("glew32d.dll");
        // KTX-Software repo only contains non-debug library for x64 hence this.
        if (glewdll == NULL) {
            glewdll = SDL_LoadObject("glew32.dll");
        }
#else
        glewdll = SDL_LoadObject("glew32.dll");
#endif
        if (glewdll == NULL) {
            std::string sName(szName);

            (void)SDL_ShowSimpleMessageBox(
                SDL_MESSAGEBOX_ERROR,
                szName,
                SDL_GetError(),
                NULL);
            return false;
        }
        
        typedef GLenum(GLEWAPIENTRY PFNGLEWINIT)(void);
        typedef const GLubyte * GLEWAPIENTRY PFNGLEWGETERRORSTRING(GLenum error);
        PFNGLEWINIT* pGlewInit;
        PFNGLEWGETERRORSTRING* pGlewGetErrorString = nullptr;
        bool loadError = true;
#define STR(s) #s
#if defined(_M_IX86)
        /* Win32 GLEW uses __stdcall. */
  #define DNAMESTR(x,n) STR(_##x##@##n)
#else
        /* x64 uses __cdecl. */
  #define DNAMESTR(x,n) STR(x)
#endif
        pGlewInit = (PFNGLEWINIT*)SDL_LoadFunction(glewdll, DNAMESTR(glewInit,0));
        if (pGlewInit != NULL) {
            pGlewGetErrorString = (PFNGLEWGETERRORSTRING*)SDL_LoadFunction(
                    glewdll, DNAMESTR(glewGetErrorString,4));
            if (pGlewGetErrorString != NULL) {
                loadError = false;
            }
        }

        if (loadError) {
            std::string sName(szName);

            (void)SDL_ShowSimpleMessageBox(
                SDL_MESSAGEBOX_ERROR,
                szName,
                SDL_GetError(),
                NULL);
            return false;
        }
        int iResult = pGlewInit();
        if (iResult != GLEW_OK) {
            std::string sName(szName);

            (void)SDL_ShowSimpleMessageBox(
                          SDL_MESSAGEBOX_ERROR,
                          szName,
                          (const char*)pGlewGetErrorString(iResult),
                          NULL);
            return false;
        }
    }
#endif

    int srgb;
    SDL_GL_GetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, &srgb);
    if (srgb && profile != SDL_GL_CONTEXT_PROFILE_ES)
        glEnable(GL_FRAMEBUFFER_SRGB);

    // In case the window is created with a different size than specified.
    resizeWindow();

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
            // Size given in event is in 'points' on some platforms.
            // Resize window will figure out the drawable pixel size.
            resizeWindow(/*event->window.data1, event->window.data2*/);
            return 0;
        }
        break;
            
    }
    return AppBaseSDL::doEvent(event);
}


void
GLAppSDL::drawFrame(uint32_t /*msTicks*/)
{
    SDL_GL_SwapWindow(pswMainWindow);
}


void
GLAppSDL::windowResized()
{
    // Derived class can override as necessary.
}


void
GLAppSDL::resizeWindow()
{
    SDL_GL_GetDrawableSize(pswMainWindow, &w_width, &w_height);
    windowResized();
}


void
GLAppSDL::onFPSUpdate()
{
    // Using onFPSUpdate avoids rewriting the title every frame.
    setWindowTitle();
}

#if 0
void
GLAppSDL::setAppTitle(const char* const szExtra)
{
    appTitle = name();
    if (szExtra != NULL && szExtra[0] != '\0') {
        appTitle += ": ";
        appTitle += szExtra;
    }
    setWindowTitle();
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

void
GLAppSDL::setWindowTitle()
{
    SDL_SetWindowTitle(pswMainWindow, appTitle.c_str());
}
#endif

#if __WINDOWS__
// Windows specific code to use icon in module
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <SDL2/SDL_syswm.h>
void
setWindowsIcon(SDL_Window *sdlWindow) {
    HINSTANCE handle = ::GetModuleHandle(nullptr);
    // Identify icon by name rather than IDI_ macro to avoid having to
    // include application's resource.h.
    HICON icon = ::LoadIcon(handle, "MAIN_ICON");// MAKEINTRESOURCE(IDI_ICON1));
    if (icon != nullptr){
        SDL_SysWMinfo wminfo;
        SDL_VERSION(&wminfo.version);
        if (SDL_GetWindowWMInfo(sdlWindow, &wminfo) == 1){
            HWND hwnd = wminfo.info.win.window;
            ::SetClassLongPtr(hwnd, GCLP_HICON, reinterpret_cast<LONG_PTR>(icon));
        }
    }
}
#endif


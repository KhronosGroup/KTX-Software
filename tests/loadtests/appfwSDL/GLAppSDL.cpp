/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/* $Id: ac63511da134f2c25a9e1da86a36bc27b6198ae3 $ */

/*
 * Copyright 2015-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file
 * @~English
 *
 * @brief GLAppSDL app class.
 */

#if defined(_WIN32)
  #if _MSC_VER < 1900
    #define snprintf _snprintf
  #endif
  #define _CRT_SECURE_NO_WARNINGS
  #define WIN32_LEAN_AND_MEAN
  #include "windows.h"
  #include "GL/glew.h"
  #include "SDL3/SDL_loadso.h"
#else
  #define GL_GLEXT_PROTOTYPES 1
  #include "GL/glcorearb.h"   // for glEnable and FRAMEBUFFER_RGB
#endif

#include <stdio.h>
#include <iomanip>
#include <sstream>

#include "GLAppSDL.h"

#if SDL_PLATFORM_WINDOWS
void setWindowsIcon(SDL_Window *sdlWindow);
#endif

#define SUPPORT_HDR 0
// GLAppSDL HDR support is disabled because:
// 1. Apple's OpenGL does not expose the extensions for the compressed HDR formats, ASTC HDR
//    and BC6H, even though the hardware supports them; thus HDR display is not very useful
//    on those platforms;
// 2. on Apple OSes, it is not possible to display HDR from the window created by SDL for OpenGL;
//    to do so requires a window backed by a CAMetalLayer with its boolean field
//    wantsExtendedDynamicRangeContent set to true; the SDL window has no CAMetalLayer;
// 3. because of no. 1, investigating alternative ways of creating the window is not worth
//    the time;
// 4. when the compositor on Apple OSes displays the content of the GL_LINEAR, RGB16F backbuffer
//    to the window created by SDL, it copies the data to the display which then decodes it as if
//    it was sRGB data; maybe the compositor would handle it differently for an HDR-enabled
//    window;
// 5. there is no portable way in SDL to determine if a window is HDR enabled, or not, so no way
//    to distinguish between a GL_LINEAR renderbuffer being displayed as linear or being
//    displayed as sRGB, as in no. 4, meaning correct color cannot be guaranteed;
// 6. the author does not have suitable hardware to test enabling HDR on Android, GNU/Linux
//    and Windows or whether it can be displayed from an SDL created window.
//
// The disabled code has been left in place to provide a starting point for anyone who wants
// to try on Android, GNU/Linux or Windows.

bool
GLAppSDL::initialize(Args& args)
{
#if SUPPORT_HDR
    const char* use_hdr_surface = SDL_GetEnvironmentVariable(SDL_GetEnvironment(),
                                                         "KTX_GL_LT_USE_HDR_SURFACE");
    if (use_hdr_surface != nullptr && !SDL_strncasecmp(use_hdr_surface, "YES", 3))
        hdr = true;

    for (uint32_t i = 1; i < args.size(); i++) {
        if (args[i].compare("--hdr") == 0) {
            hdr = true;
            args.erase(args.begin() + i--);
            continue;
        }
    }
#endif

    if (!AppBaseSDL::initialize(args))
        return false;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profile);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, majorVersion);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minorVersion);
    // On SDL3 this defaults to 8. On SDL2 it was 0.
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    if (majorVersion >= 3)
        SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
#if SUPPORT_HDR
    if (hdr) {
#if SDL_PLATFORM_APPLE || SDL_PLATFORM_EMSCRIPTEN
        //   Emscripten's WebGL-based OpenGL {,ES} does not support float framebuffers or
        // HDR display.
        //   HDR display from SDL OpenGL windows on Apple platforms is not supported for
        // the reasons described in the comment where SUPPORT_HDR is defined.
        std::stringstream message;
        message << "Ignoring --hdr. HDR display is not supported on "
                << (SDL_PLATFORM_APPLE ? "SDL with Apple" : "Emscripten")
                << " OpenGL or OpenGL ES implementations.";
        (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, szName,
                                       message.str().c_str(), NULL);
#else
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 16);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 16);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 16);
        SDL_GL_SetAttribute(SDL_GL_FLOATBUFFERS, 1);
#endif
    }
#endif
#if defined(DEBUG) && !defined(__EMSCRIPTEN__)
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
#if SDL_PLATFORM_WINDOWS || SDL_PLATFORM_LINUX
        SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
#endif

#if SDL_PLATFORM_WINDOWS
        // If using ANGLE copied from Chrome should set to "d3dcompiler_46.dll"
        // Should set value via compiler -D definition from gyp file.
        SDL_SetHint(SDL_HINT_VIDEO_WIN_D3DCOMPILER, "none");
#endif
    }

    SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");
    // CAUTION: Setting this to 0 (the default) on macOS causes loss of all touch events
    // from a trackpad not just those corresponding to mouse clicks.
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "1");

#if 0
    const char* mt = SDL_GetHint(SDL_HINT_MOUSE_TOUCH_EVENTS);
    SDL_Log("MOUSE_TOUCH_EVENTS = %s", mt);
    const char* tm = SDL_GetHint(SDL_HINT_TOUCH_MOUSE_EVENTS);
    SDL_Log("TOUCH_MOUSE_EVENTS = %s", tm);
    const char* tto = SDL_GetHint(SDL_HINT_TRACKPAD_IS_TOUCH_ONLY);
    SDL_Log("TRACKPAD_IS_TOUCH_ONLY = %s", tto);
#endif

    pswMainWindow = SDL_CreateWindow(
                        szName,
                        w_width, w_height,
                        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
                    );

    if (pswMainWindow == NULL) {
        (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, szName, SDL_GetError(), NULL);
        return false;
    }
#if SDL_PLATFORM_WINDOWS
    // Set the application's own icon in place of the Windows default set by SDL.
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

#if SDL_PLATFORM_WINDOWS
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
        SDL_SharedObject* glewdll;
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
            (void)SDL_ShowSimpleMessageBox(
                SDL_MESSAGEBOX_ERROR,
                szName,
                SDL_GetError(),
                NULL);
            return false;
        }
        int iResult = pGlewInit();
        if (iResult != GLEW_OK) {
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
    int actualWidth, actualHeight;
    SDL_GetWindowSizeInPixels(pswMainWindow, &actualWidth, &actualHeight);
    resizeWindow(actualWidth, actualHeight);

    initializeFPSTimer();
    return true;
}


void
GLAppSDL::finalize()
{
    SDL_GL_DestroyContext(sgcGLContext);
}


bool
GLAppSDL::doEvent(SDL_Event* event)
{
    switch (event->type) {
      case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        resizeWindow(event->window.data1, event->window.data2);
        return 0;
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
GLAppSDL::resizeWindow(int width, int height)
{
    w_width = width;
    w_height = height;
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

#if SDL_PLATFORM_WINDOWS
// Windows specific code to use icon in module
void
setWindowsIcon(SDL_Window *sdlWindow) {
    HINSTANCE handle = ::GetModuleHandle(nullptr);
    // Identify icon by name rather than IDI_ macro to avoid having to
    // include application's resource.h.
    HICON icon = ::LoadIcon(handle, "MAIN_ICON");// MAKEINTRESOURCE(IDI_ICON1));
    if (icon != nullptr){
        HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(sdlWindow), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
        if (hwnd) {
            ::SetClassLongPtr(hwnd, GCLP_HICON, reinterpret_cast<LONG_PTR>(icon));
        }
    }
}
#endif

//    }


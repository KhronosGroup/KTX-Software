/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/* $Id: f63e0a9e6eed51ed84a8eea1eff0708c8a6af22b $ */

/**
 * @internal
 * @file main.c
 * @~English
 *
 * @brief main() function for SDL app framework.
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

#include <stdio.h>
#include "AppBaseSDL.h"
#if defined(EMSCRIPTEN)
#include <emscripten.h>
#endif


#if defined(__IPHONEOS__)
  #define NEED_MAIN_LOOP 0
  //int SDL_iPhoneSetAnimationCallback(SDL_Window * window, int interval, void (*callback)(void*), void *callbackParam);
  #define setAnimationCallback(win, cb, userdata) \
    SDL_iPhoneSetAnimationCallback(win, 1, cb, userdata)
#elif defined(EMSCRIPTEN)
  #define NEED_MAIN_LOOP 0
  //void emscripten_set_main_loop_arg(em_arg_callback_func func, void *arg, int fps, int simulate_infinite_loop)
  #define setAnimationCallback(win, cb, userdata) \
    emscripten_set_main_loop_arg(cb, userdata, 0, EM_FALSE)
#else
  #define NEED_MAIN_LOOP 1
  #define setAnimationCallback(win, cb, userdata)
#endif


int
main(int argc, char* argv[])
{
  if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
      fprintf(stderr, "%s: SDL video initialization failed: %s\n",
              theApp->name(), SDL_GetError());
      return 1;
  }

  if (!theApp->initialize(argc, argv))
    return 1;

  // SDL_SetEventFilter(theApp->Event, theApp);  // Catches events before they are added to the event queue
  SDL_AddEventWatch(theApp->onEvent, theApp);  // Triggered when event added to queue. Will this work on iOS?

  if (!NEED_MAIN_LOOP) {
    // TODO: Fix this main to work for multiple windows. One way is to have the application
    // call setAnimationCallback and keep a list of the windows in this file, calling update for each window.
    setAnimationCallback(SDL_GL_GetCurrentWindow(), theApp->onDrawFrame, theApp);
    // iOS version of SDL will not exit when main completes.
    // The Emscripten version of the app must be compiled with -s NO_EXIT_RUNTIME=1 to prevent Emscripten
    // exiting when main completes.
    return 0;
  } else {
    for (;;) {
      SDL_PumpEvents();
      theApp->drawFrame(SDL_GetPerformanceCounter());
      // Let app return a sleep time from drawFrame()? If so
      // sleep(sleeptime);
    }
  }
}

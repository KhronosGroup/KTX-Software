/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/* $Id: f63e0a9e6eed51ed84a8eea1eff0708c8a6af22b $ */

/*
 * Copyright 2015-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file main.c
 * @~English
 *
 * @brief main() function for SDL app framework.
 */

#include <string>
#include <vector>
#include <stdio.h>
#include "AppBaseSDL.h"
#include "platform_utils.h"
#if defined(EMSCRIPTEN)
#include <emscripten.h>
#endif


#if defined(__IPHONEOS__)
  #define NEED_MAIN_LOOP 0
  //int SDL_iPhoneSetAnimationCallback(
  //                             SDL_Window * window, int interval,
  //                             void (*callback)(void*), void *callbackParam
  //                                  );
  #define setAnimationCallback(win, cb, userdata) \
    SDL_iPhoneSetAnimationCallback(win, 1, cb, userdata)
#elif defined(EMSCRIPTEN)
  #define NEED_MAIN_LOOP 0
  //void emscripten_set_main_loop_arg(em_arg_callback_func func, void *arg,
  //                                  int fps, int simulate_infinite_loop);
  #define setAnimationCallback(win, cb, userdata) \
    emscripten_set_main_loop_arg(cb, userdata, 0, 0)
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
  atexit(SDL_Quit);

  InitUTF8CLI(argc, argv);
  AppBaseSDL::Args args(argv, argv+argc);

  if (!theApp->initialize(args))
    return 1;

  // Catches events before they are added to the event queue.
  // May need this for some events that need rapid response...
  // SDL_SetEventFilter(theApp->onEvent, theApp);
  // Triggered when event added to queue.
  SDL_AddEventWatch(theApp->onEvent, theApp);
  if (!NEED_MAIN_LOOP) {
    // TODO: Fix this main to work for multiple windows. One way is to have the
    // application call setAnimationCallback and keep a list of the windows in
    // this file, calling drawFrame for each window.
    setAnimationCallback(theApp->getMainWindow(), theApp->onDrawFrame, theApp);
    // iOS version of SDL will not exit when main completes.
    // The Emscripten version of the app must be compiled with
    // -s NO_EXIT_RUNTIME=1 to prevent Emscripten exiting when main completes.
    return 0;
  } else {
    for (;;) {
      SDL_PumpEvents();
      theApp->drawFrame();
      // XXX Let app return a sleeptime from drawFrame()? If so
      // sleep(sleeptime);
    }
  }
}

/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/* $Id: f63e0a9e6eed51ed84a8eea1eff0708c8a6af22b $ */

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
 * @file main.c
 * @~English
 *
 * @brief main() function for SDL app framework.
 *
 * @author Mark Callow, www.edgewise-consulting.com
 * @copyright © 2015-2018, Mark Callow.
 */

#include <stdio.h>
#include "AppBaseSDL.h"
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

/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow, <khronos at callow dot im>.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SWIPE_DETECTOR_H
#define _SWIPE_DETECTOR_H

#include <SDL2/SDL_events.h>

class SwipeDetector {
  public:
    enum result {
        eEventNotConsumed,
        eEventConsumed,
        eSwipeUp,
        eSwipeDown,
        eSwipeLeft,
        eSwipeRight
    };

    SwipeDetector() : mgestureFirstSaved(false), mgestureSwipe(false) { }

    result doEvent(SDL_Event* event);

  protected:
    SDL_MultiGestureEvent mgestureFirst;
    bool mgestureFirstSaved;
    bool mgestureSwipe;
};

#endif /* _SWIPE_DETECTOR_H */

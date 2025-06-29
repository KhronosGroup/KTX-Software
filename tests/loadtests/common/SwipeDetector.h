/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow, <khronos at callow dot im>.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SWIPE_DETECTOR_H
#define _SWIPE_DETECTOR_H

#include <SDL3/SDL.h>
#include "SDL_gesture.h"

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
    Gesture_MultiGestureEvent mgestureFirst;
    bool mgestureFirstSaved;
    bool mgestureSwipe;
};

#endif /* _SWIPE_DETECTOR_H */

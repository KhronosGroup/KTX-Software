/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow, <khronos at callow dot im>.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SWIPE_DETECTOR_H
#define _SWIPE_DETECTOR_H
#include <optional>
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

    struct vector {
        float w;
        float h;

        vector() : w(0.0), h(0.0) { }
        vector(float _w, float _h) : w(_w), h(_h) { }
        double getAngle();
        double getAngle(const vector& v2);
        double getNormalizedAngle();
        float length() { return SDL_sqrt(w * w + h * h); }
    };

    SwipeDetector() : gestureSwipe(false) { }
    result doEvent(SDL_Event* event);

  protected:
    //Gesture_MultiGestureEvent mgestureFirst;
    //bool mgestureFirstSaved;
    struct gestureStart {
        Uint64 time;
        SDL_FPoint point;
        gestureStart() { time = 0; point.x = point.y = 0.0; }
    } gestureStart;
    std::optional<vector> lastVector;
    bool gestureSwipe;
};

#endif /* _SWIPE_DETECTOR_H */

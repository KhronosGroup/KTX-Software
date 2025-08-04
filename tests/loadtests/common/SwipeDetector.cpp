/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @class SwipeDetector
 * @~English
 *
 * @brief Definition of a class for detecting swipes.
 *
 * @author Mark Callow, www.edgewise-consulting.com.
 */

#if defined(_WIN32)
  #define _USE_MATH_DEFINES
#endif
#include <assert.h>
#include <cmath>
#include <SDL3/SDL_log.h>
#include "SwipeDetector.h"

#if !defined(SWIPEDETECTOR_LOG_GESTURE_EVENTS)
  #define SWIPEDETECTOR_LOG_GESTURE_EVENTS 1
#endif
#if !defined(SWIPEDETECTOR_LOG_GESTURE_DETECTION)
  #define SWIPEDETECTOR_LOG_GESTURE_DETECTION 1
#endif

bool
SwipeDetector::doEvent(SDL_Event* event)
{
    bool result = false;

    switch (event->type) {
      case SDL_EVENT_FINGER_UP: {
        int numFingers;
        SDL_Finger** fingers = SDL_GetTouchFingers(event->tfinger.touchID, &numFingers);
        if (SWIPEDETECTOR_LOG_GESTURE_EVENTS) {
            SDL_Log("SD: Finger: %" SDL_PRIx64 " UP - fingers: %i, x: %f, y: %f",
                    event->tfinger.fingerID, numFingers, event->tfinger.x, event->tfinger.y);
        }
        // SDL_GetTouchFingers appears to return the number of fingers
        // down *before* the event was generated, so 1 means the last finger
        // just lifted.
        if (numFingers == 1 && gestureStart.time != 0.0) {
            gestureStart.time = 0.0;
            if (SWIPEDETECTOR_LOG_GESTURE_DETECTION) {
                SDL_Log("***************** SD: FINGER_UP, MULTIGESTURE DONE *****************");
            }
        } else {
            result = true;
        }
        SDL_free(fingers);
        break;
      }
      case GESTURE_MULTIGESTURE: {
        Gesture_MultiGestureEvent& mgesture = *(Gesture_MultiGestureEvent *)event;
        if (SWIPEDETECTOR_LOG_GESTURE_EVENTS) {
            SDL_Log("SD: MG Event: x = %f, y = %f, dAng = %f (%f), dR = %f, numFingers = %i, time = %" SDL_PRIu64,
               mgesture.x,
               mgesture.y,
               mgesture.dTheta * 180.0 / M_PI,
               mgesture.dTheta,
               mgesture.dDist,
               mgesture.numFingers,
               mgesture.timestamp);
        }
        if (SWIPEDETECTOR_LOG_GESTURE_DETECTION) {
            SDL_Log("SD: mgestureSwipe = %i, time = %" SDL_PRIu64,
                     gestureSwipe,
                     (mgesture.timestamp - gestureStart.time) / 1000000);
        }
        if (gestureStart.time == 0.0) {
            if (SWIPEDETECTOR_LOG_GESTURE_DETECTION) {
                SDL_Log("************ SD: MULTIGESTURE DETECTION START **************");
            }
            gestureStart.time = mgesture.timestamp;
            gestureStart.point.x = mgesture.x;
            gestureStart.point.y = mgesture.y;
            lastVector.reset();
            gestureSwipe = false;
        } else {
            if (!gestureSwipe) {
                vector sv; // Vector from start point to current position
                float velocity;
                float theta; // Angle between current vector and previous vector.
                float duration;
                sv.w = mgesture.x - gestureStart.point.x;
                sv.h = mgesture.y - gestureStart.point.y;
                float distance = sv.length();
                if (lastVector.has_value()) {
                    // SDL2 timestamps were in milliseconds, SDL3 are nanoseconds. Given the
                    // normalized distances reported, using nanoseconds leads to 0 velocitySq.
                    duration = (mgesture.timestamp - gestureStart.time) / 1000000.0;

                    velocity = distance / duration;
                    assert(!std::isinf(velocity));
                    theta = lastVector->getAngle(sv);
                    lastVector = sv;
                    if (SWIPEDETECTOR_LOG_GESTURE_DETECTION) {
                        SDL_Log("SD: Detection: distance = %f, velocity = %f, theta = %f, sv angle = %f, lastv angle = %f",
                                distance, velocity, theta,
                                sv.getAngle(),
                                lastVector->getAngle());
                    }
                    // Multiple events with the same timestamp is a possibility
                    // hence the isinf() check.
                    if (std::abs(theta) < 3.0 && std::abs(mgesture.dDist) > 0.01 && !std::isinf(velocity) && velocity > 0.001) { // 0.0004
                        if (SWIPEDETECTOR_LOG_GESTURE_DETECTION)
                            SDL_Log("----------------- SD: Swipe detected -----------------");
                        gestureSwipe = true;
                        if (SDL_EventEnabled(SDL_EVENT_USER)) {
                            SDL_Event user_event;
                            // SDL will copy this entire struct! Initialize to keep memory
                            // checkers happy.
                            SDL_zero(user_event);
                            user_event.type = SDL_EVENT_USER;
                            user_event.user.code = swipeGesture;
                            user_event.user.data1 = reinterpret_cast<void*>(sv.getDirection());
                            user_event.user.data2 = NULL;
                            SDL_PushEvent(&user_event);
                        }
                    } else {
                        if (SWIPEDETECTOR_LOG_GESTURE_DETECTION) SDL_Log("SD: No swipe detected.");
                        result = true;
                    }
                } else {
                     lastVector = sv;
                     result = true;
                }
            }
        }
        break;
      }
      default:
          result = true;
    }

    return result;
}

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
#include <cmath>
#include <SDL3/SDL_log.h>
#include "SwipeDetector.h"

namespace Swipe {
    enum Direction {
        up = SwipeDetector::eSwipeUp,
        down = SwipeDetector::eSwipeDown,
        left = SwipeDetector::eSwipeLeft,
        right = SwipeDetector::eSwipeRight
    };

    /**
     * @~English
     * @brief Find the angle between two points in a plane.
     *
     * The angle is measured with 0/360 being the X-axis to the right, angles
     * increase counter clockwise.
     *
     * @param x1 the x position of the first point
     * @param y1 the y position of the first point
     * @param x2 the x position of the second point
     * @param y2 the y position of the second point
     *
     * @return the angle between two points
     */
    double getAngle(float x1, float y1, float x2, float y2) {
        double rad = atan2(y1-y2,x2-x1) + M_PI;
        return fmod(rad*180/M_PI + 180, 360);
    }

    /**
     * @~English
     * @brief Check if angle falls within an interval.
     *
     * @param angle an angle
     * @param init the initial bound
     * @param end the final bound
     *
     * @return true if the given angle is in the interval [init, end), false
     *         otherwise.
     */
    static bool inRange(double angle, float init, float end){
        return (angle >= init) && (angle < end);
    }

    /**
     * @~English
     * @brief Return a direction given an angle.
     *
     * Directions are defined as follows:
     *
     * Up: [45, 135]
     * Right: [0,45] and [315, 360]
     * Down: [225, 315]
     * Left: [135, 225]
     *
     * @param angle an angle from 0 to 360 - e
     * @return the direction of an angle
     */
    static Direction getDirection(double angle){
        if (inRange(angle, 45, 135)) {
            return Direction::up;
        } else if (inRange(angle, 0,45) || inRange(angle, 315, 360)) {
            return Direction::right;
        } else if (inRange(angle, 225, 315)) {
            return Direction::down;
        } else {
           return Direction::left;
       }
    }

    /**
     * Given two points in the plane p1=(x1, x2) and p2=(y1, y1), this method
     * returns the direction that an arrow pointing from p1 to p2 would have.
     *
     * @param x1 the x position of the first point
     * @param y1 the y position of the first point
     * @param x2 the x position of the second point
     * @param y2 the y position of the second point
     * @return the direction
     */
    Direction getDirection(float x1, float y1, float x2, float y2) {
        double angle = getAngle(x1, y1, x2, y2);
        return getDirection(angle);
    }
}

#if !defined(SWIPEDETECTOR_LOG_GESTURE_EVENTS)
  #define SWIPEDETECTOR_LOG_GESTURE_EVENTS 0
#endif
#if !defined(SWIPEDETECTOR_LOG_GESTURE_DETECTION)
  #define SWIPEDETECTOR_LOG_GESTURE_DETECTION 0
#endif

SwipeDetector::result
SwipeDetector::doEvent(SDL_Event* event)
{
    result result = eEventConsumed;

    switch (event->type) {
      case SDL_EVENT_FINGER_UP: {
        // SDL_GetNumTouchFingers appears to return the number of fingers
        // down *before* the event was generated, so 1 means the last finger
        // just lifted.
        int numFingers;
        SDL_Finger** fingers = SDL_GetTouchFingers(event->tfinger.touchID, &numFingers);
        if (SWIPEDETECTOR_LOG_GESTURE_EVENTS) {
            SDL_Log("SD: Finger: %" SDL_PRIs64 " up - fingers: %i, x: %f, y: %f",
                    event->tfinger.fingerID, numFingers, event->tfinger.x, event->tfinger.y);
        }
        if (numFingers == 1 && mgestureFirstSaved) {
            mgestureFirstSaved = false;
            if (SWIPEDETECTOR_LOG_GESTURE_DETECTION) {
                SDL_Log("***************** SD: FINGER_UP, MULTIGESTURE DONE *****************");
            }
        } else {
            result = eEventNotConsumed;
        }
        SDL_free(fingers);
        break;
      }
      case GESTURE_MULTIGESTURE: {
        Gesture_MultiGestureEvent& mgesture = *(Gesture_MultiGestureEvent *)event;
        if (SWIPEDETECTOR_LOG_GESTURE_EVENTS) {
            SDL_Log("SD: MG: x = %f, y = %f, dAng = %f (%f), dR = %f, numFingers = %i, time = %" SDL_PRIu64,
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
                     mgestureSwipe,
                     (mgesture.timestamp - mgestureFirst.timestamp) / 1000000);
        }
        if (!mgestureFirstSaved) {
            if (SWIPEDETECTOR_LOG_GESTURE_DETECTION) {
                SDL_Log("***************** SD: MULTIGESTURE START *******************");
            }
            mgestureFirst = mgesture;
            mgestureFirstSaved = true;
            mgestureSwipe = false;
        } else {
            if (!mgestureSwipe) {
                float dx, dy, distanceSq; double velocitySq;
                Uint64 duration;
                dx = mgesture.x - mgestureFirst.x;
                dy = mgesture.y - mgestureFirst.y;
                distanceSq = dx * dx + dy * dy;
                // SDL2 timestamps were in milliseconds, SDL3 are nanoseconds. Given the
                // normalized distances reported, using nanoseconds leads to 0 velocitySq.
                duration = (mgesture.timestamp - mgestureFirst.timestamp) / 1000000;
                velocitySq = distanceSq / duration;
                if (SWIPEDETECTOR_LOG_GESTURE_DETECTION) {
                    SDL_Log("SD: MG: dx = %f, dy = %f, distanceSq = %f, velocitySq = %f",
                            dx, dy, distanceSq, velocitySq);
                }
                // Multiple events with the same timestamp is a possibility
                // hence the isinf() check.
                if (!isinf(velocitySq) && velocitySq > 0.0002) { // 0.08
                    if (SWIPEDETECTOR_LOG_GESTURE_DETECTION)
                        SDL_Log("----------------- SD: Swipe detected -----------------");
                    mgestureSwipe = true;
                    Swipe::Direction direction = Swipe::getDirection(
                                                            mgestureFirst.x,
                                                            mgestureFirst.y,
                                                            mgesture.x,
                                                            mgesture.y);
                    return static_cast<enum result>(direction);
                } else {
                    if (SWIPEDETECTOR_LOG_GESTURE_DETECTION) SDL_Log("SD: No swipe detected.");
                    result = eEventNotConsumed;
                }
            }
        }
        break;
      }
      default:
          result = eEventNotConsumed;
    }

    return result;
}

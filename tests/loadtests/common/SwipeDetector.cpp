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

#include <math.h>
#include "SDL2/SDL_log.h"
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

#if !defined(LOG_GESTURE_EVENTS)
  #define LOG_GESTURE_EVENTS 0
#endif
#if !defined(LOG_GESTURE_DETECTION)
  #define LOG_GESTURE_DETECTION 0
#endif

SwipeDetector::result
SwipeDetector::doEvent(SDL_Event* event)
{
    result result = eEventConsumed;

    switch (event->type) {
      case SDL_FINGERUP:
#if LOG_GESTURE_EVENTS
        SDL_Log("Finger: %" SDL_PRIs64 " up - x: %f, y: %f",
               event->tfinger.fingerId,event->tfinger.x,event->tfinger.y);
#endif
#if LOG_GESTURE_DETECTION
        SDL_Log("----------------------- FINGERUP ---------------------------");
#endif

        // SDL_GetNumTouchFingers appears to return the number of fingers
        // down *before* the event was generated, so 1 means the last finger
        // just lifted.
        if (SDL_GetNumTouchFingers(event->tfinger.touchId) == 1) {
            mgestureFirstSaved = false;
        }
        break;
      case SDL_MULTIGESTURE:
#if LOG_GESTURE_EVENTS
        SDL_Log("MG: x = %f, y = %f, dAng = %f (%f), dR = %f, numFingers = %i",
           event->mgesture.x,
           event->mgesture.y,
           event->mgesture.dTheta * 180.0 / M_PI,
           event->mgesture.dTheta,
           event->mgesture.dDist,
           event->mgesture.numFingers);
#endif
#if LOG_GESTURE_DETECTION
        SDL_Log("mgestureSwipe = %i, time = %i",
                 mgestureSwipe,
                 event->mgesture.timestamp - mgestureFirst.timestamp);
#endif
        if (!mgestureFirstSaved) {
#if LOG_GESTURE_DETECTION
            SDL_Log("***************** MULTIGESTURE START *******************");
#endif
            mgestureFirst = event->mgesture;
            mgestureFirstSaved = true;
            mgestureSwipe = false;
        } else {
            if (!mgestureSwipe) {
                float dx, dy, distanceSq, velocitySq;
                uint32_t duration;
                dx = event->mgesture.x - mgestureFirst.x;
                dy = event->mgesture.y - mgestureFirst.y;
                distanceSq = dx * dx + dy * dy;
                duration = (event->mgesture.timestamp - mgestureFirst.timestamp);
                velocitySq = distanceSq / duration;
#if LOG_GESTURE_DETECTION
                SDL_Log("MG: distanceSq = %f, velocitySq = %f",
                        distanceSq, velocitySq);
#endif
                // Multiple events with the same timestamp is a possibility
                // hence the isinf() check.
                if (!isinf(velocitySq) && velocitySq > 0.0002) { // 0.08
#if LOG_GESTURE_DETECTION
                    SDL_Log("Swipe detected.");
#endif
                    mgestureSwipe = true;
                    Swipe::Direction direction = Swipe::getDirection(
                                                            mgestureFirst.x,
                                                            mgestureFirst.y,
                                                            event->mgesture.x,
                                                            event->mgesture.y);
                    return static_cast<enum result>(direction);
                } else
                    result = eEventNotConsumed;
            }
        }
        break;
      default:
          result = eEventNotConsumed;
    }

    return result;
}

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
     * @return the angle between the vector through two points and the x axis.
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

/**
 * @~English
 * @internal
 * @brief Find the angle between the vector and the X-axis
 *
 * Positive angles increase counter-clockwise from the X-axis
 * which has +x to the right.
 *
 * @return the angle between the vector and the x axis in degrees.
 */
double SwipeDetector::vector::getAngle() {
    double rad = atan2(h, w);
    return rad * 180/M_PI;
}

/**
 * @~English
 * @internal
 * @brief Find the angle between the vector and the X-axis
 *
 * Positive angles increase counter-clockwise from the X-axis
 * which has +x to the right. Value is normalized to the range 0 to 360.
 *
 * @return the angle between the vector and the x axis in degrees.
 */
double SwipeDetector::vector::getNormalizedAngle() {
    double rad = atan2(h, w) + M_PI;
    return fmod(rad*180/M_PI + 180, 360);
}

/**
 * @~English
 * @internal
 * @brief Find the angle between this vector and another.
 *
 * Positive angles increase counter-clockwise.
 *
 * @return the angle between the 2 vectors in degrees.
 */
double SwipeDetector::vector::getAngle(const vector& v2) {
    double rad = atan2f(w * v2.h - h * v2.w, w * v2.w + h * v2.h);
    return rad * 180/M_PI;
}

#if !defined(SWIPEDETECTOR_LOG_GESTURE_EVENTS)
  #define SWIPEDETECTOR_LOG_GESTURE_EVENTS 1
#endif
#if !defined(SWIPEDETECTOR_LOG_GESTURE_DETECTION)
  #define SWIPEDETECTOR_LOG_GESTURE_DETECTION 1
#endif

SwipeDetector::result
SwipeDetector::doEvent(SDL_Event* event)
{
    result result = eEventConsumed;

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
            result = eEventNotConsumed;
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
                double velocity;
                //float velocity;
                // mgesture.dTheta is the angle between the vector from x,y in the finger
                // motion of the last event to the centroid x,y and the same vector in
                // the current event. Useful?
                //float theta = mgesture.dTheta * 180.0 / M_PI;
                float theta; // Angle between previous vector current vector.
                float theta2;
                float duration;
                sv.w = mgesture.x - gestureStart.point.x;
                sv.h = mgesture.y - gestureStart.point.y;
                float distance = sv.length();
                if (lastVector.has_value()) {
                    // SDL2 timestamps were in milliseconds, SDL3 are nanoseconds. Given the
                    // normalized distances reported, using nanoseconds leads to 0 velocitySq.
                    duration = (mgesture.timestamp - gestureStart.time) / 1000000.0;

                    //velocitySq = distanceSq / duration;
                    velocity = distance / duration;
                    assert(!std::isinf(velocity));
                    // Normalize vector to simplify angle calculations.
                    sv.w /= distance;
                    sv.h /= distance;
                    //theta = SDL_atan2f(lastVector->w * sv.h - lastVector->h * sv.w, lastVector->w * sv.w + lastVector->h * sv.h);
                    //theta = theta * 180.0 / M_PI;
                    theta = lastVector->getAngle(sv);
                    theta2 = (SDL_atan2(sv.h, sv.w) - SDL_atan2(lastVector->h, lastVector->w)) * 180 / M_PI;
                    lastVector = sv;
                    if (SWIPEDETECTOR_LOG_GESTURE_DETECTION) {
                        SDL_Log("SD: Detection: distance = %f, velocity = %f, theta = %f, theta2 = %f, sv angle = %f, lastv angle = %f",
                                distance, velocity, theta, theta2,
                                sv.getAngle(),
                                lastVector->getAngle());
                    }
                    // Multiple events with the same timestamp is a possibility
                    // hence the isinf() check.
                    if (std::abs(theta) < 3.0 && std::abs(mgesture.dDist) > 0.01 && !std::isinf(velocity) && velocity > 0.001) { // 0.0004
                        if (SWIPEDETECTOR_LOG_GESTURE_DETECTION)
                            SDL_Log("----------------- SD: Swipe detected -----------------");
                        gestureSwipe = true;
                        //Swipe::Direction direction = Swipe::getDirection(Swipe::getAngle(gestureStart.point.x, gestureStart.point.y, sv.w, sv.h));
                        Swipe::Direction direction = Swipe::getDirection(sv.getNormalizedAngle());
                        return static_cast<enum result>(direction);
                    } else {
                        if (SWIPEDETECTOR_LOG_GESTURE_DETECTION) SDL_Log("SD: No swipe detected.");
                        result = eEventNotConsumed;
                    }
                } else {
                     //lastVector.x = sv.x / distance;
                     //lastVector.y = sv.y / distance;
                     lastVector = sv;
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

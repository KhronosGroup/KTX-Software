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

// These macros only allow storing a Direction in pointers, and only preserve 32 bits
// of the Direction enum' integer value; values outside the range of a 32-bit integer
// will be mangled. Ugly. Horrible. But preferable to allocating and freeing memory
// when passing the information in user events.
#define DIRECTION_TO_POINTER(i)	(reinterpret_cast<void*>(static_cast<long>(i)))
#define POINTER_TO_DIRECTION(p)	(static_cast<SwipeDetector::Direction>(reinterpret_cast<long>(p)))

class SwipeDetector {
  public:
    SwipeDetector() : gestureSwipe(false) { }
    bool doEvent(SDL_Event* event);

    enum Direction {
        up = 2,
        down = 3,
        left = 4,
        right = 5
    };

    static const Uint32 swipeGesture = 0x01;

    class vector {
      public:
        float w;
        float h;

        vector() : w(0.0), h(0.0) { }
        vector(float _w, float _h) : w(_w), h(_h) { }
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
        double getAngle() {
            double rad = atan2(h, w);
            return rad * 180/M_PI;
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
        double getAngle(const vector& v2) {
            // Reputed to be more accurate but in our use so far both approaches give same answer.
            //double rad = atan2f(v2.h, v2.w) - SDL_atan2(h, w);
            double rad = atan2f(w * v2.h - h * v2.w, w * v2.w + h * v2.h);
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
        double getAngleNormalized() {
            double rad = atan2(h, w) + M_PI;
            return fmod(rad*180/M_PI + 180, 360);
        }

        /**
         * @~English
         * @internal
         * @brief Return the length of the vector.
         *
         * @return the length of the vector.
         */
        float length() { return SDL_sqrt(w * w + h * h); }

        /**
         * @~English
         * @internal
         * @brief Return the direction of the vector.
         *
         * @return the direction
         */
        Direction getDirection() {
            double angle = getAngleNormalized();
            return getDirection(angle);
        }

        /**
         * @~English
         * @internal
         * @brief Return a direction given an angle.
         *
         * Directions are defined as follows:
         *
         * Up: [45, 135]
         * Right: [0,45] and [315, 360]
         * Down: [225, 315]
         * Left: [135, 225]
         *
         * @param angle an angle from 0 to 360°
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

      protected:
        /**
         * @~English
         * @internal
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
    };

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

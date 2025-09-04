/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow, <khronos at callow dot im>.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SWIPE_DETECTOR_H
#define _SWIPE_DETECTOR_H
#if defined(_WIN32)
  #define _USE_MATH_DEFINES
#endif
#include <optional>
#include <string>
#include <math.h>
#include <SDL3/SDL.h>
#include "SDL_gesture.h"

class SwipeDetector {
  public:
    enum class Direction { up, down, left, right };

    SwipeDetector() : gestureSwipe(false) {}
    bool doEvent(SDL_Event* event);

#if defined(_MSC_VER) && !defined(__clang__)
   // Not clangcl
   #pragma warning(push)
   #pragma warning(disable : 4311)
   #pragma warning(disable : 4302)
   #pragma warning(disable : 4312)
#endif
#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wvoid-pointer-to-int-cast"
#endif
    // These conversions allow storing a Direction in a pointer.
    // Ugly. Horrible. But preferable to allocating and freeing memory
    // when passing the information in user events.
    static inline Direction pointerToDirection(void* p) {
        return static_cast<SwipeDetector::Direction>(reinterpret_cast<long>(p));
    }

    // Only preserves the low 32-bits of the pointer; perfect for this use.
    static inline void* directionToPointer(SwipeDetector::Direction d) {
        return reinterpret_cast<void*>(static_cast<long>(d));
    }
#if defined(_MSC_VER) && !defined(__clang__)
        #pragma warning(pop)
#endif
#if defined(__clang__)
    #pragma clang diagnostic pop
#endif

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
        float length() { return sqrt(w * w + h * h); }

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
                return Direction::down;
            } else if (inRange(angle, 0, 45) || inRange(angle, 315, 360)) {
                return Direction::right;
            } else if (inRange(angle, 225, 315)) {
                return Direction::up;
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
    struct gestureStart {
        Uint64 time;
        SDL_FPoint point;
        gestureStart() { time = 0; point.x = point.y = 0.0; }
    } gestureStart;
    std::optional<vector> lastVector;
    bool gestureSwipe;
};

[[nodiscard]] inline std::string toString(SwipeDetector::Direction dir) {
    switch (dir) {
      case SwipeDetector::Direction::up: return "up";
      case SwipeDetector::Direction::down: return "down";
      case SwipeDetector::Direction::left: return "left";
      case SwipeDetector::Direction::right: return "right";
      // This is to hide a warning from MSVC. According to the solution given
      // in https://developercommunity.visualstudio.com/t/Visual-Studio-warning-on-Strongly-typed-/96302
      // it is possible to construct an enum class with any value. Thus warning.
      default: return "unknown";
    }
}

#endif /* _SWIPE_DETECTOR_H */

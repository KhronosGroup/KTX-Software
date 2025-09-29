/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @class LoadTestSample
 * @~English
 *
 * @brief Definition of a base class for texture loading test samples.
 *
 * @author Mark Callow, github.com/MarkCallow.
 */

#if defined(_WIN32)
  #define _USE_MATH_DEFINES
#endif
#include "LoadTestSample.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/vector_angle.hpp>
#include <SDL3/SDL_log.h>

#if !defined(LOADTESTSAMPLE_LOG_GESTURE_DETECTION)
  // Log detected and completed gestures.
  #define LOADTESTSAMPLE_LOG_GESTURE_DETECTION 0
#endif
#if !defined(LOADTESTSAMPLE_LOG_GESTURE_EVENTS)
  // Log events contributing to gesture detection and gestures.
  #define LOADTESTSAMPLE_LOG_GESTURE_EVENTS 0
#endif
#if !defined(LOADTESTSAMPLE_LOG_MOUSE_UP_DOWN_EVENTS)
  #define LOADTESTSAMPLE_LOG_MOUSE_UP_DOWN_EVENTS 0
#endif
#if !defined(LOADTESTSAMPLE_LOG_MOUSE_MOTION_EVENTS)
  #define LOADTESTSAMPLE_LOG_MOUSE_MOTION_EVENTS 0
#endif

#if LOADTESTSAMPLE_LOG_GESTURE_EVENTS
  #include <sstream>

  const std::string printFingerIds(SDL_Finger* fingers[], uint32_t numFingers) {
      std::stringstream msg;
      assert(numFingers > 0);
      msg << std::hex << std::showbase;
      msg << "finger id" << (numFingers > 1 ? "s" : "") << ": ";
      for (uint32_t f = 0; f < numFingers; f++) {
          if (f > 0) {
              if (f == numFingers - 1)
                  msg << " & ";
              else
                  msg << ", ";
          }
          msg << fingers[f]->id;
      }
      return msg.str();
  }

  const std::string printVector(const std::string& name, glm::vec2 v) {
      std::stringstream msg;
      msg << name << " (" << v.x << ", " << v.y << ")";
      return msg.str();
  }
#endif

[[maybe_unused]] static const char*
buttonName(Uint8 button) {
    switch(button) {
        case SDL_BUTTON_LEFT: return "left";
        case SDL_BUTTON_MIDDLE: return "middle";
        case SDL_BUTTON_RIGHT: return "right";
        default: return "other";
    }
}

int
LoadTestSample::doEvent(SDL_Event* event)
{
    switch (event->type) {
      case SDL_EVENT_MOUSE_MOTION:
      {
        SDL_MouseMotionEvent& motion = event->motion;
#if LOADTESTSAMPLE_LOG_MOUSE_MOTION_EVENTS
        SDL_Log("LTS: MOUSE_MOTION - x: %f, y: %f", motion.x, motion.y);
#endif
        // On macOS with trackpad, SDL_TOUCH_MOUSEID is never set.
        // Prefer mouse events on macOS because press is required. When
        // finger motion events are used the object starts to rotate when
        // you drag the cursor over the window. Not nice.
        if (mouseButtons.left)
        {
            rotation.x -= yflip * (mousePos.y - (float)motion.y) * 1.25f;
            rotation.y -= (mousePos.x - (float)motion.x) * 1.25f;
            viewChanged();
        }
        if (mouseButtons.right)
        {
            zoom += (mousePos.y - (float)motion.y) * .005f;
            viewChanged();
        }
        if (mouseButtons.middle)
        {
            cameraPos.x -= (mousePos.x - (float)motion.x) * 0.01f;
            cameraPos.y += yflip * (mousePos.y - (float)motion.y) * 0.01f;
            viewChanged();
        }
        mousePos = glm::vec2((float)motion.x, (float)motion.y);
        return 0;
      }
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        mousePos = glm::vec2((float)event->button.x, (float)event->button.y);
        if (LOADTESTSAMPLE_LOG_MOUSE_UP_DOWN_EVENTS) {
            SDL_Log("LTS: MOUSE_DOWN - button: %s, x: %f, y: %f", buttonName(event->button.button),
                     event->button.x, event->button.y);
        }
        switch (event->button.button) {
          case SDL_BUTTON_LEFT:
            mouseButtons.left = true;
            break;
          case SDL_BUTTON_MIDDLE:
            mouseButtons.middle = true;
            break;
          case SDL_BUTTON_RIGHT:
            mouseButtons.right = true;
            break;
          default:
            return 1;
        }
        return 0;
      case SDL_EVENT_MOUSE_BUTTON_UP:
       if (LOADTESTSAMPLE_LOG_MOUSE_UP_DOWN_EVENTS) {
            SDL_Log("LTS: MOUSE_UP - button: %s, x: %f, y: %f", buttonName(event->button.button),
                     event->button.x, event->button.y);
        }
        switch (event->button.button) {
          case SDL_BUTTON_LEFT:
            mouseButtons.left = false;
            break;
          case SDL_BUTTON_MIDDLE:
            mouseButtons.middle = false;
            break;
          case SDL_BUTTON_RIGHT:
            mouseButtons.right = false;
            break;
          default:
            return 1;
        }
        return 0;
      case SDL_EVENT_FINGER_DOWN: {
        // Prevent multifingers from triggering the left button action and
        // interfering with multigestures.
        //
        // On iOS you get a left button down event no matter how many fingers
        // you touch to the screen. We want 1 finger mouse to work so
        // behaviour is same as pressing the trackpad on macOS, etc. As iOS
        // button_down events come before finger_down we can clear the left
        // button down state, if we have multiple fingers. Hope this ordering
        // is the same on other touch screen platforms that send a left-button
        // event regardless of the number of fingers.
        //
        // On macOS button_down events come after finger_down so this code has
        // no effect.
        //
        // Another way to handle this is to identify the platform and work
        // differently for each platform.
        int numFingers;
        SDL_Finger** fingers = SDL_GetTouchFingers(event->tfinger.touchID, &numFingers);
        int retVal = 0;
#if LOADTESTSAMPLE_LOG_GESTURE_EVENTS
        SDL_Log("LTS: Finger: %#" SDL_PRIx64 " down - fingers: %i, %s, x: %f, y: %f",
                event->tfinger.fingerID, numFingers,
                printFingerIds(fingers, numFingers).c_str(),
                event->tfinger.x, event->tfinger.y);
#endif
        if (numFingers > 1) {
            mouseButtons.left = false;
            if (LOADTESTSAMPLE_LOG_GESTURE_EVENTS) {
                SDL_Log("LTS: FINGER_DOWN with multiple fingers received."
                        " Resetting mouseButtons.left.");
            }
            if (numFingers == 2) {
                firstFingerId = fingers[0]->id;
                // Calc. difference vector between fingers.
                glm::vec2 vDifference;
                vDifference.x = fingers[1]->x - fingers[0]->x;
                vDifference.y = fingers[1]->y - fingers[0]->y;
                distanceStart = glm::length(vDifference);
                distanceLast = distanceStart;
                // Need normalized vectors for glm::orientedAngle
                nvDifferenceStart = glm::normalize(vDifference);
                nvDifferenceLast = nvDifferenceStart;
                processingGesture = true;
#if LOADTESTSAMPLE_LOG_GESTURE_EVENTS
                // Angle of vector to X axis.
                xAngleStart = atan2f(vDifference.y, vDifference.x);
                SDL_Log("LTS: FINGER_DOWN, start values: %s, Distance = %f, XAngle = %f°",
                        printVector("Difference", vDifference).c_str(),
                        distanceStart, xAngleStart * 180.0 / M_PI
                        );
#endif
                retVal = 1;
            }
        }

        // It is possible to somehow get out of the window without seeing
        // FINGER_UP so as a safeguard stop any previous gesture.
        zooming = rotating = false;
        SDL_free(fingers);
        return retVal;
      }
      case SDL_EVENT_FINGER_UP: {
        int numFingers;
        SDL_Finger** fingers = SDL_GetTouchFingers(event->tfinger.touchID, &numFingers);
#if LOADTESTSAMPLE_LOG_GESTURE_EVENTS
        SDL_Log("LTS: Finger: %#" SDL_PRIx64 " up - fingers: %i, %s, x: %f, y: %f",
                event->tfinger.fingerID, numFingers,
                printFingerIds(fingers, numFingers).c_str(),
                event->tfinger.x, event->tfinger.y);
#endif
        if (processingGesture && numFingers == 2) {
            // There may still be one finger down. Even so the action is completed.
            if (LOADTESTSAMPLE_LOG_GESTURE_DETECTION) {
                SDL_Log("-------------- LTS: %s complete. -----------------",
                        zooming ? "zooming" : rotating ? "rotating" : "gesture");
            }
            zooming = rotating = processingGesture = false;
        }
        SDL_free(fingers);
        break;
      }
      case SDL_EVENT_FINGER_MOTION: {
        int numFingers;
        SDL_Finger** fingers = SDL_GetTouchFingers(event->tfinger.touchID, &numFingers);
        if (numFingers != 2)
            return 1;
        if (!processingGesture) {
            // Protect against FINGER_MOTION without FINGER_DOWN. This can
            // happen when the sample is switched by a swipe and the new sample
            // receives the tail end of the swipe motion.
            return 1;
        }
        // With two fingers down, events come in pairs. No point in processing
        // both.
        if (event->tfinger.fingerID == firstFingerId) {
            return 0;
        }

        glm::vec2 vDifference; // Difference vector between the fingers.
        vDifference.x = fingers[1]->x - fingers[0]->x;
        vDifference.y = fingers[1]->y - fingers[0]->y;
        float distance = glm::length(vDifference);
        // Normalized vectors required by glm::orientedAngle
        glm::vec2 nvDifference = glm::normalize(vDifference);
        // Angle between start and current difference vectors
        float sAngle = glm::orientedAngle(nvDifferenceStart, nvDifference);
        // Angle between current and previous difference vectors
        float dAngle = glm::orientedAngle(nvDifferenceLast, nvDifference);
        // Difference in distance since last motion event.
        float dDist = distance - distanceLast;
        // Difference in distance since start.
        float dDistStart = distance - distanceStart;
#if LOADTESTSAMPLE_LOG_GESTURE_EVENTS
        if (!(rotating || zooming)) {
                // Angle from X axis to vDifference vector
                float xAngle = atan2f(vDifference.y, vDifference.x);
                SDL_Log("LTS FINGER_MOTION: Not zooming or rotating. "
                        " timestamp = %" SDL_PRIu64 ", %s, %s",
                        event->tfinger.timestamp,
                        printFingerIds(fingers, numFingers).c_str(),
                        printVector("Difference", vDifference).c_str());
                SDL_Log("... distanceLast = %f, distance = %f, dDist = %f, dDistStart = %f, xAngle = %f°, sAngle = %f°, dAngle = %f°",
                        distanceLast, distance, dDist, dDistStart,
                        xAngle * 180.0 / M_PI, sAngle * 180.0 / M_PI, dAngle * 180.0 / M_PI);
        }
#endif
        nvDifferenceLast = nvDifference;
        distanceLast = distance;

        // This is all heuristics derived from use.
        if (zooming) {
            zoom += dDist * 10.0f;
            if (LOADTESTSAMPLE_LOG_GESTURE_EVENTS) {
                SDL_Log("LTS MG: Zooming. zoom = %f", zoom);
            }
        } else if (!rotating) {
            if (fabs(dDistStart) >= 0.1 && fabs(dAngle) < 0.5 * M_PI / 180.0) {
                zooming = true;
                zoom += dDist * 10.0f;
                if (LOADTESTSAMPLE_LOG_GESTURE_DETECTION) {
                    SDL_Log("---------------- LTS MG: pinch/zoom detected ---------------\n"
                            " dAngle = %f°, dDistStart = %f, dDist = %f, zoom = %f",
                            dAngle * 180.0 / M_PI, dDistStart, dDist, zoom);
                }
            }
        }
        if (rotating) {
            rotation.z +=
                static_cast<float>(dAngle * 180.0 / M_PI);
            if (LOADTESTSAMPLE_LOG_GESTURE_EVENTS) {
                SDL_Log("LTS MG: Rotating around Z. rotation.z = %f°", rotation.z);
            }
       } else if (!zooming) {
          if (fabs(sAngle) > 15 * M_PI / 180.0 && fabs(dDistStart) < 0.1) {
                rotating = true;
                rotation.z += static_cast<float>(dAngle * 180.0 / M_PI);
                if (LOADTESTSAMPLE_LOG_GESTURE_DETECTION) {
                    SDL_Log("---------------- LTS MG: rotation detected ---------------\n"
                            " sAngle = %f°, dAngle = %f°, dDistStart = %f, rotation.z = %f°",
                            sAngle * 180 / M_PI, dAngle * 180.0 / M_PI, dDistStart, rotation.z);
                }
            }
        }
        viewChanged();
        SDL_free(fingers);
        return 0;
      }
      case SDL_EVENT_KEY_UP:
        if (event->key.key == 'q')
            quit = true;
        keyPressed(event->key.key);
        return 0;
      default:
        break;
    }
    return 1;
}


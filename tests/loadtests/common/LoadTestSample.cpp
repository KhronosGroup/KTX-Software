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
 * @author Mark Callow, www.edgewise-consulting.com.
 */

#if defined(_WIN32)
  #define _USE_MATH_DEFINES
#endif
#include "LoadTestSample.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/vector_angle.hpp>
#include <SDL3/SDL_log.h>


#if !defined(LOADTESTSAMPLE_LOG_MOTION_EVENTS)
  #define LOADTESTSAMPLE_LOG_MOTION_EVENTS 0
#endif
#if !defined(LOADTESTSAMPLE_LOG_UP_DOWN_EVENTS)
  #define LOADTESTSAMPLE_LOG_UP_DOWN_EVENTS 1
#endif
#if !defined(LOADTESTSAMPLE_LOG_GESTURE_DETECTION)
  #define LOADTESTSAMPLE_LOG_GESTURE_DETECTION 1
#endif
#if !defined(LOADTESTSAMPLE_LOG_GESTURE_EVENTS)
  #define LOADTESTSAMPLE_LOG_GESTURE_EVENTS 1
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
#if LOADTESTSAMPLE_LOG_MOTION_EVENTS
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
        //if (event->button.which == SDL_TOUCH_MOUSEID
        //    && SDL_GetNumTouchFingers(event->tfinger.touchId) != 1)
        //    return 0;
        //if (event->button.which == SDL_TOUCH_MOUSEID)
        //    return 0;
        mousePos = glm::vec2((float)event->button.x, (float)event->button.y);
        if (LOADTESTSAMPLE_LOG_UP_DOWN_EVENTS) {
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
        //if (event->button.which == SDL_TOUCH_MOUSEID
        //    && SDL_GetNumTouchFingers(event->tfinger.touchId) != 1)
        //    return 0;
        //if (event->button.which == SDL_TOUCH_MOUSEID)
        //    return 0;
        if (LOADTESTSAMPLE_LOG_UP_DOWN_EVENTS) {
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
        if (LOADTESTSAMPLE_LOG_UP_DOWN_EVENTS) {
            SDL_Log("LTS: Finger: %#" SDL_PRIx64 " down - fingers: %i, x: %f, y: %f",
                    event->tfinger.fingerID, numFingers, event->tfinger.x, event->tfinger.y);
        }
        if (numFingers > 1) {
            mouseButtons.left = false;
            if (LOADTESTSAMPLE_LOG_UP_DOWN_EVENTS) {
                SDL_Log("LTS: FINGER_DOWN with multiple fingers received."
                        " Resetting mouseButtons.left.");
            }
            if (numFingers == 2) {
                // Calc. difference vector between fingers.
                lastDifference.x = fingers[1]->x - fingers[0]->x;
                lastDifference.y = fingers[1]->y - fingers[0]->y;
                initialDifference = lastDifference;
                initialDistance = glm::length(initialDifference);
                // Angle of vector to X axis.
                initialXAngle = atan2f(lastDifference.y, lastDifference.x);
                assert(initialXAngle != 0);
                lastVectorTimestamp = event->tfinger.timestamp;
                lastFMTimestamp = 0;
                processingGesture = true;
                if (LOADTESTSAMPLE_LOG_UP_DOWN_EVENTS) {
                    SDL_Log("LTS: FINGER_DOWN: initialDistance = %f, initialXAngle = %f°",
                            initialDistance, initialXAngle * 180.0 / M_PI
                            );
                    }
                retVal = 1;
            }
        }
        // It is possible to somehow get out of the window without seeing FINGER_UP so
        // as a safeguard stop any previous gesture.
        zooming = rotating = false;
        SDL_free(fingers);
        return retVal;
      }
      case SDL_EVENT_FINGER_UP: {
        int numFingers;
        SDL_Finger** fingers = SDL_GetTouchFingers(event->tfinger.touchID, &numFingers);
        if (LOADTESTSAMPLE_LOG_UP_DOWN_EVENTS) {
            SDL_Log("LTS: Finger: %#" SDL_PRIx64 " up - fingers: %i, x: %f, y: %f",
                    event->tfinger.fingerID, numFingers, event->tfinger.x, event->tfinger.y);
        }
        if (numFingers == 2) {
            // There may still be one finger down. Even so the action is completed.
            if (LOADTESTSAMPLE_LOG_GESTURE_EVENTS) {
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
            // Protect against FINGER_MOTION without FINGER_DOWN. This can happen
            // when the sample is switched by a swipe and the new sample receives
            // the tail end of the swipe motion.
            return 1;
        }
        if (event->tfinger.timestamp != lastFMTimestamp) {
            // This event is the motion of the first finger of the pair.
            lastFMTimestamp = event->tfinger.timestamp;
            return 0;
        }

        lastFMTimestamp = event->tfinger.timestamp;
        glm::vec2 difference; // between fingers.
        difference.x = fingers[1]->x - fingers[0]->x;
        difference.y = fingers[1]->y - fingers[0]->y;
        float distanceOfLast = glm::length(lastDifference);
        float distance = glm::length(difference);
        // Normalized vectors required by glm::angle
        //glm::normalize(lastVector);
        //glm::normalize(vector);
        //float angle_glm = glm::angle(vector, lastVector);
        // Angle from X axis to difference vector
        float xAngle = atan2f(difference.y, difference.x);
        float xAngleOfLast = atan2f(lastDifference.y, lastDifference.x);
         //atan2f(v1.x * v2.y - v1.y * v2.x, v1.x * v2.x + v1.y * v2.y);
        //float angle = atan2f(vector.x * lastVector.y - vector.y * lastVector.x, vector.x * lastVector.x + vector.y * lastVector.y);
        // Angle between current and initial difference vectors
        float iAngle = atan2f(difference.x * initialDifference.y - difference.y * initialDifference.x, difference.x * initialDifference.x + difference.y * initialDifference.y);
        // Magnitude of the angle between current and initial difference vectors
        //float iDot = glm::dot(difference, initialDifference);
        //float iAngle = acosf(iDot / fabs(distance) * fabs(initialDistance));
        //if (iDot < 0) iAngle
        //float iAngle = xAngle - initialXAngle;
        //iAngle = fmod(iAngle + M_PI, 2 * M_PI);
        // Angle between current and previous difference vectors
        //assert(initialXAngle != 0 && iAngle != xAngle);
        float iAngleMag;
        iAngleMag = fabs(iAngle);
        if (iAngle > M_PI / 2)
            iAngleMag = M_PI - iAngle;
        float dAngleCalc = atan2f(difference.x * lastDifference.y - difference.y * lastDifference.x, difference.x * lastDifference.x + difference.y * lastDifference.y);
        float dAngle = xAngle - xAngleOfLast;
        float dDist = distance - distanceOfLast;
        float dDistStart = distance - initialDistance;
        float timestep = (event->tfinger.timestamp - lastVectorTimestamp) / 1000000.0;
        //float dDist, dTheta;
        assert(timestep != 0);
        //if (timestep == 0) {
        //    dDist = distance - lastDistance;
        //    dTheta = angle - lastAngle;
        //} else {
            float dDist_r = dDist / timestep;
            float dAngle_r = dAngle / timestep;
            //dDist = distance - initialDistance;
            //dTheta = angle_x - initialAngle;
            //float dTheta1 = angle_x - lastAngle;
        //}
        lastDifference = difference;
        lastAngle = xAngle;
        lastVectorTimestamp = event->tfinger.timestamp;
        if (LOADTESTSAMPLE_LOG_GESTURE_EVENTS && !(rotating || zooming)) {
#if 0
                SDL_Log("LTS FINGER_MOTION: Not zooming or rotating. "
                        " timestamp = %" SDL_PRIu64 ", distance = %f, angle_glm = %f°, angle_x = %f°, angle = %f°, dDist = %f, dTheta = %f°",
                        event->tfinger.timestamp,
                        distance, angle_glm * 180.0 / M_PI, angle_x * 180.0 / M_PI, angle * 180.0 / M_PI,
                        dDist, dTheta * 180.0 / M_PI);
#else
                SDL_Log("LTS FINGER_MOTION: Not zooming or rotating. "
                        " timestamp = %" SDL_PRIu64 ", distanceOfLast = %f, distance = %f, dDist = %f, dDistStart = %f, dDist_r = %f, xAngle = %f°, iAngle = %f°, iAngleMag = %f°, dAngle = %f°, dAngleCalc = %f°, dAngle_r = %f°",
                        event->tfinger.timestamp,
                        distanceOfLast, distance, dDist, dDistStart, dDist_r,
                        xAngle * 180.0 / M_PI, iAngle * 180.0 / M_PI, iAngleMag * 180.0 / M_PI, dAngle * 180.0 / M_PI,
                        dAngleCalc * 180.0 / M_PI, dAngle_r * 180.0 / M_PI);
#endif
        }
#if 1
        // This is all heuristics derived from use.
#if 1
        if (zooming) {
            zoom += dDist * 10.0f;
            if (LOADTESTSAMPLE_LOG_GESTURE_EVENTS) {
                SDL_Log("LTS MG: Zooming. zoom = %f", zoom);
            }
        } else if (!rotating) {
            if (fabs(dDistStart) >= 0.1 && fabs(dAngle_r) < 0.03 * M_PI / 180.0) {
            //if (fabs(dDist_r) > 0.0022 && fabs(dAngle_r) < 0.03 * M_PI / 180.0) {
                zooming = true;
                zoom += dDist * 10.0f;
                if (LOADTESTSAMPLE_LOG_GESTURE_DETECTION) {
                    SDL_Log("---------------- LTS MG: spreading detected ---------------\n"
                            " dAngle_r = %f°, dDist = %f, zoom = %f",
                            dAngle_r * 180.0 / M_PI, dDist, zoom);
                }
            }
        }
#endif
        if (rotating) {
            rotation.z +=
                static_cast<float>(dAngle * 180.0 / M_PI);
            if (LOADTESTSAMPLE_LOG_GESTURE_EVENTS) {
                SDL_Log("LTS MG: Rotating around Z. rotation.z = %f°", rotation.z);
            }
       } else if (!zooming) {
           //if (fabs(dAngle_r) > 0.16 * M_PI / 180.0 && fabs(dDistStart) < 0.3) {
           if (iAngleMag > 10 * M_PI / 180.0 && fabs(dDistStart) < 0.04) {
                rotating = true;
                rotation.z += static_cast<float>(dAngle * 180.0 / M_PI);
                if (LOADTESTSAMPLE_LOG_GESTURE_DETECTION) {
                    SDL_Log("---------------- LTS MG: rotation detected ---------------\n"
                            " dAngle = %f°, dDist = %f, rotation.z = %f°",
                            dAngle * 180.0 / M_PI, dDist, rotation.z);
                }
            }
        }
        viewChanged();
#endif
        SDL_free(fingers);
        return 0;
      }
#if 0
      case GESTURE_MULTIGESTURE: {
        Gesture_MultiGestureEvent& mgesture = *(Gesture_MultiGestureEvent*)event;
        float dDist = 0; // Rate of change of distance between (moving) centroid and finger
                         // triggering this event.
        float dTheta = 0; // Rate of change of angle between vector from last centroid to last
                          // finger position and vector between current centroid and position.
        // NOTE: Despite the names, mgesture.dDist and mgetsure.dTheta are not derivatives.
        // mgesture.dDist is the change in distance between the centroid and finger in the
        // previous event and this one. mgesture.dTheta is the change in angle between the
        // vectors from centroid to finger in previous and current events. Note that the
        // centroid is moving too. Naming variables sucks!
        if (mgesture.numFingers != 2)  // For extra robustness.
            break;
        if (!zooming && !rotating) {
            if (lastGesture.timestamp == 0) {
                lastGesture = mgesture;
                return 0;
            } else {
                float duration = static_cast<float>(
                    (mgesture.timestamp - lastGesture.timestamp ) / 1000000.0);
                dDist = (mgesture.dDist - lastGesture.dDist) / duration;
                dTheta = (mgesture.dTheta - lastGesture.dTheta) / duration;
                lastGesture = mgesture;
            }
            if (LOADTESTSAMPLE_LOG_GESTURE_EVENTS) {
                SDL_Log("LTS MG: Not zooming or rotating. dDist = %f, dTheta = %f°"
                        " timestamp = %" SDL_PRIu64 ", dDist = %f, dTheta = %f",
                        mgesture.dDist,
                        mgesture.dTheta * 180.0 / M_PI,
                        mgesture.timestamp,
                        dDist, dTheta * 180.0 / M_PI);
            }
        }
        // This is all heuristics derived from use.
        if (zooming) {
            zoom += mgesture.dDist * 10.0f;
            if (LOADTESTSAMPLE_LOG_GESTURE_EVENTS) {
                SDL_Log("LTS MG: Zooming. zoom = %f", zoom);
            }
        } else if (!rotating) {
            //if (fabs(dDist) > 0.0006 && fabs(dTheta) < 0.1 * M_PI / 180.0) {
            if (fabs(dDist) > 0.5) {
                zooming = true;
                zoom += mgesture.dDist * 10.0f;
                if (LOADTESTSAMPLE_LOG_GESTURE_DETECTION) {
                    SDL_Log("---------------- LTS MG: spreading detected ---------------\n"
                            " dTheta = %f°, dDist = %f, zoom = %f",
                            dTheta * 180.0 / M_PI, dDist, zoom);
                }
            }
        }
        if (rotating) {
            rotation.z +=
                static_cast<float>(mgesture.dTheta * 180.0 / M_PI);
            if (LOADTESTSAMPLE_LOG_GESTURE_EVENTS) {
                SDL_Log("LTS MG: Rotating around Z. rotation.z = %f°", rotation.z);
            }
       } else if (!zooming) {
           //if (fabs(dDist) <= 0.0006 && fabs(dTheta) >= 0.1 * M_PI / 180.0) {
           if (fabs(dTheta) > 3.0) {
                rotating = true;
                rotation.z += static_cast<float>(mgesture.dTheta * 180.0 / M_PI);
                if (LOADTESTSAMPLE_LOG_GESTURE_DETECTION) {
                    SDL_Log("---------------- LTS MG: rotation detected ---------------\n"
                            " dTheta = %f°, dDist = %f, rotation.z = %f°",
                            dTheta * 180.0 / M_PI, dDist, rotation.z);
                }
            }
        }
        viewChanged();
        return 0;
      }
#endif // GESTURE_MULTIGESTURE
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


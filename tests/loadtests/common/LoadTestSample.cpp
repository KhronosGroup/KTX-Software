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
#include <SDL3/SDL_log.h>

#if !defined(LOADTESTSAMPLE_LOG_MOTION_EVENTS)
  #define LOADTESTSAMPLE_LOG_MOTION_EVENTS 0
#endif
#if !defined(LOADTESTSAMPLE_LOG_UP_DOWN_EVENTS)
  #define LOADTESTSAMPLE_LOG_UP_DOWN_EVENTS 1
#endif
#if !defined(LOADTESTSAMPLE_LOG_GESTURE_DETECTION)
  #define LOADTESTSAMPLE_LOG_GESTURE_DETECTION 0
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
        }
        // It is possible to somehow get out of the window without seeing FINGER_UP so
        // as a safeguard stop any previous gesture.
        accumDist = accumTheta = 0;
        zooming = rotating = false;
        SDL_free(fingers);
        return 0;
      }
      case SDL_EVENT_FINGER_UP: {
        int numFingers;
        SDL_Finger** fingers = SDL_GetTouchFingers(event->tfinger.touchID, &numFingers);
        if (LOADTESTSAMPLE_LOG_UP_DOWN_EVENTS) {
            SDL_Log("LTS: Finger: %#" SDL_PRIx64 " up - fingers: %i, x: %f, y: %f",
                    event->tfinger.fingerID, numFingers, event->tfinger.x, event->tfinger.y);
        }
        if (numFingers == 2) {
            accumDist = accumTheta = 0;
            zooming = rotating = false;
        }
        SDL_free(fingers);
        break;
      }
#if 0
      case SDL_FINGERMOTION: {
        int numFingers;
        SDL_Finger** fingers = SDL_GetTouchFingers(event->tfinger.touchID, &numFingers);
        SDL_free(fingers);
        if (numFingers == 1) {
            rotation.x += yflip * event->tfinger.dy * 225.0f;
            rotation.y += event->tfinger.dx * 225.0f;
            viewChanged();
            return 0;
        }
        return 1;
#endif
      case GESTURE_MULTIGESTURE: {
        Gesture_MultiGestureEvent& mgesture = *(Gesture_MultiGestureEvent*)event;
        if (mgesture.numFingers != 2)
            break;
        if (!zooming && !rotating) {
            accumDist += mgesture.dDist;
            accumTheta +=
                    static_cast<float>(mgesture.dTheta * 180.0 / M_PI);
            if (LOADTESTSAMPLE_LOG_GESTURE_EVENTS) {
                SDL_Log("LTS MG: Not zooming or rotating. dDist = %f, accumDist = %f,"
                        " dTheta = %f°, accumTheta = %f°, timestamp = %" SDL_PRIu64,
                        mgesture.dDist,
                        accumDist,
                        mgesture.dTheta * 180.0 / M_PI,
                        accumTheta,
                        mgesture.timestamp);
            }
        }
        // This is all heuristics derived from use.
        if (zooming) {
            zoom += mgesture.dDist * 10.0f;
            if (LOADTESTSAMPLE_LOG_GESTURE_EVENTS) {
                SDL_Log("LTS MG: Zooming. zoom = %f", rotation.z);
            }
        } else if (!rotating) {
            if (fabs(accumDist) > 0.012 && fabs(accumTheta) <= 3.0) {
                zooming = true;
                zoom += accumDist * 10.0f;
                if (LOADTESTSAMPLE_LOG_GESTURE_DETECTION) {
                    SDL_Log("LTS MG: spreading detected,"
                            " accumTheta = %f°, accumDist = %f, zoom = %f",
                            accumTheta, accumDist, zoom);
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
            if (fabs(accumTheta) > 15.0) {
                rotating = true;
                rotation.z += accumTheta;
                if (LOADTESTSAMPLE_LOG_GESTURE_DETECTION) {
                    SDL_Log("LTS MG: rotation detected,"
                            " accumTheta = %f°, accumDist = %f, rotation.z = %f°",
                            accumTheta,
                            accumDist,
                            rotation.z);
                }
            }
        }
        viewChanged();
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


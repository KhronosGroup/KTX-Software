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

#include "SDL2/SDL_log.h"
#include "LoadTestSample.h"

#if !defined(LOG_GESTURE_DETECTION)
  #define LOG_GESTURE_DETECTION 0
#endif

int
LoadTestSample::doEvent(SDL_Event* event)
{
    switch (event->type) {
      case SDL_MOUSEMOTION:
      {
        SDL_MouseMotionEvent& motion = event->motion;
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
      case SDL_MOUSEBUTTONDOWN:
        //if (event->button.which == SDL_TOUCH_MOUSEID
        //    && SDL_GetNumTouchFingers(event->tfinger.touchId) != 1)
        //    return 0;
        //if (event->button.which == SDL_TOUCH_MOUSEID)
        //    return 0;
        mousePos = glm::vec2((float)event->button.x, (float)event->button.y);
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
      case SDL_MOUSEBUTTONUP:
        //if (event->button.which == SDL_TOUCH_MOUSEID
        //    && SDL_GetNumTouchFingers(event->tfinger.touchId) != 1)
        //    return 0;
        //if (event->button.which == SDL_TOUCH_MOUSEID)
        //    return 0;
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
      case SDL_FINGERDOWN:
        // On iOS you get a left button down event no matter how many fingers
        // you touch to the screen. We want 1 finger mouse to work so
        // behaviour is same as pressing the trackpad on macOS, etc. On iOS
        // buttondown events come before fingerdown. Hope that is true on
        // other platforms.
        //
        // Another way to handle this is to identify the platform and work
        // differently for each platform.
        //
        // Prevent multifingers from triggering the left button action and
        // interfering with multigestures.
        if (SDL_GetNumTouchFingers(event->tfinger.touchId) > 1)
            mouseButtons.left = false;
        accumDist = accumTheta = 0;
        zooming = rotating = false;
        return 0;
#if 0
      case SDL_FINGERMOTION:
        if (SDL_GetNumTouchFingers(event->tfinger.touchId) == 1) {
            rotation.x += yflip * event->tfinger.dy * 225.0f;
            rotation.y += event->tfinger.dx * 225.0f;
            viewChanged();
            return 0;
        }
        return 1;
#endif
      case SDL_MULTIGESTURE:
        if (!zooming && !rotating) {
            accumDist += event->mgesture.dDist;
            accumTheta +=
                    static_cast<float>(event->mgesture.dTheta * 180.0 / M_PI);
            if (LOG_GESTURE_DETECTION) {
                SDL_Log("dDist = %f, accumDist = %f",
                        event->mgesture.dDist,
                        accumDist);
                SDL_Log("dTheta = %f°, accumTheta = %f°",
                        event->mgesture.dTheta * 180.0 / M_PI, accumTheta);
            }
        }
        if (zooming) {
            zoom += event->mgesture.dDist * 10.0f;
        } else {
            if (fabs(accumDist) > 0.018) {
                if (LOG_GESTURE_DETECTION) SDL_Log("zooming detected");
                zooming = true;
                zoom += accumDist * 10.0f;
            }
        }
        if (rotating) {
            rotation.z +=
                static_cast<float>(event->mgesture.dTheta * 180.0 / M_PI);
            if (LOG_GESTURE_DETECTION) {
                SDL_Log("rotation.z = %f°", rotation.z);
            }
       } else {
            if (fabs(accumTheta) > 20) {
                if (LOG_GESTURE_DETECTION) {
                    SDL_Log("rotation detected, accumTheta = %f°, rotation.z = %f°",
                            accumTheta,
                            rotation.z);
                }
                rotating = true;
                rotation.z += accumTheta;
                if (LOG_GESTURE_DETECTION) {
                    SDL_Log("rotation.z = %f°", rotation.z);
                }
            }
        }
        viewChanged();
        return 0;
      case SDL_KEYUP:
        if (event->key.keysym.sym == 'q')
            quit = true;
        keyPressed(event->key.keysym.sym);
        return 0;
      default:
        break;
    }
    return 1;
}


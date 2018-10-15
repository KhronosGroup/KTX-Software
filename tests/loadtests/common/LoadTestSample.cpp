/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2017 - 2018 Mark Callow.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
        if (zooming) {
            zoom += event->mgesture.dDist * 10.0f;
        } else {
            accumDist += event->mgesture.dDist;
            if (fabs(accumDist) > 0.018) {
                zooming = true;
                zoom += accumDist * 10.0f;
            }
            SDL_Log("accumDist = %f", accumDist);
        }
        if (rotating) {
            rotation.z += event->mgesture.dTheta * 180.0 / M_PI;
            accumTheta += event->mgesture.dTheta * 180.0 / M_PI;
            SDL_Log("**** accumTheta = %f", accumTheta);
       } else {
            accumTheta += event->mgesture.dTheta;
            if (fabs(accumTheta) > 0.2) {
                rotating = true;
                rotation.z += accumTheta * 180.0 / M_PI;
            }
            SDL_Log("accumTheta = %f", accumTheta);
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


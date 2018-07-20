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

#include "LoadTestSample.h"

int
LoadTestSample::doEvent(SDL_Event* event)
{
    switch (event->type) {
      case SDL_MOUSEMOTION:
      {
        SDL_MouseMotionEvent& motion = event->motion;
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
            mousePos.x = (float)motion.x;
            mousePos.y = (float)motion.y;
        }
        mousePos = glm::vec2((float)motion.x, (float)motion.y);
        return 0;
      }
      case SDL_MOUSEBUTTONDOWN:
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


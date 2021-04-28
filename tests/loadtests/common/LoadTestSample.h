/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow, <khronos at callow dot im>.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _LOAD_TEST_SAMPLE_H
#define _LOAD_TEST_SAMPLE_H

#include <string>
#include <SDL2/SDL_events.h>
#define GLM_FORCE_RADIANS
#include "disable_glm_warnings.h"
#include <glm/glm.hpp>
#include "reenable_warnings.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))

class LoadTestSample {
  public:
    typedef uint64_t ticks_t;
    LoadTestSample(uint32_t width, uint32_t height,
                   const std::string sBasePath,
                   int32_t yflip = 1)
           : w_width(width), w_height(height), yflip(yflip),
             sBasePath(sBasePath)
    {
        // Some compilers. e.g. VS2013, do not support initializers in the class
        // definition yet compile without warnings. So initialize the
        // old-fashioned way.
        mouseButtons.left = mouseButtons.middle = mouseButtons.right = false;
        quit = false;
        paused = false;
        timer = 0.f;
        timerSpeed = 0.25f;
        rotationSpeed = zoomSpeed = 1.f;
    }

    virtual ~LoadTestSample() { };
    virtual int doEvent(SDL_Event* event);
    virtual void resize(uint32_t width, uint32_t height) = 0;
    virtual void run(uint32_t msTicks) = 0;

    //virtual void getOverlayText(TextOverlay *textOverlay) { };

    typedef LoadTestSample* (*PFN_create)(uint32_t width, uint32_t height,
                                          const char* const szArgs,
                                          const std::string sBasePath);

  protected:
    virtual void keyPressed(uint32_t /*keyCode*/) { }
    virtual void viewChanged() { }
    
    const std::string getAssetPath() { return sBasePath; }

    glm::vec3 rotation;
    glm::vec3 cameraPos;
    glm::vec2 mousePos;
    float accumDist = 0;
    float accumTheta = 0;
    struct {
        bool left = false;
        bool right = false;
        bool middle = false;
    } mouseButtons;
    bool quit = false;
    bool rotating = false;
    bool zooming = false;

    float zoom = 0;

    uint32_t w_width;
    uint32_t w_height;

    // Defines a frame rate independent timer value clamped from -1.0...1.0
    // For use in animations, rotations, etc.
    float timer = 0.0f;
    // Multiplier for speeding up (or slowing down) the global timer
    float timerSpeed = 0.25f;

    bool paused = false;

    // Use to adjust mouse rotation speed
    float rotationSpeed = 1.0f;
    // Use to adjust mouse zoom speed
    float zoomSpeed = 1.0f;
    // multiplier to decide if Y increases down or up
    int32_t yflip;

    const std::string sBasePath;
};

#endif /* _LOAD_TEST_SAMPLE_H */

/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow, <khronos at callow dot im>.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _LOAD_TEST_SAMPLE_H
#define _LOAD_TEST_SAMPLE_H

#include <string>
#include <SDL3/SDL.h>
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
    glm::vec2 nvDifferenceStart; // Normalized difference between fingers at start of gesture.
    float distanceStart = 0.0; // Distance between fingers at start of gesture.
    float xAngleStart = 0.0;   // Angle between x-axis and nvDifferenceStart. Unused unless event logging is enabled.
    glm::vec2 nvDifferenceLast; // Normalized difference between fingers at last motion event.
    float distanceLast = 0.0;   // Distance between fingers at last motion event.
    Uint64 firstFingerId = 0;
    bool processingGesture = false;

    struct {
        bool left = false;
        bool right = false;
        bool middle = false;
    } mouseButtons;
    bool quit = false;
    bool rotating = false;
    bool zooming = false;
    bool paused = false;

    float zoom = 0;

    uint32_t w_width;
    uint32_t w_height;

    // Defines a frame rate independent timer value clamped from -1.0...1.0
    // For use in animations, rotations, etc.
    float timer = 0.0f;
    // Multiplier for speeding up (or slowing down) the global timer
    float timerSpeed = 0.25f;

    // Use to adjust mouse rotation speed
    float rotationSpeed = 1.0f;
    // Use to adjust mouse zoom speed
    float zoomSpeed = 1.0f;
    // multiplier to decide if Y increases down or up
    int32_t yflip;

    const std::string sBasePath;
};

#endif /* _LOAD_TEST_SAMPLE_H */

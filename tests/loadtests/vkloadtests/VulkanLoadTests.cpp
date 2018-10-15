/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2017 Mark Callow.
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
 * @class VulkanLoadTests
 * @~English
 *
 * @brief Framework for Vulkan texture loading test samples.
 *
 * @author Mark Callow, www.edgewise-consulting.com.
 */

#include <exception>

#define _USE_MATH_DEFINES
#include <math.h>

#include <SDL2/SDL_vulkan.h>

#include "VulkanLoadTests.h"
#include "Texture.h"
#include "TextureArray.h"
#include "TextureCubemap.h"
#include "TexturedCube.h"
#include "TextureMipmap.h"
#include "ltexceptions.h"


namespace Swipe {
    enum Direction {
        up,
        down,
        left,
        right
    };

    /**
     * @~English
     * @brief Find the angle between two points in a plane.
     *
     * The angle is measured with 0/360 being the X-axis to the right, angles
     * increase counter clockwise.
     *
     * @param x1 the x position of the first point
     * @param y1 the y position of the first point
     * @param x2 the x position of the second point
     * @param y2 the y position of the second point
     *
     * @return the angle between two points
     */
    double getAngle(float x1, float y1, float x2, float y2) {
        double rad = atan2(y1-y2,x2-x1) + M_PI;
        return fmod(rad*180/M_PI + 180, 360);
    }

    /**
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

    /**
     * Returns a direction given an angle.
     * Directions are defined as follows:
     *
     * Up: [45, 135]
     * Right: [0,45] and [315, 360]
     * Down: [225, 315]
     * Left: [135, 225]
     *
     * @param angle an angle from 0 to 360 - e
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

    /**
     * Given two points in the plane p1=(x1, x2) and p2=(y1, y1), this method
     * returns the direction that an arrow pointing from p1 to p2 would have.
     *
     * @param x1 the x position of the first point
     * @param y1 the y position of the first point
     * @param x2 the x position of the second point
     * @param y2 the y position of the second point
     * @return the direction
     */
    Direction getDirection(float x1, float y1, float x2, float y2){
        double angle = getAngle(x1, y1, x2, y2);
        return getDirection(angle);
    }
};

#define LT_VK_MAJOR_VERSION 1
#define LT_VK_MINOR_VERSION 0
#define LT_VK_PATCH_VERSION 0
#define LT_VK_VERSION VK_MAKE_VERSION(1, 0, 0)

VulkanLoadTests::VulkanLoadTests(const sampleInvocation samples[],
                                 const int numSamples,
                                 const char* const name)
                  : siSamples(samples), sampleIndex(numSamples),
                    VulkanAppSDL(name, 1280, 720, LT_VK_VERSION, true)
{
    pCurSample = nullptr;
    //eventWrite = 0;
    //swipe.start.timestamp = 0;
    mgestureFirstNotSaved = true;
    mgestureNotSwipe = true;
}

VulkanLoadTests::~VulkanLoadTests()
{
    if (pCurSample != nullptr) {
        delete pCurSample;
    }
}

bool
VulkanLoadTests::initialize(int argc, char* argv[])
{
    if (!VulkanAppSDL::initialize(argc, argv))
        return false;

    // Launch the first sample.
    invokeSample(Direction::eForward);
    return true;
}


void
VulkanLoadTests::finalize()
{
    if (pCurSample != nullptr) {
        delete pCurSample;
    }
    VulkanAppSDL::finalize();
}


int
VulkanLoadTests::doEvent(SDL_Event* event)
{
    int result = 0;
    float distanceSq = 0;

    switch (event->type) {
      case SDL_KEYUP:
        switch (event->key.keysym.sym) {
          case 'q':
            quit = true;
            break;
          case 'n':
            ++sampleIndex;
            invokeSample(Direction::eForward);
            break;
          case 'p':
            --sampleIndex;
            invokeSample(Direction::eBack);
            break;
          default:
            result = 1;
        }
        break;
#define VERBOSE 1
#if 0
      case SDL_FINGERDOWN:
#if VERBOSE
        SDL_Log("Finger: %"SDL_PRIs64" down - x: %f, y: %f",
           event->tfinger.fingerId,event->tfinger.x,event->tfinger.y);
#endif
        fingerDownTimestamp = event->tfinger.timestamp;

        break;
#endif
      case SDL_FINGERUP:
#if VERBOSE
        SDL_Log("Finger: %" SDL_PRIs64 " up - x: %f, y: %f",
               event->tfinger.fingerId,event->tfinger.x,event->tfinger.y);
#endif
#if 0
        if (swipe.start.timestamp != 0) {
            Swipe::Direction direction = Swipe::getDirection(swipe.start.x, swipe.start.y, swipe.last.x, swipe.last.y);
            if (direction == Swipe::left) {
                ++sampleIndex;
                invokeSample(Direction::eForward);
            } else if (direction == Swipe::right) {
                --sampleIndex;
                invokeSample(Direction::eBack);
            }
            // else ignore. Up & down 2-fingered swipes are equivalent to
            // right button down & drag on some systems. RDD is used for
            // zooming.
            swipe.start.timestamp = 0;
        }
        mgestureNotSwipe = false;
#endif
        mgestureNotSwipe = true;
        mgestureFirstNotSaved = true;
        //eventWrite = 0;
        break;
      case SDL_MULTIGESTURE:
#if VERBOSE
        SDL_Log("MG: x = %f, y = %f, dAng = %f, dR = %f, numFingers = %i",
           event->mgesture.x,
           event->mgesture.y,
           event->mgesture.dTheta,
           event->mgesture.dDist,
           event->mgesture.numFingers);
        //SDL_Log("eventWrite = %i, mgestureNotSwipe = %i", eventWrite, mgestureNotSwipe);
        SDL_Log("mgestureNotSwipe = %i", mgestureNotSwipe);
#endif
        if (mgestureFirstNotSaved) {
            mgestureFirst = event->mgesture;
            mgestureFirstNotSaved = false;
        }
        //events[eventWrite] = *event;
        //eventWrite++;
        //eventWrite &= eventBufSize - 1; // Rotate to 0, if necessary.
        if (mgestureNotSwipe) {
            float dx, dy, velocitySq;
            dx = event->mgesture.x - mgestureFirst.x;
            dy = event->mgesture.y - mgestureFirst.y;
            distanceSq = dx * dx + dy * dy;
            velocitySq = distanceSq / (event->mgesture.timestamp - fingerDownTimestamp);
#if VERBOSE
            SDL_Log("MG: distanceSq = %f, velocitySq = %f",
                    distanceSq, velocitySq);
#endif
            //if (distanceSq > 0.10) {
            if (velocitySq > 0.000005) {
                Swipe::Direction direction
                    = Swipe::getDirection(mgestureFirst.x, mgestureFirst.y,
                                          event->mgesture.x, event->mgesture.y);
                if (direction == Swipe::left) {
                    ++sampleIndex;
                    invokeSample(Direction::eForward);
                } else if (direction == Swipe::right) {
                    --sampleIndex;
                    invokeSample(Direction::eBack);
                }
                mgestureNotSwipe = false;
            } else
                result = 1;
        }
#if 0
        if (mgestureNotSwipe) {
            result = 1;
        } else if (swipe.start.timestamp == 0) {
            if (event->mgesture.numFingers == 2 && distanceSq > 0.05f) {
                // We've got a swipe
                swipe.start.timestamp = events[0].mgesture.timestamp;
                swipe.start.x = events[0].mgesture.x;
                swipe.start.y = events[0].mgesture.y;
                eventWrite = 0;
            } else if (eventWrite > 40) {
                // Not a swipe. Pass accumulated events to sample.
                if (pCurSample != nullptr) {
                    for (uint32_t i = 0; i < eventWrite; i++)
                        result = pCurSample->doEvent(&events[i]);
                }
                eventWrite = 0;
                mgestureNotSwipe = true;
            }
        } else {
            swipe.last.timestamp = event->mgesture.timestamp;
            swipe.last.x = event->mgesture.x;
            swipe.last.y = event->mgesture.y;
            eventWrite = 0;
        }
#endif
        break;
      default:
          result = 1;
    }
    
    if (result == 1) {
        // Further processing required.
        if (pCurSample != nullptr)
            result = pCurSample->doEvent(event);  // Give sample a chance.
        if (result == 1)
            return VulkanAppSDL::doEvent(event);  // Pass to base class.
    }
    return result;
}


void
VulkanLoadTests::windowResized()
{
    if (pCurSample != nullptr)
        pCurSample->resize(w_width, w_height);
}


void
VulkanLoadTests::drawFrame(uint32_t msTicks)
{
    pCurSample->run(msTicks);

    VulkanAppSDL::drawFrame(msTicks);
}


void
VulkanLoadTests::getOverlayText(VulkanTextOverlay * textOverlay, float yOffset)
{
    if (enableTextOverlay) {
        textOverlay->addText("Press \"n\" or 2-finger swipe left for next sample, "
                             "\"p\" or swipe right for previous.",
                             5.0f, yOffset, VulkanTextOverlay::alignLeft);

        if (pCurSample != nullptr) {
            pCurSample->getOverlayText(textOverlay, yOffset+20);
        }
    }
}

void
VulkanLoadTests::invokeSample(Direction dir)
{
    const sampleInvocation* sampleInv;

    prepared = false;  // Prevent any more rendering.
    if (pCurSample != nullptr) {
        vkctx.queue.waitIdle(); // Wait for current rendering to finish.
        delete pCurSample;
        // Certain events can be triggered during new sample initialization
        // while pCurSample is not valid, e.g. FOCUS_LOST caused by a Vulkan
        // validation failure raising a message box. Protect against problems
        // from this by indicating there is no current sample.
        pCurSample = nullptr;
    }
    sampleInv = &siSamples[sampleIndex];

    for (;;) {
        try {
            pCurSample = sampleInv->createSample(vkctx, w_width, w_height,
                                    sampleInv->args, sBasePath);
            break;
        } catch (unsupported_ttype& e) {
            (void)e; // To quiet unused variable warnings from some compilers.
            dir == Direction::eForward ? ++sampleIndex : --sampleIndex;
            sampleInv = &siSamples[sampleIndex];
        } catch (std::exception& e) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                    sampleInv->title,
                    e.what(), NULL);
            dir == Direction::eForward ? ++sampleIndex : --sampleIndex;
            sampleInv = &siSamples[sampleIndex];
        }
    }
    prepared = true;
    setAppTitle(sampleInv->title);
}


void
VulkanLoadTests::onFPSUpdate()
{
    VulkanAppSDL::onFPSUpdate();
}

/* ------------------------------------------------------------------------ */

const VulkanLoadTests::sampleInvocation siSamples[] = {
    { Texture::create,
      "testimages/orient-down-metadata.ktx",
      "RGB8 2D + KTXOrientation down"
    },
    { Texture::create,
      "testimages/orient-up-metadata.ktx",
      "RGB8 2D + KTXOrientation up"
    },
    { Texture::create,
      "--linear-tiling testimages/orient-up-metadata.ktx",
      "RGB8 2D + KTXOrientation up with Linear Tiling"
    },
    { Texture::create,
      "testimages/rgba-reference.ktx",
      "RGBA8 2D"
    },
    { Texture::create,
        "--linear-tiling testimages/rgba-reference.ktx",
        "RGBA8 2D using Linear Tiling"
    },
    { Texture::create,
      "testimages/etc2-rgb.ktx",
      "ETC2 RGB8"
    },
    { Texture::create,
      "testimages/etc2-rgba8.ktx",
      "ETC2 RGB8A8"
    },
    { Texture::create,
      "testimages/etc2-sRGB.ktx",
      "ETC2 sRGB8"
    },
    { Texture::create,
        "testimages/etc2-sRGBa8.ktx",
        "ETC2 sRGB8a8"
    },
    { Texture::create,
        "--qcolor 0.0,0.0,0.0 testimages/pattern_02_bc2.ktx",
        "BC2 (S3TC DXT3) Compressed 2D"
    },
    { TextureMipmap::create,
      "testimages/rgb-amg-reference.ktx",
      "RGB8 + Auto Mipmap"
    },
    { TextureMipmap::create,
      "--linear-tiling testimages/rgb-amg-reference.ktx",
      "RGB8 + Auto Mipmap using Linear Tiling"
    },
    { TextureMipmap::create,
      "testimages/metalplate-amg-rgba8.ktx",
      "RGBA8 2D + Auto Mipmap"
    },
    { TextureMipmap::create,
      "--linear-tiling testimages/metalplate-amg-rgba8.ktx",
      "RGBA8 2D + Auto Mipmap using Linear Tiling"
    },
    { TextureMipmap::create,
      "testimages/not4_rgb888_srgb.ktx",
      "RGB8 2D, Row length not Multiple of 4"
    },
    { TextureMipmap::create,
      "--linear-tiling testimages/not4_rgb888_srgb.ktx",
      "RGB8 2D, Row length not Multiple of 4 using Linear Tiling"
    },
    { TextureArray::create,
        "testimages/texturearray_bc3_unorm.ktx",
        "BC2 (S3TC DXT3) Compressed Texture Array"
    },
    { TextureArray::create,
        "--linear-tiling testimages/texturearray_bc3_unorm.ktx",
        "BC2 (S3TC DXT3) Compressed Texture Array using Linear Tiling"
    },
    { TextureArray::create,
        "testimages/texturearray_astc_8x8_unorm.ktx",
        "ASTC 8x8 Compressed Texture Array"
    },
    { TextureArray::create,
        "testimages/texturearray_etc2_unorm.ktx",
        "ETC2 Compressed Texture Array"
    },
    { TextureCubemap::create,
        "testimages/cubemap_yokohama_bc3_unorm.ktx",
        "BC2 (S3TC DXT3) Compressed Cube Map"
    },
    { TextureCubemap::create,
        "testimages/cubemap_yokohama_astc_8x8_unorm.ktx",
        "ASTC Compressed Cube Map"
    },
    { TextureCubemap::create,
        "testimages/cubemap_yokohama_etc2_unorm.ktx",
        "ETC2 Compressed Cube Map"
    },
    { TextureCubemap::create,
        "--preload testimages/cubemap_yokohama_bc3_unorm.ktx",
        "BC2 (S3TC DXT3) Compressed Cube Map from Preloaded Images."
    },
    { TextureCubemap::create,
        "--preload testimages/cubemap_yokohama_astc_8x8_unorm.ktx",
        "ASTC Compressed Cube Map from Preloaded Images."
    },
    { TextureCubemap::create,
        "--preload testimages/cubemap_yokohama_etc2_unorm.ktx",
        "ETC2 Compressed Cube Map from Preloaded Images."
    },
#if 0
    { TexturedCube::create,
      "testimages/rgb-amg-reference.ktx",
      "RGB8 + Auto Mipmap"
    },
    { TexturedCube::create,
      "testimages/rgb-amg-reference.ktx",
      "RGB8 + Auto Mipmap"
    },
#endif
};

const int iNumSamples = sizeof(siSamples) / sizeof(VulkanLoadTests::sampleInvocation);

AppBaseSDL* theApp = new VulkanLoadTests(siSamples, iNumSamples,
                                     "KTX Loader Tests for Vulkan");

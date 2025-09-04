/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2015-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file
 * @~English
 *
 * @brief Implementation of app for running a set of OpenGL load tests.
 */

#include <exception>
#include <sstream>
#include <ktx.h>

#include "GLLoadTests.h"
#include "ltexceptions.h"

GLLoadTests::GLLoadTests(const sampleInvocation samples[],
                         const uint32_t numSamples,
                         const char* const name,
                         const SDL_GLProfile profile,
                         const int majorVersion,
                         const int minorVersion)
                : GLAppSDL(name, 640, 480, profile, majorVersion, minorVersion),
                  siSamples(samples), sampleIndex(numSamples)
{
    pCurSample = nullptr;
}

GLLoadTests::~GLLoadTests()
{
    delete pCurSample;
}

bool
GLLoadTests::initialize(Args& args)
{
    bool explicitlyLoadOpenGL = false;
    for (uint32_t i = 1; i < args.size(); i++) {
        if (args[i].compare("--load-gl") == 0) {
            explicitlyLoadOpenGL = true;
            args.erase(args.begin() + i);
            break;
        }
    }

    if (!GLAppSDL::initialize(args))
        return false;

    // --load-gl allows testing of explicitly loading the OpenGL pointers.
    // Default is for ktxTexture_GLUpload to implicitly load them.
    if (explicitlyLoadOpenGL)
        ktxLoadOpenGL(reinterpret_cast<PFNGLGETPROCADDRESS>(SDL_GL_GetProcAddress));

    for (auto it = args.begin() + 1; it != args.end(); it++) {
        infiles.push_back(*it);
    }
    if (infiles.size() > 0) {
        sampleIndex.setNumSamples((uint32_t)infiles.size());
    }
    // Launch the first sample.
    invokeSample(Direction::eForward);
    return true;
}


void
GLLoadTests::finalize()
{
    delete pCurSample;
    GLAppSDL::finalize();
}


bool
GLLoadTests::doEvent(SDL_Event* event)
{
    bool result = false;
    switch (event->type) {
      case SDL_EVENT_KEY_UP:
        switch (event->key.key) {
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
            result = true;
        }
        break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        // Forward to sample in case this is the start of motion.
        result = true;
        switch (event->button.button) {
          case SDL_BUTTON_LEFT:
            buttonDown.x = event->button.x;
            buttonDown.y = event->button.y;
            buttonDown.timestamp = event->button.timestamp;
            break;
          default:
            break;
        }
        break;
      case SDL_EVENT_MOUSE_BUTTON_UP:
        // Forward to sample so it doesn't get stuck in button down state.
        result = true;
        switch (event->button.button) {
          case SDL_BUTTON_LEFT:
            if (SDL_fabs(event->button.x - buttonDown.x) < 5
                && SDL_fabs(event->button.y - buttonDown.y) < 5
                && (event->button.timestamp - buttonDown.timestamp) < 100) {
                // Advance to the next sample.
                ++sampleIndex;
                invokeSample(Direction::eForward);
            }
            break;
          default:
            break;
        }
        break;
      // On macOS drop events come also when Launch Pad sends a file open event.
      case SDL_EVENT_DROP_BEGIN:
        // Opens of multiple selected files from Finder/LaunchPad come as
        // a BEGIN, COMPLETE sequence per file. Only clear infiles after a
        // suitable pause between COMPLETE and BEGIN.
        if (event->drop.timestamp - dropCompleteTime > 500) {
            infiles.clear();
        }
        break;
      case SDL_EVENT_DROP_FILE:
        infiles.push_back(event->drop.data);
        break;
      case SDL_EVENT_DROP_COMPLETE:
        if (!infiles.empty()) {
            // Guard against the drop being text.
            dropCompleteTime = event->drop.timestamp;
            sampleIndex.setNumSamples((uint32_t)infiles.size());
            invokeSample(Direction::eForward);
        }
        break;
      case SDL_EVENT_USER:
        if (event->user.code == SwipeDetector::swipeGesture) {
            // This is horrible.
            uint64_t udirection = reinterpret_cast<uint64_t>(event->user.data1);
            SwipeDetector::Direction direction =
              static_cast<SwipeDetector::Direction>(udirection);
            switch (direction) {
              case SwipeDetector::Direction::left:
                ++sampleIndex;
                invokeSample(Direction::eForward);
                break;
              case SwipeDetector::Direction::right:
                --sampleIndex;
                invokeSample(Direction::eBack);
                break;
              default:
                result = true;
            }
        } else {
            result = true;
        }
        break;
      default:
        result = swipeDetector.doEvent(event);
    }
    
    if (result) {
        // Further processing required.
        if (pCurSample != nullptr)
            result = pCurSample->doEvent(event);  // Give sample a chance.
        if (result)
            return GLAppSDL::doEvent(event);  // Pass to base class.
    }
    return result;
}


void
GLLoadTests::windowResized()
{
    if (pCurSample != nullptr)
        pCurSample->resize(w_width, w_height);
}


void
GLLoadTests::drawFrame(uint32_t msTicks)
{
    if (pCurSample != nullptr)
        pCurSample->run(msTicks);

    GLAppSDL::drawFrame(msTicks);
}

#if 0
void
GLLoadTests::getOverlayText(GLTextOverlay * textOverlay)
{
    if (enableTextOverlay && pCurSample != nullptr) {
        pCurSample->getOverlayText(textOverlay);
    }
}
#endif

void
GLLoadTests::invokeSample(Direction dir)
{
    const sampleInvocation* sampleInv = &siSamples[sampleIndex];

    delete pCurSample;
    // Certain events can be triggered during new sample initialization
    // while pCurSample is not valid, e.g. FOCUS_LOST. Protect against
    // problems from this by indicating there is no current sample.
    pCurSample = nullptr;

    uint32_t unsupportedTypeExceptions = 0;
    std::string fileTitle;
    for (;;) {
        try {
            if (infiles.size() > 0) {
                fileTitle = "Viewing file ";
                fileTitle += infiles[sampleIndex];
                pCurSample = showFile(infiles[sampleIndex]);
            } else {
                sampleInv = &siSamples[sampleIndex];
                pCurSample = sampleInv->createSample(w_width, w_height,
                                                     sampleInv->args,
                                                     sBasePath);
            }
            break;
        } catch (unsupported_ctype& e) {
            (void)e; // To quiet unused variable warnings from some compilers.
            unsupportedTypeExceptions++;
            if (unsupportedTypeExceptions == sampleIndex.getNumSamples()) {
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                    infiles.size() > 0 ? fileTitle.c_str() : sampleInv->title,
                    "None of the specified samples or files use texture types "
                    "supported on this platform.",
                    NULL);
                exit(0);
            } else {
                dir == Direction::eForward ? ++sampleIndex : --sampleIndex;
            }
        } catch (std::exception& e) {
            const SDL_MessageBoxButtonData buttons[] = {
                /* .flags, .buttonid, .text */
                { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Continue" },
                { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "Abort" },
            };
#if 0
            const SDL_MessageBoxColorScheme colorScheme = {
                { /* .colors (.r, .g, .b) */
                    /* [SDL_MESSAGEBOX_COLOR_BACKGROUND] */
                    { 255,   0,   0 },
                    /* [SDL_MESSAGEBOX_COLOR_TEXT] */
                    {   0, 255,   0 },
                    /* [SDL_MESSAGEBOX_COLOR_BUTTON_BORDER] */
                    { 255, 255,   0 },
                    /* [SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND] */
                    {   0,   0, 255 },
                    /* [SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED] */
                    { 255,   0, 255 }
                }
            };
#endif
            const SDL_MessageBoxData messageboxdata = {
                SDL_MESSAGEBOX_ERROR,                               // .flags
                NULL,                                               // .window
                infiles.size() > 0 ?
                 fileTitle.c_str() : sampleInv->title,              // .title
                e.what(),                                           // .message
                SDL_arraysize(buttons),                             // .numbuttons
                buttons,                                            // .buttons
                NULL //&colorScheme                                 // .colorScheme
            };
            int buttonid;
            if (!SDL_ShowMessageBox(&messageboxdata, &buttonid)) {
                SDL_Log("error displaying error message box");
                exit(1);
            }
            if (buttonid == 0) {
                // Continue
                dir == Direction::eForward ? ++sampleIndex : --sampleIndex;
            } else {
                // We've been told to quit or no button was pressed.
                exit(1);
            }
        }
    }
    setAppTitle(infiles.size() > 0 ? fileTitle.c_str()  : sampleInv->title);
    pCurSample->resize(w_width, w_height);
}

void
GLLoadTests::onFPSUpdate()
{
    // Using onFPSUpdate avoids rewriting the title every frame.
    //setWindowTitle(pCurSample->title);
    GLAppSDL::onFPSUpdate();
}


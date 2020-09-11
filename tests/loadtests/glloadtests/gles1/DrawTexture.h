/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2018-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file    DrawTexture.h
 * @brief   Draw textures at actual size using the DrawTexture functions
 *          from OES_draw_texture.
 *
 * @author  Mark Callow
 */

#ifndef DRAW_TEXTURE_H
#define DRAW_TEXTURE_H

#include <GLES/gl.h>
#include <GLES/glext.h>

#include "LoadTestSample.h"

class DrawTexture : public LoadTestSample {
  public:
    DrawTexture(uint32_t width, uint32_t height,
                const char* const szArgs,
                const std::string sBasePath);
    ~DrawTexture();

    virtual void resize(uint32_t width, uint32_t height);
    virtual void run(uint32_t msTicks);

    //virtual void getOverlayText(GLTextOverlay *textOverlay);

    static LoadTestSample*
    create(uint32_t width, uint32_t height,
           const char* const szArgs, const std::string sBasePath);

  protected:
     PFNGLDRAWTEXSOESPROC glDrawTexsOES;
     PFNGLDRAWTEXIOESPROC glDrawTexiOES;
     PFNGLDRAWTEXXOESPROC glDrawTexxOES;
     PFNGLDRAWTEXFOESPROC glDrawTexfOES;
     PFNGLDRAWTEXSVOESPROC glDrawTexsvOES;
     PFNGLDRAWTEXIVOESPROC glDrawTexivOES;
     PFNGLDRAWTEXXVOESPROC glDrawTexxvOES;
     PFNGLDRAWTEXFVOESPROC glDrawTexfvOES;

    uint32_t uWidth;
    uint32_t uHeight;

    uint32_t uTexWidth;
    uint32_t uTexHeight;

    glm::mat4 framePMatrix;

    GLuint gnTexture;

    bool bNpotSupported;

    bool bInitialized;
};

#endif /* DRAW_TEXTURE_H */

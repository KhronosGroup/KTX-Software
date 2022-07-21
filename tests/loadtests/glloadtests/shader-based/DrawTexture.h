/* -*- tab-iWidth: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2018-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file    DrawTexture.h
 * @brief   Definition of texture loading test using glDrawTexture.
 *
 * @author  Mark Callow
 */

#ifndef DRAW_TEXTURE_H
#define DRAW_TEXTURE_H

#include "GL3LoadTestSample.h"

class DrawTexture : public GL3LoadTestSample {
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
    void processArgs(std::string sArgs);

    int preloadImages = 0;

    uint32_t uWidth;
    uint32_t uHeight;

    uint32_t uTexWidth;
    uint32_t uTexHeight;

    glm::mat4 frameMvMatrix;
    glm::mat4 quadMvMatrix;
    glm::mat4 pMatrix;

    GLuint gnTexture;
    GLuint gnTexProg;
    GLuint gnColProg;

#define FRAME 0
#define QUAD  1
    GLuint gnVaos[2];
    GLuint gnVbo;

    GLint gulMvMatrixLocTP;
    GLint gulPMatrixLocTP;
    GLint gulSamplerLocTP;
    GLint gulMvMatrixLocCP;
    GLint gulPMatrixLocCP;

    bool bInitialized;
    ktx_transcode_fmt_e transcodeTarget;
};

#endif /* DRAW_TEXTURE_H */

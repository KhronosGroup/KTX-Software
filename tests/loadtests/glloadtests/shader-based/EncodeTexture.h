/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2018-2022 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file    EncodeTexture.h
 * @brief   GLLoadTestSample derived class for encoding a texture and texturing a cube with it.
 *
 * @author Mark Callow, www.edgewise-consulting.com.
 */

#ifndef ENCODE_TEXTURE_H
#define ENCODE_TEXTURE_H

#include "GL3LoadTestSample.h"

class EncodeTexture : public GL3LoadTestSample {
  public:
    EncodeTexture(uint32_t width, uint32_t height,
                const char* const szArgs,
                const std::string sBasePath);
    ~EncodeTexture();

    virtual void resize(uint32_t width, uint32_t height);
    virtual void run(uint32_t msTicks);

    //virtual void getOverlayText(GLTextOverlay *textOverlay);

    static LoadTestSample*
    create(uint32_t width, uint32_t height,
           const char* const szArgs, const std::string sBasePath);

  protected:
    void processArgs(std::string sArgs);

    GLuint gnTexture;
    GLuint gnTexProg;

    GLuint gnVao;
    GLuint gnVbo[2];

    GLint gulMvMatrixLocTP;
    GLint gulPMatrixLocTP;
    GLint gulSamplerLocTP;

    bool bInitialized;
    ktx_transcode_fmt_e transcodeTarget;
    enum encode_fmt_e { EF_ASTC = 1, EF_ETC1S = 2, EF_UASTC = 3 };
    encode_fmt_e encodeTarget;
};

#endif /* ENCODE_TEXTURE_H */

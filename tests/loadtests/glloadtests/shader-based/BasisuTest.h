/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2018-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file    BasisuTest.h
 * @brief   GLLoadTestSample derived class for drawing a textured cube.
 *
 * @author Mark Callow, www.edgewise-consulting.com.
 */

#ifndef BASISU_TEST_H
#define BASISU_TEST_H

#include "GL3LoadTestSample.h"

class BasisuTest : public GL3LoadTestSample {
  public:
    BasisuTest(uint32_t width, uint32_t height,
                const char* const szArgs,
                const std::string sBasePath);
    ~BasisuTest();

    virtual void resize(uint32_t width, uint32_t height);
    virtual void run(uint32_t msTicks);

    //virtual void getOverlayText(GLTextOverlay *textOverlay);

    static LoadTestSample*
    create(uint32_t width, uint32_t height,
           const char* const szArgs, const std::string sBasePath);

  protected:
    GLuint gnTexture;
    GLuint gnTexProg;

    GLuint gnVao;
    GLuint gnVbo[2];

    GLint gulMvMatrixLocTP;
    GLint gulPMatrixLocTP;
    GLint gulSamplerLocTP;

    bool bInitialized;
};

#endif /* BASISU_TEST_H */

/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2018 Mark Callow.
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

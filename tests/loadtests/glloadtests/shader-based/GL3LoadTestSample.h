/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2017 - 2018 Mark Callow, <khronos at callow dot im>.
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

#ifndef GL3_LOAD_TEST_SAMPLE_H
#define GL3_LOAD_TEST_SAMPLE_H

#include "LoadTestSample.h"
#include "mygl.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))

class GL3LoadTestSample : public LoadTestSample {
  public:
    typedef uint64_t ticks_t;
    GL3LoadTestSample(uint32_t width, uint32_t height,
                     const char* const szArgs,
                     const std::string sBasePath)
           : LoadTestSample(width, height, sBasePath)
    {
    }

    virtual ~GL3LoadTestSample() { }   
    virtual void resize(uint32_t width, uint32_t height) = 0;
    virtual void run(uint32_t msTicks) = 0;

    //virtual void getOverlayText(TextOverlay *textOverlay) { };

    typedef LoadTestSample* (*PFN_create)(uint32_t width, uint32_t height,
                                          const char* const szArgs,
                                          const std::string sBasePath);

  protected:
    virtual void keyPressed(uint32_t keyCode) { }
    virtual void viewChanged() { }
    
    static void makeShader(GLenum type, const GLchar* const source,
                           GLuint* shader);
    static void makeProgram(GLuint vs, GLuint fs, GLuint* program);
};

#endif /* GL3_LOAD_TEST_SAMPLE_H */

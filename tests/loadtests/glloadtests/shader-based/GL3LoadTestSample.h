/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow, <khronos at callow dot im>.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GL3_LOAD_TEST_SAMPLE_H
#define GL3_LOAD_TEST_SAMPLE_H

#include <ktx.h>
#include "LoadTestSample.h"
#include "mygl.h"
#include "utils/GLMeshLoader.hpp"

#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))

class GL3LoadTestSample : public LoadTestSample {
  public:
    typedef uint64_t ticks_t;
    GL3LoadTestSample(uint32_t width, uint32_t height,
                     const char* const /*szArgs*/,
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
    using ShaderSource = std::vector<const GLchar*>;

    virtual void keyPressed(uint32_t /*keyCode*/) { }
    virtual void viewChanged() { }

    static bool contextSupportsSwizzle();

    std::string ktxfilename;
    int externalFile = 0;

    struct compressedTexFeatures {
        bool astc_ldr;
        bool astc_hdr;
        bool bc6h;
        bool bc7;
        bool etc1;
        bool etc2;
        bool bc3;
        bool pvrtc1;
        bool pvrtc_srgb;
        bool pvrtc2;
        bool rgtc;
    };

    static void determineCompressedTexFeatures(compressedTexFeatures& features);
    static GLint framebufferColorEncoding();
    void loadMesh(std::string filename, glMeshLoader::MeshBuffer& meshBuffer,
                  std::vector<glMeshLoader::VertexLayout> vertexLayout,
                  float scale);
    static void makeShader(GLenum type, ShaderSource& source, GLuint* shader);
    static void makeShader(GLenum type, const GLchar* const source,
                           GLuint* shader);
    static void makeProgram(GLuint vs, GLuint fs, GLuint* program);
};

#endif /* GL3_LOAD_TEST_SAMPLE_H */

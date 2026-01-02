/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2018-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file    TexturedCube.cpp
 * @brief    Draw a textured cube.
 */

#if defined(_WIN32)
  #if _MSC_VER < 1900
    #define snprintf _snprintf
  #endif
  #define _CRT_SECURE_NO_WARNINGS
#endif

#include <iomanip>
#include <sstream>
#include <ktx.h>
#include "disable_glm_warnings.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "reenable_warnings.h"

#include "TexturedCube.h"
#include "cube.h"


#if defined(_WIN32)
#define snprintf _snprintf
#endif

/* ------------------------------------------------------------------------- */

LoadTestSample*
TexturedCube::create(uint32_t width, uint32_t height,
                    const char* const szArgs,
                    const std::string sBasePath)
{
    return new TexturedCube(width, height, szArgs, sBasePath);
}

TexturedCube::TexturedCube(uint32_t width, uint32_t height,
                         const char* const szArgs,
                         const std::string sBasePath)
        : LoadTestSample(width, height, sBasePath)
{
    const GLchar*  szExtensions = (const GLchar*)glGetString(GL_EXTENSIONS);
    const char* filename;
    std::string pathname;
    GLuint gnTexture = 0;
    GLenum target;
    GLenum glerror;
    GLboolean npotSupported, npotTexture;
    ktxTexture* kTexture;
    KTX_error_code ktxresult;

    if (strstr(szExtensions, "OES_texture_npot") != NULL)
        npotSupported = GL_TRUE;
    else
        npotSupported = GL_FALSE;
    
    if ((filename = strchr(szArgs, ' ')) != NULL) {
        npotTexture = GL_FALSE;
        if (!strncmp(szArgs, "--npot ", 7)) {
            npotTexture = GL_TRUE;
#if defined(DEBUG)
        } else {
            assert(0); /* Unknown argument in sampleInvocations */
#endif
        }
    } else {
        filename = szArgs;
        npotTexture = GL_FALSE;
    }
    
    if (npotTexture  && !npotSupported) {
        /* Load error texture. */
        filename = "no-npot.ktx";
    }
    pathname = getAssetPath() + filename;

    ktxresult = ktxTexture_CreateFromNamedFile(pathname.c_str(),
                                               KTX_TEXTURE_CREATE_NO_FLAGS,
                                               &kTexture);
    if (KTX_SUCCESS != ktxresult) {
        std::stringstream message;

        message << "Creation of ktxTexture from \"" << pathname
                << "\" failed: " << ktxErrorString(ktxresult);
        throw std::runtime_error(message.str());
    }
    ktxresult = ktxTexture_GLUpload(kTexture, &gnTexture, &target, &glerror);

    if (KTX_SUCCESS == ktxresult) {
        if (target != GL_TEXTURE_2D) {
            /* Can only draw 2D textures */
            glDeleteTextures(1, &gnTexture);
            return;
        }
        glEnable(target);

        if (kTexture->numLevels > 1)
            // Enable bilinear mipmapping.
            // TO DO: application can consider inserting a key,value pair in
            // the KTX file that indicates what type of filtering to use.
            glTexParameteri(target,
                            GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        else
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

        ktxTexture_Destroy(kTexture);
    } else {
        std::stringstream message;

        message << "Load of texture from \"" << pathname << "\" failed: ";
        if (ktxresult == KTX_GL_ERROR) {
            message << std::showbase << "GL error " << std::hex << glerror
                    << "occurred.";
        } else {
            message << ktxErrorString(ktxresult);
        }
        throw std::runtime_error(message.str());
    }

    /* By default dithering is enabled. Dithering does not provide visual
     * improvement in this sample so disable it to improve performance. 
     */
    glDisable(GL_DITHER);

    glEnable(GL_CULL_FACE);
    glClearColor(0.2f,0.3f,0.4f,1.0f);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer(3, GL_FLOAT, 0, cube_face);
    glColorPointer(4, GL_FLOAT, 0, cube_color);
    glTexCoordPointer(2, GL_FLOAT, 0, cube_texture);
}

TexturedCube::~TexturedCube()
{
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DITHER);
    glDisable(GL_CULL_FACE);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    assert(GL_NO_ERROR == glGetError());
}

void
TexturedCube::resize(uint32_t uWidth, uint32_t uHeight)
{
    glm::mat4 matProj;
    glViewport(0, 0, uWidth, uHeight);

    glMatrixMode( GL_PROJECTION );
    matProj = glm::perspective(glm::radians(45.f),
                               uWidth / (float)uHeight,
                               1.f, 100.f);
    glLoadIdentity();
    glLoadMatrixf(glm::value_ptr(matProj));

    glMatrixMode( GL_MODELVIEW );
    assert(GL_NO_ERROR == glGetError());
}

void
TexturedCube::run(uint32_t msTicks)
{
    /* Setup the view matrix : just turn around the cube. */
    const float fDistance = 5.0f;
    glm::vec3 eye((float)cos( msTicks*0.001f ) * fDistance,
                  (float)sin( msTicks*0.0007f ) * fDistance,
                  (float)sin( msTicks*0.001f ) * fDistance);
    glm::vec3 look(0.,0.,0.);
    glm::vec3 up(0.,1.,0.);
    glm::mat4 matView = glm::lookAt( eye, look, up );

    glLoadIdentity();
    glLoadMatrixf(glm::value_ptr(matView));

    /* Draw */
    glClear( GL_COLOR_BUFFER_BIT );

    glDrawElements(GL_TRIANGLES, CUBE_NUM_INDICES,
                   GL_UNSIGNED_SHORT, cube_index_buffer);

    assert(GL_NO_ERROR == glGetError());
}

/* ------------------------------------------------------------------------- */


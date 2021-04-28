/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 sts expandtab: */

/*
 * Copyright 2018-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file    TexturedCube.cpp
 * @brief    Draw a textured cube.
 *
 * @author Mark Callow, www.edgewise-consulting.com.
 */

#if defined(_WIN32)
  #if _MSC_VER < 1900
    #define snprintf _snprintf
  #endif
  #define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <sstream>
#include <ktx.h>
#include "disable_glm_warnings.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "reenable_warnings.h"

#include "TexturedCube.h"
#include "cube.h"

/* ------------------------------------------------------------------------- */

extern const GLchar* pszVs;
extern const GLchar *pszDecalFs, *pszDecalSrgbEncodeFs;

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
        : GL3LoadTestSample(width, height, szArgs, sBasePath)
{
    std::string filename;
    GLenum target;
    GLenum glerror;
    GLuint gnDecalFs, gnVs;
    GLsizeiptr offset;
    ktxTexture* kTexture;
    KTX_error_code ktxresult;

    bInitialized = GL_FALSE;
    gnTexture = 0;

    filename = getAssetPath() + szArgs;
    ktxresult = ktxTexture_CreateFromNamedFile(filename.c_str(),
                                               KTX_TEXTURE_CREATE_NO_FLAGS,
                                               &kTexture);
    if (KTX_SUCCESS != ktxresult) {
        std::stringstream message;

        message << "Creation of ktxTexture from \"" << filename
                << "\" failed: " << ktxErrorString(ktxresult);
        throw std::runtime_error(message.str());
    }
    ktxresult = ktxTexture_GLUpload(kTexture, &gnTexture, &target, &glerror);

    if (KTX_SUCCESS == ktxresult) {
        if (target != GL_TEXTURE_2D) {
            /* Can only draw 2D textures */
            std::stringstream message;

            glDeleteTextures(1, &gnTexture);

            message << "App can only draw 2D textures.";
            throw std::runtime_error(message.str());
        }

        if (kTexture->numLevels > 1)
            // Enable bilinear mipmapping.
            // TO DO: application can consider inserting a key,value pair in
            // the KTX file that indicates what type of filtering to use.
            glTexParameteri(target,
                            GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        else
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        ktxTexture_Destroy(kTexture);

        assert(GL_NO_ERROR == glGetError());
    } else {
        std::stringstream message;

        message << "Load of texture from \"" << filename << "\" failed: ";
        if (ktxresult == KTX_GL_ERROR) {
            message << std::showbase << "GL error " << std::hex << glerror
                    << " occurred.";
        } else {
            message << ktxErrorString(ktxresult);
        }
        throw std::runtime_error(message.str());
    }

    // By default dithering is enabled. Dithering does not provide visual
    // improvement in this sample so disable it to improve performance.
    glDisable(GL_DITHER);

    glEnable(GL_CULL_FACE);
    glClearColor(0.2f,0.3f,0.4f,1.0f);

    // Create a VAO and bind it.
    glGenVertexArrays(1, &gnVao);
    glBindVertexArray(gnVao);

    // Must have vertex data in buffer objects to use VAO's on ES3/GL Core
    glGenBuffers(2, gnVbo);
    glBindBuffer(GL_ARRAY_BUFFER, gnVbo[0]);
    // Must be done after the VAO is bound
    // WebGL requires different buffers for data and indices.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gnVbo[1]);

    // Create the buffer data store. 
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(cube_face) + sizeof(cube_color) + sizeof(cube_texture)
                 + sizeof(cube_normal), NULL, GL_STATIC_DRAW);

    // Interleave data copying and attrib pointer setup so offset is only
    // computed once.
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    offset = 0;
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(cube_face), cube_face);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)offset);
    offset += sizeof(cube_face);
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(cube_color), cube_color);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)offset);
    offset += sizeof(cube_color);
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(cube_texture), cube_texture);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)offset);
    offset += sizeof(cube_texture);
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(cube_normal), cube_normal);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)offset);
    offset += sizeof(cube_normal);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_index_buffer),
                 cube_index_buffer, GL_STATIC_DRAW);

    const GLchar* actualDecalFs;
    if (framebufferColorEncoding() == GL_LINEAR) {
        actualDecalFs = pszDecalSrgbEncodeFs;
    } else {
        actualDecalFs = pszDecalFs;
    }
    try {
        makeShader(GL_VERTEX_SHADER, pszVs, &gnVs);
        makeShader(GL_FRAGMENT_SHADER, actualDecalFs, &gnDecalFs);
        makeProgram(gnVs, gnDecalFs, &gnTexProg);
        } catch (std::exception& e) {
        (void)e; // To quiet unused variable warnings from some compilers.
        throw;
    }
    try {
        makeShader(GL_VERTEX_SHADER, pszVs, &gnVs);
        makeShader(GL_FRAGMENT_SHADER, pszDecalFs, &gnDecalFs);
        makeProgram(gnVs, gnDecalFs, &gnTexProg);
        } catch (std::exception& e) {
        (void)e; // To quiet unused variable warnings from some compilers.
        throw;
    }
    gulMvMatrixLocTP = glGetUniformLocation(gnTexProg, "mvmatrix");
    gulPMatrixLocTP = glGetUniformLocation(gnTexProg, "pmatrix");
    gulSamplerLocTP = glGetUniformLocation(gnTexProg, "sampler");
    glUseProgram(gnTexProg);
    // We're using the default texture unit 0
    glUniform1i(gulSamplerLocTP, 0);
    glDeleteShader(gnVs);
    glDeleteShader(gnDecalFs);

    assert(GL_NO_ERROR == glGetError());
    bInitialized = GL_TRUE;
}

TexturedCube::~TexturedCube()
{
    glEnable(GL_DITHER);
    glEnable(GL_CULL_FACE);
    if (bInitialized) {
        glUseProgram(0);
        glDeleteTextures(1, &gnTexture);
        glDeleteProgram(gnTexProg);
        glDeleteBuffers(2, gnVbo);
        glDeleteVertexArrays(1, &gnVao);
    }
    assert(GL_NO_ERROR == glGetError());
}


void
TexturedCube::resize(uint32_t uWidth, uint32_t uHeight)
{
    glm::mat4 matProj;

    glViewport(0, 0, uWidth, uHeight);

    matProj = glm::perspective(glm::radians(45.f),
                               uWidth / (float)uHeight,
                               1.f, 100.f);
    glUniformMatrix4fv(gulPMatrixLocTP, 1, GL_FALSE, glm::value_ptr(matProj));
}

void
TexturedCube::run(uint32_t msTicks)
{
    // Setup the view matrix : just turn around the cube.
    const float fDistance = 5.0f;
    glm::vec3 eye((float)cos( msTicks*0.001f ) * fDistance,
                  (float)sin( msTicks*0.0007f ) * fDistance,
                  (float)sin( msTicks*0.001f ) * fDistance);
    glm::vec3 look(0.,0.,0.);
    glm::vec3 up(0.,1.,0.);
    glm::mat4 matView = glm::lookAt( eye, look, up );

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUniformMatrix4fv(gulMvMatrixLocTP, 1, GL_FALSE, glm::value_ptr(matView));

    glDrawElements(GL_TRIANGLES, CUBE_NUM_INDICES, GL_UNSIGNED_SHORT, 0);

    assert(GL_NO_ERROR == glGetError());
}

/* ------------------------------------------------------------------------- */


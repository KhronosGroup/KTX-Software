/* -*- tab-iWidth: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2018-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file    DrawTexture.cpp
 * @brief   Tests texture loading by using glDrawTexture to draw the texture.
 *
 * @author  Mark Callow
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

#include "DrawTexture.h"
#include "GLTextureTranscoder.hpp"
#include "TranscodeTargetStrToFmt.h"
#include "frame.h"
#include "quad.h"
#include "argparser.h"
#include "ltexceptions.h"

#if !defined(GL_TEXTURE_1D)
  #define GL_TEXTURE_1D                     0x0DE0
#endif

/* ------------------------------------------------------------------------- */

extern const GLchar* pszVs;
extern const GLchar *pszDecalFs, *pszDecalSrgbEncodeFs;
extern const GLchar *pszColorFs, *pszColorSrgbEncodeFs;

/* ------------------------------------------------------------------------- */

LoadTestSample*
DrawTexture::create(uint32_t width, uint32_t height,
                    const char* const szArgs,
                    const std::string sBasePath)
{
    return new DrawTexture(width, height, szArgs, sBasePath);
}

DrawTexture::DrawTexture(uint32_t width, uint32_t height,
                         const char* const szArgs,
                         const std::string sBasePath)
        : GL3LoadTestSample(width, height, szArgs, sBasePath)
{
    GLfloat* pfQuadTexCoords = quad_texture;
    GLfloat  fTmpTexCoords[sizeof(quad_texture)/sizeof(GLfloat)];
    GLenum target;
    GLenum glerror;
    GLint sign_s = 1, sign_t = 1;
    GLint i;
    GLuint gnColorFs, gnDecalFs, gnVs;
    GLsizeiptr offset;
    ktxTexture* kTexture;
    KTX_error_code ktxresult;

    bInitialized = false;
    transcodeTarget = KTX_TTF_NOSELECTION;
    gnTexture = 0;

    processArgs(szArgs);

    std::string ktxfilepath = externalFile ? ktxfilename
                                           : getAssetPath() + ktxfilename;
    ktxresult = ktxTexture_CreateFromNamedFile(ktxfilepath.c_str(),
                        preloadImages ? KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT
                                      : KTX_TEXTURE_CREATE_NO_FLAGS,
                        &kTexture);
    if (KTX_SUCCESS != ktxresult) {
        std::stringstream message;

        message << "Creation of ktxTexture from \"" << ktxfilepath
                << "\" failed: " << ktxErrorString(ktxresult);
        throw std::runtime_error(message.str());
    }

    if (ktxTexture_NeedsTranscoding(kTexture)) {
        TextureTranscoder tc;
        tc.transcode((ktxTexture2*)kTexture, transcodeTarget);
    }

    ktxresult = ktxTexture_GLUpload(kTexture, &gnTexture, &target, &glerror);

    if (KTX_SUCCESS == ktxresult) {
        // GLUpload won't set target to GL_TEXTURE_1D not supported by context.
        if (target != GL_TEXTURE_1D && target != GL_TEXTURE_2D) {
            /* Can only draw 1D & 2D textures */
            std::stringstream message;

            glDeleteTextures(1, &gnTexture);
            message << "DrawTexture supports only 1D & 2D textures. \""
                    << ktxfilename << "\" is not one of these.";
            throw std::runtime_error(message.str());
        }

        if (kTexture->orientation.x == KTX_ORIENT_X_LEFT)
            sign_s = -1;
        if (kTexture->orientation.y == KTX_ORIENT_Y_DOWN)
            sign_t = -1;

        if (sign_s < 0 || sign_t < 0) {
            // Transform the texture coordinates to get correct image
            // orientation.
            int iNumCoords = sizeof(quad_texture) / sizeof(float);
            for (i = 0; i < iNumCoords; i++) {
                fTmpTexCoords[i] = quad_texture[i];
                if (i & 1) { // odd, i.e. a y coordinate
                    if (sign_t < 1) {
                        fTmpTexCoords[i] = fTmpTexCoords[i] * -1 + 1;
                    }
                } else { // an x coordinate
                    if (sign_s < 1) {
                        fTmpTexCoords[i] = fTmpTexCoords[i] * -1 + 1;
                    }
                }
            }
            pfQuadTexCoords = fTmpTexCoords;
        }

        uTexWidth = kTexture->baseWidth;
        uTexHeight = kTexture->baseHeight;

        if (kTexture->numLevels > 1)
            // Enable bilinear mipmapping.
            // TO DO: application can consider inserting a key,value pair in
            // the KTX file that indicates what type of filtering to use.
            glTexParameteri(target,
                            GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        else
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        ktx_uint32_t swizzleLen;
        char* swizzleStr;
        ktxresult = ktxHashList_FindValue(&kTexture->kvDataHead, KTX_SWIZZLE_KEY,
                                          &swizzleLen, (void**)&swizzleStr);
        if (ktxresult == KTX_SUCCESS && swizzleLen == 5) {
            if (contextSupportsSwizzle()) {
                GLint swizzle[4];
                swizzle[0] = swizzleStr[0] == 'r' ? GL_RED
                          : swizzleStr[0] == 'g' ? GL_GREEN
                          : swizzleStr[0] == 'b' ? GL_BLUE
                          : swizzleStr[0] == 'a' ? GL_ALPHA
                          : swizzleStr[0] == '0' ? GL_ZERO
                          : GL_ONE;
                swizzle[1] = swizzleStr[1] == 'r' ? GL_RED
                          : swizzleStr[1] == 'g' ? GL_GREEN
                          : swizzleStr[1] == 'b' ? GL_BLUE
                          : swizzleStr[1] == 'a' ? GL_ALPHA
                          : swizzleStr[1] == '0' ? GL_ZERO
                          : GL_ONE;
                swizzle[2] = swizzleStr[2] == 'r' ? GL_RED
                          : swizzleStr[2] == 'g' ? GL_GREEN
                          : swizzleStr[2] == 'b' ? GL_BLUE
                          : swizzleStr[2] == 'a' ? GL_ALPHA
                          : swizzleStr[2] == '2' ? GL_ZERO
                          : GL_ONE;
                swizzle[3] = swizzleStr[3] == 'r' ? GL_RED
                          : swizzleStr[3] == 'g' ? GL_GREEN
                          : swizzleStr[3] == 'b' ? GL_BLUE
                          : swizzleStr[3] == 'a' ? GL_ALPHA
                          : swizzleStr[3] == '3' ? GL_ZERO
                          : GL_ONE;
                glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, swizzle[0]);
                glTexParameteri(target, GL_TEXTURE_SWIZZLE_G, swizzle[1]);
                glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, swizzle[2]);
                glTexParameteri(target, GL_TEXTURE_SWIZZLE_A, swizzle[3]);
            } else {
                std::stringstream message;
                message << "Input file has swizzle metadata but the "
                        << "GL context does not support swizzling.";
                // I have absolutely no idea why the following line makes clang
                // raise an error about no matching conversion from
                // std::__1::basic_stringstream to unsupported_ttype
                // so I've resorted to using a temporary variable.
                //throw(unsupported_ttype(message.str());
                std::string msg = message.str();
                throw(unsupported_ttype(msg));
            }
        }

        assert(GL_NO_ERROR == glGetError());

        ktxTexture_Destroy(kTexture);
    } else {
        std::stringstream message;

        message << "ktxTexture_GLUpload failed: ";
        if (ktxresult != KTX_GL_ERROR) {
             message << ktxErrorString(ktxresult);
             throw std::runtime_error(message.str());
        } else if (kTexture->isCompressed
                   // Emscripten/WebGL returns INVALID_VALUE for unsupported
                   // ETC formats.
                   && (glerror == GL_INVALID_ENUM || glerror == GL_INVALID_VALUE)) {
             throw unsupported_ctype();
        } else {
             message << std::showbase << "GL error " << std::hex << glerror
                    << " occurred.";
             throw std::runtime_error(message.str());
        }
    }

    glClearColor(0.4f, 0.4f, 0.5f, 1.0f);

    // Must have vertex data in buffer objects to use VAO's on ES3/GL Core
    glGenBuffers(1, &gnVbo);
    glBindBuffer(GL_ARRAY_BUFFER, gnVbo);

    // Create the buffer data store
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(frame_position) + sizeof(frame_color)
                 + sizeof(quad_position) + sizeof(quad_color)
                 + sizeof(quad_texture),
                 NULL, GL_STATIC_DRAW);

    glGenVertexArrays(2, gnVaos);

    // Interleave data copying and attrib pointer setup so offset is only
    // computed once.

    // Setup VAO and buffer the data for frame
    glBindVertexArray(gnVaos[FRAME]);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    offset = 0;
    glBufferSubData(GL_ARRAY_BUFFER, offset,
                    sizeof(frame_position), frame_position);
    glVertexAttribPointer(0, 3, GL_BYTE, GL_FALSE, 0, (GLvoid*)offset);
    offset += sizeof(frame_position);
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(frame_color), frame_color);
    glVertexAttribPointer(1, 3, GL_BYTE, GL_FALSE, 0, (GLvoid*)offset);
    offset += sizeof(frame_color);

    // Setup VAO for quad
    glBindVertexArray(gnVaos[QUAD]);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glBufferSubData(GL_ARRAY_BUFFER, offset,
                    sizeof(quad_position), quad_position);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)offset);
    offset += sizeof(quad_position);
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(quad_color), quad_color);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)offset);
    offset += sizeof(quad_color);
    glBufferSubData(GL_ARRAY_BUFFER, offset,
                    sizeof(quad_texture), pfQuadTexCoords);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)offset);

    glBindVertexArray(0);

    const GLchar *actualColorFs, *actualDecalFs;
    if (framebufferColorEncoding() == GL_LINEAR) {
        actualColorFs = pszColorSrgbEncodeFs;
        actualDecalFs = pszDecalSrgbEncodeFs;
    } else {
        actualColorFs = pszColorFs;
        actualDecalFs = pszDecalFs;
    }
    try {
        makeShader(GL_VERTEX_SHADER, pszVs, &gnVs);
        makeShader(GL_FRAGMENT_SHADER, actualColorFs, &gnColorFs);
        makeProgram(gnVs, gnColorFs, &gnColProg);
        gulMvMatrixLocCP = glGetUniformLocation(gnColProg, "mvmatrix");
        gulPMatrixLocCP = glGetUniformLocation(gnColProg, "pmatrix");
        makeShader(GL_FRAGMENT_SHADER, actualDecalFs, &gnDecalFs);
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
    glDeleteShader(gnColorFs);
    glDeleteShader(gnDecalFs);

    // Set the quad's mv matrix to scale by the texture size.
    // With the pixel-mapping ortho projection set below, the texture will
    // be rendered at actual size just like DrawTex*OES.
    quadMvMatrix = glm::scale(glm::mat4(),
                              glm::vec3((float)uTexWidth / 2,
                                        (float)uTexHeight / 2,
                                        1));

    assert(GL_NO_ERROR == glGetError());
    bInitialized = true;
}

/* ------------------------------------------------------------------------- */

DrawTexture::~DrawTexture()
{
    if (bInitialized) {
        // A bug in the PVR SDK 3.1 emulator causes the
        // glDeleteProgram(gnColProg) below to raise an INVALID_VALUE error
        // if the following glUseProgram(0) has been executed. Strangely the
        // equivalent line in TexturedCube.cpp, where only 1 program is used,
        // does not raise an error.
        glUseProgram(0);
        glDeleteTextures(1, &gnTexture);
        glDeleteProgram(gnTexProg);
        glDeleteProgram(gnColProg);
        glDeleteBuffers(1, &gnVbo);
        glDeleteVertexArrays(2, gnVaos);
    }
    assert(GL_NO_ERROR == glGetError());
}

/* ------------------------------------------------------------------------- */

void
DrawTexture::processArgs(std::string sArgs)
{
    // Options descriptor
    struct argparser::option longopts[] = {
      {"external",         argparser::option::no_argument, &externalFile, 1},
      {"preload",          argparser::option::no_argument, &preloadImages, 1},
      {"transcode-target", argparser::option::required_argument, nullptr, 2},
      {NULL,               argparser::option::no_argument,       nullptr, 0}
    };

    argvector argv(sArgs);
    argparser ap(argv);

    int ch;
    while ((ch = ap.getopt(nullptr, longopts, nullptr)) != -1) {
        switch (ch) {
          case 0: break;
          case 2:
            transcodeTarget = TranscodeTargetStrToFmt(ap.optarg);
            break;
          default: assert(false); // Error in args in sample table.
        }
    }
    assert(ap.optind < argv.size());
    ktxfilename = argv[ap.optind];
}

/* ------------------------------------------------------------------------- */

void
DrawTexture::resize(uint32_t uNewWidth, uint32_t uNewHeight)
{

    glViewport(0, 0, uNewWidth, uNewHeight);
    this->uWidth = uNewWidth;
    this->uHeight = uNewHeight;

    // Set up an orthographic projection where 1 = 1 pixel, and 0,0,0
    // is at the center of the window.
    //atSetOrthoZeroAtCenterMatrix(fPMatrix,
    //                             0.0f, (float)iWidth,
    //                           0.0f, (float)iHeight,
    //                           -1.0f, 1.0f);
    // Set up an orthographic projection where 1 = 1 pixel
    pMatrix = glm::ortho(0.f, (float)uWidth, 0.f, (float)uHeight);
    // Move (0,0,0) to the center of the window.
    pMatrix *= glm::translate(glm::mat4(),
                              glm::vec3((float)uWidth/2, (float)uHeight/2, 0));

    // Scale the frame to fill the viewport. To guarantee its lines
    // appear we need to inset them by half-a-pixel hence the -1.
    // [Lines at the edges of the clip volume may or may not appear
    // depending on the OpenGL ES implementation. This is because
    // (a) the edges are on the points of the diamonds of the diamond
    //     exit rule and slight precision errors can easily push the
    //     lines outside the diamonds.
    // (b) the specification allows lines to be up to 1 pixel either
    //     side of the exact position.]
    frameMvMatrix = glm::scale(glm::mat4(),
                               glm::vec3((float)(uWidth - 1)/2,
                                         (float)(uHeight - 1)/2,
                                         1));
}

/* ------------------------------------------------------------------------- */

void
DrawTexture::run(uint32_t /*msTicks*/)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(gnVaos[FRAME]);
    glUseProgram(gnColProg);
    glUniformMatrix4fv(gulMvMatrixLocCP, 1, GL_FALSE,
                       glm::value_ptr(frameMvMatrix));
    glUniformMatrix4fv(gulPMatrixLocCP, 1, GL_FALSE, glm::value_ptr(pMatrix));
    glDrawArrays(GL_LINE_LOOP, 0, 4);

    glBindVertexArray(gnVaos[QUAD]);
    glUseProgram(gnTexProg);
    glUniformMatrix4fv(gulMvMatrixLocTP, 1, GL_FALSE,
                       glm::value_ptr(quadMvMatrix));
    glUniformMatrix4fv(gulPMatrixLocTP, 1, GL_FALSE, glm::value_ptr(pMatrix));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    assert(GL_NO_ERROR == glGetError());
}

/* ------------------------------------------------------------------------- */

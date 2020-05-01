/* -*- tab-iWidth: 4; -*- */
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
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "DrawTexture.h"
#include "GLTextureTranscoder.hpp"
#include "frame.h"
#include "quad.h"
#include "argparser.h"
#include "ltexceptions.h"

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

    ktxresult = ktxTexture_CreateFromNamedFile(
                         (getAssetPath() + filename).c_str(),
                         preloadImages ? KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT
                                       : KTX_TEXTURE_CREATE_NO_FLAGS,
                                               &kTexture);
    if (KTX_SUCCESS != ktxresult) {
        std::stringstream message;

        message << "Creation of ktxTexture from \"" << filename
                << "\" failed: " << ktxErrorString(ktxresult);
        throw std::runtime_error(message.str());
    }

    if (ktxTexture_NeedsTranscoding(kTexture)) {
        TextureTranscoder tc;
        tc.transcode((ktxTexture2*)kTexture, transcodeTarget);
    }

    ktxresult = ktxTexture_GLUpload(kTexture, &gnTexture, &target, &glerror);

    if (KTX_SUCCESS == ktxresult) {
        if (target != GL_TEXTURE_2D) {
            /* Can only draw 2D textures */
            glDeleteTextures(1, &gnTexture);
            return;
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
        "preload",          argparser::option::no_argument, &preloadImages, 1,
        "transcode-target", argparser::option::required_argument, nullptr, 2,
        NULL,               argparser::option::no_argument,       nullptr, 0
    };

    argvector argv(sArgs);
    argparser ap(argv);

    int ch;
    while ((ch = ap.getopt(nullptr, longopts, nullptr)) != -1) {
        switch (ch) {
          case 0: break;
          case 2:
            transcodeTarget = strtofmt(ap.optarg);
            break;
          default: assert(false); // Error in args in sample table.
        }
    }
    assert(ap.optind < argv.size());
    filename = argv[ap.optind];

}

ktx_transcode_fmt_e
DrawTexture::strtofmt(_tstring format)
{
    if (!format.compare("ETC1_RGB"))
        return KTX_TTF_ETC1_RGB;
    else if (!format.compare("ETC2_RGBA"))
        return KTX_TTF_ETC2_RGBA;
    else if (!format.compare("BC1_RGB"))
        return KTX_TTF_BC1_RGB;
    else if (!format.compare("BC3_RGBA"))
        return KTX_TTF_BC3_RGBA;
    else if (!format.compare("BC4_R"))
        return KTX_TTF_BC4_R;
    else if (!format.compare("BC5_RG"))
        return KTX_TTF_BC5_RG;
    else if (!format.compare("BC7_M6_RGB"))
        return KTX_TTF_BC7_M6_RGB;
    else if (!format.compare("BC7_M5_RGBA"))
        return KTX_TTF_BC7_M5_RGBA;
    else if (!format.compare("PVRTC1_4_RGB"))
        return KTX_TTF_PVRTC1_4_RGB;
    else if (!format.compare("PVRTC1_4_RGBA"))
        return KTX_TTF_PVRTC1_4_RGBA;
    else if (!format.compare("ASTC_4x4_RGBA"))
        return KTX_TTF_ASTC_4x4_RGBA;
    else if (!format.compare("PVRTC2_4_RGB"))
        return KTX_TTF_PVRTC2_4_RGB;
    else if (!format.compare("PVRTC2_4_RGBA"))
        return KTX_TTF_PVRTC2_4_RGBA;
    else if (!format.compare("ETC2_EAC_R11"))
        return KTX_TTF_ETC2_EAC_R11;
    else if (!format.compare("ETC2_EAC_RG11"))
        return KTX_TTF_ETC2_EAC_RG11;
    else if (!format.compare("RGBA32"))
        return KTX_TTF_RGBA32;
    else if (!format.compare("RGB565"))
        return KTX_TTF_RGB565;
    else if (!format.compare("BGR565"))
        return KTX_TTF_BGR565;
    else if (!format.compare("RGBA4444"))
        return KTX_TTF_RGBA4444;
    else if (!format.compare("ETC"))
        return KTX_TTF_ETC;
    else if (!format.compare("BC1_OR_3"))
        return KTX_TTF_BC1_OR_3;
    assert(false); // Error in args in sample table.
    return static_cast<ktx_transcode_fmt_e>(-1); // To keep compilers happy.
}

/* ------------------------------------------------------------------------- */

void
DrawTexture::resize(uint32_t uWidth, uint32_t uHeight)
{

    glViewport(0, 0, uWidth, uHeight);
    this->uWidth = uWidth;
    this->uHeight = uHeight;

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
DrawTexture::run(uint32_t msTicks)
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

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
#include "frame.h"
#include "quad.h"

/* ------------------------------------------------------------------------- */

extern const GLchar* pszVs;
extern const GLchar* pszDecalFs;
extern const GLchar* pszColorFs;

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
    std::string filename;
    GLfloat* pfQuadTexCoords = quad_texture;
    GLfloat  fTmpTexCoords[sizeof(quad_texture)/sizeof(GLfloat)];
    GLenum target;
    GLboolean isMipmapped;
    GLenum glerror;
    GLubyte* pKvData;
    GLuint  kvDataLen;
    GLint sign_s = 1, sign_t = 1;
    GLint i;
    GLuint gnColorFs, gnDecalFs, gnVs;
    GLsizeiptr offset;
    KTX_dimensions dimensions;
    KTX_error_code ktxresult;
    KTX_hash_table kvtable;

    bInitialized = false;
    gnTexture = 0;
    
    filename = getAssetPath() + szArgs;    
    ktxresult = ktxLoadTextureN(filename.c_str(), &gnTexture, &target,
                               &dimensions, &isMipmapped, &glerror,
                               &kvDataLen, &pKvData);

    if (KTX_SUCCESS == ktxresult) {

        ktxresult = ktxHashTable_Deserialize(kvDataLen, pKvData, &kvtable);
        if (KTX_SUCCESS == ktxresult) {
            GLchar* pValue;
            GLuint valueLen;

            if (KTX_SUCCESS == ktxHashTable_FindValue(kvtable, KTX_ORIENTATION_KEY,
                                                      &valueLen, (void**)&pValue))
            {
                char s, t;

                if (sscanf(pValue, /*valueLen,*/ KTX_ORIENTATION2_FMT, &s, &t) == 2) {
                    if (s == 'l') sign_s = -1;
                    if (t == 'd') sign_t = -1;
                }
            }
            ktxHashTable_Destroy(kvtable);
            free(pKvData);
        }

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

        uTexWidth = dimensions.width;
        uTexHeight = dimensions.height;

        if (isMipmapped)
            // Enable bilinear mipmapping.
            // TO DO: application can consider inserting a key,value pair in
            // the KTX file that indicates what type of filtering to use.
            glTexParameteri(target,
                            GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        else
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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
    try {
        makeShader(GL_VERTEX_SHADER, pszVs, &gnVs);
        makeShader(GL_FRAGMENT_SHADER, pszColorFs, &gnColorFs);
        makeProgram(gnVs, gnColorFs, &gnColProg);
        gulMvMatrixLocCP = glGetUniformLocation(gnColProg, "mvmatrix");
        gulPMatrixLocCP = glGetUniformLocation(gnColProg, "pmatrix");
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

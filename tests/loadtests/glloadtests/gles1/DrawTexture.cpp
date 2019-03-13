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
 * @file    DrawTexture.cpp
 * @brief   Draw textures at actual size using the DrawTexture functions
 *          from OES_draw_texture.
 *
 * @author  Mark Callow
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
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "DrawTexture.h"
#include "frame.h"

/* ------------------------------------------------------------------------ */

#if 0
static int isPowerOfTwo (int x)
{
   if (x < 0) x = -1;
   return ((x != 0) && !(x & (x - 1)));
}
#endif

/* ------------------------------------------------------------------------ */

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
        : LoadTestSample(width, height, sBasePath)
{
    GLint           iCropRect[4] = {0, 0, 0, 0};
    const GLchar*  szExtensions  = (const GLchar*)glGetString(GL_EXTENSIONS);
    const char* filename;
    std::string pathname;
    GLenum target;
    GLboolean isMipmapped;
    GLboolean npotTexture;
    GLenum glerror;
    GLubyte* pKvData;
    GLuint  kvDataLen;
    KTX_dimensions dimensions;
    KTX_error_code ktxresult;
    KTX_hash_table kvtable;
    GLint sign_s = 1, sign_t = 1;

    bInitialized = false;
    gnTexture = 0;


    typedef void (*PFN_voidFunction)(void);
    if (strstr(szExtensions, "OES_draw_texture") != NULL) {
       /*
        * This strange casting is because SDL_GL_GetProcAddress returns a
        * void* thus is not compatible with ISO C which forbids conversion
        * of object pointers to function pointers. The cast masks the
        * conversion from the compiler thus no warning even though -pedantic
        * is set. Ideally, if SDL_GL_GetPRocAddress returned a (void*)(void),
        * the assignment would be
        *
        *    glDrawFooOES = (PFNHLDRAWFOOOESPROC)SDL_GetProcAddress(...)
        */                                                  \
       *(void **)(&glDrawTexsOES) = SDL_GL_GetProcAddress("glDrawTexsOES");
       *(void **)(&glDrawTexiOES) = SDL_GL_GetProcAddress("glDrawTexiOES");
       *(void **)(&glDrawTexxOES) = SDL_GL_GetProcAddress("glDrawTexxOES");
       *(void **)(&glDrawTexfOES) = SDL_GL_GetProcAddress("glDrawTexfOES");
       *(void **)(&glDrawTexsvOES) = SDL_GL_GetProcAddress("glDrawTexsvOES");
       *(void **)(&glDrawTexivOES) = SDL_GL_GetProcAddress("glDrawTexivOES");
       *(void **)(&glDrawTexxvOES) = SDL_GL_GetProcAddress("glDrawTexxvOES");
       *(void **)(&glDrawTexfvOES) = SDL_GL_GetProcAddress("glDrawTexfvOES");
    } else {
        /* Can't do anything */
        std::stringstream message;

        message << "DrawTexture: this OpenGL ES implementation does not support"
                << " OES_draw_texture. Can't Run Test";
        throw std::runtime_error(message.str());
    }
  
    if (strstr(szExtensions, "OES_texture_npot") != NULL)
       bNpotSupported = GL_TRUE;
    else
       bNpotSupported = GL_FALSE;


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

    if (npotTexture  && !bNpotSupported) {
        /* Load error texture. */
        filename = "testimages/no-npot.ktx";
    }
    
    pathname = getAssetPath() + filename;
    
    ktxresult = ktxLoadTextureN(pathname.c_str(), &gnTexture, &target,
                               &dimensions, &isMipmapped, &glerror,
                               &kvDataLen, &pKvData);
  
    if (KTX_SUCCESS == ktxresult) {
        if (target != GL_TEXTURE_2D) {
            /* Can only draw 2D textures */
            glDeleteTextures(1, &gnTexture);
            return;
        }

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

        iCropRect[2] = uTexWidth = dimensions.width;
        iCropRect[3] = uTexHeight = dimensions.height;
        iCropRect[2] *= sign_s;
        iCropRect[3] *= sign_t;

        glEnable(target);

        if (isMipmapped)
            // Enable bilinear mipmapping.
            // TO DO: application can consider inserting a key,value pair in
            // the KTX file that indicates what type of filtering to use.
            glTexParameteri(target,
                            GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        else
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
        glTexParameteriv(target, GL_TEXTURE_CROP_RECT_OES, iCropRect);

        /* Check for any errors */
        assert(GL_NO_ERROR == glGetError());
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

    glClearColor(0.4f, 0.4f, 0.5f, 1.0f);
    glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_BYTE, 0, (GLvoid *)frame_position);

    bInitialized = GL_TRUE;
}

/* ------------------------------------------------------------------------ */

DrawTexture::~DrawTexture()
{
    if (bInitialized) {
        glDeleteTextures(1, &gnTexture);
    }
    assert(GL_NO_ERROR == glGetError());
}

/* ------------------------------------------------------------------------ */

void
DrawTexture::resize(uint32_t uWidth, uint32_t uHeight)
{
    glViewport(0, 0, uWidth, uHeight);
    this->uWidth = uWidth;
    this->uHeight = uHeight;

    // Set up an orthographic projection where 1 = 1 pixel
    framePMatrix = glm::ortho(0.f, (float)uWidth, 0.f, (float)uHeight);
    // Move (0,0,0) to the center of the window.
    framePMatrix *= glm::translate(glm::mat4(),
                              glm::vec3((float)uWidth/2, (float)uHeight/2, 0));

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(framePMatrix));

    glMatrixMode(GL_MODELVIEW);
    // Scale the frame to fill the viewport. To guarantee its lines
    // appear we need to inset them by half-a-pixel hence the -1.
    // [Lines at the edges of the clip volume may or may not appear
    // depending on the OpenGL ES implementation. This is because
    // (a) the edges are on the points of the diamonds of the diamond
    //     exit rule and slight precision errors can easily push the
    //     lines outside the diamonds.
    // (b) the specification allows lines to be up to 1 pixel either
    //     side of the exact position.]
    glLoadIdentity();
    glScalef((float)(uWidth - 1) / 2, (float)(uHeight - 1) / 2, 1);
}

/* ------------------------------------------------------------------------ */

void
DrawTexture::run(uint32_t msTicks)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_TEXTURE_2D);
    glDrawArrays(GL_LINE_LOOP, 0, 4);

    glEnable(GL_TEXTURE_2D);
    glDrawTexiOES(uWidth/2 - uTexWidth/2,
                         (uHeight)/2 - uTexHeight/2,
                         0,
                         uTexWidth, uTexHeight);

    assert(GL_NO_ERROR == glGetError());
}

/* ------------------------------------------------------------------------ */

/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/* $Id$ */

/**
 * @file	sample_01_draw_texture.c
 * @brief	Tests the KTX loader with OpenGL ES 1.1 by loading and drawing
 *          KTX textures in various formats using the DrawTexture functions
 *          from OES_draw_texture.
 *
 * @author	Mark Callow
 */

/*
 * Copyright (c) 2008 HI Corporation.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * unaltered in all copies or substantial portions of the Materials.
 * Any additions, deletions, or changes to the original source files
 * must be clearly indicated in accompanying documentation.
 *
 * If only executable code is distributed, then the accompanying
 * documentation must state that "this software is based in part on the
 * work of HI Corporation."
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

#if defined(_WIN32)
  #if _MSC_VER < 1900
    #define snprintf _snprintf
  #endif
  #define _CRT_SECURE_NO_WARNINGS
#endif

#include "ktx.h"
#include "../common/at.h"
#include "../data/frame.h"

/* ----------------------------------------------------------------------------- */

typedef struct DrawTexture_def {

    PFNGLDRAWTEXSOESPROC glDrawTexsOES;
    PFNGLDRAWTEXIOESPROC glDrawTexiOES;
    PFNGLDRAWTEXXOESPROC glDrawTexxOES;
    PFNGLDRAWTEXFOESPROC glDrawTexfOES;
    PFNGLDRAWTEXSVOESPROC glDrawTexsvOES;
    PFNGLDRAWTEXIVOESPROC glDrawTexivOES;
    PFNGLDRAWTEXXVOESPROC glDrawTexxvOES;
    PFNGLDRAWTEXFVOESPROC glDrawTexfvOES;

	int iWidth;
	int iHeight;

	int iTexWidth;
	int iTexHeight;

	float fFrameMvMatrix[16];
	float fFramePMatrix[16];

	GLuint gnTexture;

    GLboolean bNpotSupported;

	GLboolean bInitialized;
} DrawTexture;


/* ----------------------------------------------------------------------------- */

#if 0
static int isPowerOfTwo (int x)
{
   if (x < 0) x = -1;
   return ((x != 0) && !(x & (x - 1)));
}
#endif

/* ----------------------------------------------------------------------------- */

void atInitialize_01_draw_texture(void** ppAppData, const char* const szArgs,
                                  const char* const szBasePath)
{
	GLint           iCropRect[4] = {0, 0, 0, 0};
	const GLchar*  szExtensions  = (const GLchar*)glGetString(GL_EXTENSIONS);
    const char* filename;
	GLenum target;
	GLboolean isMipmapped;
    GLboolean npotTexture;
	GLenum glerror;
	GLubyte* pKvData;
	GLuint  kvDataLen;
	KTX_dimensions dimensions;
	KTX_error_code ktxerror;
	KTX_hash_table kvtable;
	GLint sign_s = 1, sign_t = 1;

	DrawTexture* pData = (DrawTexture*)atMalloc(sizeof(DrawTexture), 0);

	atAssert(pData);
	atAssert(ppAppData);

	*ppAppData = pData;

	pData->bInitialized = GL_FALSE;
	pData->gnTexture = 0;

	if (strstr(szExtensions, "OES_draw_texture") != NULL) {
       pData->glDrawTexsOES =
		 	 (PFNGLDRAWTEXSOESPROC)SDL_GL_GetProcAddress("glDrawTexsOES");
       pData->glDrawTexiOES =
		 	 (PFNGLDRAWTEXIOESPROC)SDL_GL_GetProcAddress("glDrawTexiOES");
       pData->glDrawTexxOES =
		 	 (PFNGLDRAWTEXXOESPROC)SDL_GL_GetProcAddress("glDrawTexxOES");
       pData->glDrawTexfOES =
		 	 (PFNGLDRAWTEXFOESPROC)SDL_GL_GetProcAddress("glDrawTexfOES");
       pData->glDrawTexsvOES =
		 	 (PFNGLDRAWTEXSVOESPROC)SDL_GL_GetProcAddress("glDrawTexsvOES");
       pData->glDrawTexivOES =
		 	 (PFNGLDRAWTEXIVOESPROC)SDL_GL_GetProcAddress("glDrawTexivOES");
       pData->glDrawTexxvOES =
		 	 (PFNGLDRAWTEXXVOESPROC)SDL_GL_GetProcAddress("glDrawTexxvOES");
       pData->glDrawTexfvOES =
		 	 (PFNGLDRAWTEXFVOESPROC)SDL_GL_GetProcAddress("glDrawTexfvOES");
	} else {
        /* Can't do anything */
        atMessageBox("This OpenGL ES implementation does not support "
                     "OES_draw_texture.",
                     "Can't Run Test", AT_MB_OK|AT_MB_ICONERROR);
	   return;
	}
  
    if (strstr(szExtensions, "OES_texture_npot") != NULL)
       pData->bNpotSupported = GL_TRUE;
    else
       pData->bNpotSupported = GL_FALSE;


    if ((filename = strchr(szArgs, ' ')) != NULL) {
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

    if (npotTexture  && !pData->bNpotSupported) {
        /* Load error texture. */
        filename = "testimages/no-npot.ktx";
    }
    
    filename = atStrCat(szBasePath, filename);
    
    if (filename != NULL) {
        ktxerror = ktxLoadTextureN(filename, &pData->gnTexture, &target,
                                   &dimensions, &isMipmapped, &glerror,
                                   &kvDataLen, &pKvData);
      
        if (KTX_SUCCESS == ktxerror) {
            if (target != GL_TEXTURE_2D) {
                /* Can only draw 2D textures */
                glDeleteTextures(1, &pData->gnTexture);
                return;
            }

            ktxerror = ktxHashTable_Deserialize(kvDataLen, pKvData, &kvtable);
            if (KTX_SUCCESS == ktxerror) {
                GLchar* pValue;
                GLuint valueLen;

                if (KTX_SUCCESS == ktxHashTable_FindValue(kvtable, KTX_ORIENTATION_KEY,
                                                          &valueLen, (void*)&pValue))
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

            iCropRect[2] = pData->iTexWidth = dimensions.width;
            iCropRect[3] = pData->iTexHeight = dimensions.height;
            iCropRect[2] *= sign_s;
            iCropRect[3] *= sign_t;

            glEnable(target);

            if (isMipmapped) 
                /* Enable bilinear mipmapping */
                /* TO DO: application can consider inserting a key,value pair in the KTX
                 * that indicates what type of filtering to use.
                 */
                glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
            else
                glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
            glTexParameteriv(target, GL_TEXTURE_CROP_RECT_OES, iCropRect);

            /* Check for any errors */
            glerror = glGetError();
        } else {
            char message[1024];
            int maxchars = sizeof(message)/sizeof(char);
            int nchars;

			nchars = snprintf(message, maxchars, "Load of texture \"%s\" failed: ",
				              filename);
			maxchars -= nchars;
			if (ktxerror == KTX_GL_ERROR) {
				nchars += snprintf(&message[nchars], maxchars, "GL error %#x occurred.", glerror);
			} else {
				nchars += snprintf(&message[nchars], maxchars, "%s.", ktxErrorString(ktxerror));
			}
			atMessageBox(message, "Texture load failed", AT_MB_OK | AT_MB_ICONERROR);

            pData->iTexWidth = pData->iTexHeight = 50;
            pData->gnTexture = 0;
        }

        atFree((void*)filename, NULL);
    } /* else
        Out of memory. In which case, a message box is unlikely to work. */

	glClearColor(0.4f, 0.4f, 0.5f, 1.0f);
	glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_BYTE, 0, (GLvoid *)frame_position);

	pData->bInitialized = GL_TRUE;
}

/* ----------------------------------------------------------------------------- */

void atRelease_01_draw_texture (void* pAppData)
{
	DrawTexture* pData = (DrawTexture*)pAppData;
	atAssert(pData);

	if (pData->bInitialized) {
		DrawTexture* pData = (DrawTexture*)pAppData;
		atAssert(pData);
		glDeleteTextures(1, &pData->gnTexture);
	}
	atAssert(GL_NO_ERROR == glGetError());
	atFree(pData, 0);
}

/* ----------------------------------------------------------------------------- */

void atResize_01_draw_texture (void* pAppData, int iWidth, int iHeight)
{

	DrawTexture* pData = (DrawTexture*)pAppData;
	atAssert(pData);

	glViewport(0, 0, iWidth, iHeight);
	pData->iWidth = iWidth;
	pData->iHeight = iHeight;

	// Set up an orthographic projection where 1 = 1 pixel, and 0,0,0
	// is at the center of the window.
	atSetOrthoZeroAtCenterMatrix(pData->fFramePMatrix, 0.0f, (float)iWidth,
					 0.0, (float)iHeight,
					 -1.0f, 1.0f);

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(pData->fFramePMatrix);

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
	glScalef((float)(iWidth - 1) / 2, (float)(iHeight - 1) / 2, 1);
}

/* ----------------------------------------------------------------------------- */

void atRun_01_draw_texture (void* pAppData, int iTimeMS)
{
	DrawTexture* pData = (DrawTexture*)pAppData;
	atAssert(pData);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_TEXTURE_2D);
	glDrawArrays(GL_LINE_LOOP, 0, 4);

	glEnable(GL_TEXTURE_2D);
	pData->glDrawTexiOES(pData->iWidth/2 - pData->iTexWidth/2,
		         		 (pData->iHeight)/2 - pData->iTexHeight/2,
						 0,
				 		 pData->iTexWidth, pData->iTexHeight);

	atAssert(GL_NO_ERROR == glGetError());
}

/* ----------------------------------------------------------------------------- */

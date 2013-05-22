/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/**
 * @file	sample_01_draw_texture.c
 * @brief	Tests DrawTexture functionality to see if the implementation
 *          applies the viewport transform to the supplied coordinates.
 *
 * @author	Mark Callow
 *
 * $Revision: 11851 $
 * $Date:: 2010-07-05 19:49:43 +0900 #$
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

#include "ktx.h"
#include "../common/at.h"
#include "../data/frame.h"

#if defined(_WIN32)
#define snprintf _snprintf
#endif

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

	GLint gnTexture;

	GLboolean bInitialized;
} DrawTexture;


/* ----------------------------------------------------------------------------- */


void atInitialize_01_draw_texture(void** ppAppData, const char* const args)
{
	GLint           iCropRect[4]    = {0, 0, 0, 0};
	const GLubyte*  szExtensions     = glGetString(GL_EXTENSIONS);
	GLuint texture = 0;
	GLenum target;
	GLboolean isMipmapped;
	GLenum glerror;
	GLubyte* pKvData;
	GLsizei  kvDataLen;
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
		 	 (PFNGLDRAWTEXSOESPROC)eglGetProcAddress("glDrawTexsOES");
       pData->glDrawTexiOES =
		 	 (PFNGLDRAWTEXIOESPROC)eglGetProcAddress("glDrawTexiOES");
       pData->glDrawTexxOES =
		 	 (PFNGLDRAWTEXXOESPROC)eglGetProcAddress("glDrawTexxOES");
       pData->glDrawTexfOES =
		 	 (PFNGLDRAWTEXFOESPROC)eglGetProcAddress("glDrawTexfOES");
       pData->glDrawTexsvOES =
		 	 (PFNGLDRAWTEXSVOESPROC)eglGetProcAddress("glDrawTexsvOES");
       pData->glDrawTexivOES =
		 	 (PFNGLDRAWTEXIVOESPROC)eglGetProcAddress("glDrawTexivOES");
       pData->glDrawTexxvOES =
		 	 (PFNGLDRAWTEXXVOESPROC)eglGetProcAddress("glDrawTexxvOES");
       pData->glDrawTexfvOES =
		 	 (PFNGLDRAWTEXFVOESPROC)eglGetProcAddress("glDrawTexfvOES");
	} else {
	   /* Can't do anything */
	   return;
	}

	ktxerror = ktxLoadTextureN(args, &pData->gnTexture, &target, &dimensions,
							   &isMipmapped, &glerror, &kvDataLen, &pKvData);

	if (KTX_SUCCESS == ktxerror) {
		if (target != GL_TEXTURE_2D) {
			/* Can only draw 2D textures */
			glDeleteTextures(1, &pData->gnTexture);
			return;
		}

		ktxerror = ktxHashTable_Deserialize(kvDataLen, pKvData, &kvtable);
		if (KTX_SUCCESS == ktxerror) {
			GLubyte* pValue;
			GLsizei valueLen;

			if (KTX_SUCCESS == ktxHashTable_FindValue(kvtable, KTX_ORIENTATION_KEY,
													  &valueLen, &pValue))
			{
				char s, t;

				if (_snscanf(pValue, valueLen, KTX_ORIENTATION2_FMT, &s, &t) == 2) {
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

		nchars = snprintf(message, maxchars, "Load of texture \"%s\" failed: %s.",
						  args, ktxErrorString(ktxerror));
		if (ktxerror == KTX_GL_ERROR) {
			maxchars -= nchars;
			nchars += snprintf(&message[nchars], maxchars, " GL error is %#x.", glerror);
		}
		atMessageBox(message, "Texture load failed", AT_MB_OK|AT_MB_ICONERROR);

		pData->iTexWidth = pData->iTexHeight = 50;
		pData->gnTexture = 0;
	}

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
	atSetOrthoZeroAtCenterMatrix(pData->fFramePMatrix, -0.5f, iWidth - 0.5f,
					 -0.5, iHeight - 0.5f,
					 -1.0f, 1.0f);

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(pData->fFramePMatrix);

	glMatrixMode(GL_MODELVIEW);
	// Scale the frame to fit inside the viewport
    // Because rectangles are half-open in GL, a -1,-1 to +1,+1 line
	// loop with identity MVP matrix loses the topmost & rightmost lines.
	glLoadIdentity();
	glScalef((float)(iWidth - 3) / 2, (float)(iHeight - 3) / 2, 1);
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

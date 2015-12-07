/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/* $Id$ */

/**
 * @file	sample_02_cube_textured.c
 * @brief	Draw a textured cube.
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

//#include "sample.h"
#include "../common/at.h"
#include "ktx.h"

#include "../data/cube.h"

#include <math.h>

#if defined(_WIN32)
#define snprintf _snprintf
#endif

/* ------------------------------------------------------------------------- */

void atInitialize_02_cube(void** ppAppData, const char* const szArgs,
                          const char* const szBasePath)
{
    const GLchar*  szExtensions = (const GLchar*)glGetString(GL_EXTENSIONS);
    const char* filename;
	GLuint texture = 0;
	GLenum target;
	GLenum glerror;
	GLboolean isMipmapped;
    GLboolean npotSupported, npotTexture;
	KTX_error_code ktxerror;

    if (strstr(szExtensions, "OES_texture_npot") != NULL)
        npotSupported = GL_TRUE;
    else
        npotSupported = GL_FALSE;
    
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
    
    if (npotTexture  && !npotSupported) {
        /* Load error texture. */
        filename = "testimages/no-npot.ktx";
    }
    filename = atStrCat(szBasePath, filename);

    if (filename != NULL) {
        ktxerror = ktxLoadTextureN(filename, &texture, &target, NULL, &isMipmapped,
                                   &glerror, 0, NULL);

        if (KTX_SUCCESS == ktxerror) {
            if (target != GL_TEXTURE_2D) {
                /* Can only draw 2D textures */
                glDeleteTextures(1, &texture);
                return;
            }
            glEnable(target);

            if (isMipmapped) 
                /* Enable bilinear mipmapping */
                /* TO DO: application can consider inserting a key,value pair in the KTX
                 * file that indicates what type of filtering to use.
                 */
                glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
            else
                glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
        } else {
            char message[1024];
            int maxchars = sizeof(message)/sizeof(char);
            int nchars;

            nchars = snprintf(message, maxchars, "Load of texture \"%s\" failed: %s.",
                              filename, ktxErrorString(ktxerror));
            if (ktxerror == KTX_GL_ERROR) {
                maxchars -= nchars;
                nchars += snprintf(&message[nchars], maxchars, " GL error is %#x.", glerror);
            }
            atMessageBox(message, "Texture load failed", AT_MB_OK|AT_MB_ICONERROR);
        }
        
        atFree((void*)filename, NULL);
    } /* else
       Out of memory. In which case, a message box is unlikely to work. */

	/* By default dithering is enabled. Dithering does not provide visual improvement
	 * in this sample so disable it to improve performance. 
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

void atRelease_02_cube(void* pAppData)
{
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_DITHER);
	glDisable(GL_CULL_FACE);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	atAssert(GL_NO_ERROR == glGetError());
}

void atResize_02_cube(void* pAppData, int iWidth, int iHeight)
{
	GLfloat matProj[16];
	glViewport(0, 0, iWidth, iHeight);

	glMatrixMode( GL_PROJECTION );
	atSetProjectionMatrix(matProj, 45.f, iWidth / (GLfloat)iHeight, 1.0f, 100.f);
	glLoadIdentity();
	glLoadMatrixf(matProj);

	glMatrixMode( GL_MODELVIEW );
}

void atRun_02_cube(void* pAppData, int iTimeMS) 
{
	/* Setup the view matrix : just turn around the cube. */
	float matView[16];
	const float fDistance = 50.0f;
	atSetViewMatrix(matView, 
		(float)cos( iTimeMS*0.001f ) * fDistance, (float)sin( iTimeMS*0.0007f ) * fDistance, (float)sin( iTimeMS*0.001f ) * fDistance, 
		0.0f, 0.0f, 0.0f);

	glLoadIdentity();
	glLoadMatrixf(matView);

	/* Draw */
	glClear( GL_COLOR_BUFFER_BIT );

	glDrawElements(GL_TRIANGLES, sizeof(cube_index_buffer), GL_UNSIGNED_BYTE, cube_index_buffer);

	atAssert(GL_NO_ERROR == glGetError());
}

/* ------------------------------------------------------------------------- */


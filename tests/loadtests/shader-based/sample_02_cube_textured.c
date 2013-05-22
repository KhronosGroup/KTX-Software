/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/**
 * @file	sample_02_cube_textured.c
 * @brief	Draw a textured cube.
 *
 * $Revision: 21082 $
 * $Date:: 2013-04-09 14:52:45 +0900 #$
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

GLboolean makeShader(GLenum type, const GLchar* const source, GLuint* shader);
GLboolean makeProgram(GLuint vs, GLuint fs, GLuint* program);

extern const GLchar* pszVs;
extern const GLchar* pszDecalFs;

/* ------------------------------------------------------------------------- */

typedef struct CubeTextured_def {
	GLuint gnTexture;
	GLuint gnTexProg;

	GLuint gnVao;
	GLuint gnVbo;

	GLsizeiptr iIndicesOffset;

	GLint gulMvMatrixLocTP;
	GLint gulPMatrixLocTP;
	GLint gulSamplerLocTP;

	GLboolean bInitialized;
} CubeTextured;

/* ------------------------------------------------------------------------- */

void atInitialize_02_cube(void** ppAppData, const char* const args)
{
	GLuint texture = 0;
	GLenum target;
	GLenum glerror;
	GLboolean isMipmapped;
	GLuint gnDecalFs, gnVs;
	GLsizeiptr offset;
	KTX_error_code ktxerror;

	CubeTextured* pData = (CubeTextured*)atMalloc(sizeof(CubeTextured), 0);

	atAssert(pData);
	atAssert(ppAppData);

	*ppAppData = pData;

	pData->bInitialized = GL_FALSE;
	pData->gnTexture = 0;

	ktxerror = ktxLoadTextureN(args, &texture, &target, NULL, &isMipmapped, &glerror,
		                       0, NULL);

	if (KTX_SUCCESS == ktxerror) {
		if (target != GL_TEXTURE_2D) {
			/* Can only draw 2D textures */
			glDeleteTextures(1, &texture);
			return;
		}

		if (isMipmapped) 
			/* Enable bilinear mipmapping */
			/* TO DO: application can consider inserting a key,value pair in the KTX
			 * file that indicates what type of filtering to use.
			 */
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		else
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		atAssert(GL_NO_ERROR == glGetError());
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
	}

	/* By default dithering is enabled. Dithering does not provide visual improvement 
	 * in this sample so disable it to improve performance. 
	 */
	glDisable(GL_DITHER);

	glEnable(GL_CULL_FACE);
	glClearColor(0.2f,0.3f,0.4f,1.0f);

	// Create a VAO and bind it.
	glGenVertexArrays(1, &pData->gnVao);
	glBindVertexArray(pData->gnVao);

	// Must have vertex data in buffer objects to use VAO's on ES3/GL Core
	glGenBuffers(1, &pData->gnVbo);
	glBindBuffer(GL_ARRAY_BUFFER, pData->gnVbo);
	// Must be done after the VAO is bound
	// Use the same buffer for vertex attributes and element indices.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pData->gnVbo);

	// Create the buffer data store. 
	glBufferData(GL_ARRAY_BUFFER,
				 sizeof(cube_face) + sizeof(cube_color) + sizeof(cube_texture)
				 + sizeof(cube_normal) + sizeof(cube_index_buffer),
				 NULL, GL_STATIC_DRAW);

	// Interleave data copying and attrib pointer setup so offset is only computed once.
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
	pData->iIndicesOffset = offset;
	// Either of the following can be used to buffer the data.
	glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(cube_index_buffer), cube_index_buffer);
	//glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, sizeof(cube_index_buffer), cube_index_buffer);

	if (makeShader(GL_VERTEX_SHADER, pszVs, &gnVs)) {
		if (makeShader(GL_FRAGMENT_SHADER, pszDecalFs, &gnDecalFs)) {
			if (makeProgram(gnVs, gnDecalFs, &pData->gnTexProg)) {
				pData->gulMvMatrixLocTP = glGetUniformLocation(pData->gnTexProg, "mvmatrix");
				pData->gulPMatrixLocTP = glGetUniformLocation(pData->gnTexProg, "pmatrix");
				pData->gulSamplerLocTP = glGetUniformLocation(pData->gnTexProg, "sampler");
				glUseProgram(pData->gnTexProg);
				// We're using the default texture unit 0
				glUniform1i(pData->gulSamplerLocTP, 0);
			}
		}
		glDeleteShader(gnVs);
		glDeleteShader(gnDecalFs);
	}

	atAssert(GL_NO_ERROR == glGetError());
	pData->bInitialized = GL_TRUE;
}

void atRelease_02_cube(void* pAppData)
{
	CubeTextured* pData = (CubeTextured*)pAppData;
	atAssert(pData);

	glEnable(GL_DITHER);
	glDisable(GL_CULL_FACE);
	if (pData->bInitialized) {
		glUseProgram(0);
		glDeleteTextures(1, &pData->gnTexture);
		glDeleteProgram(pData->gnTexProg);
		glDeleteBuffers(1, &pData->gnVbo);
		glDeleteVertexArrays(1, &pData->gnVao);
	}
	atAssert(GL_NO_ERROR == glGetError());
	atFree(pData, 0);
}


void atResize_02_cube(void* pAppData, int iWidth, int iHeight)
{
	GLfloat matProj[16];
	CubeTextured* pData = (CubeTextured*)pAppData;
	atAssert(pData);

	glViewport(0, 0, iWidth, iHeight);

	atSetProjectionMatrix(matProj, 45.f, iWidth / (GLfloat)iHeight, 1.0f, 100.f);
	glUniformMatrix4fv(pData->gulPMatrixLocTP, 1, GL_FALSE, matProj);
}

void atRun_02_cube(void* pAppData, int iTimeMS) 
{
	/* Draw */
	CubeTextured* pData = (CubeTextured*)pAppData;
	/* Setup the view matrix : just turn around the cube. */
	float matView[16];
	const float fDistance = 50.0f;

	atAssert(pData);

	atSetViewMatrix(matView, 
		(float)cos( iTimeMS*0.001f ) * fDistance, (float)sin( iTimeMS*0.0007f ) * fDistance, (float)sin( iTimeMS*0.001f ) * fDistance, 
		0.0f, 0.0f, 0.0f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUniformMatrix4fv(pData->gulMvMatrixLocTP, 1, GL_FALSE, matView);

	glDrawElements(GL_TRIANGLES, sizeof(cube_index_buffer), GL_UNSIGNED_BYTE, (GLvoid*)(pData->iIndicesOffset));

	atAssert(GL_NO_ERROR == glGetError());
}

/* ------------------------------------------------------------------------- */


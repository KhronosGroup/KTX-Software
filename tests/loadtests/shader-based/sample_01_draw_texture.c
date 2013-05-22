/* -*- tab-iWidth: 4; -*- */
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

//#include "sample.h"
#include "ktx.h"
#include "../common/at.h"
#include "../data/frame.h"
#include "../data/quad.h"

#if defined(_WIN32)
#define snprintf _snprintf
#endif

/* ----------------------------------------------------------------------------- */

GLboolean makeShader(GLenum type, const GLchar* const source, GLuint* shader);
GLboolean makeProgram(GLuint vs, GLuint fs, GLuint* program);

extern const GLchar* pszVs;
extern const GLchar* pszDecalFs;
extern const GLchar* pszColorFs;

/* ----------------------------------------------------------------------------- */

typedef struct DrawTexture_def {
	int iWidth;
	int iHeight;

	int iTexWidth;
	int iTexHeight;

	float fFrameMvMatrix[16];
	float fQuadMvMatrix[16];
	float fPMatrix[16];

	GLuint gnTexture;
	GLuint gnTexProg;
	GLuint gnColProg;

#define FRAME 0
#define QUAD  1
	GLuint gnVaos[2];
	GLuint gnVbo;

	GLint gulMvMatrixLocTP;
	GLint gulPMatrixLocTP;
	GLint gulSamplerLocTP;
	GLint gulMvMatrixLocCP;
	GLint gulPMatrixLocCP;

	GLboolean bInitialized;
} DrawTexture;

/* ----------------------------------------------------------------------------- */

void atInitialize_01_draw_texture(void** ppAppData, const char* const args)
{
	GLfloat* pfQuadTexCoords = quad_texture;
	GLfloat  fTmpTexCoords[sizeof(quad_texture)/sizeof(GLfloat)];
	GLuint texture = 0;
	GLenum target;
	GLboolean isMipmapped;
	GLenum glerror;
	GLubyte* pKvData;
	GLsizei  kvDataLen;
	GLint sign_s = 1, sign_t = 1;
	GLint i;
	GLuint gnColorFs, gnDecalFs, gnVs;
	GLsizeiptr offset;
	KTX_dimensions dimensions;
	KTX_error_code ktxerror;
	KTX_hash_table kvtable;

	DrawTexture* pData = (DrawTexture*)atMalloc(sizeof(DrawTexture), 0);

	atAssert(pData);
	atAssert(ppAppData);

	*ppAppData = pData;

	pData->bInitialized = GL_FALSE;
	pData->gnTexture = 0;
	
	ktxerror = ktxLoadTextureN(args, &pData->gnTexture, &target, &dimensions,
							   &isMipmapped, &glerror, &kvDataLen, &pKvData);

	if (KTX_SUCCESS == ktxerror) {

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

		if (sign_s < 0 || sign_t < 0) {
			// Transform the texture coordinates to get correct image orientation.
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

		pData->iTexWidth = dimensions.width;
		pData->iTexHeight = dimensions.height;

		if (isMipmapped) 
			/* Enable bilinear mipmapping */
			/* TO DO: application can consider inserting a key,value pair in the KTX
			 * that indicates what type of filtering to use.
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

		pData->iTexWidth = pData->iTexHeight = 50;
		pData->gnTexture = 0;
	}

	glClearColor(0.4f, 0.4f, 0.5f, 1.0f);

    // Must have vertex data in buffer objects to use VAO's on ES3/GL Core
	glGenBuffers(1, &pData->gnVbo);
	glBindBuffer(GL_ARRAY_BUFFER, pData->gnVbo);

	// Create the buffer data store
	glBufferData(GL_ARRAY_BUFFER,
				 sizeof(frame_position) + sizeof(frame_color) + sizeof(quad_position)
				 + sizeof(quad_color) + sizeof(quad_texture),
				 NULL, GL_STATIC_DRAW);

	glGenVertexArrays(2, pData->gnVaos);

	// Interleave data copying and attrib pointer setup so offset is only computed once.

	// Setup VAO and buffer the data for frame
	glBindVertexArray(pData->gnVaos[FRAME]);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	offset = 0;
	glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(frame_position), frame_position);
	glVertexAttribPointer(0, 3, GL_BYTE, GL_FALSE, 0, (GLvoid*)offset);
	offset += sizeof(frame_position);
	glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(frame_color), frame_color);
	glVertexAttribPointer(1, 3, GL_BYTE, GL_FALSE, 0, (GLvoid*)offset);
	offset += sizeof(frame_color);

	// Setup VAO for quad
	glBindVertexArray(pData->gnVaos[QUAD]);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(quad_position), quad_position);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)offset);
	offset += sizeof(quad_position);
	glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(quad_color), quad_color);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)offset);
	offset += sizeof(quad_color);
	glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(quad_texture), pfQuadTexCoords);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)offset);

	glBindVertexArray(0);

	if (makeShader(GL_VERTEX_SHADER, pszVs, &gnVs)) {
		if (makeShader(GL_FRAGMENT_SHADER, pszColorFs, &gnColorFs)) {
			if (makeProgram(gnVs, gnColorFs, &pData->gnColProg)) {
				pData->gulMvMatrixLocCP = glGetUniformLocation(pData->gnColProg, "mvmatrix");
				pData->gulPMatrixLocCP = glGetUniformLocation(pData->gnColProg, "pmatrix");
			}
		}
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
		glDeleteShader(gnColorFs);
		glDeleteShader(gnDecalFs);
	}

	// Set the texture's mv matrix to scale by the texture size.
	// With the pixel-mapping ortho projection set below, the texture will
	// be rendered at actual size just like DrawTex*OES.
	for (i = 0; i < 16; i++) {
		pData->fQuadMvMatrix[i] = atIdentity[i];
		pData->fFrameMvMatrix[i] = atIdentity[i];
	}
	pData->fQuadMvMatrix[0*4 + 0] = (float)pData->iTexWidth / 2;
	pData->fQuadMvMatrix[1*4 + 1] = (float)pData->iTexHeight / 2;

	atAssert(GL_NO_ERROR == glGetError());
	pData->bInitialized = GL_TRUE;
}

/* ----------------------------------------------------------------------------- */

void atRelease_01_draw_texture (void* pAppData)
{
	DrawTexture* pData = (DrawTexture*)pAppData;
	atAssert(pData);

	if (pData->bInitialized) {
		// A bug in the PVR SDK 3.1 emulator causes the glDeleteProgram(pData->gnColProg)
		// below to raise an INVALID_VALUE error if the following glUseProgram(0)
		// has been executed. Strangely the equivalent line in sample_02_cube_textured.c,
		// where only 1 program is used, does not raise an error.
		glUseProgram(0);
		glDeleteTextures(1, &pData->gnTexture);
		glDeleteProgram(pData->gnTexProg);
		glDeleteProgram(pData->gnColProg);
		glDeleteBuffers(1, &pData->gnVbo);
		glDeleteVertexArrays(2, pData->gnVaos);
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
	atSetOrthoZeroAtCenterMatrix(pData->fPMatrix, -0.5f, iWidth - 0.5f,
					 -0.5, iHeight - 0.5f,
					 -1.0f, 1.0f);

	// Scale the frame to fit inside the viewport
    // Because rectangles are half-open in GL, a -1,-1 to +1,+1 line
	// loop with identity MVP matrix loses the topmost & rightmost lines.
	pData->fFrameMvMatrix[0*4 + 0] = (float)(iWidth - 1) / 2;
	pData->fFrameMvMatrix[1*4 + 1] = (float)(iHeight - 1) / 2;
}

/* ----------------------------------------------------------------------------- */

void atRun_01_draw_texture (void* pAppData, int iTimeMS)
{
	DrawTexture* pData = (DrawTexture*)pAppData;
	atAssert(pData);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindVertexArray(pData->gnVaos[FRAME]);
	glUseProgram(pData->gnColProg);
	glUniformMatrix4fv(pData->gulMvMatrixLocCP, 1, GL_FALSE, pData->fFrameMvMatrix);
	glUniformMatrix4fv(pData->gulPMatrixLocCP, 1, GL_FALSE, pData->fPMatrix);
	glDrawArrays(GL_LINE_LOOP, 0, 4);

	glBindVertexArray(pData->gnVaos[QUAD]);
	glUseProgram(pData->gnTexProg);
	glUniformMatrix4fv(pData->gulMvMatrixLocTP, 1, GL_FALSE, pData->fQuadMvMatrix);
	glUniformMatrix4fv(pData->gulPMatrixLocTP, 1, GL_FALSE, pData->fPMatrix);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	atAssert(GL_NO_ERROR == glGetError());
}

/* ----------------------------------------------------------------------------- */

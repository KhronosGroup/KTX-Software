/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/* $Id$ */

/**
 * @file	at.c
 * @brief	Simple interface to build applications using renderion.
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
  #define _CRT_SECURE_NO_WARNINGS
#endif

#include "at.h"
#include <math.h>

/* ----------------------------------------------------------------------------- */

// Calculate the dot product and return it
static float dot(const float srcA[3], const float srcB[3])
{
	return srcA[0]*srcB[0] + srcA[1]*srcB[1] + srcA[2]*srcB[2];
}

static float dot4(const float srcA[4], const float srcB[4])
{
	return srcA[0]*srcB[0] + srcA[1]*srcB[1]
           + srcA[2]*srcB[2] + srcA[3]*srcB[3];
}

// Calculate the cross product and return it
static void cross (float dst[3], const float srcA[3], const float srcB[3])
{
	float dstT[3];
    dstT[0] = srcA[1]*srcB[2] - srcA[2]*srcB[1];
    dstT[1] = srcA[2]*srcB[0] - srcA[0]*srcB[2];
    dstT[2] = srcA[0]*srcB[1] - srcA[1]*srcB[0];

	dst[0] = dstT[0];
	dst[1] = dstT[1];
	dst[2] = dstT[2];
}

// Normalize the input vector
static void normalize (float vec[3])
{
    float squaredLen = vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2];
    float invLen = 1.f / (float) sqrt (squaredLen);

    vec[0] *= invLen;
    vec[1] *= invLen;
    vec[2] *= invLen;
}

/* ----------------------------------------------------------------------------- */

float atIdentity[] = {
	1.0f,0.0f,0.0f,0.0f,
	0.0f,1.0f,0.0f,0.0f,
	0.0f,0.0f,1.0f,0.0f,
	0.0f,0.0f,0.0f,1.0f
};

/* ----------------------------------------------------------------------------- */

int	atSetViewMatrix (float* aMatrix_, float eyex, float eyey, float eyez,
					 float atx, float aty, float atz)
{
	if(aMatrix_ == 0) return 0;

	{
		/*
		zaxis = normal(Eye - At)
		xaxis = normal(cross(Up, zaxis))
		yaxis = cross(zaxis, xaxis)

		 xaxis.x           yaxis.x           zaxis.x				0
		 xaxis.y           yaxis.y           zaxis.y				0
		 xaxis.z           yaxis.z           zaxis.z				0
		 -dot(xaxis, eye)  -dot(yaxis, eye)	 -dot(zaxis, eye)		1
		*/

		const float up[] = {0.0f, 1.0f, 0.0f};
		float eye[3] = {eyex, eyey, eyez};

		float xaxis[3];
		float yaxis[3];
		float zaxis[3];

		zaxis[0] = eyex-atx; zaxis[1] = eyey-aty; zaxis[2] = eyez-atz;
		normalize (zaxis);

		cross (xaxis, up, zaxis);
		normalize (xaxis);

		cross (yaxis, zaxis, xaxis);
		normalize (yaxis);


		aMatrix_[0*4 + 0]		= xaxis[0];
		aMatrix_[1*4 + 0]		= xaxis[1];
		aMatrix_[2*4 + 0]		= xaxis[2];

		aMatrix_[0*4 + 1]		= yaxis[0];
		aMatrix_[1*4 + 1]		= yaxis[1];
		aMatrix_[2*4 + 1]		= yaxis[2];

		aMatrix_[0*4 + 2]		= zaxis[0];
		aMatrix_[1*4 + 2]		= zaxis[1];
		aMatrix_[2*4 + 2]		= zaxis[2];

		aMatrix_[3]				= 0.f;
		aMatrix_[7]				= 0.f;
		aMatrix_[11]			= 0.f;
		aMatrix_[15]			= 1.f;
		aMatrix_[12]			= -dot(xaxis, eye); aMatrix_[13] = -dot(yaxis, eye); aMatrix_[14] = -dot(zaxis, eye);
	}
	return 1;
}

/* ----------------------------------------------------------------------------- */

int	atSetProjectionMatrix	(float* aMatrix_, float fovy, float aspect, float zNear, float zFar )
{
	if(aMatrix_ == 0) return 0;

	{
		float xmin, xmax, ymin, ymax;

		ymax = zNear * (float)tan( fovy * 3.1415962f / 360.0 );
		ymin = -ymax;
		xmin = ymin * aspect;
		xmax = ymax * aspect;

		aMatrix_[0*4 + 0] = ( 2.0f * zNear ) / ( xmax - xmin );
		aMatrix_[1*4 + 0] =  0.0f;
		aMatrix_[2*4 + 0] = ( xmax + xmin ) / ( xmax - xmin );
		aMatrix_[3*4 + 0] =  0.0f;
		aMatrix_[0*4 + 1] =  0.0f;
		aMatrix_[1*4 + 1] = ( 2.0f * zNear ) / ( ymax - ymin );
		aMatrix_[2*4 + 1] = ( ymax + ymin ) / ( ymax - ymin );
		aMatrix_[3*4 + 1] =  0.0f;
		aMatrix_[0*4 + 2] =  0.0f;
		aMatrix_[1*4 + 2] =  0.0f;
		aMatrix_[2*4 + 2] = -( zFar + zNear ) / ( zFar - zNear );
		aMatrix_[3*4 + 2] = -( 2.0f * zFar * zNear ) / ( zFar - zNear );
		aMatrix_[0*4 + 3] =  0.0f;
		aMatrix_[1*4 + 3] =  0.0f;
		aMatrix_[2*4 + 3] = -1.0f;
		aMatrix_[3*4 + 3] =  0.0f;
	}
	return 1;
}

/* ----------------------------------------------------------------------------- */

int	atSetOrthoMatrix	(float* aMatrix_, float left, float right,
						 float bottom, float top,
						 float zNear, float zFar )
{
	if(aMatrix_ == 0) return 0;

	{
		aMatrix_[0*4 + 0] =  2.0f / ( right - left );
		aMatrix_[1*4 + 0] =  0.0f;
		aMatrix_[2*4 + 0] =  0.0f;
		aMatrix_[3*4 + 0] =  - (right + left) / (right - left);
		aMatrix_[0*4 + 1] =  0.0f;
		aMatrix_[1*4 + 1] =  2.0f / ( top - bottom );
		aMatrix_[2*4 + 1] =  0.0f;
		aMatrix_[3*4 + 1] =  - (top + bottom) / (top - bottom);
		aMatrix_[0*4 + 2] =  0.0f;
		aMatrix_[1*4 + 2] =  0.0f;
		aMatrix_[2*4 + 2] = -2.0f * ( zFar - zNear );
		aMatrix_[3*4 + 2] = - (zFar + zNear) / (zFar - zNear);
		aMatrix_[0*4 + 3] =  0.0f;
		aMatrix_[1*4 + 3] =  0.0f;
		aMatrix_[2*4 + 3] =  0.0f;
		aMatrix_[3*4 + 3] =  1.0f;
	}
	return 1;
}

/* ----------------------------------------------------------------------------- */

/**
 * As above but leaves 0,0,0 at the center instead of lower-left-front.
 */
int	atSetOrthoZeroAtCenterMatrix	(float* aMatrix_, float left, float right,
						 float bottom, float top,
						 float zNear, float zFar )
{
	if(aMatrix_ == 0) return 0;

	{
		aMatrix_[0*4 + 0] =  2.0f / ( right - left );
		aMatrix_[1*4 + 0] =  0.0f;
		aMatrix_[2*4 + 0] =  0.0f;
		aMatrix_[3*4 + 0] =  0.0f;
		aMatrix_[0*4 + 1] =  0.0f;
		aMatrix_[1*4 + 1] =  2.0f / ( top - bottom );
		aMatrix_[2*4 + 1] =  0.0f;
		aMatrix_[3*4 + 1] =  0.0f;
		aMatrix_[0*4 + 2] =  0.0f;
		aMatrix_[1*4 + 2] =  0.0f;
		aMatrix_[2*4 + 2] = -2.0f * ( zFar - zNear );
		aMatrix_[3*4 + 2] =  0.0f;
		aMatrix_[0*4 + 3] =  0.0f;
		aMatrix_[1*4 + 3] =  0.0f;
		aMatrix_[2*4 + 3] =  0.0f;
		aMatrix_[3*4 + 3] =  1.0f;
	}
	return 1;
}

/* ----------------------------------------------------------------------------- */

/**
 * Catenate two strings returning a new zero-terminated string.
 * Caller is responsible for freeing the returned string.
 */
char* atStrCat(const char* const p1, const char* const p2)
{
    char* retStr;
    size_t retStrLen, p1Len, p2Len;
    
    p1Len = strlen(p1);
    p2Len = strlen(p2);
    retStrLen = p1Len + p2Len;
    retStr = atMalloc(retStrLen + 1, NULL); /* +1 for final NUL */
    if (retStr != NULL) {
        strncpy(retStr, p1, p1Len);
        strncpy(retStr + p1Len, p2, p2Len);
    }
    retStr[retStrLen] = '\0';
    
    assert(retStr[retStrLen-1] == 'x');

    return retStr;
}

/* ----------------------------------------------------------------------------- */
#if 0
/**
 * Select the most appropriate config according to the attributes used as parameter.
 * @param [in] eglDisplay Current display.
 * @param [in] aAttribs List of prefered attributes.
 * @param [out] pResult  Resulting configuration.
 * @return Returns HI_TRUE on success, HI_FALSE on failure.
 */
EGLBoolean
atGetAppropriateEGLConfig(EGLDisplay eglDisplay, const EGLint* aAttribs,
   						  EGLConfig* pResult)
{
    EGLint  i;
    EGLint  iNbConfigs;
    EGLint  iNbChosenConfigs;
    EGLConfig* pConfigs;

    /* No error should have occured until here */
    atAssert(EGL_SUCCESS == eglGetError());
	
	eglGetConfigs(eglDisplay, NULL, 0, &iNbConfigs);
    if ( 0 > iNbConfigs || EGL_SUCCESS != eglGetError()) {
        return 0;
    }

    pConfigs = (EGLConfig*)atMalloc(iNbConfigs * sizeof(EGLConfig), 0);
    if ( 0 == pConfigs ) {
        return EGL_FALSE;
    }

	eglChooseConfig(eglDisplay, aAttribs, pConfigs, iNbConfigs, &iNbChosenConfigs);
    if ( iNbChosenConfigs <= 0 || EGL_SUCCESS != eglGetError() ) {
        atFree(pConfigs, 0);
        return EGL_FALSE;
    }

    /*
     * Select the config in the list.
     */
    {
        EGLint  iNbBits;
        EGLConfig* pSelConfig;
        EGLConfig* pCurConfig;

        /* Initialize Sel values to the worst values so that even if eglGetConfigAttrib function fails, we are not impacted for the test */
        EGLint iSelBufType = 0;    /* Selected buffer type */
        EGLint iSelRGBBits = 0x7fffffff;  /* The selected buffer uses iSelRGBBits bits for the RGB components */
        EGLint iSelABits = 0x7fffffff;  /* The selected buffer uses iSelABits bits for the A components */

        EGLint iCurBufType;      /* Current buffer type */
        EGLint iCurRGBBits;      /* The current buffer uses iCurRGBBits bits for the RGB components */
        EGLint iCurABits;      /* The current buffer uses iCurABits bits for the A components */


        pSelConfig = pConfigs[0];
        eglGetConfigAttrib(eglDisplay, pSelConfig, EGL_COLOR_BUFFER_TYPE, &iSelBufType);
        eglGetConfigAttrib(eglDisplay, pSelConfig, EGL_RED_SIZE, &iNbBits); iSelRGBBits  = iNbBits;
        eglGetConfigAttrib(eglDisplay, pSelConfig, EGL_GREEN_SIZE, &iNbBits); iSelRGBBits += iNbBits;
        eglGetConfigAttrib(eglDisplay, pSelConfig, EGL_BLUE_SIZE, &iNbBits); iSelRGBBits += iNbBits;
        eglGetConfigAttrib(eglDisplay, pSelConfig, EGL_ALPHA_SIZE, &iSelABits);

        for( i = 1; i < iNbChosenConfigs; i++) {
            /* Initialize Cur values to the worst values so that even if eglGetConfigAttrib function fails, we are not impacted for the test */
            iCurBufType = 0;
            iCurRGBBits = 0x7fffffff;
            iCurABits = 0x7fffffff;

            pCurConfig = pConfigs[i];
            eglGetConfigAttrib(eglDisplay, pCurConfig, EGL_COLOR_BUFFER_TYPE, &iCurBufType);
            eglGetConfigAttrib(eglDisplay, pCurConfig, EGL_RED_SIZE, &iNbBits); iCurRGBBits  = iNbBits;
            eglGetConfigAttrib(eglDisplay, pCurConfig, EGL_GREEN_SIZE, &iNbBits); iCurRGBBits += iNbBits;
            eglGetConfigAttrib(eglDisplay, pCurConfig, EGL_BLUE_SIZE, &iNbBits); iCurRGBBits += iNbBits;
            eglGetConfigAttrib(eglDisplay, pCurConfig, EGL_ALPHA_SIZE, &iCurABits);

            if ( ( (EGL_RGB_BUFFER == iCurBufType) && (EGL_RGB_BUFFER != iSelBufType) ) /* TODO: check that EGL_RGB_BUFFER is really what we want */
                 || (iCurRGBBits < iSelRGBBits)
                 || ( (iCurRGBBits == iSelRGBBits) && (iCurABits < iSelABits)))
            {
                pSelConfig = pCurConfig;
                iSelBufType = iCurBufType;
                iSelRGBBits = iCurRGBBits;
                iSelABits = iCurABits;
            }
        }

        atAssert(pSelConfig)
        *pResult = pSelConfig;
    }

    atFree(pConfigs, 0);

    /* EGL might have produced an error but it is handled by the code above, so just flush it, in case */
    eglGetError();

    return EGL_TRUE;
}

#endif
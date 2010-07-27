/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/**
 * @file	at.h
 * @brief	Simple interface to build applications using renderion.
 *
 * $Revision$
 * $Date::                            $
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

#ifndef __AT_H__
#define __AT_H__

/* ----------------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------------------- */

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>

/* ----------------------------------------------------------------------------- */

#include <malloc.h>	
#include <memory.h>	
#include <string.h>	
#include <assert.h>

/* ----------------------------------------------------------------------------- */

#define	AT_SURFACE_WIDTH	320
#define	AT_SURFACE_HEIGHT	240

/** EGL configuration used. */
static const EGLint configAttribs[] =
{
    EGL_LEVEL,				0,
	EGL_NATIVE_RENDERABLE,	EGL_FALSE,

    EGL_RED_SIZE,			5,
    EGL_GREEN_SIZE,			6,
    EGL_BLUE_SIZE,			5,
    EGL_ALPHA_SIZE,			EGL_DONT_CARE,
    EGL_LUMINANCE_SIZE,		EGL_DONT_CARE,

    EGL_DEPTH_SIZE,			16,
    EGL_STENCIL_SIZE,		EGL_DONT_CARE,
    EGL_SURFACE_TYPE,		EGL_WINDOW_BIT,
	EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES_BIT,
    EGL_NONE
};

/*
 * Select the most appropriate config according to the attributes used as parameter.
 */
EGLBoolean
atGetAppropriateEGLConfig(EGLDisplay eglDisplay, const EGLint* aAttribs,
   						  EGLConfig* pResult);

/* ----------------------------------------------------------------------------- */


/** Allocate a space of _size bytes. */
#define atMalloc(_size, _iAllocator)		malloc(_size)
/** Free a space previously allocated by aleMalloc. */
#define atFree(_ptr, _iAllocator)			free(_ptr)
/** Free a pointer only if valid and set it to null. */
#define atSafeFree(_ptr, _iAllocator)		{ if(_ptr){ atFree(_ptr, _iAllocator); (_ptr) = 0; } }
/** Assertion */
#define atAssert(_cond)				assert(_cond);

/* ----------------------------------------------------------------------------- */

typedef 	signed char 					tS8;
typedef 	unsigned char 				tU8;
typedef 	signed short 					tS16;
typedef 	unsigned short 				tU16;
typedef 	signed long 					tS32;
typedef 	unsigned long 				tU32;
typedef 	tS32 						tEnum;
typedef 	tU8 						tBool;
typedef 	float 						tFloat;
typedef 	tS32 						tFixed;

/* ----------------------------------------------------------------------------- */

typedef void	(*atPFInitialize)(void**		ppAppData);
typedef void	(*atPFRelease)	(void*		pAppData);
typedef void	(*atPFResize)	(void*      pAppData, int iWidth, int iHeight);
typedef void	(*atPFRun)		(void*		pAppData, int iTimeMS);

/** A single sample. */
typedef struct atSample_def {
	atPFInitialize pfInitialize;
	atPFRelease pfRelease;
	atPFResize pfResize;
	atPFRun pfRun;
} atSample;

/* ----------------------------------------------------------------------------- */

/*
 * Utility functions. Please DO NOT USE them in a real product.
 * Those functions are not here as a reference; they have limitations
 * in term of functionality (e.g. division by zero are not handled).
 */

/* Setup a view matrix. This function uses a constant UP vector of (0.0f,1.0f,0.0f)
 * to build the frenet basis; if |at-eye| is colinear to UP, the function will fail.
 */
int	atSetViewMatrix (tFloat* aMatrix_, tFloat eyex, tFloat eyey, tFloat eyez,
							tFloat atx, tFloat aty, tFloat atz);

int	atSetProjectionMatrix	(tFloat* aMatrix_, tFloat fovy, tFloat aspect, tFloat zNear, tFloat zFar );

/* ----------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

/* ----------------------------------------------------------------------------- */

#endif /*__AT_H__*/


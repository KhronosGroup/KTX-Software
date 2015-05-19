/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/* $Id$ */

/**
 * @file	at.h
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

#ifndef __AT_H__
#define __AT_H__

/* ----------------------------------------------------------------------------- */

#include <SDL2/sdl.h>

/* ----------------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------------------- */

//#if KTX_OPENGL
//  #define EGLAPI  // With OpenGL we use an emulator in a local file not libEGL.dll.
//#endif
//#include <EGL/egl.h>
//#include <EGL/eglext.h>

/* ----------------------------------------------------------------------------- */

//#include <malloc.h>
#include <memory.h>	
#include <string.h>	
#include <assert.h>

/* ----------------------------------------------------------------------------- */

#define	AT_SURFACE_WIDTH	320
#define	AT_SURFACE_HEIGHT	240


/*
 * Select the most appropriate config according to the attributes used as parameter.
 */
//EGLBoolean
//atGetAppropriateEGLConfig(EGLDisplay eglDisplay, const EGLint* aAttribs,
//   						  EGLConfig* pResult);

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

typedef 	signed char 				tS8;
typedef 	unsigned char 				tU8;
typedef 	signed short 				tS16;
typedef 	unsigned short 				tU16;
typedef 	signed long 				tS32;
typedef 	unsigned long 				tU32;
typedef 	tS32 						tEnum;
typedef 	tU8 						tBool;
typedef 	float 						tFloat;
typedef 	tS32 						tFixed;

/* ----------------------------------------------------------------------------- */

typedef void	(*atPFInitialize)(void**	ppAppData, const char* const args);
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

//#define ATE_LEFT_ARROW		0x00000001
//#define ATE_RIGHT_ARROW		0x00000002
//#define ATE_UP_ARROW		0x00000003
//#define ATE_DOWN_ARROW		0x00000004
//#define ATE_ENTER			0x00000005
//#define ATE_LBUTTON			0x00000006
//#define ATE_RBUTTON			0x00000007

//#define ATE_NUM_SUPPORTED_EVENTS 0x7

//typedef void (*atPFHandleEvent)(void* pAppData, unsigned int uEvent, int iPressed);

//atPFHandleEvent atSetEventHandler(unsigned int uEvent, atPFHandleEvent pfHandle);

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

/* Set a perspective projection matrix */
int	atSetProjectionMatrix (tFloat* aMatrix_, tFloat fovy, tFloat aspect, tFloat zNear, tFloat zFar );

/* Set an orthographic projection matrix. */
int	atSetOrthoMatrix (tFloat* aMatrix_, tFloat left, tFloat right,
					  tFloat bottom, tFloat top, tFloat zNear, tFloat zFar );

/* Set an orthographic projection matrix that retains 0 at the center. */
int	atSetOrthoZeroAtCenterMatrix (tFloat* aMatrix_, tFloat left, tFloat right,
					              tFloat bottom, tFloat top, tFloat zNear, tFloat zFar );

/* ----------------------------------------------------------------------------- */

/* Platform independent interface to a message box function */
#define atMessageBox(message, caption, type) \
    SDL_ShowSimpleMessageBox(type, caption, message, NULL)

/* message box types */
#define AT_MB_OK		0x00000000
#define AT_MB_OKCANCEL	0x00000000
#define AT_MB_ICONINFO	SDL_MESSAGEBOX_INFORMATION
#define AT_MB_ICONERROR	SDL_MESSAGEBOX_ERROR

/* ----------------------------------------------------------------------------- */

extern float atIdentity[16];

/* ----------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

/* ----------------------------------------------------------------------------- */

#endif /*__AT_H__*/


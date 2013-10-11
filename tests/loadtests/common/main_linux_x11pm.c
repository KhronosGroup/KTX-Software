/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/**
 * @file	main_linux_x11pm.c
 * @brief	Main loop, draw inside an egl pixmap surface.
 *          Blit to the screen using X11.
 * 		    The EGL configuration is RGB565 with a 16-bit depth-buffer.
 *
 * @warning	THIS SAMPLE IS NOT OPTIMIZED AND IS NOT A REFERENCE ON USING
 *          RENDERION OR ANY OTHER OPENGL IMPLEMENTATION ON X11 WINDOW SYSTEM. 
 *		    DO NOT USE IN A REAL PRODUCT.
 *
 * @warning This code has not been used with the current KTX load tests.
 *          It is provided as a helpful starting point for a Linux port.
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

/* ------------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>

#include <signal.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <termios.h>
#include <sys/types.h>
#include <sys/time.h>

#include "sample.h"

/* ------------------------------------------------------------------------- */

#define ERR_XI_OK             0x00
#define ERR_XI_DISPLAY        0x02
#define ERR_XI_BADDEPTH       0x03
#define ERR_XI_WINDOW         0x04
#define ERR_XI_VIRTALLOC      0x05
#define ERR_XI_XIMAGE         0x06
#define ERR_XI_PIXMAP         0x07

typedef struct {
	Display*	display;
	Window		window;
	Pixmap		pixmap;
	GC			gc;
	int 		width;
	int 		height;
	int 		depth;
} XWindow;

/* ------------------------------------------------------------------------- */

static 			EGLDisplay 			g_eglDisplay;
static 			EGLSurface 			g_eglSurface;
static 			EGLContext 			g_eglContext;

static			int					g_iInfiniteLoop = 1;
static			XWindow				g_XWindow;

/* ------------------------------------------------------------------------- */

static void InitializeEGL()
{
	EGLBoolean bResult = EGL_FALSE;
	EGLConfig pkConfig;
	EGLint iNumConfigs = 0;
	EGLint iTotalNumConfigs = 0;
	EGLint iScreenWidth, iScreenHeight;
	EGLint iEGLVersionMajor;
	EGLint iEGLVersionMinor;

	atAssert(g_XWindow.window != NULL);

	g_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	atAssert(g_eglDisplay != NULL);

	bResult = eglInitialize(g_eglDisplay, &iEGLVersionMajor, &iEGLVersionMinor);
	atAssert(bResult == EGL_TRUE);

	bResult = atGetAppropriateEGLConfig(g_eglDisplay, configAttribs, &pkConfig);
	atAssert(bResult);

	g_eglSurface = eglCreatePixmapSurface(g_eglDisplay, pkConfig, (EGLNativePixmapType)&g_XWindow.pixmap, NULL);

	atAssert(g_eglSurface != NULL);

	g_eglContext = eglCreateContext(g_eglDisplay, pkConfig, EGL_NO_CONTEXT, NULL);
	atAssert(g_eglContext != NULL);

	bResult = eglMakeCurrent(g_eglDisplay, g_eglSurface, g_eglSurface, g_eglContext);
	atAssert(bResult == EGL_TRUE);

	eglQuerySurface(g_eglDisplay, g_eglSurface, EGL_WIDTH, &iScreenWidth);
	eglQuerySurface(g_eglDisplay, g_eglSurface, EGL_HEIGHT, &iScreenHeight);
	atAssert(AT_SURFACE_WIDTH == iScreenWidth);
	atAssert(AT_SURFACE_HEIGHT == iScreenHeight);
}

static void TerminateEGL(void)
{
	EGLBoolean bResult = EGL_FALSE;

    bResult = eglMakeCurrent(g_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	atAssert(bResult == EGL_TRUE);

	bResult = eglDestroyContext(g_eglDisplay, g_eglContext);
	atAssert(bResult == EGL_TRUE);
	g_eglContext = NULL;

	bResult = eglDestroySurface(g_eglDisplay, g_eglSurface);
	atAssert(bResult == EGL_TRUE);
	g_eglSurface = NULL;

	bResult = eglTerminate(g_eglDisplay);
	atAssert(bResult == EGL_TRUE);
	g_eglDisplay = NULL;
}

/* ------------------------------------------------------------------------- */
/* TIMER ------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
static unsigned long long	s_timeStart;
static float s_fFPSTimeStart;
static int s_iFPSFrames;

static void timer_initialize(void)
{
	struct timeval	tv;
	gettimeofday(&tv, NULL);
	
	s_timeStart = ((unsigned long long)tv.tv_sec*1000000 + tv.tv_usec);
	s_fFPSTimeStart = 0.0f;
	s_iFPSFrames = 0;
}
static float timer_gettime(void)
{
	unsigned long long	timeCurrent;
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	timeCurrent = ((unsigned long long)tv.tv_sec*1000000 + tv.tv_usec);

	return (float)(timeCurrent - s_timeStart) * 1.0e-3;
}
static int timer_fps(void)
{
	float	fFPSTimeCurrent = timer_gettime();

	s_iFPSFrames++;

	if( fFPSTimeCurrent-s_fFPSTimeStart > 5000.0f )
	{
		float fFPS = (s_iFPSFrames * 1000) / (fFPSTimeCurrent - s_fFPSTimeStart);
		printf( "%.2f\n", fFPS);

		s_fFPSTimeStart = fFPSTimeCurrent;
		s_iFPSFrames	= 0;
	}

	return (int)fFPSTimeCurrent;
}

/* ------------------------------------------------------------------------- */
/* X11 --------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

int
CreateXWindow( XWindow *XWnd,
			int width,
			int height,
			int bpp, /* put bpp=-1 to run on every bpp displays */
			char *title)
{
	int screennum;
	Screen* screenptr;
	Visual *visual;
	XSizeHints Hints;

	XWnd->width = width;
	XWnd->height = height;

	XWnd->display = XOpenDisplay(NULL);
	if(!XWnd->display)
		return ERR_XI_DISPLAY;

	screennum	= DefaultScreen(XWnd->display);
	screenptr	= DefaultScreenOfDisplay(XWnd->display);
	visual		= DefaultVisualOfScreen(screenptr);
	XWnd->depth = DefaultDepth(XWnd->display, screennum);

	if(bpp!=-1 && XWnd->depth!=bpp)
		return ERR_XI_BADDEPTH;

	XWnd->window = XCreateWindow(XWnd->display,
								RootWindowOfScreen(screenptr),
								0,
								0,
								XWnd->width,
								XWnd->height,
								0,
								XWnd->depth,
								InputOutput,
								visual,
								0,
								NULL);
	if(!XWnd->window)
		return ERR_XI_WINDOW;

	Hints.flags = PSize|PMinSize|PMaxSize;
	Hints.min_width = Hints.max_width = Hints.base_width = width;
	Hints.min_height = Hints.max_height = Hints.base_height = height;

	XSetWMNormalHints(XWnd->display,XWnd->window,&Hints);
	XStoreName(XWnd->display,XWnd->window,title);

	XSelectInput(XWnd->display, XWnd->window, ExposureMask|KeyPressMask|KeyReleaseMask|ButtonPressMask);

	XMapRaised(XWnd->display,XWnd->window);

	XFlush(XWnd->display);

	return ERR_XI_OK;
}

/* ------------------------------------------------------------------------- */

void
DestroyXWindow(XWindow *XWnd)
{
	if(XWnd && XWnd->display && XWnd->window)
	{
		XDestroyWindow(XWnd->display,XWnd->window);
		XCloseDisplay(XWnd->display);
	}
}

/* ------------------------------------------------------------------------- */

int
CreatePixmap( XWindow *XWnd,
			int width,
			int height )
{
	XWnd->gc = DefaultGC(XWnd->display, DefaultScreen(XWnd->display));

	XWnd->pixmap = XCreatePixmap(XWnd->display,
								DefaultRootWindow(XWnd->display),
								width,
								height,
								XWnd->depth);
	if(!XWnd->pixmap)
		return ERR_XI_PIXMAP;

	XFlush(XWnd->display);

	return ERR_XI_OK;
}

void
DestroyPixmap(XWindow *XWnd)
{
	if(XWnd->pixmap)
	{
		XFreePixmap(XWnd->display, XWnd->pixmap);
	}
}

/* ------------------------------------------------------------------------- */
/* MAIN -------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

static void OnBreak(int Arg){	g_iInfiniteLoop = 0;	}
static void ClearKBuf(void) {
	XEvent ev;
	while( XCheckWindowEvent(g_XWindow.display, g_XWindow.window, KeyPressMask, &ev) ) {
		XLookupKeysym((XKeyEvent *)&ev, 0);  
	}
}

/* --- MAIN LOOP --- */
int main(int argc, char** argv)
{
	int res = 0;
	int iTime = 0;
	void* pSampleData;
	int	iSampleIndex;
	int iNbSamples = sizeof(sc_aSamples) / sizeof(const atSample*);

	printf( "linux x11 PixmapSurface: starting\n");
	printf( "Press 'q' to quit or any key to go to the next demo. \n");

	if( 0 != (res = CreateXWindow(&g_XWindow, AT_SURFACE_WIDTH, AT_SURFACE_HEIGHT, -1, "rt_sample PixmapSurface")) )
	{
		printf("Error %d \n", res);
		return 1;
	}

	printf( "XWindow %dbpp\n", g_XWindow.depth);

	if( 0 != (res = CreatePixmap(&g_XWindow, AT_SURFACE_WIDTH, AT_SURFACE_HEIGHT)) )
	{
		printf("Error %d \n", res);
		return 1;
	}

	signal(SIGHUP,	OnBreak);
	signal(SIGINT,	OnBreak);
	signal(SIGQUIT, OnBreak);
	signal(SIGTERM, OnBreak);

	InitializeEGL();
	timer_initialize();

	/* By default use the first sample. */
	iSampleIndex = 0;
	sc_aSamples[iSampleIndex]->pfInitialize(&pSampleData);
	sc_aSamples[iSampleIndex]->pfResize( pSampleData, AT_SURFACE_WIDTH, AT_SURFACE_HEIGHT );

	while(g_iInfiniteLoop)
    {
		XEvent ev;
		long lKey;

		sc_aSamples[iSampleIndex]->pfRun(pSampleData, iTime);

		eglCopyBuffers(g_eglDisplay, g_eglSurface, (EGLNativePixmapType)g_XWindow.pixmap );

		XCopyArea(g_XWindow.display, g_XWindow.pixmap, g_XWindow.window, g_XWindow.gc, 0, 0, g_XWindow.width, g_XWindow.height, 0, 0);

		while(XPending(g_XWindow.display))
		{
			XNextEvent(g_XWindow.display, &ev);
			if( KeyPress == ev.type)
			{
				lKey = XLookupKeysym((XKeyEvent *)&ev,0);
				switch(lKey)
				{
				case XK_Escape:
				case XK_Q:
				case XK_q:
					/* Quit the demo */
					g_iInfiniteLoop = 0;
					break;

				default:
					/* Go to the next demo */
					sc_aSamples[iSampleIndex]->pfRelease(pSampleData);

					iSampleIndex++;
					if(iSampleIndex >= iNbSamples) iSampleIndex = 0;

					sc_aSamples[iSampleIndex]->pfInitialize(&pSampleData);
					sc_aSamples[iSampleIndex]->pfResize( pSampleData, AT_SURFACE_WIDTH, AT_SURFACE_HEIGHT );
					break;
				}
				ClearKBuf();
			}
		}

		/* fps update / draw */
		iTime = timer_fps();
	}

	sc_aSamples[iSampleIndex]->pfRelease(pSampleData);

 	/* Cleanup */
	TerminateEGL();		
	DestroyPixmap(&g_XWindow);
	DestroyXWindow(&g_XWindow);

	printf( "linux x11 PixmapSurface: finishing \n");
	return 0;
}

/* ------------------------------------------------------------------------- */



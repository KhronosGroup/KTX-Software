/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/**
 * @file	main_linux_fbdev.c
 * @brief	Main loop, draw inside a Renderion client-pixmap (HI extension).
 *		Blit to the screen using fbdev.
 * 		The EGL configuration is RGB565 with a 16Bit depth-buffer.
 *
 * @version $Revision$ on $Date::                            $
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

#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>

#include <termios.h>
#include <sys/types.h>
#include <sys/time.h>

#include "sample.h"

/* ------------------------------------------------------------------------- */

static 			EGLDisplay 			g_eglDisplay;
static 			EGLSurface 			g_eglSurface;
static 			EGLContext 			g_eglContext;
static struct 	EGLClientPixmapHI 	g_pixmapClient;

/** Framebuffer data. */
typedef struct fbData_def
{
/* public: */
	struct EGLClientPixmapHI pixmap;

/* private: */
	int fb;
	char *fbp;
	long int screensize;
} fbData;

/* ------------------------------------------------------------------------- */

static void CreateHiPixmap(EGLConfig config, struct EGLClientPixmapHI* hi_pixmap)
{
	EGLint		r, g, b, a, iColorFormat;
	EGLBoolean	bRes;

	bRes = eglGetConfigAttrib(g_eglDisplay, config, EGL_RED_SIZE, &r);
	atAssert(bRes && r == 5);
	bRes = eglGetConfigAttrib(g_eglDisplay, config, EGL_GREEN_SIZE, &g);
	atAssert(bRes && g == 6);
	bRes = eglGetConfigAttrib(g_eglDisplay, config, EGL_BLUE_SIZE, &b);
	atAssert(bRes && b == 5);
	bRes = eglGetConfigAttrib(g_eglDisplay, config, EGL_ALPHA_SIZE, &a);
	atAssert(bRes && a == 0);
	bRes = eglGetConfigAttrib(g_eglDisplay, config, EGL_COLOR_FORMAT_HI, &iColorFormat);
	atAssert(bRes && iColorFormat == EGL_COLOR_RGB_HI);

	hi_pixmap->iWidth  = AT_SURFACE_WIDTH;
	hi_pixmap->iHeight = AT_SURFACE_HEIGHT;
	hi_pixmap->iStride = AT_SURFACE_WIDTH;
	hi_pixmap->pData   = malloc(AT_SURFACE_WIDTH * AT_SURFACE_HEIGHT * 2);
	atAssert(hi_pixmap->pData);
}

static void ReleaseHiPixmap(struct EGLClientPixmapHI* hi_pixmap)
{
	free(hi_pixmap->pData);
}

/* ------------------------------------------------------------------------- */

static void InitializeEGL()
{
	EGLBoolean bResult = EGL_FALSE;
	EGLConfig *ppkConfig;
	EGLint iNumConfigs = 0;
	EGLint iTotalNumConfigs = 0;
	EGLint iScreenWidth, iScreenHeight;
	EGLint iEGLVersionMajor;
	EGLint iEGLVersionMinor;

	g_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	atAssert(g_eglDisplay != NULL);

	bResult = eglInitialize(g_eglDisplay, &iEGLVersionMajor, &iEGLVersionMinor);
	atAssert(bResult == EGL_TRUE);

	bResult = atGetAppropriateEGLConfig(g_eglDisplay, configAttribs, &pkConfig);
	atAssert(bResult);

	CreateHiPixmap(pkConfig, &g_pixmapClient);

	g_eglSurface = eglCreatePixmapSurfaceHI(g_eglDisplay, pkConfig, &g_pixmapClient);
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

	ReleaseHiPixmap(&g_pixmapClient);

	bResult = eglTerminate(g_eglDisplay);
	atAssert(bResult == EGL_TRUE);
	g_eglDisplay = NULL;
}

/* ------------------------------------------------------------------------- */


/* --- KEYBOARD --- */
static void kb_changemode(int dir)
{
	static struct termios oldt, newt;

	if ( dir == 1 )
	{
		tcgetattr( STDIN_FILENO, &oldt);
		newt = oldt;
		newt.c_lflag &= ~( ICANON | ECHO );
		tcsetattr( STDIN_FILENO, TCSANOW, &newt);
	}
	else
		tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
}

static int kb_hit (void)
{
	struct timeval tv;
	fd_set rdfs;

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	FD_ZERO(&rdfs);
	FD_SET (STDIN_FILENO, &rdfs);

	select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
	return FD_ISSET(STDIN_FILENO, &rdfs);

}

/* --- FRAME-BUFFER --- */
static int fb_initialize(fbData* pFBData)
{
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;

	atAssert(pFBData != 0);
	pFBData->fb = 0;
	pFBData->fbp = 0;
	pFBData->screensize = 0;

	pFBData->pixmap.iWidth = 0;
	pFBData->pixmap.iHeight = 0;
	pFBData->pixmap.iStride = 0;
	pFBData->pixmap.pData = 0;

	
	pFBData->fb = open("/dev/fb0", O_RDWR);
	if (!pFBData->fb)
	{   printf("Fail to open framebuffer device.\n"); return(1);   }

	if (ioctl(pFBData->fb, FBIOGET_FSCREENINFO, &finfo))
	{   printf("Error reading fixed information.\n"); return(2);   }

	if (ioctl(pFBData->fb, FBIOGET_VSCREENINFO, &vinfo))
	{   printf("Error reading variable information.\n"); return(3); }


	printf("Display information: \n");
	printf("   vinfo.xres: %d\n", vinfo.xres);
	printf("   vinfo.yres: %d\n", vinfo.yres);
	printf("   finfo.line_length: %d\n", finfo.line_length);
	printf("   vinfo.bits_per_pixel: %d\n", vinfo.bits_per_pixel);
	printf("   vinfo.xoffset: %d\n", vinfo.xoffset);
	printf("   vinfo.yoffset: %d\n", vinfo.yoffset);

	pFBData->screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
	pFBData->fbp = (char *)mmap(0, pFBData->screensize, PROT_READ | PROT_WRITE, MAP_SHARED, pFBData->fb, 0);
	if ((int)pFBData->fbp == -1)
	{ printf("Fail to map framebuffer device to memory.\n"); return(4); }

	if( 16 != vinfo.bits_per_pixel )
	{ printf("Unsupported framebuffer format: this sample uses 16 bpp.\n"); return(8); }

	{
		int location;
		unsigned short* pFBPtr;
		int byte_per_pixel = vinfo.bits_per_pixel / 8;

		location = vinfo.xoffset * byte_per_pixel + vinfo.yoffset * finfo.line_length;
		pFBPtr = (unsigned short*)(pFBData->fbp + location);
	
		pFBData->pixmap.iWidth = AT_SURFACE_WIDTH;
		pFBData->pixmap.iHeight = AT_SURFACE_HEIGHT;
		pFBData->pixmap.iStride = finfo.line_length / byte_per_pixel;
		pFBData->pixmap.pData = pFBPtr;
	}

	return(0);
}

static void fb_terminate(fbData* pFBData)
{
	atAssert(pFBData != 0);
	munmap(pFBData->fbp, pFBData->screensize);
	close(pFBData->fb);
}

/* --- TIMER --- */
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


/* --- MAIN LOOP --- */
int main(int argc, char** argv)
{
	fbData fb;
	int res = 0;
	int key = 0;
	int iTime = 0;
	void* pSampleData;
	int	iSampleIndex;
	int iNbSamples = sizeof(sc_aSamples) / sizeof(const atSample*);

	printf( "linux fbdev: starting\n");
	printf( "Press 'q' to quit or any key to go to the next demo. \n");

#ifdef RENDERION_STATICLIB
	if( 0 == brStaticLibInitialize() )
	{
		return -1;
	}
#endif /* RENDERION_STATICLIB */	

	if( 0 != (res = fb_initialize(&fb)) ) {
		return res;
	}
	kb_changemode(1);

	InitializeEGL();
	timer_initialize();

	/* By default use the first sample. */
	iSampleIndex = 0;
	
	do
	{
		sc_aSamples[iSampleIndex]->pfInitialize(&pSampleData);
		sc_aSamples[iSampleIndex]->pfResize( pSampleData, AT_SURFACE_WIDTH, AT_SURFACE_HEIGHT );

		while( !kb_hit() )
		{
			sc_aSamples[iSampleIndex]->pfRun(pSampleData, iTime);

			/* blit : the surface in vertically swapped, we swap it here.
			 * Another solution could be to set the g_pixmapClient.iHeight
			 * value to -g_pixmapClient.iHeight.
			 * Or we could also modify the plateform library so that the 
			 * bInverse member of the bpSurface returned when creating the
			 * surface is TRUE.
 			 */
			{	
				unsigned short* pDst;
				unsigned short* pSrc;
				int y;
				int iWidth = fb.pixmap.iWidth;
				int iHeight = fb.pixmap.iHeight;
				int iStrideDst = fb.pixmap.iStride;
				int iStrideSrc = g_pixmapClient.iStride;
				atAssert(iWidth == g_pixmapClient.iWidth);
				atAssert(iHeight == g_pixmapClient.iHeight);
				
				pDst = (unsigned short*)fb.pixmap.pData;
				pSrc = (unsigned short*)g_pixmapClient.pData;

				pDst += (iHeight-1) * iStrideDst;

				for(y = iHeight; y > 0; y--)
				{
					memcpy(pDst, pSrc, iWidth<<1);
					pDst -= iStrideDst;
					pSrc += iStrideSrc;
				}
			}

			/* fps update / draw */
			iTime = timer_fps();
		}

		sc_aSamples[iSampleIndex]->pfRelease(pSampleData);

		key = getchar();

		iSampleIndex++;
		if(iSampleIndex >= iNbSamples) iSampleIndex = 0;

	} while( 'q' != key );
	

 	/* Cleanup */
	TerminateEGL();		
	kb_changemode(0);
	fb_terminate(&fb);

#ifdef RENDERION_STATICLIB
	if( 0 == brStaticLibTerminate() )
	{
		return -2;
	}
#endif /* RENDERION_STATICLIB */

	printf( "linux fbdev: finishing \n");
	return 0;
}

/* ------------------------------------------------------------------------- */



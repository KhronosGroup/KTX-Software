/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/**
 *
 * @file	main_win32.c
 * @brief	Main loop for win32 / winCE/ win-mobile, draw inside an 
 *			egl window surface.
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

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

#include "sample.h"

/* ------------------------------------------------------------------------- */

static HINSTANCE		g_hInst = NULL;
static HWND				g_hWnd  = NULL;

static EGLDisplay		g_eglDisplay;
static EGLSurface		g_eglSurface;
static EGLContext		g_eglContext;

static int				g_iScreenWidth;
static int				g_iScreenHeight;

static int				g_iSampleIndex = -1;
static void*			g_pSampleData;

/* ------------------------------------------------------------------------- */

#if UNDER_CE
	#define LPCMDLINE LPTSTR

	static void DrawFPS(float fFPS)
	{
		TCHAR buff[16];
		swprintf(buff, TEXT("%.2f"), fFPS);
		SetWindowText(g_hWnd, buff);
	}

#else
	#define LPCMDLINE LPSTR

	static void DrawFPS(float fFPS)
	{
		TCHAR buff[16];

#if defined(__CYGWIN__)
#include <locale.h>    /* setlocale */
		CHAR buff_s[32];
		CHAR *p   = buff_s;
		TCHAR *pw = buff;
		int len, length;
		sprintf((char*)buff_s, "%.2f", fFPS);

		setlocale(LC_ALL, "jpn");
		while ((len=mblen(p, MB_CUR_MAX)) > 0) {
			mbtowc(pw, p, len);
			p += len;
			++pw;
		}
		length = strlen(buff_s);
		buff[length]  =0;
#else
		swprintf(buff, 16, TEXT("%.2f"), fFPS);
#endif /* __CYGWIN__ */

		SetWindowText(g_hWnd, buff);
	}

#endif /* UNDER_CE*/

/* ------------------------------------------------------------------------- */

static void InitializeEGL(NativeWindowType hwnd)
{
	EGLBoolean bResult = EGL_FALSE;
	EGLConfig pkConfig;
	EGLint iNumConfigs = 0;
	EGLint iTotalNumConfigs = 0;
	EGLint iEGLVersionMajor;
	EGLint iEGLVersionMinor;

	atAssert(hwnd != NULL);

	g_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	atAssert(g_eglDisplay != NULL);

	bResult = eglInitialize(g_eglDisplay, &iEGLVersionMajor, &iEGLVersionMinor);
	atAssert(bResult == EGL_TRUE);

	bResult = atGetAppropriateEGLConfig(g_eglDisplay, configAttribs, &pkConfig);
	atAssert(bResult);

	g_eglSurface = eglCreateWindowSurface(g_eglDisplay, pkConfig, hwnd, NULL);
	atAssert(g_eglSurface != NULL);

	g_eglContext = eglCreateContext(g_eglDisplay, pkConfig, EGL_NO_CONTEXT, NULL);
	atAssert(g_eglContext != NULL);
	bResult = eglMakeCurrent(g_eglDisplay, g_eglSurface, g_eglSurface,
			g_eglContext);
	atAssert(bResult == EGL_TRUE);

	eglQuerySurface(g_eglDisplay, g_eglSurface, EGL_WIDTH, &g_iScreenWidth);
	eglQuerySurface(g_eglDisplay, g_eglSurface, EGL_HEIGHT, &g_iScreenHeight);
}

/* ------------------------------------------------------------------------- */

static void TerminateEGL(void)
{
	EGLBoolean bResult = EGL_FALSE;

    bResult = eglMakeCurrent(g_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
			EGL_NO_CONTEXT);
	atAssert(bResult == EGL_TRUE);

	bResult = eglDestroyContext(g_eglDisplay, g_eglContext);
	atAssert(bResult == EGL_TRUE);
	g_eglContext = NULL;

	bResult = eglDestroySurface(g_eglDisplay, g_eglSurface);
	atAssert(bResult == EGL_TRUE);
	g_eglSurface = NULL;

	bResult = eglMakeCurrent(g_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
			EGL_NO_CONTEXT);
	atAssert(bResult == EGL_TRUE);

	bResult = eglTerminate(g_eglDisplay);
	atAssert(bResult == EGL_TRUE);
	g_eglDisplay = NULL;
}

/* ------------------------------------------------------------------------- */

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) 
    {
#if UNDER_CE
	case WM_ACTIVATE: /* Simply quit when the window is minimized. */
		if( WA_INACTIVE == wParam )
		{
			PostQuitMessage(0);
		}
		break;
#endif

    case WM_SIZE:
		if(g_iSampleIndex >= 0)
		{
			g_iScreenWidth = LOWORD(lParam);
			g_iScreenHeight = HIWORD(lParam);
			sc_aSamples[g_iSampleIndex]->pfResize(g_pSampleData, g_iScreenWidth, g_iScreenHeight);
		}
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

	case WM_LBUTTONDOWN:
		{
			int iNbSamples = sizeof(sc_aSamples) / sizeof(const atSample*);

			sc_aSamples[g_iSampleIndex]->pfRelease(g_pSampleData);

			g_iSampleIndex++;
			if(g_iSampleIndex >= iNbSamples) g_iSampleIndex = 0;

			sc_aSamples[g_iSampleIndex]->pfInitialize(&g_pSampleData);
			sc_aSamples[g_iSampleIndex]->pfResize(g_pSampleData, g_iScreenWidth, g_iScreenHeight);
		}
		break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

static BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    TCHAR szTitle[] = L"Sample";
    TCHAR szWindowClass[]  = L"Sample";
	WNDCLASS wc;
	GLint iScreenWidth;
	GLint iScreenHeight;
	int iStyle;

    g_hInst = hInstance;

	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = NULL;
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = szWindowClass;

    if (!RegisterClass(&wc))
    {
    	return FALSE;
    }

	iStyle = WS_VISIBLE;
#if UNDER_CE
	iScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	iScreenHeight = GetSystemMetrics(SM_CYSCREEN);
#else
	iStyle |=  WS_OVERLAPPEDWINDOW;
	iScreenWidth = AT_SURFACE_WIDTH;
	iScreenHeight = AT_SURFACE_HEIGHT;
#endif

    g_hWnd = CreateWindow(szWindowClass, szTitle, iStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, iScreenWidth, iScreenHeight, NULL, NULL, hInstance, NULL);

    if (!g_hWnd)
    {
        return FALSE;
    }

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    return TRUE;
}

/* ------------------------------------------------------------------------- */

/* --- TIMER --- */

static LARGE_INTEGER freq;
static LARGE_INTEGER cur;
static LARGE_INTEGER pre;

static float s_fFPSTimeStart;
static int s_iFPSFrames;

static void timer_initialize(void)
{
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&cur);
	pre = cur;

	s_fFPSTimeStart = 0.0f;
	s_iFPSFrames = 0;
}

static float timer_gettime(void)
{
	QueryPerformanceCounter(&cur);
	return (float)( cur.QuadPart - pre.QuadPart ) * 1000 / freq.QuadPart;
}

static int timer_fps(void)
{
	float	fFPSTimeCurrent = timer_gettime();

	s_iFPSFrames++;

	if( fFPSTimeCurrent-s_fFPSTimeStart > 5000.0f )
	{
		float fFPS = (s_iFPSFrames * 1000) / (fFPSTimeCurrent - s_fFPSTimeStart);
		DrawFPS(fFPS);

		s_fFPSTimeStart = fFPSTimeCurrent;
		s_iFPSFrames	= 0;
	}

	return (int)fFPSTimeCurrent;
}

/* ------------------------------------------------------------------------- */

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPCMDLINE lpCmdLine,
                   int       nCmdShow)
{
	MSG msg;
	int iTime = 0;

	if (!InitInstance(hInstance, nCmdShow)) 
	{
		return 1;
	}
	
	InitializeEGL(g_hWnd);
	timer_initialize();

	/* By default use the first sample. */
	g_iSampleIndex = 0;

	sc_aSamples[g_iSampleIndex]->pfInitialize(&g_pSampleData);
	sc_aSamples[g_iSampleIndex]->pfResize(g_pSampleData, g_iScreenWidth, g_iScreenHeight);

	for (;;)
	{
		if (PeekMessage(&msg, NULL, 0, 0, TRUE)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
			{
				break;
			}
		}
		else
		{
			sc_aSamples[g_iSampleIndex]->pfRun(g_pSampleData, iTime);

			eglSwapBuffers(g_eglDisplay,g_eglSurface);

			InvalidateRect(g_hWnd, NULL, FALSE);
		}

		/* fps update / draw */
		iTime = timer_fps();
	}

	sc_aSamples[g_iSampleIndex]->pfRelease(g_pSampleData);

	TerminateEGL();

	DestroyWindow(g_hWnd);

	return 0;
}

/* ------------------------------------------------------------------------- */

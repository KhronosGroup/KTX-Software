/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/**
 *
 * @file	main_win32.c
 * @brief	Main loop for win32 / winCE/ win-mobile, draw inside an 
 *			egl window surface.
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

#include "at.h"

#ifndef SHOW_GL_VERSION
#define SHOW_GL_VERSION 0
#endif

/* ------------------------------------------------------------------------- */

/* Don't want to include gl.h etc, in this file */
typedef unsigned int GLenum;

#if KTX_OPENGL
  // Have to use GLEW as there is no .lib with entry points > OpenGL 1.2
  #define GLEW_OK 0
  //GLEWAPI GLenum GLEWAPIENTRY glewInit (void);
  extern GLenum __stdcall glewInit(void);
#endif

#if SHOW_GL_VERSION
typedef unsigned char GLuchar;
GLuchar* __stdcall glGetString(GLenum string);
#define GL_VENDOR                                        0x1F00
#define GL_RENDERER                                      0x1F01
#define GL_VERSION                                       0x1F02
#define GL_EXTENSIONS                                    0x1F03
#endif


/* ------------------------------------------------------------------------- */


//extern const atSample* const gc_aSamples[];
extern const atSampleInvocation gc_aSamples[];
extern const int gc_iNumSamples;
extern const EGLint gc_eiConfigAttribs[];
extern const EGLint gc_eiContextAttribs[];

/* ------------------------------------------------------------------------- */

static HINSTANCE		g_hInst = NULL;
static HWND				g_hWnd  = NULL;

static EGLDisplay		g_eglDisplay;
static EGLSurface		g_eglSurface;
static int              polyfilla[200];
static EGLContext		g_eglContext;

static int				g_iScreenWidth;
static int				g_iScreenHeight;

static int				g_iSampleIndex = -1;
static void*			g_pSampleData;

static float			g_fFPS;

static atPFHandleEvent  g_pfEventHandlers[ATE_NUM_SUPPORTED_EVENTS];

typedef struct VersionInfo_def {
	const char* szEGLVendor; 
	const char* szEGLVersion;
	const char* szEGLExtensions;
	const char* szEGLClientAPIs;
	const char* szGLVendor;
	const char* szGLVersion;
	const char* szGLExtensions;
} VersionInfo;

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

	static void SetTitle()
	{
		TCHAR buff[100];
		char buff_s[100];
		char *p   = buff_s;
		TCHAR *pw = buff;
		int length;

		length = _snprintf_s(buff_s, sizeof(buff_s), sizeof(buff_s),
			                 "%s: %.2ffps", gc_aSamples[g_iSampleIndex].title, g_fFPS);
#if defined(__CYGWIN__)
#include <locale.h>    /* setlocale */
		setlocale(LC_ALL, "jpn");
		while ((length=mblen(p, MB_CUR_MAX)) > 0) {
			mbtowc(pw, p, len);
			p += length;
			++pw;
		}
		length = strlen(buff_s);
		buff[length] = 0;
#else
		mbstowcs_s(&length, buff, sizeof(buff)/sizeof(TCHAR), buff_s, length);
#endif /* __CYGWIN__ */

		SetWindowText(g_hWnd, buff);
	}

#endif /* UNDER_CE*/

/* ------------------------------------------------------------------------- */

unsigned int
atMessageBox(const char* message, const char* caption, UINT type)
{
	wchar_t msgBuf[2048], capBuf[1024];
  	wchar_t *msg, *cap;
	int msgLen, capLen;
	UINT uType;
	UINT retVal;

	assert(message != 0 && caption != 0);

#if defined(_UNICODE)
	msgLen = strlen(message) + 1;
	if (msgLen > sizeof(msgBuf)/sizeof(wchar_t))
		msg = (wchar_t*)malloc(msgLen*sizeof(wchar_t));
	else
	    msg = msgBuf;

	capLen = strlen(caption) + 1;
	if (capLen > sizeof(capBuf)/sizeof(wchar_t))
		cap = (wchar_t*)malloc(capLen*sizeof(wchar_t));
	else
	    cap = capBuf;

	mbstowcs_s(&msgLen, msg, msgLen, message, msgLen-1);
	mbstowcs_s(&capLen, cap, capLen, caption, capLen);
    #define MSGBUF msg
#else
    #define MSGBUF message
#endif

	uType = 0;
	if (type & AT_MB_OK)
		uType = MB_OK;
	else if (type & AT_MB_OKCANCEL)
		uType = MB_OKCANCEL;
	if (type & AT_MB_ICONINFO)
		uType |= MB_ICONINFORMATION;
	else if (type & AT_MB_ICONERROR)
		uType |= MB_ICONSTOP;

	retVal = MessageBox(g_hWnd, MSGBUF, cap, uType);

#if defined(_UNICODE)
	if (msgLen > sizeof(msgBuf)/sizeof(wchar_t))
		(void)free(msg);
	if (capLen > sizeof(capBuf)/sizeof(wchar_t))
		(void)free(cap);
#endif

	return retVal;
}

/* ------------------------------------------------------------------------- */

#if SHOW_GL_VERSION
static void displayVersionInfo(VersionInfo* ptVersionInfo)
{
	char* msgBuf1;
	int allocSize;
	int msgLen;

	allocSize = strlen(ptVersionInfo->szEGLClientAPIs)
		        + strlen(ptVersionInfo->szEGLExtensions)
		        + strlen(ptVersionInfo->szEGLVendor)
		        + strlen(ptVersionInfo->szEGLVersion)
		        + strlen(ptVersionInfo->szGLExtensions)
		        + strlen(ptVersionInfo->szGLVendor)
		        + strlen(ptVersionInfo->szGLVersion)
		        + 100;

	msgBuf1 = malloc(allocSize);

	msgLen = _snprintf_s(msgBuf1, allocSize, allocSize, "EGLVendor: %s\nEGLVersion: %s\nEGLExtensions: %s\n"
		"EGLClientAPIs: %s\nGLVendor: %s\nGLVersion: %s\nGLExtensions: %s\n",
		ptVersionInfo->szEGLVendor,
		ptVersionInfo->szEGLVersion,
		ptVersionInfo->szEGLExtensions,
		ptVersionInfo->szEGLClientAPIs,
		ptVersionInfo->szGLVendor,
		ptVersionInfo->szGLVersion,
		ptVersionInfo->szGLExtensions);

	atMessageBox(msgBuf1, "EGL & GL Version Info", AT_MB_ICONINFO | AT_MB_OK);

	free(msgBuf1);
}
#endif

/* ------------------------------------------------------------------------- */

static void InitializeEGL(NativeWindowType hwnd)
{
	EGLBoolean bResult = EGL_FALSE;
	EGLConfig pkConfig;
	EGLint iNumConfigs = 0;
	EGLint iTotalNumConfigs = 0;
	EGLint iEGLVersionMajor;
	EGLint iEGLVersionMinor;
#if SHOW_GL_VERSION
	VersionInfo vi;
#endif

	atAssert(hwnd != NULL);

	g_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	atAssert(g_eglDisplay != NULL);

	bResult = eglInitialize(g_eglDisplay, &iEGLVersionMajor, &iEGLVersionMinor);
	atAssert(bResult == EGL_TRUE);

	bResult = atGetAppropriateEGLConfig(g_eglDisplay, gc_eiConfigAttribs, &pkConfig);
	atAssert(bResult);

	g_eglSurface = eglCreateWindowSurface(g_eglDisplay, pkConfig, hwnd, NULL);
	atAssert(g_eglSurface != NULL);

#if KTX_OPENGL
	bResult = eglBindAPI(EGL_OPENGL_API);
	atAssert(bResult == EGL_TRUE);
#endif

	g_eglContext = eglCreateContext(g_eglDisplay, pkConfig, EGL_NO_CONTEXT, gc_eiContextAttribs);
	atAssert(g_eglContext != NULL);
	bResult = eglMakeCurrent(g_eglDisplay, g_eglSurface, g_eglSurface, g_eglContext);
	atAssert(bResult == EGL_TRUE);

#if KTX_OPENGL
	// No choice but to use GLEW on Windows; there is no .lib with static bindings.
    bResult = glewInit();
	atAssert(bResult == GLEW_OK);
#endif

#if SHOW_GL_VERSION
	vi.szEGLVendor = eglQueryString(g_eglDisplay, EGL_VENDOR);
	vi.szEGLVersion = eglQueryString(g_eglDisplay, EGL_VERSION);
	vi.szEGLExtensions = eglQueryString(g_eglDisplay, EGL_EXTENSIONS);
	vi.szEGLClientAPIs = eglQueryString(g_eglDisplay, EGL_CLIENT_APIS);

	vi.szGLVendor = glGetString(GL_VENDOR);
	vi.szGLVersion = glGetString(GL_VERSION);
	// GL_EXTENSIONS is not supported in core profile.
	//vi.szGLExtensions = glGetString(GL_EXTENSIONS);
	vi.szGLExtensions = "";

	displayVersionInfo(&vi);
#endif

	eglQuerySurface(g_eglDisplay, g_eglSurface, EGL_WIDTH, &g_iScreenWidth);
	eglQuerySurface(g_eglDisplay, g_eglSurface, EGL_HEIGHT, &g_iScreenHeight);
}

/* ------------------------------------------------------------------------- */

static void TerminateEGL(void)
{
	EGLBoolean bResult = EGL_FALSE;

    bResult = eglMakeCurrent(g_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
			EGL_NO_CONTEXT);
	// atAssert(bResult == EGL_TRUE); // Adreno emulator does not return true.

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

static void handleLButton(void* pSampleData, unsigned int button, BOOL pressed)
{
	if (pressed) {
		gc_aSamples[g_iSampleIndex].sample->pfRelease(pSampleData);

		g_iSampleIndex++;
		if(g_iSampleIndex >= gc_iNumSamples) g_iSampleIndex = 0;

		gc_aSamples[g_iSampleIndex].sample->pfInitialize(&g_pSampleData, gc_aSamples[g_iSampleIndex].args);
		gc_aSamples[g_iSampleIndex].sample->pfResize(g_pSampleData, g_iScreenWidth, g_iScreenHeight);
		SetTitle();
	}
}

static void keyProc(WPARAM wKeyCode, BOOL pressed)
{
	unsigned int iKey;

	switch(wKeyCode) {
	  case VK_LEFT:
        iKey = ATE_LEFT_ARROW;
        break;

      case VK_RIGHT:
        iKey = ATE_RIGHT_ARROW;
        break;

      case VK_UP:
        iKey = ATE_UP_ARROW;
        break;

      case VK_DOWN:
		iKey = ATE_DOWN_ARROW;
		break;

      case VK_RETURN:
	    iKey = ATE_ENTER;

	  default:
	    return;
	}

	if (g_pfEventHandlers[iKey] != 0)
		g_pfEventHandlers[iKey](g_pSampleData, iKey, pressed);
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
			gc_aSamples[g_iSampleIndex].sample->pfResize(g_pSampleData, g_iScreenWidth, g_iScreenHeight);
		}
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

	case WM_KEYDOWN:
		keyProc(wParam, TRUE);
		break;

	case WM_KEYUP:
		keyProc(wParam, FALSE);
		break;

	case WM_LBUTTONDOWN:
        if (g_pfEventHandlers[ATE_LBUTTON] != 0)
			g_pfEventHandlers[ATE_LBUTTON](g_pSampleData, ATE_LBUTTON, TRUE);
		break;

	case WM_LBUTTONUP:
        if (g_pfEventHandlers[ATE_LBUTTON] != 0)
			g_pfEventHandlers[ATE_LBUTTON](g_pSampleData, ATE_LBUTTON, FALSE);
		break;

	case WM_RBUTTONDOWN:
        if (g_pfEventHandlers[ATE_RBUTTON] != 0)
			g_pfEventHandlers[ATE_RBUTTON](g_pSampleData, ATE_RBUTTON, TRUE);
		break;

	case WM_RBUTTONUP:
        if (g_pfEventHandlers[ATE_RBUTTON] != 0)
			g_pfEventHandlers[ATE_RBUTTON](g_pSampleData, ATE_RBUTTON, FALSE);
		break;

	default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

static BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    TCHAR szTitle[] = TEXT("KTX Loadtest");
    TCHAR szWindowClass[]  = TEXT("KTX Loadtest");
	WNDCLASS wc;
	EGLint iScreenWidth;
	EGLint iScreenHeight;
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
	iStyle |=  WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_OVERLAPPEDWINDOW;
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

	if( fFPSTimeCurrent-s_fFPSTimeStart > 1000.0f )
	{
		g_fFPS = (s_iFPSFrames * 1000) / (fFPSTimeCurrent - s_fFPSTimeStart);
		SetTitle();

		s_fFPSTimeStart = fFPSTimeCurrent;
		s_iFPSFrames	= 0;
	}

	return (int)fFPSTimeCurrent;
}

/* ------------------------------------------------------------------------- */

// XXX Move this to at.c?
atPFHandleEvent atSetEventHandler(unsigned int uEvent, atPFHandleEvent pfHandle)
{
	atPFHandleEvent current;
	assert(uEvent > 0 && uEvent < ATE_NUM_SUPPORTED_EVENTS);
	current = g_pfEventHandlers[uEvent];
	g_pfEventHandlers[uEvent] = pfHandle;
	return current;
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

	atSetEventHandler(ATE_LBUTTON, handleLButton);

	/* By default use the first sample. */
	g_iSampleIndex = 0;

	gc_aSamples[g_iSampleIndex].sample->pfInitialize(&g_pSampleData, gc_aSamples[g_iSampleIndex].args);
	gc_aSamples[g_iSampleIndex].sample->pfResize(g_pSampleData, g_iScreenWidth, g_iScreenHeight);
	SetTitle();

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
			gc_aSamples[g_iSampleIndex].sample->pfRun(g_pSampleData, iTime);

			eglSwapBuffers(g_eglDisplay,g_eglSurface);

			InvalidateRect(g_hWnd, NULL, FALSE);
		}

		/* fps update / draw */
		iTime = timer_fps();
	}

	gc_aSamples[g_iSampleIndex].sample->pfRelease(g_pSampleData);

	TerminateEGL();

	DestroyWindow(g_hWnd);

	return 0;
}

/* ------------------------------------------------------------------------- */

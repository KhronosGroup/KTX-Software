/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/**
 * @internal
 * @file egltowgl.c
 * @~English
 *
 * @brief An incomplete emulator of EGL 1.0 on WGL
 *
 * @author Piers Daniell
 * @author Mark Callow
 *
 * $Revision: 21008 $
 * $Date:: 2013-04-02 16:29:06 +0900 #$
 */

/*
Copyright (c) 2013 The Khronos Group Inc.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and/or associated documentation files (the
"Materials"), to deal in the Materials without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Materials, and to
permit persons to whom the Materials are furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
unaltered in all copies or substantial portions of the Materials.
Any additions, deletions, or changes to the original source files
must be clearly indicated in accompanying documentation.

If only executable code is distributed, then the accompanying
documentation must state that "this software is based in part on the
work of the Khronos Group."

THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

#define EGLAPI
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <GL/gl.h>
#include <GL/wglext.h>

#define VENDOR  "Khronos"
#define VERSION "1.4 (incomplete) Layered on WGL"
#define CLIENT_APIS "OpenGL OpenGL_ES"

// This is needed because calls to ReleaseDC in eglDestroyContext
// and to GetClientRect in eglQuerySurface need the HWND. Reasons
// for using GetClientRect are explained there. Everything else
// can be done with just the HDC.
typedef struct _CEGLSurface {
	HDC  deviceContext;
	HWND window;
} CEGLSurface;

// For now just support 2 surfaces & 2 contexts
static const int s_maxSurfaces = 2;
static const int s_maxContexts = 2;
typedef struct _CEGLDisplay {
	EGLBoolean initialized;
	EGLBoolean supportsES;
	EGLint numSurfaces;
	EGLint numContexts;
	CEGLSurface surfaces[2];
	EGLContext contexts[2];
	HWND hwndHidden;
	ATOM atomHiddenClass;
} CEGLDisplay;


static CEGLDisplay s_defaultDisplay = {
	EGL_FALSE,
	EGL_FALSE,
	0,
	0
};


static struct GtfEsEgl {
    HMODULE hOpengl32;

    struct WglFuncs {
        HGLRC (WINAPI *wglCreateContext)(HDC hdc);
        BOOL  (WINAPI *wglDeleteContext)(HGLRC hglrc);
        HGLRC (WINAPI *wglGetCurrentContext)(VOID);
		HDC   (WINAPI *wglGetCurrentDC)(VOID);
        PROC  (WINAPI *wglGetProcAddress)(LPCSTR lpszProc);
        BOOL  (WINAPI *wglMakeCurrent)(HDC hdc, HGLRC hglrc);
		PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribs;
    } wglFuncs;
} s_gtfEsEgl;

static EGLenum s_boundAPI = EGL_OPENGL_ES_API;
static EGLenum s_error = EGL_SUCCESS;


static setError(EGLenum error)
{
	if (s_error == EGL_SUCCESS)
		s_error = error;
}

static void clearError()
{
	s_error = EGL_SUCCESS;
}

static void removeContext(CEGLDisplay* cdisplay, EGLContext ctx)
{
	int i;
	for (i = 0; cdisplay->contexts[i] != ctx; i++) { }
	for (; i < cdisplay->numContexts-1; i++)
	{
		cdisplay->contexts[i] = cdisplay->contexts[i+1];
	}
	cdisplay->numContexts--;
}

static void removeSurface(CEGLDisplay* cdisplay, CEGLSurface* csurface)
{
	int i;
	for (i = 0; &cdisplay->surfaces[i] != csurface; i++) { }
	for (; i < cdisplay->numSurfaces-1; i++)
	{
		cdisplay->surfaces[i].deviceContext = cdisplay->surfaces[i+1].deviceContext;
		cdisplay->surfaces[i].window = cdisplay->surfaces[i+1].window;
	}
	cdisplay->numSurfaces--;
}

static EGLBoolean validContext(CEGLDisplay* cdisplay, EGLContext ctx)
{
	int i;
	for (i = 0; i < cdisplay->numContexts; i++)
	{
		if (ctx == cdisplay->contexts[i])
			return EGL_TRUE;
	}
	return EGL_FALSE;
}

static EGLBoolean validSurface(CEGLDisplay* cdisplay, EGLSurface surface)
{
	int i;
	for (i = 0; i < cdisplay->numSurfaces; i++)
	{
		if (surface == &cdisplay->surfaces[i])
			return EGL_TRUE;
	}
	return EGL_FALSE;
}


EGLAPI EGLDisplay EGLAPIENTRY eglGetDisplay(EGLNativeDisplayType display_id)
{
    return (EGLDisplay) &s_defaultDisplay;
}


//EXTERN_C IMAGE_DOS_HEADER __ImageBase;
//#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

static HWND CreateHiddenWindow()
{
	CEGLDisplay* display = &s_defaultDisplay;
	HINSTANCE hInstance = GetModuleHandle(NULL);
    TCHAR szWindowClass[]  = TEXT("gomi");
	WNDCLASS wc;
	int iStyle;

	wc.style         = 0;
	wc.lpfnWndProc   = DefWindowProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = NULL;
	wc.hCursor       = NULL;
	wc.hbrBackground = NULL;
	wc.lpszMenuName  = 0;
	wc.lpszClassName = szWindowClass;

	display->atomHiddenClass = RegisterClass(&wc);
    if (!display->atomHiddenClass)
    {
    	return NULL;
    }

	iStyle =  WS_DISABLED;

    display->hwndHidden = CreateWindow((LPCTSTR)(display->atomHiddenClass), NULL, iStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, NULL, hInstance, NULL);

	if (!display->hwndHidden)
	{
		UnregisterClass((LPCTSTR)display->atomHiddenClass, hInstance);
	}
    return display->hwndHidden;
}


static void DestroyHiddenWindow()
{
	CEGLDisplay* display = &s_defaultDisplay;

	DestroyWindow(display->hwndHidden);
	UnregisterClass((LPCTSTR)display->atomHiddenClass, GetModuleHandle(NULL));
}


EGLAPI EGLBoolean EGLAPIENTRY eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
{
	PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsString;
    HGLRC legacyContext = 0;
	HDC hdcWaste;
	HWND hwndWaste;
    PIXELFORMATDESCRIPTOR pfd;
    int pixelFormat;
	CEGLDisplay* display = (CEGLDisplay*)dpy;

	if (dpy != (EGLDisplay)&s_defaultDisplay)
	{
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

    s_gtfEsEgl.hOpengl32 = LoadLibrary(L"opengl32.dll");
    s_gtfEsEgl.wglFuncs.wglCreateContext = (HGLRC (WINAPI*)(HDC))GetProcAddress(s_gtfEsEgl.hOpengl32, "wglCreateContext");
    s_gtfEsEgl.wglFuncs.wglDeleteContext = (BOOL (WINAPI*)(HGLRC))GetProcAddress(s_gtfEsEgl.hOpengl32, "wglDeleteContext");
    s_gtfEsEgl.wglFuncs.wglGetCurrentContext = (HGLRC (WINAPI*)(VOID))GetProcAddress(s_gtfEsEgl.hOpengl32, "wglGetCurrentContext");
    s_gtfEsEgl.wglFuncs.wglGetCurrentDC = (HDC (WINAPI*)(VOID))GetProcAddress(s_gtfEsEgl.hOpengl32, "wglGetCurrentDC");
    s_gtfEsEgl.wglFuncs.wglGetProcAddress = (PROC (WINAPI*)(LPCSTR))GetProcAddress(s_gtfEsEgl.hOpengl32, "wglGetProcAddress");
    s_gtfEsEgl.wglFuncs.wglMakeCurrent = (BOOL (WINAPI *)(HDC,HGLRC))GetProcAddress(s_gtfEsEgl.hOpengl32, "wglMakeCurrent");

	// To query WGL extensions it is necessary to have a current context. In order
	// to create that it is necessary to get a DC and set its pixel format to
	// something compatible with GL. Ideally we could do
	//
	//   hdcScreen = GetDC(NULL);
	//
	// Unfortunately security changes in Windows 7 have affected NVIDIA drivers such that
	// hdcScreen cannot be used. It works with AMD drivers and for both on Windows XP
	// though. Instead we have to create a hidden window. Ugh!
	hwndWaste = CreateHiddenWindow();
	hdcWaste = GetDC(hwndWaste);
	memset( &pfd, 0, sizeof(PIXELFORMATDESCRIPTOR) );
    pfd.nSize = sizeof( PIXELFORMATDESCRIPTOR );
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.iLayerType = PFD_MAIN_PLANE;
    pfd.cColorBits = 8+8+8;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 32;
    pfd.cStencilBits = 8;

    pixelFormat = ChoosePixelFormat(hdcWaste, &pfd);
	if (pixelFormat && SetPixelFormat(hdcWaste, pixelFormat, &pfd)) {
		legacyContext = s_gtfEsEgl.wglFuncs.wglCreateContext(hdcWaste);
		s_gtfEsEgl.wglFuncs.wglMakeCurrent(hdcWaste, legacyContext);

		wglGetExtensionsString = (PFNWGLGETEXTENSIONSSTRINGARBPROC) wglGetProcAddress( "wglGetExtensionsStringARB" );

		if (wglGetExtensionsString) {
			const char* extensions = wglGetExtensionsString(hdcWaste);
			if (strstr(extensions, "WGL_ARB_create_context_profile")) {
				s_gtfEsEgl.wglFuncs.wglCreateContextAttribs = (PFNWGLCREATECONTEXTATTRIBSARBPROC) wglGetProcAddress( "wglCreateContextAttribsARB" );
			}
			if (strstr(extensions, "WGL_EXT_create_context_es_profile")
				|| strstr(extensions, "WGL_EXT_create_context_es2_profile")) {
				display->supportsES = EGL_TRUE;
			}
		}
		s_gtfEsEgl.wglFuncs.wglMakeCurrent(hdcWaste, 0);
		s_gtfEsEgl.wglFuncs.wglDeleteContext(legacyContext);

	}
	// Need to keep for eglCreateContext.
	//ReleaseDC(hwndWaste, hdcWaste);
	//DestroyHiddenWindow(hwndWaste);

	display->initialized = EGL_TRUE;

    return EGL_TRUE;
}


EGLAPI EGLBoolean EGLAPIENTRY eglBindAPI(EGLenum api)
{
    switch (api)
	{
      case EGL_OPENGL_API:
        s_boundAPI = EGL_OPENGL_API;
        return EGL_TRUE;

      case EGL_OPENGL_ES_API:
        s_boundAPI = EGL_OPENGL_ES_API;
        return EGL_TRUE;

	  default:
		setError(EGL_BAD_PARAMETER);
    }
    return EGL_FALSE;
}


EGLAPI const char * EGLAPIENTRY eglQueryString(EGLDisplay dpy, EGLint name)
{
	if (dpy != (EGLDisplay)&s_defaultDisplay)
	{
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	if (!((CEGLDisplay*)dpy)->initialized)
	{
		setError(EGL_NOT_INITIALIZED);
		return NULL;
	}

    switch (name)
    {
        case EGL_VERSION:
            return VERSION;
        case EGL_VENDOR:
            return VENDOR;
		case EGL_CLIENT_APIS:
			return CLIENT_APIS;
		case EGL_EXTENSIONS:
			return "";
		default:
			setError(EGL_BAD_PARAMETER);
			return NULL;
    }
}

EGLAPI EGLBoolean EGLAPIENTRY eglGetConfigAttrib (EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value)
{
	if (dpy != (EGLDisplay)&s_defaultDisplay)
	{
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

    if(attribute == EGL_SAMPLE_BUFFERS)
    {
        *value = 0;
    }
    else if(attribute == EGL_RENDERABLE_TYPE)
    {
        *value = EGL_OPENGL_BIT;
		if (((CEGLDisplay*)dpy)->supportsES)
		{
			*value |= EGL_OPENGL_ES2_BIT | EGL_OPENGL_ES3_BIT_KHR;
		}
    }
    else if(attribute == EGL_CONFORMANT)
    {
        *value = EGL_OPENGL_BIT | EGL_OPENGL_ES2_BIT | EGL_OPENGL_ES3_BIT_KHR;
    }
    else if(attribute == EGL_RED_SIZE || attribute == EGL_GREEN_SIZE || attribute == EGL_BLUE_SIZE)
    {
        *value = 8;
    }
    else if(attribute == EGL_ALPHA_SIZE)
    {
        *value = 8;
    }
    else if(attribute == EGL_DEPTH_SIZE)
    {
        *value = 32;
    }
    else if(attribute == EGL_STENCIL_SIZE)
    {
        *value = 8;
    }
    else if(attribute == EGL_SURFACE_TYPE)
    {
        *value = EGL_WINDOW_BIT;
    }
    else if(attribute == EGL_CONFIG_ID)
    {
        *value = 1;
    }

    return EGL_TRUE;
}

EGLAPI EGLSurface EGLAPIENTRY eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config,
                  EGLNativeWindowType win,
                  const EGLint *attrib_list)
{
    HWND hWnd = (HWND)win;
    HDC winDC = GetDC(hWnd);
    PIXELFORMATDESCRIPTOR pfd;
	int i;
    int pixelFormat;
    EGLint renderBuffer = EGL_BACK_BUFFER;
	CEGLDisplay* cdisplay = (CEGLDisplay*)dpy;
	CEGLSurface* csurface;

	if (dpy != (EGLDisplay)&s_defaultDisplay)
	{
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	if (cdisplay->numSurfaces >= s_maxSurfaces)
	{
		setError(EGL_BAD_ALLOC);
		return EGL_NO_SURFACE;
	}

	for (i = 0; attrib_list != 0 && attrib_list[i] != EGL_NONE; i++)
    {
		switch (attrib_list[i++]) {
		  case EGL_RENDER_BUFFER:
			 renderBuffer = attrib_list[i];
			 break;
		  default:
		     setError(EGL_BAD_ATTRIBUTE);
			 return(EGL_NO_SURFACE);
		}
	}

    memset( &pfd, 0, sizeof(PIXELFORMATDESCRIPTOR) );
    pfd.nSize = sizeof( PIXELFORMATDESCRIPTOR );
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
	if (renderBuffer == EGL_BACK_BUFFER)
	{
		pfd.dwFlags |= PFD_DOUBLEBUFFER;
	}
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.iLayerType = PFD_MAIN_PLANE;

    pfd.cColorBits = 8+8+8;

    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 32;
    pfd.cStencilBits = 8;

    pixelFormat = ChoosePixelFormat(winDC, &pfd);
    if( !pixelFormat )
    {
		setError(EGL_BAD_MATCH);
        return EGL_NO_SURFACE;
    }
    if( !SetPixelFormat(winDC, pixelFormat, &pfd) )
    {
		setError(EGL_BAD_MATCH);
        return EGL_NO_SURFACE;
    }
	csurface = &cdisplay->surfaces[cdisplay->numSurfaces++];
	csurface->window = hWnd;
	csurface->deviceContext = winDC;
	return (EGLSurface)csurface;
}


EGLAPI EGLSurface EGLAPIENTRY eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config,
                   const EGLint *attrib_list)
{
	setError(EGL_BAD_MATCH);
    return EGL_NO_SURFACE;
}


EGLAPI EGLSurface EGLAPIENTRY eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config,
                  EGLNativePixmapType pixmap,
                  const EGLint *attrib_list)
{
	if (dpy != (EGLDisplay)&s_defaultDisplay)
	{
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	setError(EGL_BAD_MATCH);
    return EGL_NO_SURFACE;
}


EGLAPI EGLBoolean EGLAPIENTRY eglQuerySurface(EGLDisplay dpy, EGLSurface surface,
											  EGLint attribute, EGLint* value)
{
	CEGLSurface* csurface = (CEGLSurface*)surface;
	RECT rRect;

	if (dpy != (EGLDisplay)&s_defaultDisplay)
	{
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	if (!validSurface((CEGLDisplay*)dpy, csurface))
	{
		setError(EGL_BAD_SURFACE);
		return EGL_FALSE;
	}

	switch (attribute)
	{
	    
	  case EGL_WIDTH:
	    // Must use GetClientRect for width & height, which means we need
		// the window corresponding to the surface. There are system and
		// application clip regions available in the DC but the former is
	    // 0 when the window is not visible and the latter is not set. The
		// latter could be set in eglCreateWindowSurface but since we need
		// the window for eglDestroyContext we might as well store and use
		// it here.
		GetClientRect(csurface->window, &rRect);
		*value =  rRect.right - rRect.left;
		break;
	  case EGL_HEIGHT:
		GetClientRect(csurface->window, &rRect);
		*value =  rRect.bottom - rRect.top;
		break;
	  default:
		setError(EGL_BAD_ATTRIBUTE);
		return EGL_FALSE;
	}
	return EGL_TRUE;
}


static int ConvertEGLAttribListToWGL(const EGLint *egl_attrib_list, int* wgl_attrib_list)
{
    int i = 0, j = 0;
    while (egl_attrib_list[j] != EGL_NONE)
    {
        int attrValue = 0;
        switch (egl_attrib_list[j++])
        {
            case EGL_CONTEXT_MAJOR_VERSION_KHR:
            {
                wgl_attrib_list[i++] = WGL_CONTEXT_MAJOR_VERSION_ARB;
                wgl_attrib_list[i++] = egl_attrib_list[j++];
                break;
            }

            case EGL_CONTEXT_MINOR_VERSION_KHR:
            {
                wgl_attrib_list[i++] = WGL_CONTEXT_MINOR_VERSION_ARB;
                wgl_attrib_list[i++] = egl_attrib_list[j++];
                break;
            }

            case EGL_CONTEXT_FLAGS_KHR:
            {
				if (s_boundAPI == EGL_OPENGL_ES_API)
				{
					setError(EGL_BAD_ATTRIBUTE);
					return 0;
				}
                wgl_attrib_list[i++] = WGL_CONTEXT_FLAGS_ARB;
                if (egl_attrib_list[j] & EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR)
                    attrValue |= WGL_CONTEXT_DEBUG_BIT_ARB;
                if (egl_attrib_list[j] & EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR)
                    attrValue |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
                if (egl_attrib_list[j] & EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR)
                    attrValue |= WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB;
                j++;
                wgl_attrib_list[i++] = attrValue;
                break;
            }

            case EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR:
            {
				if (s_boundAPI == EGL_OPENGL_ES_API)
				{
					setError(EGL_BAD_ATTRIBUTE);
					return 0;
				}
                wgl_attrib_list[i++] = WGL_CONTEXT_PROFILE_MASK_ARB;
                if (egl_attrib_list[j] & EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR)
                    attrValue |= WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
                if (egl_attrib_list[j] & EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR)
                    attrValue |= WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
                j++;
                wgl_attrib_list[i++] = attrValue;
                break;
            }

            case EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_KHR:
            {
				if (s_boundAPI == EGL_OPENGL_ES_API)
				{
					setError(EGL_BAD_ATTRIBUTE);
					return 0;
				}
                wgl_attrib_list[i++] = WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB;
                if (egl_attrib_list[j] & EGL_NO_RESET_NOTIFICATION_KHR)
                    attrValue |= WGL_NO_RESET_NOTIFICATION_ARB;
                if (egl_attrib_list[j] & EGL_LOSE_CONTEXT_ON_RESET_KHR)
                    attrValue |= WGL_LOSE_CONTEXT_ON_RESET_ARB;
                j++;
                wgl_attrib_list[i++] = attrValue;
                break;
            }
            default:
            {
				setError(EGL_BAD_ATTRIBUTE);
                return 0;
            }
        }
    }
    if (s_boundAPI == EGL_OPENGL_ES_API)
	{
        wgl_attrib_list[i++] = WGL_CONTEXT_PROFILE_MASK_ARB;
        wgl_attrib_list[i++] = WGL_CONTEXT_ES2_PROFILE_BIT_EXT;
    }
    wgl_attrib_list[i] = 0;
    return 1;
}


void GetVersionFromWGLAttribList( int* wgl_attrib_list, int *major, int *minor )
{
    int attribId = 0;
    *major = 0;
    *minor = 0;

    for (attribId = 0; wgl_attrib_list[attribId] != 0; attribId += 2 )
    {
        if (wgl_attrib_list[attribId] == WGL_CONTEXT_MAJOR_VERSION_ARB)
        {
            *major = wgl_attrib_list[attribId + 1];
        }
        if (wgl_attrib_list[attribId] == WGL_CONTEXT_MINOR_VERSION_ARB)
        {
            *minor = wgl_attrib_list[attribId + 1];
        }
    }
}


void SetVersionInWGLAttribList( int* wgl_attrib_list, int major, int minor )
{
    int attribId;

    for (attribId = 0; wgl_attrib_list[attribId] != 0; attribId += 2 )
    {
        if (wgl_attrib_list[attribId] == WGL_CONTEXT_MAJOR_VERSION_ARB)
        {
            wgl_attrib_list[attribId+1] = major;
        }
        if (wgl_attrib_list[attribId] == WGL_CONTEXT_MINOR_VERSION_ARB)
        {
            wgl_attrib_list[attribId+1] = minor;
        }
    }
}


EGLAPI EGLContext EGLAPIENTRY eglCreateContext(EGLDisplay dpy, EGLConfig config,
											   EGLContext share_context,
											   const EGLint *attrib_list)
{
    HGLRC ctx = 0;
	HDC hdcWaste;
    CEGLDisplay* cdisplay = (CEGLDisplay*)dpy;
    int WGLContextAttribList[128];
    int versionId = 0;
    int major = 0;
    int minor = 0;
    int requestedVersionIndex = 0;

	if (dpy != (EGLDisplay)&s_defaultDisplay)
	{
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	if (cdisplay->numContexts >= s_maxContexts) {
		setError(EGL_BAD_ALLOC);
		return EGL_NO_CONTEXT;
	}

    if(!ConvertEGLAttribListToWGL(attrib_list, WGLContextAttribList))
    {
		setError(EGL_BAD_MATCH);
        return EGL_NO_CONTEXT;
    }

    if(s_gtfEsEgl.wglFuncs.wglCreateContextAttribs)
    {
		//hdcScreen = GetDC(NULL);
		hdcWaste = GetDC(cdisplay->hwndHidden);
        ctx = s_gtfEsEgl.wglFuncs.wglCreateContextAttribs(hdcWaste, 0, WGLContextAttribList);
		ReleaseDC(cdisplay->hwndHidden, hdcWaste);
		if (ctx) {
		   cdisplay->contexts[cdisplay->numContexts++] = ctx;
           return ctx;
		}
    }

	setError(EGL_BAD_MATCH);
	return EGL_NO_CONTEXT;
}


EGLAPI EGLBoolean EGLAPIENTRY eglMakeCurrent(EGLDisplay dpy, EGLSurface draw,
											 EGLSurface read, EGLContext ctx)
{
	EGLBoolean b;
	CEGLSurface* cdraw = (CEGLSurface*)draw;
	HDC hDC = NULL;

	if (dpy != (EGLDisplay)&s_defaultDisplay)
	{
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	// wglMakeContextCurrentARB could be used to support separate draw and read
	// surfaces but since currently only window surfaces are supported there
	// doesn't seem much need for supporting separate surfaces.
	if (draw != read)
	{
		setError(EGL_BAD_MATCH);
		return EGL_FALSE;
	}
	// wglMakeCurrent(NULL, NULL) makes the current rendering context no longer current,
	// and releases the device context that is used by the rendering context.
	if (draw == EGL_NO_SURFACE)
	{
		if (ctx == EGL_NO_CONTEXT)
		{
			hDC = NULL;
		}
		else
		{
			setError(EGL_BAD_SURFACE);
			return EGL_FALSE;
		}			
	}
	else if (ctx == EGL_NO_CONTEXT)
	{
		setError(EGL_BAD_CONTEXT);
		return EGL_FALSE;
	}
	else
	{
		if (!validSurface((CEGLDisplay*)dpy, draw))
		{
			setError(EGL_BAD_SURFACE);
			return EGL_FALSE;
		}

		if (!validContext((CEGLDisplay*)dpy, ctx))
		{
			setError(EGL_BAD_CONTEXT);
			return EGL_FALSE;
		}
		hDC = cdraw->deviceContext;
	}
	b = s_gtfEsEgl.wglFuncs.wglMakeCurrent(hDC, ctx);
	return b;
}


EGLAPI EGLint EGLAPIENTRY eglGetError(void)
{
	EGLenum errorTmp = s_error;
    clearError();
	return errorTmp;
}


EGLAPI EGLBoolean EGLAPIENTRY eglDestroyContext(EGLDisplay dpy, EGLContext ctx)
{
	if (dpy != (EGLDisplay)&s_defaultDisplay)
	{
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	if (!validContext((CEGLDisplay*)dpy, ctx))
	{
		setError(EGL_BAD_CONTEXT);
		return EGL_FALSE;
	}

	s_gtfEsEgl.wglFuncs.wglDeleteContext((HGLRC)ctx);
	removeContext((CEGLDisplay*)dpy, ctx);

    return EGL_TRUE;
}


EGLAPI EGLBoolean EGLAPIENTRY eglDestroySurface(EGLDisplay dpy, EGLSurface surface)
{
	CEGLDisplay* cdisplay = (CEGLDisplay*)dpy;
	CEGLSurface* csurface = (CEGLSurface*)surface;

	if (dpy != (EGLDisplay)&s_defaultDisplay)
	{
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	if (!validSurface((CEGLDisplay*)dpy, csurface))
	{
		setError(EGL_BAD_CONTEXT);
		return EGL_FALSE;
	}

	ReleaseDC(csurface->window, csurface->deviceContext);
	removeSurface(cdisplay, csurface);

	return EGL_TRUE;
}


EGLAPI EGLBoolean EGLAPIENTRY eglTerminate(EGLDisplay dpy)
{
	int i;
	CEGLDisplay* cdisplay = (CEGLDisplay*)dpy;

	if (dpy != (EGLDisplay)&s_defaultDisplay)
	{
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	for (i = 0; i < cdisplay->numContexts; i++)
	{
		s_gtfEsEgl.wglFuncs.wglDeleteContext(cdisplay->contexts[i]);
	}
	cdisplay->numContexts = 0;
	for (i = 0; i < cdisplay->numSurfaces; i++)
	{
		CEGLSurface* csurface = &cdisplay->surfaces[i];
		ReleaseDC(csurface->window, csurface->deviceContext);
	}
	cdisplay->numSurfaces = 0;

    DestroyHiddenWindow();

    return EGL_TRUE;
}


EGLAPI EGLBoolean EGLAPIENTRY eglGetConfigs(EGLDisplay dpy, EGLConfig *configs,
             EGLint config_size, EGLint *num_config)
{
	if (dpy != (EGLDisplay)&s_defaultDisplay)
	{
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

    if(configs == NULL)
    {
        *num_config = 1;
        return EGL_TRUE;
    }
    else
    {
        configs[0] = (EGLConfig)1;
        return EGL_TRUE;
    }
}


EGLAPI EGLBoolean EGLAPIENTRY eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list,
    EGLConfig *configs, EGLint config_size,
    EGLint *num_config)
{
	if (dpy != (EGLDisplay)&s_defaultDisplay)
	{
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

    if(num_config != NULL)
    {
        *num_config = 1;
    }
    if (configs != NULL && config_size > 0)
    {
        configs[0] = (EGLConfig)1;
    }
    return EGL_TRUE;
}


EGLAPI EGLContext EGLAPIENTRY eglGetCurrentContext(void)
{
    HGLRC ctx = (HGLRC)s_gtfEsEgl.wglFuncs.wglGetCurrentContext();

    return ctx;
}


EGLAPI EGLBoolean EGLAPIENTRY eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
	CEGLSurface* csurface = (CEGLSurface*)surface;
	if (dpy != (EGLDisplay)&s_defaultDisplay)
	{
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	if (!validSurface((CEGLDisplay*)dpy, csurface))
	{
		setError(EGL_BAD_CONTEXT);
		return EGL_FALSE;
	}

    SwapBuffers(csurface->deviceContext);
    return EGL_TRUE;
}


EGLAPI __eglMustCastToProperFunctionPointerType EGLAPIENTRY eglGetProcAddress(const char *procname)
{
    return (__eglMustCastToProperFunctionPointerType)s_gtfEsEgl.wglFuncs.wglGetProcAddress(procname);
}


EGLAPI EGLSurface EGLAPIENTRY eglGetCurrentSurface(EGLint readdraw)
{
    HDC hDC = wglGetCurrentDC();
	int i;

	if (readdraw != EGL_DRAW && readdraw != EGL_READ)
	{
		setError(EGL_BAD_PARAMETER);
		return EGL_NO_SURFACE;
	}

	for (i = 0; hDC != NULL && i < s_defaultDisplay.numSurfaces; i++)
	{
		if (s_defaultDisplay.surfaces[i].deviceContext == hDC)
			return (EGLSurface)&s_defaultDisplay.surfaces[i];
	}
	return EGL_NO_SURFACE;
}


EGLAPI EGLDisplay EGLAPIENTRY eglGetCurrentDisplay(void)
{
    return (EGLDisplay) &s_defaultDisplay;
}


EGLAPI EGLBoolean EGLAPIENTRY eglQueryContext(EGLDisplay dpy, EGLContext ctx,
                                              EGLint attribute, EGLint *value)
{
	if (dpy != (EGLDisplay)&s_defaultDisplay)
	{
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

    return EGL_FALSE;
}

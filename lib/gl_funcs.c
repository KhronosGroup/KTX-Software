/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab textwidth=70: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file gl_funcs.c
 * @~English
 *
 * @brief Retrieve OpenGL function pointers needed by libktx
 */

#define UNIX 0
#define MACOS 0
#define WINDOWS 0
#define IOS 0

#if defined(_WIN32)
#undef WINDOWS
#define WINDOWS 1
#endif
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__bsdi__) || defined(__DragonFly__)
#undef UNIX
#define UNIX 1
#endif
#if defined(linux) || defined(__linux) || defined(__linux__)
#undef UNIX
#define UNIX 1
#endif
#if defined(__APPLE__) && defined(__x86_64__)
#undef MACOS
#define MACOS 1
#endif
#if defined(__APPLE__) && (defined(__arm64__) || defined (__arm__))
#undef IOS
#define IOS 1
#endif
#if defined(__EMSCRIPTEN__)
#define WEB 1
#endif
#if (IOS + MACOS + UNIX + WINDOWS + WEB) > 1
#error "Multiple OS\'s defined"
#endif

#if WINDOWS
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#include <stdlib.h>
#define WINAPI
#endif
#define NO_SHORTCUTS
#include "gl_funcs.h"

#if WINDOWS
#define GetOpenGLModuleHandle(flags) ktxFindOpenGL()
#define LoadProcAddr GetProcAddress
HMODULE ktxOpenGLModuleHandle;
#elif MACOS || UNIX || IOS
// Using NULL returns a handle that can be used to search the process that
// loaded us and any other libraries it has loaded. That's all we need to
// search as the app is responsible for creating the GL context so it must
// be there.
#define GetOpenGLModuleHandle(flags) dlopen(NULL, flags)
#define LoadProcAddr dlsym
#define LIBRARY_NAME NULL
void* ktxOpenGLModuleHandle;
#elif WEB
extern void* emscripten_GetProcAddress(const char *name_);
#define GetOpenGLModuleHandle(flag) (void*)0x0000ffff // Value doesn't matter.
#define LoadProcAddr(lib, proc) emscripten_GetProcAddress(proc)
#define LIBRARY_NAME "unused"
void* ktxOpenGLModuleHandle;
#else
#error "Don\'t know how to load symbols on this OS."
#endif

typedef void (WINAPI *PFNVOIDFUNCTION)(void);
typedef  PFNVOIDFUNCTION *(WINAPI * PFNWGLGETPROCADDRESS) (const char *proc);
static  PFNWGLGETPROCADDRESS wglGetProcAddressPtr;
static const char* noloadmsg = "Could not load OpenGL command: %s!\n";

/* Define pointers for functions libktx is using. */
struct glFuncPtrs gl;

#if defined(__GNUC__)
// This strange casting is because dlsym returns a void* thus is not
// compatible with ISO C which forbids conversion of object pointers
// to function pointers. The cast masks the conversion from the
// compiler thus no warning even though -pedantic is set. Since the
// platform supports dlsym, conversion to function pointers must
// work, despite the mandated ISO C warning.
#define GL_FUNCTION(type, func, required)                                  \
  if ( wglGetProcAddressPtr )                                              \
  *(void **)(&gl.func) = wglGetProcAddressPtr(#func);                      \
  if ( !gl.func )                                                          \
    *(void **)(&gl.func) = LoadProcAddr(ktxOpenGLModuleHandle, #func);     \
  if ( !gl.func && required ) {                                            \
        fprintf(stderr, noloadmsg, #func);                                 \
        return KTX_NOT_FOUND;                                              \
  }
#else
#define GL_FUNCTION(type, func, required)                                  \
  if ( wglGetProcAddressPtr )                                              \
    gl.func = (type)wglGetProcAddressPtr(#func);                           \
  if ( !gl.func)                                                           \
    gl.func = (type)LoadProcAddr(ktxOpenGLModuleHandle, #func);            \
  if ( !gl.func && required) {                                             \
    fprintf(stderr, noloadmsg, #func);                                     \
    return KTX_NOT_FOUND;                                                  \
  }
#endif

#if WINDOWS
static HMODULE
ktxFindOpenGL() {
    HMODULE module = 0;
    BOOL found;
    // Use GetModule not LoadLibrary so we only succeed if the process
    // has already loaded some OpenGL library.
    // Check current module to see if we are statically linked to GL.
    found = GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        (LPCSTR)ktxFindOpenGL,
        &module
    );
    if (found) {
        if (LoadProcAddr(module, "glGetError") != NULL)
            return module;
    }
    // Not statically linked. See what dll the process has loaded.
    // Emulators probably also have opengl32.lib loaded so check that last.
    found = GetModuleHandleExA(
        0,
        "libGLESv2.dll",
        &module
    );
    if (found) return module;
    found = GetModuleHandleExA(
        0,
        "libGLES_CM.dll",
        &module
    );
    if (found) return module;
    found = GetModuleHandleExA(
        0,
        "opengl32.dll",
        &module
    );
    if (found) {
        // Need wglGetProcAddr for non-OpenGL-2 functions.
        wglGetProcAddressPtr =
            (PFNWGLGETPROCADDRESS)LoadProcAddr(module,
                                               "wglGetProcAddress");
        if (wglGetProcAddressPtr != NULL)
            return module;
    }
    return module; // Keep the compiler happy!
}
#endif

ktx_error_code_e
ktxLoadOpenGLLibrary(void)
{
    if (ktxOpenGLModuleHandle)
        return KTX_SUCCESS;

    ktxOpenGLModuleHandle = GetOpenGLModuleHandle(RTLD_LAZY);
    if (ktxOpenGLModuleHandle == NULL) {
        fprintf(stderr, "OpenGL lib not linked or loaded by application.\n");
        // Normal use is for this constructor to be called by an
        // application that has completed OpenGL initialization. In that
        // case the only cause for failure would be a coding error in our
        // library loading. The only other cause would be an application
        // calling GLUpload without having initialized OpenGL.
#if defined(DEBUG)
        abort();
#else
        return KTX_LIBRARY_NOT_LINKED; // So release version doesn't crash.
#endif
    }

#include "gl_funclist.inl"

    return KTX_SUCCESS;
}

#undef GL_FUNCTION


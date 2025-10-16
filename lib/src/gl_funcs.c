/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab textwidth=70: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file
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
#endif
#define NO_SHORTCUTS
#include "gl_funcs.h"

#if WINDOWS
#define GetOpenGLModuleHandle(flags) ktxFindOpenGL()
static HMODULE ktxOpenGLModuleHandle;
static PFNGLGETPROCADDRESS pfnWglGetProcAddress;

PFNVOIDFUNCTION
defaultGLGetProcAddress(const char* proc)
{
    PFNVOIDFUNCTION pfnGLProc = NULL;

    if (pfnWglGetProcAddress)
        pfnGLProc = pfnWglGetProcAddress(proc);
    if (!pfnGLProc) {
        pfnGLProc = (PFNVOIDFUNCTION)GetProcAddress(ktxOpenGLModuleHandle,
                                                    proc);
    }
    return pfnGLProc;
}
#elif MACOS || UNIX || IOS
// Using NULL returns a handle that can be used to search the process that
// loaded us and any other libraries it has loaded. That's all we need to
// search as the app is responsible for creating the GL context so it must
// be there.
#define GetOpenGLModuleHandle(flags) dlopen(NULL, flags)
static void* ktxOpenGLModuleHandle;

PFNVOIDFUNCTION
defaultGLGetProcAddress(const char* proc)
{
    return dlsym(ktxOpenGLModuleHandle, proc);
}
#elif WEB
extern void* emscripten_GetProcAddress(const char *name_);
#define GetOpenGLModuleHandle(flag) (void*)0x0000ffff // Value doesn't matter.
void* ktxOpenGLModuleHandle;

#define defaultGLGetProcAddress ((PFNGLGETPROCADDRESS)emscripten_GetProcAddress)
#else
#error "Don\'t know how to load symbols on this OS."
#endif

static bool openGLLoaded = false;
static const char* noloadmsg = "Could not load OpenGL command: %s!\n";

/* Define pointers for functions libktx is using. */
struct glFuncPtrs gl;

#define GL_FUNCTION(type, func, required)                                  \
  gl.func = (type)pfnGLGetProcAddress(#func);                              \
  if ( !gl.func && required) {                                             \
    fprintf(stderr, noloadmsg, #func);                                     \
    return KTX_NOT_FOUND;                                                  \
  }

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
        if (GetProcAddress(module, "glGetError") != NULL)
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
#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wcast-function-type-mismatch"
#endif
        pfnWglGetProcAddress =
            (PFNGLGETPROCADDRESS)GetProcAddress(module,
                                                "wglGetProcAddress");
#ifdef __clang__
    #pragma clang diagnostic pop
#endif

        if (pfnWglGetProcAddress != NULL)
            return module;
    }
    return 0;
}
#endif

ktx_error_code_e
ktxLoadOpenGLLibrary(void)
{
    if (openGLLoaded)
        return KTX_SUCCESS;

    // Look for OpenGL module and set up default GetProcAddress.
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
    return KTX_SUCCESS;
}

/**
 * @ingroup ktx\_glloader
 * @~English
 * @brief Load pointers for the GL functions used by the ktxTexture\*\_GLUpload functions.
 *
 * Should be called by an application before its first call to a
 * ktxTexture\*\_GLUpload function, passing a pointer to the GLGetProcAddress function
 * provided by whatever OpenGL framework it is using. For backward
 * compatibility, the ktxTexture\*\_GLUpload functions call this with a NULL pointer causing an
 * attempt to load the pointers from the program module using
 * @c dlsym (GNU/Linux, macOS), @c wglGetProcAddr and @c GetProcAddr (Windows)
 * or @c emscripten_GetProcAddress (Web). This works with the vast majority of
 * OpenGL implementations but issues have been seen on Fedora systems
 * particularly with NVIDIA hardware. For full robustness, applications should
 * call this function.
 *
 * @param [in] pfnGLGetProcAddress pointer to function for retrieving pointers
 *                                 to GL functions. If NULL, retrieval is
 *                                 attempted using system dependent generic
 *                                 functions.
 */
ktx_error_code_e
ktxLoadOpenGL(PFNGLGETPROCADDRESS pfnGLGetProcAddress)
{
    if (openGLLoaded)
        return KTX_SUCCESS;

    if (!pfnGLGetProcAddress) {
        ktx_error_code_e result = ktxLoadOpenGLLibrary();
        if (result != KTX_SUCCESS) {
            return result;
        }
        pfnGLGetProcAddress = defaultGLGetProcAddress;
    }

    // Load function pointers

#include "gl_funclist.inl"

    openGLLoaded = true;
    return KTX_SUCCESS;
}

#undef GL_FUNCTION


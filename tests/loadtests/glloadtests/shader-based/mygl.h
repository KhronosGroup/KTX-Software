/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2016-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file    mygl.h
 * @brief   Include appropriate version of gl.h for the shader-based tests.
 *
 * This is a separate header to avoid repetition of these conditionals.
 */

#ifndef MYGL_H
#define MYGL_H

/* These are enum values in SDL2/SDL_video.h. Need defines for the
 * following pre-processor conditionals to work.
 */
#define SDL_GL_CONTEXT_PROFILE_CORE          0x0001
#define SDL_GL_CONTEXT_PROFILE_COMPATIBILITY 0x0002
#define SDL_GL_CONTEXT_PROFILE_ES            0x0004

#if GL_CONTEXT_PROFILE == SDL_GL_CONTEXT_PROFILE_CORE
  #ifdef _WIN32
    #include <windows.h>
    #undef KTX_USE_GETPROC  /* Must use GETPROC on Windows */
    #define KTX_USE_GETPROC 1
  #else
    #if !defined(KTX_USE_GETPROC)
      #define KTX_USE_GETPROC 0
    #endif
  #endif
  #if KTX_USE_GETPROC
    #include <GL/glew.h>
  #else
    #define GL_GLEXT_PROTOTYPES
    #include <GL/glcorearb.h>
  #endif

//#define GL_APIENTRY APIENTRY

#elif GL_CONTEXT_PROFILE == SDL_GL_CONTEXT_PROFILE_COMPATIBILITY

  #error This application is not intended to run in compatibility mode.

#elif GL_CONTEXT_PROFILE == SDL_GL_CONTEXT_PROFILE_ES

  #if GL_CONTEXT_MAJOR_VERSION == 1

    #error This application cannot run on OpenGL ES 1.

  #elif GL_CONTEXT_MAJOR_VERSION == 2

    #define GL_GLEXT_PROTOTYPES
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>

  #elif GL_CONTEXT_MAJOR_VERSION == 3

    #define GL_GLEXT_PROTOTYPES
    #include <GLES3/gl3.h>
    #include <GLES2/gl2ext.h>

  #else

    #error Unexpected GL_CONTEXT_MAJOR_VERSION

  #endif

#endif

/* To help find supported transcode targets */
#if !defined(GL_ETC1_RGB8_OES)
  #define GL_ETC1_RGB8_OES                  0x8D64
#endif
#if !defined(GL_COMPRESSED_RGB8_ETC2)
  #define GL_COMPRESSED_RGB8_ETC2           0x9274
#endif
#if !defined(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
  #define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3
#endif

/* undef to avoid needing ordering of mygl.h & SDL2/sdl.h inclusion. */
#undef SDL_GL_CONTEXT_PROFILE_CORE
#undef SDL_GL_CONTEXT_PROFILE_COMPATIBILITY
#undef SDL_GL_CONTEXT_PROFILE_ES

#endif /* MYGL_H */

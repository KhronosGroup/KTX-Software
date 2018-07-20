/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2016 Mark Callow.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file    mygl.h
 * @brief   Include appropriate version of gl.h for the shader-based tests.
 *
 * This is a separate header to avoid repetition of these conditionals.
 *
 * @author Mark Callow
 * @copyright (c) 2016, Mark Callow.
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

/* undef to avoid needing ordering of mygl.h & SDL2/sdl.h inclusion. */
#undef SDL_GL_CONTEXT_PROFILE_CORE
#undef SDL_GL_CONTEXT_PROFILE_COMPATIBILITY
#undef SDL_GL_CONTEXT_PROFILE_ES

#endif /* MYGL_H */

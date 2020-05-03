/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab textwidth=70: */

/*
 * ©2017 Mark Callow.
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
 * @internal
 * @file vk_funcs.h
 * @~English
 *
 * @brief Declare pointers for OpenGL {,ES} functions.
 *
 * Dynamically retrieving pointers avoids apps or shared library builds having to link with OpenGL {,ES}
 * and avoids the need for compiling different versions of the libary for different OpenGL {,ES} versions.
 */

#ifndef _GL_FUNCS_H_
#define _GL_FUNCS_H_

#undef GL_GLEXT_PROTOTYPES // Just to be sure.
#include "GL/glcorearb.h"
#include "ktx.h"

#if WINDOWS
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
extern HMODULE ktxOpenGLModuleHandle;
#else
extern void* ktxOpenGLModuleHandle;
#endif

extern ktx_error_code_e ktxLoadOpenGLLibrary(void);

#define GL_FUNCTION(type, fun, required) type fun;

extern struct glFuncPtrs {
#include "gl_funclist.inl"
} gl;

#undef GL_FUNCTION

#if !defined(NO_SHORTCUTS)
// Macros to allow standard, i.e always present functions, to be called
// by their prototype names.
#define glBindTexture gl.glBindTexture
#define glCompressedTexImage2D gl.glCompressedTexImage2D
#define glCompressedTexSubImage2D gl.glCompressedTexSubImage2D
#define glDeleteTextures gl.glDeleteTextures
#define glGenTextures gl.glGenTextures
#define glGetError gl.glGetError
#define glGetIntegerv gl.glGetIntegerv
#define glGetString(x) (const char*)gl.glGetString(x)
#define glPixelStorei gl.glPixelStorei
#define glTexImage2D gl.glTexImage2D
#define glTexSubImage2D gl.glTexSubImage2D
#define glTexParameteri gl.glTexParameteri
#define glTexParameteriv gl.glTexParameteriv
#endif

#endif /* _GL_FUNCS_H_ */


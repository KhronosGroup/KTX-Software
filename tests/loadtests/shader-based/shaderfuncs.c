/* -*- tab-iWidth: 4; -*- */
/* vi: set sw=2 ts=4: */

/**
 * @file	sample_01_draw_texture.c
 * @brief	Tests DrawTexture functionality to see if the implementation
 *          applies the viewport transform to the supplied coordinates.
 *
 * @author	Mark Callow
 *
 * $Revision: 21082 $
 * $Date:: 2013-04-09 14:52:45 +0900 #$
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

#include "../common/at.h"
#include "ktx.h"

/* ----------------------------------------------------------------------------- */

extern const GLchar* pszESLangVer;
extern const GLchar* pszGLLangVer;

/* ----------------------------------------------------------------------------- */

GLboolean makeShader(GLenum type, const GLchar* const source, GLuint* shader)
{
	GLint sh = glCreateShader(type);
	GLint shaderCompiled;
	const GLchar* ss[2];

#if KTX_OPENGL
	// XXX Probably should figure out a run-time check for this.
	ss[0] = pszGLLangVer;
#else
	ss[0] = pszESLangVer;
#endif
	ss[1] = source;
	glShaderSource(sh, 2, ss, NULL);
	glCompileShader(sh);

	// Check if compilation succeeded
    glGetShaderiv(sh, GL_COMPILE_STATUS, &shaderCompiled);

	if (!shaderCompiled) {
		// An error happened, first retrieve the length of the log message
		int logLength, charsWritten;
		char* infoLog;
		glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &logLength);

		// Allocate enough space for the message and retrieve it
		infoLog = malloc(logLength);
        glGetShaderInfoLog(sh, logLength, &charsWritten, infoLog);

		// Displays the error in a dialog box
		atMessageBox(logLength ? infoLog : "", "Shader compilation error", AT_MB_OK|AT_MB_ICONERROR);
	    free(infoLog);
		glDeleteShader(sh);
		return GL_FALSE;
	} else {
		*shader = sh;
		return GL_TRUE;
	}
}

GLboolean makeProgram(GLuint vs, GLuint fs, GLuint* program)
{

	GLint error = glGetError();
	GLint linked;
	GLint fsCompiled, vsCompiled;

	glGetShaderiv(vs, GL_COMPILE_STATUS, &fsCompiled);
	glGetShaderiv(fs, GL_COMPILE_STATUS, &vsCompiled);
	if (fsCompiled && vsCompiled) {
		GLuint prog = glCreateProgram();

		glAttachShader(prog, vs);
		glAttachShader(prog, fs);

		glLinkProgram(prog);
		// Check if linking succeeded in the same way we checked for compilation success
		glGetProgramiv(prog, GL_LINK_STATUS, &linked);
		if (!linked) {
			int logLength, charsWritten;
			char* infoLog;
			glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
			infoLog = malloc(logLength);
			glGetProgramInfoLog(prog, logLength, &charsWritten, infoLog);

			atMessageBox(logLength ? infoLog : "", "Program link error", MB_OK|MB_ICONERROR);

			free(infoLog);
		}

		glDeleteShader(vs);
		glDeleteShader(fs);
		*program = prog;
		return GL_TRUE;
	} else {
		return GL_FALSE;
	}
}

/* ----------------------------------------------------------------------------- */

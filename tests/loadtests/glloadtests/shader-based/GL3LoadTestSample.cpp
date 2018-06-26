/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2017 - 2018 Mark Callow.
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
 * @class GLLoadTestSample
 * @~English
 *
 * @brief Definition of a base class for OpenGL texture loading test samples.
 *
 * @author Mark Callow, www.edgewise-consulting.com.
 */

#include <string.h>
#include <sstream>
#include <ktx.h>

#include "GL3LoadTestSample.h"

/* ------------------------------------------------------------------------- */

static const GLchar* pszESLangVer = "#version 300 es\n";
// location layout qualifier did not appear until version 330.
static const GLchar* pszGLLangVer = "#version 330 core\n";

/* ------------------------------------------------------------------------- */

void
GL3LoadTestSample::makeShader(GLenum type, const GLchar* const source,
                              GLuint* shader)
{
    GLint sh = glCreateShader(type);
    GLint shaderCompiled;
    const GLchar* ss[2];

    if (strstr((const char*)glGetString(GL_VERSION), "GL ES") == NULL)
        ss[0] = pszGLLangVer;
    else
        ss[0] = pszESLangVer;
    ss[1] = source;
    glShaderSource(sh, 2, ss, NULL);
    glCompileShader(sh);

    // Check if compilation succeeded
    glGetShaderiv(sh, GL_COMPILE_STATUS, &shaderCompiled);

    if (!shaderCompiled) {
        // An error happened, first retrieve the length of the log message
        int logLength, charsWritten;
        char* infoLog;
        std::stringstream message;

        glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &logLength);

        // Allocate enough space for the message and retrieve it
        infoLog = new char[logLength];
        glGetShaderInfoLog(sh, logLength, &charsWritten, infoLog);

        message << "makeShader compilation error" << std::endl << infoLog;
        delete infoLog;
        glDeleteShader(sh);
        throw std::runtime_error(message.str());
    } else {
        *shader = sh;
    }
}

void
GL3LoadTestSample::makeProgram(GLuint vs, GLuint fs, GLuint* program)
{
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
            std::stringstream message;

            glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
            infoLog = new char[logLength];
            glGetProgramInfoLog(prog, logLength, &charsWritten, infoLog);
            
            message << "makeProgram link error" << std::endl << infoLog;
            delete infoLog;
            glDeleteProgram(prog);
            throw std::runtime_error(message.str());
        }
        *program = prog;
    } else {
        std::stringstream message;
        message << "makeProgram: either vertex or fragment shader is not compiled.";
        throw std::runtime_error(message.str());
    }
}

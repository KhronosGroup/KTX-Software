/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @class GLLoadTestSample
 * @~English
 *
 * @brief Definition of a base class for OpenGL texture loading test samples.
 *
 * @author Mark Callow, github.com/MarkCallow.
 */

#include <string.h>
#include <sstream>
#include <ktx.h>
#include <SDL3/SDL_platform.h>

#include "GL3LoadTestSample.h"

/* ------------------------------------------------------------------------- */

static const GLchar* pszESLangVer = "#version 300 es\n";
// location layout qualifier did not appear until version 330.
static const GLchar* pszGLLangVer = "#version 330 core\n";

/* ------------------------------------------------------------------------- */

void
GL3LoadTestSample::makeShader(GLenum type,
                              ShaderSource& source,
                              GLuint* shader)
{
    GLint sh = glCreateShader(type);
    GLint shaderCompiled;

    if (strstr((const char*)glGetString(GL_VERSION), "GL ES") == NULL)
        source.insert(source.cbegin(), pszGLLangVer);
    else
        source.insert(source.cbegin(), pszESLangVer);
    glShaderSource(sh, (GLsizei)source.size(), source.data(), NULL);
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
        delete[] infoLog;
        glDeleteShader(sh);
        throw std::runtime_error(message.str());
    } else {
        *shader = sh;
    }
}

void
GL3LoadTestSample::makeShader(GLenum type, const GLchar* const source,
                              GLuint* shader)
{
    ShaderSource ss;
    ss.push_back(source);
    makeShader(type, ss, shader);
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
            delete[] infoLog;
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

bool
GL3LoadTestSample::contextSupportsSwizzle()
{
    bool esProfile = false;
    GLint majorVersion, minorVersion;
    if (strstr((const char*)glGetString(GL_VERSION), "GL ES") != NULL) {
        esProfile = true;
    }
    // MAJOR & MINOR only introduced in GL {,ES} 3.0
    glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
    if (glGetError() != GL_NO_ERROR) {
        // This is not a GL {,ES} 3.0 context...
        assert(false);
        return false;
    }
    if (esProfile)
        return true; // ES 3.0+ has swizzle
    else if (majorVersion == 3 && minorVersion < 3)
        return false; // Swizzle was introduced in OpenGL 3.3.
    else
        return true;
}
#if !defined(SDL_PLATFORM_IOS)
  // SDL only defines this on IOS and on Windows undefined != 0
  #define SDL_PLATFORM_IOS 0
#endif

#if !defined(GL_BACK_LEFT)
#define GL_BACK_LEFT 0x0402
#endif

GLint
GL3LoadTestSample::framebufferColorEncoding()
{
    GLint encoding = GL_SRGB;
    GLenum attachment;
    if (strstr((const char*)glGetString(GL_VERSION), "GL ES") == NULL)
        attachment = GL_BACK_LEFT;
    else if (SDL_PLATFORM_IOS)
        // iOS does not use the default framebuffer.
        attachment = GL_COLOR_ATTACHMENT0;
    else
        attachment = GL_BACK;

    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment,
                                      GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING,
                                      &encoding);
    return encoding;
}

#define SUPPORT_HDR 0
#if SUPPORT_HDR
GLint
GL3LoadTestSample::framebufferAttachmentType()
{
    GLint type;
    GLenum attachment;
    if (strstr((const char*)glGetString(GL_VERSION), "GL ES") == NULL)
        attachment = GL_BACK_LEFT;
    else if (SDL_PLATFORM_IOS)
        // iOS does not use the default framebuffer.
        attachment = GL_COLOR_ATTACHMENT0;
    else
        attachment = GL_BACK;

    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment,
                                      GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE,
                                      &type);
    return type;
}
#endif

/*
 * @internal
 * @~English
 * @brief Check color encoding.
 *
 * This provides a central place for tests to determine whether test samples need to do
 * their own sRGB encoding to get correct color.
 *
 * GLAppSDL creates an GL_SRGB framebuffer for LDR rendering, if supported by the
 * implementation. In this case framebufferColorEncoding() will return @c GL_SRGB. On
 * implementations that do not support @c GL_SRGB framebuffers, e.g. WebGL, we have found
 * that @c GL_LINEAR framebuffers are simply copied to the display which decodes the content
 * as if sRGB. In such cases test samples must do their own sRGB encoding to work around
 * these fake GL_LINEAR buffers.
 *
 * GLAppSDL creates a GL_LINEAR half-float or float framebuffer for HDR rendering.
 */
bool
GL3LoadTestSample::colorEncodingSRGBOrTrueLinear()
{
    if (framebufferColorEncoding() == GL_SRGB)
        return true;

    // Encoding is GL_LINEAR.
#if SUPPORT_HDR
    // This test is intended to distinguish a half-float or float and therefore genuinely
    // GL_LINEAR framebuffer from the fake ones. Unfortunately we have observed on Apple
    // OSes that when HDR display is not enabled for a window, the compositor clamps and
    // copies the data to the display where it is decoded as sRGB. There is no portable
    // query in SDL that would let us determine if the window is HDR enabled which would
    // be needed to fix this test.
    //
    // Due to inability to enable HDR display for SDL OpenGL windows on Apple OSes and
    // to the author's lack of suitable hardware for other platforms, it is only
    // an educated guess that a half-float or float GL_LINEAR framebuffer displayed
    // as HDR will be genuinely linear.
    //
    GLint colorType = framebufferAttachmentType();
    if (colorType == GL_SIGNED_NORMALIZED || colorType == GL_UNSIGNED_NORMALIZED)
        return false;
    else
        return true;
#else
    return false;
#endif
}

// Emscripten assimp port not yet available.
#if !defined(__EMSCRIPTEN__)
void
GL3LoadTestSample::loadMesh(std::string filename,
                            glMeshLoader::MeshBuffer& meshBuffer,
                            std::vector<glMeshLoader::VertexLayout> vertexLayout,
                            float scale)
{
    GLMeshLoader *mesh = new GLMeshLoader();

    mesh->LoadMesh(filename);

    assert(mesh->m_Entries.size() > 0);

    mesh->CreateBuffers(
          meshBuffer,
          vertexLayout,
          scale);

    meshBuffer.dim = mesh->dim.size;

    delete(mesh);
}
#else
void
GL3LoadTestSample::loadMesh(std::string,
                            glMeshLoader::MeshBuffer&,
                            std::vector<glMeshLoader::VertexLayout>,
                            float)
{
}
#endif

bool
GL3LoadTestSample::transcodeIfNeeded(ktxTexture* kTexture)
{
    if (ktxTexture_IsTranscodable(kTexture)) {
        // ktxTexture2_TranscodeBasis has an early out for the UASTC HDR 4x4 to ASTC HDR 4x4
        // so just call the transcoder.
        transcoder.transcode((ktxTexture2*)kTexture);
        return true;
    }
    return false;
}




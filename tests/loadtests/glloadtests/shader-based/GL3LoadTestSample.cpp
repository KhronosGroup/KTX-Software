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
 * @author Mark Callow, www.edgewise-consulting.com.
 */

#include <string.h>
#include <sstream>
#include <ktx.h>
#define __IPHONEOS__ 0
#include <SDL2/SDL_platform.h>

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

#if !defined(GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT)
#define GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT 0x8A54
#define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG  0x8C01
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG 0x9137
#endif
#if !defined(GL_COMPRESSED_RG_RGTC2)
#define GL_COMPRESSED_RG_RGTC2              0x8DBD
#endif
#if !defined(GL_COMPRESSED_RGBA_BPTC_UNORM)
#define GL_COMPRESSED_RGBA_BPTC_UNORM       0x8E8C
#endif
#if !defined(GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT)
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT 0x8E8E
#endif

void
GL3LoadTestSample::determineCompressedTexFeatures(compressedTexFeatures& features)
{
    ktx_int32_t numCompressedFormats;

    memset(&features, false, sizeof(features));

    glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &numCompressedFormats);
    GLint* formats = new GLint[numCompressedFormats];
    glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, formats);

    for (ktx_int32_t i = 0; i < numCompressedFormats; i++) {
        if (formats[i] == GL_COMPRESSED_RGBA8_ETC2_EAC)
            features.etc2 = true;
        if (formats[i] == GL_ETC1_RGB8_OES)
            features.etc1 = true;
        if (formats[i] == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
            features.bc3 = true;
        if (formats[i] == GL_COMPRESSED_RG_RGTC2)
            features.rgtc = true;
        if (formats[i] == GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT)
            features.pvrtc_srgb = true;
        if (formats[i] == GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG)
            features.pvrtc1 = true;
        if (formats[i] == GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG)
            features.pvrtc2 = true;
        if (formats[i] == GL_COMPRESSED_RGBA_ASTC_4x4_KHR)
            features.astc_ldr = true;
        if (formats[i] == GL_COMPRESSED_RGBA_BPTC_UNORM)
            features.bc7 = true;
        if (formats[i] == GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT)
            features.bc6h = true;
    }
    delete[] formats;

    // Just in case COMPRESSED_TEXTURE_FORMATS didn't return anything.
    // There is no ETC2 extension. It went into core in OpenGL ES 2.0.
    // ARB_es_compatibility is not a good indicator. ETC2 could be supported
    // by software decompression. Better to report unsupported.
    if (!features.etc1 && SDL_GL_ExtensionSupported("GL_OES_compressed_ETC1_RGB8_texture"))
        features.etc1 = true;;
    if (!features.bc3 && SDL_GL_ExtensionSupported("GL_EXT_texture_compression_s3tc"))
        features.bc3 = true;
    if (!features.rgtc && SDL_GL_ExtensionSupported("GL_ARB_texture_compression_rgtc"))
        features.rgtc = true;
    if (!features.pvrtc1 && SDL_GL_ExtensionSupported("GL_IMG_texture_compression_pvrtc"))
        features.pvrtc1 = true;
    if (!features.pvrtc2 && SDL_GL_ExtensionSupported("GL_IMG_texture_compression_pvrtc2"))
        features.pvrtc2 = true;
    if (!features.pvrtc_srgb && SDL_GL_ExtensionSupported("GL_EXT_pvrtc_sRGB"))
        features.pvrtc_srgb = true;
    if (!(features.bc7 && features.bc6h) && SDL_GL_ExtensionSupported("GL_ARB_texture_compression_bptc"))
        features.bc6h = features.bc7 = true;
    if (!features.astc_ldr && SDL_GL_ExtensionSupported("GL_KHR_texture_compression_astc_ldr"))
        features.astc_ldr = true;
    // The only way to identify this support is the extension string.
    // The format name is the same.
    if (SDL_GL_ExtensionSupported("GL_KHR_texture_compression_astc_hdr"))
        features.astc_hdr = true;
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

GLint
GL3LoadTestSample::framebufferColorEncoding()
{
    GLint encoding = GL_SRGB;
    GLenum attachment;
#if !defined(GL_BACK_LEFT)
#define GL_BACK_LEFT 0x0402
#endif
    if (strstr((const char*)glGetString(GL_VERSION), "GL ES") == NULL)
        attachment = GL_BACK_LEFT;
    else if (__IPHONEOS__)
        // iOS does not use the default framebuffer.
        attachment = GL_COLOR_ATTACHMENT0;
    else
        attachment = GL_BACK;

    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment,
                                      GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING,
                                      &encoding);
    return encoding;
}

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


/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab textwidth=70: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file gl_funclist.h
 * @~English
 *
 * @brief List of OpenGL {,ES} functions used by libktx.
 */

// There is no way for the pre-processor to uppercase stringized macro args
// so we have to explicitly give the types.

#define required 1     // Present in all GL versions. Load failure is an error.
#define not_required 0 // May not be present. Code must check before calling.

GL_FUNCTION(PFNGLBINDTEXTUREPROC, glBindTexture, required)
GL_FUNCTION(PFNGLCOMPRESSEDTEXIMAGE1DPROC, glCompressedTexImage1D, not_required)
GL_FUNCTION(PFNGLCOMPRESSEDTEXIMAGE2DPROC, glCompressedTexImage2D, required)
GL_FUNCTION(PFNGLCOMPRESSEDTEXIMAGE3DPROC, glCompressedTexImage3D, not_required)
GL_FUNCTION(PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC, glCompressedTexSubImage1D, not_required)
GL_FUNCTION(PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC, glCompressedTexSubImage2D, required)
GL_FUNCTION(PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC, glCompressedTexSubImage3D, not_required)
GL_FUNCTION(PFNGLDELETETEXTURESPROC, glDeleteTextures, required)
GL_FUNCTION(PFNGLGENERATEMIPMAPPROC, glGenerateMipmap, not_required)
GL_FUNCTION(PFNGLGENTEXTURESPROC, glGenTextures, required)
GL_FUNCTION(PFNGLGETERRORPROC, glGetError, required)
GL_FUNCTION(PFNGLGETINTEGERVPROC, glGetIntegerv, required)
GL_FUNCTION(PFNGLGETSTRINGPROC, glGetString, required)
GL_FUNCTION(PFNGLGETSTRINGIPROC, glGetStringi, not_required)
GL_FUNCTION(PFNGLPIXELSTOREIPROC, glPixelStorei, required)
GL_FUNCTION(PFNGLTEXIMAGE1DPROC, glTexImage1D, not_required)
GL_FUNCTION(PFNGLTEXIMAGE2DPROC, glTexImage2D, required)
GL_FUNCTION(PFNGLTEXIMAGE3DPROC, glTexImage3D, not_required)
GL_FUNCTION(PFNGLTEXPARAMETERIPROC, glTexParameteri, required)
GL_FUNCTION(PFNGLTEXPARAMETERIVPROC, glTexParameteriv, required)
GL_FUNCTION(PFNGLTEXSTORAGE1DPROC, glTexStorage1D, not_required)
GL_FUNCTION(PFNGLTEXSTORAGE2DPROC, glTexStorage2D, not_required)
GL_FUNCTION(PFNGLTEXSTORAGE3DPROC, glTexStorage3D, not_required)
GL_FUNCTION(PFNGLTEXSUBIMAGE1DPROC, glTexSubImage1D, not_required)
GL_FUNCTION(PFNGLTEXSUBIMAGE2DPROC, glTexSubImage2D, required)
GL_FUNCTION(PFNGLTEXSUBIMAGE3DPROC, glTexSubImage3D, not_required)

#undef required
#undef not_required

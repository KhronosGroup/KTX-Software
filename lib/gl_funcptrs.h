/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/* $Id$ */

/*
Copyright (c) 2010 The Khronos Group Inc.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and/or associated documentation files (the
"Materials"), to deal in the Materials without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Materials, and to
permit persons to whom the Materials are furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
unaltered in all copies or substantial portions of the Materials.
Any additions, deletions, or changes to the original source files
must be clearly indicated in accompanying documentation.

If only executable code is distributed, then the accompanying
documentation must state that "this software is based in part on the
work of the Khronos Group."

THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

/*
 * Author: Mark Callow based on code from Georg Kolling
 */

#ifndef _GL_FUNCPTRS_H_
#define _GL_FUNCPTRS_H_

#ifdef __cplusplus
extern "C" {
#endif

extern PFNGLTEXIMAGE1DPROC pfGlTexImage1D;
extern PFNGLTEXIMAGE3DPROC pfGlTexImage3D;
extern PFNGLCOMPRESSEDTEXIMAGE1DPROC pfGlCompressedTexImage1D;
extern PFNGLCOMPRESSEDTEXIMAGE3DPROC pfGlCompressedTexImage3D;
extern PFNGLGENERATEMIPMAPPROC pfGlGenerateMipmap;

#define DECLARE_GL_FUNCPTRS \
    PFNGLTEXIMAGE1DPROC pfGlTexImage1D; \
    PFNGLTEXIMAGE3DPROC pfGlTexImage3D; \
    PFNGLCOMPRESSEDTEXIMAGE1DPROC pfGlCompressedTexImage1D; \
    PFNGLCOMPRESSEDTEXIMAGE3DPROC pfGlCompressedTexImage3D; \
    PFNGLGENERATEMIPMAPPROC pfGlGenerateMipmap;

#define INITIALIZE_GL_FUNCPTRS \
    pfGlTexImage1D = glTexImage1D; \
    pfGlTexImage3D = glTexImage3D; \
    pfGlCompressedTexImage1D = glCompressedTexImage1D; \
    pfGlCompressedTexImage3D = glCompressedTexImage3D; \
    pfGlGenerateMipmap = glGenerateMipmap;
    
#ifdef __cplusplus
}
#endif

#endif /* GL_FUNCPTRS */

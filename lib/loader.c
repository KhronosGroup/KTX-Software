/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/* $Id$ */

/**
 * @file
 * @~English
 *
 * @brief Functions for instantiating GL or GLES textures from KTX files.
 *
 * @author Georg Kolling, Imagination Technology
 * @author Mark Callow, HI Corporation
 */

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
work of the Khronos Group".

THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#if KTX_OPENGL

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

  #define GL_APIENTRY APIENTRY
  #include "gl_funcptrs.h"

#elif KTX_OPENGL_ES1

  #include <GLES/gl.h>
  #include <GLES/glext.h>
  #include "gles1_funcptrs.h"

#elif KTX_OPENGL_ES2

  #define GL_GLEXT_PROTOTYPES
  #include <GLES2/gl2.h>
  #include <GLES2/gl2ext.h>
  #include "gles2_funcptrs.h"

#elif KTX_OPENGL_ES3

  #define GL_GLEXT_PROTOTYPES
  #include <GLES3/gl3.h>
  #include <GLES2/gl2ext.h>
  #include "gles3_funcptrs.h"

#else
  #error Please #define one of KTX_OPENGL, KTX_OPENGL_ES1, KTX_OPENGL_ES2 or KTX_OPENGL_ES3 as 1
#endif

#include "ktx.h"
#include "ktxint.h"
#include "ktxcontext.h"

DECLARE_GL_FUNCPTRS

/**
 * @internal
 * @~English
 * @brief Additional contextProfile bit indicating an OpenGL ES context.
 *
 * This is the same value NVIDIA returns when using an OpenGL ES profile
 * of their desktop drivers. However it is not specified in any official
 * specification as OpenGL ES does not support the GL_CONTEXT_PROFILE_MASK
 * query.
 */
#define _CONTEXT_ES_PROFILE_BIT 0x4

/**
 * @internal
 * @~English
 * @name Supported Sized Format Macros
 *
 * These macros describe values that may be used with the sizedFormats
 * variable.
 */
/**@{*/
#define _NON_LEGACY_FORMATS 0x1 /*< @internal Non-legacy sized formats are supported. */
#define _LEGACY_FORMATS 0x2  /*< @internal Legacy sized formats are supported. */
/**
 * @internal
 * @~English
 * @brief All sized formats are supported
 */
#define _ALL_SIZED_FORMATS (_NON_LEGACY_FORMATS | _LEGACY_FORMATS)
#define _NO_SIZED_FORMATS 0 /*< @internal No sized formats are supported. */
/**@}*/

/**
 * @internal
 * @~English
 * @brief indicates the profile of the current context.
 */
static GLint contextProfile = 0;
/**
 * @internal
 * @~English
 * @brief Indicates what sized texture formats are supported
 *        by the current context.
 */
static GLint sizedFormats = _ALL_SIZED_FORMATS;
static GLboolean supportsSwizzle = GL_TRUE;
/**
 * @internal
 * @~English
 * @brief Indicates which R16 & RG16 formats are supported by the current context.
 */
static GLint R16Formats = _KTX_ALL_R16_FORMATS;
/**
 * @internal
 * @~English
 * @brief Indicates if the current context supports sRGB textures.
 */
static GLboolean supportsSRGB = GL_TRUE;
/**
 * @internal
 * @~English
 * @brief Indicates if the current context supports cube map arrays.
 */
static GLboolean supportsCubeMapArrays = GL_FALSE;

/**
 * @internal
 * @~English
 * @brief Workaround mismatch of glGetString declaration and standard string
 *        function parameters.
 */
#define glGetString(x) (const char*)glGetString(x)

/**
 * @internal
 * @~English
 * @brief Workaround mismatch of glGetStringi declaration and standard string
 *        function parameters.
 */
#define pfGlGetStringi(x,y) (const char*)pfGlGetStringi(x,y)

/**
 * @internal
 * @~English
 * @brief Check for existence of OpenGL extension
 */
static GLboolean
hasExtension(const char* extension) 
{
    if (pfGlGetStringi == NULL) {
        if (strstr(glGetString(GL_EXTENSIONS), extension) != NULL)
            return GL_TRUE;
        else
            return GL_FALSE;
    } else {
        int i, n;

        glGetIntegerv(GL_NUM_EXTENSIONS, &n);
        for (i = 0; i < n; i++) {
            if (strcmp(pfGlGetStringi(GL_EXTENSIONS, i), extension) == 0)
                return GL_TRUE;
        }
        return GL_FALSE;
    }
}

/**
 * @internal
 * @~English
 * @brief Discover the capabilities of the current GL context.
 *
 * Queries the context and sets several the following internal variables
 * indicating the capabilities of the context:
 *
 * @li sizedFormats
 * @li supportsSwizzle
 * @li supportsSRGB
 * @li b16Formats
 */
static void
discoverContextCapabilities(void)
{
	GLint majorVersion = 1;
	GLint minorVersion = 0;

    // Done here so things will work when GLEW, or equivalent, is being used
    // and GL function names are defined as pointers. Initialization at
    // declaration would happen before these pointers have been initialized.
    INITIALIZE_GL_FUNCPTRS

	if (strstr(glGetString(GL_VERSION), "GL ES") != NULL)
		contextProfile = _CONTEXT_ES_PROFILE_BIT;
	// MAJOR & MINOR only introduced in GL {,ES} 3.0
	glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
	glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
	if (glGetError() != GL_NO_ERROR) {
		// < v3.0; resort to the old-fashioned way.
		if (contextProfile & _CONTEXT_ES_PROFILE_BIT)
			sscanf(glGetString(GL_VERSION), "OpenGL ES %d.%d ",
                   &majorVersion, &minorVersion);
		else
			sscanf(glGetString(GL_VERSION), "OpenGL %d.%d ",
                   &majorVersion, &minorVersion);
	}
	if (contextProfile & _CONTEXT_ES_PROFILE_BIT) {
		if (majorVersion < 3) {
			supportsSwizzle = GL_FALSE;
			sizedFormats = _NO_SIZED_FORMATS;
			R16Formats = _KTX_NO_R16_FORMATS;
			supportsSRGB = GL_FALSE;
		} else {
			sizedFormats = _NON_LEGACY_FORMATS;
            if (hasExtension("GL_EXT_texture_cube_map_array")) {
                supportsCubeMapArrays = GL_TRUE;
            }
		}
		if (hasExtension("GL_OES_required_internalformat")) {
			sizedFormats |= _ALL_SIZED_FORMATS;
		}
		// There are no OES extensions for sRGB textures or R16 formats.
	} else {
		// PROFILE_MASK was introduced in OpenGL 3.2.
		// Profiles: CONTEXT_CORE_PROFILE_BIT 0x1, CONTEXT_COMPATIBILITY_PROFILE_BIT 0x2.
		glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &contextProfile);
		if (glGetError() == GL_NO_ERROR) {
			// >= 3.2
			if (majorVersion == 3 && minorVersion < 3)
				supportsSwizzle = GL_FALSE;
			if ((contextProfile & GL_CONTEXT_CORE_PROFILE_BIT))
				sizedFormats &= ~_LEGACY_FORMATS;
            if (majorVersion >= 4)
                supportsCubeMapArrays = GL_TRUE;
        } else {
			// < 3.2
			contextProfile = GL_CONTEXT_COMPATIBILITY_PROFILE_BIT;
			supportsSwizzle = GL_FALSE;
			// sRGB textures introduced in 2.0
			if (majorVersion < 2 && hasExtension("GL_EXT_texture_sRGB")) {
				supportsSRGB = GL_FALSE;
			}
			// R{,G]16 introduced in 3.0; R{,G}16_SNORM introduced in 3.1.
			if (majorVersion == 3) {
				if (minorVersion == 0)
					R16Formats &= ~_KTX_R16_FORMATS_SNORM;
			} else if (hasExtension("GL_ARB_texture_rg")) {
				R16Formats &= ~_KTX_R16_FORMATS_SNORM;
			} else {
				R16Formats = _KTX_NO_R16_FORMATS;
			}
		}
        if (!supportsCubeMapArrays) {
            if (hasExtension("GL_ARB_texture_cube_map_array")) {
                supportsCubeMapArrays = GL_TRUE;
            }
        }
	}
}

#if SUPPORT_LEGACY_FORMAT_CONVERSION
/**
 * @internal
 * @~English
 * @brief Convert deprecated legacy-format texture to modern format.
 *
 * The function sets the GL_TEXTURE_SWIZZLEs necessary to get the same
 * behavior as the legacy format.
 *
 * @param [in] target       texture target on which the swizzle will
 *                          be set.
 * @param [in, out] pFormat pointer to variable holding the base format of the
 *                          texture. The new base format is written here.
 * @param [in, out] pInternalformat  pointer to variable holding the internalformat
 *                                   of the texture. The new internalformat is
 *                                   written here.
 * @return void unrecognized formats will be passed on to OpenGL. Any loading error
 *              that arises will be handled in the usual way.
 */
static void convertFormat(GLenum target, GLenum* pFormat, GLenum* pInternalformat) {
	switch (*pFormat) {
	  case GL_ALPHA:
		{
		  GLint swizzle[] = {GL_ZERO, GL_ZERO, GL_ZERO, GL_RED};
		  *pFormat = GL_RED;
		  glTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
		  switch (*pInternalformat) {
	        case GL_ALPHA:
		    case GL_ALPHA4:
		    case GL_ALPHA8:
			  *pInternalformat = GL_R8;
			  break;
		    case GL_ALPHA12:
		    case GL_ALPHA16:
			  *pInternalformat = GL_R16;
			  break;
		  }
		}
	  case GL_LUMINANCE:
		{
		  GLint swizzle[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
		  *pFormat = GL_RED;
		  glTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
		  switch (*pInternalformat) {
			case GL_LUMINANCE:
		    case GL_LUMINANCE4:
		    case GL_LUMINANCE8:
			  *pInternalformat = GL_R8;
			  break;
		    case GL_LUMINANCE12:
		    case GL_LUMINANCE16:
			  *pInternalformat = GL_R16;
			  break;
#if 0
		    // XXX Must avoid setting TEXTURE_SWIZZLE in these cases
            // XXX Must manually swizzle.
			case GL_SLUMINANCE:
			case GL_SLUMINANCE8:
			  *pInternalformat = GL_SRGB8;
			  break;
#endif
		  }
		  break;
		}
	  case GL_LUMINANCE_ALPHA:
		{
		  GLint swizzle[] = {GL_RED, GL_RED, GL_RED, GL_GREEN};
		  *pFormat = GL_RG;
		  glTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
		  switch (*pInternalformat) {
			case GL_LUMINANCE_ALPHA:
		    case GL_LUMINANCE4_ALPHA4:
			case GL_LUMINANCE6_ALPHA2:
		    case GL_LUMINANCE8_ALPHA8:
			  *pInternalformat = GL_RG8;
			  break;
		    case GL_LUMINANCE12_ALPHA4:
			case GL_LUMINANCE12_ALPHA12:
		    case GL_LUMINANCE16_ALPHA16:
			  *pInternalformat = GL_RG16;
			  break;
#if 0
		    // XXX Must avoid setting TEXTURE_SWIZZLE in these cases
            // XXX Must manually swizzle.
			case GL_SLUMINANCE_ALPHA:
			case GL_SLUMINANCE8_ALPHA8:
			  *pInternalformat = GL_SRGB8_ALPHA8;
			  break;
#endif
		  }
		  break;
		}
	  case GL_INTENSITY:
		{
		  GLint swizzle[] = {GL_RED, GL_RED, GL_RED, GL_RED};
		  *pFormat = GL_RED;
		  glTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
		  switch (*pInternalformat) {
			case GL_INTENSITY:
		    case GL_INTENSITY4:
		    case GL_INTENSITY8:
			  *pInternalformat = GL_R8;
			  break;
		    case GL_INTENSITY12:
		    case GL_INTENSITY16:
			  *pInternalformat = GL_R16;
			  break;
		  }
		  break;
		}
	  default:
	    break;
	}
}
#endif /* SUPPORT_LEGACY_FORMAT_CONVERSION */

typedef struct ktx_cbdata {
    GLenum glTarget;
    GLenum glFormat;
    GLenum glInternalformat;
    GLenum glType;
    GLenum glError;
} ktx_cbdata;

KTX_error_code KTXAPIENTRY
texImage1DCallback(int miplevel, int face,
                   int width, int heightOrLayers,
                   int depthOrLayers,
                   ktx_uint32_t faceLodSize,
                   void* pixels, void* userdata)
{
    ktx_cbdata* cbData = (ktx_cbdata*)userdata;
    
    assert(pfGlTexImage1D != NULL);
    pfGlTexImage1D(cbData->glTarget + face, miplevel,
                   cbData->glInternalformat, width, 0,
                   cbData->glFormat, cbData->glType, pixels);
    
    if ((cbData->glError = glGetError()) == GL_NO_ERROR) {
        return KTX_SUCCESS;
    } else {
        return KTX_GL_ERROR;
    }
}

KTX_error_code KTXAPIENTRY
compressedTexImage1DCallback(int miplevel, int face,
                             int width, int heightOrLayers,
                             int depthOrLayers,
                             ktx_uint32_t faceLodSize,
                             void* pixels, void* userdata)
{
    ktx_cbdata* cbData = (ktx_cbdata*)userdata;
    
    assert(pfGlCompressedTexImage1D != NULL);
    pfGlCompressedTexImage1D(cbData->glTarget + face, miplevel,
                             cbData->glInternalformat, width, 0,
                             cbData->glType, pixels);
    
    if ((cbData->glError = glGetError()) == GL_NO_ERROR) {
        return KTX_SUCCESS;
    } else {
        return KTX_GL_ERROR;
    }
}

KTX_error_code KTXAPIENTRY
texImage2DCallback(int miplevel, int face,
                   int width, int heightOrLayers,
                   int depthOrLayers,
                   ktx_uint32_t faceLodSize,
                   void* pixels, void* userdata)
{
    ktx_cbdata* cbData = (ktx_cbdata*)userdata;
 
    glTexImage2D(cbData->glTarget + face, miplevel,
                 cbData->glInternalformat, width, heightOrLayers, 0,
                 cbData->glFormat, cbData->glType, pixels);

    if ((cbData->glError = glGetError()) == GL_NO_ERROR) {
        return KTX_SUCCESS;
    } else {
        return KTX_GL_ERROR;
    }
}


KTX_error_code KTXAPIENTRY
compressedTexImage2DCallback(int miplevel, int face,
                             int width, int heightOrLayers,
                             int depthOrLayers,
                             ktx_uint32_t faceLodSize,
                             void* pixels, void* userdata)
{
    ktx_cbdata* cbData = (ktx_cbdata*)userdata;
    GLenum glerror;
    KTX_error_code errorCode;
    
    // It is simpler to just attempt to load the format, rather than divine which
    // formats are supported by the implementation. In the event of an error,
    // software unpacking can be attempted.
    glCompressedTexImage2D(cbData->glTarget + face, miplevel,
                           cbData->glInternalformat, width, heightOrLayers, 0,
                           faceLodSize, pixels);
    
    glerror = glGetError();
#if SUPPORT_SOFTWARE_ETC_UNPACK
    // Renderion is returning INVALID_VALUE. Oops!!
    if ((glerror == GL_INVALID_ENUM || glerror == GL_INVALID_VALUE)
        && (cbData->glInternalformat == GL_ETC1_RGB8_OES
            || (cbData->glInternalformat >= GL_COMPRESSED_R11_EAC
                && cbData->glInternalformat <= GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC)
            ))
    {
        GLubyte* unpacked;
        GLenum format, internalformat, type;
        
        errorCode = _ktxUnpackETC((GLubyte*)pixels, cbData->glInternalformat,
                                  width, heightOrLayers, &unpacked,
                                  &format, &internalformat,
                                  &type, R16Formats, supportsSRGB);
        if (errorCode != KTX_SUCCESS) {
            return errorCode;
        }
        if (!(sizedFormats & _NON_LEGACY_FORMATS)) {
            if (internalformat == GL_RGB8)
                internalformat = GL_RGB;
            else if (internalformat == GL_RGBA8)
                internalformat = GL_RGBA;
        }
        glTexImage2D(cbData->glTarget + face, miplevel,
                     internalformat,
                     width, heightOrLayers,
                     0,
                     format, type, unpacked);
        
        free(unpacked);
        glerror = glGetError();
    }
#endif
    
    if ((cbData->glError = glerror) == GL_NO_ERROR) {
        return KTX_SUCCESS;
    } else {
        return KTX_GL_ERROR;
    }
}

KTX_error_code KTXAPIENTRY
texImage3DCallback(int miplevel, int face,
                   int width, int heightOrLayers,
                   int depthOrLayers,
                   ktx_uint32_t faceLodSize,
                   void* pixels, void* userdata)
{
    ktx_cbdata* cbData = (ktx_cbdata*)userdata;
    
    assert(pfGlTexImage3D != NULL);
    pfGlTexImage3D(cbData->glTarget + face, miplevel,
                   cbData->glInternalformat,
                   width, heightOrLayers, depthOrLayers,
                   0,
                   cbData->glFormat, cbData->glType, pixels);
    
    if ((cbData->glError = glGetError()) == GL_NO_ERROR) {
        return KTX_SUCCESS;
    } else {
        return KTX_GL_ERROR;
    }
}

KTX_error_code KTXAPIENTRY
compressedTexImage3DCallback(int miplevel, int face,
                             int width, int heightOrLayers,
                             int depthOrLayers,
                             ktx_uint32_t faceLodSize,
                             void* pixels, void* userdata)
{
    ktx_cbdata* cbData = (ktx_cbdata*)userdata;
    
    assert(pfGlCompressedTexImage3D != NULL);
    pfGlCompressedTexImage3D(cbData->glTarget + face, miplevel,
                             cbData->glInternalformat,
                             width, heightOrLayers, depthOrLayers,
                             0,
                             cbData->glType, pixels);
    
    if ((cbData->glError = glGetError()) == GL_NO_ERROR) {
        return KTX_SUCCESS;
    } else {
        return KTX_GL_ERROR;
    }
}

/**
 * @~English
 * @brief Load a GL texture object from a file represented by a ktxContext.
 *
 * The function sets the texture object's GL_TEXTURE_MAX_LEVEL parameter
 * according to the number of levels in the ktxStream, provided the library
 * has been compiled with a version of gl.h where GL_TEXTURE_MAX_LEVEL is
 * defined.
 *
 * It will unpack compressed GL_ETC1_RGB8_OES and GL_ETC2_* format
 * textures in software when the format is not supported by the GL context,
 * provided the library has been compiled with SUPPORT_SOFTWARE_ETC_UNPACK
 * defined as 1.
 *
 * It will also convert textures with legacy formats to their modern equivalents
 * when the format is not supported by the GL context, provided the library
 * has been compiled with SUPPORT_LEGACY_FORMAT_CONVERSION defined as 1.
 *
 * @param [in] ctx			handle of the KTX_context representing the file
 *                          from which to load.
 * @param [in,out] pTexture	name of the GL texture object to load. If NULL or if
 *                          <tt>*pTexture == 0</tt> the function will generate
 *                          a texture name. The function binds either the
 *                          generated name or the name given in @p *pTexture
 * 						    to the texture target returned in @p *pTarget,
 * 						    before loading the texture data. If @p pTexture
 *                          is not NULL and a name was generated, the generated
 *                          name will be returned in *pTexture.
 * @param [out] pTarget 	@p *pTarget is set to the texture target used. The
 * 						    target is chosen based on the file contents.
 * @param [out] pDimensions	If @p pDimensions is not NULL, the width, height and
 *							depth of the texture's base level are returned in
 *                          the fields of the KTX_dimensions structure to which
 *                          it points.
 * @param [out] pIsMipmapped
 *	                        If @p pIsMipmapped is not NULL, @p *pIsMipmapped is
 *                          set to GL_TRUE if the KTX texture is mipmapped,
 *                          GL_FALSE otherwise.
 * @param [out] pGlerror    @p *pGlerror is set to the value returned by
 *                          glGetError when this function returns the error
 *                          KTX_GL_ERROR. glerror can be NULL.
 * @param [in,out] pKvdLen	If not NULL, @p *pKvdLen is set to the number of
 *                          bytes of key-value data pointed at by @p *ppKvd.
 *                          Must not be NULL, if @p ppKvd is not NULL.
 * @param [in,out] ppKvd	If not NULL, @p *ppKvd is set to the point to a
 *                          block of memory containing key-value data read from
 *                          the file. The application is responsible for freeing
 *                          the memory.
 *
 *
 * @return	KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p target is @c NULL or the size of a mip
 * 							    level is greater than the size of the
 * 							    preceding level.
 * @exception KTX_INVALID_OPERATION @p ppKvd is not NULL but pKvdLen is NULL.
 * @exception KTX_UNEXPECTED_END_OF_FILE the file does not contain the
 * 										 expected amount of data.
 * @exception KTX_OUT_OF_MEMORY Sufficient memory could not be allocated to
 *                              store the requested key-value data.
 * @exception KTX_GL_ERROR      A GL error was raised by glBindTexture,
 * 								glGenTextures or gl*TexImage*. The GL error
 *                              will be returned in @p *glerror, if glerror
 *                              is not @c NULL.
 */
KTX_error_code
ktxLoadTexture(KTX_context ctx, GLuint* pTexture, GLenum* pTarget,
               KTX_dimensions* pDimensions, GLboolean* pIsMipmapped,
               GLenum* pGlerror,
               unsigned int* pKvdLen, unsigned char** ppKvd)
{
    ktxContext*           kc = (ktxContext*)ctx;
    KTX_header	          header;
    KTX_supplemental_info texinfo;
	GLint				  previousUnpackAlignment;
	GLuint				  texname;
	int					  texnameUser;
	KTX_error_code		  errorCode = KTX_SUCCESS;
    PFNKTXIMAGECB         imageCb = NULL;
    ktx_cbdata            cbData;
    int                   dimension;

	if (pGlerror)
		*pGlerror = GL_NO_ERROR;

	if (ppKvd) {
		*ppKvd = NULL;
	}

	if (!kc || !kc->stream.read || !kc->stream.skip) {
		return KTX_INVALID_VALUE;
	}

	if (!pTarget) {
		return KTX_INVALID_VALUE;
	}

    errorCode = ktxReadHeader(kc, &header, &texinfo);
	if (errorCode != KTX_SUCCESS)
		return errorCode;

    ktxReadKVData(kc, pKvdLen, ppKvd);
    if (errorCode != KTX_SUCCESS) {
        return errorCode;
    }

	if (contextProfile == 0)
		discoverContextCapabilities();

	/* KTX files require an unpack alignment of 4 */
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);
	if (previousUnpackAlignment != KTX_GL_UNPACK_ALIGNMENT) {
		glPixelStorei(GL_UNPACK_ALIGNMENT, KTX_GL_UNPACK_ALIGNMENT);
	}

    cbData.glFormat = header.glFormat;
    cbData.glInternalformat = header.glInternalFormat;
    cbData.glType = header.glType;

	texnameUser = pTexture && *pTexture;
	if (texnameUser) {
		texname = *pTexture;
	} else {
		glGenTextures(1, &texname);
	}

    dimension = texinfo.textureDimension;
    if (header.numberOfArrayElements > 0) {
        dimension += 1;
        if (header.numberOfFaces == 6) {
            /* _ktxCheckHeader should have caught this. */
            assert(texinfo.textureDimension == 2);
            cbData.glTarget = GL_TEXTURE_CUBE_MAP_ARRAY;
        } else {
            switch (texinfo.textureDimension) {
              case 1: cbData.glTarget = GL_TEXTURE_1D_ARRAY_EXT; break;
              case 2: cbData.glTarget = GL_TEXTURE_2D_ARRAY_EXT; break;
                /* _ktxCheckHeader should have caught this. */
              default: assert(KTX_TRUE);
            }
        }
    } else {
        if (header.numberOfFaces == 6) {
            /* _ktxCheckHeader should have caught this. */
            assert(texinfo.textureDimension == 2);
            cbData.glTarget = GL_TEXTURE_CUBE_MAP;
        } else {
            switch (texinfo.textureDimension) {
              case 1: cbData.glTarget = GL_TEXTURE_1D; break;
              case 2: cbData.glTarget = GL_TEXTURE_2D; break;
                case 3: cbData.glTarget = GL_TEXTURE_3D; break;
              /* _ktxCheckHeader shold have caught this. */
              default: assert(KTX_TRUE);
            }
        }
    }
    
    if (cbData.glTarget == GL_TEXTURE_1D &&
        ((texinfo.compressed && (pfGlCompressedTexImage1D == NULL)) ||
         (!texinfo.compressed && (pfGlTexImage1D == NULL))))
    {
        return KTX_UNSUPPORTED_TEXTURE_TYPE;
    }
    
    /* Reject 3D texture if unsupported. */
    if (cbData.glTarget == GL_TEXTURE_3D &&
        ((texinfo.compressed && (pfGlCompressedTexImage3D == NULL)) ||
         (!texinfo.compressed && (pfGlTexImage3D == NULL))))
    {
        return KTX_UNSUPPORTED_TEXTURE_TYPE;
    }
    
    /* Reject cube map arrays if not supported. */
    if (cbData.glTarget == GL_TEXTURE_CUBE_MAP_ARRAY && !supportsCubeMapArrays) {
        return KTX_UNSUPPORTED_TEXTURE_TYPE;
    }
    
    /* XXX Need to reject other array textures & cube maps if not supported. */
    
    switch (dimension) {
      case 1:
        imageCb = texinfo.compressed
                  ? compressedTexImage1DCallback : texImage1DCallback;
        break;
      case 2:
        imageCb = texinfo.compressed
                  ? compressedTexImage2DCallback : texImage2DCallback;
            break;
      case 3:
        imageCb = texinfo.compressed
                  ? compressedTexImage3DCallback : texImage3DCallback;
        break;
      default:
            assert(KTX_TRUE);
    }
   
    glBindTexture(cbData.glTarget, texname);
    
    // Prefer glGenerateMipmaps over GL_GENERATE_MIPMAP
    if (texinfo.generateMipmaps && (pfGlGenerateMipmap == NULL)) {
        glTexParameteri(cbData.glTarget, GL_GENERATE_MIPMAP, GL_TRUE);
    }
#ifdef GL_TEXTURE_MAX_LEVEL
	if (!texinfo.generateMipmaps)
		glTexParameteri(cbData.glTarget, GL_TEXTURE_MAX_LEVEL, header.numberOfMipmapLevels - 1);
#endif

	if (cbData.glTarget == GL_TEXTURE_CUBE_MAP) {
		cbData.glTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
	}
    
	cbData.glInternalformat = header.glInternalFormat;
	cbData.glFormat = header.glFormat;
	if (!texinfo.compressed) {
#if SUPPORT_LEGACY_FORMAT_CONVERSION
		// If sized legacy formats are supported there is no need to convert.
		// If only unsized formats are supported, there is no point in converting
		// as the modern formats aren't supported either.
		if (sizedFormats == _NON_LEGACY_FORMATS && supportsSwizzle) {
			convertFormat(cbData.glTarget, &cbData.glFormat, &cbData.glInternalformat);
		} else if (sizedFormats == _NO_SIZED_FORMATS)
			cbData.glInternalformat = header.glBaseInternalFormat;
#else
		// When no sized formats are supported, or legacy sized formats are not
		// supported, must change internal format.
		if (sizedFormats == _NO_SIZED_FORMATS
			|| (!(sizedFormats & _LEGACY_FORMATS) &&
				(header.glBaseInternalFormat == GL_ALPHA
				|| header.glBaseInternalFormat == GL_LUMINANCE
				|| header.glBaseInternalFormat == GL_LUMINANCE_ALPHA
				|| header.glBaseInternalFormat == GL_INTENSITY))) {
			cbData.glInternalformat = header.glBaseInternalFormat;
		}
#endif
	}

    errorCode = ktxReadImages((void*)kc, imageCb, &cbData);
    /* GL errors are the only reason for failure. */
    if (errorCode != KTX_SUCCESS && cbData.glError != GL_NO_ERROR) {
        if (pGlerror)
            *pGlerror = cbData.glError;
    }

	/* restore previous GL state */
	if (previousUnpackAlignment != KTX_GL_UNPACK_ALIGNMENT) {
		glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);
	}

	if (errorCode == KTX_SUCCESS)
	{
        // Prefer glGenerateMipmaps over GL_GENERATE_MIPMAP
        if (texinfo.generateMipmaps && pfGlGenerateMipmap) {
			pfGlGenerateMipmap(cbData.glTarget);
		}
		*pTarget = cbData.glTarget;
		if (pTexture) {
			*pTexture = texname;
		}
		if (pDimensions) {
			pDimensions->width = header.pixelWidth;
			pDimensions->height = header.pixelHeight;
			pDimensions->depth = header.pixelDepth;
		}
		if (pIsMipmapped) {
			if (texinfo.generateMipmaps || header.numberOfMipmapLevels > 1)
				*pIsMipmapped = GL_TRUE;
			else
				*pIsMipmapped = GL_FALSE;
		}
	} else {
		if (ppKvd && *ppKvd)
		{
			free(*ppKvd);
			*ppKvd = NULL;
		}

		if (!texnameUser) {
			glDeleteTextures(1, &texname);
		}
	}
	return errorCode;
}

/**
 * @~English
 * @brief Load a GL texture object from a stdio FILE stream.
 *
 * The function sets the texture object's GL_TEXTURE_MAX_LEVEL parameter
 * according to the number of levels in the ktxStream, provided the library
 * has been compiled with a version of gl.h where GL_TEXTURE_MAX_LEVEL is
 * defined.
 *
 * It will unpack compressed GL_ETC1_RGB8_OES and GL_ETC2_* format
 * textures in software when the format is not supported by the GL context,
 * provided the library has been compiled with SUPPORT_SOFTWARE_ETC_UNPACK
 * defined as 1.
 *
 * It will also convert texture with legacy formats to their modern equivalents
 * when the format is not supported by the GL context, provided the library
 * has been compiled with SUPPORT_LEGACY_FORMAT_CONVERSION defined as 1.
 *
 * @param [in] file			pointer to the stdio FILE stream from which to
 * 							load.
 * @param [in,out] pTexture	name of the GL texture object to load. If NULL or if
 *                          <tt>*pTexture == 0</tt> the function will generate
 *                          a texture name. The function binds either the
 *                          generated name or the name given in @p *pTexture
 * 						    to the texture target returned in @p *pTarget,
 * 						    before loading the texture data. If @p pTexture
 *                          is not NULL and a name was generated, the generated
 *                          name will be returned in *pTexture.
 * @param [out] pTarget 	@p *pTarget is set to the texture target used. The
 * 						    target is chosen based on the file contents.
 * @param [out] pDimensions	If @p pDimensions is not NULL, the width, height and
 *							depth of the texture's base level are returned in the
 *                          fields of the KTX_dimensions structure to which it points.
 * @param [out] pIsMipmapped
 *	                        If @p pIsMipmapped is not NULL, @p *pIsMipmapped is set
 *                          to GL_TRUE if the KTX texture is mipmapped, GL_FALSE
 *                          otherwise.
 * @param [out] pGlerror    @p *pGlerror is set to the value returned by
 *                          glGetError when this function returns the error
 *                          KTX_GL_ERROR. glerror can be NULL.
 * @param [in,out] pKvdLen	If not NULL, @p *pKvdLen is set to the number of bytes
 *                          of key-value data pointed at by @p *ppKvd. Must not be
 *                          NULL, if @p ppKvd is not NULL.
 * @param [in,out] ppKvd	If not NULL, @p *ppKvd is set to the point to a block of
 *                          memory containing key-value data read from the file.
 *                          The application is responsible for freeing the memory.
 *
 *
 * @return	KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p target is @c NULL or the size of a mip
 * 							    level is greater than the size of the
 * 							    preceding level.
 * @exception KTX_INVALID_OPERATION @p ppKvd is not NULL but pKvdLen is NULL.
 * @exception KTX_UNEXPECTED_END_OF_FILE the file does not contain the
 * 										 expected amount of data.
 * @exception KTX_OUT_OF_MEMORY Sufficient memory could not be allocated to store
 *                              the requested key-value data.
 * @exception KTX_GL_ERROR      A GL error was raised by glBindTexture,
 * 								glGenTextures or gl*TexImage*. The GL error
 *                              will be returned in @p *glerror, if glerror
 *                              is not @c NULL.
 */
KTX_error_code
ktxLoadTextureF(FILE* file, GLuint* pTexture, GLenum* pTarget,
				KTX_dimensions* pDimensions, GLboolean* pIsMipmapped,
				GLenum* pGlerror,
				unsigned int* pKvdLen, unsigned char** ppKvd)
{
	KTX_context ctx;
	KTX_error_code errorCode = KTX_SUCCESS;

	errorCode = ktxOpenKTXF(file, &ctx);
	if (errorCode != KTX_SUCCESS)
		return errorCode;

    errorCode = ktxLoadTexture(ctx, pTexture, pTarget, pDimensions, pIsMipmapped,
                               pGlerror, pKvdLen, ppKvd);
    ktxCloseKTX(ctx);

    return errorCode;
}

/**
 * @~English
 * @brief Load a GL texture object from a named file on disk.
 *
 * @param [in] filename		pointer to a C string that contains the path of
 * 							the file to load.
 * @param [in,out] pTexture	name of the GL texture object to load. See
 *                          ktxLoadTextureF() for details.
 * @param [out] pTarget 	@p *pTarget is set to the texture target used. See
 *                          ktxLoadTextureF() for details.
 * @param [out] pDimensions @p the texture's base level width depth and height
 *                          are returned in structure to which this points.
 *                          See ktxLoadTextureF() for details.
 * @param [out] pIsMipmapped @p pIsMipMapped is set to indicate if the loaded
 *                          texture is mipmapped. See ktxLoadTextureF() for
 *                          details.
 * @param [out] pGlerror    @p *pGlerror is set to the value returned by
 *                          glGetError when this function returns the error
 *                          KTX_GL_ERROR. glerror can be NULL.
 * @param [in,out] pKvdLen	If not NULL, @p *pKvdLen is set to the number of bytes
 *                          of key-value data pointed at by @p *ppKvd. Must not be
 *                          NULL, if @p ppKvd is not NULL.
 * @param [in,out] ppKvd	If not NULL, @p *ppKvd is set to the point to a block of
 *                          memory containing key-value data read from the file.
 *                          The application is responsible for freeing the memory.*
 * @return	KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_FILE_OPEN_FAILED	The specified file could not be opened.
 * @exception KTX_INVALID_VALUE		See ktxLoadTextureF() for causes.
 * @exception KTX_INVALID_OPERATION	See ktxLoadTextureF() for causes.
 * @exception KTX_UNEXPECTED_END_OF_FILE See ktxLoadTextureF() for causes.
 *
 * @exception KTX_GL_ERROR			See ktxLoadTextureF() for causes.
 */
KTX_error_code
ktxLoadTextureN(const char* const filename, GLuint* pTexture, GLenum* pTarget,
				KTX_dimensions* pDimensions, GLboolean* pIsMipmapped,
				GLenum* pGlerror,
				unsigned int* pKvdLen, unsigned char** ppKvd)
{
	KTX_error_code errorCode;
	FILE* file = fopen(filename, "rb");

	if (file) {
		errorCode = ktxLoadTextureF(file, pTexture, pTarget, pDimensions,
								    pIsMipmapped, pGlerror, pKvdLen, ppKvd);
		fclose(file);
	} else
		errorCode = KTX_FILE_OPEN_FAILED;

	return errorCode;
}

/**
 * @~English
 * @brief Load a GL texture object from KTX formatted data in memory.
 *
 * @param [in] bytes		pointer to the array of bytes containing
 * 							the KTX format data to load.
 * @param [in] size			size of the memory array containing the
 *                          KTX format data.
 * @param [in,out] pTexture	name of the GL texture object to load. See
 *                          ktxLoadTextureF() for details.
 * @param [out] pTarget 	@p *pTarget is set to the texture target used. See
 *                          ktxLoadTextureF() for details.
 * @param [out] pDimensions @p the texture's base level width depth and height
 *                          are returned in structure to which this points.
 *                          See ktxLoadTextureF() for details.
 * @param [out] pIsMipmapped @p *pIsMipMapped is set to indicate if the loaded
 *                          texture is mipmapped. See ktxLoadTextureF() for
 *                          details.
 * @param [out] pGlerror    @p *pGlerror is set to the value returned by
 *                          glGetError when this function returns the error
 *                          KTX_GL_ERROR. glerror can be NULL.
 * @param [in,out] pKvdLen	If not NULL, @p *pKvdLen is set to the number of bytes
 *                          of key-value data pointed at by @p *ppKvd. Must not be
 *                          NULL, if @p ppKvd is not NULL.
 * @param [in,out] ppKvd	If not NULL, @p *ppKvd is set to the point to a block of
 *                          memory containing key-value data read from the file.
 *                          The application is responsible for freeing the memory.*
 *
 * @return	KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_FILE_OPEN_FAILED	The specified memory could not be opened as a file.
 * @exception KTX_INVALID_VALUE		See ktxLoadTextureF() for causes.
 * @exception KTX_INVALID_OPERATION	See ktxLoadTextureF() for causes.
 * @exception KTX_UNEXPECTED_END_OF_FILE See ktxLoadTextureF() for causes.
 *
 * @exception KTX_GL_ERROR			See ktxLoadTextureF() for causes.
 */
KTX_error_code
ktxLoadTextureM(const void* bytes, GLsizei size, GLuint* pTexture,
                GLenum* pTarget, KTX_dimensions* pDimensions,
                GLboolean* pIsMipmapped, GLenum* pGlerror,
				unsigned int* pKvdLen, unsigned char** ppKvd)
{
	KTX_context ctx;
	KTX_error_code errorCode = KTX_SUCCESS;

	errorCode = ktxOpenKTXM(bytes, size, &ctx);
	if (errorCode != KTX_SUCCESS)
		return errorCode;

	errorCode = ktxLoadTexture(ctx, pTexture, pTarget, pDimensions,
                               pIsMipmapped, pGlerror, pKvdLen, ppKvd);
    ktxCloseKTX(ctx);
    
    return errorCode;
}



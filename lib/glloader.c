/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2010-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @internal
 * @file
 * @~English
 *
 * @brief Functions for instantiating GL or GLES textures from KTX files.
 *
 * @author Georg Kolling, Imagination Technology
 * @author Mark Callow, HI Corporation & Edgewise Consulting
 */

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "gl_funcs.h"

#include "ktx.h"
#include "ktxint.h"
#include "texture.h"
#include "vk2gl.h"
#include "unused.h"

/**
 * @defgroup ktx_glloader OpenGL Texture Image Loader
 * @brief Create texture objects in current OpenGL context.
 * @{
 */

/*
 * These are defined only in compatibility mode (gl.h) not glcorearb.h
 */
#if !defined( GL_LUMINANCE )
#define GL_LUMINANCE                  0x1909	// deprecated
#endif
#if !defined( GL_LUMINANCE_ALPHA )
#define GL_LUMINANCE_ALPHA			  0x190A	// deprecated
#endif
#if !defined( GL_INTENSITY )
#define GL_INTENSITY				  0x8049	// deprecated
#endif

/*
 * N.B. As of Doxygen 1.9.6 non-class members must use fully qualified
 * names with @ref and @copy* references to classes. This means prefixing
 * a reference with the name of the (pseudo-)class of which it is a member.
 * We use @memberof to improve the index and toc for the doc for our
 * pseudo classes so we need to prefix. Since we don't want, e.g.,
 * ktxTexture1::ktxTexture1_GLUpload appearing in the documentation we have
 * to explicitly provide the link text making references very long-winded.
 * Sigh!
 */

/**
 * @example glloader.c
 * This is an example of using the low-level ktxTexture API to create and load
 * an OpenGL texture. It is a fragment of the code used by
 * @ref ktxTexture1::ktxTexture1\_GLUpload "ktxTexture1_GLUpload" and
 * @ref ktxTexture2::ktxTexture2\_GLUpload "ktxTexture2_GLUpload".
 *
 * @code
 * #include <ktx.h>
 * @endcode
 *
 * This structure is used to pass to a callback function data that is uniform
 * across all images.
 * @snippet this cbdata
 *
 * One of these callbacks, selected by @ref
 * ktxTexture1.ktxTexture1\_GLUpload "ktxTexture1_GLUpload" or
 * @ref ktxTexture2.ktxTexture2\_GLUpload "ktxTexture2_GLUpload" based on the
 * dimensionality and arrayness of the texture, is called from
 * @ref ktxTexture::ktxTexture_IterateLevelFaces
 * "ktxTexture_IterateLevelFaces" to upload the texture data to OpenGL.
 * @snippet this imageCallbacks
 *
 * This function creates the GL texture object and sets up the callbacks to
 * load the image data into it.
 * @snippet this loadGLTexture
 */

/**
 * @internal
 * @~English
 * @brief Token for use with OpenGL ES 1 and old versions of OpenGL.
 *
 * Only used when glGenerateMipmaps not available.
 */
#define GL_GENERATE_MIPMAP              0x8191

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
 * @brief Indicates which R16 & RG16 formats are supported by the current
 *        context.
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
 * @brief Indicates if the current context supports cube map arrays.
 */
static GLboolean supportsMaxLevel = GL_FALSE;

/**
 * @internal
 * @~English
 * @brief Check for existence of OpenGL extension
 */
static GLboolean
hasExtension(const char* extension)
{
    if (gl.glGetStringi == NULL) {
        if (strstr(glGetString(GL_EXTENSIONS), extension) != NULL) {
            return GL_TRUE;
        } else {
            return GL_FALSE;
        }
    } else {
        int i, n;

        glGetIntegerv(GL_NUM_EXTENSIONS, &n);
        for (i = 0; i < n; i++) {
            if (strcmp((const char*)gl.glGetStringi(GL_EXTENSIONS, i), extension) == 0)
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
 * @li supportsMaxLevel
 */
static void
discoverContextCapabilities(void)
{
    GLint majorVersion = 1;
    GLint minorVersion = 0;

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
            // These things could be found by dlsym when, e.g. the same driver
            // supports ES1, ES2 and ES3. For all but Tex*3D, there's no
            // corresponding extension whose presence we could check. Just zero
            // the pointers to prevent use.
            gl.glGetStringi = NULL;
            gl.glCompressedTexImage1D = NULL;
            gl.glTexStorage1D = NULL;
            gl.glTexStorage2D = NULL;
            gl.glTexStorage3D = NULL;
            if (!hasExtension("GL_OES_texture_3D")) {
                gl.glCompressedTexImage3D = NULL;
                gl.glCompressedTexSubImage3D = NULL;
                gl.glTexImage3D = NULL;
                gl.glTexSubImage3D = NULL;
            }
            if (majorVersion < 2)
                gl.glGenerateMipmap = NULL;

        } else {
            sizedFormats = _NON_LEGACY_FORMATS;
            if (hasExtension("GL_EXT_texture_cube_map_array")) {
                supportsCubeMapArrays = GL_TRUE;
            }
            supportsMaxLevel = GL_TRUE;
        }
        if (hasExtension("GL_OES_required_internalformat")) {
            sizedFormats |= _ALL_SIZED_FORMATS;
        }
        // There are no OES extensions for sRGB textures or R16 formats.
    } else {
        // PROFILE_MASK was introduced in OpenGL 3.2.
        // Profiles: CONTEXT_CORE_PROFILE_BIT 0x1,
        //           CONTEXT_COMPATIBILITY_PROFILE_BIT 0x2.
        glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &contextProfile);
        if (glGetError() == GL_NO_ERROR) {
            // >= 3.2
            if (majorVersion == 3 && minorVersion < 3)
                supportsSwizzle = GL_FALSE;
            if ((contextProfile & GL_CONTEXT_CORE_PROFILE_BIT))
                sizedFormats &= ~_LEGACY_FORMATS;
            if (majorVersion >= 4)
                supportsCubeMapArrays = GL_TRUE;
            supportsMaxLevel = GL_TRUE;
        } else {
            // < 3.2
            contextProfile = GL_CONTEXT_COMPATIBILITY_PROFILE_BIT;
            supportsSwizzle = GL_FALSE;
            // sRGB textures introduced in 2.0
            if (majorVersion < 2 && !hasExtension("GL_EXT_texture_sRGB")) {
                supportsSRGB = GL_FALSE;
            }
            // R{,G]16 introduced in 3.0; R{,G}16_SNORM introduced in 3.1.
            if (majorVersion == 3) {
                if (minorVersion == 0)
                    R16Formats &= ~_KTX_R16_FORMATS_SNORM;
                if (minorVersion < 1) {
                    if (hasExtension("GL_ARB_texture_query_levels"))
                        supportsMaxLevel = GL_TRUE;
                } else {
                  supportsMaxLevel = GL_TRUE;
                }
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
 * @param[in] target       texture target on which the swizzle will
 *                          be set.
 * @param[in,out] pFormat  pointer to variable holding the base format of the
 *                          texture. The new base format is written here.
 * @param[in,out] pInternalformat  pointer to variable holding the
 *                                  internalformat of the texture. The new
 *                                  internalformat is written here.
 * @return void unrecognized formats will be passed on to OpenGL. Any loading
 *              error that arises will be handled in the usual way.
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

typedef struct ktx_glformatinfo {
   ktx_uint32_t glFormat;
   ktx_uint32_t glInternalformat;
   ktx_uint32_t glBaseInternalformat;
   ktx_uint32_t glType;
} ktx_glformatinfo;

/* [cbdata] */
typedef struct ktx_cbdata {
    GLenum glTarget;
    GLenum glFormat;
    GLenum glInternalformat;
    GLenum glType;
    GLenum glError;
    GLuint numLayers;
} ktx_cbdata;
/* [cbdata] */

/* [imageCallbacks] */

KTX_error_code
texImage1DCallback(int miplevel, int face,
                   int width, int height,
                   int depth,
                   ktx_uint64_t faceLodSize,
                   void* pixels, void* userdata)
{
    ktx_cbdata* cbData = (ktx_cbdata*)userdata;
    UNUSED(faceLodSize);
    UNUSED(depth);
    UNUSED(height);

    assert(gl.glTexImage1D != NULL);
    gl.glTexImage1D(cbData->glTarget + face, miplevel,
                   cbData->glInternalformat, width, 0,
                   cbData->glFormat, cbData->glType, pixels);

    if ((cbData->glError = glGetError()) == GL_NO_ERROR) {
        return KTX_SUCCESS;
    } else {
        return KTX_GL_ERROR;
    }
}

KTX_error_code
compressedTexImage1DCallback(int miplevel, int face,
                             int width, int height,
                             int depth,
                             ktx_uint64_t faceLodSize,
                             void* pixels, void* userdata)
{
    ktx_cbdata* cbData = (ktx_cbdata*)userdata;
    UNUSED(depth);
    UNUSED(height);

    if (faceLodSize > UINT32_MAX)
        return KTX_INVALID_OPERATION; // Too big for OpenGL {,ES}.

    assert(gl.glCompressedTexImage1D != NULL);
    gl.glCompressedTexImage1D(cbData->glTarget + face, miplevel,
                             cbData->glInternalformat, width, 0,
                             (ktx_uint32_t)faceLodSize, pixels);

    if ((cbData->glError = glGetError()) == GL_NO_ERROR) {
        return KTX_SUCCESS;
    } else {
        return KTX_GL_ERROR;
    }
}

KTX_error_code
texImage2DCallback(int miplevel, int face,
                   int width, int height,
                   int depth,
                   ktx_uint64_t faceLodSize,
                   void* pixels, void* userdata)
{
    ktx_cbdata* cbData = (ktx_cbdata*)userdata;
    UNUSED(depth);
    UNUSED(faceLodSize);

    glTexImage2D(cbData->glTarget + face, miplevel,
                 cbData->glInternalformat, width,
                 cbData->numLayers == 0 ? (GLuint)height : cbData->numLayers, 0,
                 cbData->glFormat, cbData->glType, pixels);

    if ((cbData->glError = glGetError()) == GL_NO_ERROR) {
        return KTX_SUCCESS;
    } else {
        return KTX_GL_ERROR;
    }
}


KTX_error_code
compressedTexImage2DCallback(int miplevel, int face,
                             int width, int height,
                             int depth,
                             ktx_uint64_t faceLodSize,
                             void* pixels, void* userdata)
{
    ktx_cbdata* cbData = (ktx_cbdata*)userdata;
    GLenum glerror;
    KTX_error_code result;
    UNUSED(depth);

    if (faceLodSize > UINT32_MAX)
        return KTX_INVALID_OPERATION; // Too big for OpenGL {,ES}.

    // It is simpler to just attempt to load the format, rather than divine
    // which formats are supported by the implementation. In the event of an
    // error, software unpacking can be attempted.
    glCompressedTexImage2D(cbData->glTarget + face, miplevel,
                           cbData->glInternalformat, width,
                           cbData->numLayers == 0 ? (GLuint)height : cbData->numLayers,
                           0,
                           (ktx_uint32_t)faceLodSize, pixels);

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

        result = _ktxUnpackETC((GLubyte*)pixels, cbData->glInternalformat,
                                  width, height, &unpacked,
                                  &format, &internalformat,
                                  &type, R16Formats, supportsSRGB);
        if (result != KTX_SUCCESS) {
            return result;
        }
        if (!(sizedFormats & _NON_LEGACY_FORMATS)) {
            if (internalformat == GL_RGB8)
                internalformat = GL_RGB;
            else if (internalformat == GL_RGBA8)
                internalformat = GL_RGBA;
        }
        glTexImage2D(cbData->glTarget + face, miplevel,
                     internalformat, width,
                     cbData->numLayers == 0 ? (GLuint)height : cbData->numLayers, 0,
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

KTX_error_code
texImage3DCallback(int miplevel, int face,
                   int width, int height,
                   int depth,
                   ktx_uint64_t faceLodSize,
                   void* pixels, void* userdata)
{
    ktx_cbdata* cbData = (ktx_cbdata*)userdata;
    UNUSED(faceLodSize);

    assert(gl.glTexImage3D != NULL);
    gl.glTexImage3D(cbData->glTarget + face, miplevel,
                   cbData->glInternalformat,
                   width, height,
                   cbData->numLayers == 0 ? (GLuint)depth : cbData->numLayers,
                   0,
                   cbData->glFormat, cbData->glType, pixels);

    if ((cbData->glError = glGetError()) == GL_NO_ERROR) {
        return KTX_SUCCESS;
    } else {
        return KTX_GL_ERROR;
    }
}

KTX_error_code
compressedTexImage3DCallback(int miplevel, int face,
                             int width, int height,
                             int depth,
                             ktx_uint64_t faceLodSize,
                             void* pixels, void* userdata)
{
    ktx_cbdata* cbData = (ktx_cbdata*)userdata;

    if (faceLodSize > UINT32_MAX)
        return KTX_INVALID_OPERATION; // Too big for OpenGL {,ES}.

    assert(gl.glCompressedTexImage3D != NULL);
    gl.glCompressedTexImage3D(cbData->glTarget + face, miplevel,
                             cbData->glInternalformat,
                             width, height,
                             cbData->numLayers == 0 ? (GLuint)depth : cbData->numLayers,
                             0,
                             (ktx_uint32_t)faceLodSize, pixels);

    if ((cbData->glError = glGetError()) == GL_NO_ERROR) {
        return KTX_SUCCESS;
    } else {
        return KTX_GL_ERROR;
    }
}
/* [imageCallbacks] */

/**
 * @memberof ktxTexture @private
 * @~English
 * @brief Do the common work of creating a GL texture object from a
 *        ktxTexture object.
 *
 * Sets the texture object's GL_TEXTURE_MAX_LEVEL parameter according to the
 * number of levels in the KTX data, provided the context supports this feature.
 *
 * Unpacks compressed GL_ETC1_RGB8_OES and GL_ETC2_* format
 * textures in software when the format is not supported by the GL context,
 * provided the library has been compiled with SUPPORT_SOFTWARE_ETC_UNPACK
 * defined as 1.
 *
 * It will also convert textures with legacy formats to their modern equivalents
 * when the format is not supported by the GL context, provided the library
 * has been compiled with SUPPORT_LEGACY_FORMAT_CONVERSION defined as 1.
 *
 * @param[in] This          handle of the ktxTexture to upload.
 * @param[in] formatInfo    pointer to a ktx_glformatinfo structure providing
 *                          information about the texture format.
 * @param[in,out] pTexture  name of the GL texture object to load. If NULL or if
 *                          <tt>*pTexture == 0</tt> the function will generate
 *                          a texture name. The function binds either the
 *                          generated name or the name given in @p *pTexture
 *                          to the texture target returned in @p *pTarget,
 *                          before loading the texture data. If @p pTexture
 *                          is not NULL and a name was generated, the generated
 *                          name will be returned in *pTexture.
 * @param[out] pTarget      @p *pTarget is set to the texture target used. The
 *                          target is chosen based on the file contents.
 * @param[out] pGlerror     @p *pGlerror is set to the value returned by
 *                          glGetError when this function returns the error
 *                          KTX_GL_ERROR. pGlerror can be NULL.
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p This or @p target is @c NULL or the size of
 *                              a mip level is greater than the size of the
 *                              preceding level.
 * @exception KTX_GL_ERROR      A GL error was raised by glBindTexture,
 *                              glGenTextures or gl*TexImage*. The GL error
 *                              will be returned in @p *glerror, if glerror
 *                              is not @c NULL.
 * @exception KTX_UNSUPPORTED_TEXTURE_TYPE The type of texture is not supported
 *                                         by the current OpenGL context.
 */
/* [loadGLTexture] */
KTX_error_code
ktxTexture_GLUploadPrivate(ktxTexture* This, ktx_glformatinfo* formatInfo,
                           GLuint* pTexture, GLenum* pTarget, GLenum* pGlerror)
{
    GLuint                texname;
    GLenum                target = GL_TEXTURE_2D;
    int                   texnameUser;
    KTX_error_code        result = KTX_SUCCESS;
    ktx_cbdata            cbData;
    PFNKTXITERCB          iterCb = NULL;
    int                   dimensions;

    if (pGlerror)
        *pGlerror = GL_NO_ERROR;

    assert(This && pTarget);

    if (contextProfile == 0)
        discoverContextCapabilities();

    texnameUser = pTexture && *pTexture;
    if (texnameUser) {
        texname = *pTexture;
    } else {
        glGenTextures(1, &texname);
    }

    cbData.glFormat = formatInfo->glFormat;
    cbData.glInternalformat = formatInfo->glInternalformat;
    cbData.glType = formatInfo->glType;

    dimensions = This->numDimensions;
    if (This->isArray) {
        dimensions += 1;
        if (This->numFaces == 6) {
            /* ktxCheckHeader1_ should have caught this. */
            assert(This->numDimensions == 2);
            target = GL_TEXTURE_CUBE_MAP_ARRAY;
        } else {
            switch (This->numDimensions) {
              case 1: target = GL_TEXTURE_1D_ARRAY; break;
              case 2: target = GL_TEXTURE_2D_ARRAY; break;
              /* _ktxCheckHeader should have caught this. */
              default: assert(KTX_TRUE);
            }
        }
        cbData.numLayers = This->numLayers;
    } else {
        if (This->numFaces == 6) {
            /* ktxCheckHeader1_ should have caught this. */
            assert(This->numDimensions == 2);
            target = GL_TEXTURE_CUBE_MAP;
        } else {
            switch (This->numDimensions) {
              case 1: target = GL_TEXTURE_1D; break;
              case 2: target = GL_TEXTURE_2D; break;
              case 3: target = GL_TEXTURE_3D; break;
              /* _ktxCheckHeader shold have caught this. */
              default: assert(KTX_TRUE);
            }
        }
        cbData.numLayers = 0;
    }

    if (target == GL_TEXTURE_1D &&
        ((This->isCompressed && (gl.glCompressedTexImage1D == NULL)) ||
         (!This->isCompressed && (gl.glTexImage1D == NULL))))
    {
        return KTX_UNSUPPORTED_TEXTURE_TYPE;
    }

    /* Reject 3D texture if unsupported. */
    if (target == GL_TEXTURE_3D &&
        ((This->isCompressed && (gl.glCompressedTexImage3D == NULL)) ||
         (!This->isCompressed && (gl.glTexImage3D == NULL))))
    {
        return KTX_UNSUPPORTED_TEXTURE_TYPE;
    }

    /* Reject cube map arrays if not supported. */
    if (target == GL_TEXTURE_CUBE_MAP_ARRAY && !supportsCubeMapArrays) {
        return KTX_UNSUPPORTED_TEXTURE_TYPE;
    }

    /* XXX Need to reject other array textures & cube maps if not supported. */

    switch (dimensions) {
      case 1:
        iterCb = This->isCompressed
                  ? compressedTexImage1DCallback : texImage1DCallback;
        break;
      case 2:
        iterCb = This->isCompressed
                  ? compressedTexImage2DCallback : texImage2DCallback;
            break;
      case 3:
        iterCb = This->isCompressed
                  ? compressedTexImage3DCallback : texImage3DCallback;
        break;
      default:
            assert(KTX_TRUE);
    }

    glBindTexture(target, texname);

    // Prefer glGenerateMipmaps over GL_GENERATE_MIPMAP
    if (This->generateMipmaps && (gl.glGenerateMipmap == NULL)) {
        glTexParameteri(target, GL_GENERATE_MIPMAP, GL_TRUE);
    }

    if (!This->generateMipmaps && supportsMaxLevel)
        glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, This->numLevels - 1);

    if (target == GL_TEXTURE_CUBE_MAP) {
        cbData.glTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
    } else {
        cbData.glTarget = target;
    }

    if (!This->isCompressed) {
#if SUPPORT_LEGACY_FORMAT_CONVERSION
        // If sized legacy formats are supported there is no need to convert.
        // If only unsized formats are supported, there is no point in
        // converting as the modern formats aren't supported either.
        if (sizedFormats == _NON_LEGACY_FORMATS && supportsSwizzle) {
            convertFormat(target, &cbData.glFormat, &cbData.glInternalformat);
        } else if (sizedFormats == _NO_SIZED_FORMATS)
            cbData.glInternalformat = formatInfo->glBaseInternalformat;
#else
        // When no sized formats are supported, or legacy sized formats are not
        // supported, must change internal format.
        if (sizedFormats == _NO_SIZED_FORMATS
            || (!(sizedFormats & _LEGACY_FORMATS) &&
                (formatInfo->glBaseInternalformat == GL_ALPHA
                || formatInfo->glBaseInternalformat == GL_LUMINANCE
                || formatInfo->glBaseInternalformat == GL_LUMINANCE_ALPHA
                || formatInfo->glBaseInternalformat == GL_INTENSITY))) {
            cbData.glInternalformat = formatInfo->glBaseInternalformat;
        }
#endif
    }

    if (ktxTexture_isActiveStream(ktxTexture(This)))
        result = ktxTexture_IterateLoadLevelFaces(This, iterCb, &cbData);
    else
        result = ktxTexture_IterateLevelFaces(This, iterCb, &cbData);

    /* GL errors are the only reason for failure. */
    if (result != KTX_SUCCESS && cbData.glError != GL_NO_ERROR) {
        if (pGlerror)
            *pGlerror = cbData.glError;
    }

    if (result == KTX_SUCCESS)
    {
        // Prefer glGenerateMipmaps over GL_GENERATE_MIPMAP
        if (This->generateMipmaps && gl.glGenerateMipmap) {
            gl.glGenerateMipmap(target);
        }
        *pTarget = target;
        if (pTexture) {
            *pTexture = texname;
        }
    } else if (!texnameUser) {
        glDeleteTextures(1, &texname);
    }
    return result;
}
/* [loadGLTexture] */

/**
 * @memberof ktxTexture1
 * @~English
 * @brief Create a GL texture object from a ktxTexture1 object.
 *
 * Sets the texture object's GL\_TEXTURE\_MAX\_LEVEL parameter according to the
 * number of levels in the KTX data, provided the context supports this feature.
 *
 * Unpacks compressed GL\_ETC1\_RGB8\_OES and GL\_ETC2\_\* format
 * textures in software when the format is not supported by the GL context,
 * provided the library has been compiled with @c SUPPORT_SOFTWARE_ETC_UNPACK
 * defined as 1.
 *
 * It will also convert textures with legacy formats to their modern equivalents
 * when the format is not supported by the GL context, provided the library
 * has been compiled with @c SUPPORT_LEGACY_FORMAT_CONVERSION defined as 1.
 *
 * @param[in] This          handle of the ktxTexture to upload.
 * @param[in,out] pTexture  name of the GL texture object to load. If NULL or if
 *                          <tt>*pTexture == 0</tt> the function will generate
 *                          a texture name. The function binds either the
 *                          generated name or the name given in @p *pTexture
 *                          to the texture target returned in @p *pTarget,
 *                          before loading the texture data. If @p pTexture
 *                          is not NULL and a name was generated, the generated
 *                          name will be returned in *pTexture.
 * @param[out] pTarget      @p *pTarget is set to the texture target used. The
 *                          target is chosen based on the file contents.
 * @param[out] pGlerror     @p *pGlerror is set to the value returned by
 *                          glGetError when this function returns the error
 *                          KTX_GL_ERROR. pGlerror can be NULL.
 *
 * @return  KTX\_SUCCESS on success, other KTX\_\* enum values on error.
 *
 * @exception KTX_GL_ERROR    A GL error was raised by glBindTexture,
 *                            glGenTextures or gl*TexImage*. The GL error
 *                            will be returned in @p *glerror, if glerror
 *                            is not @c NULL.
 * @exception KTX_INVALID_VALUE @p This or @p target is @c NULL or the size of
 *                              a mip level is greater than the size of the
 *                              preceding level.
 * @exception KTX_NOT_FOUND   A dynamically loaded OpenGL {,ES} function
 *                            required by the loader was not found.
 * @exception KTX_UNSUPPORTED_TEXTURE_TYPE The type of texture is not supported
 *                                         by the current OpenGL context.
 */
KTX_error_code
ktxTexture1_GLUpload(ktxTexture1* This, GLuint* pTexture, GLenum* pTarget,
                     GLenum* pGlerror)
{
    GLint                 previousUnpackAlignment;
    KTX_error_code        result = KTX_SUCCESS;
    ktx_glformatinfo      formatInfo;

    if (!This) {
        return KTX_INVALID_VALUE;
    }

    if (!pTarget) {
        return KTX_INVALID_VALUE;
    }

    if (!ktxOpenGLModuleHandle) {
        result = ktxLoadOpenGLLibrary();
        if (result != KTX_SUCCESS) {
            return result;
        }
    }
    /* KTX 1 files require an unpack alignment of 4 */
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);
    if (previousUnpackAlignment != KTX_GL_UNPACK_ALIGNMENT) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, KTX_GL_UNPACK_ALIGNMENT);
    }

    formatInfo.glFormat = This->glFormat;
    formatInfo.glInternalformat = This->glInternalformat;
    formatInfo.glBaseInternalformat = This->glBaseInternalformat;
    formatInfo.glType = This->glType;
 
    result = ktxTexture_GLUploadPrivate(ktxTexture(This), &formatInfo,
                                        pTexture, pTarget, pGlerror);

    /* restore previous GL state */
    if (previousUnpackAlignment != KTX_GL_UNPACK_ALIGNMENT) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);
    }

    return result;
}

/**
 * @memberof ktxTexture2
 * @~English
 * @brief Create a GL texture object from a ktxTexture2 object.
 *
 * Sets the texture object's GL\_TEXTURE\_MAX\_LEVEL parameter according to the
 * number of levels in the KTX data, provided the context supports this feature.
 *
 * Unpacks compressed GL\_ETC1\_RGB8\_OES and GL\_ETC2\_\* format
 * textures in software when the format is not supported by the GL context,
 * provided the library has been compiled with @c SUPPORT_SOFTWARE_ETC_UNPACK
 * defined as 1.
 *
 * @param[in] This          handle of the ktxTexture to upload.
 * @param[in,out] pTexture  name of the GL texture object to load. If NULL or if
 *                          <tt>*pTexture == 0</tt> the function will generate
 *                          a texture name. The function binds either the
 *                          generated name or the name given in @p *pTexture
 *                          to the texture target returned in @p *pTarget,
 *                          before loading the texture data. If @p pTexture
 *                          is not NULL and a name was generated, the generated
 *                          name will be returned in *pTexture.
 * @param[out] pTarget      @p *pTarget is set to the texture target used. The
 *                          target is chosen based on the file contents.
 * @param[out] pGlerror     @p *pGlerror is set to the value returned by
 *                          glGetError when this function returns the error
 *                          KTX_GL_ERROR. pGlerror can be NULL.
 *
 * @return  KTX\_SUCCESS on success, other KTX\_\* enum values on error.
 *
 * @exception KTX_GL_ERROR    A GL error was raised by glBindTexture,
 *                            glGenTextures or gl*TexImage*. The GL error
 *                            will be returned in @p *glerror, if glerror
 *                            is not @c NULL.
 * @exception KTX_INVALID_VALUE @p This or @p target is @c NULL or the size of
 *                              a mip level is greater than the size of the
 *                              preceding level.
 * @exception KTX_NOT_FOUND   A dynamically loaded OpenGL {,ES} function
 *                            required by the loader was not found.
 * @exception KTX_UNSUPPORTED_TEXTURE_TYPE The type of texture is not supported
 *                                         by the current OpenGL context.
 */
KTX_error_code
ktxTexture2_GLUpload(ktxTexture2* This, GLuint* pTexture, GLenum* pTarget,
                     GLenum* pGlerror)
{
    GLint                 previousUnpackAlignment;
    ktx_error_code_e      result = KTX_SUCCESS;
    ktx_glformatinfo      formatInfo;

    if (!This) {
        return KTX_INVALID_VALUE;
    }

    if (!pTarget) {
        return KTX_INVALID_VALUE;
    }

    if (!ktxOpenGLModuleHandle) {
        result = ktxLoadOpenGLLibrary();
        if (result != KTX_SUCCESS) {
            return result;
        }
    }

    if (This->vkFormat != VK_FORMAT_UNDEFINED) {
        formatInfo.glInternalformat =
                            vkFormat2glInternalFormat(This->vkFormat);
        if (formatInfo.glInternalformat == GL_INVALID_VALUE) {
            // TODO Check for mapping metadata. If none
            return KTX_INVALID_OPERATION;
        }
    } else {
       // TODO: Check DFD for ASTC HDR or 3D or RGB[DEM] and figure out format.
       return KTX_INVALID_OPERATION; // BasisU textures must be transcoded
                                     // before upload.
    }

    if (This->isCompressed) {
        /* Unused. */
        formatInfo.glFormat = GL_INVALID_VALUE;
        formatInfo.glType = GL_INVALID_VALUE;
        formatInfo.glBaseInternalformat = GL_INVALID_VALUE;
    } else {
        formatInfo.glFormat = vkFormat2glFormat(This->vkFormat);
        formatInfo.glType = vkFormat2glType(This->vkFormat);
        formatInfo.glBaseInternalformat = formatInfo.glInternalformat;

        if (formatInfo.glFormat == GL_INVALID_VALUE || formatInfo.glType == GL_INVALID_VALUE)
            return KTX_INVALID_OPERATION;
    }
    /* KTX 2 files require an unpack alignment of 1. OGL default is 4. */
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);
    if (previousUnpackAlignment != 1) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }

    result = ktxTexture_GLUploadPrivate(ktxTexture(This), &formatInfo,
                                        pTexture, pTarget, pGlerror);

    /* restore previous GL state */
    if (previousUnpackAlignment != 1) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);
    }

    return result;
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Create a GL texture object from a ktxTexture1 object.
 *
 * In order to ensure that the GL uploader is not linked into an application unless explicitly called,
 * this is not a virtual function. It determines the texture type then dispatches to the correct function.
 *
 * @copydetails ktxTexture1::ktxTexture1_GLUpload
 */
KTX_error_code
ktxTexture_GLUpload(ktxTexture* This, GLuint* pTexture, GLenum* pTarget,
                    GLenum* pGlerror)
{
    if (This->classId == ktxTexture2_c)
        return ktxTexture2_GLUpload((ktxTexture2*)This, pTexture, pTarget,
                                     pGlerror);
    else
        return ktxTexture1_GLUpload((ktxTexture1*)This, pTexture, pTarget,
                                     pGlerror);
}

/** @} */

/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

#ifndef KTX_H_A55A6F00956F42F3A137C11929827FE1
#define KTX_H_A55A6F00956F42F3A137C11929827FE1

/**
 * @file
 * @~English
 *
 * @brief Declares the public functions and structures of the
 *        KTX API.
 *
 * @author Georg Kolling, Imagination Technology
 * @author with modifications by Mark Callow, HI Corporation
 *
 * $Id$
 * $Date$
 *
 * @todo Find a way so that applications do not have to define KTX_OPENGL{,_ES*}
 *       when using the library.
 */

/*
 * This file copyright (c) 2010 The Khronos Group, Inc.
 */

/*
@~English

LibKTX contains code

@li (c) 2010 The Khronos Group Inc.
@li (c) 2008 and (c) 2010 HI Corporation
@li (c) 2005 Ericsson AB
@li (c) 2003-2010, Troy D. Hanson
@li (c) 2015 Mark Callow

The KTX load tests contain code

@li (c) 2013 The Khronos Group Inc.
@li (c) 2008 and (c) 2010 HI Corporation
@li (c) 1997-2014 Sam Lantinga
@li (c) 2015 Mark Callow

@section default Default License

With the exception of the files listed explicitly below, the source
files are made available under the following BSD-like license. Most
files contain this license explicitly. Some files refer to LICENSE.md
which contains this same text. Such files are licensed under this
Default License.

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

@section hi_mark hi_mark{,_sq}.ktx

The HI logo textures are &copy; & &trade; HI Corporation and are
provided for use only in testing the KTX loader. Any other use requires
specific prior written permission from HI. Furthermore the name HI may
not be used to endorse or promote products derived from this software
without specific prior written permission.

@section uthash uthash.h

uthash.h is made available under the following revised BSD license.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

@section SDL2 include/SDL2/

These files are part of the SDL2 source distributed by the [SDL project]
(http://libsdl.org) under the terms of the [zlib license]
(http://www.zlib.net/zlib_license.html).
*/

/**
 * @~English
 * @mainpage The KTX Library
 *
 * @section intro_sec Introduction
 *
 * libktx is a small library of functions for creating KTX (Khronos
 * TeXture) files and instantiating OpenGL&reg; and OpenGL&reg; ES
 * textures from them.
 *
 * For information about the KTX format see the
 * <a href="http://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/">
 * formal specification.</a>
 *
 * The library is open source software. Most of the code is licensed under a
 * modified BSD license. The code for unpacking ETC1, ETC2 and EAC compressed
 * textures has a separate license that restricts it to uses associated with
 * Khronos Group APIs. See @ref license for more details.
 *
 * See @ref history for the list of changes.
 *
 * @author Mark Callow, <a href="http://www.hicorp.co.jp">HI Corporation</a>
 * @author Georg Kolling, <a href="http://www.imgtec.com">Imagination Technology</a>
 * @author Jacob Str&ouml;m, <a href="http://www.ericsson.com">Ericsson AB</a>
 *
 * @version 2.0.X
 *
 * $Date$
 */

#include <stdio.h>

/* To avoid including <KHR/khrplatform.h> define our own types. */
typedef unsigned char ktx_uint8_t;
#ifdef _MSC_VER
typedef unsigned short ktx_uint16_t;
typedef   signed short ktx_int16_t;
typedef unsigned int   ktx_uint32_t;
typedef   signed int   ktx_int32_t;
#else
#include <stdint.h>
typedef uint16_t ktx_uint16_t;
typedef int16_t  ktx_int16_t;
typedef uint32_t ktx_uint32_t;
typedef int32_t  ktx_int32_t;
#endif

/* This will cause compilation to fail if uint32 != 4. */
typedef unsigned char ktx_uint32_t_SIZE_ASSERT[sizeof(ktx_uint32_t) == 4];

/*
 * To avoid including gl.h ...
 * Compilers don't warn of duplicate typedefs if there is no conflict.
 */
typedef unsigned char GLboolean;
typedef unsigned int GLenum;
typedef int GLint;
typedef int GLsizei;
typedef unsigned int GLuint;
typedef unsigned char GLubyte;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Key String for standard orientation value.
 */
#define KTX_ORIENTATION_KEY	"KTXorientation"
/**
 * @brief Standard format for 2D orientation value.
 */
#define KTX_ORIENTATION2_FMT "S=%c,T=%c"
/**
 * @brief Standard format for 3D orientation value.
 */
#define KTX_ORIENTATION3_FMT "S=%c,T=%c,R=%c"
/**
 * @brief Required unpack alignment
 */
#define KTX_GL_UNPACK_ALIGNMENT 4
    
#define KTX_TRUE  1
#define KTX_FALSE 0

/**
 * @brief Error codes returned by library functions.
 */
typedef enum KTX_error_code_t {
	KTX_SUCCESS = 0,		 /*!< Operation was successful. */
	KTX_FILE_OPEN_FAILED,	 /*!< The target file could not be opened. */
	KTX_FILE_WRITE_ERROR,    /*!< An error occurred while writing to the file. */
    KTX_FILE_DATA_ERROR,     /*!< The data in the file is inconsistent with the spec. */
	KTX_GL_ERROR,            /*!< GL operations resulted in an error. */
	KTX_INVALID_OPERATION,   /*!< The operation is not allowed in the current state. */
	KTX_INVALID_VALUE,	     /*!< A parameter value was not valid */
	KTX_NOT_FOUND,			 /*!< Requested key was not found */
	KTX_OUT_OF_MEMORY,       /*!< Not enough memory to complete the operation. */
	KTX_UNEXPECTED_END_OF_FILE, /*!< The file did not contain enough data */
	KTX_UNKNOWN_FILE_FORMAT, /*!< The file not a KTX file */
	KTX_UNSUPPORTED_TEXTURE_TYPE, /*!< The KTX file specifies an unsupported texture type. */
} KTX_error_code;

#define KTX_IDENTIFIER_REF  { 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A }
#define KTX_ENDIAN_REF      (0x04030201)
#define KTX_ENDIAN_REF_REV  (0x01020304)
#define KTX_HEADER_SIZE		(64)
    
/**
 * @brief KTX file header
 *
 * See the KTX specification for descriptions
 */
typedef struct KTX_header {
    ktx_uint8_t  identifier[12];
    ktx_uint32_t endianness;
    ktx_uint32_t glType;
    ktx_uint32_t glTypeSize;
    ktx_uint32_t glFormat;
    ktx_uint32_t glInternalFormat;
    ktx_uint32_t glBaseInternalFormat;
    ktx_uint32_t pixelWidth;
    ktx_uint32_t pixelHeight;
    ktx_uint32_t pixelDepth;
    ktx_uint32_t numberOfArrayElements;
    ktx_uint32_t numberOfFaces;
    ktx_uint32_t numberOfMipmapLevels;
    ktx_uint32_t bytesOfKeyValueData;
} KTX_header;

/* This will cause compilation to fail if the struct size doesn't match */
typedef int KTX_header_SIZE_ASSERT [sizeof(KTX_header) == KTX_HEADER_SIZE];


/**
 * @brief Structure for supplemental information about the texture.
 *
 * ktxReadHeader returns supplemental information about the texture that
 * is derived during checking of the file header.
 */
typedef struct KTX_supplemental_info
{
    ktx_uint8_t compressed;
    ktx_uint8_t generateMipmaps;
    ktx_uint16_t textureDimension;
} KTX_supplemental_info;
/**
 * @var KTX_supplemental_info::compressed
 * @brief KTX_TRUE, if this a compressed texture, KTX_FALSE otherwise?
 */
/**
 * @var KTX_supplemental_info::generateMipmaps
 * @brief KTX_TRUE, if mipmap generation is required, KTX_FALSE otherwise.
 */
/**
 * @var KTX_supplemental_info::textureDimension
 * @brief The number of dimensions, 1, 2 or 3, of data in the texture image.
 */

    
/**
 * @brief Structure holding information about the texture.
 *
 * Used to pass texture information to ktxWriteKTX. Retained for backward
 * compatibility.
 */
typedef struct KTX_texture_info
{
	ktx_uint32_t glType;
	ktx_uint32_t glTypeSize;
	ktx_uint32_t glFormat;
	ktx_uint32_t glInternalFormat;
	ktx_uint32_t glBaseInternalFormat;
	ktx_uint32_t pixelWidth;
 	ktx_uint32_t pixelHeight;
	ktx_uint32_t pixelDepth;
	ktx_uint32_t numberOfArrayElements;
	ktx_uint32_t numberOfFaces;
	ktx_uint32_t numberOfMipmapLevels;
} KTX_texture_info;

/**
 * @var KTX_texture_info::glType
 * @brief The type of the image data.
 *
 * Values are the same as in the @p type parameter of
 * glTexImage*D. Must be 0 for compressed images.
 */
/**
 * @var KTX_texture_info::glTypeSize;
 * @brief The data type size to be used in case of endianness
 *        conversion.
 *
 * This value is used in the event conversion is required when the
 * KTX file is loaded. It should be the size in bytes corresponding
 * to glType. Must be 1 for compressed images.
 */
/**
 * @var KTX_texture_info::glFormat;
 * @brief The format of the image(s).
 *
 * Values are the same as in the format parameter
 * of glTexImage*D. Must be 0 for compressed images.
 */
/**
 * @var KTX_texture_info::glInternalFormat;
 * @brief The internalformat of the image(s).
 *
 * Values are the same as for the internalformat parameter of
 * glTexImage*2D. Note: it will not be used when a KTX file
 * containing an uncompressed texture is loaded into OpenGL ES.
 */
/**
 * @var KTX_texture_info::glBaseInternalFormat;
 * @brief The base internalformat of the image(s)
 *
 * For non-compressed textures, should be the same as glFormat.
 * For compressed textures specifies the base internal, e.g.
 * GL_RGB, GL_RGBA.
 */
/**
 * @var KTX_texture_info::pixelWidth;
 * @brief Width of the image for texture level 0, in pixels.
 */
/**
 * @var KTX_texture_info::pixelHeight;
 * @brief Height of the texture image for level 0, in pixels.
 *
 * Must be 0 for 1D textures.
 */
/**
 * @var KTX_texture_info::pixelDepth;
 * @brief Depth of the texture image for level 0, in pixels.
 *
 * Must be 0 for 1D, 2D and cube textures.
 */
/**
 * @var KTX_texture_info::numberOfArrayElements;
 * @brief The number of array elements.
 *
 * Must be 0 if not an array texture.
 */
/**
 * @var KTX_texture_info::numberOfFaces;
 * @brief The number of cubemap faces.
 *
 * Must be 6 for cubemaps and cubemap arrays, 1 otherwise. Cubemap
 * faces must be provided in the order: +X, -X, +Y, -Y, +Z, -Z.
 */
/**
 * @var KTX_texture_info::numberOfMipmapLevels;
 * @brief The number of mipmap levels.
 *
 * 1 for non-mipmapped texture. 0 indicates that a full mipmap pyramid should
 * be generated from level 0 at load time (this is usually not allowed for
 * compressed formats). Mipmaps must be provided in order from largest size to
 * smallest size. The first mipmap level is always level 0.
 */


/**
 * @brief Structure used to pass image data to ktxWriteKTX.
 */
typedef struct KTX_image_info {
	GLsizei size;	/*!< Size of the image data in bytes. */
	GLubyte* data;  /*!< Pointer to the image data. */
} KTX_image_info;

/**
 * @brief Structure used by load functions to return texture dimensions
 */
typedef struct KTX_dimensions {
	GLsizei width;  /*!< width in texels */
	GLsizei height; /*!< height in texels */
	GLsizei depth;  /*!< depth in texels */
} KTX_dimensions;
    
/**
 * @brief Opaque handle to a KTX_hash_table.
 */
typedef void* KTX_hash_table;

/**
 * @brief Opaque handle to a KTX_context.
 */
typedef void* KTX_context;
    
#define KTXAPIENTRY
#define KTXAPIENTRYP KTXAPIENTRY *
/**
 * @brief Signature of function called by ktxReadImages to receive image data.
 *
 * The function parameters give the values which change for each image.
 *
 * @tparam [in] miplevel        MIP level from 0 to the max level which is
 *                              dependent on the texture size.
 * @tparam [in] face            usually 0; for cube maps and cube map arrays,
 *                              one of the 6 cube faces in the order
 *                              +X, -X, +Y, -Y, +Z, -Z.
 * @tparam [in] width           width of the image.
 * @tparam [in] height          height of the image or, for 1D textures
 *                              textures, 1.
 * @tparam [in] depth           depth of the image or, for 1D & 2D
 *                              textures, 1.
 * @tparam [in] layers          number of array layers in the texture.
 *                              For non array textures, 1.
 * @tparam [in] faceLodSize     number of bytes of data pointed at by
 *                              @p pixels.
 * @tparam [in] pixels          pointer to the image data.
 * @tparam [in,out] userdata    pointer for the application to pass data to and
 *                              from the callback function.
 */
typedef KTX_error_code (KTXAPIENTRYP PFNKTXIMAGECB)(int miplevel, int face,
                                               int width, int height, int depth,
                                               int layers,
                                               ktx_uint32_t faceLodSize,
                                               void* pixels, void* userdata);

/*
 * See the implementation files for the full documentation of the following
 * functions.
 */

/*
 * Open a KTX file from a stdio FILE and return a KTX_context object.
 */
KTX_error_code
ktxOpenKTXF(FILE* file, KTX_context* pContext);
    
/*
 * Open the KTX file with the given file name and return a KTX_context object.
 */
KTX_error_code
ktxOpenKTXN(const char* const filename, KTX_context* pContext);
    
/*
 * Open a KTX file that is in memory and return a KTX_context object.
 */
KTX_error_code
ktxOpenKTXM(const void* bytes, size_t size, KTX_context* pContext);
    
/*
 * Close a KTX file, freeing the context object.
 */
KTX_error_code
ktxCloseKTX(KTX_context ctx);

/*
 * Read the header of the KTX file identified by @p ctx.
 */
KTX_error_code
ktxReadHeader(KTX_context ctx, KTX_header* pHeader,
              KTX_supplemental_info* pSuppInfo);

/*
 * Read the key-value data from the KTX file identified by @p ctx.
 */
KTX_error_code
ktxReadKVData(KTX_context ctx, ktx_uint32_t* pKvdLen, ktx_uint8_t ** ppKvd);

/*
 * Read the images from the KTX file identified by @p ctx. @p imageCb
 * will be called with the data for each image.
 */
KTX_error_code
ktxReadImages(KTX_context ctx, PFNKTXIMAGECB imageCb, void* userdata);
    
/*
 * Return the number of bytes needed to store all of the data in the
 * KTX file.
 */
size_t
ktxReader_getDataSize(KTX_context ctx);

/*
 * Loads a texture from a the KTX file identified by @p ctx.
 */
KTX_error_code
ktxLoadTexture(KTX_context ctx, GLuint* pTexture, GLenum* pTarget,
               KTX_dimensions* pDimensions, GLboolean* pIsMipmapped,
               GLenum* pGlerror,
               unsigned int* pKvdLen, unsigned char** ppKvd);

/*
 * Loads a texture from a stdio FILE.
 */
KTX_error_code
ktxLoadTextureF(FILE*, GLuint* pTexture, GLenum* pTarget,
				KTX_dimensions* pDimensions, GLboolean* pIsMipmapped,
				GLenum* pGlerror,
				unsigned int* pKvdLen, unsigned char** ppKvd);

/*
 * Loads a texture from a KTX file on disk.
 */
KTX_error_code
ktxLoadTextureN(const char* const filename, GLuint* pTexture, GLenum* pTarget,
				KTX_dimensions* pDimensions, GLboolean* pIsMipmapped,
				GLenum* pGlerror,
				unsigned int* pKvdLen, unsigned char** ppKvd);

/*
 * Loads a texture from a KTX file in memory.
 */
KTX_error_code
ktxLoadTextureM(const void* bytes, GLsizei size, GLuint* pTexture,
                GLenum* pTarget, KTX_dimensions* pDimensions,
                GLboolean* pIsMipmapped, GLenum* pGlerror,
				unsigned int* pKvdLen, unsigned char** ppKvd);

/*
 * Writes a KTX file using supplied data.
 */
KTX_error_code
ktxWriteKTXF(FILE*, const KTX_texture_info* imageInfo,
			 GLsizei bytesOfKeyValueData, const void* keyValueData,
			 GLuint numImages, KTX_image_info images[]);

/*
 * Writes a KTX file using supplied data.
 */
KTX_error_code
ktxWriteKTXN(const char* dstname, const KTX_texture_info* imageInfo,
			 GLsizei bytesOfKeyValueData, const void* keyValueData,
			 GLuint numImages, KTX_image_info images[]);

/*
 * Writes a KTX file into memory using supplied data.
 */
KTX_error_code
ktxWriteKTXM(unsigned char** bytes, GLsizei* size,
             const KTX_texture_info* textureInfo, GLsizei bytesOfKeyValueData,
             const void* keyValueData, GLuint numImages,
             KTX_image_info images[]);

/*
 * Returns a string corresponding to a KTX error code.
 */
const char* const ktxErrorString(KTX_error_code error);

/*
 * Creates a key-value hash table
 */
KTX_hash_table ktxHashTable_Create();

/*
 * Destroys a key-value hash table
 */
void ktxHashTable_Destroy(KTX_hash_table This);

/*
 * Adds a key-value pair to a hash table.
 */
KTX_error_code
ktxHashTable_AddKVPair(KTX_hash_table This, const char* key,
					   unsigned int valueLen, const void* value);


/*
 * Looks up a key and returns the value.
 */
KTX_error_code
ktxHashTable_FindValue(KTX_hash_table This, const char* key,
					   unsigned int* pValueLen, void** pValue);


/*
 * Serializes the hash table to a block of memory suitable for
 * writing to a KTX file.
 */
KTX_error_code
ktxHashTable_Serialize(KTX_hash_table This,
                       unsigned int* kvdLen, unsigned char** kvd);


/*
 * Creates a hash table from the serialized data read from a
 * a KTX file.
 */
KTX_error_code
ktxHashTable_Deserialize(unsigned int kvdLen, void* kvd, KTX_hash_table* pKvt);

#ifdef __cplusplus
}
#endif

/**
@page history KTX Library Revision History

@section v6 Version 3.0.0
Added:
@li new API for reading KTX files without an OpenGL context.

Changed:
@li ktx.h to not depend on KHR/khrplatform.h and GL{,ES*}/gl{corearb,}.h.
    Applications using OpenGL must now include these files themselves.
@li ktxLoadTexture[FMN], removing the hack of loading 1D textures as 2D textures
    when the OpenGL context does not support 1D textures.
    KTX_UNSUPPORTED_TEXTURE_TYPE is now returned.

@section v5 Version 2.0.X
Changed:
@li New build system

@section v4 Version 2.0.1
Added:
@li CMake build files. Thanks to Pavel Rotjberg for the initial version.

Changed:
@li ktxWriteKTXF to check the validity of the type & format combinations
    passed to it.

Fixed:
@li Public Bugzilla <a href="http://www.khronos.org/bugzilla/show_bug.cgi?id=999">999</a>: 16-bit luminance texture cannot be written.
@li compile warnings from compilers stricter than MS Visual C++. Thanks to
    Pavel Rotjberg.

@section v3 Version 2.0
Added:
@li support for decoding ETC2 and EAC formats in the absence of a hardware
    decoder.
@li support for converting textures with legacy LUMINANCE, LUMINANCE_ALPHA,
    etc. formats to the equivalent R, RG, etc. format with an
	appropriate swizzle, when loading in OpenGL Core Profile contexts.
@li ktxErrorString function to return a string corresponding to an error code.
@li	tests for ktxLoadTexture[FN] that run under OpenGL ES 3.0 and OpenGL 3.3.
    The latter includes an EGL on WGL wrapper that makes porting apps between
	OpenGL ES and OpenGL easier on Windows.
@li more texture formats to ktxLoadTexture[FN] and toktx tests.

Changed:
@li ktxLoadTexture[FMN] to discover the capabilities of the GL context at
    run time and load textures, or not, according to those capabilities.

Fixed:
@li failure of ktxWriteKTXF to pad image rows to 4 bytes as required by the KTX
    format.
@li ktxWriteKTXF exiting with KTX_FILE_WRITE_ERROR when attempting to write
    more than 1 byte of face-LOD padding.

Although there is only a very minor API change, the addition of ktxErrorString,
the functional changes are large enough to justify bumping the major revision
number.

@section v2 Version 1.0.1
Implemented ktxLoadTextureM.
Fixed the following:
@li Public Bugzilla <a href="http://www.khronos.org/bugzilla/show_bug.cgi?id=571">571</a>: crash when null passed for pIsMipmapped.
@li Public Bugzilla <a href="http://www.khronos.org/bugzilla/show_bug.cgi?id=572">572</a>: memory leak when unpacking ETC textures.
@li Public Bugzilla <a href="http://www.khronos.org/bugzilla/show_bug.cgi?id=573">573</a>: potential crash when unpacking ETC textures with unused padding pixels.
@li Public Bugzilla <a href="http://www.khronos.org/bugzilla/show_bug.cgi?id=576">576</a>: various small fixes.

Thanks to Krystian Bigaj for the ktxLoadTextureM implementation and these fixes.

@section v1 Version 1.0
Initial release.

*/

#endif /* KTX_H_A55A6F00956F42F3A137C11929827FE1 */

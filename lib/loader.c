/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/**
 * @file
 * @~English
 *
 * @brief Functions for instantiating GL or GLES textures from KTX files.
 *
 * @author Georg Kolling, Imagination Technology
 * @author Mark Callow, HI Corporation
 *
 * $Revision$
 * $Date::                            $
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

#include <string.h>
#include <stdlib.h>

#include "KHR/khrplatform.h"
#include "ktx.h"
#include "ktxint.h"

#include KTX_GLFUNCPTRS


/**
 * @~English
 * @brief Load a GL texture object from a stdio FILE stream.
 *
 * This function will unpack GL_ETC1_RGB8_OES format compressed textures in
 * software when the format is not supported by the GL implementation,
 * provided the library has been compiled with SUPPORT_SOFTWARE_ETC_UNPACK
 * defined as 1.
 *
 * @param [in] file			pointer to the stdio FILE stream from which to
 * 							load.
 * @param [in,out] pTexture	name of the GL texture to load. If NULL or if
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
	GLint				previousUnpackAlignment;
	KTX_header			header;
	KTX_texinfo			texinfo;
	void*				data = NULL;
	khronos_uint32_t	dataSize = 0;
	GLuint				texname;
	khronos_uint32_t    faceLodSize;
	khronos_uint32_t    faceLodSizeRounded;
	khronos_uint32_t	level;
	khronos_uint32_t	face;
	GLenum				glInternalFormat;
	KTX_error_code		errorCode = KTX_SUCCESS;
	GLenum				errorTmp;


	if (pGlerror)
		*pGlerror = GL_NO_ERROR;

	if (!file) {
		return KTX_INVALID_VALUE;
	}

	if (!pTarget) {
		return KTX_INVALID_VALUE;
	}

	if (fread(&header, KTX_HEADER_SIZE, 1, file) != 1) {
		return KTX_UNEXPECTED_END_OF_FILE;
	}

	errorCode = _ktxCheckHeader(&header, &texinfo);
	if (errorCode != KTX_SUCCESS) {
		return errorCode;
	}

	if (ppKvd) {
		if (pKvdLen == NULL)
			return KTX_INVALID_OPERATION;
		*pKvdLen = header.bytesOfKeyValueData;
		if (*pKvdLen) {
			*ppKvd = malloc(*pKvdLen);
			if (*ppKvd == NULL)
				return KTX_OUT_OF_MEMORY;
		    if (fread(*ppKvd, *pKvdLen, 1, file) != 1)
				return KTX_UNEXPECTED_END_OF_FILE;
		}
	} else {
		/* skip key/value metadata */
		if (fseek(file, (long)header.bytesOfKeyValueData, SEEK_CUR) != 0) {
			return KTX_UNEXPECTED_END_OF_FILE;
		}
	}


	/* KTX files require an unpack alignment of 4 */
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	if (pTexture && *pTexture) {
		texname = *pTexture;
	} else {
		glGenTextures(1, &texname);
	}
	glBindTexture(texinfo.glTarget, texname);

	if (texinfo.generateMipmaps && (glGenerateMipmap == NULL)) {
		glTexParameteri(texinfo.glTarget, GL_GENERATE_MIPMAP, GL_TRUE);
	}

	if (texinfo.glTarget == GL_TEXTURE_CUBE_MAP) {
		texinfo.glTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
	}

#if KTX_SUPPORT_SIZEDINTERNALFORMATS
	glInternalFormat = header.glInternalFormat;
#else
	if (texinfo.compressed)
		glInternalFormat = header.glInternalFormat;
	else
		glInternalFormat = header.glBaseInternalFormat;
#endif

	for (level = 0; level < header.numberOfMipmapLevels; ++level)
	{
		GLsizei pixelWidth  = MAX(1, header.pixelWidth  >> level);
		GLsizei pixelHeight = MAX(1, header.pixelHeight >> level);
		GLsizei pixelDepth  = MAX(1, header.pixelDepth  >> level);

		if (fread(&faceLodSize, sizeof(khronos_uint32_t), 1, file) != 1) {
			errorCode = KTX_UNEXPECTED_END_OF_FILE;
			goto cleanup;
		}
		if (header.endianness == KTX_ENDIAN_REF_REV) {
			_ktxSwapEndian32(&faceLodSize, 1);
		}
		faceLodSizeRounded = (faceLodSize + 3) & ~(khronos_uint32_t)3;
		if (!data) {
			/* allocate memory sufficient for the first level */
			data = malloc(faceLodSizeRounded);
			if (!data) {
				errorCode = KTX_OUT_OF_MEMORY;
				goto cleanup;
			}
			dataSize = faceLodSizeRounded;
		}
		else if (dataSize < faceLodSizeRounded) {
			/* subsequent levels cannot be larger than the first level */
			errorCode = KTX_INVALID_VALUE;
			goto cleanup;
		}

		for (face = 0; face < header.numberOfFaces; ++face)
		{
			if (fread(data, faceLodSizeRounded, 1, file) != 1) {
				errorCode = KTX_UNEXPECTED_END_OF_FILE;
				goto cleanup;
			}

			/* Perform endianness conversion on texture data */
			if (header.endianness == KTX_ENDIAN_REF_REV && header.glTypeSize == 2) {
				_ktxSwapEndian16((khronos_uint16_t*)data, faceLodSize / 2);
			}
			else if (header.endianness == KTX_ENDIAN_REF_REV && header.glTypeSize == 4) {
				_ktxSwapEndian32((khronos_uint32_t*)data, faceLodSize / 4);
			}

			if (texinfo.textureDimensions == 1) {
				if (texinfo.compressed) {
					glCompressedTexImage1D(texinfo.glTarget + face, level, 
						glInternalFormat, pixelWidth, 0,
						faceLodSize, data);
				} else {
					glTexImage1D(texinfo.glTarget + face, level, 
						glInternalFormat, pixelWidth, 0, 
						header.glFormat, header.glType, data);
				}
			} else if (texinfo.textureDimensions == 2) {
				if (header.numberOfArrayElements) {
					pixelHeight = header.numberOfArrayElements;
				}
				if (texinfo.compressed) {
#if SUPPORT_SOFTWARE_ETC_UNPACK
					if (glInternalFormat == GL_ETC1_RGB8_OES
						&& !GLEW_OES_compressed_ETC1_RGB8_texture) {
						GLubyte* unpacked;

						_ktxUnpackETC(data, &unpacked, pixelWidth, pixelHeight);
						glTexImage2D(texinfo.glTarget + face, level, 
									 GL_RGB, pixelWidth, pixelHeight, 0, 
									 GL_RGB, GL_UNSIGNED_BYTE, unpacked);
					} else
#endif
						glCompressedTexImage2D(texinfo.glTarget + face, level, 
							glInternalFormat, pixelWidth, pixelHeight, 0,
							faceLodSize, data);
				} else {
					glTexImage2D(texinfo.glTarget + face, level, 
						glInternalFormat, pixelWidth, pixelHeight, 0, 
						header.glFormat, header.glType, data);
				}
			} else if (texinfo.textureDimensions == 3) {
				if (header.numberOfArrayElements) {
					pixelDepth = header.numberOfArrayElements;
				}
				if (texinfo.compressed) {
					glCompressedTexImage3D(texinfo.glTarget + face, level, 
						glInternalFormat, pixelWidth, pixelHeight, pixelDepth, 0,
						faceLodSize, data);
				} else {
					glTexImage3D(texinfo.glTarget + face, level, 
						glInternalFormat, pixelWidth, pixelHeight, pixelDepth, 0, 
						header.glFormat, header.glType, data);
				}
			}

			errorTmp = glGetError();
			if (errorTmp != GL_NO_ERROR) {
				if (pGlerror)
					*pGlerror = errorTmp;
				errorCode = KTX_GL_ERROR;
				goto cleanup;
			}
		}
	}

cleanup:
	free(data);

	/* restore previous GL state */
	glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);

	if (errorCode == KTX_SUCCESS)
	{
		if (texinfo.generateMipmaps && glGenerateMipmap) {
			glGenerateMipmap(texinfo.glTarget);
		}
		*pTarget = texinfo.glTarget;
		if (pTexture) {
			*pTexture = texname;
		}
		if (pDimensions) {
			pDimensions->width = header.pixelWidth;
			pDimensions->height = header.pixelHeight;
			pDimensions->depth = header.pixelDepth;
		}
		if (*pIsMipmapped) {
			if (texinfo.generateMipmaps || header.numberOfMipmapLevels > 1)
				*pIsMipmapped = GL_TRUE;
			else
				*pIsMipmapped = GL_FALSE;
		}
	}
	return errorCode;
}


/**
 * @~English
 * @brief Load a GL texture object from a named file on disk.
 *
 * @param [in] filename		pointer to a C string that contains the path of
 * 							the file to load.
 * @param [in,out] pTexture	name of the GL texture to load. See
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
 * @param [in,out] pTexture	name of the GL texture to load. See
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
ktxLoadTextureM(const char* bytes, GLsizei size, GLuint* pTexture, GLenum* pTarget,
				KTX_dimensions* pDimensions, GLboolean* pIsMipmapped,
				GLenum* pGlerror,
				unsigned int* pKvdLen, unsigned char** ppKvd)
{
	/** @todo Implement ktxLoadTextureM.
	
	On GNU/Linux it is as simple as adding

	@verbatim
	#define _GNU_SOURCE
	#include <stdio.h>

	somewhere outside this function, then

	KTX_error_code errorCode;
	FILE* file = fmemopen(bytes, size, "rb");

	if (file) {
		errorCode = ktxLoadTextureF(file, pTexture, pDimensions, pTarget,
									pIsMipmapped, pGlerror, pKvdLen, ppKvd);
		fclose(file);
	} else
	    errorCode = KTX_FILE_OPEN_FAILED;

	return errorCode;
	@endverbatim

	Unfortunately it is not as simple on Windows. One possibility is
	poking around in the not exactly public FILE structure to 
	implement the equivalent of fmemopen.
	*/
	return KTX_FILE_OPEN_FAILED;
}



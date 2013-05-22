/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/**
 * @file writer.c
 * @~English
 *
 * @brief Functions for creating KTX-format files from a set of images.
 *
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

#include <stdio.h>
#include <assert.h>

#include "ktx.h"
#include "ktxint.h"

static GLint sizeofGLtype(GLenum type);
static GLint groupSize(GLenum format, GLenum type, GLint* elementBytes);


/**
 * @~English
 * @brief Write image(s) in a KTX-formatted stdio FILE stream.
 *
 * @param [in] dst		    pointer to the FILE stream to write to.
 * @param [in] textureInfo  pointer to a KTX_image_info structure providing
 *                          information about the images to be included in
 *                          the KTX file.
 * @param [in] bytesOfKeyValueData
 *                          specifies the number of bytes of key-value data.
 * @param [in] keyValueData a pointer to the keyValue data.
 * @param [in] numImages    number of images in the following array
 * @param [in] images       array of KTX_image_info providing image size and
 *                          data.
 *
 * @return	KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p dst or @p target are @c NULL
 * @exception KTX_INVALID_VALUE @c glTypeSize in @p textureInfo is not 1, 2, or 4 or
 *                              is different from the size of the type specified
 *                              in @c glType.
 * @exception KTX_INVALID_VALUE @c pixelWidth in @p textureInfo is 0 or pixelDepth != 0
 *                              && pixelHeight == 0.
 * @exception KTX_INVALID_VALUE @c numberOfFaces != 1 || numberOfFaces != 6 or
 *                              numberOfArrayElements or numberOfMipmapLevels are < 0.
 * @exception KTX_INVALID_OPERATION
 *                              numberOfFaces == 6 and images are either not 2D or
 *                              are not square.
 * @exception KTX_INVALID_OPERATION
 *							    number of images is insufficient for the specified
 *                              number of mipmap levels and faces.
 * @exception KTX_INVALID_OPERATION
 *                              the size of a provided image is different than that
 *                              required for the specified width, height or depth
 *                              or for the mipmap level being processed.
 * @exception KTX_FILE_WRITE_ERROR a system error occurred while writing the file.
 */
KTX_error_code
ktxWriteKTXF(FILE* dst, const KTX_texture_info* textureInfo,
			 GLsizei bytesOfKeyValueData, const void* keyValueData,
			 GLuint numImages, KTX_image_info images[])
{
	KTX_header header = KTX_IDENTIFIER_REF;
	GLuint i, level, dimension, cubemap = 0;
	GLuint numMipmapLevels, numArrayElements;
	GLbyte pad[4] = { 0, 0, 0, 0 };
	KTX_error_code errorCode = KTX_SUCCESS;
	GLboolean compressed = GL_FALSE;
	
	if (!dst) {
		return KTX_INVALID_VALUE;
	}

	//endianess int.. if this comes out reversed, all of the other ints will too.
	header.endianness = KTX_ENDIAN_REF;
	header.glType = textureInfo->glType;
	header.glTypeSize = textureInfo->glTypeSize;
	header.glFormat = textureInfo->glFormat;
	header.glInternalFormat = textureInfo->glInternalFormat;
	header.glBaseInternalFormat = textureInfo->glBaseInternalFormat;
	header.pixelWidth = textureInfo->pixelWidth;
	header.pixelHeight = textureInfo->pixelHeight;
	header.pixelDepth = textureInfo->pixelDepth;
	header.numberOfArrayElements = textureInfo->numberOfArrayElements;
	header.numberOfFaces = textureInfo->numberOfFaces;
	header.numberOfMipmapLevels = textureInfo->numberOfMipmapLevels;
	header.bytesOfKeyValueData = bytesOfKeyValueData;

	/* Do some sanity checking */
	if (header.glTypeSize != 1 &&
		header.glTypeSize != 2 &&
		header.glTypeSize != 4)
	{
		/* Only 8, 16, and 32-bit types supported so far */
		return KTX_INVALID_VALUE;
	}
	if (header.glTypeSize != sizeofGLtype(header.glType))
		return KTX_INVALID_VALUE;

	if (header.glType == 0 || header.glFormat == 0)
	{
		if (header.glType + header.glFormat != 0) {
			/* either both or none of glType & glFormat must be zero */
			return KTX_INVALID_VALUE;
		} else
			compressed = GL_TRUE;

	}

	/* Check texture dimensions. KTX files can store 8 types of textures:
     * 1D, 2D, 3D, cube, and array variants of these. There is currently
     * no GL extension that would accept 3D array or cube array textures
	 * but we'll let such files be created.
	 */
	if ((header.pixelWidth == 0) ||
		(header.pixelDepth > 0 && header.pixelHeight == 0))
	{
		/* texture must have width */
		/* texture must have height if it has depth */
		return KTX_INVALID_VALUE; 
	}
	if (header.pixelHeight > 0 && header.pixelDepth > 0)
		dimension = 3;
	else if (header.pixelHeight > 0)
		dimension = 2;
	else
		dimension = 1;

	if (header.numberOfFaces == 6)
	{
		if (dimension != 2)
		{
			/* cube map needs 2D faces */
			return KTX_INVALID_OPERATION;
		}
		if (header.pixelWidth != header.pixelHeight)
		{
			/* cube maps require square images */
			return KTX_INVALID_OPERATION;
		}
	}
	else if (header.numberOfFaces != 1)
	{
		/* numberOfFaces must be either 1 or 6 */
		return KTX_INVALID_VALUE;
	}

	if (header.numberOfArrayElements < 0 || header.numberOfMipmapLevels < 0)
		return KTX_INVALID_VALUE;

	if (header.numberOfArrayElements == 0)
	{
		numArrayElements = 1;
		if (header.numberOfFaces == 6)
			cubemap = 1;
	}
	else
	    numArrayElements = header.numberOfArrayElements;

	/* Check number of mipmap levels */
	if (header.numberOfMipmapLevels == 0)
	{
		numMipmapLevels = 1;
	}
	else
		numMipmapLevels = header.numberOfMipmapLevels;
	if (numMipmapLevels > 1) {
		GLuint max_dim = MAX(MAX(header.pixelWidth, header.pixelHeight), header.pixelDepth);
		if (max_dim < ((GLuint)1 << (header.numberOfMipmapLevels - 1)))
		{
			/* Can't have more mip levels than 1 + log2(max(width, height, depth)) */
			return KTX_INVALID_VALUE;
		}
	}

	if (numImages < numMipmapLevels * header.numberOfFaces)
	{
		/* Not enough images */
		return KTX_INVALID_OPERATION;
	}

	//write header
	fwrite(&header, sizeof(KTX_header), 1, dst);

	//write keyValueData
	if (bytesOfKeyValueData != 0) {
		if (keyValueData == NULL)
			return KTX_INVALID_OPERATION;

		if (fwrite(keyValueData, 1, bytesOfKeyValueData, dst) != bytesOfKeyValueData)
			return KTX_FILE_WRITE_ERROR;
	}

	/* Write the image data */
	for (level = 0, i = 0; level < numMipmapLevels; ++level)
	{
		GLuint elementBytes;
		GLsizei expectedFaceSize;
		GLuint face, faceLodSize, faceLodRounding;
		GLuint groupBytes = groupSize(header.glFormat,
									  header.glType,
								      &elementBytes);
		GLuint pixelWidth, pixelHeight, pixelDepth;
		GLuint packedRowBytes, rowBytes, rowRounding;

		pixelWidth  = MAX(1, header.pixelWidth  >> level);
		pixelHeight = MAX(1, header.pixelHeight >> level);
		pixelDepth  = MAX(1, header.pixelDepth  >> level);

		/* Calculate face sizes for this LoD based on glType, glFormat, width & height */
		expectedFaceSize = groupBytes
						   * pixelWidth
						   * pixelHeight
						   * pixelDepth
						   * numArrayElements;

		rowRounding = 0;
		packedRowBytes = groupBytes * pixelWidth;
		/* KTX format specifies UNPACK_ALIGNMENT==4 */
		if (!compressed && elementBytes < KTX_GL_UNPACK_ALIGNMENT) {
			rowBytes = KTX_GL_UNPACK_ALIGNMENT / elementBytes;
			/* The following statement is equivalent to:
			/*     packedRowBytes *= ceil((groupBytes * width) / KTX_GL_UNPACK_ALIGNMENT);
			 */
			rowBytes *= ((groupBytes * pixelWidth) + (KTX_GL_UNPACK_ALIGNMENT - 1)) / KTX_GL_UNPACK_ALIGNMENT;
			rowRounding = rowBytes - packedRowBytes;
		}

		if (rowRounding == 0) {
			faceLodSize = images[i].size;
		} else {
			/* Need to pad the rows to meet the required UNPACK_ALIGNMENT */
			faceLodSize = rowBytes * pixelHeight * pixelDepth * numArrayElements;
		}
		faceLodRounding = 3 - ((faceLodSize + 3) % 4);
		
		if (fwrite(&faceLodSize, sizeof(faceLodSize), 1, dst) != 1) {
			errorCode = KTX_FILE_WRITE_ERROR;
			goto cleanup;
		}

		for (face = 0; face < header.numberOfFaces; ++face, ++i) {
			if (!compressed) {
				/* Sanity check. */
				if (images[i].size != expectedFaceSize) {
					errorCode = KTX_INVALID_OPERATION;
					goto cleanup;
				}
			}
			if (rowRounding == 0) {
				/* Can write whole face at once */
				if (fwrite(images[i].data, faceLodSize, 1, dst) != 1) {
					errorCode = KTX_FILE_WRITE_ERROR;
					goto cleanup;
				}
			} else {
				/* Write the rows individually, padding each one */
				GLuint row;
				GLuint numRows = pixelHeight
								* pixelDepth
								* numArrayElements;
				for (row = 0; row < numRows; row++) {
					if (fwrite(&images[i].data[row*packedRowBytes], packedRowBytes, 1, dst) != 1) {
						errorCode = KTX_FILE_WRITE_ERROR;
						goto cleanup;
					}
					if (fwrite(pad, sizeof(GLbyte), rowRounding, dst) != rowRounding) {
						errorCode = KTX_FILE_WRITE_ERROR;
						goto cleanup;
					}
				}
			}
			if (faceLodRounding) {
				if (fwrite(pad, sizeof(GLbyte), faceLodRounding, dst) != faceLodRounding) {
					errorCode = KTX_FILE_WRITE_ERROR;
					goto cleanup;
				}
			}
		}
	}

cleanup:
	return errorCode;
}


/**
 * @~English
 * @brief Write image(s) to a KTX file on disk.
 *
 * @param [in] dstname		pointer to a C string that contains the path of
 * 							the file to load.
 * @param [in] textureInfo  pointer to a KTX_image_info structure providing
 *                          information about the images to be included in
 *                          the KTX file.
 * @param [in] bytesOfKeyValueData
 *                          specifies the number of bytes of key-value data.
 * @param [in] keyValueData a pointer to the keyValue data.
 * @param [in] numImages    number of images in the following array.
 * @param [in] images       array of KTX_image_info providing image size and
 *                          data.
 *
 * @return	KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_FILE_OPEN_FAILED unable to open the specified file for
 *                                 writing.
 *
 * For other exceptions, see ktxWriteKTXF().
 */
KTX_error_code
ktxWriteKTXN(const char* dstname, const KTX_texture_info* textureInfo,
			 GLsizei bytesOfKeyValueData, const void* keyValueData,
			 GLuint numImages, KTX_image_info images[])
{	
	KTX_error_code errorCode;
	FILE* dst = fopen(dstname, "wb");

	if (dst) {
		errorCode = ktxWriteKTXF(dst, textureInfo, bytesOfKeyValueData, keyValueData,
			                     numImages, images);
		fclose(dst);
	} else
	    errorCode = KTX_FILE_OPEN_FAILED;

	return errorCode;
}


/*
 * @brief Return the size of the group of elements constituting a pixel.
 *
 * @param [in] format	the format of the image data
 * @param [in] type		the type of the image data
 *
 * @return	the size in bytes or < 0 if the type, format or combination
 *          is invalid.
 */
static GLint
groupSize(GLenum format, GLenum type, GLint* elementBytes)
{
	switch (format) {
	case GL_ALPHA:
    #if defined(GL_RED)
	case GL_RED:
	case GL_GREEN:
	case GL_BLUE:
    #endif
	case GL_LUMINANCE:
		return (*elementBytes = sizeofGLtype(type));
		break;
	case GL_LUMINANCE_ALPHA:
    #if defined(GL_RG)
	case GL_RG:
    #endif
		return (*elementBytes = sizeofGLtype(type)) * 2;
		break;
	case GL_RGB:
    #if defined(GL_BGR)
	case GL_BGR:
    #endif
		if(type == GL_UNSIGNED_SHORT_5_6_5) return (*elementBytes = 2);
		else                                return (*elementBytes = sizeofGLtype(type)) * 3;
		break;
	case GL_RGBA:
    #if defined(GL_BGRA)
	case GL_BGRA:
    #endif
		if(type == GL_UNSIGNED_SHORT_4_4_4_4 || type == GL_UNSIGNED_SHORT_5_5_5_1)
			return (*elementBytes = 2);
		else
			return (*elementBytes = sizeofGLtype(type)) * 4;
		break;
	default:
		break;
	}

	return -1;
}

/*
 * @brief Return the sizeof the GL type in basic machine units
 */
static GLint
sizeofGLtype(GLenum type)
{
	switch (type) {
		case GL_BYTE:
			return sizeof(GLbyte);
		case GL_UNSIGNED_BYTE:
			return sizeof(GLubyte);
		case GL_SHORT:
			return sizeof(GLshort);
		case GL_UNSIGNED_SHORT:
			return sizeof(GLushort);
        #if defined(GL_INT)
		case GL_INT:
			return sizeof(GLint);
		#endif
        #if defined(GL_UNSIGNED_INT)
		case GL_UNSIGNED_INT:
			return sizeof(GLuint);
		#endif
        #if defined(GL_HALF_FLOAT)
		case GL_HALF_FLOAT:
			return sizeof(GLhalf);
		#endif
		case GL_FLOAT:
			return sizeof(GLfloat);
	}
	return -1;
}


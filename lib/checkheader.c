/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/* $Id$ */

/**
 * @internal
 * @file checkheader.c
 * @~English
 *
 * @brief Function to verify a KTX file header
 *
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
 * Author: Georg Kolling, Imagination Technology with modifications
 * by Mark Callow, HI Corporation.
 */
#include <assert.h>
#include <string.h>

#include "GL/glcorearb.h"
#include "ktx.h"
#include "ktxint.h"

/**
 * @internal
 * @~English
 * @brief Check a KTX file header.
 *
 * As well as checking that the header identifies a KTX file, the function
 * sanity checks the values and returns information about the texture in a
 * struct KTX_supplementary_info.
 *
 * @param pHeader	pointer to the KTX header to check
 * @param pSuppInfo	pointer to a KTX_supplementary_info structure in which to
 *                  return information about the texture.
 * 
 * @author Georg Kolling, Imagination Technology
 * @author Mark Callow, HI Corporation
 */
KTX_error_code _ktxCheckHeader(KTX_header* pHeader,
                               KTX_supplemental_info* pSuppInfo)
{
	ktx_uint8_t identifier_reference[12] = KTX_IDENTIFIER_REF;
	ktx_uint32_t max_dim;
    
    assert(pHeader != NULL && pSuppInfo != NULL);

	/* Compare identifier, is this a KTX file? */
	if (memcmp(pHeader->identifier, identifier_reference, 12) != 0)
	{
		return KTX_UNKNOWN_FILE_FORMAT;
	}

	if (pHeader->endianness == KTX_ENDIAN_REF_REV)
	{
		/* Convert endianness of pHeader fields. */
		_ktxSwapEndian32(&pHeader->glType, 12);

		if (pHeader->glTypeSize != 1 &&
			pHeader->glTypeSize != 2 &&
			pHeader->glTypeSize != 4)
		{
			/* Only 8-, 16-, and 32-bit types supported so far. */
			return KTX_FILE_DATA_ERROR;
		}
	}
	else if (pHeader->endianness != KTX_ENDIAN_REF)
	{
		return KTX_FILE_DATA_ERROR;
	}

	/* Check glType and glFormat */
	pSuppInfo->compressed = 0;
	if (pHeader->glType == 0 || pHeader->glFormat == 0)
	{
		if (pHeader->glType + pHeader->glFormat != 0)
		{
			/* either both or none of glType, glFormat must be zero */
			return KTX_FILE_DATA_ERROR;
		}
		pSuppInfo->compressed = 1;
	}
    
    if (pHeader->glFormat == pHeader->glInternalFormat) {
        // glInternalFormat is either unsized (which is no longer and should
        // never have been supported by libktx) or glFormat is sized.
        return KTX_FILE_DATA_ERROR;
    }

	/* Check texture dimensions. KTX files can store 8 types of textures:
	   1D, 2D, 3D, cube, and array variants of these. There is currently
	   no GL extension for 3D array textures. */
	if ((pHeader->pixelWidth == 0) ||
		(pHeader->pixelDepth > 0 && pHeader->pixelHeight == 0))
	{
		/* texture must have width */
		/* texture must have height if it has depth */
		return KTX_FILE_DATA_ERROR; 
	}

    
    if (pHeader->pixelDepth > 0)
    {
        if (pHeader->numberOfArrayElements > 0)
        {
            /* No 3D array textures yet. */
            return KTX_UNSUPPORTED_TEXTURE_TYPE;
        }
        pSuppInfo->textureDimension = 3;
    }
    else if (pHeader->pixelHeight > 0)
	{
		pSuppInfo->textureDimension = 2;
	}
    else
    {
        pSuppInfo->textureDimension = 1;
    }

	if (pHeader->numberOfFaces == 6)
	{
		if (pSuppInfo->textureDimension != 2)
		{
			/* cube map needs 2D faces */
			return KTX_FILE_DATA_ERROR;
		}
	}
	else if (pHeader->numberOfFaces != 1)
	{
		/* numberOfFaces must be either 1 or 6 */
		return KTX_FILE_DATA_ERROR;
	}
    
	/* Check number of mipmap levels */
	if (pHeader->numberOfMipmapLevels == 0)
	{
		pSuppInfo->generateMipmaps = 1;
		pHeader->numberOfMipmapLevels = 1;
	}
    else
    {
        pSuppInfo->generateMipmaps = 0;
    }

    /* This test works for arrays too because height or depth will be 0. */
    max_dim = MAX(MAX(pHeader->pixelWidth, pHeader->pixelHeight), pHeader->pixelDepth);
	if (max_dim < ((ktx_uint32_t)1 << (pHeader->numberOfMipmapLevels - 1)))
	{
		/* Can't have more mip levels than 1 + log2(max(width, height, depth)) */
		return KTX_FILE_DATA_ERROR;
	}

	return KTX_SUCCESS;
}

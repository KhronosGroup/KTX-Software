/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/* $Id$ */

/**
 * @file
 * @~English
 *
 * @brief Functions for reading KTX files.
 *
 * @author Mark Callow, Edgewise Consulting
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

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "ktx.h"
#include "ktxint.h"
#include "ktxcontext.h"

#if 0
/**
 * @~English
 * @brief Load a GL texture object from a stdio FILE stream.
 *
 * This function will unpack compressed GL_ETC1_RGB8_OES and GL_ETC2_* format
 * textures in software when the format is not supported by the GL context,
 * provided the library has been compiled with SUPPORT_SOFTWARE_ETC_UNPACK
 * defined as 1.
 *
 * It will also convert texture with legacy formats to their modern equivalents
 * when the format is not supported by the GL context, provided that the library
 * has been compiled with SUPPORT_LEGACY_FORMAT_CONVERSION defined as 1.
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
#endif
KTX_error_code
ktxReadHeader(KTX_context ctx, KTX_header* pHeader,
              KTX_supplemental_info* pSuppInfo)
{
    ktxContext* kc = (ktxContext*)ctx;
    KTX_error_code errorCode;
    
    if (pHeader == NULL || pSuppInfo == NULL || kc == NULL
        || !kc->stream.read || !kc->stream.skip)
        return KTX_INVALID_VALUE;
    
    if (kc->state != KTX_CS_START)
        return KTX_INVALID_OPERATION;

    errorCode = kc->stream.read(&kc->stream, &kc->header, KTX_HEADER_SIZE);
    if (errorCode != KTX_SUCCESS)
        return errorCode;
    
    errorCode = _ktxCheckHeader(&kc->header, pSuppInfo);
    if (errorCode == KTX_SUCCESS) {
        memcpy(pHeader, &kc->header, sizeof(KTX_header));
        kc->textureDimension = pSuppInfo->textureDimension;
        kc->state = KTX_CS_HEADER_READ;
    }

    return errorCode;
}

/* XXX What about the library needing some of this data, e.g. miplevel order? */
KTX_error_code
ktxReadKVData(KTX_context ctx, ktx_uint32_t* pKvdLen, ktx_uint8_t ** ppKvd)
{
    ktxContext* kc = (ktxContext*)ctx;
    KTX_error_code errorCode = KTX_SUCCESS;

    if (kc == NULL || !kc->stream.read || !kc->stream.skip)
        return KTX_INVALID_VALUE;
    
    if (kc->state != KTX_CS_HEADER_READ)
        return KTX_INVALID_OPERATION;
    
    if (ppKvd) {
        *ppKvd = NULL;
        if (pKvdLen == NULL)
            return KTX_INVALID_OPERATION;
        *pKvdLen = kc->header.bytesOfKeyValueData;
        if (*pKvdLen) {
            *ppKvd = (unsigned char*)malloc(*pKvdLen);
            if (*ppKvd == NULL)
                return KTX_OUT_OF_MEMORY;
            errorCode = kc->stream.read(&kc->stream, *ppKvd, *pKvdLen);
            if (errorCode != KTX_SUCCESS)
            {
                free(*ppKvd);
                *ppKvd = NULL;
            }
        }
    } else {
        /* skip key/value metadata */
        errorCode = kc->stream.skip(&kc->stream,
                                    kc->header.bytesOfKeyValueData);
    }
    if (errorCode == KTX_SUCCESS)
        kc->state = KTX_CS_KVD_READ;
    
    return errorCode;
}

/**
 * @~English
 * @brief Open a KTX from file on disk and return texture information.
 *
 * @param [in] filename		pointer to a C string that contains the path of
 * 							the file to load.
 * @param [in,out] pTexinfo	pointer to a struct KTX_texture_info into which
 *                          the function writes information about the texture.
 *
 * @return	KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_FILE_OPEN_FAILED	The specified file could not be opened.
 * @exception KTX_UNEXPECTED_END_OF_FILE The file does not contain the expected
 *                                       amount of data.
 *
 */
KTX_error_code
ktxOpenKTXF(FILE* file, KTX_context* pContext)
{
    ktxContext* kc;
    KTX_error_code errorCode;

    if (file == NULL || pContext == NULL)
        return KTX_INVALID_VALUE;

    kc = (ktxContext*)malloc(sizeof(ktxContext));
    if (kc == NULL)
        return KTX_OUT_OF_MEMORY;
    
    errorCode = ktxContext_fileInit(kc, file);
    if (errorCode == KTX_SUCCESS)
        *pContext = (void*)kc;
    
    return errorCode;
}

/**
 * @~English
 * @brief Open a KTX from file on disk and return texture information.
 *
 * @param [in] filename		pointer to a C string that contains the path of
 * 							the file to load.
 * @param [in,out] pTexinfo	pointer to a struct KTX_texture_info into which
 *                          the function writes information about the texture.
 *
 * @return	KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_FILE_OPEN_FAILED	The specified file could not be opened.
 * @exception KTX_UNEXPECTED_END_OF_FILE The file does not contain the expected
 *                                       amount of data.
 *
 */
KTX_error_code
ktxOpenKTXN(const char* const filename, KTX_context* pContext)
{
    KTX_error_code errorCode;
    FILE* file;
    
    if (filename == NULL || pContext == NULL)
        return KTX_INVALID_VALUE;
    
	file = fopen(filename, "rb");

	if (file) {
		errorCode = ktxOpenKTXF(file, pContext);
	} else
		errorCode = KTX_FILE_OPEN_FAILED;

    return errorCode;
}

/**
 * @~English
 * @brief Open a KTX from file on disk and return texture information.
 *
 * @param [in] filename		pointer to a C string that contains the path of
 * 							the file to load.
 * @param [in,out] pTexinfo	pointer to a struct KTX_texture_info into which
 *                          the function writes information about the texture.
 *
 * @return	KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_FILE_OPEN_FAILED	The specified file could not be opened.
 * @exception KTX_UNEXPECTED_END_OF_FILE The file does not contain the expected
 *                                       amount of data.
 *
 */
KTX_error_code
ktxOpenKTXM(const void* bytes, size_t size, KTX_context* pContext)
{
    ktxContext* kc;
    KTX_error_code errorCode;

    if (bytes == NULL || size == 0 || pContext == NULL)
        return KTX_INVALID_VALUE;
    
    kc = (ktxContext*)malloc(sizeof(ktxContext));
    if (kc == NULL)
        return KTX_OUT_OF_MEMORY;
    
    errorCode = ktxContext_memInit(kc, bytes, size);

    if (errorCode == KTX_SUCCESS)
        *pContext = kc;
    
    return errorCode;
}

KTX_error_code
ktxCloseKTX(KTX_context ctx)
{
    ktxContext* kc = (ktxContext*)ctx;
    
    if (kc == NULL || !kc->stream.close)
        return KTX_INVALID_VALUE;
    
    kc->stream.close(&kc->stream);
    
    free(kc);
}

#if 0
/**
 * @~English
 * @brief Load a GL texture object from KTX formatted data in memory.
 *
 * @param [in] bytes		pointer to the array of bytes containing
 * 							the KTX format data to load.
 * @param [in] size			size of the memory array containing the
 *                          KTX format data.
 * @param [in,out] pTexture	name of the GL texture to load. See
 *                          ktxReadKTXF() for details.
 * @param [out] pTarget 	@p *pTarget is set to the texture target used. See
 *                          ktxReadKTXF() for details.
 * @param [out] pDimensions @p the texture's base level width depth and height
 *                          are returned in structure to which this points.
 *                          See ktxReadKTXF() for details.
 * @param [out] pIsMipmapped @p *pIsMipMapped is set to indicate if the loaded
 *                          texture is mipmapped. See ktxReadKTXF() for
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
 * @exception KTX_INVALID_VALUE		See ktxReadKTXF() for causes.
 * @exception KTX_INVALID_OPERATION	See ktxReadKTXF() for causes.
 * @exception KTX_UNEXPECTED_END_OF_FILE See ktxReadKTXF() for causes.
 *
 * @exception KTX_GL_ERROR			See ktxReadKTXF() for causes.
 */
#endif
KTX_error_code
ktxReadImages(KTX_context ctx, PFNKTXIMAGECB imageCb, void* userdata)
{
    ktxContext* kc = (ktxContext*)ctx;
    ktx_uint32_t    dataSize = 0;
    ktx_uint32_t    faceLodSize;
    ktx_uint32_t    faceLodSizeRounded;
    ktx_uint32_t    miplevel;
    ktx_uint32_t    face;
    KTX_error_code	errorCode = KTX_SUCCESS;
    void*			data = NULL;
 
    if (kc == NULL || imageCb == NULL)
        return KTX_INVALID_VALUE;
    
    if (kc->state != KTX_CS_KVD_READ)
        return KTX_INVALID_OPERATION;
    
    for (miplevel = 0; miplevel < kc->header.numberOfMipmapLevels; ++miplevel)
    {
        GLsizei width, heightOrLayers, depthOrLayers;
        
        width = MAX(1, kc->header.pixelWidth  >> miplevel);
        /* Array textures have the same number of layers at each mip level. */
        if (kc->header.numberOfArrayElements > 0 && kc->textureDimension == 1)
            heightOrLayers = kc->header.numberOfArrayElements;
        else
            heightOrLayers = MAX(1, kc->header.pixelHeight >> miplevel);
        if (kc->header.numberOfArrayElements > 0 && kc->textureDimension == 2)
            depthOrLayers = kc->header.numberOfArrayElements;
        else
            depthOrLayers  = MAX(1, kc->header.pixelDepth  >> miplevel);
        
        errorCode = kc->stream.read(&kc->stream, &faceLodSize,
                                    sizeof(ktx_uint32_t));
        if (errorCode != KTX_SUCCESS) {
            goto cleanup;
        }
        if (kc->header.endianness == KTX_ENDIAN_REF_REV) {
            _ktxSwapEndian32(&faceLodSize, 1);
        }
        faceLodSizeRounded = (faceLodSize + 3) & ~(ktx_uint32_t)3;
        if (!data) {
            /* allocate memory sufficient for the first miplevel */
            data = malloc(faceLodSizeRounded);
            if (!data) {
                errorCode = KTX_OUT_OF_MEMORY;
                goto cleanup;
            }
            dataSize = faceLodSizeRounded;
        }
        else if (dataSize < faceLodSizeRounded) {
            /* subsequent miplevels cannot be larger than the base miplevel */
            errorCode = KTX_FILE_DATA_ERROR;
            goto cleanup;
        }
        
        /* All array elements are passed in a group because that is how GL needs
         * them. Hence no for (element = 0; element < kc->numArrayElements ...)
         */
        for (face = 0; face < kc->header.numberOfFaces; ++face)
        {
            errorCode = kc->stream.read(&kc->stream, data, faceLodSizeRounded);
            if (errorCode != KTX_SUCCESS) {
                goto cleanup;
            }
            
            /* Perform endianness conversion on texture data */
            if (kc->header.endianness == KTX_ENDIAN_REF_REV) {
                if (kc->header.glTypeSize == 2)
                    _ktxSwapEndian16((ktx_uint16_t*)data, faceLodSize / 2);
                else if (kc->header.glTypeSize == 4)
                    _ktxSwapEndian32((ktx_uint32_t*)data, faceLodSize / 4);
            }
            
            // face is either 0 or one of the 6 cube map faces
            // miplevel is the mipmiplevel from 0 to the max miplevel dependent on the
            //       texture size.
            // width is the width of the image
            // height is either the height of the image or, for 1D array
            //        textures the number of layers in the array
            // depth is either the depth of the image or for 2D array or cube
            //       map arrays the number of levels in the array.
            errorCode = imageCb(miplevel, face,
                                width, heightOrLayers, depthOrLayers,
                                faceLodSize, data, userdata);

            if (errorCode != KTX_SUCCESS)
                goto cleanup;
            
        }
    }
    
cleanup:
    free(data);

    if (errorCode == KTX_SUCCESS)
        kc->state = KTX_CS_IMAGES_READ;

    return errorCode;
}


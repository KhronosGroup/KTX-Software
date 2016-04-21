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

/**
 * @~English
 * @brief Open the KTX file pointed at by a stdio FILE.
 *
 * The function returns a handle to a KTX_context object which must be used
 * for further operations on the file.
 *
 * @param [in] file		    pointer to a stdio FILE created from the desired
 * 							file.
 * @param [in,out] pContext	pointer to a KTX_context into which the context
 *                          handle is written.
 *
 * @return	KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE	@p file is @c NULL or @p pContext is @c NULL.
 * @exception KTX_OUT_OF_MEMORY not enough memory to allocation the context.
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
 * @brief Open the named KTX file on disk.
 *
 * The function returns a handle to a KTX_context object which must be used
 * for further operations on the file.
 *
 * @param [in] filename		pointer to a C string that contains the path of
 * 							the file to load.
 * @param [in,out] pContext	pointer to a KTX_context into which the context
 *                          handle is written.
 *
 * @return	KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE	@p filename is @c NULL or @p pContext is
 *                              @c NULL.
 * @exception KTX_FILE_OPEN_FAILED	The specified file could not be opened.
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
 * @brief Open a KTX file from KTX formatted data in memory.
 *
 * The function returns a handle to a KTX_context object which must be used
 * for further operations on the file.
 *
 * @param [in] bytes		pointer to the data in memory.
 * @param [in] size         length of the KTX data in memory.
 * @param [in,out] pContext	pointer to a KTX_context into which the context
 *                          handle is written.
 *
 * @return	KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE	@p bytes is @c NULL, @p size is 0 or @p pContext
 *                              is @c NULL.
 * @exception KTX_OUT_OF_MEMORY not enough memory to allocation the context.
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

/**
 * @~English
 * @brief Close a KTX file, freeing the associated context.
 *
 * @return	KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE	@p ctx is @c NULL.
 */
KTX_error_code
ktxCloseKTX(KTX_context ctx)
{
    ktxContext* kc = (ktxContext*)ctx;
    
    if (kc == NULL || !kc->stream.close)
        return KTX_INVALID_VALUE;
    
    kc->stream.close(&kc->stream);
    
    free(kc);
}

/**
 * @~English
 * @brief Read the header of the KTX file associated with a KTX_context object.
 *
 * This function byte-swaps the header, if necessary, and checks it for
 * validity.
 *
 * @param [in] ctx			handle of the KTX_context representing the file.
 * @param [in,out] pHeader  pointer to a KTX_header struct into which the
 *                          function writes the header data of the file.
 * @param [out] pSuppInfo 	pointer to a KTX_supplemental_info struct into
 *                          which the function writes information about the
 *                          texture derived while it is checking the validity
 *                          of the header.
 *
 * @return	KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_FILE_DATA_ERROR the data in the header is inconsistent with
 *                                the KTX specification. 
 * @exception KTX_INVALID_VALUE @p ctx, @p pHeader or @p pSuppInfo is @c NULL.
 * @exception KTX_INVALID_OPERATION the header of the file associated with the
 *                                  @p ctx has already been read.
 * @exception KTX_UNEXPECTED_END_OF_FILE the file does not contain the
 * 										 expected amount of data.
 * @exception KTX_UNKNOWN_FILE_FORMAT the file is not a KTX file.
 * @exception KTX_UNSUPPORTED_TEXTURE_TYPE the header indicates a 3D array
 *                                         texture.
 */
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

/**
 * @~English
 * @brief Read the key-value data of the KTX file associated with a KTX_context
 *        object.
 *
 * @param [in] ctx			handle of the KTX_context representing the file.
 * @param [in,out] pKvdLen	if not NULL, @p *pKvdLen is set to the number of
 *                          bytes of key-value data pointed at by @p *ppKvd.
 *                          Must not be NULL, if @p ppKvd is not NULL.
 * @param [in,out] ppKvd	if not NULL, @p *ppKvd is set to the point to a
 *                          block of memory containing key-value data read from
 *                          the file. The application is responsible for
 *                          freeing the memory.
 *
 * @return	KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_OPERATION the header of the file associated with the
 *                                  @p ctx has not yet been read or the key-
 *                                  value data has already been read.
 * @exception KTX_INVALID_VALUE     @p ctx is @c NULL or not a valid ctx handle.
 * @exception KTX_OUT_OF_MEMORY     not enough memory to allocate a block to
 *                                  hold the data.
 */
/* XXX What to do when the library needs some of this data too,
 * e.g. miplevel order?
 */
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
            return KTX_INVALID_VALUE;
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
 * @brief Read the images from the KTX file associated with a KTX_context
 *        object.
 *
 * Each image is passed to an application-supplied callback function. Note that
 * in the case of array textures all layers are passed in a single invocation
 * of the callback as graphics APIs typically need them this way. The callback
 * must copy the image data if it wishes to preserve it. The buffer whose
 * pointer is passed is freed when this function exits.
 *
 * @param [in] ctx			handle of the KTX_context representing the file.
 * @param [in,out] imageCb  a PFNKTXIMAGECB pointer holding the address of the
 *                          callback function.
 * @param [in,out] userdata a pointer to application-specific data which is
 *                          passed to the callback along with the image data.
 *
 * @return	KTX_SUCCESS on success, other KTX_* enum values on error. The
 *          following are returned directly by ktxReadImages. @p imageCb may
 *          return these for other causes or additional errors.
 *
 * @exception KTX_INVALID_OPERATION the header or key-value data of the file
 *                                  associated with the @p ctx has not yet been
 *                                  read or the images have already been read.
 * @exception KTX_INVALID_VALUE     @p ctx is @c NULL or not a valid ctx handle
 *                                  or @p imageCb is @c NULL.
 * @exception KTX_OUT_OF_MEMORY     not enough memory to allocate a block to
 *                                  hold the base level image.
 */
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
 
    if (kc == NULL || !kc->stream.read || !kc->stream.skip || imageCb == NULL)
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


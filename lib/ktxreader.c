/* -*- tab-width: 4; -*- */
/* vi: set et sw=2 ts=4: */

/* $Id$ */

/**
 * @internal
 * @file
 * @~English
 *
 * @brief Functions for reading KTX files.
 *
 * @author Mark Callow, Edgewise Consulting
 */

/*
Copyright (c) 2016 Mark Callow, Edgewise Consulting.

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

#include "ktx.h"
#include "ktxint.h"
#include "gl_format.h"

#include "ktxreader.h"
#include "ktxfilestream.h"

/**
 * @defgroup ktx_reader KTX Reader
 * @brief Read KTX files independently of OpenGL and Vulkan.
 * @{
 */

/**
 * @~English
 * @internal
 * @brief Initialize a ktxReader object.
 *
 * @param [in] This     pointer to the ktxReader to initialize.
 */
void
ktxReader_construct(ktxReader* This)
{
  assert(This != NULL);
  memset(This, 0, sizeof(ktxReader));
  This->state = KTX_RS_START;
  This->selfOpenedFile = KTX_FALSE;
}

/**
 * @~English
 * @internal
 * @brief Initialize a ktxReader object to read KTX data from a file
 *        represented by a stdio FILE.
 *
 * @param [in] This     pointer to the ktxReader to initialize.
 * @param [in] file     pointer to the stdio FILE to use.
 */
KTX_error_code
ktxReader_constructFromFile(ktxReader* This, FILE* file)
{
    assert(This != NULL);
    ktxReader_construct(This);
    return ktxFileStream_construct(&This->stream, file);
}

/**
 * @~English
 * @internal
 * @brief Initialize a ktxReader object to read KTX data in memory.
 *
 * @param [in] This     pointer to the ktxReader to initialize.
 * @param [in] bytes    pointer to the memory location to use.
 * @param [in] size     length in bytes of the KTX file pointed to by @a bytes.
 */
KTX_error_code
ktxReader_constructFromMem(ktxReader* This, const void* bytes, size_t size)
{
    assert(This != NULL);
    ktxReader_construct(This);
    return ktxMemStream_construct(&This->stream, &This->mem, bytes, size);
}

/**
 * @~English
 * @brief Create a ktxReader to read KTX data from a stdio FILE.
 *
 * The function returns a KTX_reader, an opaque handle to the underlying
 * ktxReader, which must be used for further operations on the file.
 *
 * @param [in] file		    pointer to a stdio FILE created from the desired
 * 							file.
 * @param [in,out] pReader	pointer to a KTX_reader into which the reader's
 *                          handle is written.
 *
 * @return	KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE	@p file is @c NULL or @p pReader is @c NULL.
 * @exception KTX_OUT_OF_MEMORY not enough memory to allocation the context.
 *
 * @warning This command is subject to change before it is merged to master.
 */
KTX_error_code
ktxOpenKTXF(FILE* file, KTX_reader* pReader)
{
    ktxReader* This;
    KTX_error_code errorCode;
    
    if (file == NULL || pReader == NULL)
        return KTX_INVALID_VALUE;
    
    This = (ktxReader*)malloc(sizeof(ktxReader));
    if (This == NULL)
        return KTX_OUT_OF_MEMORY;
    
    errorCode = ktxReader_constructFromFile(This, file);
    if (errorCode == KTX_SUCCESS)
        *pReader = (void*)This;
    
    return errorCode;
}

/**
 * @~English
 * @brief Create a ktxReader to read KTX data from a named file on disk.
 *
 * The function returns a KTX_reader, an opaque handle to the underlying
 * ktxReader, which must be used for further operations on the file.
 *
 * @param [in] filename		pointer to a C string that contains the path of
 * 							the file to open.
 * @param [in,out] pReader	pointer to a KTX_reader into which the reader's
 *                          handle is written.
 *
 * @return	KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE	@p filename is @c NULL or @p pReader is
 *                              @c NULL.
 * @exception KTX_FILE_OPEN_FAILED	The specified file could not be opened.
 *
 * @warning This command is subject to change before it is merged to master.
 */
KTX_error_code
ktxOpenKTXN(const char* const filename, KTX_reader* pReader)
{
    KTX_error_code errorCode;
    FILE* file;
    
    if (filename == NULL || pReader == NULL)
        return KTX_INVALID_VALUE;
    
    file = fopen(filename, "rb");
    
    if (file) {
        errorCode = ktxOpenKTXF(file, pReader);
        if (errorCode == KTX_SUCCESS) {
            ktxReader* This = (ktxReader*)*pReader;
            This->selfOpenedFile = KTX_TRUE;
        }
    } else
        errorCode = KTX_FILE_OPEN_FAILED;
    
    return errorCode;
}

/**
 * @~English
 * @brief Create a KTX reader to read KTX formatted data in memory.
 *
 * The function returns a KTX_reader, an opaque handle to the underlying
 * ktxReader, which must be used for further operations on the file.
 *
 * @param [in] bytes		pointer to the data in memory.
 * @param [in] size         length of the KTX data in memory.
 * @param [in,out] pReader	pointer to a KTX_reader into which the reader's
 *                          handle is written.
 *
 * @return	KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE	@p bytes is @c NULL, @p size is 0 or @p pReader
 *                              is @c NULL.
 * @exception KTX_OUT_OF_MEMORY not enough memory to allocation the context.
 *
 * @warning This command is subject to change before it is merged to master.
 */
KTX_error_code
ktxOpenKTXM(const void* bytes, size_t size, KTX_reader* pReader)
{
    ktxReader* This;
    KTX_error_code errorCode;
    
    if (bytes == NULL || size == 0 || pReader == NULL)
        return KTX_INVALID_VALUE;
    
    This = (ktxReader*)malloc(sizeof(ktxReader));
    if (This == NULL)
        return KTX_OUT_OF_MEMORY;
    
    errorCode = ktxReader_constructFromMem(This, bytes, size);
    
    if (errorCode == KTX_SUCCESS)
        *pReader = This;
    
    return errorCode;
}

/**
 * @~English
 * @brief Close a ktxReader.
 *
 * Closes the file opened for a ktxReader created by ktxOpenKTXN. Frees the
 * memory associated with the ktxReader.
 *
 * @param reader    handle of the ktxReader to close.
 *
 * @return KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE     @p reader is @c NULL or not a valid reader
 *                                  handle.
 *
 * @warning This command is subject to change before it is merged to master.
 */
KTX_error_code
ktxReader_close(KTX_reader reader)
{
    ktxReader* This = (ktxReader*)reader;
    
    if (This == NULL || !This->stream.read || !This->stream.skip)
        return KTX_INVALID_VALUE;
    
    if (This->selfOpenedFile) {
        assert(This->stream.type == eStreamTypeFile);
        fclose(This->stream.data.file);
    }
    
    free(This);
	return KTX_SUCCESS;
}

/**
 * @~English
 * @brief Read the header from KTX data.
 *
 * This function byte-swaps the header, if necessary, and checks it for
 * validity.
 *
 * @param [in] reader		handle of the ktxReader opened on the data.
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
 * @exception KTX_INVALID_VALUE @p reader, @p pHeader or @p pSuppInfo is
 *                              @c NULL or @p reader is not a valid reader
 *                              handle.
 * @exception KTX_INVALID_OPERATION the header of the file associated with the
 *                                  @p reader has already been read.
 * @exception KTX_UNEXPECTED_END_OF_FILE the file does not contain the
 * 										 expected amount of data.
 * @exception KTX_UNKNOWN_FILE_FORMAT the file is not a KTX file.
 * @exception KTX_UNSUPPORTED_TEXTURE_TYPE the header indicates a 3D array
 *                                         texture.
 *
 * @warning This command is subject to change before it is merged to master.
 */
KTX_error_code
ktxReader_readHeader(KTX_reader reader, KTX_header* pHeader,
                   KTX_supplemental_info* pSuppInfo)
{
    ktxReader* This = (ktxReader*)reader;
    KTX_error_code errorCode;
    
    if (pHeader == NULL || pSuppInfo == NULL || This == NULL
        || !This->stream.read || !This->stream.skip)
        return KTX_INVALID_VALUE;
    
    if (This->state != KTX_RS_START)
        return KTX_INVALID_OPERATION;

    errorCode = This->stream.read(&This->stream, &This->header, KTX_HEADER_SIZE);
    if (errorCode != KTX_SUCCESS)
        return errorCode;
    
    errorCode = _ktxCheckHeader(&This->header, pSuppInfo);
    if (errorCode == KTX_SUCCESS) {
        memcpy(pHeader, &This->header, sizeof(KTX_header));
        This->textureDimension = pSuppInfo->textureDimension;
        This->state = KTX_RS_HEADER_READ;
    }

    return errorCode;
}

/**
 * @~English
 * @brief Read the key-value data from KTX data.
 *
 * @param [in] reader		handle of the ktxReader opened on the data.
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
 *                                  @p reader has not yet been read or the key-
 *                                  value data has already been read.
 * @exception KTX_INVALID_VALUE     @p reader is @c NULL or not a valid reader
 *                                  handle.
 * @exception KTX_OUT_OF_MEMORY     not enough memory to allocate a block to
 *                                  hold the data.
 *
 * @warning This command is subject to change before it is merged to master.
 */
/* XXX What to do when the library needs some of this data too,
 * e.g. miplevel order?
 */
KTX_error_code
ktxReader_readKVData(KTX_reader reader,
                     ktx_uint32_t* pKvdLen, ktx_uint8_t** ppKvd)
{
    ktxReader* This = (ktxReader*)reader;
    KTX_error_code errorCode = KTX_SUCCESS;

    if (This == NULL || !This->stream.read || !This->stream.skip)
        return KTX_INVALID_VALUE;
    
    if (This->state != KTX_RS_HEADER_READ)
        return KTX_INVALID_OPERATION;
    
    if (ppKvd) {
        *ppKvd = NULL;
        if (pKvdLen == NULL)
            return KTX_INVALID_VALUE;
        *pKvdLen = This->header.bytesOfKeyValueData;
        if (*pKvdLen) {
            *ppKvd = (unsigned char*)malloc(*pKvdLen);
            if (*ppKvd == NULL)
                return KTX_OUT_OF_MEMORY;
            errorCode = This->stream.read(&This->stream, *ppKvd, *pKvdLen);
            if (errorCode != KTX_SUCCESS)
            {
                free(*ppKvd);
                *ppKvd = NULL;
            }
        }
    } else {
        /* skip key/value metadata */
        errorCode = This->stream.skip(&This->stream,
                                    This->header.bytesOfKeyValueData);
    }
    if (errorCode == KTX_SUCCESS)
        This->state = KTX_RS_KVD_READ;
    
    return errorCode;
}

/**
 * @~English
 * @brief Read the images from KTX data.
 *
 * Each image is passed to an application-supplied callback function. Note that
 * in the case of array textures all layers are passed in a single invocation
 * of the callback as graphics APIs typically need them this way. The callback
 * must copy the image data if it wishes to preserve it. The buffer whose
 * pointer is passed to the callback is freed when this function exits.
 *
 * @param [in] reader		handle of the ktxReader opened on the data.
 * @param [in,out] imageCb  the address of a callback function which is called
 *                          with the data for each image.
 * @param [in,out] userdata the address of application-specific data which is
 *                          passed to the callback along with the image data.
 *
 * @return	KTX_SUCCESS on success, other KTX_* enum values on error. The
 *          following are returned directly by ktxReadImages. @p imageCb may
 *          return these for other causes or may return additional errors.
 *
 * @exception KTX_INVALID_OPERATION the header or key-value data of the file
 *                                  associated with the @p reader has not yet
 *                                  been read or the images have already been
 *                                  read.
 * @exception KTX_INVALID_VALUE     @p reader is @c NULL or not a valid reader
 *                                  handle or @p imageCb is @c NULL.
 * @exception KTX_OUT_OF_MEMORY     not enough memory to allocate a block to
 *                                  hold the base level image.
 *
 * @warning This command is subject to change before it is merged to master.
 */
KTX_error_code
ktxReader_readImages(KTX_reader reader, PFNKTXIMAGECB imageCb, void* userdata)
{
    ktxReader* This = (ktxReader*)reader;
    ktx_uint32_t    dataSize = 0;
    ktx_uint32_t    faceLodSize;
    ktx_uint32_t    faceLodSizeRounded;
    ktx_uint32_t    miplevel;
    ktx_uint32_t    face;
    ktx_uint32_t    layers;
    KTX_error_code	errorCode = KTX_SUCCESS;
    void*			data = NULL;
 
    if (This == NULL || !This->stream.read || !This->stream.skip)
        return KTX_INVALID_VALUE;
    
    if (imageCb == NULL)
        return KTX_INVALID_VALUE;

    if (This->state != KTX_RS_KVD_READ)
        return KTX_INVALID_OPERATION;
    
    if (This->header.numberOfArrayElements == 0)
        layers = 1;
    else
        layers = This->header.numberOfArrayElements;

    for (miplevel = 0; miplevel < This->header.numberOfMipmapLevels; ++miplevel)
    {
        GLsizei width, height, depth;
        
        width = MAX(1, This->header.pixelWidth  >> miplevel);
        /* Array textures have the same number of layers at each mip level. */
        if (This->textureDimension == 1) {
            height = 1;
            depth = 1;
        } else
            height = MAX(1, This->header.pixelHeight >> miplevel);
        if (This->textureDimension == 2)
            depth = 1;
        else
            depth  = MAX(1, This->header.pixelDepth  >> miplevel);
        
        errorCode = This->stream.read(&This->stream, &faceLodSize,
                                    sizeof(ktx_uint32_t));
        if (errorCode != KTX_SUCCESS) {
            goto cleanup;
        }
        if (This->header.endianness == KTX_ENDIAN_REF_REV) {
            _ktxSwapEndian32(&faceLodSize, 1);
        }
        faceLodSizeRounded = (faceLodSize + 3) & ~(ktx_uint32_t)3;
        if (!data) {
            /* allocate memory sufficient for the base miplevel */
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
        
        /* All array elements are passed in a group because that is how
         * GL & Vulkan need them. Hence no
         *    for (element = 0; element < This->numArrayElements ...)
         */
        for (face = 0; face < This->header.numberOfFaces; ++face)
        {
            errorCode = This->stream.read(&This->stream, data,
                                          faceLodSizeRounded);
            if (errorCode != KTX_SUCCESS) {
                goto cleanup;
            }
            
            /* Perform endianness conversion on texture data */
            if (This->header.endianness == KTX_ENDIAN_REF_REV) {
                if (This->header.glTypeSize == 2)
                    _ktxSwapEndian16((ktx_uint16_t*)data, faceLodSize / 2);
                else if (This->header.glTypeSize == 4)
                    _ktxSwapEndian32((ktx_uint32_t*)data, faceLodSize / 4);
            }
            
            errorCode = imageCb(miplevel, face,
                                width, height, depth, layers,
                                faceLodSize, data, userdata);

            if (errorCode != KTX_SUCCESS)
                goto cleanup;
            
        }
    }
    
cleanup:
    free(data);

    if (errorCode == KTX_SUCCESS)
        This->state = KTX_RS_IMAGES_READ;

    return errorCode;
}

/*
 * The KTX format does not provide the total size of the image
 * data, so we provide the following functions to calculate it.
 */

static size_t
levelSize(const GlFormatSize* formatSize, ktx_uint32_t level,
          ktx_uint32_t width, ktx_uint32_t height, ktx_uint32_t depth)
{
    struct blockCount {
    	ktx_uint32_t x, y, z;
    } blockCount;
    ktx_uint32_t levelSizeX;

    blockCount.x = MAX(1, (width / formatSize->blockWidth)  >> level);
    blockCount.y = MAX(1, (height / formatSize->blockHeight)  >> level);
    blockCount.z = MAX(1, (depth / formatSize->blockDepth)  >> level);

    levelSizeX = (formatSize->blockSizeInBits / 8) * blockCount.x;
    if (!(formatSize->flags & GL_FORMAT_SIZE_COMPRESSED_BIT)) {
        ktx_uint32_t rowRounding;
        // Round to KTX_GL_UNPACK_ALIGNMENT. levelSizeX is the packed no. of
        // bytes in a row since formatInfo.block{Width.Height,Depth} is 1 for
        // uncompressed.
        // Equivalent to UNPACK_ALIGNMENT * ceil((groupSize * pixelWidth) / UNPACK_ALIGNMENT)
        rowRounding = 3 - ((levelSizeX + KTX_GL_UNPACK_ALIGNMENT-1) % KTX_GL_UNPACK_ALIGNMENT);
        levelSizeX += rowRounding;
    }
    return levelSizeX * blockCount.y * blockCount.z;
}

static inline size_t
layerSize(const GlFormatSize* formatSize, ktx_uint32_t levels,
          ktx_uint32_t width, ktx_uint32_t height, ktx_uint32_t depth)
{
    size_t layerSize = 0;

    // The size of a face is the sum of the size of each level.
    for(ktx_uint32_t level = 0; level <= levels; level++)
        layerSize += levelSize(formatSize, level, width, height, depth);

    return layerSize;
}

static inline size_t
dataSize(const GlFormatSize* formatSize, ktx_uint32_t levels, ktx_uint32_t layers,
         ktx_uint32_t width, ktx_uint32_t height, ktx_uint32_t depth)
{
	size_t ls = layerSize(formatSize, levels, width, height, depth);
    return (ls * layers) + formatSize->paletteSizeInBits / 8;
}

/**
 * @~English
 * @brief Return the number of bytes needed to store all the images in
 *        a blob of KTX data.
 *
 * This information is not stored in the KTX data so it must be calculated.
 *
 * @param [in] reader		  handle of the ktxReader opened on the data.
 * @param [in,out] pDataSize  pointer to a @c size_t variable to which the
 *                            size is written.
 *
 * @return	KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_OPERATION the header or key-value data of the file
 *                                  associated with the @p reader has not yet
 *                                  been read.
 * @exception KTX_INVALID_VALUE     @p reader is @c NULL or not a valid reader
 *                                  handle.
 */
KTX_error_code
ktxReader_getDataSize(KTX_reader reader, size_t* pDataSize)
{
    ktxReader* This = (ktxReader*)reader;
	GlFormatSize formatSize;
	KTX_header* header;
	ktx_uint32_t layers;

    if (This == NULL || !This->stream.read || !This->stream.skip)
        return KTX_INVALID_VALUE;

    if (This->state == KTX_RS_START)
    	// XXX Consider reading the header instead of erroring.
        return KTX_INVALID_OPERATION;

    header = &This->header;
    if (header->glFormat == header->glInternalFormat)
    	// It's an old unsized texture.
    	glGetFormatSizeFromType(header->glFormat, header->glType, &formatSize);
    else
    	glGetFormatSize(header->glInternalFormat, &formatSize);

    /* XXX Consider setting this and isArray, etc. in check header to
     * avoid multiple instances of similar checks. */
    layers = header->numberOfArrayElements == 0 ?
    		 	 	 	 	 	 	 	 	 1 : header->numberOfArrayElements;
    layers *= header->numberOfFaces;

    *pDataSize = dataSize(&formatSize,
                          header->numberOfMipmapLevels,
                          layers,
                          header->pixelWidth,
                          header->pixelHeight,
                          header->pixelDepth);
  return KTX_SUCCESS;
}

/**
 * @~English
 * @brief Return the number of bytes needed to store all the images in
 *        the specified mip level of a blob of KTX data.
 *
 * This information is not stored in the KTX data so it must be calculated.
 *
 * @param [in] reader		   handle of the ktxReader opened on the data.
 * @param [in] level           mip level for which the size is to be calculated.
 * @param [in,out] pLevelSize  pointer to a @c size_t variable to which the
 *                             size is written.
 *
 * @return	KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_OPERATION the header or key-value data of the file
 *                                  associated with the @p reader has not yet
 *                                  been read.
 * @exception KTX_INVALID_VALUE     @p reader is @c NULL or not a valid reader
 *                                  handle.
 */
KTX_error_code
ktxReader_getLevelSize(KTX_reader reader, ktx_uint32_t level, size_t* pLevelSize)
{
    ktxReader* This = (ktxReader*)reader;
	KTX_header* header;
    GlFormatSize formatSize;

    if (This == NULL || !This->stream.read || !This->stream.skip)
      return KTX_INVALID_VALUE;
    
    if (This->state == KTX_RS_START)
      // XXX Consider reading the header instead of erroring.
      return KTX_INVALID_OPERATION;
    
    header = &This->header;
    if (header->glFormat == header->glInternalFormat)
    	// It's an old unsized texture.
    	glGetFormatSizeFromType(header->glFormat, header->glType, &formatSize);
    else
    	glGetFormatSize(header->glInternalFormat, &formatSize);

    *pLevelSize = levelSize(&formatSize,
                            level,
                            header->pixelWidth,
                            header->pixelHeight,
                            header->pixelDepth);
  return KTX_SUCCESS;
}

/** @} */



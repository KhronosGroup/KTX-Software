/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2019 Khronos Group, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @internal
 * @file texture2.c
 * @~English
 *
 * @brief ktxTexture1 implementation. Support for KTX format.
 *
 * @author Mark Callow, www.edgewise-consulting.com
 */

#include <stdlib.h>

#include "dfdutils/dfd.h"
#include "ktx.h"
#include "ktxint.h"
#include "stream.h"
#include "filestream.h"
#include "memstream.h"
#include "texture.h"
#include "uthash.h"
#include "gl_format.h"

typedef struct ktxTexture1_private {
   ktx_uint32_t _glTypeSize;
   ktx_bool_t   _needSwap;
} ktxTexture1_private;

struct ktxTexture_vtbl ktxTexture1_vtbl = {
    (PFNDESTROY)ktxTexture1_Destroy,
    (PFNGLUPLOAD)ktxTexture1_GLUpload,
    (PFNITERATELEVELFACES)ktxTexture1_IterateLevelFaces,
    (PFNITERATELOADLEVELFACES)ktxTexture1_IterateLoadLevelFaces
};

static KTX_error_code
ktxTexture1_constructBase(ktxTexture1* This)
{
    assert(This != NULL);

    This->_private = (ktxTexture1_private*)malloc(sizeof(ktxTexture1_private));
    if (This->_private == NULL) {
        return KTX_OUT_OF_MEMORY;
    }
    return KTX_SUCCESS;
}

static KTX_error_code
ktxTexture1_construct(ktxTexture1* This, ktxTextureCreateInfo* createInfo,
                      ktxTextureCreateStorageEnum storageAllocation)
{
    DECLARE_SUPER(ktxTexture);
    ktxTexture1_private* private;
    ktxFormatSize formatSize;
    GLuint typeSize;
    GLenum glFormat;
    KTX_error_code result;

    This->glInternalformat = createInfo->glInternalformat;
    glGetFormatSize(This->glInternalformat, &formatSize);
    if (formatSize.blockSizeInBits == 0)
        return KTX_INVALID_VALUE; // TODO Return a more reasonable error?

    glFormat= glGetFormatFromInternalFormat(createInfo->glInternalformat);
    if (glFormat == GL_INVALID_VALUE) {
            result = KTX_INVALID_VALUE;
            goto cleanup;
    }
    result =  ktxTexture_construct(&This->super, createInfo, &formatSize,
                                   storageAllocation);
    super->vtbl = (struct ktxTexture_vtbl*)&ktxTexture1_vtbl;
    super->classId = ktxTexture1_c;
    ktxTexture1_constructBase(This);
    private = This->_private;

    super->isCompressed
                    = (formatSize.flags & KTX_FORMAT_SIZE_COMPRESSED_BIT);
    if (super->isCompressed) {
        This->glFormat = 0;
        This->glBaseInternalformat = glFormat;
        This->glType = 0;
        private->_glTypeSize = 0;
    } else {
        This->glBaseInternalformat = This->glFormat = glFormat;
        This->glType
                = glGetTypeFromInternalFormat(createInfo->glInternalformat);
        if (This->glType == GL_INVALID_VALUE) {
            result = KTX_INVALID_VALUE;
            goto cleanup;
        }
        typeSize = glGetTypeSizeFromType(This->glType);
        assert(typeSize != GL_INVALID_VALUE);

        /* Do some sanity checking */
        if (typeSize != 1 &&
            typeSize != 2 &&
            typeSize != 4)
        {
            /* Only 8, 16, and 32-bit types are supported for byte-swapping.
             * See UNPACK_SWAP_BYTES & table 8.4 in the OpenGL 4.4 spec.
             */
            result = KTX_INVALID_VALUE;
            goto cleanup;
        }
        private->_glTypeSize = typeSize;
    }

    if (storageAllocation == KTX_TEXTURE_CREATE_ALLOC_STORAGE) {
        super->dataSize
                    = ktxTexture_calcDataSizeTexture(super,
                                                     KTX_FORMAT_VERSION_ONE);
        super->pData = malloc(super->dataSize);
        if (super->pData == NULL)
            return KTX_OUT_OF_MEMORY;
    }
    return result;

cleanup:
    ktxTexture1_destruct(This);
    ktxTexture_destruct(super);
    return result;
}

/**
 * @memberof ktxTexture1 @private
 * @brief Construct a ktxTexture1 from a ktxStream reading from a KTX source.
 *
 * The KTX header, that must have been read prior to calling this, is passed
 * to the function.
 *
 * The stream object is copied into the constructed ktxTexture1.
 *
 * The create flag KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT should not be set,
 * if the ktxTexture1 is ultimately to be uploaded to OpenGL or Vulkan. This
 * will minimize memory usage by allowing, for example, loading the images
 * directly from the source into a Vulkan staging buffer.
 *
 * The create flag KTX_TEXTURE_CREATE_RAW_KVDATA_BIT should not be used. It is
 * provided solely to enable implementation of the @e libktx v1 API on top of
 * ktxTexture1.
 *
 * @param[in] This pointer to a ktxTexture1-sized block of memory to
 *                 initialize.
 * @param[in] pStream pointer to the stream to read.
 * @param[in] pHeader pointer to a KTX header that has already been read from
 *            the stream.
 * @param[in] createFlags bitmask requesting specific actions during creation.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_FILE_DATA_ERROR
 *                              Source data is inconsistent with the KTX
 *                              specification.
 * @exception KTX_FILE_READ_ERROR
 *                              An error occurred while reading the source.
 * @exception KTX_FILE_UNEXPECTED_EOF
 *                              Not enough data in the source.
 * @exception KTX_OUT_OF_MEMORY Not enough memory to load either the images or
 *                              the key-value data.
 * @exception KTX_UNKNOWN_FILE_FORMAT
 *                              The source is not in KTX format.
 * @exception KTX_UNSUPPORTED_TEXTURE_TYPE
 *                              The source describes a texture type not
 *                              supported by OpenGL or Vulkan, e.g, a 3D array.
 */
KTX_error_code
ktxTexture1_constructFromStreamAndHeader(ktxTexture1* This, ktxStream* pStream,
                                          KTX_header* pHeader,
                                          ktxTextureCreateFlags createFlags)
{
    DECLARE_SUPER(ktxTexture);
    ktxTexture1_private* private;
    KTX_error_code result;
    KTX_supplemental_info suppInfo;
    ktxStream stream;
    ktx_off_t pos;
    ktx_size_t size;
    ktxFormatSize formatSize;

    assert(pHeader != NULL && pStream != NULL);

    ktxTexture1_constructBase(This);
    ktxTexture_constructFromStream(&This->super, pStream,createFlags);
    super->vtbl = (struct ktxTexture_vtbl*)&ktxTexture1_vtbl;
    super->classId = ktxTexture1_c;

    private = This->_private;
    stream = *ktxTexture1_getStream(This);

    result = ktxCheckHeader1_(pHeader, &suppInfo);
    if (result != KTX_SUCCESS)
        goto cleanup;

    /*
     * Initialize from pHeader info.
     */
    This->glFormat = pHeader->glFormat;
    This->glInternalformat = pHeader->glInternalformat;
    This->glType = pHeader->glType;
    glGetFormatSize(This->glInternalformat, &formatSize);
    This->glBaseInternalformat = pHeader->glBaseInternalformat;
    // Can these be done by a ktxTexture_constructFromStream?
    super->numDimensions = suppInfo.textureDimension;
    super->baseWidth = pHeader->pixelWidth;
    assert(suppInfo.textureDimension > 0 && suppInfo.textureDimension < 4);
    switch (suppInfo.textureDimension) {
      case 1:
        super->baseHeight = super->baseDepth = 1;
        break;
      case 2:
        super->baseHeight = pHeader->pixelHeight;
        super->baseDepth = 1;
        break;
      case 3:
        super->baseHeight = pHeader->pixelHeight;
        super->baseDepth = pHeader->pixelDepth;
        break;
    }
    if (pHeader->numberOfArrayElements > 0) {
        super->numLayers = pHeader->numberOfArrayElements;
        super->isArray = KTX_TRUE;
    } else {
        super->numLayers = 1;
        super->isArray = KTX_FALSE;
    }
    super->numFaces = pHeader->numberOfFaces;
    if (pHeader->numberOfFaces == 6)
        super->isCubemap = KTX_TRUE;
    else
        super->isCubemap = KTX_FALSE;
    super->numLevels = pHeader->numberOfMipLevels;
    super->isCompressed = suppInfo.compressed;
    super->generateMipmaps = suppInfo.generateMipmaps;
    if (pHeader->endianness == KTX_ENDIAN_REF_REV)
        private->_needSwap = KTX_TRUE;
    private->_glTypeSize = pHeader->glTypeSize;

    /*
     * Make an empty hash list.
     */
    ktxHashList_Construct(&super->kvDataHead);
    /*
     * Load KVData.
     */
    if (pHeader->bytesOfKeyValueData > 0) {
        if (!(createFlags & KTX_TEXTURE_CREATE_SKIP_KVDATA_BIT)) {
            ktx_uint32_t kvdLen = pHeader->bytesOfKeyValueData;
            ktx_uint8_t* pKvd;

            pKvd = malloc(kvdLen);
            if (pKvd == NULL) {
                result = KTX_OUT_OF_MEMORY;
                goto cleanup;
            }

            result = stream.read(&stream, pKvd, kvdLen);
            if (result != KTX_SUCCESS)
                goto cleanup;

            if (private->_needSwap) {
                /* Swap the counts inside the key & value data. */
                ktx_uint8_t* src = pKvd;
                ktx_uint8_t* end = pKvd + kvdLen;
                while (src < end) {
                    ktx_uint32_t keyAndValueByteSize = *((ktx_uint32_t*)src);
                    _ktxSwapEndian32(&keyAndValueByteSize, 1);
                    src += _KTX_PAD4(keyAndValueByteSize);
                }
            }

            if (!(createFlags & KTX_TEXTURE_CREATE_RAW_KVDATA_BIT)) {
                result = ktxHashList_Deserialize(&super->kvDataHead,
                                                 kvdLen, pKvd);
                if (result != KTX_SUCCESS) {
                    free(pKvd);
                    goto cleanup;
                }
            } else {
                super->kvDataLen = kvdLen;
                super->kvData = pKvd;
            }
        } else {
            stream.skip(&stream, pHeader->bytesOfKeyValueData);
        }
    }

    /*
     * Get the size of the image data.
     */
    result = stream.getsize(&stream, &size);
    if (result != KTX_SUCCESS)
        goto cleanup;

    result = stream.getpos(&stream, &pos);
    if (result != KTX_SUCCESS)
        goto cleanup;

                                 /* Remove space for faceLodSize fields */
    super->dataSize = size - pos - super->numLevels * sizeof(ktx_uint32_t);

    /*
     * Load the images, if requested.
     */
    if (createFlags & KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT) {
        result = ktxTexture_LoadImageData((ktxTexture*)super, NULL, 0);
    }
    if (result == KTX_SUCCESS)
        return result;

cleanup:
    ktxTexture1_destruct(This);
    ktxTexture_destruct(super);
    return result;
}

/**
 * @memberof ktxTexture1 @private
 * @brief Construct a ktxTexture1 from a ktxStream reading from a KTX source.
 *
 * The stream object is copied into the constructed ktxTexture1.
 *
 * The create flag KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT should not be set,
 * if the ktxTexture1 is ultimately to be uploaded to OpenGL or Vulkan. This
 * will minimize memory usage by allowing, for example, loading the images
 * directly from the source into a Vulkan staging buffer.
 *
 * The create flag KTX_TEXTURE_CREATE_RAW_KVDATA_BIT should not be used. It is
 * provided solely to enable implementation of the @e libktx v1 API on top of
 * ktxTexture1.
 *
 * @param[in] This pointer to a ktxTexture1-sized block of memory to
 *            initialize.
 * @param[in] pStream pointer to the stream to read.
 * @param[in] createFlags bitmask requesting specific actions during creation.
 *
 * @return    KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_FILE_READ_ERROR
 *                              An error occurred while reading the source.
 *
 * For other exceptions see ktxTexture1_constructFromStreamAndHeader().
 */
static KTX_error_code
ktxTexture1_constructFromStream(ktxTexture1* This, ktxStream* pStream,
                                ktxTextureCreateFlags createFlags)
{
    KTX_header header;
    KTX_error_code result;

    // Read header.
    result = pStream->read(pStream, &header, KTX_HEADER_SIZE);
    if (result != KTX_SUCCESS)
        return result;

    return ktxTexture1_constructFromStreamAndHeader(This, pStream,
                                                    &header, createFlags);
}

/**
 * @memberof ktxTexture1 @private
 * @brief Construct a ktxTexture1 from a stdio stream reading from a KTX source.
 *
 * See ktxTextureInt_constructFromStream for details.
 *
 * @note Do not close the stdio stream until you are finished with the texture
 *       object.
 *
 * @param[in] This pointer to a ktxTextureInt-sized block of memory to
 *                 initialize.
 * @param[in] stdioStream a stdio FILE pointer opened on the source.
 * @param[in] createFlags bitmask requesting specific actions during creation.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE Either @p stdiostream or @p This is null.
 *
 * For other exceptions, see ktxTexture_constructFromStream().
 */
static KTX_error_code
ktxTexture1_constructFromStdioStream(ktxTexture1* This, FILE* stdioStream,
                                     ktxTextureCreateFlags createFlags)
{
    ktxStream stream;
    KTX_error_code result;

    if (stdioStream == NULL || This == NULL)
        return KTX_INVALID_VALUE;

    memset(This, 0, sizeof(*This));

    result = ktxFileStream_construct(&stream, stdioStream, KTX_FALSE);
    if (result == KTX_SUCCESS)
        result = ktxTexture1_constructFromStream(This, &stream, createFlags);
    return result;
}

/**
 * @memberof ktxTexture1 @private
 * @brief Construct a ktxTexture1 from a named KTX file.
 *
 * See ktxTextureInt_constructFromStream for details.
 *
 * @param[in] This pointer to a ktxTextureInt-sized block of memory to
 *                 initialize.
 * @param[in] filename    pointer to a char array containing the file name.
 * @param[in] createFlags bitmask requesting specific actions during creation.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_FILE_OPEN_FAILED The file could not be opened.
 * @exception KTX_INVALID_VALUE @p filename is @c NULL.
 *
 * For other exceptions, see ktxTexture_constructFromStream().
 */
static KTX_error_code
ktxTexture1_constructFromNamedFile(ktxTexture1* This,
                                   const char* const filename,
                                   ktxTextureCreateFlags createFlags)
{
    FILE* file;
    ktxStream stream;
    KTX_error_code result;

    if (This == NULL || filename == NULL)
        return KTX_INVALID_VALUE;

    memset(This, 0, sizeof(*This));

    file = fopen(filename, "rb");
    if (!file)
       return KTX_FILE_OPEN_FAILED;

    result = ktxFileStream_construct(&stream, file, KTX_TRUE);
    if (result == KTX_SUCCESS)
        result = ktxTexture1_constructFromStream(This, &stream, createFlags);

    return result;
}

/**
 * @memberof ktxTexture1 @private
 * @brief Construct a ktxTexture1 from KTX-formatted data in memory.
 *
 * See ktxTextureInt_constructFromStream for details.
 *
 * @param[in] This  pointer to a ktxTextureInt-sized block of memory to
 *                  initialize.
 * @param[in] bytes pointer to the memory containing the serialized KTX data.
 * @param[in] size  length of the KTX data in bytes.
 * @param[in] createFlags bitmask requesting specific actions during creation.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE Either @p bytes is NULL or @p size is 0.
 *
 * For other exceptions, see ktxTexture_constructFromStream().
 */
static KTX_error_code
ktxTexture1_constructFromMemory(ktxTexture1* This,
                                  const ktx_uint8_t* bytes, ktx_size_t size,
                                  ktxTextureCreateFlags createFlags)
{
    ktxStream stream;
    KTX_error_code result;

    if (bytes == NULL || size == 0)
        return KTX_INVALID_VALUE;

    memset(This, 0, sizeof(*This));

    result = ktxMemStream_construct_ro(&stream, bytes, size);
    if (result == KTX_SUCCESS)
        result = ktxTexture1_constructFromStream(This, &stream, createFlags);

    return result;
}

void
ktxTexture1_destruct(ktxTexture1* This)
{
    free(This->_private);
    ktxTexture_destruct(&This->super);
}

/**
 * @memberof ktxTexture1
 * @ingroup writer
 * @brief Create a new empty ktxTexture1.
 *
 * The address of the newly created ktxTexture1 is written to the location
 * pointed at by @p newTex.
 *
 * @param[in] createInfo pointer to a ktxTextureCreateInfo struct with
 *                       information describing the texture.
 * @param[in] storageAllocation
 *                       enum indicating whether or not to allocate storage
 *                       for the texture images.
 * @param[in,out] newTex pointer to a location in which store the address of
 *                       the newly created texture.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @c glInternalFormat in @p createInfo is not a
 *                              valid OpenGL internal format value.
 * @exception KTX_INVALID_VALUE @c numDimensions in @p createInfo is not 1, 2
 *                              or 3.
 * @exception KTX_INVALID_VALUE One of <tt>base{Width,Height,Depth}</tt> in
 *                              @p createInfo is 0.
 * @exception KTX_INVALID_VALUE @c numFaces in @p createInfo is not 1 or 6.
 * @exception KTX_INVALID_VALUE @c numLevels in @p createInfo is 0.
 * @exception KTX_INVALID_OPERATION
 *                              The <tt>base{Width,Height,Depth}</tt> specified
 *                              in @p createInfo are inconsistent with
 *                              @c numDimensions.
 * @exception KTX_INVALID_OPERATION
 *                              @p createInfo is requesting a 3D array or
 *                              3D cubemap texture.
 * @exception KTX_INVALID_OPERATION
 *                              @p createInfo is requesting a cubemap with
 *                              non-square or non-2D images.
 * @exception KTX_INVALID_OPERATION
 *                              @p createInfo is requesting more mip levels
 *                              than needed for the specified
 *                              <tt>base{Width,Height,Depth}</tt>.
 * @exception KTX_OUT_OF_MEMORY Not enough memory for the texture's images.
 */
KTX_error_code
ktxTexture1_Create(ktxTextureCreateInfo* createInfo,
                  ktxTextureCreateStorageEnum storageAllocation,
                  ktxTexture1** newTex)
{
    KTX_error_code result;

    if (newTex == NULL)
        return KTX_INVALID_VALUE;

    ktxTexture1* tex = (ktxTexture1*)malloc(sizeof(ktxTexture1));
    if (tex == NULL)
        return KTX_OUT_OF_MEMORY;

    result = ktxTexture1_construct(tex, createInfo, storageAllocation);
    if (result != KTX_SUCCESS) {
        free(tex);
    } else {
        *newTex = tex;
    }
    return result;
}

/**
 * @memberof ktxTexture1
 * @~English
 * @brief Create a ktxTexture1 from a stdio stream reading from a KTX source.
 *
 * The address of a newly created ktxTexture1 reflecting the contents of the
 * stdio stream is written to the location pointed at by @p newTex.
 *
 * The create flag KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT should not be set,
 * if the ktxTexture1 is ultimately to be uploaded to OpenGL or Vulkan. This
 * will minimize memory usage by allowing, for example, loading the images
 * directly from the source into a Vulkan staging buffer.
 *
 * The create flag KTX_TEXTURE_CREATE_RAW_KVDATA_BIT should not be used. It is
 * provided solely to enable implementation of the @e libktx v1 API on top of
 * ktxTexture1.
 *
 * @param[in] stdioStream stdio FILE pointer created from the desired file.
 * @param[in] createFlags bitmask requesting specific actions during creation.
 * @param[in,out] newTex  pointer to a location in which store the address of
 *                        the newly created texture.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p newTex is @c NULL.
 * @exception KTX_FILE_DATA_ERROR
 *                              Source data is inconsistent with the KTX
 *                              specification.
 * @exception KTX_FILE_READ_ERROR
 *                              An error occurred while reading the source.
 * @exception KTX_FILE_UNEXPECTED_EOF
 *                              Not enough data in the source.
 * @exception KTX_OUT_OF_MEMORY Not enough memory to create the texture object,
 *                              load the images or load the key-value data.
 * @exception KTX_UNKNOWN_FILE_FORMAT
 *                              The source is not in KTX format.
 * @exception KTX_UNSUPPORTED_TEXTURE_TYPE
 *                              The source describes a texture type not
 *                              supported by OpenGL or Vulkan, e.g, a 3D array.
 */
KTX_error_code
ktxTexture1_CreateFromStdioStream(FILE* stdioStream,
                                  ktxTextureCreateFlags createFlags,
                                  ktxTexture1** newTex)
{
    KTX_error_code result;
    if (newTex == NULL)
        return KTX_INVALID_VALUE;

    ktxTexture1* tex = (ktxTexture1*)malloc(sizeof(ktxTexture1));
    if (tex == NULL)
        return KTX_OUT_OF_MEMORY;

    result = ktxTexture1_constructFromStdioStream(tex, stdioStream,
                                                  createFlags);
    if (result == KTX_SUCCESS)
        *newTex = (ktxTexture1*)tex;
    else {
        free(tex);
        *newTex = NULL;
    }
    return result;
}

/* FIXME: try @copydoc and @copydetails for these functions. Does it copy
   the args or just the text? */
/**
 * @memberof ktxTexture1
 * @~English
 * @brief Create a ktxTexture1 from a named KTX file.
 *
 * The address of a newly created ktxTexture1 reflecting the contents of the
 * file is written to the location pointed at by @p newTex.
 *
 * The create flag KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT should not be set,
 * if the ktxTexture1 is ultimately to be uploaded to OpenGL or Vulkan. This
 * will minimize memory usage by allowing, for example, loading the images
 * directly from the source into a Vulkan staging buffer.
 *
 * The create flag KTX_TEXTURE_CREATE_RAW_KVDATA_BIT should not be used. It is
 * provided solely to enable implementation of the @e libktx v1 API on top of
 * ktxTexture1.
 *
 * @param[in] filename    pointer to a char array containing the file name.
 * @param[in] createFlags bitmask requesting specific actions during creation.
 * @param[in,out] newTex  pointer to a location in which store the address of
 *                        the newly created texture.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.

 * @exception KTX_FILE_OPEN_FAILED The file could not be opened.
 * @exception KTX_INVALID_VALUE @p filename is @c NULL.
 *
 * For other exceptions, see ktxTexture_CreateFromStdioStream().
 */
KTX_error_code
ktxTexture1_CreateFromNamedFile(const char* const filename,
                                ktxTextureCreateFlags createFlags,
                                ktxTexture1** newTex)
{
    KTX_error_code result;

    if (newTex == NULL)
        return KTX_INVALID_VALUE;

    ktxTexture1* tex = (ktxTexture1*)malloc(sizeof(ktxTexture1));
    if (tex == NULL)
        return KTX_OUT_OF_MEMORY;

    result = ktxTexture1_constructFromNamedFile(tex, filename, createFlags);
    if (result == KTX_SUCCESS)
        *newTex = (ktxTexture1*)tex;
    else {
        free(tex);
        *newTex = NULL;
    }
    return result;
}

/**
 * @memberof ktxTexture1
 * @~English
 * @brief Create a ktxTexture1 from KTX-formatted data in memory.
 *
 * The address of a newly created ktxTexture1 reflecting the contents of the
 * serialized KTX data is written to the location pointed at by @p newTex.
 *
 * The create flag KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT should not be set,
 * if the ktxTexture1 is ultimately to be uploaded to OpenGL or Vulkan. This
 * will minimize memory usage by allowing, for example, loading the images
 * directly from the source into a Vulkan staging buffer.
 *
 * The create flag KTX_TEXTURE_CREATE_RAW_KVDATA_BIT should not be used. It is
 * provided solely to enable implementation of the @e libktx v1 API on top of
 * ktxTexture1.
 *
 * @param[in] bytes pointer to the memory containing the serialized KTX data.
 * @param[in] size  length of the KTX data in bytes.
 * @param[in] createFlags bitmask requesting specific actions during creation.
 * @param[in,out] newTex  pointer to a location in which store the address of
 *                        the newly created texture.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE Either @p bytes is NULL or @p size is 0.
 *
 * For other exceptions, see ktxTexture_CreateFromStdioStream().
 */
KTX_error_code
ktxTexture1_CreateFromMemory(const ktx_uint8_t* bytes, ktx_size_t size,
                             ktxTextureCreateFlags createFlags,
                             ktxTexture1** newTex)
{
    KTX_error_code result;
    if (newTex == NULL)
        return KTX_INVALID_VALUE;

    ktxTexture1* tex = (ktxTexture1*)malloc(sizeof(ktxTexture1));
    if (tex == NULL)
        return KTX_OUT_OF_MEMORY;

    result = ktxTexture1_constructFromMemory(tex, bytes, size,
                                             createFlags);
    if (result == KTX_SUCCESS)
        *newTex = (ktxTexture1*)tex;
    else {
        free(tex);
        *newTex = NULL;
    }
    return result;
}

/**
 * @memberof ktxTexture1
 * @~English
 * @brief Destroy a ktxTexture1 object.
 *
 * This frees the memory associated with the texture contents and the memory
 * of the ktxTexture1 object. This does @e not delete any OpenGL or Vulkan
 * texture objects created by ktxTexture1_GLUpload or ktxTexture1_VkUpload.
 *
 * @param[in] This pointer to the ktxTexture1 object to destroy
 */
void
ktxTexture1_Destroy(ktxTexture1* This)
{
    ktxTexture1_destruct(This);
    free(This);
}

/**
 * @memberof ktxTexture1 @private
 * @~English
 * @brief Return the size of the primitive type of a single color component
 *
 * @param[in]     This       pointer to the ktxTexture1 object of interest.
 *
 * @return the type size in bytes.
 */
ktx_uint32_t
ktxTexture1_glTypeSize(ktxTexture1* This)
{
    assert(This != NULL);
    return This->_private->_glTypeSize;
}

/**
 * @memberof ktxTexture1
 * @~English
 * @brief Iterate over the images in a ktxTexture1 object.
 *
 * Blocks of image data are passed to an application-supplied callback
 * function. This is not a strict per-image iteration. Rather it reflects how
 * OpenGL needs the images. For most textures the block of data includes all
 * images of a mip level which implies all layers of an array. However, for
 * non-array cube map textures the block is a single face of the mip level,
 * i.e the callback is called once for each face.
 *
 * This function works even if @p This->pData == 0 so it can be used to
 * obtain offsets and sizes for each level by callers who have loaded the data
 * externally.
 *
 * @param[in]     This      pointer to the ktxTexture1 object of interest.
 * @param[in,out] iterCb    the address of a callback function which is called
 *                          with the data for each image block.
 * @param[in,out] userdata  the address of application-specific data which is
 *                          passed to the callback along with the image data.
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error. The
 *          following are returned directly by this function. @p iterCb may
 *          return these for other causes or may return additional errors.
 *
 * @exception KTX_FILE_DATA_ERROR   Mip level sizes are increasing not
 *                                  decreasing
 * @exception KTX_INVALID_VALUE     @p This is @c NULL or @p iterCb is @c NULL.
 *
 */
KTX_error_code
ktxTexture1_IterateLevelFaces(ktxTexture1* This, PFNKTXITERCB iterCb,
                              void* userdata)
{
    DECLARE_SUPER(ktxTexture);
    ktx_uint32_t    miplevel;
    KTX_error_code  result = KTX_SUCCESS;

    if (This == NULL)
        return KTX_INVALID_VALUE;

    if (super->classId != ktxTexture1_c)
        return KTX_INVALID_OPERATION;

    if (iterCb == NULL)
        return KTX_INVALID_VALUE;

    for (miplevel = 0; miplevel < super->numLevels; ++miplevel)
    {
        ktx_uint32_t faceLodSize;
        ktx_uint32_t face;
        ktx_uint32_t innerIterations;
        GLsizei      width, height, depth;

        /* Array textures have the same number of layers at each mip level. */
        width = MAX(1, super->baseWidth  >> miplevel);
        height = MAX(1, super->baseHeight >> miplevel);
        depth = MAX(1, super->baseDepth  >> miplevel);

        faceLodSize = (ktx_uint32_t)ktxTexture_calcFaceLodSize(
                                                    super, miplevel,
                                                    KTX_FORMAT_VERSION_ONE);

        /* All array layers are passed in a group because that is how
         * GL & Vulkan need them. Hence no
         *    for (layer = 0; layer < This->numLayers)
         */
        if (super->isCubemap && !super->isArray)
            innerIterations = super->numFaces;
        else
            innerIterations = 1;
        for (face = 0; face < innerIterations; ++face)
        {
            /* And all z_slices are also passed as a group hence no
             *    for (slice = 0; slice < This->depth)
             */
            ktx_size_t offset;

            ktxTexture_GetImageOffset(super, miplevel, 0, face, &offset);
            result = iterCb(miplevel, face,
                             width, height, depth,
                             faceLodSize, super->pData + offset, userdata);

            if (result != KTX_SUCCESS)
                break;
        }
    }

    return result;
}

/**
 * @memberof ktxTexture1
 * @~English
 * @brief Iterate over the images in a ktxTexture1 object while loading the
 *        image data.
 *
 * This operates similarly to ktxTexture_IterateLevelFaces() except that it
 * loads the images from the ktxTexture1's source to a temporary buffer
 * while iterating. The callback function must copy the image data if it
 * wishes to preserve it as the temporary buffer is reused for each level and
 * is freed when this function exits.
 *
 * This function is helpful for reducing memory usage when uploading the data
 * to a graphics API.
 *
 * @param[in]     This     pointer to the ktxTexture1 object of interest.
 * @param[in,out] iterCb   the address of a callback function which is called
 *                         with the data for each image.
 * @param[in,out] userdata the address of application-specific data which is
 *                         passed to the callback along with the image data.
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error. The
 *          following are returned directly by this function. @p iterCb may
 *          return these for other causes or may return additional errors.
 *
 * @exception KTX_FILE_DATA_ERROR   mip level sizes are increasing not
 *                                  decreasing
 * @exception KTX_INVALID_OPERATION the ktxTexture1 was not created from a
 *                                  stream, i.e there is no data to load, or
 *                                  this ktxTexture1's images have already
 *                                  been loaded.
 * @exception KTX_INVALID_VALUE     @p This is @c NULL or @p iterCb is @c NULL.
 * @exception KTX_OUT_OF_MEMORY     not enough memory to allocate a block to
 *                                  hold the base level image.
 */
KTX_error_code
ktxTexture1_IterateLoadLevelFaces(ktxTexture1* This, PFNKTXITERCB iterCb,
                                  void* userdata)
{
    DECLARE_SUPER(ktxTexture);
    DECLARE_PRIVATE(ktxTexture1);
    struct ktxTexture_protected* prtctd = super->_protected;
    ktxStream stream = *(ktxStream *)&prtctd->_stream;
    ktx_uint32_t    dataSize = 0;
    ktx_uint32_t    miplevel;
    KTX_error_code  result = KTX_SUCCESS;
    void*           data = NULL;

    if (This == NULL)
        return KTX_INVALID_VALUE;

    if (super->classId != ktxTexture1_c)
        return KTX_INVALID_OPERATION;

    if (iterCb == NULL)
        return KTX_INVALID_VALUE;

    if (prtctd->_stream.data.file == NULL)
        // This Texture not created from a stream or images are already loaded.
        return KTX_INVALID_OPERATION;

    for (miplevel = 0; miplevel < super->numLevels; ++miplevel)
    {
        ktx_uint32_t faceLodSize;
        ktx_uint32_t faceLodSizePadded;
        ktx_uint32_t face;
        ktx_uint32_t innerIterations;
        GLsizei      width, height, depth;

        /* Array textures have the same number of layers at each mip level. */
        width = MAX(1, super->baseWidth  >> miplevel);
        height = MAX(1, super->baseHeight >> miplevel);
        depth = MAX(1, super->baseDepth  >> miplevel);

        result = stream.read(&stream, &faceLodSize, sizeof(ktx_uint32_t));
        if (result != KTX_SUCCESS) {
            goto cleanup;
        }
        if (private->_needSwap) {
            _ktxSwapEndian32(&faceLodSize, 1);
        }
#if (KTX_GL_UNPACK_ALIGNMENT != 4)
        faceLodSizePadded = _KTX_PAD4(faceLodSize);
#else
        faceLodSizePadded = faceLodSize;
#endif
        if (!data) {
            /* allocate memory sufficient for the base miplevel */
            data = malloc(faceLodSizePadded);
            if (!data) {
                result = KTX_OUT_OF_MEMORY;
                goto cleanup;
            }
            dataSize = faceLodSizePadded;
        }
        else if (dataSize < faceLodSizePadded) {
            /* subsequent miplevels cannot be larger than the base miplevel */
            result = KTX_FILE_DATA_ERROR;
            goto cleanup;
        }

        /* All array layers are passed in a group because that is how
         * GL & Vulkan need them. Hence no
         *    for (layer = 0; layer < This->numLayers)
         */
        if (super->isCubemap && !super->isArray)
            innerIterations = super->numFaces;
        else
            innerIterations = 1;
        for (face = 0; face < innerIterations; ++face)
        {
            /* And all z_slices are also passed as a group hence no
             *    for (z_slice = 0; z_slice < This->depth)
             */
            result = stream.read(&stream, data, faceLodSizePadded);
            if (result != KTX_SUCCESS) {
                goto cleanup;
            }

            /* Perform endianness conversion on texture data */
            if (private->_needSwap) {
                if (private->_glTypeSize == 2)
                    _ktxSwapEndian16((ktx_uint16_t*)data, faceLodSize / 2);
                else if (private->_glTypeSize == 4)
                    _ktxSwapEndian32((ktx_uint32_t*)data, faceLodSize / 4);
            }

            result = iterCb(miplevel, face,
                             width, height, depth,
                             faceLodSize, data, userdata);

            if (result != KTX_SUCCESS)
                goto cleanup;
        }
    }

cleanup:
    free(data);
    // No further need for this.
    stream.destruct(&stream);

    return result;
}

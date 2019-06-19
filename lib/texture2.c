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
 * @brief ktxTexture2 implementation. Support for KTX2 format.
 *
 * @author Mark Callow, www.edgewise-consulting.com
 */

#include <stdlib.h>

#include "dfd.h"
#include "ktx.h"
#include "ktxint.h"
#include "stream.h"
#include "filestream.h"
#include "memstream.h"
#include "texture.h"
#include "uthash.h"
#include "vk_format.h"
#include "vkformat_enum.h"

static KTX_error_code
ktxTexture2_construct(ktxTexture2* This, ktxTextureCreateInfo* createInfo,
                        ktxTextureCreateStorageEnum storageAllocation)
{
    DECLARE_SUPER(ktxTexture);
    ktxFormatSize formatSize;
    KTX_error_code result;

    if (createInfo->vkFormat != VK_FORMAT_UNDEFINED) {
        vkGetFormatSize(createInfo->vkFormat, &formatSize);
        if (formatSize.blockSizeInBits == 0) {
            return KTX_INVALID_VALUE; // TODO Return a more reasonable error?
        } else {
            This->pDfd = createDFD4VkFormat(createInfo->vkFormat);
        }
    } else {
        // TODO Validate createInfo->pDfd and create formatSize from it.
    }
    result =  ktxTexture_construct(&This->super, createInfo, &formatSize,
                                   storageAllocation);

    if (result == KTX_SUCCESS) {
        This->vkFormat = createInfo->vkFormat;
        This->supercompressionScheme = KTX_SUPERCOMPRESSION_NONE;

        if (storageAllocation == KTX_TEXTURE_CREATE_ALLOC_STORAGE) {
            super->dataSize
                        = ktxTexture_calcDataSizeTexture(super,
                                                         KTX_FORMAT_VERSION_ONE);
            super->pData = malloc(super->dataSize);
            if (super->pData == NULL) {
                ktxTexture_destruct(super);
                return KTX_OUT_OF_MEMORY;
            }
        }
    }
    return result;
}

/**
 * @memberof ktxTexture2 @private
 * @brief Construct a ktxTexture from a ktxStream reading from a KTX source.
 *
 * The KTX header, that must have been read prior to calling this, is passed
 * to the function.
 *
 * The stream object is copied into the constructed ktxTexture2.
 *
 * The create flag KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT should not be set,
 * if the ktxTexture is ultimately to be uploaded to OpenGL or Vulkan. This
 * will minimize memory usage by allowing, for example, loading the images
 * directly from the source into a Vulkan staging buffer.
 *
 * The create flag KTX_TEXTURE_CREATE_RAW_KVDATA_BIT should not be used. It is
 * provided solely to enable implementation of the @e libktx v1 API on top of
 * ktxTexture.
 *
 * @param[in] This pointer to a ktxTexture2-sized block of memory to
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
ktxTexture2_constructFromStreamAndHeader(ktxTexture2* This, ktxStream* pStream,
                                          KTX_header2* pHeader,
                                          ktxTextureCreateFlags createFlags)
{
    DECLARE_SUPER(ktxTexture);
    KTX_error_code result;
    KTX_supplemental_info suppInfo;
    ktxStream stream;
    ktxFormatSize formatSize;
#if 0
    ktx_off_t pos;
    ktx_size_t size;
#endif

    assert(pHeader != NULL && pStream != NULL);

    ktxTexture_constructFromStream(&This->super, pStream,createFlags);
    stream = *ktxTexture2_getStream(This);

    result = ktxCheckHeader2_(pHeader, &suppInfo);
    if (result != KTX_SUCCESS)
        return result;

    /*
     * Initialize from pHeader->info.
     */
    This->vkFormat = pHeader->vkFormat;
    This->supercompressionScheme = pHeader->supercompressionScheme;
    //This->pDfd =
    vkGetFormatSize(This->vkFormat, &formatSize);
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
    if (pHeader->arrayElementCount > 0) {
        super->numLayers = pHeader->arrayElementCount;
        super->isArray = KTX_TRUE;
    } else {
        super->numLayers = 1;
        super->isArray = KTX_FALSE;
    }
    super->numFaces = pHeader->faceCount;
    if (pHeader->faceCount == 6)
        super->isCubemap = KTX_TRUE;
    else
        super->isCubemap = KTX_FALSE;
    super->numLevels = pHeader->levelCount;
    super->isCompressed = suppInfo.compressed;
    super->generateMipmaps = suppInfo.generateMipmaps;

#if 0
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
            if (pKvd == NULL)
                return KTX_OUT_OF_MEMORY;

            result = stream.read(&stream, pKvd, kvdLen);
            if (result != KTX_SUCCESS)
                return result;

            if (This->_needSwap) {
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
                    return result;
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
    if (result == KTX_SUCCESS) {
        result = stream.getpos(&stream, &pos);
        if (result == KTX_SUCCESS)
            super->dataSize = size - pos
                                 /* Remove space for faceLodSize fields */
                                 - super->numLevels * sizeof(ktx_uint32_t);
    }

    /*
     * Load the images, if requested.
     */
    if (result == KTX_SUCCESS
        && (createFlags & KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT)) {
        result = ktxTexture_LoadImageData((ktxTexture*)super, NULL, 0);
    }
#endif
    return result;
}

/**
 * @memberof ktxTexture2 @private
 * @brief Construct a ktxTexture from a ktxStream reading from a KTX source.
 *
 * The stream object is copied into the constructed ktxTexture2.
 *
 * The create flag KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT should not be set,
 * if the ktxTexture is ultimately to be uploaded to OpenGL or Vulkan. This
 * will minimize memory usage by allowing, for example, loading the images
 * directly from the source into a Vulkan staging buffer.
 *
 * The create flag KTX_TEXTURE_CREATE_RAW_KVDATA_BIT should not be used. It is
 * provided solely to enable implementation of the @e libktx v1 API on top of
 * ktxTexture.
 *
 * @param[in] This pointer to a ktxTexture2-sized block of memory to
 *            initialize.
 * @param[in] pStream pointer to the stream to read.
 * @param[in] createFlags bitmask requesting specific actions during creation.
 *
 * @return    KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_FILE_READ_ERROR
 *                              An error occurred while reading the source.
 *
 * For other exceptions see ktxTexture2_constructFromStreamAndHeader().
 */
static KTX_error_code
ktxTexture2_constructFromStream(ktxTexture2* This, ktxStream* pStream,
                                ktxTextureCreateFlags createFlags)
{
    KTX_header2 header;
    KTX_error_code result;

    // Read header.
    result = pStream->read(pStream, &header, KTX2_HEADER_SIZE);
    if (result != KTX_SUCCESS)
        return result;

    return ktxTexture2_constructFromStreamAndHeader(This, pStream,
                                                    &header, createFlags);
}

/**
 * @memberof ktxTexture2 @private
 * @brief Construct a ktxTexture from a stdio stream reading from a KTX source.
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
ktxTexture2_constructFromStdioStream(ktxTexture2* This, FILE* stdioStream,
                                     ktxTextureCreateFlags createFlags)
{
    KTX_error_code result;
    ktxStream stream;

    if (stdioStream == NULL || This == NULL)
        return KTX_INVALID_VALUE;

    memset(This, 0, sizeof(*This));

    result = ktxFileStream_construct(&stream, stdioStream, KTX_FALSE);
    if (result == KTX_SUCCESS)
        result = ktxTexture2_constructFromStream(This, &stream, createFlags);
    return result;
}

/**
 * @memberof ktxTexture2 @private
 * @brief Construct a ktxTexture from a named KTX file.
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
ktxTexture2_constructFromNamedFile(ktxTexture2* This,
                                   const char* const filename,
                                   ktxTextureCreateFlags createFlags)
{
    KTX_error_code result;
    ktxStream stream;
    FILE* file;

    if (This == NULL || filename == NULL)
        return KTX_INVALID_VALUE;

    memset(This, 0, sizeof(*This));

    file = fopen(filename, "rb");
    if (!file)
       return KTX_FILE_OPEN_FAILED;

    result = ktxFileStream_construct(&stream, file, KTX_TRUE);
    if (result == KTX_SUCCESS)
        result = ktxTexture2_constructFromStream(This, &stream, createFlags);

    return result;
}

/**
 * @memberof ktxTexture2 @private
 * @brief Construct a ktxTexture from KTX-formatted data in memory.
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
ktxTexture2_constructFromMemory(ktxTexture2* This,
                                  const ktx_uint8_t* bytes, ktx_size_t size,
                                  ktxTextureCreateFlags createFlags)
{
    KTX_error_code result;
    ktxStream stream;

    if (bytes == NULL || size == 0)
        return KTX_INVALID_VALUE;

    memset(This, 0, sizeof(*This));

    result = ktxMemStream_construct_ro(&stream, bytes, size);
    if (result == KTX_SUCCESS)
        result = ktxTexture2_constructFromStream(This, &stream, createFlags);

    return result;
}

void
ktxTexture2_destruct(ktxTexture2* This)
{
    free(This->pDfd);
    ktxTexture_destruct(&This->super);
}

/**
 * @memberof ktxTexture2
 * @ingroup writer
 * @brief Create a new empty ktxTexture2.
 *
 * The address of the newly created ktxTexture2 is written to the location
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
ktxTexture2_Create(ktxTextureCreateInfo* createInfo,
                  ktxTextureCreateStorageEnum storageAllocation,
                  ktxTexture2** newTex)
{
    KTX_error_code result;

    if (newTex == NULL)
        return KTX_INVALID_VALUE;

    ktxTexture2* tex = (ktxTexture2*)malloc(sizeof(ktxTexture2));
    if (tex == NULL)
        return KTX_OUT_OF_MEMORY;

    result = ktxTexture2_construct(tex, createInfo, storageAllocation);
    if (result != KTX_SUCCESS) {
        free(tex);
    } else {
        *newTex = tex;
    }
    return result;
}

/**
 * @memberof ktxTexture2
 * @~English
 * @brief Create a ktxTexture from a stdio stream reading from a KTX source.
 *
 * The address of a newly created ktxTexture reflecting the contents of the
 * stdio stream is written to the location pointed at by @p newTex.
 *
 * The create flag KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT should not be set,
 * if the ktxTexture is ultimately to be uploaded to OpenGL or Vulkan. This
 * will minimize memory usage by allowing, for example, loading the images
 * directly from the source into a Vulkan staging buffer.
 *
 * The create flag KTX_TEXTURE_CREATE_RAW_KVDATA_BIT should not be used. It is
 * provided solely to enable implementation of the @e libktx v1 API on top of
 * ktxTexture.
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
ktxTexture2_CreateFromStdioStream(FILE* stdioStream,
                                  ktxTextureCreateFlags createFlags,
                                  ktxTexture2** newTex)
{
    KTX_error_code result;
    if (newTex == NULL)
        return KTX_INVALID_VALUE;

    ktxTexture2* tex = (ktxTexture2*)malloc(sizeof(ktxTexture2));
    if (tex == NULL)
        return KTX_OUT_OF_MEMORY;

    result = ktxTexture2_constructFromStdioStream(tex, stdioStream,
                                                  createFlags);
    if (result == KTX_SUCCESS)
        *newTex = (ktxTexture2*)tex;
    else {
        free(tex);
        *newTex = NULL;
    }
    return result;
}

/**
 * @memberof ktxTexture2
 * @~English
 * @brief Create a ktxTexture from a named KTX file.
 *
 * The address of a newly created ktxTexture reflecting the contents of the
 * file is written to the location pointed at by @p newTex.
 *
 * The create flag KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT should not be set,
 * if the ktxTexture is ultimately to be uploaded to OpenGL or Vulkan. This
 * will minimize memory usage by allowing, for example, loading the images
 * directly from the source into a Vulkan staging buffer.
 *
 * The create flag KTX_TEXTURE_CREATE_RAW_KVDATA_BIT should not be used. It is
 * provided solely to enable implementation of the @e libktx v1 API on top of
 * ktxTexture.
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
ktxTexture2_CreateFromNamedFile(const char* const filename,
                                ktxTextureCreateFlags createFlags,
                                ktxTexture2** newTex)
{
    KTX_error_code result;

    if (newTex == NULL)
        return KTX_INVALID_VALUE;

    ktxTexture2* tex = (ktxTexture2*)malloc(sizeof(ktxTexture2));
    if (tex == NULL)
        return KTX_OUT_OF_MEMORY;

    result = ktxTexture2_constructFromNamedFile(tex, filename, createFlags);
    if (result == KTX_SUCCESS)
        *newTex = (ktxTexture2*)tex;
    else {
        free(tex);
        *newTex = NULL;
    }
    return result;
}

/**
 * @memberof ktxTexture2
 * @~English
 * @brief Create a ktxTexture from KTX-formatted data in memory.
 *
 * The address of a newly created ktxTexture reflecting the contents of the
 * serialized KTX data is written to the location pointed at by @p newTex.
 *
 * The create flag KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT should not be set,
 * if the ktxTexture is ultimately to be uploaded to OpenGL or Vulkan. This
 * will minimize memory usage by allowing, for example, loading the images
 * directly from the source into a Vulkan staging buffer.
 *
 * The create flag KTX_TEXTURE_CREATE_RAW_KVDATA_BIT should not be used. It is
 * provided solely to enable implementation of the @e libktx v1 API on top of
 * ktxTexture.
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
ktxTexture2_CreateFromMemory(const ktx_uint8_t* bytes, ktx_size_t size,
                             ktxTextureCreateFlags createFlags,
                             ktxTexture2** newTex)
{
    KTX_error_code result;
    if (newTex == NULL)
        return KTX_INVALID_VALUE;

    ktxTexture2* tex = (ktxTexture2*)malloc(sizeof(ktxTexture2));
    if (tex == NULL)
        return KTX_OUT_OF_MEMORY;

    result = ktxTexture2_constructFromMemory(tex, bytes, size,
                                             createFlags);
    if (result == KTX_SUCCESS)
        *newTex = (ktxTexture2*)tex;
    else {
        free(tex);
        *newTex = NULL;
    }
    return result;
}

/**
 * @memberof ktxTexture2
 * @~English
 * @brief Destroy a ktxTexture2 object.
 *
 * This frees the memory associated with the texture contents and the memory
 * of the ktxTexture2 object. This does @e not delete any OpenGL or Vulkan
 * texture objects created by ktxTexture2_GLUpload or ktxTexture2_VkUpload.
 *
 * @param[in] This pointer to the ktxTexture2 object to destroy
 */
void
ktxTexture2_Destroy(ktxTexture2* This)
{
    ktxTexture2_destruct(This);
    free(This);
}

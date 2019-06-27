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

#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdlib.h>

#include "dfdutils/dfd.h"
#include "ktx.h"
#include "ktxint.h"
#include "stream.h"
#include "filestream.h"
#include "memstream.h"
#include "texture2.h"
#include "uthash.h"
#include "vk_format.h"

// FIXME Test this #define and put it in a header somewhere.
//#define IS_BIG_ENDIAN (1 == *(unsigned char *)&(const int){0x01000000ul})
#define IS_BIG_ENDIAN 0

typedef struct ktxTexture2_private {
   ktx_uint8_t* supercompressionGlobalData;
   ktxLevelIndexEntry _levelIndex[1]; // Must be last so it can grow.
} ktxTexture2_private;

struct ktxTexture_vtbl ktxTexture2_vtbl;
extern struct ktxTexture_vvtbl* pKtxTexture2_vvtbl;

static KTX_error_code
ktxTexture2_constructCommon(ktxTexture2* This, ktx_uint32_t numLevels)
{
    assert(This != NULL);
    ktx_size_t privateSize;

    This->classId = ktxTexture2_c;
    This->vtbl = &ktxTexture2_vtbl;
#if !KTX_OMIT_VULKAN
    This->vvtbl = pKtxTexture2_vvtbl;
#else
    This->vvtbl = NULL;
#endif
    privateSize = sizeof(ktxTexture2_private)
                + sizeof(ktxLevelIndexEntry) * (numLevels - 1);
    This->_private = (ktxTexture2_private*)malloc(privateSize);
    if (This->_private == NULL) {
        return KTX_OUT_OF_MEMORY;
    }
    memset(This->_private, 0, privateSize);
    return KTX_SUCCESS;
}


static KTX_error_code
ktxTexture2_construct(ktxTexture2* This, ktxTextureCreateInfo* createInfo,
                        ktxTextureCreateStorageEnum storageAllocation)
{
    ktxFormatSize formatSize;
    KTX_error_code result;

    memset(This, 0, sizeof(*This));

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
    result =  ktxTexture_construct(ktxTexture(This), createInfo, &formatSize,
                                   storageAllocation);
    if (result != KTX_SUCCESS)
        return result;
    result = ktxTexture2_constructCommon(This, createInfo->numLevels);
    if (result != KTX_SUCCESS)
        goto cleanup;;

    This->vkFormat = createInfo->vkFormat;
    This->supercompressionScheme = KTX_SUPERCOMPRESSION_NONE;

    if (storageAllocation == KTX_TEXTURE_CREATE_ALLOC_STORAGE) {
        This->dataSize
                = ktxTexture_calcDataSizeTexture(ktxTexture(This),
                                                 KTX_FORMAT_VERSION_TWO);
        This->pData = malloc(This->dataSize);
        if (This->pData == NULL) {
            result = KTX_OUT_OF_MEMORY;
            goto cleanup;
        }
    }
    return result;

cleanup:
    ktxTexture2_destruct(This);
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
    ktxTexture2_private* private;
    KTX_error_code result;
    KTX_supplemental_info suppInfo;
    ktxStream* stream;
    ktxFormatSize formatSize;
    ktx_size_t levelIndexSize;

    assert(pHeader != NULL && pStream != NULL);

    memset(This, 0, sizeof(*This));
    result = ktxTexture_constructFromStream(ktxTexture(This), pStream,
                                            createFlags);
    if (result != KTX_SUCCESS)
        return result;

    result = ktxCheckHeader2_(pHeader, &suppInfo);
    if (result != KTX_SUCCESS)
        goto cleanup;
    // ktxCheckHeader2_ has done the max(1, levelCount) on pHeader->levelCount.
    result = ktxTexture2_constructCommon(This, pHeader->levelCount);
    if (result != KTX_SUCCESS)
        goto cleanup;
    private = This->_private;

    stream = ktxTexture2_getStream(This);

    /*
     * Initialize from pHeader->info.
     */
    This->vkFormat = pHeader->vkFormat;
    This->supercompressionScheme = pHeader->supercompressionScheme;
    //This->pDfd =
    vkGetFormatSize(This->vkFormat, &formatSize);
    This->_protected->_typeSize = pHeader->typeSize;
    // Can these be done by a ktxTexture_constructFromStream?
    This->numDimensions = suppInfo.textureDimension;
    This->baseWidth = pHeader->pixelWidth;
    assert(suppInfo.textureDimension > 0 && suppInfo.textureDimension < 4);
    switch (suppInfo.textureDimension) {
      case 1:
        This->baseHeight = This->baseDepth = 1;
        break;
      case 2:
        This->baseHeight = pHeader->pixelHeight;
        This->baseDepth = 1;
        break;
      case 3:
        This->baseHeight = pHeader->pixelHeight;
        This->baseDepth = pHeader->pixelDepth;
        break;
    }
    if (pHeader->layerCount > 0) {
        This->numLayers = pHeader->layerCount;
        This->isArray = KTX_TRUE;
    } else {
        This->numLayers = 1;
        This->isArray = KTX_FALSE;
    }
    This->numFaces = pHeader->faceCount;
    if (pHeader->faceCount == 6)
        This->isCubemap = KTX_TRUE;
    else
        This->isCubemap = KTX_FALSE;
    // ktxCheckHeader2_ does the max(1, levelCount) and sets
    // suppInfo.generateMipmaps when it was originally 0.
    This->numLevels = pHeader->levelCount;
    This->isCompressed = suppInfo.compressed;
    This->generateMipmaps = suppInfo.generateMipmaps;

    // Read level index
    levelIndexSize = sizeof(ktxLevelIndexEntry) * This->numLevels;
    result = stream->read(stream, &private->_levelIndex, levelIndexSize);
    if (result != KTX_SUCCESS)
        goto cleanup;

    // Read DFD
    This->pDfd =
            (ktx_uint32_t*)malloc(pHeader->dataFormatDescriptor.byteLength);
    result = stream->read(stream, This->pDfd,
                          pHeader->dataFormatDescriptor.byteLength);
    if (result != KTX_SUCCESS)
        goto cleanup;

    // Make an empty hash list.
    ktxHashList_Construct(&This->kvDataHead);
    // Load KVData.
    if (pHeader->keyValueData.byteLength > 0) {
        if (!(createFlags & KTX_TEXTURE_CREATE_SKIP_KVDATA_BIT)) {
            ktx_uint32_t kvdLen = pHeader->keyValueData.byteLength;
            ktx_uint8_t* pKvd;

            pKvd = malloc(kvdLen);
            if (pKvd == NULL) {
                result = KTX_OUT_OF_MEMORY;
                goto cleanup;
            }

            result = stream->read(stream, pKvd, kvdLen);
            if (result != KTX_SUCCESS)
                goto cleanup;

            if (IS_BIG_ENDIAN) {
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
                result = ktxHashList_Deserialize(&This->kvDataHead,
                                                 kvdLen, pKvd);
                if (result != KTX_SUCCESS) {
                    free(pKvd);
                    goto cleanup;
                }
            } else {
                This->kvDataLen = kvdLen;
                This->kvData = pKvd;
            }
        } else {
            stream->skip(stream, pHeader->keyValueData.byteLength);
        }
    }

    if (pHeader->supercompressionGlobalData.byteLength > 0) {
        // There could be padding here so seek to the next item.
        (void)stream->setpos(stream,
                             pHeader->supercompressionGlobalData.byteOffset);

        // Read supercompressionGlobalData
        private->supercompressionGlobalData =
          (ktx_uint8_t*)malloc(pHeader->supercompressionGlobalData.byteLength);
        result = stream->read(stream, This->pDfd,
                          pHeader->supercompressionGlobalData.byteLength);
        if (result != KTX_SUCCESS)
            goto cleanup;
    }

    // There could be more padding here so seek to start of images.
    (void)stream->setpos(stream,
                         private->_levelIndex[This->numLevels - 1].byteOffset);

    // Calculate size of the image data.
    This->dataSize = 0;
    for (ktx_uint32_t i = 0; i < This->numLevels; i++) {
        This->dataSize += private->_levelIndex[i].byteLength;
    }

    /*
     * Load the images, if requested.
     */
    if (result == KTX_SUCCESS
        && (createFlags & KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT)) {
        result = ktxTexture2_LoadImageData(This, NULL, 0);
    }
    return result;

cleanup:
    ktxTexture2_destruct(This);
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

#if BIG_ENDIAN
    // byte swap the header
#endif
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

    result = ktxMemStream_construct_ro(&stream, bytes, size);
    if (result == KTX_SUCCESS)
        result = ktxTexture2_constructFromStream(This, &stream, createFlags);

    return result;
}

void
ktxTexture2_destruct(ktxTexture2* This)
{
    if (This->pDfd) free(This->pDfd);
    if (This->_private) {
      ktx_uint8_t* sgd = This->_private->supercompressionGlobalData;
      if (sgd) free(sgd);
      free(This->_private);
    }
    ktxTexture_destruct(ktxTexture(This));
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

/**
 * @memberof ktxTexture2
 * @~English
 * @brief Find the offset of an image within a ktxTexture's image data.
 *
 * As there is no such thing as a 3D cubemap we make the 3rd location parameter
 * do double duty.
 *
 * @param[in]     This      pointer to the ktxTexture object of interest.
 * @param[in]     level     mip level of the image.
 * @param[in]     layer     array layer of the image.
 * @param[in]     faceSlice cube map face or depth slice of the image.
 * @param[in,out] pOffset   pointer to location to store the offset.
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_OPERATION
 *                         @p level, @p layer or @p faceSlice exceed the
 *                         dimensions of the texture.
 * @exception KTX_INVALID_VALID @p This is NULL.
 */
KTX_error_code
ktxTexture2_GetImageOffset(ktxTexture2* This, ktx_uint32_t level,
                          ktx_uint32_t layer, ktx_uint32_t faceSlice,
                          ktx_size_t* pOffset)
{
    DECLARE_PRIVATE(ktxTexture2);

    if (This == NULL)
        return KTX_INVALID_VALUE;

    if (level >= This->numLevels || layer >= This->numLayers)
        return KTX_INVALID_OPERATION;

    if (This->isCubemap) {
        if (faceSlice >= This->numFaces)
            return KTX_INVALID_OPERATION;
    } else {
        ktx_uint32_t maxSlice = MAX(1, This->baseDepth >> level);
        if (faceSlice >= maxSlice)
            return KTX_INVALID_OPERATION;
    }

    // Get the offset of the start of the level.
    *pOffset = private->_levelIndex[level].byteOffset
               - private->_levelIndex[This->numLevels-1].byteOffset;

    if (This->supercompressionScheme == KTX_SUPERCOMPRESSION_NONE) {
        // All layers, faces & slices within a level are the same size.
        if (layer != 0) {
            ktx_size_t layerSize;
            layerSize = ktxTexture_layerSize(ktxTexture(This), level,
                                             KTX_FORMAT_VERSION_TWO);
            *pOffset += layer * layerSize;
        }
        if (faceSlice != 0) {
            ktx_size_t imageSize;
            imageSize = ktxTexture2_GetImageSize(This, level);
            *pOffset += faceSlice * imageSize;
        }
    } else {
        // TODO
    }
    return KTX_SUCCESS;
}


/**
 * @memberof ktxTexture2
 * @~English
 * @brief Calculate & return the size in bytes of an image at the specified
 *        mip level.
 *
 * For arrays, this is the size of layer, for cubemaps, the size of a face
 * and for 3D textures, the size of a depth slice.
 *
 * The size reflects the padding of each row to KTX_GL_UNPACK_ALIGNMENT.
 *
 * @param[in]     This     pointer to the ktxTexture object of interest.
 * @param[in]     level    level of interest. *
 */
ktx_size_t
ktxTexture2_GetImageSize(ktxTexture2* This, ktx_uint32_t level)
{
    return ktxTexture_calcImageSize(ktxTexture(This), level,
                                    KTX_FORMAT_VERSION_TWO);
}

/**
 * @memberof ktxTexture2
 * @~English
 * @brief Iterate over the images in a ktxTexture2 object.
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
 * @param[in]     This      pointer to the ktxTexture2 object of interest.
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
ktxTexture2_IterateLevelFaces(ktxTexture2* This, PFNKTXITERCB iterCb,
                              void* userdata)
{
    ktx_uint32_t    miplevel;
    KTX_error_code  result = KTX_SUCCESS;

    if (This == NULL)
        return KTX_INVALID_VALUE;

    if (This->classId != ktxTexture1_c)
        return KTX_INVALID_OPERATION;

    if (iterCb == NULL)
        return KTX_INVALID_VALUE;

    for (miplevel = 0; miplevel < This->numLevels; ++miplevel)
    {
        ktx_uint32_t faceLodSize;
        ktx_uint32_t face;
        ktx_uint32_t innerIterations;
        GLsizei      width, height, depth;

        /* Array textures have the same number of layers at each mip level. */
        width = MAX(1, This->baseWidth  >> miplevel);
        height = MAX(1, This->baseHeight >> miplevel);
        depth = MAX(1, This->baseDepth  >> miplevel);

        faceLodSize = (ktx_uint32_t)ktxTexture_calcFaceLodSize(
                                                    ktxTexture(This), miplevel,
                                                    KTX_FORMAT_VERSION_ONE);

        /* All array layers are passed in a group because that is how
         * GL & Vulkan need them. Hence no
         *    for (layer = 0; layer < This->numLayers)
         */
        if (This->isCubemap && !This->isArray)
            innerIterations = This->numFaces;
        else
            innerIterations = 1;
        for (face = 0; face < innerIterations; ++face)
        {
            /* And all z_slices are also passed as a group hence no
             *    for (slice = 0; slice < This->depth)
             */
            ktx_size_t offset;

            ktxTexture_GetImageOffset(ktxTexture(This), miplevel, 0, face, &offset);
            result = iterCb(miplevel, face,
                             width, height, depth,
                             faceLodSize, This->pData + offset, userdata);

            if (result != KTX_SUCCESS)
                break;
        }
    }

    return result;
}

/**
 * @memberof ktxTexture2
 * @~English
 * @brief Iterate over the images in a ktxTexture2 object while loading the
 *        image data.
 *
 * This operates similarly to ktxTexture_IterateLevelFaces() except that it
 * loads the images from the ktxTexture2's source to a temporary buffer
 * while iterating. The callback function must copy the image data if it
 * wishes to preserve it as the temporary buffer is reused for each level and
 * is freed when this function exits.
 *
 * This function is helpful for reducing memory usage when uploading the data
 * to a graphics API.
 *
 * @param[in]     This     pointer to the ktxTexture2 object of interest.
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
 * @exception KTX_INVALID_OPERATION the ktxTexture2 was not created from a
 *                                  stream, i.e there is no data to load, or
 *                                  this ktxTexture2's images have already
 *                                  been loaded.
 * @exception KTX_INVALID_VALUE     @p This is @c NULL or @p iterCb is @c NULL.
 * @exception KTX_OUT_OF_MEMORY     not enough memory to allocate a block to
 *                                  hold the base level image.
 */
KTX_error_code
ktxTexture2_IterateLoadLevelFaces(ktxTexture2* This, PFNKTXITERCB iterCb,
                                  void* userdata)
{
    struct ktxTexture_protected* prtctd = This->_protected;
    ktxStream* stream = (ktxStream *)&prtctd->_stream;
    ktx_uint32_t    dataSize = 0;
    ktx_uint32_t    miplevel;
    KTX_error_code  result = KTX_SUCCESS;
    void*           data = NULL;

    if (This == NULL)
        return KTX_INVALID_VALUE;

    if (This->classId != ktxTexture1_c)
        return KTX_INVALID_OPERATION;

    if (iterCb == NULL)
        return KTX_INVALID_VALUE;

    if (prtctd->_stream.data.file == NULL)
        // This Texture not created from a stream or images are already loaded.
        return KTX_INVALID_OPERATION;

    for (miplevel = 0; miplevel < This->numLevels; ++miplevel)
    {
        ktx_uint32_t faceLodSize;
        ktx_uint32_t faceLodSizePadded;
        ktx_uint32_t face;
        ktx_uint32_t innerIterations;
        GLsizei      width, height, depth;

        /* Array textures have the same number of layers at each mip level. */
        width = MAX(1, This->baseWidth  >> miplevel);
        height = MAX(1, This->baseHeight >> miplevel);
        depth = MAX(1, This->baseDepth  >> miplevel);

        result = stream->read(stream, &faceLodSize, sizeof(ktx_uint32_t));
        if (result != KTX_SUCCESS) {
            goto cleanup;
        }

        faceLodSizePadded = faceLodSize;

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
        if (This->isCubemap && !This->isArray)
            innerIterations = This->numFaces;
        else
            innerIterations = 1;
        for (face = 0; face < innerIterations; ++face)
        {
            /* And all z_slices are also passed as a group hence no
             *    for (z_slice = 0; z_slice < This->depth)
             */
            result = stream->read(stream, data, faceLodSizePadded);
            if (result != KTX_SUCCESS) {
                goto cleanup;
            }

#if IS_BIG_ENDIAN
            /* FIXME Perform endianness conversion on texture data */
#endif

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
    stream->destruct(stream);

    return result;
}

/**
 * @memberof ktxTexture1
 * @~English
 * @brief Load all the image data from the ktxTexture's source.
 *
 * The data is loaded into the provided buffer or to an internally allocated
 * buffer, if @p pBuffer is @c NULL.
 *
 * @param[in] This pointer to the ktxTexture object of interest.
 * @param[in] pBuffer pointer to the buffer in which to load the image data.
 * @param[in] bufSize size of the buffer pointed at by @p pBuffer.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p This is NULL.
 * @exception KTX_INVALID_VALUE @p bufSize is less than the the image data size.
 * @exception KTX_INVALID_OPERATION
 *                              The data has already been loaded or the
 *                              ktxTexture was not created from a KTX source.
 * @exception KTX_OUT_OF_MEMORY Insufficient memory for the image data.
 */
KTX_error_code
ktxTexture2_LoadImageData(ktxTexture2* This,
                          ktx_uint8_t* pBuffer, ktx_size_t bufSize)
{
    DECLARE_PROTECTED(ktxTexture);
    DECLARE_PRIVATE(ktxTexture2);
    ktx_uint8_t*    pDest;
    KTX_error_code  result = KTX_SUCCESS;

    if (This == NULL)
        return KTX_INVALID_VALUE;

    if (prtctd->_stream.data.file == NULL)
        // This Texture not created from a stream or images already loaded;
        return KTX_INVALID_OPERATION;

    if (pBuffer == NULL) {
        This->pData = malloc(This->dataSize);
        if (This->pData == NULL)
            return KTX_OUT_OF_MEMORY;
        pDest = This->pData;
    } else if (bufSize < This->dataSize) {
        return KTX_INVALID_VALUE;
    } else {
        pDest = pBuffer;
    }

    result = prtctd->_stream.read(&prtctd->_stream, pDest,
                                  This->dataSize);

    if (BIG_ENDIAN) {
        // Perform endianness conversion on texture data.
        // To avoid mip padding, need to convert each level individually.
        for (ktx_uint32_t level = 0; level < This->numLevels; ++level)
        {
            ktx_size_t levelOffset;
            ktx_size_t levelByteLength;

            levelByteLength = private->_levelIndex[level].byteLength;
            ktxTexture2_GetImageOffset(This, level, 0, 0, &levelOffset);
            pDest = This->pData + levelOffset;
            switch (prtctd->_typeSize) {
              case 2:
                _ktxSwapEndian16((ktx_uint16_t*)pDest, levelByteLength / 2);
                break;
              case 4:
                _ktxSwapEndian32((ktx_uint32_t*)pDest, levelByteLength / 4);
                break;
              case 8:
                _ktxSwapEndian64((ktx_uint64_t*)pDest, levelByteLength / 8);
                break;
            }
        }
    }

cleanup:
    // No further need for this.
    prtctd->_stream.destruct(&prtctd->_stream);
    return result;
}

/*
 * Initialized here at the end to avoid the need for multiple declarations of
 * these functions.
 */
struct ktxTexture_vtbl ktxTexture2_vtbl = {
    (PFNKTEXDESTROY)ktxTexture2_Destroy,
    (PFNKTEXGETIMAGEOFFSET)ktxTexture2_GetImageOffset,
    (PFNKTEXGETIMAGESIZE)ktxTexture2_GetImageSize,
    (PFNKTEXGLUPLOAD)ktxTexture2_GLUpload,
    (PFNKTEXITERATELEVELFACES)ktxTexture2_IterateLevelFaces,
    (PFNKTEXITERATELOADLEVELFACES)ktxTexture2_IterateLoadLevelFaces,
    (PFNKTEXLOADIMAGEDATA)ktxTexture2_LoadImageData,
    (PFNKTEXSETIMAGEFROMMEMORY)ktxTexture2_SetImageFromMemory,
    (PFNKTEXSETIMAGEFROMSTDIOSTREAM)ktxTexture2_SetImageFromStdioStream,
    (PFNKTEXWRITETOSTDIOSTREAM)ktxTexture2_WriteToStdioStream,
    (PFNKTEXWRITETONAMEDFILE)ktxTexture2_WriteToNamedFile,
    (PFNKTEXWRITETOMEMORY)ktxTexture2_WriteToMemory,
};

/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

#ifndef KTX_H_A55A6F00956F42F3A137C11929827FE1
#define KTX_H_A55A6F00956F42F3A137C11929827FE1

/*
 * ©2010-2018 The Khronos Group, Inc.
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
 *
 * See the accompanying LICENSE.md for licensing details for all files in
 * the KTX library and KTX loader tests.
 */

/**
 * @file
 * @~English
 *
 * @brief Declares the public functions and structures of the
 *        KTX API.
 *
 * @author Mark Callow, Edgewise Consulting and while at HI Corporation
 * @author Based on original work by Georg Kolling, Imagination Technology
 *
 * @version 3.0
 *
 * @todo Find a way so that applications do not have to define KTX_OPENGL{,_ES*}
 *       when using the library.
 */

#include <stdio.h>
#include <stdbool.h>

/* To avoid including <KHR/khrplatform.h> define our own types. */
typedef unsigned char ktx_uint8_t;
typedef bool ktx_bool_t;
#ifdef _MSC_VER
typedef unsigned __int16 ktx_uint16_t;
typedef   signed __int16 ktx_int16_t;
typedef unsigned __int32 ktx_uint32_t;
typedef   signed __int32 ktx_int32_t;
typedef          size_t  ktx_size_t;
typedef unsigned __int64 ktx_uint64_t;
typedef   signed __int64 ktx_int64_t;
#else
#include <stdint.h>
typedef uint16_t ktx_uint16_t;
typedef  int16_t ktx_int16_t;
typedef uint32_t ktx_uint32_t;
typedef  int32_t ktx_int32_t;
typedef   size_t ktx_size_t;
typedef uint64_t ktx_uint64_t;
typedef  int64_t ktx_int64_t;
#endif

/* This will cause compilation to fail if size of uint32 != 4. */
typedef unsigned char ktx_uint32_t_SIZE_ASSERT[sizeof(ktx_uint32_t) == 4];

/*
 * This #if allows libktx to be compiled with strict c99. It avoids
 * compiler warnings or even errors when a gl.h is already included.
 * "Redefinition of (type) is a c11 feature". Obviously this doesn't help if
 * gl.h comes after. However nobody has complained about the unguarded typedefs
 * since they were introduced so this is unlikely to be a problem in practice.
 * Presumably everybody is using platform default compilers not c99 or else
 * they are using C++.
 */
#if !defined(GL_NO_ERROR)
  /*
   * To avoid having to including gl.h ...
   */
  typedef unsigned char GLboolean;
  typedef unsigned int GLenum;
  typedef int GLint;
  typedef int GLsizei;
  typedef unsigned int GLuint;
  typedef unsigned char GLubyte;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @~English
 * @brief Key String for standard orientation value.
 */
#define KTX_ORIENTATION_KEY "KTXorientation"
/**
 * @~English
 * @brief Key String for standard writer value.
 */
#define KTX_WRITER_KEY "KTXwriter"
/**
 * @~English
 * @brief Standard KTX 1 format for 1D orientation value.
 */
#define KTX_ORIENTATION1_FMT "S=%c"
/**
 * @~English
 * @brief Standard KTX 1 format for 2D orientation value.
 */
#define KTX_ORIENTATION2_FMT "S=%c,T=%c"
/**
 * @~English
 * @brief Standard KTX 1 format for 3D orientation value.
 */
#define KTX_ORIENTATION3_FMT "S=%c,T=%c,R=%c"
/**
 * @~English
 * @brief Required unpack alignment
 */
#define KTX_GL_UNPACK_ALIGNMENT 4

#define KTX_TRUE  true
#define KTX_FALSE false

/**
 * @~English
 * @brief Error codes returned by library functions.
 */
typedef enum KTX_error_code_t {
    KTX_SUCCESS = 0,         /*!< Operation was successful. */
    KTX_FILE_DATA_ERROR,     /*!< The data in the file is inconsistent with the spec. */
    KTX_FILE_ISPIPE,         /*!< The file is a pipe or named pipe. */
    KTX_FILE_OPEN_FAILED,    /*!< The target file could not be opened. */
    KTX_FILE_OVERFLOW,       /*!< The operation would exceed the max file size. */
    KTX_FILE_READ_ERROR,     /*!< An error occurred while reading from the file. */
    KTX_FILE_SEEK_ERROR,     /*!< An error occurred while seeking in the file. */
    KTX_FILE_UNEXPECTED_EOF, /*!< File does not have enough data to satisfy request. */
    KTX_FILE_WRITE_ERROR,    /*!< An error occurred while writing to the file. */
    KTX_GL_ERROR,            /*!< GL operations resulted in an error. */
    KTX_INVALID_OPERATION,   /*!< The operation is not allowed in the current state. */
    KTX_INVALID_VALUE,       /*!< A parameter value was not valid */
    KTX_NOT_FOUND,           /*!< Requested key was not found */
    KTX_OUT_OF_MEMORY,       /*!< Not enough memory to complete the operation. */
    KTX_UNKNOWN_FILE_FORMAT, /*!< The file not a KTX file */
    KTX_UNSUPPORTED_TEXTURE_TYPE, /*!< The KTX file specifies an unsupported texture type. */
} KTX_error_code;

#define KTX_IDENTIFIER_REF  { 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A }
#define KTX_ENDIAN_REF      (0x04030201)
#define KTX_ENDIAN_REF_REV  (0x01020304)
#define KTX_HEADER_SIZE     (64)

/**
 * @~English
 * @brief Result codes returned by library functions.
 */
 typedef enum KTX_error_code_t ktxResult;

/**
 * @class ktxHashList
 * @~English
 * @brief Opaque handle to a ktxHashList.
 */
typedef struct ktxKVListEntry* ktxHashList;

#define KTXAPIENTRY
#define KTXAPIENTRYP KTXAPIENTRY *
/**
 * @class ktxHashListEntry
 * @~English
 * @brief Opaque handle to an entry in a @ref ktxHashList.
 */
typedef struct ktxKVListEntry ktxHashListEntry;

typedef enum ktxOrientationX {
    KTX_ORIENT_X_LEFT = 0, KTX_ORIENT_X_RIGHT = 1
} ktxOrientationX;

typedef enum ktxOrientationY {
    KTX_ORIENT_Y_UP = 0, KTX_ORIENT_Y_DOWN = 1
} ktxOrientationY;

typedef enum ktxOrientationZ {
    KTX_ORIENT_Z_IN = 1, KTX_ORIENT_Z_OUT = 2
} ktxOrientationZ;

typedef enum class_id {
    ktxTexture1_c = 1,
    ktxTexture2_c = 2
} class_id;

#define KTXTEXTURECLASSDEFN                   \
    class_id classId;                         \
    struct ktxTexture_vtbl* vtbl;             \
    struct ktxTexture_vvtbl* vvtbl;           \
    struct ktxTexture_protected* _protected;  \
    ktx_bool_t   isArray;                     \
    ktx_bool_t   isCubemap;                   \
    ktx_bool_t   isCompressed;                \
    ktx_bool_t   generateMipmaps;             \
    ktx_uint32_t baseWidth;                   \
    ktx_uint32_t baseHeight;                  \
    ktx_uint32_t baseDepth;                   \
    ktx_uint32_t numDimensions;               \
    ktx_uint32_t numLevels;                   \
    ktx_uint32_t numLayers;                   \
    ktx_uint32_t numFaces;                    \
    struct {                                  \
        ktxOrientationX x;                    \
        ktxOrientationY y;                    \
        ktxOrientationZ z;                    \
    } orientation;                            \
    ktxHashList  kvDataHead;                  \
    ktx_uint32_t kvDataLen;                   \
    ktx_uint8_t* kvData;                      \
    ktx_size_t dataSize;                      \
    ktx_uint8_t* pData;


/**
 * @class ktxTexture
 * @~English
 * @brief Base class representing a texture.
 *
 * ktxTextures should be created only by one of the provided
 * functions and these fields should be considered read-only.
 */
typedef struct ktxTexture {
    KTXTEXTURECLASSDEFN
} ktxTexture;
/**
 * @typedef ktxTexture::classId
 * @~English
 * @brief Identify the class type.
 *
 * Since there are no public ktxTexture constructors, this can only have
 * values of ktxTexture1_c or ktxTexture2_c.
 */
/**
 * @typedef ktxTexture::vtbl
 * @~English
 * @brief Pointer to the class's vtble.
 */
/**
 * @typedef ktxTexture::vvtbl
 * @~English
 * @brief Pointer to the class's vtble for Vulkan functions.
 *
 * A separate vtble is used so this header does not need to include vulkan.h.
 */
/**
 * @typedef ktxTexture::_protected
 * @~English
 * @brief Opaque pointer to the class's protected variables.
 */
/**
 * @typedef ktxTexture::isArray
 * @~English
 *
 * KTX_TRUE if the texture is an array texture, i.e,
 * a GL_TEXTURE_*_ARRAY target is to be used.
 */
/**
 * @typedef ktxTexture::isCubemap
 * @~English
 *
 * KTX_TRUE if the texture is a cubemap or cubemap array.
 */
/**
 * @typedef ktxTexture::isCubemap
 * @~English
 *
 * KTX_TRUE if the texture's format is a block compressed format.
 */
/**
 * @typedef ktxTexture::generateMipmaps
 * @~English
 *
 * KTX_TRUE if mipmaps should be generated for the texture by
 * ktxTexture_GLUpload() or ktxTexture_VkUpload().
 */
/**
 * @typedef ktxTexture::baseWidth
 * @~English
 * @brief Width of the texture's base level.
 */
/**
 * @typedef ktxTexture::baseHeight
 * @~English
 * @brief Height of the texture's base level.
 */
/**
 * @typedef ktxTexture::baseDepth
 * @~English
 * @brief Depth of the texture's base level.
 */
/**
 * @typedef ktxTexture::numDimensions
 * @~English
 * @brief Number of dimensions in the texture: 1, 2 or 3.
 */
/**
 * @typedef ktxTexture::numLevels
 * @~English
 * @brief Number of mip levels in the texture.
 *
 * Must be 1, if @c generateMipmaps is KTX_TRUE. Can be less than a
 * full pyramid but always starts at the base level.
 */
/**
 * @typedef ktxTexture::numLevels
 * @~English
 * @brief Number of array layers in the texture.
 */
/**
 * @typedef ktxTexture::numFaces
 * @~English
 * @brief Number of faces: 6 for cube maps, 1 otherwise.
 */
/**
 * @typedef ktxTexture::orientation
 * @~English
 * @brief Describes the logical orientation of the images in each dimension.
 *
 * ktxOrientationX for X, ktxOrientationY for Y and ktxOrientationZ for Z.
 */
/**
 * @typedef ktxTexture::kvDataHead
 * @~English
 * @brief Head of the hash list of metadata.
 */
/**
 * @typedef ktxTexture::kvDataLen
 * @~English
 * @brief Length of the metadata, if it has been extracted in its raw form,
 *       otherwise 0.
 */
/**
 * @typedef ktxTexture::kvData
 * @~English
 * @brief Pointer to the metadata, if it has been extracted in its raw form,
 *       otherwise NULL.
 */
/**
 * @typedef ktxTexture::dataSize
 * @~English
 * @brief Byte length of the texture's uncompressed image data.
 */
/**
 * @typedef ktxTexture::pData
 * @~English
 * @brief Pointer to the start of the image data.
 */

/**
 * @memberof ktxTexture
 * @~English
 * @brief Signature of function called by the <tt>ktxTexture_Iterate*</tt>
 *        functions to receive image data.
 *
 * The function parameters are used to pass values which change for each image.
 * Obtain values which are uniform across all images from the @c ktxTexture
 * object.
 *
 * @param [in] miplevel        MIP level from 0 to the max level which is
 *                             dependent on the texture size.
 * @param [in] face            usually 0; for cube maps, one of the 6 cube
 *                             faces in the order +X, -X, +Y, -Y, +Z, -Z,
 *                             0 to 5.
 * @param [in] width           width of the image.
 * @param [in] height          height of the image or, for 1D textures
 *                             textures, 1.
 * @param [in] depth           depth of the image or, for 1D & 2D
 *                             textures, 1.
 * @param [in] faceLodSize     number of bytes of data pointed at by
 *                             @p pixels.
 * @param [in] pixels          pointer to the image data.
 * @param [in,out] userdata    pointer for the application to pass data to and
 *                             from the callback function.
 */
/* Don't use KTXAPIENTRYP to avoid a Doxygen bug. */
typedef KTX_error_code
    (KTXAPIENTRY* PFNKTXITERCB)(int miplevel, int face,
                                int width, int height, int depth,
                                ktx_uint32_t faceLodSize,
                                void* pixels, void* userdata);

typedef void (KTXAPIENTRY* PFNKTEXDESTROY)(ktxTexture* This);
typedef KTX_error_code
    (KTXAPIENTRY* PFNKTEXGETIMAGEOFFSET)(ktxTexture* This, ktx_uint32_t level,
                                         ktx_uint32_t layer,
                                         ktx_uint32_t faceSlice,
                                         ktx_size_t* pOffset);
typedef ktx_size_t
(KTXAPIENTRY* PFNKTEXGETIMAGESIZE)(ktxTexture* This, ktx_uint32_t level);
typedef KTX_error_code
    (KTXAPIENTRY* PFNKTEXGLUPLOAD)(ktxTexture* This,
                                   GLuint* pTexture, GLenum* pTarget,
                                   GLenum* pGlerror);
typedef KTX_error_code
    (KTXAPIENTRY* PFNKTEXITERATELEVELFACES)(ktxTexture* This,
                                            PFNKTXITERCB iterCb,
                                            void* userdata);
typedef KTX_error_code
    (KTXAPIENTRY* PFNKTEXITERATELOADLEVELFACES)(ktxTexture* This,
                                                PFNKTXITERCB iterCb,
                                                void* userdata);
typedef KTX_error_code
    (KTXAPIENTRY* PFNKTEXLOADIMAGEDATA)(ktxTexture* This,
                                        ktx_uint8_t* pBuffer,
                                        ktx_size_t bufSize);
typedef KTX_error_code
    (KTXAPIENTRY* PFNKTEXSETIMAGEFROMMEMORY)(ktxTexture* This,
                                             ktx_uint32_t level,
                                             ktx_uint32_t layer,
                                             ktx_uint32_t faceSlice,
                                             const ktx_uint8_t* src,
                                             ktx_size_t srcSize);

typedef KTX_error_code
    (KTXAPIENTRY* PFNKTEXSETIMAGEFROMSTDIOSTREAM)(ktxTexture* This,
                                                  ktx_uint32_t level,
                                                  ktx_uint32_t layer,
                                                  ktx_uint32_t faceSlice,
                                                  FILE* src, ktx_size_t srcSize);
typedef KTX_error_code
    (KTXAPIENTRY* PFNKTEXWRITETOSTDIOSTREAM)(ktxTexture* This, FILE* dstsstr);
typedef KTX_error_code
    (KTXAPIENTRY* PFNKTEXWRITETONAMEDFILE)(ktxTexture* This,
                                           const char* const dstname);
typedef KTX_error_code
    (KTXAPIENTRY* PFNKTEXWRITETOMEMORY)(ktxTexture* This,
                                        ktx_uint8_t** bytes, ktx_size_t* size);

/**
 * @class ktxTexture
 * @~English
 * @brief Table of virtual ktxTexture's functions.
 */
 struct ktxTexture_vtbl {
    PFNKTEXDESTROY Destroy;
    PFNKTEXGETIMAGEOFFSET GetImageOffset;
    PFNKTEXGETIMAGESIZE GetImageSize;
    PFNKTEXGLUPLOAD GLUpload;
    PFNKTEXITERATELEVELFACES IterateLevelFaces;
    PFNKTEXITERATELOADLEVELFACES IterateLoadLevelFaces;
    PFNKTEXLOADIMAGEDATA LoadImageData;
    PFNKTEXSETIMAGEFROMMEMORY SetImageFromMemory;
    PFNKTEXSETIMAGEFROMSTDIOSTREAM SetImageFromStdioStream;
    PFNKTEXWRITETOSTDIOSTREAM WriteToStdioStream;
    PFNKTEXWRITETONAMEDFILE WriteToNamedFile;
    PFNKTEXWRITETOMEMORY WriteToMemory;
};

/****************************************************************
 * Macros to give some backward compatibility to the previous API
 ****************************************************************/

#define ktxTexture_Destroy(obj) obj->vtbl->Destroy(obj)

/*
 * Returns the offset of the image for the specified mip level, array layer
 * and face or depth slice within the image data of a ktxTexture object.
 */
#define ktxTexture_GetImageOffset(obj, a, b, c, d) \
                            obj->vtbl->GetImageOffset(obj, a, b, c, d)

#define ktxTexture_GetImageSize(obj, a) obj->vtbl->GetImageSize(obj, a)

/*
 * Uploads the image data from a ktxTexture object to an OpenGL {,ES} texture
 * object.
 */
#define ktxTexture_GLUpload(obj, a, b, c) obj->vtbl->GLUpload(obj, a, b, c)

/*
 * Iterates over the already loaded level-faces in a ktxTexture object.
 * iterCb is called for each level-face.
 */
 #define ktxTexture_IterateLevelFaces(obj, a, b) \
                            obj->vtbl->IterateLevelFaces(obj, a, b)

/*
 * Iterates over the level-faces of a ktxTexture object, loading each from
 * the KTX-formatted source then calling iterCb.
 */
 #define ktxTexture_IterateLoadLevelFaces(obj, a, b) \
                    obj->vtbl->IterateLoadLevelFaces(obj, a, b)

/*
 * Loads the image data into a ktxTexture object from the KTX-formatted source.
 * Used when the image data was not loaded during ktxTexture_CreateFrom*.
 */
#define ktxTexture_LoadImageData(This, pBuffer, bufSize) \
                    This->vtbl->LoadImageData(This, pBuffer, bufSize)

/*
 * Sets the image for the specified level, layer & faceSlice within a
 * ktxTexture object from packed image data in memory. The destination image
 * data is padded to the KTX specified row alignment of 4, if necessary.
 */
#define ktxTexture_SetImageFromMemory(This, level, layer, faceSlice, \
                                      src, srcSize)                  \
    This->vtbl->SetImageFromMemory(This, level, layer, faceSlice, src, srcSize)

/*
 * Sets the image for the specified level, layer & faceSlice within a
 * ktxTexture object from a stdio stream reading from a KTX source. The
 * destination image data is padded to the KTX specified row alignment of 4,
 * if necessary.
 */
#define ktxTexture_SetImageFromStdioStream(This, level, layer, faceSlice, \
                                           src, srcSize)                  \
    This->vtbl->SetImageFromStdioStream(This, level, layer, faceSlice,    \
                                        src, srcSize)

/*
 * Write a ktxTexture object to a stdio stream in KTX format.
 */
#define ktxTexture_WriteToStdioStream(This, dstsstr) \
                                This->vtbl->WriteToStdioStream(This, dstsstr)

/*
 * Write a ktxTexture object to a named file in KTX format.
 */
#define ktxTexture_WriteToNamedFile(This, dstname) \
                                This->vtbl->WriteToNamedfile(This, dstname)

/*
 * Write a ktxTexture object to a block of memory in KTX format.
 */
#define ktxTexture_WriteToMemory(This, bytes, size) \
                                This->vtbl->WritetoMemory(This, bytes, size)


/*===========================================================*
 * KTX format version 2                                      *
 *===========================================================*/

/**
 * @class ktxTexture1
 * @~English
 * @brief Class representing a KTX version 1 format texture.
 *
 * ktxTextures should be created only by one of the ktxTexture_Create*
 * functions and these fields should be considered read-only.
 */
typedef struct ktxTexture1 {
    KTXTEXTURECLASSDEFN
    ktx_uint32_t glFormat; /*!< Format of the texture data, e.g., GL_RGB. */
    ktx_uint32_t glInternalformat; /*!< Internal format of the texture data,
                                       e.g., GL_RGB8. */
    ktx_uint32_t glBaseInternalformat; /*!< Base format of the texture data,
                                           e.g., GL_RGB. */
    ktx_uint32_t glType; /*!< Type of the texture data, e.g, GL_UNSIGNED_BYTE.*/
    struct ktxTexture1_private* _private;
} ktxTexture1;

/**
 * @~English
 * @brief Enum identifying supercompression scheme.
 */

typedef enum ktxSupercmpScheme {
    KTX_SUPERCOMPRESSION_NONE = 0,  /*!< No supercompression. */
    KTX_SUPERCOMPRESSION_BASIS = 1,  /*!< Basis Universal supercompression. */
    KTX_SUPERCOMPRESSION_LZMA = 2,  /*!< LZMA supercompression. */
    KTX_SUPERCOMPRESSION_ZLIB = 2,  /*!< Zlib supercompression. */
    KTX_SUPERCOMPRESSION_ZSTD = 3,  /*!< ZStd supercompression. */
    KTX_SUPERCOMPRESSION_BEGIN_RANGE = KTX_SUPERCOMPRESSION_NONE,
    KTX_SUPERCOMPRESSION_END_RANGE = KTX_SUPERCOMPRESSION_ZSTD
} ktxSupercmpScheme;

/**
 * @class ktxTexture2
 * @~English
 * @brief Class representing a KTX version 2 format texture.
 *
 * ktxTextures should be created only by one of the ktxTexture_Create*
 * functions and these fields should be considered read-only.
 */
typedef struct ktxTexture2 {
    KTXTEXTURECLASSDEFN
    ktx_uint32_t  vkFormat;
    ktx_uint32_t* pDfd;
    ktxSupercmpScheme supercompressionScheme;
} ktxTexture2;

#define ktxTexture(t) ((ktxTexture*)t)

/**
 * @memberof ktxTexture
 * @~English
 * @brief Structure for passing texture information to ktxTexture[12]_Create().
 *
 * @sa ktxTexture_Create()
 */
typedef struct
{
    ktx_uint32_t glInternalformat; /*!< Internal format for the texture, e.g.,
                                        GL_RGB8. Ignored when creating a
                                        ktxTexture2. */
    ktx_uint32_t vkFormat;   /*!< VkFormat for texture. Ignored when creating a
                                  ktxTexture1. */
    ktx_uint32_t* pDfd;      /*!< Pointer to DFD. Used only when creating a
                                  ktxTexture2 and only if vkFormat is
                                  VK_FORMAT_UNDEFINED. */
    ktx_uint32_t baseWidth;  /*!< Width of the base level of the texture. */
    ktx_uint32_t baseHeight; /*!< Height of the base level of the texture. */
    ktx_uint32_t baseDepth;  /*!< Depth of the base level of the texture. */
    ktx_uint32_t numDimensions; /*!< Number of dimensions in the texture, 1, 2
                                     or 3. */
    ktx_uint32_t numLevels; /*!< Number of mip levels in the texture. Should be
                                 1 if @c generateMipmaps is KTX_TRUE; */
    ktx_uint32_t numLayers; /*!< Number of array layers in the texture. */
    ktx_uint32_t numFaces;  /*!< Number of faces: 6 for cube maps, 1 otherwise. */
    ktx_bool_t   isArray;  /*!< Set to KTX_TRUE if the texture is to be an
                                array texture. Means OpenGL will use a
                                GL_TEXTURE_*_ARRAY target. */
    ktx_bool_t   generateMipmaps; /*!< Set to KTX_TRUE if mipmaps should be
                                       generated for the texture when loading
                                       into a 3D API. */
} ktxTextureCreateInfo;

/**
 * @memberof ktxTexture
 * @~English
 * @brief Enum for requesting, or not, allocation of storage for images.
 *
 * @sa ktxTexture_Create()
 */
typedef enum {
    KTX_TEXTURE_CREATE_NO_STORAGE = 0,  /*!< Don't allocate any image storage. */
    KTX_TEXTURE_CREATE_ALLOC_STORAGE = 1 /*!< Allocate image storage. */
} ktxTextureCreateStorageEnum;

/**
 * @memberof ktxTexture
 * @~English
 * @brief Flags for requesting services during creation.
 *
 * @sa ktxTexture_CreateFrom*
 */
enum ktxTextureCreateFlagBits {
    KTX_TEXTURE_CREATE_NO_FLAGS = 0x00,
    KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT = 0x01,
                                   /*!< Load the images from the KTX source. */
    KTX_TEXTURE_CREATE_RAW_KVDATA_BIT = 0x02,
                                   /*!< Load the raw key-value data instead of
                                        creating a @c ktxHashList from it. */
    KTX_TEXTURE_CREATE_SKIP_KVDATA_BIT = 0x04
                                   /*!< Skip any key-value data. This overrides
                                        the RAW_KVDATA_BIT. */
};
/**
 * @memberof ktxTexture
 * @~English
 * @brief Type for TextureCreateFlags parameters.
 *
 * @sa ktxTexture_CreateFrom*()
 */
typedef ktx_uint32_t ktxTextureCreateFlags;

/*
 * See the implementation files for the full documentation of the following
 * functions.
 */

/*
 * These three create a ktxTexture1 or ktxTexture2 according to the header in
 * the data, and return a pointer to the base ktxTexture class.
 */
KTX_error_code
ktxTexture_CreateFromStdioStream(FILE* stdioStream,
                                 ktxTextureCreateFlags createFlags,
                                 ktxTexture** newTex);

KTX_error_code
ktxTexture_CreateFromNamedFile(const char* const filename,
                               ktxTextureCreateFlags createFlags,
                               ktxTexture** newTex);

KTX_error_code
ktxTexture_CreateFromMemory(const ktx_uint8_t* bytes, ktx_size_t size,
                            ktxTextureCreateFlags createFlags,
                            ktxTexture** newTex);

/*
 * Returns a pointer to the image data of a ktxTexture object.
 */
ktx_uint8_t*
ktxTexture_GetData(ktxTexture* This);

/*
 * Returns the pitch of a row of an image at the specified level.
 * Similar to the rowPitch in a VkSubResourceLayout.
 */
 ktx_uint32_t
 ktxTexture_GetRowPitch(ktxTexture* This, ktx_uint32_t level);

 /*
  * Return the element size of the texture's images.
  */
 ktx_uint32_t
 ktxTexture_GetElementSize(ktxTexture* This);

/*
 * Returns the size of all the image data of a ktxTexture object in bytes.
 */
ktx_size_t
ktxTexture_GetSize(ktxTexture* This);

/*
 * Iterates over the already loaded levels in a ktxTexture object.
 * iterCb is called for each level. The data passed to iterCb
 * includes all faces for each level.
 */
KTX_error_code
ktxTexture_IterateLevels(ktxTexture* This, PFNKTXITERCB iterCb,
                         void* userdata);

/*
 * Create a new ktxTexture1.
 */
KTX_error_code
ktxTexture1_Create(ktxTextureCreateInfo* createInfo,
                   ktxTextureCreateStorageEnum storageAllocation,
                   ktxTexture1** newTex);

/*
 * These three create a ktxTexture1 provided the data is in KTX format.
 */
KTX_error_code
ktxTexture1_CreateFromStdioStream(FILE* stdioStream,
                                 ktxTextureCreateFlags createFlags,
                                 ktxTexture1** newTex);

KTX_error_code
ktxTexture1_CreateFromNamedFile(const char* const filename,
                               ktxTextureCreateFlags createFlags,
                               ktxTexture1** newTex);

KTX_error_code
ktxTexture1_CreateFromMemory(const ktx_uint8_t* bytes, ktx_size_t size,
                            ktxTextureCreateFlags createFlags,
                            ktxTexture1** newTex);

void ktxTexture1_Destroy(ktxTexture1* This);

/*
 * Create a new ktxTexture2.
 */
KTX_error_code
ktxTexture2_Create(ktxTextureCreateInfo* createInfo,
                   ktxTextureCreateStorageEnum storageAllocation,
                   ktxTexture2** newTex);

/*
 * These three create a ktxTexture2 provided the data is in KTX2 format.
 */
KTX_error_code
ktxTexture2_CreateFromStdioStream(FILE* stdioStream,
                                 ktxTextureCreateFlags createFlags,
                                 ktxTexture2** newTex);

KTX_error_code
ktxTexture2_CreateFromNamedFile(const char* const filename,
                               ktxTextureCreateFlags createFlags,
                               ktxTexture2** newTex);

KTX_error_code
ktxTexture2_CreateFromMemory(const ktx_uint8_t* bytes, ktx_size_t size,
                            ktxTextureCreateFlags createFlags,
                            ktxTexture2** newTex);

void ktxTexture2_Destroy(ktxTexture2* This);

/*
 * Write a ktxTexture object to a stdio stream in KTX format.
 */
KTX_error_code
ktxTexture1_WriteKTX2ToStdioStream(ktxTexture1* This, FILE* dstsstr);

/*
 * Write a ktxTexture object to a named file in KTX format.
 */
KTX_error_code
ktxTexture1_WriteKTX2ToNamedFile(ktxTexture1* This, const char* const dstname);

/*
 * Write a ktxTexture object to a block of memory in KTX format.
 */
KTX_error_code
ktxTexture1_WriteKTX2ToMemory(ktxTexture1* This,
                             ktx_uint8_t** bytes, ktx_size_t* size);

/*
 * Returns a string corresponding to a KTX error code.
 */
const char* const ktxErrorString(KTX_error_code error);

KTX_error_code ktxHashList_Create(ktxHashList** ppHl);
void ktxHashList_Construct(ktxHashList* pHl);
void ktxHashList_Destroy(ktxHashList* head);
void ktxHashList_Destruct(ktxHashList* head);
/*
 * Adds a key-value pair to a hash list.
 */
KTX_error_code
ktxHashList_AddKVPair(ktxHashList* pHead, const char* key,
                      unsigned int valueLen, const void* value);

/*
 * Deletes a ktxHashListEntry from a ktxHashList.
 */
KTX_error_code
ktxHashList_DeleteEntry(ktxHashList* pHead,  ktxHashListEntry* pEntry);

/*
 * Finds the entry for a key in a ktxHashList and deletes it.
 */
KTX_error_code
ktxHashList_DeleteKVPair(ktxHashList* pHead, const char* key);

/*
 * Looks up a key and returns the ktxHashListEntry.
 */
KTX_error_code
ktxHashList_FindEntry(ktxHashList* pHead, const char* key,
                      ktxHashListEntry** ppEntry);

/*
 * Looks up a key and returns the value.
 */
KTX_error_code
ktxHashList_FindValue(ktxHashList* pHead, const char* key,
                      unsigned int* pValueLen, void** pValue);

/*
 * Return the next entry in a ktxHashList.
 */
ktxHashListEntry*
ktxHashList_Next(ktxHashListEntry* entry);

/*
 * Sorts a ktxHashList into order of the key codepoints.
 */
KTX_error_code
ktxHashList_Sort(ktxHashList* pHead);

/*
 * Serializes a ktxHashList to a block of memory suitable for
 * writing to a KTX file.
 */
KTX_error_code
ktxHashList_Serialize(ktxHashList* pHead,
                      unsigned int* kvdLen, unsigned char** kvd);

/*
 * Creates a hash table from the serialized data read from a
 * a KTX file.
 */
KTX_error_code
ktxHashList_Deserialize(ktxHashList* pHead, unsigned int kvdLen, void* kvd);

/*
 * Get the key from a ktxHashListEntry
 */
KTX_error_code
ktxHashListEntry_GetKey(ktxHashListEntry* This,
                        unsigned int* pKeyLen, char** ppKey);

/*
 * Get the value from a ktxHashListEntry
 */
KTX_error_code
ktxHashListEntry_GetKey(ktxHashListEntry* This,
                        unsigned int* pValueLen, char** ppValue);

/*
 * Get the value from a ktxHashListEntry
 */
KTX_error_code
ktxHashListEntry_GetValue(ktxHashListEntry* This,
                          unsigned int* pValueLen, void** ppValue);

/*===========================================================*
 * Utilities for printing ingo about a KTX file.             *
 *===========================================================*/

KTX_error_code ktxPrintInfoForStdioStream(FILE* stdioStream);
KTX_error_code ktxPrintInfoForNamedFile(const char* const filename);
KTX_error_code ktxPrintInfoForMemory(const ktx_uint8_t* bytes, ktx_size_t size);

#ifdef __cplusplus
}
#endif

/**
@~English
@page libktx_history Revision History

@section v6 Version 3.0
Added:
@li new ktxTexture object based API for reading KTX files without an OpenGL context.
@li Vulkan loader. @#include <ktxvulkan.h> to use it.

Changed:
@li ktx.h to not depend on KHR/khrplatform.h and GL{,ES*}/gl{corearb,}.h.
    Applications using OpenGL must now include these files themselves.
@li ktxLoadTexture[FMN], removing the hack of loading 1D textures as 2D textures
    when the OpenGL context does not support 1D textures.
    KTX_UNSUPPORTED_TEXTURE_TYPE is now returned.

@section v5 Version 2.0.2
Added:
@li Support for cubemap arrays.

Changed:
@li New build system

Fixed:
@li GitHub issue #40: failure to byte-swap key-value lengths.
@li GitHub issue #33: returning incorrect target when loading cubemaps.
@li GitHub PR #42: loading of texture arrays.
@li GitHub PR #41: compilation error when KTX_OPENGL_ES2=1 defined.
@li GitHub issue #39: stack-buffer-overflow in toktx
@li Don't use GL_EXTENSIONS on recent OpenGL versions.

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
@li tests for ktxLoadTexture[FN] that run under OpenGL ES 3.0 and OpenGL 3.3.
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


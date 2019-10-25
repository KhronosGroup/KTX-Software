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

/*
 * Don't use khrplatform.h in order not to break apps existing
 * before this definitions were needed.
 */
#if defined(KHRONOS_STATIC)
  #define KTX_APICALL
#elif defined(_WIN32)
  #if !defined(KTX_APICALL)
    #define KTX_APICALL __declspec(dllimport)
  #endif
#elif defined(__ANDROID__)
  #define KTX_APICALL __attribute__((visibility("default")))
#else
  #define KTX_APICALL
#endif

#if defined(_WIN32) && !defined(KHRONOS_STATIC)
  #if !defined(KTX_APIENTRY)
    #define KTX_APIENTRY __stdcall
  #endif
#else
  #define KTX_APIENTRY
#endif

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
 * @brief Key String for standard orientation metadata.
 */
#define KTX_ORIENTATION_KEY "KTXorientation"
/**
 * @~English
 * @brief Key String for standard swizzle metadata.
 */
#define KTX_SWIZZLE_KEY "KTXswizzle"
/**
 * @~English
 * @brief Key String for standard writer metadata.
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
typedef enum ktx_error_code_e {
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
    KTX_TRANSCODE_FAILED,    /*!< Transcoding of block compressed texture failed. */
    KTX_UNKNOWN_FILE_FORMAT, /*!< The file not a KTX file */
    KTX_UNSUPPORTED_TEXTURE_TYPE, /*!< The KTX file specifies an unsupported texture type. */
    KTX_UNSUPPORTED_FEATURE  /*!< Feature not included in in-use library or not yet implemented. */
} ktx_error_code_e;
/**
 * @deprecated
 * @~English
 * @brief For backward compatibility
 */
#define KTX_error_code ktx_error_code_e

#define KTX_IDENTIFIER_REF  { 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A }
#define KTX_ENDIAN_REF      (0x04030201)
#define KTX_ENDIAN_REF_REV  (0x01020304)
#define KTX_HEADER_SIZE     (64)

/**
 * @~English
 * @brief Result codes returned by library functions.
 */
 typedef enum ktx_error_code_e ktxResult;

/**
 * @class ktxHashList
 * @~English
 * @brief Opaque handle to a ktxHashList.
 */
typedef struct ktxKVListEntry* ktxHashList;

#define KTX_APIENTRYP KTX_APIENTRY *
/**
 * @class ktxHashListEntry
 * @~English
 * @brief Opaque handle to an entry in a @ref ktxHashList.
 */
typedef struct ktxKVListEntry ktxHashListEntry;

typedef enum ktxOrientationX {
    KTX_ORIENT_X_LEFT = 'l', KTX_ORIENT_X_RIGHT = 'r'
} ktxOrientationX;

typedef enum ktxOrientationY {
    KTX_ORIENT_Y_UP = 'u', KTX_ORIENT_Y_DOWN = 'd'
} ktxOrientationY;

typedef enum ktxOrientationZ {
    KTX_ORIENT_Z_IN = 'i', KTX_ORIENT_Z_OUT = 'o'
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

typedef KTX_error_code
    (* PFNKTXITERCB)(int miplevel, int face,
                     int width, int height, int depth,
                     ktx_uint64_t faceLodSize,
                     void* pixels, void* userdata);

/* Don't use KTX_APIENTRYP to avoid a Doxygen bug. */
typedef void (KTX_APIENTRY* PFNKTEXDESTROY)(ktxTexture* This);
typedef KTX_error_code
    (KTX_APIENTRY* PFNKTEXGETIMAGEOFFSET)(ktxTexture* This, ktx_uint32_t level,
                                          ktx_uint32_t layer,
                                          ktx_uint32_t faceSlice,
                                          ktx_size_t* pOffset);
typedef ktx_size_t
    (KTX_APIENTRY* PFNKTEXGETIMAGESIZE)(ktxTexture* This, ktx_uint32_t level);
typedef KTX_error_code
    (KTX_APIENTRY* PFNKTEXGLUPLOAD)(ktxTexture* This,
                                    GLuint* pTexture, GLenum* pTarget,
                                    GLenum* pGlerror);
typedef KTX_error_code
    (KTX_APIENTRY* PFNKTEXITERATELEVELS)(ktxTexture* This, PFNKTXITERCB iterCb,
                                         void* userdata);

typedef KTX_error_code
    (KTX_APIENTRY* PFNKTEXITERATELOADLEVELFACES)(ktxTexture* This,
                                                 PFNKTXITERCB iterCb,
                                                 void* userdata);
typedef KTX_error_code
    (KTX_APIENTRY* PFNKTEXLOADIMAGEDATA)(ktxTexture* This,
                                         ktx_uint8_t* pBuffer,
                                         ktx_size_t bufSize);
typedef KTX_error_code
    (KTX_APIENTRY* PFNKTEXSETIMAGEFROMMEMORY)(ktxTexture* This,
                                              ktx_uint32_t level,
                                              ktx_uint32_t layer,
                                              ktx_uint32_t faceSlice,
                                              const ktx_uint8_t* src,
                                              ktx_size_t srcSize);

typedef KTX_error_code
    (KTX_APIENTRY* PFNKTEXSETIMAGEFROMSTDIOSTREAM)(ktxTexture* This,
                                                   ktx_uint32_t level,
                                                   ktx_uint32_t layer,
                                                   ktx_uint32_t faceSlice,
                                                   FILE* src, ktx_size_t srcSize);
typedef KTX_error_code
    (KTX_APIENTRY* PFNKTEXWRITETOSTDIOSTREAM)(ktxTexture* This, FILE* dstsstr);
typedef KTX_error_code
    (KTX_APIENTRY* PFNKTEXWRITETONAMEDFILE)(ktxTexture* This,
                                            const char* const dstname);
typedef KTX_error_code
    (KTX_APIENTRY* PFNKTEXWRITETOMEMORY)(ktxTexture* This,
                                         ktx_uint8_t** bytes, ktx_size_t* size);

/**
 * @memberof ktxTexture
 * @~English
 * @brief Table of virtual ktxTexture methods.
 */
 struct ktxTexture_vtbl {
    PFNKTEXDESTROY Destroy;
    PFNKTEXGETIMAGEOFFSET GetImageOffset;
    PFNKTEXGETIMAGESIZE GetImageSize;
    PFNKTEXGLUPLOAD GLUpload;
    PFNKTEXITERATELEVELS IterateLevels;
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

/**
 * @~English
 * @brief Helper for calling the Destroy virtual method of a ktxTexture.
 * @copydoc ktxTexture2_Destroy
 */
#define ktxTexture_Destroy(This) (This)->vtbl->Destroy(This)

/**
 * @~English
 * @brief Helper for calling the GetImageOffset virtual method of a
 *        ktxTexture.
 * @copydoc ktxTexture2_GetImageOffset
 */
#define ktxTexture_GetImageOffset(This, faceSlice, layer, level, pOffset) \
            (This)->vtbl->GetImageOffset(This, faceSlice, layer, level, pOffset)

/**
 * @~English
 * @brief Helper for calling the GetImageSize virtual method of a ktxTexture.
 * @copydoc ktxTexture2_GetImageSize
 */
#define ktxTexture_GetImageSize(This, level) \
            (This)->vtbl->GetImageSize(This, level)

/*
 * Uploads the image data from a ktxTexture Thisect to an OpenGL {,ES} texture
 * Thisect.
 */
#define ktxTexture_GLUpload(This, a, b, c) (This)->vtbl->GLUpload(This, a, b, c)

/**
 * @~English
 * @brief Helper for calling the IterateLevels virtual method of a ktxTexture.
 * @copydoc ktxTexture2_IterateLevels
 */
#define ktxTexture_IterateLevels(This, iterCb, userdata) \
                            (This)->vtbl->IterateLevels(This, iterCb, userdata)

/**
 * @~English
 * @brief Helper for calling the IterateLoadLevelFaces virtual method of a
 * ktxTexture.
 * @copydoc ktxTexture2_IterateLoadLevelFaces
 */
 #define ktxTexture_IterateLoadLevelFaces(This, iterCb, userdata) \
                    (This)->vtbl->IterateLoadLevelFaces(This, iterCb, userdata)

/**
 * @~English
 * @brief Helper for calling the LoadImageData virtual method of a ktxTexture.
 * @copydoc ktxTexture2_LoadImageData
 */
#define ktxTexture_LoadImageData(This, pBuffer, bufSize) \
                    (This)->vtbl->LoadImageData(This, pBuffer, bufSize)

/**
 * @~English
 * @brief Helper for calling the SetImageFromMemory virtual method of a
 * ktxTexture.
 * @copydoc ktxTexture2_SetImageFromMemory
 */
#define ktxTexture_SetImageFromMemory(This, level, layer, faceSlice, \
                                      src, srcSize)                  \
    (This)->vtbl->SetImageFromMemory(This, level, layer, faceSlice, src, srcSize)

/**
 * @~English
 * @brief Helper for calling the SetImageFromStdioStream virtual method of a
 * ktxTexture.
 * @copydoc ktxTexture2_SetImageFromStdioStream
 */
#define ktxTexture_SetImageFromStdioStream(This, level, layer, faceSlice, \
                                           src, srcSize)                  \
    (This)->vtbl->SetImageFromStdioStream(This, level, layer, faceSlice,  \
                                        src, srcSize)

/**
 * @~English
 * @brief Helper for calling the WriteToStdioStream virtual method of a
 * ktxTexture.
 * @copydoc ktxTexture2_WriteToStdioStream
 */
#define ktxTexture_WriteToStdioStream(This, dstsstr) \
                                (This)->vtbl->WriteToStdioStream(This, dstsstr)

/**
 * @~English
 * @brief Helper for calling the WriteToNamedfile virtual method of a
 * ktxTexture.
 * @copydoc ktxTexture2_WriteToNamedFile
 */
#define ktxTexture_WriteToNamedFile(This, dstname) \
                                (This)->vtbl->WriteToNamedFile(This, dstname)

/**
 * @~English
 * @brief Helper for calling the WriteToMemory virtual method of a ktxTexture.
 * @copydoc ktxTexture2_WriteToMemory
 */
#define ktxTexture_WriteToMemory(This, ppDstBytes, pSize) \
                  (This)->vtbl->WriteToMemory(This, ppDstBytes, pSize)


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
    struct ktxTexture1_private* _private; /*!< Private data. */
} ktxTexture1;

/*===========================================================*
* KTX format version 2                                      *
*===========================================================*/

/**
 * @~English
 * @brief Enumerators identifying the supercompression scheme.
 */
typedef enum ktxSupercmpScheme {
    KTX_SUPERCOMPRESSION_NONE = 0,  /*!< No supercompression. */
    KTX_SUPERCOMPRESSION_BASIS = 1, /*!< Basis Universal supercompression. */
    KTX_SUPERCOMPRESSION_LZMA = 2,  /*!< LZMA supercompression. */
    KTX_SUPERCOMPRESSION_ZLIB = 3,  /*!< Zlib supercompression. */
    KTX_SUPERCOMPRESSION_ZSTD = 4,  /*!< ZStd supercompression. */
    KTX_SUPERCOMPRESSION_BEGIN_RANGE = KTX_SUPERCOMPRESSION_NONE,
    KTX_SUPERCOMPRESSION_END_RANGE = KTX_SUPERCOMPRESSION_ZSTD,
    KTX_SUPERCOMPRESSION_BEGIN_VENDOR_RANGE = 0x10000,
    KTX_SUPERCOMPRESSION_END_VENDOR_RANGE = 0x1ffff,
    KTX_SUPERCOMPRESSION_BEGIN_RESERVED = 0x20000,
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
    struct ktxTexture2_private* _private;  /*!< Private data. */
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
 * These three create a ktxTexture1 or ktxTexture2 according to the data
 * header, and return a pointer to the base ktxTexture class.
 */
KTX_APICALL KTX_error_code KTX_APIENTRY
ktxTexture_CreateFromStdioStream(FILE* stdioStream,
                                 ktxTextureCreateFlags createFlags,
                                 ktxTexture** newTex);

KTX_APICALL KTX_error_code KTX_APIENTRY
ktxTexture_CreateFromNamedFile(const char* const filename,
                               ktxTextureCreateFlags createFlags,
                               ktxTexture** newTex);

KTX_APICALL KTX_error_code KTX_APIENTRY
ktxTexture_CreateFromMemory(const ktx_uint8_t* bytes, ktx_size_t size,
                            ktxTextureCreateFlags createFlags,
                            ktxTexture** newTex);

/*
 * Returns a pointer to the image data of a ktxTexture object.
 */
KTX_APICALL ktx_uint8_t* KTX_APIENTRY
ktxTexture_GetData(ktxTexture* This);

/*
 * Returns the pitch of a row of an image at the specified level.
 * Similar to the rowPitch in a VkSubResourceLayout.
 */
KTX_APICALL ktx_uint32_t KTX_APIENTRY
ktxTexture_GetRowPitch(ktxTexture* This, ktx_uint32_t level);

 /*
  * Return the element size of the texture's images.
  */
KTX_APICALL ktx_uint32_t KTX_APIENTRY
ktxTexture_GetElementSize(ktxTexture* This);

/*
 * Returns the size of all the image data of a ktxTexture object in bytes.
 */
KTX_APICALL ktx_size_t KTX_APIENTRY
ktxTexture_GetSize(ktxTexture* This);

/*
 * Iterate over the levels or faces in a ktxTexture object.
 */
KTX_APICALL KTX_error_code KTX_APIENTRY
ktxTexture_IterateLevelFaces(ktxTexture* This, PFNKTXITERCB iterCb,
                             void* userdata);
/*
 * Create a new ktxTexture1.
 */
KTX_APICALL KTX_error_code KTX_APIENTRY
ktxTexture1_Create(ktxTextureCreateInfo* createInfo,
                   ktxTextureCreateStorageEnum storageAllocation,
                   ktxTexture1** newTex);

/*
 * These three create a ktxTexture1 provided the data is in KTX format.
 */
KTX_APICALL KTX_error_code KTX_APIENTRY
ktxTexture1_CreateFromStdioStream(FILE* stdioStream,
                                 ktxTextureCreateFlags createFlags,
                                 ktxTexture1** newTex);

KTX_APICALL KTX_error_code KTX_APIENTRY
ktxTexture1_CreateFromNamedFile(const char* const filename,
                               ktxTextureCreateFlags createFlags,
                               ktxTexture1** newTex);

KTX_APICALL KTX_error_code KTX_APIENTRY
ktxTexture1_CreateFromMemory(const ktx_uint8_t* bytes, ktx_size_t size,
                            ktxTextureCreateFlags createFlags,
                            ktxTexture1** newTex);

/*
 * Write a ktxTexture object to a stdio stream in KTX format.
 */
KTX_APICALL KTX_error_code KTX_APIENTRY
ktxTexture1_WriteKTX2ToStdioStream(ktxTexture1* This, FILE* dstsstr);

/*
 * Write a ktxTexture object to a named file in KTX format.
 */
KTX_APICALL KTX_error_code KTX_APIENTRY
ktxTexture1_WriteKTX2ToNamedFile(ktxTexture1* This, const char* const dstname);

/*
 * Write a ktxTexture object to a block of memory in KTX format.
 */
KTX_APICALL KTX_error_code KTX_APIENTRY
ktxTexture1_WriteKTX2ToMemory(ktxTexture1* This,
                             ktx_uint8_t** bytes, ktx_size_t* size);

/*
 * Create a new ktxTexture2.
 */
KTX_APICALL KTX_error_code KTX_APIENTRY
ktxTexture2_Create(ktxTextureCreateInfo* createInfo,
                   ktxTextureCreateStorageEnum storageAllocation,
                   ktxTexture2** newTex);

/*
 * Create a new ktxTexture2 as a copy of an existing texture.
 */
 KTX_APICALL KTX_error_code KTX_APIENTRY
 ktxTexture2_CreateCopy(ktxTexture2* orig, ktxTexture2** newTex);

 /*
  * These three create a ktxTexture2 provided the data is in KTX2 format.
  */
KTX_APICALL KTX_error_code KTX_APIENTRY
ktxTexture2_CreateFromStdioStream(FILE* stdioStream,
                                 ktxTextureCreateFlags createFlags,
                                 ktxTexture2** newTex);

KTX_APICALL KTX_error_code KTX_APIENTRY
ktxTexture2_CreateFromNamedFile(const char* const filename,
                               ktxTextureCreateFlags createFlags,
                               ktxTexture2** newTex);

KTX_APICALL KTX_error_code KTX_APIENTRY
ktxTexture2_CreateFromMemory(const ktx_uint8_t* bytes, ktx_size_t size,
                            ktxTextureCreateFlags createFlags,
                            ktxTexture2** newTex);

KTX_APICALL KTX_error_code KTX_APIENTRY
ktxTexture2_CompressBasis(ktxTexture2* This, ktx_uint32_t quality);

KTX_APICALL ktx_uint32_t KTX_APIENTRY
ktxTexture2_GetOETF(ktxTexture2* This);

KTX_APICALL void KTX_APIENTRY
ktxTexture2_GetComponentInfo(ktxTexture2* This, ktx_uint32_t* numComponents,
                             ktx_uint32_t* componentByteLength);

/**
 * @memberof ktxTexture2
 * @~English
 * @brief Structure for passing extended parameters to
 *        ktxTexture2_CompressBasisEx.
 *
 * Passing a struct initialized to 0 (e.g. " = {};") will use the default
 * values. Only those settings to be modified need be non-zero.
 */
typedef struct ktxBasisParams {
    ktx_uint32_t structSize;
        /*!< Size of this struct. Used so library can tell which version
             of struct is being passed.
         */
    ktx_uint32_t threadCount;
        /*!< Number of threads used for compression. Default is 1. */
    ktx_uint32_t compressionLevel;
        /*!< Encoding speed vs. quality tradeoff. Range is 0 - 5, default
             is 1. Higher values are slower, but give higher quality.
        */
    ktx_uint32_t qualityLevel;
        /*!< Compression quality. Range is 1 - 255.  Lower gives better
             compression/lower quality/faster. Higher gives less compression
             /higher quality/slower. Values of @c maxEndpoints and
             @c maxSelectors computed from this override any explicitly set
             values. Default is 128, if either of @c maxEndpoints or
             @c maxSelectors is unset, otherwise those settings rule.
        */
    ktx_uint32_t maxEndpoints;
        /*!< Manually set the max number of color endpoint clusters
             from 1-16128. Default is 0, unset.
         */
    float endpointRDOThreshold;
        /*!< Set endpoint RDO quality threshold. The default is 1.25. Lower is
             higher quality but less quality per output bit (try 1.0-3.0).
             This will override the value chosen by @c qualityLevel.
         */
    ktx_uint32_t maxSelectors;
        /*!< Manually set the max number of color selector clusters
             from 1-16128. Default is 0, unset.
         */
    float selectorRDOThreshold;
        /*!< Set selector RDO quality threshold. The default is 1.5. Lower is
             higher quality but less quality per output bit (try 1.0-3.0).
             This will override the value chosen by @c qualityLevel.
         */
    ktx_bool_t normalMap;
        /*!< Tunes codec parameters for better quality on normal maps (no
             selector RDO, no endpoint RDO). Only valid for linear textures.
         */
    ktx_bool_t separateRGToRGB_A;
        /*!< Separates the input R and G channels to RGB and A (for tangent
             space XY normal maps). Only valid for 2-component textures.
         */
    ktx_bool_t preSwizzle;
        /*!< If the texture has @c KTXswizzle metadata, apply it before
             compressing. Swizzling, like @c rabb may yield drastically
             different error metrics if done after supercompression.
         */
    ktx_bool_t noEndpointRDO;
        /*!< Disable endpoint rate distortion optimizations. Slightly faster,
             less noisy output, but lower quality per output bit. Default is
             KTX_FALSE.
         */
    ktx_bool_t noSelectorRDO;
        /*!< Disable selector rate distortion optimizations. Slightly faster,
             less noisy output, but lower quality per output bit. Default is
             KTX_FALSE.
         */
} ktxBasisParams;

KTX_APICALL KTX_error_code KTX_APIENTRY
ktxTexture2_CompressBasisEx(ktxTexture2* This, ktxBasisParams* params);

/**
 * @~English
 * @brief Enumerators for specifying the transcode target format.
 *
 * @e Opaque and @e alpha here refer to 2 separate RGB images, a.k.a slices within the Basis compressed
 * data. The opaque slice holds the RGB components of the original image. The alpha slice holds the alpha
 * component whose value is replicated in all three components. If the original image had only 2
 * components, R will be in the opaque slice and G in the alpha slice which each value replicated in all
 * 3 components of its slice. If the original image had only 1 component it's value is replicated in all
 * 3 components of the opaque slice and there is no alpha slice.
 */
typedef enum ktx_transcode_fmt_e {
        // Compressed formats

        // ETC1-2
        KTX_TTF_ETC1_RGB = 0,
            /*!< Opaque only. Returns RGB or alpha data, if
              KTX_TF_TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS flag is specified. */
        KTX_TTF_ETC2_RGBA = 1,
            /*!< Opaque+alpha. EAC_A8 block followed by an ETC1 block. The alpha channel will be
              opaque for textures without an alpha channel. */

        // BC1-5, BC7 (desktop, some mobile devices)
        KTX_TTF_BC1_RGB = 2,
            /*!< Opaque only, no punchthrough alpha support yet.  Returns RGB or alpha data, if
              KTX_TF_TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS flag is specified. */
        KTX_TTF_BC3_RGBA = 3,
            /*!< Opaque+alpha. BC4 block with alpha followed by a BC1 block. The alpha channel will
              be opaque for textures without an alpha channel. */
        KTX_TTF_BC4_R = 4,
        	/*!< One BC4 block. R = opaque.g or alpha.g, if
              KTX_TF_TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS flag is specified. */
        KTX_TTF_BC5_RG = 5,
            /*!<Two BC4 blocks, R=opaque.g and G=alpha.g The texture should have an alpha
              channel (if not G will be all 255's. For tangent space normal maps. */
        KTX_TTF_BC7_M6_RGB = 6,
            /*!< Opaque only.  Returns RGB or alpha data, if
              KTX_TF_TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS flag is specified.
              Highest quality of all the non-ETC formats. The texture memory footprint is the same as
              @c KTX_TTF_BC7_M5_RGBA but transcoding is slower. */
        KTX_TTF_BC7_M5_RGBA = 7,
        	/*!< Opaque+alpha. The alpha channel will be opaque for textures without an alpha channel.
              The texture memory footprint is the same as @c KTX_TTF_BC7_M6_RGB but transcoding
              is faster. */

        // PVRTC1 4bpp (mobile, PowerVR devices)
        KTX_TTF_PVRTC1_4_RGB = 8,
            /*!< Opaque only. Returns RGB or alpha data, if
              KTX_TF_TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS flag is specified. */
        KTX_TTF_PVRTC1_4_RGBA = 9,
            /*!< Opaque+alpha. Most useful for simple opacity maps. If the texture doesn't have an
              alpha channel KTX_TTF_PVRTC1_4_RGB will be used instead. Lowest quality of any
              supported texture format. */

        // ASTC (mobile, Intel devices, hopefully all desktop GPU's one day)
        KTX_TTF_ASTC_4x4_RGBA = 10,
            /*!< Opaque+alpha, ASTC 4x4. The alpha channel will be opaque for textures without an
              alpha channel.  The transcoder uses RGB/RGBA/L/LA modes, void extent, and up to
              two ([0,47] and [0,255]) endpoint precisions. */

        // ATC and FXT1 formats are not supported by KTX2 as there
        // are no equivalent VkFormats.

        KTX_TTF_PVRTC2_4_RGB = 18,
        	/*!<  Opaque-only. Almost BC1 quality, much faster to transcode and supports arbitrary
                 texture dimensions (unlike PVRTC1 RGB). */
        KTX_TTF_PVRTC2_4_RGBA = 19,
        	/*!< Opaque+alpha. Slower to transcode than cTFPVRTC2_4_RGB. Premultiplied alpha
              is highly recommended, otherwise the color channel can leak into the alpha channel
              on transparent blocks. */

        KTX_TTF_ETC2_EAC_R11 = 20,
            /*!< R only (ETC2 EAC R11 unsigned). R = opaque.g or alpha.g, if
              KTX_TF_TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS flag is specified. */
        KTX_TTF_ETC2_EAC_RG11 = 21,
            /*!< RG only (ETC2 EAC RG11 unsigned), R=opaque.g, G=alpha.g. The texture should have
              an alpha channel (if not G will be all 255's. For tangent space normal maps. */
        
        // Uncompressed (raw pixel) formats
        KTX_TTF_RGBA32 = 13,
            /*!< 32bpp RGBA image stored in raster (not block) order in memory, R is first byte, A is last
              byte. */
        KTX_TTF_RGB565 = 14,
            /*!< 16bpp RGB image stored in raster (not block) order in memory, R at bit position 11. */
        KTX_TTF_BGR565 = 15,
            /*!< 16bpp RGB image stored in raster (not block) order in memory, R at bit position 0. */
        KTX_TTF_RGBA4444 = 16,
            /*!< 16bpp RGBA image stored in raster (not block) order in memory, R at bit position 12,
              A at bit position 0. */

        // Values for automatic selection of RGB or RGBA depending if alpha
        KTX_TTF_ETC = 22,
            /*!< Automatically selects @c KTX_TTF_ETC1_RGB or @c KTX_TTF_ETC2_RGBA
              according to presence of alpha. */
        KTX_TTF_BC1_OR_3 = 23,
            /*!< Automatically selects @c KTX_TTF_BC1_RGB or @c KTX_TTF_BC3_RGBA
              according to presence of alpha. */

        // Old enums for compatibility with code compiled against previous
        // versions of libktx.
        KTX_TF_ETC1 = KTX_TTF_ETC1_RGB,
            //!< @deprecated. Use #KTX_TTF_ETC1_RGB.
        KTX_TF_ETC2 = KTX_TTF_ETC,
            //!< @deprecated. Use #KTX_TTF_ETC.
        KTX_TF_BC1 = KTX_TTF_BC1_RGB,
            //!< @deprecated. Use #KTX_TTF_BC1_RGB.
        KTX_TF_BC3 = KTX_TTF_BC3_RGBA,
            //!< @deprecated. Use #KTX_TTF_BC3_RGBA.
        KTX_TF_BC4 = KTX_TTF_BC4_R,
            //!< @deprecated. Use #KTX_TTF_BC4_R.
        KTX_TF_BC5 = KTX_TTF_BC5_RG,
            //!< @deprecated. Use #KTX_TTF_BC5_RG.
        KTX_TF_BC7_M6_OPAQUE_ONLY = KTX_TTF_BC7_M6_RGB,
            //!< @deprecated. Use #KTX_TTF_BC7_M6_RGB.
        KTX_TF_PVRTC1_4_OPAQUE_ONLY = KTX_TTF_PVRTC1_4_RGB
            //!< @deprecated. Use #KTX_TTF_PVRTC1_4_RGB.
} ktx_transcode_fmt_e;

/**
 * @~English
 * @brief Flags guiding transcoding of Basis Universal compressed textures.
 */
typedef enum ktx_transcode_flag_bits_e {
    KTX_TF_PVRTC_WRAP_ADDRESSING = 1,
        /*!< PVRTC1: texture will use wrap addressing vs. clamp (most PVRTC
             viewer tools assume wrap addressing, so we default to wrap although
             that can cause edge artifacts).
        */
    KTX_TF_PVRTC_DECODE_TO_NEXT_POW2 = 2,
        /*!< PVRTC1: decode non-pow2 ETC1S texture level to the next larger
             power of 2 (not implemented yet, but we're going to support it).
             Ignored if the slice's dimensions are already a power of 2.
         */
    KTX_TF_TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS = 4,
        /*!< When decoding to an opaque texture format, if the Basis data has
             alpha, decode the alpha slice instead of the color slice to the
             output texture format. Has no effect if there is no alpha data.
         */
} ktx_transcode_flag_bits_e;
typedef ktx_uint32_t ktx_transcode_flags;

KTX_APICALL KTX_error_code KTX_APIENTRY
ktxTexture2_TranscodeBasis(ktxTexture2* This, ktx_transcode_fmt_e fmt,
                           ktx_transcode_flags transcodeFlags);

/*
 * Returns a string corresponding to a KTX error code.
 */
KTX_APICALL const char* const KTX_APIENTRY
ktxErrorString(KTX_error_code error);

/*
 * Returns a string corresponding to a transcode target format.
 */
KTX_APICALL const char* const KTX_APIENTRY
ktxTranscodeFormatString(ktx_transcode_fmt_e format);

KTX_APICALL KTX_error_code KTX_APIENTRY ktxHashList_Create(ktxHashList** ppHl);
KTX_APICALL KTX_error_code KTX_APIENTRY
ktxHashList_CreateCopy(ktxHashList** ppHl, ktxHashList orig);
KTX_APICALL void KTX_APIENTRY ktxHashList_Construct(ktxHashList* pHl);
KTX_APICALL void KTX_APIENTRY
ktxHashList_ConstructCopy(ktxHashList* pHl, ktxHashList orig);
KTX_APICALL void KTX_APIENTRY ktxHashList_Destroy(ktxHashList* head);
KTX_APICALL void KTX_APIENTRY ktxHashList_Destruct(ktxHashList* head);

/*
 * Adds a key-value pair to a hash list.
 */
KTX_APICALL KTX_error_code KTX_APIENTRY
ktxHashList_AddKVPair(ktxHashList* pHead, const char* key,
                      unsigned int valueLen, const void* value);

/*
 * Deletes a ktxHashListEntry from a ktxHashList.
 */
KTX_APICALL KTX_error_code KTX_APIENTRY
ktxHashList_DeleteEntry(ktxHashList* pHead,  ktxHashListEntry* pEntry);

/*
 * Finds the entry for a key in a ktxHashList and deletes it.
 */
KTX_APICALL KTX_error_code KTX_APIENTRY
ktxHashList_DeleteKVPair(ktxHashList* pHead, const char* key);

/*
 * Looks up a key and returns the ktxHashListEntry.
 */
KTX_APICALL KTX_error_code KTX_APIENTRY
ktxHashList_FindEntry(ktxHashList* pHead, const char* key,
                      ktxHashListEntry** ppEntry);

/*
 * Looks up a key and returns the value.
 */
KTX_APICALL KTX_error_code KTX_APIENTRY
ktxHashList_FindValue(ktxHashList* pHead, const char* key,
                      unsigned int* pValueLen, void** pValue);

/*
 * Return the next entry in a ktxHashList.
 */
KTX_APICALL ktxHashListEntry* KTX_APIENTRY
ktxHashList_Next(ktxHashListEntry* entry);

/*
 * Sorts a ktxHashList into order of the key codepoints.
 */
KTX_APICALL KTX_error_code KTX_APIENTRY
ktxHashList_Sort(ktxHashList* pHead);

/*
 * Serializes a ktxHashList to a block of memory suitable for
 * writing to a KTX file.
 */
KTX_APICALL KTX_error_code KTX_APIENTRY
ktxHashList_Serialize(ktxHashList* pHead,
                      unsigned int* kvdLen, unsigned char** kvd);

/*
 * Creates a hash table from the serialized data read from a
 * a KTX file.
 */
KTX_APICALL KTX_error_code KTX_APIENTRY
ktxHashList_Deserialize(ktxHashList* pHead, unsigned int kvdLen, void* kvd);

/*
 * Get the key from a ktxHashListEntry
 */
KTX_APICALL KTX_error_code KTX_APIENTRY
ktxHashListEntry_GetKey(ktxHashListEntry* This,
                        unsigned int* pKeyLen, char** ppKey);

/*
 * Get the value from a ktxHashListEntry
 */
KTX_APICALL KTX_error_code KTX_APIENTRY
ktxHashListEntry_GetValue(ktxHashListEntry* This,
                          unsigned int* pValueLen, void** ppValue);

/*===========================================================*
 * Utilities for printing info about a KTX file.             *
 *===========================================================*/

KTX_APICALL KTX_error_code KTX_APIENTRY ktxPrintInfoForStdioStream(FILE* stdioStream);
KTX_APICALL KTX_error_code KTX_APIENTRY ktxPrintInfoForNamedFile(const char* const filename);
KTX_APICALL KTX_error_code KTX_APIENTRY ktxPrintInfoForMemory(const ktx_uint8_t* bytes, ktx_size_t size);

#ifdef __cplusplus
}
#endif

/*========================================================================*
 * For backward compatibilty with the V3 & early versions of the V4 APIs. *
 *========================================================================*/

/**
 * @deprecated Will be dropped before V4 release.
 */
#define ktx_texture_transcode_fmt_e ktx_transcode_fmt_e

/**
 * @deprecated Will be dropped before V4 release.
 */
#define ktx_texture_decode_flags ktx_transcode_flag_bits

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


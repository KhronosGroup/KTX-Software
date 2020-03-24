/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2010-2018 The khronos Group, Inc.
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
 * @file strings.c
 * @~English
 *
 * @brief Functions to return a string corresponding to various enumerations.
 *
 * @author Mark Callow, HI Corporation
 */

#include "ktx.h"

static const char* const errorStrings[] = {
    "Operation succeeded.",                           /* KTX_SUCCESS */
    "File data is inconsistent with KTX spec.",       /* KTX_FILE_DATA_ERROR */
    "File is a pipe; seek operations not possible.",  /* KTX_FILE_ISPIPE */
    "File open failed.",                              /* KTX_FILE_OPEN_FAILED */
    "Operation would exceed the max file size.",      /* KTX_FILE_OVERFLOW */
    "File read error.",                               /* KTX_FILE_READ_ERROR */
    "File seek error.",                               /* KTX_FILE_SEEK_ERROR */
    "File does not have enough data for request.",    /* KTX_FILE_UNEXPECTED_EOF */
    "File write error.",                              /* KTX_FILE_WRITE_ERROR */
    "GL error occurred.",                             /* KTX_GL_ERROR */
    "Operation not allowed in the current state.",    /* KTX_INVALID_OPERATION */
    "Invalid parameter value.",                       /* KTX_INVALID_VALUE */
    "Key not found.",                                 /* KTX_NOT_FOUND */
    "Out of memory.",                                 /* KTX_OUT_OF_MEMORY */
    "Transcoding of block compressed texture failed.",/* KTX_TRANSCODE_FAILED */
    "Not a KTX file.",                                /* KTX_UNKNOWN_FILE_FORMAT */
    "Texture type not supported.",      /* KTX_UNSUPPORTED_TEXTURE_TYPE */
    "Feature not included in in-use library or not yet implemented." /* KTX_UNSUPPORTED_FEATURE */
};
/* This will cause compilation to fail if number of messages and codes doesn't match */
typedef int errorStrings_SIZE_ASSERT[sizeof(errorStrings) / sizeof(char*) - 1 == KTX_ERROR_MAX_ENUM];

/**
 * @~English
 * @brief Return a string corresponding to a KTX error code.
 *
 * @param error     the error code for which to return a string
 *
 * @return pointer to the message string.
 *
 * @internal Use UTF-8 for translated message strings.
 *
 * @author Mark Callow
 */
const char* ktxErrorString(KTX_error_code error)
{
    if (error > KTX_ERROR_MAX_ENUM)
        return "Unrecognized error code";
    return errorStrings[error];
}

/**
* @~English
* @brief Return a string corresponding to a transcode format enumeration.
*
* @param format    the transcode format for which to return a string.
*
* @return pointer to the message string.
*
* @internal Use UTF-8 for translated message strings.
*
* @author Mark Callow
*/
const char* ktxTranscodeFormatString(ktx_transcode_fmt_e format)
{
    switch (format) {
        case KTX_TTF_ETC1_RGB: return "ETC1_RGB";
        case KTX_TTF_ETC2_RGBA: return "ETC2_RGBA";
        case KTX_TTF_BC1_RGB: return "BC1_RGB";
        case KTX_TTF_BC3_RGBA: return "BC3_RGBA";
        case KTX_TTF_BC4_R: return "BC4_R";
        case KTX_TTF_BC5_RG: return "BC5_RG";
        case KTX_TTF_BC7_RGBA: return "BC7_RGBA";
        case KTX_TTF_PVRTC1_4_RGB: return "PVRTC1_4_RGB";
        case KTX_TTF_PVRTC1_4_RGBA: return "PVRTC1_4_RGBA";
        case KTX_TTF_ASTC_4x4_RGBA: return "ASTC_4x4_RGBA";
        case KTX_TTF_RGBA32: return "RGBA32";
        case KTX_TTF_RGB565: return "RGB565";
        case KTX_TTF_BGR565: return "BGR565";
        case KTX_TTF_RGBA4444: return "RGBA4444";
        case KTX_TTF_PVRTC2_4_RGB: return "PVRTC2_4_RGB";
        case KTX_TTF_PVRTC2_4_RGBA: return "PVRTC2_4_RGBA";
        case KTX_TTF_ETC2_EAC_R11: return "ETC2_EAC_R11";
        case KTX_TTF_ETC2_EAC_RG11: return "ETC2_EAC_RG11";
        case KTX_TTF_ETC: return "ETC";
        case KTX_TTF_BC1_OR_3: return "BC1 or BC3";
        default: return "Unrecognized format";
    }
}

/**
* @~English
* @brief Return a string corresponding to a supercompressionScheme enumeration.
*
* @param scheme    the supercompression scheme for which to return a string.
*
* @return pointer to the message string.
*
* @internal Use UTF-8 for translated message strings.
*
* @author Mark Callow
*/
const char *
ktxSupercompressionSchemeString(ktxSupercmpScheme scheme)
{
    switch (scheme) {
      case KTX_SUPERCOMPRESSION_NONE: return "KTX_SUPERCOMPRESSION_NONE";
      case KTX_SUPERCOMPRESSION_BASIS: return "KTX_SUPERCOMPRESSION_BASIS";
      case KTX_SUPERCOMPRESSION_LZMA: return "KTX_SUPERCOMPRESSION_LZMA";
      case KTX_SUPERCOMPRESSION_ZLIB: return "KTX_SUPERCOMPRESSION_ZLIB";
      case KTX_SUPERCOMPRESSION_ZSTD: return "KTX_SUPERCOMPRESSION_ZSTD";
      default: return "Invalid scheme value";
    }
}

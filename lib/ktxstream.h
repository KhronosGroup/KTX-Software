/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

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

/*
 * Author: Maksim Kolesin from original code
 * by Mark Callow and Georg Kolling
 */

#ifndef KTXSTREAM_H
#define KTXSTREAM_H

#include "ktx.h"

typedef struct ktxMem ktxMem;
typedef struct ktxStream ktxStream;

/**
 * @internal
 * @~English
 * @brief type for a pointer to a stream reading function
 */
typedef KTX_error_code (*ktxStream_read)(ktxStream* str, void* dst,
                                         const GLsizei count);
/**
 * @internal
 * @~English
 * @brief type for a pointer to a stream skipping function
 */
typedef KTX_error_code (*ktxStream_skip)(ktxStream* str, const GLsizei count);

/**
 * @internal
 * @~English
 * @brief type for a pointer to a stream reading function
 */
typedef KTX_error_code (*ktxStream_write)(ktxStream* str, const void *src,
                                          const GLsizei size,
                                          const GLsizei count);

/**
 * @internal
 * @~English
 * @brief type for a pointer to a stream closing function
 */
typedef KTX_error_code (*ktxStream_close)(ktxStream* str);

/**
 * @internal
 * @~English
 * @brief KTX stream class
 */
typedef struct ktxStream
{
    ktxStream_read read;   /*!< @internal pointer to function for reading bytes. */
    ktxStream_skip skip;   /*!< @internal pointer to function for skipping bytes. */
    ktxStream_write write; /*!< @internal pointer to function for writing bytes. */
    ktxStream_close close; /*!< @internal pointer to function for closing stream. */

    union {
        FILE* file;
        ktxMem* mem;
    } data;                /**< @internal pointer to the stream data. */
} ktxStream;

#endif /* KTXSTREAM_H */
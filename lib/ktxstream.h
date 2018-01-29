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

#include <sys/types.h>
#include "ktx.h"

/* 
 * This is unsigned to allow ktxmemstreams to use the
 * full amount of memory available. Platforms will
 * limit the size of ktxfilestreams to, e.g, MAX_LONG
 * on 32-bit and ktxfilestreams raises errors if
 * offset values exceed the limits. This choice may
 * need to be revisited if we ever start needing -ve
 * offsets.
 *
 * Should the 2GB file size handling limit on 32-bit
 * platforms become a problem, ktxfilestream will have
 * to be changed to explicitly handle large files by
 * using the 64-bit stream functions.
 */
#if defined(_MSC_VER) && defined(_WIN64)
  typedef unsigned __int64 ktx_off_t;
#else
  typedef   size_t ktx_off_t;
#endif
typedef struct ktxMem ktxMem;
typedef struct ktxStream ktxStream;

enum streamType { eStreamTypeFile, eStreamTypeMemory };

/**
 * @internal
 * @~English
 * @brief type for a pointer to a stream reading function
 */
typedef KTX_error_code (*ktxStream_read)(ktxStream* str, void* dst,
                                         const ktx_size_t count);
/**
 * @internal
 * @~English
 * @brief type for a pointer to a stream skipping function
 */
typedef KTX_error_code (*ktxStream_skip)(ktxStream* str,
	                                     const ktx_size_t count);

/**
 * @internal
 * @~English
 * @brief type for a pointer to a stream reading function
 */
typedef KTX_error_code (*ktxStream_write)(ktxStream* str, const void *src,
                                          const ktx_size_t size,
                                          const ktx_size_t count);

/**
 * @internal
 * @~English
 * @brief type for a pointer to a stream position query function
 */
typedef KTX_error_code (*ktxStream_getpos)(ktxStream* str, ktx_off_t* const offset);

/**
 * @internal
 * @~English
 * @brief type for a pointer to a stream position query function
 */
typedef KTX_error_code (*ktxStream_setpos)(ktxStream* str, const ktx_off_t offset);

/**
 * @internal
 * @~English
 * @brief type for a pointer to a stream size query function
 */
typedef KTX_error_code (*ktxStream_getsize)(ktxStream* str, ktx_size_t* const size);

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
    ktxStream_getpos getpos; /*!< @internal pointer to function for getting current position in stream. */
    ktxStream_setpos setpos; /*!< @internal pointer to function for setting current position in stream. */
    ktxStream_getsize getsize; /*!< @internal pointer to function for querying size. */

    enum streamType type;
    union {
        FILE* file;
        ktxMem* mem;
    } data;                /**< @internal pointer to the stream data. */
} ktxStream;

#endif /* KTXSTREAM_H */

/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2010-2020 The Khronos Group Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file
 * @~English
 *
 * @brief Interface of ktxStream.
 *
 * @author Maksim Kolesin
 * @author Georg Kolling, Imagination Technology
 * @author Mark Callow, HI Corporation
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
  typedef   off_t ktx_off_t;
#endif
typedef struct ktxMem ktxMem;
typedef struct ktxStream ktxStream;

enum streamType { eStreamTypeFile = 1, eStreamTypeMemory = 2 };

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
 * @brief Destruct a stream
 */
typedef void (*ktxStream_destruct)(ktxStream* str);

/**
 * @internal
 * @~English
 * @brief KTX stream class
 */
struct ktxStream
{
    ktxStream_read read;   /*!< @internal pointer to function for reading bytes. */
    ktxStream_skip skip;   /*!< @internal pointer to function for skipping bytes. */
    ktxStream_write write; /*!< @internal pointer to function for writing bytes. */
    ktxStream_getpos getpos; /*!< @internal pointer to function for getting current position in stream. */
    ktxStream_setpos setpos; /*!< @internal pointer to function for setting current position in stream. */
    ktxStream_getsize getsize; /*!< @internal pointer to function for querying size. */
    ktxStream_destruct destruct; /*!< @internal destruct the stream. */

    enum streamType type;
    union {
        FILE* file;
        ktxMem* mem;
    } data;                /**< @internal pointer to the stream data. */
    ktx_off_t readpos;     /**< @internal used by FileStream for stdin. */
    ktx_bool_t closeOnDestruct; /**< @internal Close FILE* or dispose of memory on destruct. */
};

#endif /* KTXSTREAM_H */

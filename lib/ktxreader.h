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

/**
 * @file
 * @internal
 * @~English
 *
 * @brief Declares the functions and structures of a ktxReader.
 *
 * @author Mark Callow
 */

#ifndef KTXREADER_H
#define KTXREADER_H

#include "ktx.h"
#include "ktxint.h"
#include "ktxstream.h"
#include "ktxfilestream.h"
#include "ktxmemstream.h"

typedef enum ktx_reader_state {
    KTX_RS_START,
    KTX_RS_HEADER_READ,
    KTX_RS_KVD_READ,
    KTX_RS_IMAGES_READ,
} ktx_reader_state;


/**
 * @internal
 * @~English
 * @brief ktxReader class
 */
typedef struct ktxReader {
    ktx_reader_state state;  /*!< @internal current context state. */
    ktxStream stream;        /*!< @internal stream representing the KTX file. */
    ktx_uint8_t selfOpenedFile; /*!< @internal file was opened by ktxReader. */
    KTX_header header;       /*!< @internal header of the KTX file. */
    int textureDimension;    /*!< @internal number of dimensions in the texture images. */
} ktxReader;

void ktxReader_construct(ktxReader* This);
KTX_error_code ktxReader_constructFromFILE(ktxReader* This, FILE* file);
KTX_error_code ktxReader_constructFromMem(ktxReader* This,
                                          const void* bytes, size_t size);

#endif /* KTXREADER_H */

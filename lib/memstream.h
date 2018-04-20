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

#ifndef MEMSTREAM_H
#define MEMSTREAM_H

#include "ktx.h"
#include "stream.h"

/*
 * Initialize a ktxStream to a ktxMemStream with internally
 * allocated memory. Can be read or written.
 */
KTX_error_code ktxMemStream_construct(ktxStream* str,
                                      ktx_bool_t freeOnDestruct);
/*
 * Initialize a ktxStream to a read-only ktxMemStream reading
 * from an array of bytes.
 */
KTX_error_code ktxMemStream_construct_ro(ktxStream* str,
										 const ktx_uint8_t* pBytes,
										 const ktx_size_t size);
void ktxMemStream_destruct(ktxStream* str);

KTX_error_code ktxMemStream_getdata(ktxStream* str, ktx_uint8_t** ppBytes);

#endif /* MEMSTREAM_H */

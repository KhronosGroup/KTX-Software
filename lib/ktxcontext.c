/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/**
 * @file
 * @internal
 * @~English
 *
 * @brief Implementation of ktxContext base class.
 *
 * @author Mark Callow, HI Corporation
 */

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

#include <assert.h>
#include <string.h>

#include "ktxcontext.h"
#include "ktxfilestream.h"

void
ktxContext_init(ktxContext* kc)
{
    assert(kc != NULL);
    kc->state = KTX_CS_START;
    memset(kc, 0, sizeof(ktxContext));
}


KTX_error_code
ktxContext_fileInit(ktxContext* kc, FILE* file)
{
    assert(kc != NULL);
    ktxContext_init(kc);
    return ktxFileStream_init(&kc->stream, file);
}

KTX_error_code
ktxContext_memInit(ktxContext* kc, const void* bytes, size_t size)
{
    assert(kc != NULL);
    ktxContext_init(kc);
   	return ktxMemStream_init(&kc->stream, &kc->mem, bytes, size);
}
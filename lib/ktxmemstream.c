/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/**
 * @file
 * @~English
 *
 * @brief Implementation of ktxStream for memory.
 *
 * @author Maksim Kolesin, Under Development
 * @author Georg Kolling, Imagination Technology
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

#include <string.h>
#include <stdlib.h>

#include "KHR/khrplatform.h"
#include "ktx.h"
#include "ktxint.h"
#include "ktxmemstream.h"

static
int ktxMem_expand(struct ktxMem *mem, const GLsizei newsize)
{
        GLsizei new_alloc_size = mem->alloc_size;
        while (new_alloc_size < newsize) {
                new_alloc_size <<= 1;
        }
        mem->bytes = (unsigned char*)realloc(mem->bytes, new_alloc_size);
        if(!mem->bytes)
        {
                mem->alloc_size = 0;
                mem->used_size = 0;
                return 0;
        }
        mem->alloc_size = new_alloc_size;
}

static
int ktxMemStream_read(void* dst, const GLsizei count, void* src)
{
        struct ktxMem* mem = (struct ktxMem*)src;

        if(!dst || !mem || (mem->pos + count > mem->used_size) || (mem->pos + count < mem->pos))
        {
                return 0;
        }

        memcpy(dst, mem->bytes + mem->pos, count);
        mem->pos += count;

        return 1;
}

static
int ktxMemStream_skip(const GLsizei count, void* src)
{
        struct ktxMem* mem = (struct ktxMem*)src;

        if(!mem || (mem->pos + count > mem->used_size) || (mem->pos + count < mem->pos))
        {
                return 0;
        }

        mem->pos += count;

        return 1;
}

static
int ktxMemStream_write(const void* src, const GLsizei size, const GLsizei count, void* dst)
{
        struct ktxMem* mem = (struct ktxMem*)dst;

        if(!dst || !mem)
        {
                return 0;
        }

        if(mem->alloc_size < mem->used_size + size*count)
        {
                if(!ktxMem_expand(mem, mem->used_size + size*count))
                {
                        return 0;
                }
        }

        memcpy(mem->bytes + mem->used_size, src, size*count);
        mem->used_size += size*count;

        return count;
}

#define KTX_MEM_DEFAULT_ALLOCATED_SIZE 256

int
ktxMemInit(struct ktxStream* stream, struct ktxMem* mem, const void* bytes, GLsizei size)
{
        if (!stream || !mem || size < 0)
        {
                return 0;
        }

        if(!bytes)
        {
                if (size == 0)
                        size = KTX_MEM_DEFAULT_ALLOCATED_SIZE;
                mem->bytes = (unsigned char*)malloc(size);
                if (!mem->bytes)
                        return 0;
                mem->alloc_size = size;
                mem->used_size = 0;
                mem->pos = 0;
        }
        else
        {
                mem->bytes = (unsigned char*)bytes;
                mem->used_size = size;
                mem->alloc_size = size;
                mem->pos = 0;
        }

        stream->src = mem;
        stream->read = ktxMemStream_read;
        stream->skip = ktxMemStream_skip;
        stream->write = ktxMemStream_write;

        return 1;
}

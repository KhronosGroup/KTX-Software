/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/**
 * @file
 * @~English
 *
 * @brief FImplementation of ktxStream for FILE.
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
#include "ktxfilestream.h"

static
int ktxFileStream_read(void* dst, const GLsizei count, void* src)
{
        if (!dst || !src || (fread(dst, count, 1, (FILE*)src) != 1))
        {
                return 0;
        }

        return 1;
}

static
int ktxFileStream_skip(const GLsizei count, void* src)
{
        if (!src || (count < 0) || (fseek((FILE*)src, count, SEEK_CUR) != 0))
        {
                return 0;
        }

        return 1;
}

static
int ktxFileStream_write(const void *src, const GLsizei size, const GLsizei count, void* dst)
{
        if (!dst || !src || (fwrite(src, size, count, (FILE*)dst) != count))
        {
                return 0;
        }

        return count;
}

int ktxFileInit(struct ktxStream* stream, FILE* file)
{
        if (!stream || !file)
        {
                return 0;
        }

        stream->src = (void*)file;
        stream->read = ktxFileStream_read;
        stream->skip = ktxFileStream_skip;
        stream->write = ktxFileStream_write;

        return 1;
}

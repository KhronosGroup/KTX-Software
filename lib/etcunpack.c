/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/* @internal
 * @~English
 * @file
 *
 * Unpack a texture compressed with ETC1
 *
 * @author Mark Callow, HI Corporation.
 *
 * $Revision$
 * $Date::                            $
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
work of the Khronos Group."

THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

#include <stdlib.h>
#include "ktx.h"
#include "ktxint.h"

#if SUPPORT_SOFTWARE_ETC_UNPACK

extern void
decompressBlockDiffFlip(unsigned int block_part1, unsigned int block_part2,
						unsigned char* img, int width, int height,
						int startx, int starty);

static void
readBigEndian4byteWord(khronos_uint32_t* pBlock, const GLubyte *s)
{
	*pBlock = (s[0] << 24) | (s[1] << 16) | (s[2] << 8) | s[3];
}


/* Unpack an ETC1_RGB8_OES format compressed texture */
KTX_error_code
_ktxUnpackETC(const GLubyte* srcETC, GLubyte** dstImage,
			  khronos_uint32_t active_width, khronos_uint32_t active_height)
{
	unsigned int width, height;
	unsigned int block_part1, block_part2;
	GLubyte *newimg;
	unsigned int x, y, xx, yy;
	const GLubyte* src = srcETC;

    /* active_{width,height} show how many pixels contain active data,
	 * (the rest are just for making sure we have a 2*a x 4*b size).
	 */

	/* Compute the full width & height. */
	width = ((active_width+3)/4)*4;
	height = ((active_height+3)/4)*4;

	/* printf("Width = %d, Height = %d\n", width, height); */
	/* printf("active pixel area: top left %d x %d area.\n", active_width, active_height); */

	*dstImage = (GLubyte*)malloc(3*width*height);
	if (!*dstImage) {
		return KTX_OUT_OF_MEMORY;
	}
	

	for(y=0; y<height/4; y++) {
		for(x=0; x<width/4; x++) {
			readBigEndian4byteWord(&block_part1, src);
			src += 4;
			readBigEndian4byteWord(&block_part2, src);
			src += 4;
			decompressBlockDiffFlip(block_part1, block_part2, *dstImage, width, height, 4*x, 4*y);
		}
	}

	/* Ok, now write out the active pixels to the destination image.
	 * (But only if the active pixels differ from the total pixels)
	 */

	if( !(height == active_height && width == active_width) )
	{
		newimg = (GLubyte*)malloc(3*active_width*active_height);
		if (!newimg)
		{
			free(*dstImage);
			return KTX_OUT_OF_MEMORY;
		}
		
		/* Convert from total area to active area: */

		for(yy = 0; yy<active_height; yy++)
		{
			for(xx = 0; xx< active_width; xx++)
			{
				newimg[ (yy*active_width)*3 + xx*3 + 0 ] = (*dstImage)[ (yy*width)*3 + xx*3 + 0];
				newimg[ (yy*active_width)*3 + xx*3 + 1 ] = (*dstImage)[ (yy*width)*3 + xx*3 + 1];
				newimg[ (yy*active_width)*3 + xx*3 + 2 ] = (*dstImage)[ (yy*width)*3 + xx*3 + 2];
			}
		}

		free(*dstImage);
		*dstImage = newimg;
	}
	return KTX_SUCCESS;
}

#endif /* SUPPORT_SOFTWARE_ETC_UNPACK */

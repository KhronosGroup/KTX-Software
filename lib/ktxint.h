/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/* $Revision$ on $Date::                            $ */

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

/* 
 * Author: Mark Callow from original code by Georg Kolling
 */

#ifndef _KTXINT_H_
#define _KTXINT_H_

/* Define this to include the ETC unpack software in the library. */
#define SUPPORT_SOFTWARE_ETC_UNPACK 1

#ifdef __cplusplus
extern "C" {
#endif


#define KTX_IDENTIFIER_REF  { 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A }
#define KTX_ENDIAN_REF      (0x04030201)
#define KTX_ENDIAN_REF_REV  (0x01020304)
#define KTX_HEADER_SIZE		(64)

/**
 * @internal
 * @brief KTX file header
 *
 * See the KTX specification for descriptions
 * 
 */
typedef struct KTX_header_t {
	khronos_uint8_t  identifier[12];
	khronos_uint32_t endianness;
	khronos_uint32_t glType;
	khronos_uint32_t glTypeSize;
	khronos_uint32_t glFormat;
	khronos_uint32_t glInternalFormat;
	khronos_uint32_t glBaseInternalFormat;
	khronos_uint32_t pixelWidth;
	khronos_uint32_t pixelHeight;
	khronos_uint32_t pixelDepth;
	khronos_uint32_t numberOfArrayElements;
	khronos_uint32_t numberOfFaces;
	khronos_uint32_t numberOfMipmapLevels;
	khronos_uint32_t bytesOfKeyValueData;
} KTX_header;

/* This will cause compilation to fail if the struct size doesn't match */
typedef int KTX_header_SIZE_ASSERT [sizeof(KTX_header) == KTX_HEADER_SIZE];

/**
 * @internal
 * @brief _ktxCheckHeader returns texture information in this structure
 *
 * TO DO: document properly
 */
typedef struct KTX_texinfo_t {
	/* Data filled in by _ktxCheckHeader() */
	khronos_uint32_t textureDimensions;
	khronos_uint32_t glTarget;
	khronos_uint32_t compressed;
	khronos_uint32_t generateMipmaps;
} KTX_texinfo;

#ifndef GL_TEXTURE_1D
#define GL_TEXTURE_1D                    0x0DE0
#endif
#ifndef GL_TEXTURE_3D
#define GL_TEXTURE_3D                    0x806F
#endif
#ifndef GL_TEXTURE_CUBE_MAP
#define GL_TEXTURE_CUBE_MAP              0x8513
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X   0x8515
#endif
/* from GL_EXT_texture_array */
#ifndef GL_TEXTURE_1D_ARRAY_EXT
#define GL_TEXTURE_1D_ARRAY_EXT          0x8C18
#define GL_TEXTURE_2D_ARRAY_EXT          0x8C1A
#endif
#ifndef GL_GENERATE_MIPMAP
#define GL_GENERATE_MIPMAP               0x8191
#endif

#ifndef GL_UNSIGNED_SHORT_5_6_5
#define GL_UNSIGNED_SHORT_5_6_5          0x8363
#endif
#ifndef GL_UNSIGNED_SHORT_4_4_4_4
#define GL_UNSIGNED_SHORT_4_4_4_4        0x8033
#endif
#ifndef GL_UNSIGNED_SHORT_5_5_5_1
#define GL_UNSIGNED_SHORT_5_5_5_1        0x8034
#endif

#ifndef GL_ETC1_RGB8_OES
#define GL_ETC1_RGB8_OES				 0x8D64
#endif


#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

/* CheckHeader
 * 
 * Reads the KTX file header and performs some sanity checking on the values
 */
KTX_error_code _ktxCheckHeader(KTX_header* header, KTX_texinfo* texinfo);

/*
 * SwapEndian16: Swaps endianness in an array of 16-bit values
 */
void _ktxSwapEndian16(khronos_uint16_t* pData16, int count);

/*
 * SwapEndian32: Swaps endianness in an array of 32-bit values
 */
void _ktxSwapEndian32(khronos_uint32_t* pData32, int count);

/*
 * UncompressETC: uncompresses an ETC compressed texture image
 */
KTX_error_code _ktxUnpackETC(const GLubyte *srcETC, GLubyte **dstImage,
							 khronos_uint32_t active_width,
							 khronos_uint32_t active_height);

#ifdef __cplusplus
}
#endif

#endif /* _KTXINT_H_ */
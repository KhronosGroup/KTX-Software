// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4:

//!
//! @internal
//! @~English
//! @file
//!
//! @brief Read netpbm format (.pam, .pbm or .pgm) files.
//!
//! @author Mark Callow, HI Corporation.
//! @author Jacob Str&oumlm, Ericsson AB.
//!
//! $Revision$
//! $Date::                            $
//!

//
// Copyright (c) 2010 The Khronos Group Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and/or associated documentation files (the
// "Materials"), to deal in the Materials without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Materials, and to
// permit persons to whom the Materials are furnished to do so, subject to
// the following conditions:
// 
// The above copyright notice and this permission notice shall be included
// unaltered in all copies or substantial portions of the Materials.
// Any additions, deletions, or changes to the original source files
// must be clearly indicated in accompanying documentation.
// 
// If only executable code is distributed, then the accompanying
// documentation must state that "this software is based in part on the
// work of the Khronos Group."
// 
// THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
//

#include "stdafx.h"
#include <stdlib.h>
#include "image.h"

static int tupleSize(const char* tupleType);
static FileResult readImage(FILE* src, unsigned int imageSize, unsigned char*& pixels);

// Skips over comments in a netpbm file
// (i.e., lines starting with #)
//
// Written by Jacob Strom
//
static
void skipComments(FILE *src)
{
	int c;

	while((c = getc(src)) == '#')
	{
		char line[1024];
		fgets(line, 1024, src);
	}
	ungetc(c, src);
}


// Skips over white spaces in a netpbm file
//
// Written by Jacob Strom
//
static
void skipSpaces(FILE *src)
{
	int c;

	c = getc(src);
	while(c == ' ' || c == '\t' || c == '\n' || c == '\f' || c == '\r')
	{
		c = getc(src);
	}
	ungetc(c, src);
}


// Skips over intervening non-data elements in a netpbm file
static
void skipNonData(FILE *src)
{
	skipSpaces(src);
	skipComments(src);
	skipSpaces(src);
}


//!
//! @internal
//! @~English
//! @brief Read a netpbm file, either PAM, PGM or PPM
//!
//! The file type is determined from the magic number.
//! P5 is a PGM file. P6 is a PPM binary file, P7 is a PAM file.
//!
//! @param [in]  src		pointer to FILE stream to read
//! @param [out] width		reference to variable in which to store the image width
//! @param [out] height		reference to variable in which to store the image height
//! @param [out] components	reference to variable in which to store the number of
//!                         components in an image pixel
//! @param [out] componentSize
//!                         reference to variable in which to store the size in bytes
//!                         of each component of a pixel.
//! @param [out] imageSize  reference to variable in which to store the size in bytes
//!                         of the image.
//! @param [out] pixels		reference to variable in which to store a pointer to the
//!                         image's pixels.
//!
//! @return an error indicator or SUCCESS
//!
//! @exception INVALID_FORMAT the file is not in .pam, .pgm or .ppm format
//!
//! @author Mark Callow
//!
FileResult
readNPBM(FILE* src, unsigned int& width, unsigned int& height,
		 unsigned int& components, unsigned int& componentSize,
		 unsigned int &imageSize, unsigned char*& pixels)
{
	char line[255];

	skipNonData(src);

	fscanf(src, "%s", line);

	if (strcmp(line, "P6") == 0) {
		return readPPM(src, width, height, components, componentSize, imageSize, pixels);
	} else if (strcmp(line, "P5") == 0) {
		components = 1;
		return readPGM(src, width, height, components, componentSize, imageSize, pixels);
	} else if (strcmp(line, "P7") == 0) {
		return readPAM(src, width, height, components, componentSize, imageSize, pixels);
	} else
		return INVALID_FORMAT;
}


//!
//! @internal
//! @~English
//! @brief Read a PPM file with P6 header
//!
//! P6 indicates binary, as opposed to P5, which is ASCII format. The header must
//! look like this:
//! 
//! P6
//! # Comments (not necessary)
//! width height
//! 255
//!
//! after that follows RGBRGBRGB...
//!
//! @param [in]  src		pointer to FILE stream to read
//! @param [out] width		reference to variable in which to store the image width
//! @param [out] height		reference to variable in which to store the image height
//! @param [out] components	reference to variable in which to store the number of
//!                         components in an image pixel. Always set to 3.
//! @param [out] componentSize
//!                         reference to variable in which to store the size in bytes
//!                         of each component of a pixel. Set to 1 or 2.
//! @param [out] imageSize  reference to variable in which to store the size in bytes
//!                         of the image.
//! @param [out] pixels		reference to variable in which to store a pointer to the
//!                         image's pixels.
//!
//! @return an error indicator or SUCCESS
//!
//! @exception INVALID_VALUE the width or height is < 0 or the maxval value is not
//!                          between 1 and 65535.
//! @exception OUT_OF_MEMORY not enough memory to allocate a buffer for the file
//!                          contents.
//! @exception UNEXPECTED_EOF not enough bytes in the file for the specified image
//!                           size.
//!
//! @author Jacob Str&ouml;m
//! @author Mark Callow
//!
FileResult
readPPM(FILE* src, unsigned int& width, unsigned int& height,
		unsigned int& components, unsigned int& componentSize,
		unsigned int &imageSize, unsigned char*& pixels)
{
	int maxval;

	skipNonData(src);
	
	fscanf(src, "%d %d", &width, &height);
	if( width<=0 || height <=0)
	{
		fprintf(stderr, "Error: width or height negative.\n");
		fclose(src);
		return INVALID_VALUE;
	}

	skipNonData(src);

	components = 3;

	fscanf(src, "%d", &maxval);
	if (maxval <= 0 || maxval >= (1<<16)) {
		fprintf(stderr, "Error: Color resolution must be > 0 && < 65536.\n");
		return INVALID_VALUE;
	}
	//fprintf(stderr, "maxval is %d\n",maxval);
	if (maxval > 255)
		componentSize=2;
	else
		componentSize=1;

	// We need to remove the newline.
	char c = 0;
	while(c != '\n')
		fscanf(src, "%c", &c);
	
	imageSize = width * height * components * componentSize;
	return readImage(src, imageSize, pixels);
}


//!
//! @internal
//! @~English
//! @brief Read a PGM file with P5 header
//!
//! The header must look like this:
//! 
//! P5
//! # Comments if you want to
//! width height
//! 255
//!
//! then follows GRAYGRAYGRAYGRAY...
//!
//! @param [in]  src		pointer to FILE stream to read
//! @param [out] width		reference to variable in which to store the image width
//! @param [out] height		reference to variable in which to store the image height
//! @param [out] components	reference to variable in which to store the number of
//!                         components in an image pixel. Always set to 1.
//! @param [out] componentSize
//!                         reference to variable in which to store the size in bytes
//!                         of each component of a pixel. Set to 1 or 2.
//! @param [out] imageSize  reference to variable in which to store the size in bytes
//!                         of the image.
//! @param [out] pixels		reference to variable in which to store a pointer to the
//!                         image's pixels.
//!
//! @return an error indicator or SUCCESS
//!
//! @exception INVALID_VALUE the width or height is < 0 or the maxval value is not
//!                          between 1 and 65535.
//! @exception OUT_OF_MEMORY not enough memory to allocate a buffer for the file
//!                          contents.
//! @exception UNEXPECTED_EOF not enough bytes in the file for the specified image
//!                           size.
//!
//! @author Jacob Str&ouml;m
//! @author Mark Callow
//!
FileResult
readPGM(FILE* src, unsigned int& width, unsigned int& height,
		unsigned int& components, unsigned int& componentSize,
		unsigned int &imageSize, unsigned char*& pixels)
{
	int maxval;

	skipNonData(src);
	fscanf(src,"%d %d", &width, &height);
	if (width<=0 || height<=0)
	{
		fprintf(stderr, "Error: width and height of the image must be greater than zero.\n");
		return INVALID_VALUE;
	}
	skipNonData(src);

	components = 1;

	fscanf(src,"%d",&maxval);
	if (maxval <= 0 || maxval >= (1<<16)) {
		fprintf(stderr, "Error: maxval must be > 1 && < 65536.\n");
		return INVALID_VALUE;
	}
	if (maxval>255)
		componentSize = 2;
	else
		componentSize = 1;

	/* gotta eat the newline too */
	char ch=0;
	while(ch!='\n') fscanf(src,"%c",&ch);

	imageSize = width * height * componentSize;
	return readImage(src, imageSize, pixels);
}


//!
//! @internal
//! @~English
//! @brief Read a PAM file with P7 header
//!
//! The header must look like this:
//! 
//! P7
//! # Comments if you want to
//! WIDTH nnn
//! HEIGHT nnn
//! DEPTH n
//! MAXVAL nnn
//! TUPLTYPE nnn
//! ENDHDR
//!
//! then follows TUPLETUPLETUPLETUPLE...
//!
//! @param [in]  src		pointer to FILE stream to read
//! @param [out] width		reference to variable in which to store the image width
//! @param [out] height		reference to variable in which to store the image height
//! @param [out] components	reference to variable in which to store the number of
//!                         components in an image pixel.
//! @param [out] componentSize
//!                         reference to variable in which to store the size in bytes
//!                         of each component of a pixel.
//! @param [out] imageSize  reference to variable in which to store the size in bytes
//!                         of the image.
//! @param [out] pixels		reference to variable in which to store a pointer to the
//!                         image's pixels.
//!
//! @return an error indicator or SUCCESS
//!
//! @exception INVALID_VALUE the width or height is < 0 or the maxval value is not
//!                          between 1 and 65535.
//! @exception OUT_OF_MEMORY not enough memory to allocate a buffer for the file
//!                          contents.
//! @exception UNEXPECTED_EOF not enough bytes in the file for the specified image
//!                           size.
//!
//! @author Mark Callow
//!
FileResult
readPAM(FILE* src, unsigned int& width, unsigned int& height,
		unsigned int& components, unsigned int& componentSize,
		unsigned int &imageSize, unsigned char*& pixels)
{
	char line[255];
	char tupleType[20];	// If you change this, change the width in the sscanf below.
	int maxval, depth;
	int numFieldsFound = 0;

	for (;;) {
		skipNonData(src);
		if (!fgets(line, sizeof(line), src)) {
			if (feof(src))
				return UNEXPECTED_EOF;
			else
				return IO_ERROR;
		}
		if (strcmp(line, "ENDHDR\n") == 0)
			break;

		if (sscanf(line, "HEIGHT %d", &height))
			numFieldsFound++;
		else if (sscanf(line, "WIDTH %d", &width))
			numFieldsFound++;
		else if (sscanf(line, "DEPTH %d", &depth))
			numFieldsFound++;
		else if (sscanf(line, "MAXVAL %d", &maxval))
			numFieldsFound++;
		else if (sscanf(line, "TUPLTYPE %20s", tupleType))
			numFieldsFound++;
	};

	if (numFieldsFound < 5)
		return INVALID_PAM_HEADER;

    if ((components = tupleSize(tupleType)) < 1)
		return INVALID_TUPLETYPE;

	if (components != depth)
		return INVALID_VALUE;

	if (maxval <= 0 || maxval >= (1<<16)) {
		fprintf(stderr, "Error: maxval must be > 1 && < 65536.\n");
		return INVALID_VALUE;
	}
	if (maxval > 255)
		componentSize = 2;
	else
		componentSize = 1;

	imageSize = width * height * components * componentSize;
    return readImage(src, imageSize, pixels);
}


static int
tupleSize(const char* tupleType)
{
	if (strcmp(tupleType, "BLACKANDWHITE") == 0)
		return -1;
	else if (strcmp(tupleType, "GRAYSCALE") == 0)
		return 1;
	else if (strcmp(tupleType, "GRAYSCALE_ALPHA") == 0)
		return 2;
	else if (strcmp(tupleType, "RGB") == 0)
		return 3;
	else if (strcmp(tupleType, "RGB_ALPHA") == 0)
		return 4;
	else
		return -1;
}


static FileResult
readImage(FILE* src, unsigned int imageSize, unsigned char*& pixels)
{
	pixels = new unsigned char[imageSize];
	if (!pixels)
	{
		fprintf(stderr, "Error: could not allocate memory for the pixels of the texture.\n");
		return OUT_OF_MEMORY;	 
	}

	if (fread(pixels, imageSize, 1, src) != 1)
	{
		fprintf(stderr, "Error: could not read %d bytes of pixel data.\n",imageSize);
		free(pixels);
		pixels = 0;
		return UNEXPECTED_EOF;
	}
	return SUCCESS;
}

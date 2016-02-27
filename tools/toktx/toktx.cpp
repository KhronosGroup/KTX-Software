// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4:

//!
//! @~English
//! @mainpage
//!
//! toktx: create a KTX file from netpbm  (.pam, .pgm, .ppm) format files.
//!
//! @author Mark Callow, HI Corporation, www.hicorp.co.jp
//!
//! @version 1.1
//! $Date$

// $Id$

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

// To use, download from http://www.billbaxter.com/projects/imdebug/
// Put imdebug.dll in %SYSTEMROOT% (usually C:\WINDOWS), imdebug.h in
// ../../include, imdebug.lib in ../../build/vc9 & add ..\imdebug.lib
// to the libraries list in the project properties.
#define IMAGE_DEBUG 0

#include "GL/glcorearb.h"
#include "stdafx.h"
#include "ktx.h"
#include "image.h"
#include <cstdlib>
#if (IMAGE_DEBUG) && defined(_DEBUG) && defined(_WIN32) && !defined(_WIN32_WCE)
#  include "imdebug.h"
#elif defined(IMAGE_DEBUG) && IMAGE_DEBUG
#  undef IMAGE_DEBUG
#  define IMAGE_DEBUG 0
#endif

#define ALLOW_LEGACY_FORMAT_CREATION 1

#if ALLOW_LEGACY_FORMAT_CREATION
#if !defined(GL_LUMINANCE)
#define GL_LUMINANCE					0x1909
#define GL_LUMINANCE_ALPHA				0x190A
#endif
#if !defined(GL_LUMINANCE4)
#define GL_ALPHA4						0x803B
#define GL_ALPHA8						0x803C
#define GL_ALPHA12						0x803D
#define GL_ALPHA16						0x803E
#define GL_LUMINANCE4					0x803F
#define GL_LUMINANCE8					0x8040
#define GL_LUMINANCE12					0x8041
#define GL_LUMINANCE16					0x8042
#define GL_LUMINANCE4_ALPHA4			0x8043
#define GL_LUMINANCE6_ALPHA2			0x8044
#define GL_LUMINANCE8_ALPHA8			0x8045
#define GL_LUMINANCE12_ALPHA4			0x8046
#define GL_LUMINANCE12_ALPHA12			0x8047
#define GL_LUMINANCE16_ALPHA16			0x8048
#endif
#if !defined(GL_SLUMINANCE)
#define GL_SLUMINANCE_ALPHA				0x8C44
#define GL_SLUMINANCE8_ALPHA8			0x8C45
#define GL_SLUMINANCE					0x8C46
#define GL_SLUMINANCE8					0x8C47
#endif
#endif /* ALLOW_LEGACY_FORMAT_CREATION */

#if !defined(GL_RED)
#define GL_RED							0x1903
#define GL_RGB8							0x8051
#define GL_RGB16						0x8054
#define GL_RGBA8						0x8058
#define GL_RGBA16						0x805B
#endif
#if !defined(GL_RG)
#define GL_RG							0x8227
#define GL_R8							0x8229
#define GL_R16							0x822A
#define GL_RG8							0x822B
#define GL_RG16							0x822C
#endif


struct commandOptions {
	_TCHAR*		 appName;
	bool		 alpha;
	bool		 automipmap;
	bool		 cubemap;
	bool		 luminance;
	bool		 mipmap;
	bool		 sized;
	bool		 useStdin;
	bool		 lower_left_maps_to_s0t0;
	_TCHAR*		 outfile;
	int			 numInputFiles;
	unsigned int firstInfileIndex;
};

static khronos_uint32_t log2(khronos_uint32_t v);
static void processCommandLine(int argc, _TCHAR* argv[],
                               struct commandOptions& options);
static bool processOption(const _TCHAR* option, struct commandOptions& options);
static void yflip(unsigned char*& srcImage, unsigned int imageSize,
                  unsigned int w, unsigned int h, unsigned int pixelSize);
#if IMAGE_DEBUG
static void dumpImage(_TCHAR* name, int width, int height, int components,
                      int componentSize, bool isLuminance,
                      unsigned char* srcImage);
#endif

static void
usage(_TCHAR* appName)
{
	fprintf(stderr, 
		"Usage: %s [options] <outfile> [<infile>.{pam,pgm,ppm} ...]\n"
		"\n"
	    "  <outfile>    The destination ktx file. \".ktx\" will appended if necessary.\n"
	    "               If it is '-' the output will be written to stdout.\n"
        "  <infile>     One or more image files in .pam, .ppm or .pgm format. Other\n"
		"               formats can be readily converted to these formats using tools\n"
		"               such as ImageMagick and XnView. When no infile is specified,\n"
		"               stdin is used. .ppm files yield RGB textures, .pgm files RED\n"
		"               textures and .pam files RED, RG, RGB or RGBA textures according\n"
		"               to the file's TUPLTYPE and DEPTH.\n"
		"\n"
        "  Options are:\n"
		"\n"
#if ALLOW_LEGACY_FORMAT_CREATION
		"  --alpha      Create ALPHA textures from .pgm or 1 channel GRAYSCALE .pam\n"
		"               infiles. The default is to create RED textures. This is ignored\n"
		"               for files with 2 or more channels. This option is mutually\n"
		"               exclusive with --luminance.\n"
#endif
        "  --automipmap A mipmap pyramid will be automatically generated when the KTX\n"
        "               file is loaded. This option is mutually exclusive with --mipmap.\n"
        "  --cubemap    KTX file is for a cubemap. At least 6 <infile>s must be provided,\n"
        "               more if --mipmap is also specified. Provide the images in the\n"
		"               order: +X, -X, +Y, -Y, +Z, -Z.\n"
#if ALLOW_LEGACY_FORMAT_CREATION
        "  --luminance  Create LUMINANCE or LUMINANCE_ALPHA textures from .pgm and\n"
		"               1 or 2 channel GRAYSCALE .pam infiles. The default is to create\n"
		"               RED or RG textures. This option is mutually exclusive with\n"
		"               --alpha.\n"
#endif
        "  --mipmap     KTX file is for a mipmap pyramid. One <infile> per level must\n"
        "               be provided. Provide the base-level image first then in order\n"
		"               down to the 1x1 image. This option is mutually exclusive with\n"
		"               --automipmap.\n"
		"  --sized      Set the texture's internal format to a sized format based on\n"
		"               the component size of the input file. Otherwise set it to an\n"
		"               unsized internal format.\n"
        "  --upper_left_maps_to_s0t0\n"
		"               Map the logical upper left corner of the image to s0,t0.\n"
		"               Although opposite to the OpenGL convention, this is the DEFAULT\n"
		"               BEHAVIOUR. netpbm files have an upper left origin so this option\n"
		"               means do nothing. When this option is in effect, toktx writes a\n"
		"               KTXorientation value of S=r,T=d into the output file so that\n"
		"               loaders can tell that the logical orientation is different from\n"
		"               GL.\n"
		"  --lower_left_maps_to_s0t0\n"
		"               Map the logical lower left corner of the image to s0,t0.\n"
        "               This causes the input netpbm files to be flipped vertically to\n"
		"               OpenGL's lower-left origin.\n"
        "  --help       Print this usage message and exit.\n"
        "  --version    Print the version number of this program and exit.\n"
		"\n"
		"Options can also be set in the environment variable TOKTX_OPTIONS.\n"
		"TOKTX_OPTIONS is parsed first. If conflicting options appear in TOKTX_OPTIONS\n"
		"or the command line, the last one seen wins. However if both --automipmap and\n"
		"--mipmap are seen, it is always flagged as an error. You can, for example,\n"
		"set TOKTX_OPTIONS=--lower_left_maps_to_s0t0 to change the default mapping of\n"
		"the logical image origin to match the GL convention.\n",
		appName);
}


static void
version(_TCHAR* appName)
{
	fprintf(stderr, "%s version 1.1\n", appName);
}


int _tmain(int argc, _TCHAR* argv[])
{
	FILE* f;
	KTX_error_code ret;
	KTX_texture_info tinfo;
	KTX_image_info* infiles;
	struct commandOptions options;
	unsigned int imageSize;
	int exitCode = 0, face, i;
	unsigned int levelWidth, levelHeight;
	FileResult readResult;
	bool useStdin = false;
	unsigned char* kvData = NULL;
	unsigned int kvDataLen = 0;

	processCommandLine(argc, argv, options);

	infiles = new KTX_image_info[options.numInputFiles];

	if (options.cubemap)
		tinfo.numberOfFaces = 6;
	else
		tinfo.numberOfFaces = 1;

	// TO DO: handle array textures
	tinfo.numberOfArrayElements = 0;

	// TO DO: handle 3D textures. Concatenate the files here or in WriteKTXF?

	if (!options.lower_left_maps_to_s0t0) {
		// Non-standard orientation. Add metadata.
		KTX_hash_table ht = ktxHashTable_Create();
		char orientation[10];

		assert(strlen(KTX_ORIENTATION2_FMT) < sizeof(orientation));
		_snprintf(orientation, sizeof(orientation), KTX_ORIENTATION2_FMT, 'r', 'd');
		ktxHashTable_AddKVPair(ht, KTX_ORIENTATION_KEY, (unsigned int)strlen(orientation) + 1,
							   orientation);
		if (KTX_SUCCESS != ktxHashTable_Serialize(ht, &kvDataLen, &kvData)) {
			fprintf(stderr, "%s: Out of memory\n", options.appName);
			exit(2);
		}
		ktxHashTable_Destroy(ht);
	}

	for (i = 0; i < options.numInputFiles; i++) {
		_TCHAR* infile;

		if (options.useStdin) {
			infile = 0;
			f = stdin;
#if defined(_WIN32)
            /* Set "stdin" to have binary mode */
            (void)_setmode( _fileno( stdin ), _O_BINARY );
#endif
		} else {
			infile = argv[options.firstInfileIndex + i];
			f = fopen(infile,"rb");
		}

		if (f) {
			unsigned int w, h, components, componentSize;
			unsigned char* srcImg;

			readResult = readNPBM(f, w, h, components, componentSize, imageSize, srcImg);
			if (SUCCESS == readResult) {

				/* Sanity check. */
				assert(w * h * componentSize * components == imageSize);

#if IMAGE_DEBUG
				dumpImage(infile, w, h, components, componentSize, options.luminance, srcImg);
#endif

				if (h > 1 && options.lower_left_maps_to_s0t0)
					yflip(srcImg, imageSize, w, h, components*componentSize);

				infiles[i].size = imageSize;
				infiles[i].data = srcImg;

				if (i == 0) {
					if (componentSize == 1) {
						tinfo.glType = GL_UNSIGNED_BYTE;
						tinfo.glTypeSize = 1;
					} else {
						tinfo.glType = GL_UNSIGNED_SHORT;
						tinfo.glTypeSize = 2;
					}
					switch (components) {
					  case 1:
						if (ALLOW_LEGACY_FORMAT_CREATION && options.luminance) {
							tinfo.glFormat = tinfo.glBaseInternalFormat = GL_LUMINANCE;
							if (options.sized)
								tinfo.glInternalFormat = componentSize == 1 ? GL_LUMINANCE8 : GL_LUMINANCE16;
							else
								tinfo.glInternalFormat = GL_LUMINANCE;
						} else if (ALLOW_LEGACY_FORMAT_CREATION && options.alpha) {
							tinfo.glFormat = tinfo.glBaseInternalFormat = GL_ALPHA;
							if (options.sized)
								tinfo.glInternalFormat = componentSize == 1 ? GL_ALPHA8 : GL_ALPHA16;
							else
								tinfo.glInternalFormat = GL_ALPHA;
						} else {
							tinfo.glFormat = tinfo.glBaseInternalFormat = GL_RED;
							if (options.sized)
								tinfo.glInternalFormat = componentSize == 1 ? GL_R8 : GL_R16;
							else
								tinfo.glInternalFormat = GL_RED;
						}
						break;

					  case 2:
						if (ALLOW_LEGACY_FORMAT_CREATION && options.luminance) {
							tinfo.glFormat = tinfo.glBaseInternalFormat = GL_LUMINANCE_ALPHA;
							if (options.sized)
								tinfo.glInternalFormat = componentSize == 1 ? GL_LUMINANCE8_ALPHA8 : GL_LUMINANCE16_ALPHA16;
							else
								tinfo.glInternalFormat = GL_LUMINANCE_ALPHA;
						} else {
							tinfo.glFormat = tinfo.glBaseInternalFormat = GL_RG;
							if (options.sized)
								tinfo.glInternalFormat = componentSize == 1 ? GL_RG8 : GL_RG16;
							else
								tinfo.glInternalFormat = GL_RG;
						}
						break;

					  case 3:
						tinfo.glFormat = tinfo.glBaseInternalFormat = GL_RGB;
						if (options.sized)
							tinfo.glInternalFormat = componentSize == 1 ? GL_RGB8 : GL_RGB16;
						else
							tinfo.glInternalFormat = GL_RGB;
						break;

					  case 4:
						tinfo.glFormat = tinfo.glBaseInternalFormat = GL_RGBA;
						if (options.sized)
							tinfo.glInternalFormat = componentSize == 1 ? GL_RGBA8 : GL_RGBA16;
						else
							tinfo.glInternalFormat = GL_RGBA;
						break;
						break;

					  default:
					    /* If we get here there's a bug in readPAM */
					    assert(0);
					}
					tinfo.pixelWidth = levelWidth = w;
					tinfo.pixelHeight = levelHeight = h;
					tinfo.pixelDepth = 0;
					if (h == 1 && kvData != NULL) {
						/* 1D. Don't need orientation metadata */
						delete(kvData);
						kvData = NULL;
						kvDataLen = 0;
					}
					face = tinfo.numberOfFaces;
					if (options.automipmap)
						tinfo.numberOfMipmapLevels = 0;
					else if (options.mipmap) {
						// Calculate number of miplevels
						GLuint max_dim = w > h ? w : h;
						GLint levels = log2(max_dim) + 1;
						// Check we have enough.
						if (levels * face > options.numInputFiles) {
							fprintf(stderr, "%s: not enough input files for %d mipmap levels and faces\n",
								    options.appName, levels);
							exitCode = 1;
							goto cleanup2;
						}
						tinfo.numberOfMipmapLevels = levels;
					} else
						tinfo.numberOfMipmapLevels = 1;
				} else {
					if (options.mipmap) {
						if (face == 0) {
							levelWidth >>= 1;
							levelHeight >>= 1;
							if (w != levelWidth || h != levelHeight) {
								fprintf(stderr, "%s: \"%s\" has incorrect width or height for current mipmap level\n",
									    options.appName, infile);
								exitCode = 1;
								goto cleanup2;
							}
							face = options.cubemap ? 6 : 1;
						}
					}
				}
				face--;
				if (options.cubemap && w != h && w != levelWidth) {
						fprintf(stderr, "%s: \"%s,\" intended for a cubemap face, is not square or has incorrect\n"
							            "size for current mipmap level\n",
							    options.appName, infile);
						exitCode = 1;
						goto cleanup2;
				}
			} else {
				fprintf(stderr, "%s: \"%s\" is not a valid .pam, .pgm or .ppm file\n",
					    options.appName, infile ? infile : "data from stdin");
				exitCode = 1;
				goto cleanup1;
			}
			(void)fclose(f);
		} else {
			fprintf(stderr, "%s could not open input file \"%s\". %s\n",
				    options.appName, infile ? infile : "stdin", strerror(errno));
			exitCode = 2;
			goto cleanup1;
		}
	}

	if (_tcscmp(options.outfile, "-") == 0) {
		f = stdout;
#if defined(_WIN32)
        /* Set "stdout" to have binary mode */
        (void)_setmode( _fileno( stdout ), _O_BINARY );
#endif
	} else
		f = fopen(options.outfile,"wb");
	if (f) {
		ret = ktxWriteKTXF(f, &tinfo, kvDataLen, kvData, options.numInputFiles, infiles);
		if (KTX_SUCCESS == ret) {
			fclose(f);
		} else {
			fprintf(stderr, "%s failed to write KTX file \"%s\"; KTX error: %s\n",
					options.appName, options.outfile, ktxErrorString(ret));
			fclose(f);
			if (f != stdout)
				_unlink(options.outfile);
			exitCode = 2;
		}
	
	} else {
		fprintf(stderr, "%s: could not open output file \"%s\". %s\n",
			    options.appName, options.outfile, strerror(errno));
		exitCode = 2;
	}

cleanup1:
	// File data deletion code assumes infiles[i] points to  a successfully
	// opened file.
	i--;
cleanup2:
	delete(options.outfile);
	// Delete data of successfully opened files
	for (; i >= 0; i--)
		delete infiles[i].data;
	if (kvData)
		delete(kvData);
	delete(infiles);
	return exitCode;
}


static void processCommandLine(int argc, _TCHAR* argv[], struct commandOptions& options)
{
	int i, addktx = 0;
	unsigned int outfilenamelen;
	const _TCHAR* toktx_options;
	_TCHAR option[31];

	options.alpha = false;
	options.automipmap = false;
	options.cubemap = false;
	options.luminance = false;
	options.mipmap = false;
	options.sized = false;
	options.outfile = 0;
	options.numInputFiles = 0;
	options.firstInfileIndex = 0;
	options.useStdin = false;
	/* The OGLES WG recommended approach, even though it is opposite
	 * to the OpenGL convention. Suki dewa nai.
	 */
	options.lower_left_maps_to_s0t0 = false;

	options.appName = _tcsrchr(argv[0], '\\');
	if (options.appName == NULL)
		options.appName = _tcsrchr(argv[0], '/');
	options.appName++;

    // NOTE: If options with arguments are ever added, this option handling
    // code will need revamping.
	toktx_options = _tgetenv(_T("TOKTX_OPTIONS"));
	while (toktx_options && _stscanf(toktx_options, "%30s", (char*)&option) != EOF) {
		if (processOption(option, options) == 0) {
			fprintf(stderr, "Only options are allowed in the TOKTX_OPTIONS environment variable\n");
			usage(options.appName);
			exit(1);
		}
		toktx_options = _tcschr(toktx_options, ' ');
		if (toktx_options)
			while (*toktx_options == ' ') toktx_options++;
	}

    if (argc > 2
        && _tcscmp(argv[argc-2], "-NSDocumentRevisionsDebugMode") == 0
        && _tcscmp(argv[argc-1], "YES") == 0) {
        // -NSDocumentRevisionsDebugMode YES is appended to the end
        // of the command by Xcode when debugging and "Allow debugging when
        // using document Versions Browser" is checked in the scheme. It
        // defaults to checked and is saved in a user-specific file not the
        // pbxproj file so it can't be disabled in a generated project.
        // Remove these from the arguments under consideration.
        argc -= 2;
    }
    for (i = 1; i < argc; i++) {
        if (!processOption(argv[i], options))
			break; // No more options
	}
	
	if (argc - i < 1) {
		usage(options.appName);
		exit(1);
	}
	if (options.cubemap && (argc - i < 7)) {
		usage(options.appName);
		exit(1);
	}

	outfilenamelen = (unsigned int)_tcslen(argv[i]) + 1;
	if (_tcscmp(argv[i], "-") != 0 && _tcsrchr(argv[i], '.') == NULL) {
		addktx = 1;
		outfilenamelen += 4;
	}
	options.outfile = new _TCHAR[outfilenamelen];
	if (options.outfile) {
		_tcscpy(options.outfile, argv[i++]);
		if (addktx)
			_tcscat(options.outfile, ".ktx");
	} else {
		fprintf(stderr, "%s: out of memory.\n", options.appName);
		exit(2);
	}
	options.numInputFiles = argc - i;
	if (options.numInputFiles == 0) {
		options.numInputFiles = 1;
		options.useStdin = true;
	} else {
		options.firstInfileIndex = i;
	    /* Check for attempt to use stdin as one of the
		 * input files.
	     */
		for (i = options.firstInfileIndex; i < argc; i++) {
			if (_tcscmp(argv[i], "-") == 0) {
				usage(options.appName);
				exit(1);
			}
		}
	}
	/* Whether there are enough input files for all the mipmap levels can
	 * only be checked when the first file has been read and the
	 * size determined.
	 */
}


/*
 * @brief process a potential command line option
 * 
 * @return  false, if not an option; true, if it is an option
 *
 * @param[in]    option        a word from the command line.
 * @param[inout] options       commandOptions struct in which option information
 *                             is set.
 */
static bool
processOption(const _TCHAR* option, struct commandOptions& options)
{
	bool retVal = true;

	if (_tcsncmp(option, "--", 2) == 0) {
		if (_tcscmp(&option[2], "help") == 0) {
			usage(options.appName);
			exit(0);
		} else if (_tcscmp(&option[2], "version") == 0) {
			version(options.appName);
			exit(0);
		} else if (_tcscmp(&option[2], "alpha") == 0) {
			if (!ALLOW_LEGACY_FORMAT_CREATION || options.luminance) {
				usage(options.appName);
				exit(1);
			}
			options.alpha = true;
		} else if (_tcscmp(&option[2], "automipmap") == 0) {
			if (options.mipmap) {
				usage(options.appName);
				exit(1);
			} else
				options.automipmap = 1;
		} else if (_tcscmp(&option[2], "mipmap") == 0) {
			if (options.automipmap) {
				usage(options.appName);
				exit(1);
			} else
				options.mipmap = 1;
		} else if (_tcscmp(&option[2], "cubemap") == 0) {
			options.cubemap = true;
		} else if (_tcscmp(&option[2], "luminance") == 0) {
			if (!ALLOW_LEGACY_FORMAT_CREATION || options.alpha) {
				usage(options.appName);
				exit(1);
			}
			options.luminance = true;
		} else if (_tcscmp(&option[2], "sized") == 0) {
			options.sized = true;
		} else if (_tcscmp(&option[2], "upper_left_maps_to_s0t0") == 0) {
			options.lower_left_maps_to_s0t0 = false;
		} else if (_tcscmp(&option[2], "lower_left_maps_to_s0t0") == 0) {
			options.lower_left_maps_to_s0t0 = true;
		} else {
			// unrecognized argument
			usage(options.appName);
			exit(1);
		}
    } else if (option[0] == _T('-') && option[1] != _T('\0')) {
            // old style option specification
            usage(options.appName);
            exit(1);
    } else
		retVal = false;

	return retVal;
}

static khronos_uint32_t
log2(khronos_uint32_t v)
{
	khronos_uint32_t e;

	/* http://aggregate.org/MAGIC/ */
	v |= (v >> 1);
	v |= (v >> 2);
	v |= (v >> 4);
	v |= (v >> 8);
	v |= (v >> 16);
	v = v & ~(v >> 1);

	e = (v & 0xAAAAAAAA) ? 1 : 0;
	e |= (v & 0xCCCCCCCC) ? 2 : 0;
	e |= (v & 0xF0F0F0F0) ? 4 : 0;
	e |= (v & 0xFF00FF00) ? 8 : 0;
	e |= (v & 0xFFFF0000) ? 16 : 0;

	return e;
}


static void
yflip(unsigned char*& srcImage, unsigned int imageSize, unsigned int w, unsigned int h,
	  unsigned int pixelSize)
{
	int rowSize = w * pixelSize;
	unsigned char *flipped, *temp;

	flipped = new unsigned char[imageSize];
	if (!flipped) {
		fprintf(stderr, "Not enough memory\n");
		exit(2);
	}

	for (int sy = h-1, dy = 0; sy >= 0; sy--, dy++) {
		unsigned char* src = &srcImage[rowSize * sy];
		unsigned char* dst = &flipped[rowSize * dy];

		memcpy(dst, src, rowSize);
	}
	temp = srcImage;
	srcImage = flipped;
	delete temp;
}


#if IMAGE_DEBUG
static void
dumpImage(_TCHAR* name, int width, int height, int components, int componentSize,
		  bool isLuminance, unsigned char* srcImage)
{
	char formatstr[2048];
	char *imagefmt;
	char *fmtname;
	int bitsPerComponent = componentSize == 2 ? 16 : 8;

	switch (components) {
	  case 1:
		if (isLuminance) {
			imagefmt = "lum b=";
			fmtname = "LUMINANCE";
		} else {
			imagefmt = "a b=";
			fmtname = "ALPHA";
	    }
		break;
	  case 2:
	  	imagefmt = "luma b=";
		fmtname = "LUMINANCE_ALPHA";
		break;
	  case 3:
	  	imagefmt = "rgb b=";
		fmtname = "RGB";
		break;
	  case 4:
	  	imagefmt = "rgba b=";
		fmtname = "RGBA";
		break;
	  default:
		assert(0);
	}
	sprintf(formatstr, "%s%d w=%%d h=%%d t=\'%s %s%d\' %%p",
			imagefmt,
			bitsPerComponent,
			name,
			fmtname,
			bitsPerComponent);
	imdebug(formatstr, width, height, srcImage);
}
#endif

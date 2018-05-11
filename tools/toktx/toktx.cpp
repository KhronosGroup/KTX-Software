// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 sts=4 expandtab:

// $Id$

//
// ©2010-2018 The Khronos Group, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

// To use, download from http://www.billbaxter.com/projects/imdebug/
// Put imdebug.dll in %SYSTEMROOT% (usually C:\WINDOWS), imdebug.h in
// ../../include, imdebug.lib in ../../build/msvs/<platform>/vs<ver> &
// add ..\imdebug.lib to the libraries list in the project properties.
#define IMAGE_DEBUG 0

#include "stdafx.h"
#include <cstdlib>
#include <sstream>
#include <vector>

#include "GL/glcorearb.h"
#include "ktx.h"
#include "argparser.h"
#include "image.h"
#if (IMAGE_DEBUG) && defined(_DEBUG) && defined(_WIN32) && !defined(_WIN32_WCE)
#  include "imdebug.h"
#elif defined(IMAGE_DEBUG) && IMAGE_DEBUG
#  undef IMAGE_DEBUG
#  define IMAGE_DEBUG 0
#endif

#define ALLOW_LEGACY_FORMAT_CREATION 1

#if ALLOW_LEGACY_FORMAT_CREATION
#if !defined(GL_LUMINANCE)
#define GL_LUMINANCE                    0x1909
#define GL_LUMINANCE_ALPHA              0x190A
#endif
#if !defined(GL_LUMINANCE4)
#define GL_ALPHA4                       0x803B
#define GL_ALPHA8                       0x803C
#define GL_ALPHA12                      0x803D
#define GL_ALPHA16                      0x803E
#define GL_LUMINANCE4                   0x803F
#define GL_LUMINANCE8                   0x8040
#define GL_LUMINANCE12                  0x8041
#define GL_LUMINANCE16                  0x8042
#define GL_LUMINANCE4_ALPHA4            0x8043
#define GL_LUMINANCE6_ALPHA2            0x8044
#define GL_LUMINANCE8_ALPHA8            0x8045
#define GL_LUMINANCE12_ALPHA4           0x8046
#define GL_LUMINANCE12_ALPHA12          0x8047
#define GL_LUMINANCE16_ALPHA16          0x8048
#endif
#if !defined(GL_SLUMINANCE)
#define GL_SLUMINANCE_ALPHA             0x8C44
#define GL_SLUMINANCE8_ALPHA8           0x8C45
#define GL_SLUMINANCE                   0x8C46
#define GL_SLUMINANCE8                  0x8C47
#endif
#endif /* ALLOW_LEGACY_FORMAT_CREATION */

#if !defined(GL_RED)
#define GL_RED                          0x1903
#define GL_RGB8                         0x8051
#define GL_RGB16                        0x8054
#define GL_RGBA8                        0x8058
#define GL_RGBA16                       0x805B
#endif
#if !defined(GL_RG)
#define GL_RG                           0x8227
#define GL_R8                           0x8229
#define GL_R16                          0x822A
#define GL_RG8                          0x822B
#define GL_RG16                         0x822C
#endif


struct commandOptions {
    _TCHAR*      appName;
    int          alpha;
    int          automipmap;
    int          cubemap;
    int          luminance;
    int          metadata;
    int          mipmap;
    int          two_d;
    int          useStdin;
    int          lower_left_maps_to_s0t0;
    _TCHAR*      outfile;
    unsigned int levels;
    unsigned int numInputFiles;
    unsigned int firstInfileIndex;
};

static ktx_uint32_t log2(ktx_uint32_t v);
static void processCommandLine(int argc, _TCHAR* argv[],
                               struct commandOptions& options);
static void processOptions(argparser& parser, struct commandOptions& options);
static void yflip(unsigned char*& srcImage, unsigned int imageSize,
                  unsigned int w, unsigned int h, unsigned int pixelSize);
#if IMAGE_DEBUG
static void dumpImage(_TCHAR* name, int width, int height, int components,
                      int componentSize, bool isLuminance,
                      unsigned char* srcImage);
#endif

/** @page toktx toktx
@~English

Create a KTX file from netpbm format files.
 
@section synopsis SYNOPSIS
    toktx [options] @e outfile [@e infile.{pam,pgm,ppm} ...]

@section description DESCRIPTION
    @b toktx creates Khronos format texture files (KTX) from a set of Netpbm
    format  (.pam, .pgm, .ppm) images. Currently it only supports creating KTX
    files holding 2D and cube map textures. It writes the destination ktx file
    to @e outfile, appending ".ktx" if necessary. If @e outfile is '-' the
    output will be written to stdout.
 
    @b toktx reads each named @e infile which must be in .pam, .ppm or .pgm
    format. Other formats can be readily converted to these formats using
    tools such as ImageMagick and XnView. When no infile is specified, a single
    image will be read from stdin. .ppm files yield RGB textures, .pgm files
    RED textures and .pam files RED, RG, RGB or RGBA textures according to the
    file's TUPLTYPE and DEPTH.
 
    The following options are always available:
    <dl>
    <dt>--2d</dt>
    <dd>If the image height is 1, by default a KTX file for a 1D texture is
        created. With this option one for a 2D texture is created instead.</dd>
    <dt>--automipmap</dt>
    <dd>A mipmap pyramid will be automatically generated when the KTX
        file is loaded. This option is mutually exclusive with @b --levels and
        @b --mipmap.</dd>
    <dt>--cubemap</dt>
    <dd>KTX file is for a cubemap. At least 6 @e infiles must be provided,
        more if --mipmap is also specified. Provide the images in the
        order: +X, -X, +Y, -Y, +Z, -Z.</dd>
    <dt>--levels levels</dt>
    <dd>KTX file is for a mipmap pyramid with @e levels rather than a
        full pyramid. @e levels must be <= the maximum number of levels
        determined from the size of the base image. Provide the base level
        image first. This option is mutually exclusive with @b --automipmap
        and @b --mipmap.
    <dt>--mipmap</dt>
    <dd>KTX file is for a full mipmap pyramid. One @e infile per level must
        be provided. Provide the base-level image first then in order
        down to the 1x1 image. This option is mutually exclusive with
        @b --automipmap and @b --levels.</dd>
    <dt>--nometadata</dt>
    <dd>Do not write KTXorientation metadata into the output file. Metadata
        is written by default. Use of this option is not recommended.</dd>
    <dt>--upper_left_maps_to_s0t0</dt>
    <dd>Map the logical upper left corner of the image to s0,t0.
        Although opposite to the OpenGL convention, this is the DEFAULT
        BEHAVIOUR. netpbm files have an upper left origin so this option
        does not flip the input files. When this option is in effect,
        toktx writes a KTXorientation value of S=r,T=d into the output file
        to inform loaders of the logical orientation. If an OpenGL {,ES}
        loader ignores the orientation value, the image will appear upside
        down.</dd>
    <dt>--lower_left_maps_to_s0t0</dt>
    <dd>Map the logical lower left corner of the image to s0,t0.
        This causes the input netpbm images to be flipped vertically to a
        lower-left origin. When this option is in effect, toktx
        writes a KTXorientation value of S=r,T=u into the output file
        to inform loaders of the logical orientation. If a Vulkan loader
        ignores the orientation value, the image will appear upside down.</dd>
    <dt>--help</dt>
    <dd>Print this usage message and exit.</dd>
    <dt>--version</dt>
    <dd>Print the version number of this program and exit.</dd>
    </dl>
 
    The following options are available if @b toktx was compiled with
    @p ALLOW_LEGACY_FORMAT_CREATION:
    <dl>
    <dt>--alpha</dt>
    <dd>Create ALPHA textures from .pgm or 1 channel GRAYSCALE .pam
        infiles. The default is to create RED textures. This is ignored
        for files with 2 or more channels. This option is mutually
        exclusive with @b --luminance.</dd>
    <dt>--luminance</dt>
    <dd>Create LUMINANCE or LUMINANCE_ALPHA textures from .pgm and
        1 or 2 channel GRAYSCALE .pam infiles. The default is to create
        RED or RG textures. This option is mutually exclusive with
        @b --alpha.</dd>
    </dl>

    Options can also be set in the environment variable TOKTX_OPTIONS.
    TOKTX_OPTIONS is parsed first. If conflicting options appear in
    TOKTX_OPTIONS or the command line, the last one seen wins. However if both
    @b --automipmap and @b --mipmap are seen, it is always flagged as an error.
    You can, for example, set TOKTX_OPTIONS=--lower_left_maps_to_s0t0 to change
    the default mapping of the logical image origin to match the GL convention.

@section exitstatus EXIT STATUS
    @b toktx exits 0 on success, 1 on command line errors and 2 on
    functional errors.

@section toktx_history HISTORY

@version 1.3:
Sat, 28 Apr 2018 14:41:22 +0900
 - Switch to ktxTexture API.
 - Add --levels option.
 - Add --2d option.
 
@version 1.2:
Fri Oct 13 18:15:05 2017 +0900
 - Remove --sized; always create sized format.
 - Write metadata by default.
 - Bug fixes.

@version 1.1:
Sun Dec 25 07:02:41 2016 -0200
 - Moved --alpha and --luminance to legacy.

@section author AUTHOR
    Mark Callow, Edgewise Consulting www.edgewise-consulting.com
*/

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
        "  --2d         If the image height is 1, by default a KTX file for a 1D\n"
        "               texture is created. With this option one for a 2D texture is\n"
        "               created instead.\n"
#if ALLOW_LEGACY_FORMAT_CREATION
        "  --alpha      Create ALPHA textures from .pgm or 1 channel GRAYSCALE .pam\n"
        "               infiles. The default is to create RED textures. This is ignored\n"
        "               for files with 2 or more channels. This option is mutually\n"
        "               exclusive with --luminance.\n"
#endif
        "  --automipmap A mipmap pyramid will be automatically generated when the KTX\n"
        "               file is loaded. This option is mutually exclusive with --levels\n"
        "               and --mipmap.\n"
        "  --cubemap    KTX file is for a cubemap. At least 6 <infile>s must be provided,\n"
        "               more if --mipmap is also specified. Provide the images in the\n"
        "               order: +X, -X, +Y, -Y, +Z, -Z.\n"
#if ALLOW_LEGACY_FORMAT_CREATION
        "  --luminance  Create LUMINANCE or LUMINANCE_ALPHA textures from .pgm and\n"
        "               1 or 2 channel GRAYSCALE .pam infiles. The default is to create\n"
        "               RED or RG textures. This option is mutually exclusive with\n"
        "               --alpha.\n"
#endif
        "  --levels levels\n"
        "               KTX file is for a mipmap pyramid with @e levels rather than a\n"
        "               full pyramid. @e levels must be <= the maximum number of levels\n"
        "               determined from the size of the base image. Provide the base\n"
        "               level image first. This option is mutually exclusive with\n"
        "               --automipmap and --mipmap.\n"
        "  --mipmap     KTX file is for a full mipmap pyramid. One <infile> per level\n"
        "               must be provided. Provide the base-level image first then in\n"
        "               order down to the 1x1 image. This option is mutually exclusive\n"
        "               with --automipmap and --levels.\n"
        "  --nometadata Do not write KTXorientation metadata into the output file.\n"
        "               Use of this option is not recommended.\n"
        "  --upper_left_maps_to_s0t0\n"
        "               Map the logical upper left corner of the image to s0,t0.\n"
        "               Although opposite to the OpenGL convention, this is the DEFAULT\n"
        "               BEHAVIOUR. netpbm files have an upper left origin so this option\n"
        "               does not flip the input files. When this option is in effect,\n"
        "               toktx writes a KTXorientation value of S=r,T=d into the output file\n"
        "               to inform loaders of the logical orientation. If an OpenGL {,ES}\n"
        "               loader ignores the orientation value, the image will appear upside\n"
        "               down.\n"
        "  --lower_left_maps_to_s0t0\n"
        "               Map the logical lower left corner of the image to s0,t0.\n"
        "               This causes the input netpbm images to be flipped vertically to a\n"
        "               lower-left origin. When this option is in effect, toktx\n"
        "               writes a KTXorientation value of S=r,T=u into the output file\n"
        "               to inform loaders of the logical orientation. If a Vulkan loader\n"
        "               ignores the orientation value, the image will appear upside down.\n"
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
    fprintf(stderr, "%s version 1.3\n", appName);
}


int _tmain(int argc, _TCHAR* argv[])
{
    FILE* f;
    KTX_error_code ret;
    ktxTextureCreateInfo createInfo;
    ktxTexture* texture = 0;
    struct commandOptions options;
    unsigned int imageSize;
    int exitCode = 0, face;
    unsigned int i, level, levelWidth, levelHeight;
    FileResult readResult;

    processCommandLine(argc, argv, options);

    if (options.cubemap)
      createInfo.numFaces = 6;
    else
      createInfo.numFaces = 1;

    // TO DO: handle array textures
    createInfo.numLayers = 1;
    createInfo.isArray = KTX_FALSE;

    // TO DO: handle 3D textures. Concatenate the files here or in WriteKTXF?

    for (i = 0, face = 0, level = 0; i < options.numInputFiles; i++) {
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
            unsigned char* srcImg = 0;

            readResult = readNPBM(f, w, h, components, componentSize, imageSize,
                                  0);
            if (SUCCESS == readResult) {

                /* Sanity check. */
                assert(w * h * componentSize * components == imageSize);


                if (h > 1 && options.lower_left_maps_to_s0t0) {
                    readResult = readImage(f, imageSize, srcImg);
                    if (SUCCESS != readResult) {
                        fprintf(stderr, "%s: \"%s\" is not a valid .pam, .pgm or .ppm file\n",
                                options.appName, infile ? infile : "data from stdin");
                        exitCode = 1;
                        goto cleanup;
                    }
                    yflip(srcImg, imageSize, w, h, components*componentSize);
                }

                if (i == 0) {
                    switch (components) {
                      case 1:
                        createInfo.glInternalformat = componentSize == 1 ? GL_R8 : GL_R16;
                        break;

                      case 2:
                        createInfo.glInternalformat = componentSize == 1 ? GL_RG8 : GL_RG16;
                        break;

                      case 3:
                        createInfo.glInternalformat = componentSize == 1 ? GL_RGB8 : GL_RGB16;
                        break;

                      case 4:
                        createInfo.glInternalformat = componentSize == 1 ? GL_RGBA8 : GL_RGBA16;
                        break;

                      default:
                        /* If we get here there's a bug in readPAM */
                        assert(0);
                    }
                    createInfo.baseWidth = levelWidth = w;
                    createInfo.baseHeight = levelHeight = h;
                    createInfo.baseDepth = 1;
                    if (h == 1 && !options.two_d)
                        createInfo.numDimensions = 1;
                    else
                        createInfo.numDimensions = 2;
                    if (options.automipmap) {
                        createInfo.numLevels = 1;
                        createInfo.generateMipmaps = KTX_TRUE;
                    } else {
                        createInfo.generateMipmaps = KTX_FALSE;
                        GLuint levels = 0;
                        if (options.mipmap) {
                            // Calculate number of miplevels
                            GLuint max_dim = w > h ? w : h;
                            levels = log2(max_dim) + 1;
                        } else {
                            levels = options.levels;
                        }
                        // Check we have enough.
                        if (levels * createInfo.numFaces > options.numInputFiles) {
                            fprintf(stderr,
                                    "%s: too few files for %d mipmap levels and %d faces.\n",
                                    options.appName, levels,
                                    createInfo.numFaces);
                            exitCode = 1;
                            goto cleanup;
                        } else if (levels * createInfo.numFaces < options.numInputFiles) {
                            fprintf(stderr,
                                    "%s: too many files for %d mipmap levels and %d faces."
                                    " Extras will be ignored.\n",
                                    options.appName, levels,
                                    createInfo.numFaces);
                        }
                        createInfo.numLevels = levels;
                    }
                    ret = ktxTexture_Create(&createInfo,
                                            KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                            &texture);
                    if (KTX_SUCCESS != ret) {
                        fprintf(stderr, "%s failed to create ktxTexture; KTX error: %s\n",
                                options.appName, ktxErrorString(ret));
                        exitCode = 2;
                        goto cleanup;
                    }
#if ALLOW_LEGACY_FORMAT_CREATION
                    // Hack so can still create legacy textures for testing
                    // legacy loader. ktxTexture_Create would reject these.
                    switch (components) {
                      case 1:
                        if (options.luminance) {
                            texture->glInternalformat = componentSize == 1 ? GL_LUMINANCE8 : GL_LUMINANCE16;
                            texture->glFormat = texture->glBaseInternalformat = GL_LUMINANCE;
                        } else if (options.alpha) {
                            texture->glInternalformat = componentSize == 1 ? GL_ALPHA8 : GL_ALPHA16;
                            texture->glFormat = texture->glBaseInternalformat = GL_ALPHA;
                       }
                        break;
                            
                      case 2:
                        if (options.luminance) {
                            texture->glInternalformat = componentSize == 1 ? GL_LUMINANCE8_ALPHA8 : GL_LUMINANCE16_ALPHA16;
                            texture->glFormat = texture->glBaseInternalformat = GL_LUMINANCE_ALPHA;                        }
                        break;
                    }
#endif
                } else {
                    if (face == (options.cubemap ? 6 : 1)) {
                        level++;
                        if (level < createInfo.numLevels) {
                            levelWidth >>= 1;
                            levelHeight >>= 1;
                            if (w != levelWidth || h != levelHeight) {
                                fprintf(stderr, "%s: \"%s\" has incorrect width or height for current mipmap level\n",
                                        options.appName, infile);
                                exitCode = 1;
                                goto cleanup;
                            }
                            face = 0;
                        } else {
                            break;
                        }
                    }
                }
                if (options.cubemap && w != h && w != levelWidth) {
                        fprintf(stderr, "%s: \"%s,\" intended for a cubemap face, is not square or has incorrect\n"
                                        "size for current mipmap level\n",
                                options.appName, infile);
                        exitCode = 1;
                        goto cleanup;
                }
                if (srcImg)
                    ktxTexture_SetImageFromMemory(texture,
                                                  level,
                                                  0,
                                                  face,
                                                  srcImg,
                                                  imageSize);
                else
                    ktxTexture_SetImageFromStdioStream(texture,
                                                       level,
                                                       0,
                                                       face,
                                                       f, imageSize);
#if IMAGE_DEBUG
                {
                    ktx_size_t offset;
                    ktxTexture_GetImageOffset(texture, level, 0, face, &offset);
                    dumpImage(infile, w, h, components, componentSize,
                              options.luminance,
                              texture.pData + offset);
                }
#endif

                face++;
            } else {
                fprintf(stderr, "%s: \"%s\" is not a valid .pam, .pgm or .ppm file\n",
                        options.appName, infile ? infile : "data from stdin");
                exitCode = 1;
                goto cleanup;
            }
            (void)fclose(f);
        } else {
            fprintf(stderr, "%s could not open input file \"%s\". %s\n",
                    options.appName, infile ? infile : "stdin", strerror(errno));
            exitCode = 2;
            goto cleanup;
        }
    }

    /*
     * Add orientation metadata.
     * Note: 1D textures and 2D textures with a height of 1 don't need
     * orientation metadata
     */
    if (options.metadata && createInfo.baseHeight > 1) {
        ktxHashList* ht = &texture->kvDataHead;
        char orientation[10];
        
        assert(strlen(KTX_ORIENTATION2_FMT) < sizeof(orientation));
        
        snprintf(orientation, sizeof(orientation), KTX_ORIENTATION2_FMT,
                 'r', options.lower_left_maps_to_s0t0 ? 'u' : 'd');
        ktxHashTable_AddKVPair(ht, KTX_ORIENTATION_KEY,
                               (unsigned int)strlen(orientation) + 1,
                               orientation);
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
        ret = ktxTexture_WriteToStdioStream(texture, f);
        if (KTX_SUCCESS != ret) {
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

cleanup:
    if (texture) ktxTexture_Destroy(texture);
    if (f) (void)fclose(f);
    delete(options.outfile);
    return exitCode;
}


static void processCommandLine(int argc, _TCHAR* argv[], struct commandOptions& options)
{
    int i, addktx = 0;
    unsigned int outfilenamelen;
    const _TCHAR* toktx_options;
    _TCHAR* slash;

    options.alpha = 0;
    options.automipmap = 0;
    options.cubemap = 0;
    options.luminance = 0;
    options.metadata = 1;
    options.mipmap = 0;
    options.outfile = 0;
    options.numInputFiles = 0;
    options.firstInfileIndex = 0;
    options.useStdin = false;
    options.levels = 1;
    /* The OGLES WG recommended approach, even though it is opposite
     * to the OpenGL convention. Suki ja nai.
     */
    options.lower_left_maps_to_s0t0 = 0;

    slash = _tcsrchr(argv[0], '\\');
    if (slash == NULL)
        slash = _tcsrchr(argv[0], '/');
    options.appName = slash != NULL ? slash + 1 : argv[0];

    // NOTE: If options with arguments are ever added, this option handling
    // code will need revamping.

    toktx_options = _tgetenv(_T("TOKTX_OPTIONS"));
    if (toktx_options) {
        std::istringstream iss(toktx_options);
        argvector arglist;
        for (std::string w; iss >> w; )
            arglist.push_back(w);

        argparser optparser(arglist, 0);
        processOptions(optparser, options);
        if (optparser.optind != (int)arglist.size()) {
            fprintf(stderr, "Only options are allowed in the TOKTX_OPTIONS environment variable.\n");
            usage(options.appName);
            exit(1);
        }
    }

    argparser parser(argc, argv);
    processOptions(parser, options);

    if (options.alpha && (!ALLOW_LEGACY_FORMAT_CREATION || options.luminance)) {
        usage(options.appName);
        exit(1);
    }
    if (options.luminance && !ALLOW_LEGACY_FORMAT_CREATION) {
        usage(options.appName);
        exit(1);
    }
    if (options.mipmap && options.levels > 1) {
        usage(options.appName);
        exit(1);
    }
    if (options.automipmap && (options.mipmap || options.levels > 1)) {
        usage(options.appName);
        exit(1);
    }
    ktx_uint32_t requiredInputFiles = options.cubemap ? 6 : 1 * options.levels;
    if (argc - parser.optind < requiredInputFiles + 1) {
        // stdin is permitted for a single input file.
        if (requiredInputFiles != 1 || argc - parser.optind < 1) {
            usage(options.appName);
            exit(1);
        }
    }

    i = parser.optind;
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
 * @brief process potential command line options
 *
 * @return
 *
 * @param[in]     parser,     an @c argparser holding the options to process.
 * @param[in,out] options     commandOptions struct in which option information
 *                            is set.
 */
static void
processOptions(argparser& parser,
               struct commandOptions& options)
{
    _TCHAR ch;
    static struct argparser::option option_list[] = {
        { "help", argparser::option::no_argument, NULL, 'h' },
        { "version", argparser::option::no_argument, NULL, 'v' },
        { "2d", argparser::option::no_argument, &options.two_d, 1 },
        { "alpha", argparser::option::no_argument, &options.alpha, 1 },
        { "automipmap", argparser::option::no_argument, &options.automipmap, 1 },
        { "cubemap", argparser::option::no_argument, &options.cubemap, 1 },
        { "levels", argparser::option::required_argument, NULL, 'l' },
        { "luminance", argparser::option::no_argument, &options.luminance, 1 },
        { "mipmap", argparser::option::no_argument, &options.mipmap, 1 },
        { "nometadata", argparser::option::no_argument, &options.metadata, 0 },
        { "lower_left_maps_to_s0t0", argparser::option::no_argument, &options.lower_left_maps_to_s0t0, 1 },
        { "upper_left_maps_to_s0t0", argparser::option::no_argument, &options.lower_left_maps_to_s0t0, 0 },
        // -NSDocumentRevisionsDebugMode YES is appended to the end
        // of the command by Xcode when debugging and "Allow debugging when
        // using document Versions Browser" is checked in the scheme. It
        // defaults to checked and is saved in a user-specific file not the
        // pbxproj file so it can't be disabled in a generated project.
        // Remove these from the arguments under consideration.
        { "-NSDocumentRevisionsDebugMode", argparser::option::required_argument, NULL, 'i' },
        { nullptr, argparser::option::no_argument, nullptr, 0 }
    };

    tstring shortopts("hvl:");
    while ((ch = parser.getopt(&shortopts, option_list, NULL)) != -1) {
        switch (ch) {
          case 0:
            break;
          case 'l':
            options.levels = atoi(parser.optarg.c_str());
            break;
          case 'h':
            usage(options.appName);
            exit(0);
          case 'v':
            version(options.appName);
            exit(0);
          case 'i':
            break;
          case '?':
          case ':':
          default:
            usage(options.appName);
            exit(1);
        }
    }
}

static ktx_uint32_t
log2(ktx_uint32_t v)
{
    ktx_uint32_t e;

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
yflip(unsigned char*& srcImage, unsigned int imageSize,
      unsigned int w, unsigned int h, unsigned int pixelSize)
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

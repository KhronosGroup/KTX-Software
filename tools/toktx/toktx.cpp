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
#include <cstring>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
#include <limits.h>
#include <inttypes.h>

#include "GL/glcorearb.h"
#include "ktx.h"
#include "../../lib/vkformat_enum.h"
#include "argparser.h"
#include "image.h"
#include "lodepng.h"
#if (IMAGE_DEBUG) && defined(_DEBUG) && defined(_WIN32) && !defined(_WIN32_WCE)
#  include "imdebug.h"
#elif defined(IMAGE_DEBUG) && IMAGE_DEBUG
#  undef IMAGE_DEBUG
#  define IMAGE_DEBUG 0
#endif

#if defined(_WIN32)
  #define PATH_MAX MAX_PATH
#endif

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

#ifndef GL_SR8
// From GL_EXT_texture_sRGB_R8
#define GL_SR8                          0x8FBD // same as GL_SR8_EXT
#endif

#ifndef GL_SRG8
// From GL_EXT_texture_sRGB_RG8
#define GL_SRG8                         0x8FBE // same as GL_SRG8_EXT
#endif

enum oetf_e {
    OETF_LINEAR = 0,
    OETF_SRGB = 1,
    OETF_UNSET = 2
};

#if defined(_MSC_VER)
  #undef min
  #undef max
#endif

template<typename T>
struct clampedOption
{
  clampedOption(T& val, T min_v, T max_v) :
    value(val),
    min(min_v),
    max(max_v)
  {
  }

  void clear()
  {
    value = 0;
  }

  operator T() const
  {
    return value;
  }

  T operator= (T v)
  {
    value = clamp<T>(v, min, max);
    return value;
  }

  T& value;
  T min;
  T max;
};

struct commandOptions {
    struct basisOptions : public ktxBasisParams {
        // The remaining numeric fields are clamped within the Basis library.
        clampedOption<ktx_uint32_t> threadCount;
        clampedOption<ktx_uint32_t> qualityLevel;
        clampedOption<ktx_uint32_t> maxEndpoints;
        clampedOption<ktx_uint32_t> maxSelectors;
        int noMultithreading;

        basisOptions() :
            threadCount(ktxBasisParams::threadCount, 1, 10000),
            qualityLevel(ktxBasisParams::qualityLevel, 1, 255),
            maxEndpoints(ktxBasisParams::maxEndpoints, 1, 16128),
            maxSelectors(ktxBasisParams::maxSelectors, 1, 16128),
            noMultithreading(0)
        {
            uint32_t tc = std::thread::hardware_concurrency();
            if (tc == 0) tc = 1;
            threadCount.max = tc;
            threadCount = tc;

            structSize = sizeof(ktxBasisParams);
            compressionLevel = 0;
            qualityLevel.clear();
            maxEndpoints.clear();
            endpointRDOThreshold = 0.0f;
            maxSelectors.clear();
            selectorRDOThreshold = 0.0f;
            normalMap = false;
            separateRGToRGB_A = false;
            preSwizzle = false;
            noEndpointRDO = false;
            noSelectorRDO = false;
        }

#define TRAVIS_DEBUG 0
#if TRAVIS_DEBUG
        void print() {
            std::cout << "threadCount = " << threadCount.value << std::endl;
            std::cout << "qualityLevel = " << qualityLevel.value << std::endl;
            std::cout << "maxEndpoints = " << maxEndpoints.value << std::endl;
            std::cout << "maxSelectors = " << maxSelectors.value << std::endl;
            std::cout << "structSize = " << structSize << std::endl;
            std::cout << "threadCount = " << ktxBasisParams::threadCount << std::endl;
            std::cout << "compressionLevel = " << compressionLevel << std::endl;
            std::cout << "qualityLevel = " << ktxBasisParams::qualityLevel << std::endl;
            std::cout << "compressionLevel = " << compressionLevel << std::endl;
            std::cout << "maxEndpoints = " << ktxBasisParams::maxEndpoints << std::endl;
            std::cout << "endpointRDOThreshold = " << endpointRDOThreshold << std::endl;
            std::cout << "maxSelectors = " << ktxBasisParams::maxSelectors << std::endl;
            std::cout << "selectorRDOThreshold = " << selectorRDOThreshold << std::endl;
            std::cout << "normalMap = " << normalMap << std::endl;
            std::cout << "separateRGToRGB_A = " << separateRGToRGB_A << std::endl;
            std::cout << "preSwizzle = " << preSwizzle << std::endl;
            std::cout << "noEndpointRDO = " << noEndpointRDO << std::endl;
            std::cout << "noSelectorRDO = " << noSelectorRDO << std::endl;
        }
#endif
    };

    int          automipmap;
    int          cubemap;
    int          ktx2;
    int          metadata;
    int          mipmap;
    int          two_d;
    oetf_e       oetf;
    int          useStdin;
    int          lower_left_maps_to_s0t0;
    int          bcmp;
    struct basisOptions bopts;
    _tstring     outfile;
    unsigned int levels;
    std::vector<_tstring> infilenames;

    commandOptions() {
      automipmap = 0;
      cubemap = 0;
      ktx2 = 0;
      metadata = 1;
      mipmap = 0;
      two_d = false;
      useStdin = false;
      bcmp = false;
      levels = 1;
      oetf = OETF_UNSET;
      /* The OGLES WG recommended approach, even though it is opposite
       * to the OpenGL convention. Suki ja nai.
       */
      lower_left_maps_to_s0t0 = 0;
    }
};

static _tstring      appName;

static bool loadFileList(const _tstring &f,
                         std::vector<_tstring>& filenames);
static ktx_uint32_t log2(ktx_uint32_t v);
static void processCommandLine(int argc, _TCHAR* argv[],
                               struct commandOptions& options);
static void processOptions(argparser& parser, struct commandOptions& options);
static void yflip(unsigned char*& srcImage, size_t imageSize,
                  unsigned int w, unsigned int h, unsigned int pixelSize);
#if IMAGE_DEBUG
static void dumpImage(_TCHAR* name, int width, int height, int components,
                      int componentSize, unsigned char* srcImage);
#endif

/** @page toktx toktx
@~English

Create a KTX file from netpbm format files.
 
@section toktx_synopsis SYNOPSIS
    toktx [options] @e outfile [@e infile.{pam,pgm,ppm} ...]

@section toktx_description DESCRIPTION
    Create a Khronos format texture file (KTX) from a set of Netpbm
    format (.pam, .pgm, .ppm) or PNG (.png) images. Currently @b toktx only
    supports creating a KTX file holding a 2D or cube map texture. It writes
    the destination ktx file to @e outfile, appending ".ktx{,2}" if necessary.
    If @e outfile is '-' the output will be written to stdout.
 
    @b toktx reads each named @e infile. which must be in .pam, .ppm, .pgm or
    .png format. @e infiles prefixed with '@' are read as text files listing
    actual file names to process with one name per line.

    .ppm files yield RGB textures, .pgm files RED textures, .pam files RED, RG,
    RGB or RGBA textures according to the file's TUPLTYPE and DEPTH and .png
    files RED, RG, RGB or RGBA textures according to the files's @e color type.
    Other formats can be readily converted to the supported formats using tools
    such as ImageMagick and XnView.

    The primaries, transfer function (OETF) and the texture's sRGB-ness is set
    based on the input file. Netpbm files always use BT.709/sRGB primaries and
    the BT.709 OETF. @b toktx tranforms the image to sRGB, sets the transfer
    function to sRGB and creates sRGB textures for these inputs.

    For .png files the OETF is set as follows:

    <dl>
    <dt>No color-info chunks or sRGB chunk present:</dt>
        <dd>primaries are set to BT.709 and OETF to sRGB.</dd>
    <dt>sRGB chunk present:</dt>
        <dd>primaries are set to BT.709 and OETF to sRGB. gAMA and cHRM chunks
        are ignored.</dd>
    <dt>iCCP chunk present:</dt>
        <dd>General ICC profiles are not supported by toktx or the KTX2 format.
        sRGB chunk must not be present.</dd>
    <dt>gAMA and/or cHRM chunks present without sRGB or iCCP:</dt>
        <dd>If gAMA is 45455 the OETF is set to sRGB, if 100000 it
        is set to linear. Other gAMA values are unsupported. cHRM is currently
        unsupported. We should attempt to map the primary values to one of
        the standard sets listed in the Khronos Data Format specification.</dd>
    </dl>
 
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
        BEHAVIOUR. netpbm and PNG files have an upper left origin so this
		option does not flip the input images. When this option is in effect,
        toktx writes a KTXorientation value of S=r,T=d into the output file
        to inform loaders of the logical orientation. If an OpenGL {,ES}
        loader ignores the orientation value, the image will appear upside
        down.</dd>
    <dt>--lower_left_maps_to_s0t0</dt>
    <dd>Map the logical lower left corner of the image to s0,t0.
        This causes the input netpbm and PNG images to be flipped vertically
		to a lower-left origin. When this option is in effect, toktx
        writes a KTXorientation value of S=r,T=u into the output file
        to inform loaders of the logical orientation. If a Vulkan loader
        ignores the orientation value, the image will appear upside down.</dd>
    <dt>--linear</dt>
    <dd>Force the created texture to have a linear transfer function. Use this
        only when you know the file format information is wrong and the input
        file uses a linear transfer function. If this is specified, the default
        color transform of Netpbm images to sRGB color space will not be
		performed.
    </dd>
    <dt>--srgb</dt>
    <dd>Force the created texture to have an srgb transfer function. As with
        @b --linear, use with caution. Like @b --linear, the default color
        transform of Netpbm images will not be performed./dd>
    <dt>--t2</dt>
    <dd>Output in KTX2 format. Default is KTX.</dd>
    <dt>--bcmp</dt>
    <dd>Supercompress the image data with Basis Universal. Implies @b --t2.
        RED images will become RGB with RED in each component. RG images will
        have R in the RGB part and G in the alpha part of the compressed
        texture. When set, the following Basis-related options become valid
        otherwise they are ignored.
      <dl>
      <dt>--no_multithreading</dt>
      <dd>Disable multithreading. By default Basis compression will use the
          numnber of threads reported by @c std::thread::hardware_concurrency or
          1 if value returned is 0.</dd>
      <dt>--threads &lt;count&gt;</dt>
      <dd>Explicitly set the number of threads to use during compression.
          @b --no_multithreading must not be set.</dd>
      <dt>--clevel &lt;level&gt;</dt>
      <dd>Basis compression level, an encoding speed vs. quality tradeoff. Range
          is 0 - 5, default is 1. Higher values are slower, but give higher
          quality.</dd>
      <dt>--qlevel &lt;level&gt;</dt>
      <dd>Basis quality level. Range is 1 - 255.  Lower gives better
          compression/lower quality/faster. Higher gives less compression
          /higher quality/slower. Values of @b --max_endpoints and
          @b --max-selectors computed from this override any explicitly set
          values. Default is 128, if either of @b --max_endpoints or
          @b --max_selectors is unset, otherwise those settings rule.</dd>
      <dt>--max_endpoints &lt;arg&gt;</dt>
      <dd>Manually set the maximum number of color endpoint clusters
          from 1-16128. Default is 0, unset.</dd>
      <dt>--endpoint_rdo_threshold &lt;arg&gt;</dt>
      <dd>Set endpoint RDO quality threshold. The default is 1.25. Lower is
          higher quality but less quality per output bit (try 1.0-3.0).
          This will override the value chosen by @c --qual.</dd>
      <dt>--max_selectors &lt;arg&gt;</dt>
      <dd>Manually set the maximum number of color selector clusters
          from 1-16128. Default is 0, unset.</dd>
      <dt>--selector_rdo_threshold &lt;arg&gt;</dt>
      <dd>Set selector RDO quality threshold. The default is 1.5. Lower is
          higher quality but less quality per output bit (try 1.0-3.0).
          This will override the value chosen by @c --qual.</dd>
      <dt>--normal_map</dt>
      <dd>Tunes codec parameters for better quality on normal maps (no
          selector RDO, no endpoint RDO). Only valid for linear textures.</dd>
      <dt>--separate_rg_to_color_alpha</dt>
      <dd>Separates the input R and G channels to RGB and A (for tangent
          space XY normal maps). Automatically selected if the input images
          are 2-component.</dd>
      <dt>--no_endpoint_rdo</dt>
      <dd>Disable endpoint rate distortion optimizations. Slightly faster,
          less noisy output, but lower quality per output bit. Default is
          to do endpoint RDO.</dd>
      <dt>--no_selector_rdo</dt>
      <dd>Disable selector rate distortion optimizations. Slightly faster,
          less noisy output, but lower quality per output bit. Default is
          to do selector RDO.</dd>
      </dl>
    </dd>
    <dt>--help</dt>
    <dd>Print this usage message and exit.</dd>
    <dt>--version</dt>
    <dd>Print the version number of this program and exit.</dd>
    </dl>
 
    Options can also be set in the environment variable TOKTX_OPTIONS.
    TOKTX_OPTIONS is parsed first. If conflicting options appear in
    TOKTX_OPTIONS or the command line, the last one seen wins. However if both
    @b --automipmap and @b --mipmap are seen, it is always flagged as an error.
    You can, for example, set TOKTX_OPTIONS=--lower_left_maps_to_s0t0 to change
    the default mapping of the logical image origin to match the GL convention.

@section toktx_exitstatus EXIT STATUS
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

@section toktx_author AUTHOR
    Mark Callow, Edgewise Consulting www.edgewise-consulting.com
*/

// I really HATE this duplication of text but I cannot find a simple way to
// it that works on all platforms (e.g running man toktx) even if I was
// willing to tolerate markup commands in the usage output.
static void
usage(const _tstring appName)
{
    fprintf(stderr, 
        "Usage: %s [options] <outfile> [<infile>.{pam,pgm,ppm} ...]\n"
        "\n"
        "  <outfile>    The destination ktx file. \".ktx\" will appended if necessary.\n"
        "               If it is '-' the output will be written to stdout.\n"
        "  <infile>     One or more image files in .pam, .ppm, .pgm or .png format. Other\n"
        "               formats can be readily converted to these formats using tools\n"
        "               such as ImageMagick and XnView. When no infile is specified,\n"
        "               stdin is used. .ppm files yield RGB textures, .pgm files RED\n"
        "               textures, .pam files RED, RG, RGB or RGBA textures according\n"
        "               to the file's TUPLTYPE and DEPTH and .png files RED, RG, RGB\n"
        "               or RGBA textures according to the files's @e color type\n"
        "\n"
        "  Options are:\n"
        "\n"
        "  --2d         If the image height is 1, by default a KTX file for a 1D\n"
        "               texture is created. With this option one for a 2D texture is\n"
        "               created instead.\n"
        "  --automipmap A mipmap pyramid will be automatically generated when the KTX\n"
        "               file is loaded. This option is mutually exclusive with --levels\n"
        "               and --mipmap.\n"
        "  --cubemap    KTX file is for a cubemap. At least 6 <infile>s must be provided,\n"
        "               more if --mipmap is also specified. Provide the images in the\n"
        "               order: +X, -X, +Y, -Y, +Z, -Z.\n"
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
        "               BEHAVIOUR. netpbm and PNG files have an upper left origin so\n"
        "               this option does not flip the input images. When this option is\n"
        "               in effect, toktx writes a KTXorientation value of S=r,T=d into\n"
        "               the output file to inform loaders of the logical orientation. If\n"
        "               an OpenGL {,ES} loader ignores the orientation value, the image\n"
        "               will appear upside down.\n"
        "  --lower_left_maps_to_s0t0\n"
        "               Map the logical lower left corner of the image to s0,t0.\n"
        "               This causes the input netpbm and PNG images to be flipped|n"
        "               vertically to a ower-left origin. When this option is in effect,\n"
        "               toktx writes a KTXorientation value of S=r,T=u into the output\n"
        "               file to inform loaders of the logical orientation. If a Vulkan\n"
        "               loader ignores the orientation value, the image will appear\n"
        "               upside down.\n"
        "  --linear     Force the created texture to have a linear transfer function.\n"
        "               Use this only when you know the file format information is wrong\n"
        "               and the input file uses a linear transfer function. If this is\n"
        "               specified, the default transform of Netpbm images to sRGB color\n"
        "               space will not be performed.\n"
        "  --srgb       Force the created texture to have an srgb transfer function.\n"
        "               Ass with --linear, use with caution.  Like @b --linear, the\n"
        "               default color transform of Netpbm images will not be performed."
        "  --t2         Output in KTX2 format. Default is KTX.\n"
        "  --bcmp\n"
        "               Supercompress the image data with Basis Universal. Implies --t2.\n"
        "               RED images will become RGB with RED in each component. RG images\n"
        "               will have R in the RGB part and G in the alpha part of the\n"
        "               compressed texture. When set, the following Basis-related\n"
        "               options become valid, otherwise they are ignored.\n"
        "       --no_multithreading\n"
        "               Disable multithreading. By default Basis compression will use\n"
        "               the numnber of threads reported by\n"
        "               @c std::thread::hardware_concurrency or 1 if value returned is 0.\n"
        "      --threads <count>\n"
        "               Explicitly set the number of threads to use during compression.\n"
        "               --no_multithreading must not be set.\n"
        "      --clevel <level>\n"
        "               Basis compression level, an encoding speed vs. quality tradeoff.\n"
        "               Range is 0 - 5, default is 1. Higher values are slower, but give\n"
        "               higher quality.\n"
        "      --qlevel <level>\n"
        "               Basis quality level. Range is 1 - 255.  Lower gives better\n"
        "               compression/lower quality/faster. Higher gives less compression\n"
        "               /higher quality/slower. Values of --max_endpoints and\n"
        "               --max-selectors computed from this override any explicitly set\n"
        "               values. Default is 128, if either of --max_endpoints or\n"
        "               --max_selectors is unset, otherwise those settings rule.\n"
        "      --max_endpoints <arg>\n"
        "               Manually set the maximum number of color endpoint clusters from\n"
        "               1-16128. Default is 0, unset.\n"
        "      --endpoint_rdo_threshold <arg>\n"
        "               Set endpoint RDO quality threshold. The default is 1.25. Lower\n"
        "               is higher quality but less quality per output bit (try 1.0-3.0).\n"
        "               This will override the value chosen by --qual.\n"
        "      --max_selectors <arg>\n"
        "               Manually set the maximum number of color selector clusters from\n"
        "               1-16128. Default is 0, unset.\n"
        "      --selector_rdo_threshold <arg>\n"
        "               Set selector RDO quality threshold. The default is 1.25. Lower\n"
        "               is higher quality but less quality per output bit (try 1.0-3.0).\n"
        "               This will override the value chosen by --qual.\n"
        "      --normal_map\n"
        "               Tunes codec parameters for better quality on normal maps (no\n"
        "               selector RDO, no endpoint RDO). Only valid for linear textures.\n"
        "      --separate_rg_to_color_alpha\n"
        "               Separates the input R and G channels to RGB and A (for tangent\n"
        "               space XY normal maps). Automatically selected if the input\n"
        "               image(s) are 2-component."
        "      --no_endpoint_rdo\n"
        "               Disable endpoint rate distortion optimizations. Slightly faster,\n"
        "               less noisy output, but lower quality per output bit. Default is\n"
        "               to do endpoint RDO.\n"
        "      --no_selector_rdo\n"
        "               Disable selector rate distortion optimizations. Slightly faster,\n"
        "               less noisy output, but lower quality per output bit. Default is\n"
        "               to do selector RDO.\n"
        "  --help       Print this usage message and exit.\n"
        "  --version    Print the version number of this program and exit.\n"
        "\n"
        "Options can also be set in the environment variable TOKTX_OPTIONS.\n"
        "TOKTX_OPTIONS is parsed first. If conflicting options appear in TOKTX_OPTIONS\n"
        "or the command line, the last one seen wins. However if both --automipmap and\n"
        "--mipmap are seen, it is always flagged as an error. You can, for example,\n"
        "set TOKTX_OPTIONS=--lower_left_maps_to_s0t0 to change the default mapping of\n"
        "the logical image origin to match the GL convention.\n",
        appName.c_str());
}

#define STR(x) #x
#define VERSION 2.0

static void
writeId(std::ostream& dst, _tstring& appName)
{
    dst << appName << " version " << VERSION;
}

static void
version(const _tstring& appName)
{
    fprintf(stderr, "%s version %s\n", appName.c_str(), STR(VERSION));
}


int _tmain(int argc, _TCHAR* argv[])
{
    FILE* f;
    KTX_error_code ret;
    ktxTextureCreateInfo createInfo;
    ktxTexture* texture = 0;
    struct commandOptions options;
    size_t imageSize;
    int exitCode = 0, face;
    unsigned int components, i, level, levelWidth, levelHeight;
    oetf_e chosenOETF, fileOETF;

    processCommandLine(argc, argv, options);

    if (options.cubemap)
      createInfo.numFaces = 6;
    else
      createInfo.numFaces = 1;

    // TO DO: handle array textures
    createInfo.numLayers = 1;
    createInfo.isArray = KTX_FALSE;

    // TO DO: handle 3D textures.

    for (i = 0, face = 0, level = 0; i < options.infilenames.size(); i++) {
        _tstring& infile = options.infilenames[i];

        f = _tfopen(infile.c_str(),"rb");

        if (f) {
            unsigned int w, h, componentSize;
            uint8_t* srcImg = 0;
            enum fileType_e { NPBM, PNG } fileType;
            oetf_e curfileOETF;

            FileResult npbmResult = readNPBM(f, w, h, components,
                                             componentSize, imageSize, &srcImg);
            if (npbmResult == INVALID_FORMAT) {
                // Try .png. Unfortunately LoadPNG doesn't believe in stdio plus
                // the function we need only reads from memory. To avoid
                // a potentially unnecessary read of the whole file check the
                // signature ourselves.
                uint8_t pngsig[8] = {
                    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a
                };
                uint8_t filesig[sizeof(pngsig)];
                if (fseek(f, 0L, SEEK_SET) < 0) {
                    fprintf(stderr, "%s: Could not seek in \"%s\". %s\n",
                            appName.c_str(), infile.c_str(), strerror(errno));
                    exitCode = 1;
                    goto cleanup;
                }
                if (fread(filesig, sizeof(pngsig), 1, f) != 1) {
                    fprintf(stderr, "%s: Could not read \"%s\". %s\n",
                            appName.c_str(), infile.c_str(), strerror(errno));
                    exitCode = 1;
                    goto cleanup;
                }
                if (memcmp(filesig, pngsig, sizeof(pngsig))) {
                    fprintf(stderr, "%s: \"%s\" is not a .pam, .pgm, .ppm or .png file\n",
                            appName.c_str(), infile.c_str());
                    exitCode = 1;
                    goto cleanup;
                }
                fileType = PNG;
            } else if (npbmResult == SUCCESS) {
                fileType = NPBM;
                if (options.oetf == OETF_UNSET) {
                    // Convert to sRGB
                    if (componentSize == 1) {
                        OETFtransform(imageSize, srcImg, components,
                                      decode709, encode_sRGB);
                    } else {
                        OETFtransform(imageSize, (uint16_t*)srcImg, components,
                                      decode709, encode_sRGB);
                    }
                    curfileOETF = OETF_SRGB;
                }
            } else {
                fprintf(stderr, "%s: \"%s\" is not a valid .pam, .pgm, or .ppm file\n",
                        appName.c_str(), infile.c_str());
                exitCode = 1;
                goto cleanup;
            }

            if (fileType == PNG) {
                size_t fsz;
                fseek(f, 0L, SEEK_END);
                fsz = ftell(f);
                fseek(f, 0L, SEEK_SET);

                std::vector<uint8_t> png;
                png.resize(fsz);
                if (fread(png.data(), 1L, png.size(), f) != png.size()) {
                    if (feof(f)) {
                        fprintf(stderr,
                                "%s: Unexpected end of file reading \"%s\" \n",
                                appName.c_str(), infile.c_str());
                    } else {
                        fprintf(stderr, "%s: Error reading \"%s\": %s\n",
                                appName.c_str(), infile.c_str(),
                                strerror(ferror(f)));
                    }
                    exitCode = 1;
                    goto cleanup;
                }

                lodepng::State state;
                // Find out the color type so we can request that type when
                // decoding and avoid conversions. Oh for an option to decode
                // to file's colortype. What a palaver! Sigh!
                lodepng_inspect(&w, &h, &state, &png[0], png.size());
                switch (state.info_png.color.colortype) {
                  case LCT_GREY:
                    components = 1;
                    break;
                  case LCT_RGB:
                    components = 3;
                    break;
                  case LCT_PALETTE:
                    fprintf(stderr,
                    "%s: \"%s\" is a paletted image which are not supported.\n",
                    appName.c_str(), infile.c_str());
                    exitCode = 1;
                    goto cleanup;
                  case LCT_GREY_ALPHA:
                    components = 2;
                    break;
                  case LCT_RGBA:
                    components = 4;
                    break;
                }
                componentSize = state.info_png.color.bitdepth / 8;

                // Tell the decoder we want the same color type as the file
                state.info_raw = state.info_png.color;
                uint32_t error = lodepng_decode(&srcImg, &w, &h, &state,
                                                png.data(), png.size());
                if (srcImg && !error) {
                    imageSize = lodepng_get_raw_size(w, h, &state.info_raw);
                } else {
                    delete srcImg;
                    std::cerr << appName << ": " << "PNG decoder error:"
                              << lodepng_error_text(error) << std::endl;
                    exitCode = 1;
                    goto cleanup;
                }

                // state will have been updated with the rest of the file info.

                // Here is the priority of the color space info in PNG:
                //
                // 1. No color-info chunks: assume sRGB default or 2.2 gamma
                //    (up to the implementation).
                // 2. sRGB chunk: use sRGB intent specified in the chunk, ignore
                //    all other color space information.
                // 3. iCCP chunk: use the provided ICC profile, ignore gamma and
                //    primaries.
                // 4. gAMA and/or cHRM chunks: use provided gamma and primaries.
                //
                // A PNG image could signal linear transfer function with one
                // of these two options:
                //
                // 1. Provide an ICC profile in iCCP chunk.
                // 2. Use a gAMA chunk with a value that yields linear
                //    function (100000).
                //
                // Using no. 1 above or setting transfer func & primaries from
                // the ICC profile would require parsing the ICC payload.
                //
                // Using cHRM to get the primaries would require matching a set
                // of primary values to a DFD primaries id.

                if (state.info_png.srgb_defined) {
                    // intent is a matter for the user when a color transform
                    // is needed during rendering, especially when gamut
                    // mapping. It does not affect the meaning or value of the
                    // image pixels so there is nothing to do here.
                    curfileOETF = OETF_SRGB;
                } else {
                    if (state.info_png.iccp_defined) {
                        delete srcImg;
                        std::cerr << appName
                                  << ": PNG file has ICC profile chunk. "
                                  << "These are not supported." << std::endl;
                        exitCode = 1;
                        goto cleanup;
                    } else if (state.info_png.gama_defined) {
                        if (state.info_png.gama_gamma == 100000)
                            curfileOETF = OETF_LINEAR;
                        else if (state.info_png.gama_gamma == 45455)
                            curfileOETF = OETF_SRGB;
                        else {
                            delete srcImg;
                            std::cerr << appName
                                      << ": PNG image has gamma of "
                                      << (float)100000 / state.info_png.gama_gamma
                                      << ". This is currently unsupported."
                                      << std::endl;
                            exitCode = 1;
                            goto cleanup;
                        }
                    } else {
                        curfileOETF = OETF_SRGB;
                    }
                }
            }

            if (srcImg || npbmResult == SUCCESS) {

                /* Sanity check. */
                assert(w * h * componentSize * components == imageSize);

                if (h > 1 && options.lower_left_maps_to_s0t0) {
                    if (!srcImg) {
                        FileResult readResult = readImage(f, imageSize, srcImg);
                        if (SUCCESS != readResult) {
                            fprintf(stderr, "%s: \"%s\" is not a valid .pam, .pgm or .ppm file\n",
                                    appName.c_str(), infile.c_str());
                            exitCode = 1;
                            goto cleanup;
                        }
                    }
                    yflip(srcImg, imageSize, w, h, components*componentSize);
                }

                if (i == 0) {
                    bool srgb;

                    fileOETF = curfileOETF;
                    if (options.oetf == OETF_UNSET) {
                        chosenOETF = fileOETF;
                    } else {
                        chosenOETF = options.oetf;
                    }

                    srgb = (chosenOETF == OETF_SRGB);
                    switch (components) {
                      case 1:
                        switch (componentSize) {
                          case 1:
                            createInfo.glInternalformat
                                            = srgb ? GL_SR8 : GL_R8;
                            createInfo.vkFormat
                                            = srgb ? VK_FORMAT_R8_SRGB
                                                   : VK_FORMAT_R8_UNORM;
                            break;
                          case 2:
                            createInfo.glInternalformat = GL_R16;
                            createInfo.vkFormat = VK_FORMAT_R16_UNORM;
                            break;
                          case 4:
                            createInfo.glInternalformat = GL_R32F;
                            createInfo.vkFormat = VK_FORMAT_R32_SFLOAT;
                            break;
                        }
                        break;

                      case 2:
                         switch (componentSize) {
                          case 1:
                            createInfo.glInternalformat
                                            = srgb ? GL_SRG8 : GL_RG8;
                            createInfo.vkFormat
                                            = srgb ? VK_FORMAT_R8G8_SRGB
                                                   : VK_FORMAT_R8G8_UNORM;
                            break;
                          case 2:
                            createInfo.glInternalformat = GL_RG16;
                            createInfo.vkFormat = VK_FORMAT_R16G16_UNORM;
                            break;
                          case 4:
                            createInfo.glInternalformat = GL_RG32F;
                            createInfo.vkFormat = VK_FORMAT_R32G32_SFLOAT;
                            break;
                        }
                        break;

                      case 3:
                         switch (componentSize) {
                          case 1:
                            createInfo.glInternalformat
                                            = srgb ? GL_SRGB8 : GL_RGB8;
                            createInfo.vkFormat
                                            = srgb ? VK_FORMAT_R8G8B8_SRGB
                                                   : VK_FORMAT_R8G8B8_UNORM;
                            break;
                          case 2:
                            createInfo.glInternalformat = GL_RGB16;
                            createInfo.vkFormat = VK_FORMAT_R16G16B16_UNORM;
                            break;
                          case 4:
                            createInfo.glInternalformat = GL_RGB32F;
                            createInfo.vkFormat = VK_FORMAT_R32G32B32_SFLOAT;
                            break;
                        }
                        break;

                      case 4:
                         switch (componentSize) {
                          case 1:
                            createInfo.glInternalformat
                                            = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
                            createInfo.vkFormat
                                            = srgb ? VK_FORMAT_R8G8B8A8_SRGB
                                                   : VK_FORMAT_R8G8B8A8_UNORM;
                            break;
                          case 2:
                            createInfo.glInternalformat = GL_RGBA16;
                            createInfo.vkFormat = VK_FORMAT_R16G16B16A16_UNORM;
                            break;
                          case 4:
                            createInfo.glInternalformat = GL_RGBA32F;
                            createInfo.vkFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
                            break;
                        }
                        break;

                      default:
                        /* If we get here there's a bug. */
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
                        if (levels * createInfo.numFaces > options.infilenames.size()) {
                            fprintf(stderr,
                                    "%s: too few files for %d mipmap levels and %d faces.\n",
                                    appName.c_str(), levels,
                                    createInfo.numFaces);
                            exitCode = 1;
                            goto cleanup;
                        } else if (levels * createInfo.numFaces < options.infilenames.size()) {
                            fprintf(stderr,
                                    "%s: too many files for %d mipmap levels and %d faces."
                                    " Extras will be ignored.\n",
                                    appName.c_str(), levels,
                                    createInfo.numFaces);
                        }
                        createInfo.numLevels = levels;
                    }
                    if (options.ktx2) {
                        ret = ktxTexture2_Create(&createInfo,
                                                 KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                                 (ktxTexture2**)&texture);
                    } else {
                        ret = ktxTexture1_Create(&createInfo,
                                                 KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                                 (ktxTexture1**)&texture);
                    }
                    if (KTX_SUCCESS != ret) {
                        fprintf(stderr, "%s failed to create ktxTexture; KTX error: %s\n",
                                appName.c_str(), ktxErrorString(ret));
                        exitCode = 2;
                        goto cleanup;
                    }
                } else {
                    if (options.oetf == OETF_UNSET) {
                        if (curfileOETF != fileOETF) {
                            fprintf(stderr, "%s: \"%s\" is encoded with a different transfer function"
                                            "(OETF) than preceding files.\n",
                                            appName.c_str(), infile.c_str());
                            exitCode = 1;
                            goto cleanup;
                        }
                    }
                    if (face == (options.cubemap ? 6 : 1)) {
                        level++;
                        if (level < createInfo.numLevels) {
                            levelWidth >>= 1;
                            levelHeight >>= 1;
                            if (w != levelWidth || h != levelHeight) {
                                fprintf(stderr, "%s: \"%s\" has incorrect width or height for current mipmap level\n",
                                        appName.c_str(), infile.c_str());
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
                            appName.c_str(), infile.c_str());
                    exitCode = 1;
                    goto cleanup;
                }
                if (srcImg) {
#if TRAVIS_DEBUG
                    if (options.bcmp) {
                        std::cout << "level = " << level << ", face = " << face;
                        std::cout << ", srcImg = " << std::hex  << (void *)srcImg << std::dec;
                        std::cout << ", imageSize = " << imageSize << std::endl;
                    }
#endif
                    ktxTexture_SetImageFromMemory(ktxTexture(texture),
                                                  level,
                                                  0,
                                                  face,
                                                  srcImg,
                                                  imageSize);

                } else
                    ktxTexture_SetImageFromStdioStream(ktxTexture(texture),
                                                       level,
                                                       0,
                                                       face,
                                                       f, imageSize);

#if IMAGE_DEBUG
                {
                    ktx_size_t offset;
                    ktxTexture_GetImageOffset(texture, level, 0, face, &offset);
                    dumpImage(infile, w, h, components, componentSize,
                              texture.pData + offset);
                }
#endif

                face++;
            }
            (void)fclose(f);
        } else {
            fprintf(stderr, "%s could not open input file \"%s\". %s\n",
                    appName.c_str(), infile.c_str(), strerror(errno));
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
        if (options.ktx2) {
            orientation[0] = 'r';
            orientation[1] = options.lower_left_maps_to_s0t0 ? 'u' : 'd';
            orientation[2] = 0;
        } else {
            assert(strlen(KTX_ORIENTATION2_FMT) < sizeof(orientation));

            snprintf(orientation, sizeof(orientation), KTX_ORIENTATION2_FMT,
                     'r', options.lower_left_maps_to_s0t0 ? 'u' : 'd');
        }
        ktxHashList_AddKVPair(ht, KTX_ORIENTATION_KEY,
                              (unsigned int)strlen(orientation) + 1,
                              orientation);
    }
    if (options.ktx2) {
        // Add required writer metadata.
        std::stringstream writer;
        writeId(writer, appName);
        ktxHashList_AddKVPair(&texture->kvDataHead, KTX_WRITER_KEY,
                              (ktx_uint32_t)writer.str().length() + 1,
                              writer.str().c_str());
    }

    if (options.outfile.compare("-") == 0) {
        f = stdout;
#if defined(_WIN32)
        /* Set "stdout" to have binary mode */
        (void)_setmode( _fileno( stdout ), _O_BINARY );
#endif
    } else
        f = _tfopen(options.outfile.c_str(), "wb");

    if (f) {
        if (options.bcmp) {
            commandOptions::basisOptions& bopts = options.bopts;
            if (bopts.normalMap && chosenOETF != OETF_LINEAR) {
                fprintf(stderr, "%s: --normal_map specified but input file(s) are"
                        " not linear.", appName.c_str());
                exitCode = 1;
                goto cleanup;
            }
            if (bopts.noMultithreading)
                bopts.threadCount = 1;
            if (components == 2) {
                bopts.separateRGToRGB_A = true;
            }
#if TRAVIS_DEBUG
            bopts.print();
#endif
            ret = ktxTexture2_CompressBasisEx((ktxTexture2*)texture, &bopts);
            if (KTX_SUCCESS != ret) {
                fprintf(stderr, "%s failed to write KTX file \"%s\"; KTX error: %s\n",
                        appName.c_str(), options.outfile.c_str(),
                        ktxErrorString(ret));
            }
        } else {
            ret = KTX_SUCCESS;
        }
        if (KTX_SUCCESS == ret) {
            ret = ktxTexture_WriteToStdioStream(ktxTexture(texture), f);
            if (KTX_SUCCESS != ret) {
                fprintf(stderr, "%s failed to write KTX file \"%s\"; KTX error: %s\n",
                    appName.c_str(), options.outfile.c_str(),
                    ktxErrorString(ret));
            }
        }
        if (KTX_SUCCESS != ret) {
            fclose(f);
            if (f != stdout)
                _tunlink(options.outfile.c_str());
            exitCode = 2;
        }  
    } else {
        fprintf(stderr, "%s: could not open output file \"%s\". %s\n",
                appName.c_str(), options.outfile.c_str(), strerror(errno));
        exitCode = 2;
    }

cleanup:
    if (texture) ktxTexture_Destroy(ktxTexture(texture));
    if (f) (void)fclose(f);
    return exitCode;
}


static void processCommandLine(int argc, _TCHAR* argv[], struct commandOptions& options)
{
    int i;
    const _TCHAR* toktx_options;
    size_t slash, dot;

	appName = argv[0];
    // For consistent Id, only use the stem of appName;
	slash = appName.find_last_of(_T('\\'));
	if (slash == _tstring::npos)
		slash = appName.find_last_of(_T('/'));
	if (slash != _tstring::npos)
		appName.erase(0, slash+1);  // Remove directory name.
	dot = appName.find_last_of(_T('.'));
    if (dot != _tstring::npos)
	  appName.erase(dot, _tstring::npos); // Remove extension.

    toktx_options = _tgetenv(_T("TOKTX_OPTIONS"));
    if (toktx_options) {
        std::istringstream iss(toktx_options);
        argvector arglist;
        for (_tstring w; iss >> w; )
            arglist.push_back(w);

        argparser optparser(arglist, 0);
        processOptions(optparser, options);
        if (optparser.optind != (int)arglist.size()) {
            fprintf(stderr, "Only options are allowed in the TOKTX_OPTIONS environment variable.\n");
            usage(appName);
            exit(1);
        }
    }

    argparser parser(argc, argv);
    processOptions(parser, options);

    if (options.mipmap && options.levels > 1) {
        usage(appName);
        exit(1);
    }
    if (options.automipmap && (options.mipmap || options.levels > 1)) {
        usage(appName);
        exit(1);
    }

    i = parser.optind;

    options.outfile = parser.argv[i++];

    if (options.outfile.compare(_T("-")) != 0
        && options.outfile.find_last_of('.') == _tstring::npos)
    {
        options.outfile.append(options.ktx2 ? _T(".ktx2") : _T(".ktx"));
    }

    if (argc - i > 0) {
        for (; i < argc; i++) {
            if (parser.argv[i].front() == _T('@')) {
                if (!loadFileList(parser.argv[i], options.infilenames)) {
                    exit(1);
                }
            } else {
                options.infilenames.push_back(parser.argv[i]);
            }
        }
        /* Check for attempt to use stdin as one of the
         * input files.
         */
        std::vector<_tstring>::const_iterator it;
        for (it = options.infilenames.begin(); it < options.infilenames.end(); it++) {
            if (it->compare(_T("-")) == 0) {
                fprintf(stderr, "%s: cannot use stdin as one among many inputs.\n", appName.c_str());
                usage(appName);
                exit(1);
            }
        }
        ktx_uint32_t requiredInputFiles = options.cubemap ? 6 : 1 * options.levels;
        if (requiredInputFiles > options.infilenames.size()) {
            fprintf(stderr, "%s: too few input files.\n", appName.c_str());
            exit(1);
        }
        /* Whether there are enough input files for all the mipmap levels in
         * a full pyramid can only be checked when the first file has been
         * read and the size determined.
         */
    } else {
        // Need some input files.
        usage(appName);
        exit(1);
    }
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
        { "automipmap", argparser::option::no_argument, &options.automipmap, 1 },
        { "cubemap", argparser::option::no_argument, &options.cubemap, 1 },
        { "levels", argparser::option::required_argument, NULL, 'l' },
        { "mipmap", argparser::option::no_argument, &options.mipmap, 1 },
        { "nometadata", argparser::option::no_argument, &options.metadata, 0 },
        { "lower_left_maps_to_s0t0", argparser::option::no_argument, &options.lower_left_maps_to_s0t0, 1 },
        { "upper_left_maps_to_s0t0", argparser::option::no_argument, &options.lower_left_maps_to_s0t0, 0 },
        { "linear", argparser::option::no_argument, (int*)&options.oetf, OETF_LINEAR },
        { "srgb", argparser::option::no_argument, (int*)&options.oetf, OETF_SRGB },
        { "t2", argparser::option::no_argument, &options.ktx2, 1},
        { "bcmp", argparser::option::no_argument, NULL, 'b' },
        { "mo_multithreading", argparser::option::no_argument, NULL, 'm' },
        { "threads", argparser::option::required_argument, NULL, 't' },
        { "clevel", argparser::option::required_argument, NULL, 'c' },
        { "qlevel", argparser::option::required_argument, NULL, 'q' },
        { "max_endpoints", argparser::option::required_argument, NULL, 'e' },
        { "endpoint_rdo_threshold", argparser::option::required_argument, NULL, 'f' },
        { "max_selectors", argparser::option::required_argument, NULL, 's' },
        { "selector_rdo_threshold", argparser::option::required_argument, NULL, 'u' },
        { "normal_map", argparser::option::no_argument, NULL, 'n' },
        { "separate_rg_to_color_alpha", argparser::option::no_argument, NULL, 'r' },
        { "no_endpoint_rdo", argparser::option::no_argument, NULL, 'o' },
        { "no_selector_rdo", argparser::option::no_argument, NULL, 'p' },
        // -NSDocumentRevisionsDebugMode YES is appended to the end
        // of the command by Xcode when debugging and "Allow debugging when
        // using document Versions Browser" is checked in the scheme. It
        // defaults to checked and is saved in a user-specific file not the
        // pbxproj file so it can't be disabled in a generated project.
        // Remove these from the arguments under consideration.
        { "-NSDocumentRevisionsDebugMode", argparser::option::required_argument, NULL, 'i' },
        { nullptr, argparser::option::no_argument, nullptr, 0 }
    };

    _tstring shortopts("bc:e:f:hmnrops:t:u:vl:q:");
    while ((ch = parser.getopt(&shortopts, option_list, NULL)) != -1) {
        switch (ch) {
          case 0:
            break;
          case 'l':
            options.levels = atoi(parser.optarg.c_str());
            break;
          case 'h':
            usage(appName);
            exit(0);
          case 'v':
            version(appName);
            exit(0);
          case 'b':
            options.bcmp = 1;
            options.ktx2 = 1;
            break;
          case 'c':
            options.bopts.compressionLevel = atoi(parser.optarg.c_str());
            break;
          case 'e':
            options.bopts.maxEndpoints = atoi(parser.optarg.c_str());
            break;
          case 'f':
            options.bopts.endpointRDOThreshold = strtof(parser.optarg.c_str(), nullptr);
            break;
          case 'm':
            options.bopts.noMultithreading = 1;
            break;
          case 'n':
            options.bopts.normalMap = 1;
            break;
          case 'o':
            options.bopts.noEndpointRDO = 1;
            break;
          case 'p':
            options.bopts.noSelectorRDO = 1;
            break;
          case 'q':
            options.bopts.qualityLevel = atoi(parser.optarg.c_str());
            break;
          case 'r':
            options.bopts.separateRGToRGB_A = 1;
            break;
          case 's':
            options.bopts.maxSelectors = atoi(parser.optarg.c_str());
            break;
          case 't':
            options.bopts.threadCount = atoi(parser.optarg.c_str());
            break;
          case 'u':
            options.bopts.selectorRDOThreshold = strtof(parser.optarg.c_str(), nullptr);
            break;
          case 'i':
            break;
          case '?':
          case ':':
          default:
            usage(appName);
            exit(1);
        }
    }
}

static bool
loadFileList(const _tstring &f, std::vector<_tstring>& filenames)
{
    _tstring listName(f);
    listName.erase(0, 1);

    FILE *lf = nullptr;
#ifdef _WIN32
    _tfopen_s(&lf, listName.c_str(), "r");
#else
    lf = _tfopen(listName.c_str(), "r");
#endif

    if (!lf) {
        fprintf(stderr, "%s: Failed opening filename list: \"%s\": %s\n",
                appName.c_str(), listName.c_str(), strerror(errno));
        return false;
    }

    uint32_t totalFilenames = 0;

    for (;;) {
        char buf[PATH_MAX];
        buf[0] = '\0';

        char *p = fgets(buf, sizeof(buf), lf);
        if (!p) {
          if (ferror(lf)) {
            fprintf(stderr, "%s: Failed reading filename list: \"%s\": %s\n",
                    appName.c_str(), listName.c_str(), strerror(errno));
            fclose(lf);
            return false;
          } else
            break;
        }

        std::string readFilename(p);
        while (readFilename.size()) {
            if (readFilename[0] == _T(' '))
              readFilename.erase(0, 1);
            else
              break;
        }

        while (readFilename.size()) {
            const char c = readFilename.back();
            if ((c == _T(' ')) || (c == _T('\n')) || (c == _T('\r')))
              readFilename.erase(readFilename.size() - 1, 1);
            else
              break;
        }

        if (readFilename.size()) {
            filenames.push_back(readFilename);
            totalFilenames++;
        }
    }

    fclose(lf);

    return true;
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
yflip(unsigned char*& srcImage, size_t imageSize,
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
          unsigned char* srcImage)
{
    char formatstr[2048];
    char *imagefmt;
    char *fmtname;
    int bitsPerComponent = componentSize == 2 ? 16 : 8;

    switch (components) {
      case 1:
        imagefmt = "r b=";
        fmtname = "R";
        break;
      case 2:
        imagefmt = "rg b=";
        fmtname = "RG";
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

// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 sts=4 expandtab:

// $Id: aaf1dc131758d6a2a60a7ad1e797c02451aecd32 $

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
#include "version.h"
#include "image.hpp"
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

using namespace std;

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
            uint32_t tc = thread::hardware_concurrency();
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
            cout << "threadCount = " << threadCount.value << endl;
            cout << "qualityLevel = " << qualityLevel.value << endl;
            cout << "maxEndpoints = " << maxEndpoints.value << endl;
            cout << "maxSelectors = " << maxSelectors.value << endl;
            cout << "structSize = " << structSize << endl;
            cout << "threadCount = " << ktxBasisParams::threadCount << endl;
            cout << "compressionLevel = " << compressionLevel << endl;
            cout << "qualityLevel = " << ktxBasisParams::qualityLevel << endl;
            cout << "compressionLevel = " << compressionLevel << endl;
            cout << "maxEndpoints = " << ktxBasisParams::maxEndpoints << endl;
            cout << "endpointRDOThreshold = " << endpointRDOThreshold << endl;
            cout << "maxSelectors = " << ktxBasisParams::maxSelectors << endl;
            cout << "selectorRDOThreshold = " << selectorRDOThreshold << endl;
            cout << "normalMap = " << normalMap << endl;
            cout << "separateRGToRGB_A = " << separateRGToRGB_A << endl;
            cout << "preSwizzle = " << preSwizzle << endl;
            cout << "noEndpointRDO = " << noEndpointRDO << endl;
            cout << "noSelectorRDO = " << noSelectorRDO << endl;
        }
#endif
    };
    struct mipgenOptions {
        string filter;
        float filterScale;
        enum basisu::Resampler::Boundary_Op wrapMode;

        mipgenOptions() : filter("lanczos4"), filterScale(1.0),
                  wrapMode(basisu::Resampler::Boundary_Op::BOUNDARY_CLAMP) { }
    };

    int          automipmap;
    int          cubemap;
    int          genmipmap;
    int          ktx2;
    int          metadata;
    int          mipmap;
    int          two_d;
    Image::eOETF oetf;
    int          useStdin;
    int          lower_left_maps_to_s0t0;
    int          bcmp;
    int          test;
    struct basisOptions bopts;
    struct mipgenOptions gmopts;
    _tstring     outfile;
    unsigned int depth;
    unsigned int layers;
    unsigned int levels;
    vector<_tstring> infilenames;

    commandOptions() {
      automipmap = false;
      cubemap = false;
      genmipmap = false;
      ktx2 = 0;
      metadata = 1;
      mipmap = 0;
      two_d = false;
      useStdin = false;
      bcmp = false;
      test = false;
      depth = 1;
      layers = 1;
      levels = 1;
      oetf = Image::eOETF::Unset;
      /* The OGLES WG recommended approach, even though it is opposite
       * to the OpenGL convention. Suki ja nai.
       */
      lower_left_maps_to_s0t0 = 0;
    }
};

static _tstring      appName;

static bool loadFileList(const _tstring &f, bool relativize,
                         vector<_tstring>& filenames);
static ktx_uint32_t log2(ktx_uint32_t v);
static void processCommandLine(int argc, _TCHAR* argv[],
                               struct commandOptions& options);
static void processOptions(argparser& parser, struct commandOptions& options);
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
    Create a Khronos format texture file (KTX) from a set of PNG (.png)
    or Netpbm format (.pam, .pgm, .ppm) images. It writes the destination
    ktx file to @e outfile, appending ".ktx{,2}" if necessary. If @e outfile
    is '-' the output will be written to stdout.
 
    @b toktx reads each named @e infile. which must be in .png, .pam, .ppm,
    or .pgm format. @e infiles prefixed with '@' are read as text files listing
    actual file names to process with one file path per line. Paths must be
    absolute or relative to the current directory when @b toktx is run. If
    '\@@' is used instead, paths must be absolute or relative to the location
    of the list file.

    .png files yield RED, RG, RGB or RGBA textures according to the files's
    @e color type, .ppm files RGB textures, .pgm files RED textures and .pam
    files RED, RG, RGB or RGBA textures according to the file's TUPLTYPE
    and DEPTH. Other formats can be readily converted to the supported
    formats using tools such as ImageMagick and XnView.

    The primaries, transfer function (OETF) and the texture's sRGB-ness is set
    based on the input file. Netpbm files always use BT.709/sRGB primaries and
    the BT.709 OETF. @b toktx tranforms the image to the sRGB OETF, sets the
    transfer function to sRGB and creates sRGB textures for these inputs.

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
        <dd>If gAMA is 45455 the OETF is set to sRGB, if 100000 it is set to
        linear. Other gAMA values are unsupported. cHRM is currently
        unsupported. We should attempt to map the primary values to one of
        the standard sets listed in the Khronos Data Format specification.</dd>
    </dl>

    The following options are always available:
    <dl>
    <dt>--2d</dt>
    <dd>If the image height is 1, by default a KTX file for a 1D texture is
        created. With this option one for a 2D texture is created instead.</dd>
    <dt>--automipmap</dt>
    <dd>Causes the KTX file to be marked to request generation of a mipmap
        pyramid when the file is loaded. This option is mutually exclusive
        with @b --genmipmap, @b --levels and @b --mipmap.</dd>
    <dt>--cubemap</dt>
    <dd>KTX file is for a cubemap. At least 6 @e infiles must be provided,
        more if @b --mipmap or @b --layers is also specified. Provide the
        images in the order +X, -X, +Y, -Y, +Z, -Z where the arrangement is a
        left-handed coordinate system with +Y up. So if you're facing +Z,
        -X will be on your left and +X on your right. If @b --layers &gt; 1 is
        specified, provide the faces for layer 0 first then for layer 1, etc.
        Images must have an upper left origin so --lower_left_maps_to_s0t0
        is ignored with this option.</dd>
    <dt>--depth &lt;number&gt;</dt>
    <dd>KTX file is for a 3D texture with a depth of @e number where
        @e number &gt; 1. Provide the file(s) for z=0 first then those for
        z=1, etc. It is an error to specify this together with
        @b --layers &gt; 1 or @b --cubemap.</dd>
    <dt>--genmipmap</dt>
    <dd>Causes mipmaps to be generated for each input file. This option is
        mutually exclusive with @b --automipmap and @b --mipmap. When set,
        the following mipmap-generation related options become valid,
        otherwise they are ignored.
        <dl>
        <dt>--filter &lt;name&gt;</dt>
        <dd>Specifies the filter to use when generating the mipmaps. @e name
            is a string. The default is @e lanczos4. The following names are
            recognized: @e box, @e tent, @e bell, @e b-spline, @e mitchell,
            @e lanczos3, @e lanczos4, @e lanczos6, @e lanczos12, @e blackman,
            @e kaiser, @e gaussian, @e catmullrom, @e quadratic_interp,
            @e quadratic_approx and @e quadratic_mix.</dd>
        <dt>--fscale &lt;floatVal&gt;</dt>
        <dd>The filter scale to use. The default is 1.0.</dd>
        <dt>--wmode &lt;mode&gt;</dt>
        <dd>Specify how to sample pixels near the image boundaries. Values
            are @e wrap, @e reflect and @e clamp. The default is @e clamp.</dd>
        </dl>
    </dd>
    <dt>--layers &lt;number&gt;</dt>
    <dd>KTX file is for an array texture with @e number of layers where
        @e number &gt; 1. Provide the file(s) for layer 0 first then those
        for layer 1, etc. It is an error to specify this together with
        @b --depth &gt; 1.</dd>
    <dt>--levels &lt;number&gt;</dt>
    <dd>KTX file is for a mipmap pyramid with @e number of levels rather than
        a full pyramid. @e number must be &lt;= the maximum number of levels
        determined from the size of the base image. Provide the base level
        image first, if using @b --mipmap. This option is mutually exclusive
        with @b --automipmap.</dd>
    <dt>--mipmap</dt>
    <dd>KTX file is for a mipmap pyramid with one @b infile being explicitly
        provided for each level. Provide the images in the order of layer
        then face or depth slice then level with the base-level image first
        then in order down to the 1x1 image or the level specified by
        @b --levels.  This option is mutually exclusive with @b --automipmap
        and @b --genmipmap. @note This ordering differs from that in the
        created texture as it is felt to be more user-friendly.</dd>
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
        ignores the orientation value, the image will appear upside down.
        This option is ignored with @b --cubemap.</dd>
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
        transform of Netpbm images will not be performed.</dd>
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
          numnber of threads reported by @c thread::hardware_concurrency or
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
          space XY normal maps). Only needed with 3 or 4 component input
          images.</dd>
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

@par Version 4.0 (using new version numbering system)
  - Add KTX version 2 support including Basis Universal encoding.
  - Add .png reader.
  - Transform NetPBM input files to sRGB OETF.
  - Add mipmap generation.
  - Remove legacy items.

@par Version 1.3
 - Switch to ktxTexture API.
 - Add --levels option.
 - Add --2d option.
 
@par Version 1.2
 - Remove --sized; always create sized format.
 - Write metadata by default.
 - Bug fixes.

@par Version 1.1
 - Moved --alpha and --luminance to legacy.

@section toktx_author AUTHOR
    Mark Callow, Edgewise Consulting www.edgewise-consulting.com
*/

// I really HATE this duplication of text but I cannot find a simple way to
// avoid it that works on all platforms (e.g running man toktx) even if I was
// willing to tolerate markup commands in the usage output.
static void
usage(const _tstring appName)
{
    fprintf(stderr, 
        "Usage: %s [options] <outfile> [<infile>.{pam,pgm,ppm} ...]\n"
        "\n"
        "  <outfile>    The destination ktx file. \".ktx\" will appended if necessary.\n"
        "               If it is '-' the output will be written to stdout.\n"
        "  <infile>     One or more image files in .png, .pam, .ppm, or .pgm format. Other\n"
        "               formats can be readily converted to these formats using tools\n"
        "               such as ImageMagick and XnView. When no infile is specified,\n"
        "               stdin is used. infiles prefixed with '@' are read as text files\n"
        "               listing actual file names to process with one file path per line.\n"
        "               Paths must be absolute or relative to the current directory when\n"
        "               toktx is run. If '@@' is used instead, paths must be absolute or\n"
        "               relative to the location of the list file.\n"
        "\n"
        "               .png files yield RED, RG, RGB or RGBA textures according to the\n"
        "               files's color type, .ppm files RGB textures, .pgm files RED\n"
        "               textures and .pam files RED, RG, RGB or RGBA textures according\n"
        "               to the file's TUPLTYPE and DEPTH.\n"
        "\n"
        "  Options are:\n"
        "\n"
        "  --2d         If the image height is 1, by default a KTX file for a 1D\n"
        "               texture is created. With this option one for a 2D texture is\n"
        "               created instead.\n"
        "  --automipmap Causes the KTX file to be marked to request generation of a\n"
        "               mipmap pyramid when the file is loaded. This option is mutually\n"
        "               exclusive with --genmipmap, --levels and --mipmap.\n"
        "  --cubemap    KTX file is for a cubemap. At least 6 <infile>s must be provided,\n"
        "               more if --mipmap is also specified. Provide the images in the\n"
        "               order +X, -X, +Y, -Y, +Z, -Z where the arrangement is a\n"
        "               left-handed coordinate system with +Y up. So if you're facing +Z,\n"
        "               -X will be on your left and +X on your right. If --layers > 1\n"
        "               is specified, provide the faces for layer 0 first then for\n"
        "               layer 1, etc. Images must have an upper left origin so\n"
        "               --lower_left_maps_to_s0t0 is ignored with this option.\n"
        "  --depth <number>\n"
        "               KTX file is for a 3D texture with a depth of number where\n"
        "               number > 1. Provide the file(s) for z=0 first then those for\n"
        "               z=1, etc. It is an error to specify this together with\n"
        "               --layers > 1 or --cubemap.\n"
        "  --genmipmap  Causes mipmaps to be generated for each input file. This option\n"
        "               is mutually exclusive with --automipmap and --mipmap. When set\n"
        "               the following mipmap-generation related options become valid,\n"
        "               otherwise they are ignored.\n"
        "      --filter <name>\n"
        "               Specifies the filter to use when generating the mipmaps. name\n"
        "               is a string. The default is lanczos4. The following names are\n"
        "               recognized: box, tent, bell, b-spline, mitchell, lanczos3\n"
        "               lanczos4, lanczos6, lanczos12, blackman, kaiser, gaussian,\n"
        "               catmullrom, quadratic_interp, quadratic_approx and\n"
        "               quadratic_mix.\n"
        "      --fscale <floatVal>\n"
        "               The filter scale to use. The default is 1.0.\n"
        "      --wmode <mode>\n"
        "               Specify how to sample pixels near the image boundaries. Values\n"
        "               are wrap, reflect and clamp. The default is clamp.\n"
        "  --layers <number>\n"
        "               KTX file is for an array texture with number of layers\n"
        "               where number > 1. Provide the file(s) for layer 0 first then\n"
        "               those for layer 1, etc. It is an error to specify this\n"
        "               together with --depth > 1.\n"
        "  --levels <number>\n"
        "               KTX file is for a mipmap pyramid with number of levels rather\n"
        "               than a full pyramid. number must be <= the maximum number of\n"
        "               levels determined from the size of the base image. This option is\n"
        "               mutually exclusive with @b --automipmap.\n"
        "  --mipmap     KTX file is for a mipmap pyramid with one infile being explicitly\n"
        "               provided for each level. Provide the images in the order of layer\n"
        "               then face or depth slice then level with the base-level image\n"
        "               first then in order down to the 1x1 image or the level specified\n"
        "               by --levels.  This option is mutually exclusive with --automipmap\n"
        "               and --genmipmap. Note that this ordering differs from that in the\n"
        "               created texture as it is felt to be more user-friendly.\n"
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
        "               This causes the input netpbm and PNG images to be flipped\n"
        "               vertically to a lower-left origin. When this option is in effect,\n"
        "               toktx writes a KTXorientation value of S=r,T=u into the output\n"
        "               file to inform loaders of the logical orientation. If a Vulkan\n"
        "               loader ignores the orientation value, the image will appear\n"
        "               upside down. This option is ignored with --cubemap.\n"
        "  --linear     Force the created texture to have a linear transfer function.\n"
        "               Use this only when you know the file format information is wrong\n"
        "               and the input file uses a linear transfer function. If this is\n"
        "               specified, the default transform of Netpbm images to sRGB color\n"
        "               space will not be performed.\n"
        "  --srgb       Force the created texture to have an srgb transfer function.\n"
        "               As with --linear, use with caution.  Like @b --linear, the\n"
        "               default color transform of Netpbm images will not be performed.\n"
        "  --t2         Output in KTX2 format. Default is KTX.\n"
        "  --bcmp\n"
        "               Supercompress the image data with Basis Universal. Implies --t2.\n"
        "               RED images will become RGB with RED in each component. RG images\n"
        "               will have R in the RGB part and G in the alpha part of the\n"
        "               compressed texture. When set, the following Basis-related\n"
        "               options become valid, otherwise they are ignored.\n\n"
        "      --no_multithreading\n"
        "               Disable multithreading. By default Basis compression will use\n"
        "               the numnber of threads reported by\n"
        "               @c thread::hardware_concurrency or 1 if value returned is 0.\n"
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
        "               space XY normal maps). Only needed with 3 or 4 component input\n"
        "               images.\n"
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

#define QUOTE(x) #x
#define STR(x) QUOTE(x)

static void
writeId(ostream& dst, const _tstring& appName, bool test = false)
{
    dst << appName << " " << (test ? STR(TOKTX_DEFAULT_VERSION)
                                   : STR(TOKTX_VERSION));
}

static void
version(const _tstring& appName)
{
    writeId(cerr, appName);
    cerr << endl;
}

static uint32_t
imageCount(uint32_t levelCount, uint32_t layerCount,
           uint32_t faceCount, uint32_t baseDepth)
{
    assert((faceCount == 1 && baseDepth >= 1)
           || faceCount > 1 && baseDepth == 1);

    uint32_t layerPixelDepth = baseDepth;
    for(uint32_t level = 1; level < levelCount; level++)
        layerPixelDepth += maximum(baseDepth >> level, 1U);
    // NOTA BENE: faceCount * layerPixelDepth is only reasonable because
    // faceCount and depth can't both be > 1. I.e there are no 3d cubemaps.
    return layerCount * faceCount * layerPixelDepth;
}

int _tmain(int argc, _TCHAR* argv[])
{
    KTX_error_code ret;
    ktxTextureCreateInfo createInfo;
    ktxTexture* texture = 0;
    struct commandOptions options;
    int exitCode = 0;
    unsigned int componentCount = 1, faceSlice, i, level, layer, levelCount = 1;
    unsigned int levelWidth, levelHeight, levelDepth;
    Image::eOETF chosenOETF, firstImageOETF;

    processCommandLine(argc, argv, options);

    if (options.cubemap)
      createInfo.numFaces = 6;
    else
      createInfo.numFaces = 1;

    // TO DO: handle array textures
    createInfo.numLayers = options.layers;
    createInfo.isArray = KTX_FALSE;

    // TO DO: handle 3D textures.

    faceSlice = layer = level = 0;
    for (i = 0; i < options.infilenames.size(); i++) {
        _tstring& infile = options.infilenames[i];

        Image* image;
        try {
            image =
              Image::CreateFromFile(infile, options.oetf == Image::eOETF::Unset);
        } catch (exception& e) {
            cerr << appName << ": failed to create image from "
                      << infile << ". " << e.what() << endl;
            exit(2);
        }

        /* Sanity check. */
        assert(image->getWidth() * image->getHeight() * image->getPixelSize()
                  == image->getByteCount());

        if (image->getHeight() > 1 && options.lower_left_maps_to_s0t0) {
            image->yflip();
        }

        if (i == 0) {
            // First file.
            bool srgb;

            firstImageOETF = image->getOetf();
            if (options.oetf == Image::eOETF::Unset) {
                chosenOETF = firstImageOETF;
            } else {
                chosenOETF = options.oetf;
            }

            componentCount = image->getComponentCount();
            srgb = (chosenOETF == Image::eOETF::sRGB);
            switch (componentCount) {
              case 1:
                switch (image->getComponentSize()) {
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
                 switch (image->getComponentSize()) {
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
                 switch (image->getComponentSize()) {
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
                 switch (image->getComponentSize()) {
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
            createInfo.baseWidth = levelWidth = image->getWidth();
            createInfo.baseHeight = levelHeight = image->getHeight();
            createInfo.baseDepth = levelDepth = options.depth;
            if (image->getWidth() == 1 && !options.two_d)
                createInfo.numDimensions = 1;
            else
                createInfo.numDimensions = 2;
            if (options.automipmap) {
                createInfo.numLevels = 1;
                createInfo.generateMipmaps = KTX_TRUE;
            } else {
                createInfo.generateMipmaps = KTX_FALSE;
                if (options.mipmap || options.genmipmap) {
                    // Calculate number of miplevels
                    GLuint max_dim = image->getWidth() > image->getHeight() ?
                                     image->getWidth() : image->getHeight();
                    createInfo.numLevels = log2(max_dim) + 1;
                    if (options.levels > 1) {
                        if (options.levels > createInfo.numLevels) {
                            cerr << appName << "--levels value is greater than "
                                 << "the maximum levels for the image size."
                                 << endl;
                            exitCode = 1;
                            goto cleanup;
                        }
                        // Override the above.
                        createInfo.numLevels = options.levels;
                    }
                } else {
                    createInfo.numLevels = 1;
                }
                // Figure out how many levels we'll read from files.
                if (options.mipmap) {
                    levelCount = createInfo.numLevels;
                } else {
                    levelCount = 1;
                }
            }
            // Check we have enough files.
            uint32_t requiredFileCount = imageCount(options.genmipmap ? 1 : levelCount,
                                             createInfo.numLayers,
                                             createInfo.numFaces,
                                             createInfo.baseDepth);
            if (requiredFileCount > options.infilenames.size()) {
                cerr << appName << ": too few files for " << levelCount
                     << " levels, " << createInfo.numLayers
                     << " layers and " << createInfo.numFaces
                     << " faces." << endl;
                exitCode = 1;
                goto cleanup;
            } else if (requiredFileCount < options.infilenames.size()) {
                cerr << appName << ": too many files for " << levelCount
                     << " levels, " << createInfo.numLayers
                     << " layers and " << createInfo.numFaces
                     << " faces. Extras will be ignored." << endl;
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
            // Subsequent files.
            if (image->getOetf() != firstImageOETF) {
                fprintf(stderr, "%s: \"%s\" is encoded with a different transfer"
                                " function (OETF) than preceding files.\n",
                                appName.c_str(), infile.c_str());
                exitCode = 1;
                goto cleanup;
            }
            // Input file order is layer, faceSlice, level. This seems easier for
            // a human to manage than the order in a KTX file. It keeps the
            // base level images and their mip levels together.
            level++;
            levelWidth >>= 1;
            levelHeight >>= 1;
            levelDepth >>= 1;
            if (level == levelCount) {
                faceSlice++;
                level = 0;
                levelWidth = createInfo.baseWidth;
                levelHeight = createInfo.baseHeight;
                if (faceSlice == (options.cubemap ? 6 : levelDepth)) {
                    faceSlice = 0;
                    layer++;
                    if (layer == options.layers) {
                        // We're done.
                        break;
                    }
                }
            }
        }

        if (options.cubemap && image->getWidth() != image->getHeight()
            && image->getWidth() != levelWidth) {
            cerr << appName << ": \"" << infile << "\" intended for a cubemap "
                            << "face, is not square or has incorrect" << endl
                            << "size for current mipmap level" << endl;
            exitCode = 1;
            goto cleanup;
        }
#if TRAVIS_DEBUG
        if (options.bcmp) {
            cout << "level = " << level << ", faceSlice = " << faceSlice;
            cout << ", srcImg = " << hex  << (void *)srcImg << dec;
            cout << ", imageSize = " << imageSize << endl;
        }
#endif
        ktxTexture_SetImageFromMemory(ktxTexture(texture),
                                      level,
                                      layer,
                                      faceSlice,
                                      *image,
                                      image->getByteCount());
        if (options.genmipmap) {
            for (uint32_t level = 1; level < createInfo.numLevels; level++)
            {
                // Note: level variable in this loop is different from that
                // with the same name outside it.
                const uint32_t levelWidth
                    = maximum<uint32_t>(1, image->getWidth() >> level);
                const uint32_t levelHeight
                    = maximum<uint32_t>(1, image->getHeight() >> level);

                Image *levelImage = image->createImage(levelWidth, levelHeight);

                try {
                    image->resample(*levelImage,
                                    chosenOETF == Image::eOETF::sRGB,
                                    options.gmopts.filter.c_str(),
                                    options.gmopts.filterScale,
                                    options.gmopts.wrapMode);
                } catch (runtime_error e) {
                    cerr << "Image::resample() failed! "
                              << e.what() << endl;
                    exitCode = 1;
                    goto cleanup;
                }

                // TODO: and an option for renormalize;
                //if (m_params.m_mip_renormalize)
                //    levelImage->renormalize_normal_map();

                ktxTexture_SetImageFromMemory(ktxTexture(texture),
                                              level,
                                              layer,
                                              faceSlice,
                                              *levelImage,
                                              levelImage->getByteCount());
                delete levelImage;
            }
        }

#if IMAGE_DEBUG
        {
            ktx_size_t offset;
            ktxTexture_GetImageOffset(texture, level, 0, faceSlice, &offset);
            dumpImage(infile, image->getWidth(), image->getHeight(),
                      image->getComponentCount(), image->getComponentSize(),
                      texture.pData + offset);
        }
#endif

        delete image;
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
        stringstream writer;
        writeId(writer, appName, options.test);
        ktxHashList_AddKVPair(&texture->kvDataHead, KTX_WRITER_KEY,
                              (ktx_uint32_t)writer.str().length() + 1,
                              writer.str().c_str());
    }

    FILE* f;
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
            if (bopts.normalMap && chosenOETF != Image::eOETF::Linear) {
                fprintf(stderr, "%s: --normal_map specified but input file(s) are"
                        " not linear.", appName.c_str());
                exitCode = 1;
                goto cleanup;
            }
            if (bopts.noMultithreading)
                bopts.threadCount = 1;
            if (componentCount == 1 || componentCount == 2) {
                // Ensure this is not set as it would result in R in both
                // RGB and A. This is because we have to pass RGBA to the BasisU
                // encoder and, since a 2-channel file is considered
                // grayscale-alpha, the "grayscale" component is swizzled to
                // RGB and the alpha component is swizzled to A. (The same thing
                // happens in `basisu` and the BasisU library because lodepng,
                // which it uses, does the same swizzling.) If this flag is
                // set the BasisU encoder will then copy "G" which is actually
                // "R" into A.
                bopts.separateRGToRGB_A = false;
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
        istringstream iss(toktx_options);
        argvector arglist;
        for (_tstring w; iss >> w; )
            arglist.push_back(w);

        argparser optparser(arglist, 0);
        processOptions(optparser, options);
        if (optparser.optind != arglist.size()) {
            cerr << "Only options are allowed in the TOKTX_OPTIONS "
                 << "environment variable." << endl;
            usage(appName);
            exit(1);
        }
    }

    argparser parser(argc, argv);
    processOptions(parser, options);

    if (options.automipmap + options.genmipmap + options.mipmap > 1) {
        cerr << "Only one of --automipmap, --genmipmap and "
             << "--mipmap may be specified." << endl;
        usage(appName);
        exit(1);
    }
    if ((options.automipmap || options.genmipmap) && options.levels > 1) {
        cerr << "Cannot specify --levels > 1 with --automipmap or --genmipmap."
            << endl;
        usage(appName);
        exit(1);
    }
    if (options.cubemap && options.lower_left_maps_to_s0t0) {
        cerr << "Cubemaps require images to have an upper-left origin. "
             << "Ignoring --lower_left_maps_to_s0t0.\n" << endl;
        options.lower_left_maps_to_s0t0 = 0;
    }
    if (options.cubemap && options.depth > 1) {
        cerr << "Cubemaps cannot have depth > 1." << endl;
        usage(appName);
        exit(1);
    }
    if (options.layers > 1 && options.depth > 1) {
        cerr << "Cannot have 3D array textures." << endl;
        usage(appName);
        exit(1);
    }

    i = parser.optind;

    if (argc - i > 0) {
        options.outfile = parser.argv[i++];

        if (options.outfile.compare(_T("-")) != 0
            && options.outfile.find_last_of('.') == _tstring::npos)
        {
            options.outfile.append(options.ktx2 ? _T(".ktx2") : _T(".ktx"));
        }

        for (; i < argc; i++) {
            if (parser.argv[i][0] == _T('@')) {
                if (!loadFileList(parser.argv[i],
                                  parser.argv[i][1] == _T('@'),
                                  options.infilenames)) {
                    exit(1);
                }
            } else {
                options.infilenames.push_back(parser.argv[i]);
            }
        }
        if (options.infilenames.size() > 0) {
            /* Check for attempt to use stdin as one of the
             * input files.
             */
            vector<_tstring>::const_iterator it;
            for (it = options.infilenames.begin(); it < options.infilenames.end(); it++) {
                if (it->compare(_T("-")) == 0) {
                    cerr << "Cannot use stdin as one among many inputs."
                         << endl;
                    usage(appName);
                    exit(1);
                }
            }
            ktx_uint32_t requiredInputFiles = options.cubemap ? 6 : 1 * options.levels;
            if (requiredInputFiles > options.infilenames.size()) {
                cerr << "Too few input files." << endl;
                exit(1);
            }
            /* Whether there are enough input files for all the mipmap levels in
             * a full pyramid can only be checked when the first file has been
             * read and the size determined.
             */
        } else {
            cerr << "Need some input files." << endl;
            usage(appName);
            exit(1);
        }
    } else {
        cerr << "Need an output file and some input files." << endl;
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
    int ch;
    static struct argparser::option option_list[] = {
        { "help", argparser::option::no_argument, NULL, 'h' },
        { "version", argparser::option::no_argument, NULL, 'v' },
        { "2d", argparser::option::no_argument, &options.two_d, 1 },
        { "automipmap", argparser::option::no_argument, &options.automipmap, 1 },
        { "cubemap", argparser::option::no_argument, &options.cubemap, 1 },
        { "genmipmap", argparser::option::no_argument, &options.genmipmap, 1 },
        { "filter", argparser::option::required_argument, NULL, 'f' },
        { "fscale", argparser::option::required_argument, NULL, 'F' },
        { "wrapping", argparser::option::required_argument, NULL, 'w' },
        { "depth", argparser::option::required_argument, NULL, 'd' },
        { "layers", argparser::option::required_argument, NULL, 'a' },
        { "levels", argparser::option::required_argument, NULL, 'l' },
        { "mipmap", argparser::option::no_argument, &options.mipmap, 1 },
        { "nometadata", argparser::option::no_argument, &options.metadata, 0 },
        { "lower_left_maps_to_s0t0", argparser::option::no_argument, &options.lower_left_maps_to_s0t0, 1 },
        { "upper_left_maps_to_s0t0", argparser::option::no_argument, &options.lower_left_maps_to_s0t0, 0 },
        { "linear", argparser::option::no_argument, (int*)&options.oetf, OETF_LINEAR },
        { "srgb", argparser::option::no_argument, (int*)&options.oetf, OETF_SRGB },
        { "t2", argparser::option::no_argument, &options.ktx2, 1},
        { "bcmp", argparser::option::no_argument, NULL, 'b' },
        { "no_multithreading", argparser::option::no_argument, NULL, 'N' },
        { "threads", argparser::option::required_argument, NULL, 't' },
        { "clevel", argparser::option::required_argument, NULL, 'c' },
        { "qlevel", argparser::option::required_argument, NULL, 'q' },
        { "max_endpoints", argparser::option::required_argument, NULL, 'e' },
        { "endpoint_rdo_threshold", argparser::option::required_argument, NULL, 'E' },
        { "max_selectors", argparser::option::required_argument, NULL, 's' },
        { "selector_rdo_threshold", argparser::option::required_argument, NULL, 'u' },
        { "normal_map", argparser::option::no_argument, NULL, 'n' },
        { "separate_rg_to_color_alpha", argparser::option::no_argument, NULL, 1000 },
        { "no_endpoint_rdo", argparser::option::no_argument, NULL, 'o' },
        { "no_selector_rdo", argparser::option::no_argument, NULL, 'p' },
        { "test", argparser::option::no_argument, &options.test, 1},
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
          case 'a':
            options.layers = atoi(parser.optarg.c_str());
            break;
          case 'd':
            options.depth = atoi(parser.optarg.c_str());
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
          case 'E':
            options.bopts.endpointRDOThreshold = strtof(parser.optarg.c_str(), nullptr);
            break;
          case 'f':
            options.gmopts.filter = parser.optarg;
            break;
          case 'F':
            options.gmopts.filterScale = strtof(parser.optarg.c_str(), nullptr);
            break;
          case 'w':
            if (!parser.optarg.compare("wrap")) {
                options.gmopts.wrapMode
                      = basisu::Resampler::Boundary_Op::BOUNDARY_WRAP;
            } else if (!parser.optarg.compare("clamp")) {
                options.gmopts.wrapMode
                      = basisu::Resampler::Boundary_Op::BOUNDARY_CLAMP;
            } else if (!parser.optarg.compare("reflect")) {
                options.gmopts.wrapMode
                      = basisu::Resampler::Boundary_Op::BOUNDARY_REFLECT;
            } else {
                cerr << "Unrecognized mode \"" << parser.optarg
                     << "\" passed to --wmode" << endl;
                usage(appName);
                exit(1);
            }
            break;
          case 'N':
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
          case 1000:
            options.bopts.separateRGToRGB_A = 1;
            break;
          case 's':
            options.bopts.maxSelectors = atoi(parser.optarg.c_str());
            break;
          case 'S':
            options.bopts.selectorRDOThreshold = strtof(parser.optarg.c_str(), nullptr);
            break;
          case 't':
            options.bopts.threadCount = atoi(parser.optarg.c_str());
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
loadFileList(const _tstring &f, bool relativize,
             vector<_tstring>& filenames)
{
    _tstring listName(f);
    listName.erase(0, relativize ? 2 : 1);

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
    _tstring dirname;

    if (relativize) {
        size_t dirnameEnd = listName.find_last_of('/');
        if (dirnameEnd == string::npos) {
            relativize = false;
        } else {
            dirname = listName.substr(0, dirnameEnd + 1);
        }
    }

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

        string readFilename(p);
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
            if (relativize)
                filenames.push_back(dirname + readFilename);
            else
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

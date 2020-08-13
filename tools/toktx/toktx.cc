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
#include <inttypes.h>
#include <zstd.h>

#include "GL/glcorearb.h"
#include "ktx.h"
#include "../../lib/vkformat_enum.h"
#include "argparser.h"
#include "scapp.h"
#include "version.h"
#include "image.hpp"
#if (IMAGE_DEBUG) && defined(_DEBUG) && defined(_WIN32) && !defined(_WIN32_WCE)
#  include "imdebug.h"
#elif defined(IMAGE_DEBUG) && IMAGE_DEBUG
#  undef IMAGE_DEBUG
#  define IMAGE_DEBUG 0
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

static ktx_uint32_t log2(ktx_uint32_t v);
#if IMAGE_DEBUG
static void dumpImage(_TCHAR* name, int width, int height, int components,
                      int componentSize, unsigned char* srcImage);
#endif

using namespace std;

/** @page toktx toktx
@~English

Create a KTX file from JPEG, PNG or netpbm format files.
 
@section toktx_synopsis SYNOPSIS
    toktx [options] @e outfile [@e infile.{pam,pgm,ppm} ...]

@section toktx_description DESCRIPTION
    Create a Khronos format texture file (KTX) from a set of JPEG (.jpg),
    PNG (.png) or Netpbm format (.pam, .pgm, .ppm) images. It writes the
    destination ktx file to @e outfile, appending ".ktx{,2}" if necessary. If
    @e outfile is '-' the output will be written to stdout.
 
    @b toktx reads each named @e infile. which must be in .jpg, .png, .pam,
    .ppm or .pgm format. @e infiles prefixed with '@' are read as text files listing
    actual file names to process with one file path per line. Paths must be
    absolute or relative to the current directory when @b toktx is run. If
    '\@@' is used instead, paths must be absolute or relative to the location
    of the list file.

    .jpg files yeild RED or RGB textures according to the actual components
    in the file. .png files yield RED, RG, RGB or RGBA textures according to
    the files's @e color type, .ppm files RGB textures, .pgm files RED textures
    and .pam files RED, RG, RGB or RGBA textures according to the file's
    TUPLTYPE and DEPTH. Other formats can be readily converted to the
    supported formats using tools such as ImageMagick and XnView.

    The primaries, transfer function (OETF) and the texture's sRGB-ness is set
    based on the input file unless @b --linear or @b --srgb is specified. For .jpg
    files @b toktx always sets BT709/sRGB primaries and the sRGB OETF in the
    output file and creates sRGB format textures. Netpbm files always use
    BT.709/sRGB primaries and the BT.709 OETF. @b toktx tranforms these
    images to the sRGB OETF, sets BT709/sRGB primaries and the sRGB OETF in
    the output file and creates sRGB format textures.

    For .png files the OETF is set as follows:

    <dl>
    <dt>No color-info chunks or sRGB chunk present:</dt>
        <dd>primaries are set to BT.709 and OETF to sRGB.</dd>
    <dt>sRGB chunk present:</dt>
        <dd>primaries are set to BT.709 and OETF to sRGB. gAMA and cHRM chunks
        are ignored.</dd>
    <dt>iCCP chunk present:</dt>
        <dd>General ICC profiles are not yet supported by toktx or the KTX2 format.
        In future these images may be transformed to linear or sRGB OETF as
        appropriate for the profile. sRGB chunk must not be present.</dd>
    <dt>gAMA and/or cHRM chunks present without sRGB or iCCP:</dt>
        <dd>If gAMA is < 60000 the image is transformed to and the OETF is set to
        sRGB. otherwise the image is transformed to and the OETF is set to
        linear. The color primaries in cHRM are matched to one of the
        standard sets listed in the Khronos Data Format Specification (the
        KHR_DF_PRIMARIES values from khr_df.h) and the primaries
        field of the output file's DFD is set to the matched value. If no match is
        found the primaries field is set to UNSPECIFIED.</dd>
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
        @b --levels.  @note This ordering differs from that in the
        created texture as it is felt to be more user-friendly.

        This option is mutually exclusive with @b --automipmap and
        @b --genmipmap.</dd>
    <dt>--nometadata</dt>
    <dd>Do not write KTXorientation metadata into the output file. Metadata
        is written by default. Use of this option is not recommended.</dd>
    <dt>--nowarn</dt>
    <dd>Silence warnings which are issued when certain transformations are
        performed on input images.</dd>
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
        This option is ignored with @b --cubemap. </dd>
    <dt>--linear</dt>
    <dd>Force the created texture to have a linear transfer function. If this is
        specified, implicit or explicit color space information from the input file(s)
        will be ignored and no color transformation will be performed. USE WITH
        CAUTION preferably only when you know the file format information is
        wrong.
    </dd>
    <dt>--srgb</dt>
    <dd>Force the created texture to have an srgb transfer function. Like
        @b --linear, USE WITH CAUTION. As with @b --linear, no color
        transformation will be performed.</dd>
    <dt>--resize &lt;width&gt;x&lt;height&gt;
    <dd>Resize images to @e width X @e height. This should not be used with
        @b --mipmap as it would resize all the images to the same size.
        Resampler options can be set via @b --filter and  @b --fscale. </dd>
    <dt>--scale &lt;value&gt;</dt>
    <dd>Scale images by @e value as they are read. Resampler options can
        be set via @b --filter and  @b --fscale. </dd>.
    <dt>--t2</dt>
    <dd>Output in KTX2 format. Default is KTX.</dd>
    </dl>
    @snippet{doc} scApp.h scApp options

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
  - Add .png and .jpg readers.
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

#define QUOTE(x) #x
#define STR(x) QUOTE(x)

string myversion(STR(TOKTX_VERSION));
string mydefversion(STR(TOKTX_DEFAULT_VERSION));

class toktxApp : public scApp {
  public:
    toktxApp();

    virtual int main(int argc, _TCHAR* argv[]);
    virtual void usage();

    void warning(const char *pFmt, va_list args);

  protected:
    virtual bool processOption(argparser& parser, int opt);
    void processEnvOptions();
    void validateOptions();

    struct commandOptions : public scApp::commandOptions {
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
        int          metadata;
        int          mipmap;
        int          two_d;
        khr_df_transfer_e oetf;
        int          useStdin;
        int          lower_left_maps_to_s0t0;
        int          warn;
        struct mipgenOptions gmopts;
        unsigned int depth;
        unsigned int layers;
        unsigned int levels;
        float        scale;
        int          resize;
        struct {
            unsigned int width;
            unsigned int height;
        } newGeom;

        commandOptions() {
            automipmap = 0;
            cubemap = 0;
            genmipmap = 0;
            ktx2 = 0;
            metadata = 1;
            mipmap = 0;
            two_d = 0;
            useStdin = 0;
            test = 0;
            depth = 1;
            layers = 1;
            levels = 1;
            oetf = KHR_DF_TRANSFER_UNSPECIFIED;
            // As required by spec. Opposite of OpenGL {,ES}, same as
            // Vulkan, et al.
            lower_left_maps_to_s0t0 = 0;
            warn = 1;
            scale = 1.0f;
            resize = 0;
            newGeom.width = newGeom.height = 0;
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
            cout << "uastc = " << uastc << endl;
            cout << "uastcFlags = " << uastcFlags << endl;
            cout << "uastcRDO = " << uastcRDO << endl;
            cout << "uastcRDODictSize = " << uastcRDODictSize << endl;
            cout << "uastcRDOQualityScalar = " << uastcRDOQualityScalar << endl;
        }
#endif
    } options;
};

toktxApp::toktxApp() : scApp(myversion, mydefversion, options)
{
    argparser::option my_option_list[] = {
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
        { "nowarn", argparser::option::no_argument, &options.warn, 0 },
        { "lower_left_maps_to_s0t0", argparser::option::no_argument, &options.lower_left_maps_to_s0t0, 1 },
        { "upper_left_maps_to_s0t0", argparser::option::no_argument, &options.lower_left_maps_to_s0t0, 0 },
        { "linear", argparser::option::no_argument, (int*)&options.oetf, KHR_DF_TRANSFER_LINEAR },
        { "srgb", argparser::option::no_argument, (int*)&options.oetf, KHR_DF_TRANSFER_SRGB },
        { "resize", argparser::option::required_argument, NULL, 'r' },
        { "scale", argparser::option::required_argument, NULL, 's' },
        { "t2", argparser::option::no_argument, &options.ktx2, 1},
    };

    const int lastOptionIndex = sizeof(my_option_list)
                                / sizeof(argparser::option);
    option_list.insert(option_list.begin(), my_option_list,
                       my_option_list + lastOptionIndex);
    short_opts += "f:F:w:d:a:l:r:s:";
}

toktxApp theApp;

// I really HATE this duplication of text but I cannot find a simple way to
// avoid it that works on all platforms (e.g running man toktx) even if I was
// willing to tolerate markup commands in the usage output.
void
toktxApp::usage()
{
    cerr <<
        "Usage: " << name << " [options] <outfile> [<infile>.{pam,pgm,ppm} ...]\n"
        "\n"
        "  <outfile>    The destination ktx file. \".ktx\" will appended if necessary.\n"
        "               If it is '-' the output will be written to stdout.\n"
        "  <infile>     One or more image files in .jpg, .png, .pam, .ppm, or .pgm\n"
        "               format. Other formats can be readily converted to these formats\n"
        "               using tools such as ImageMagick and XnView. When no infile is\n"
        "               specified, stdin is used. infiles prefixed with '@' are read as\n"
        "               text files listing actual file names to process with one file\n"
        "               path per line. Paths must be absolute or relative to the current\n"
        "               directory when toktx is run. If '@@' is used instead, paths must\n"
        "               be absolute or relative to the location of the list file.\n"
        "\n"
        "               .jpg files yield RED or RGB textures according to the actual\n"
        "               number of components. .png files yield RED, RG, RGB or RGBA\n"
        "               textures according to the files's color type, .ppm files RGB\n"
        "               textures, .pgm files RED textures and .pam files RED, RG, RGB\n"
        "               or RGBA textures according to the file's TUPLTYPE and DEPTH.\n"
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
        "               KTX file is for a mipmap pyramid with <number> of levels rather\n"
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
        "  --nowarn     Silence warnings which are issued when certain transformations\n"
        "               are performed on input images.\n"
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
        "               If this is pecified, implicit or explicit color space information\n"
        "               from the input file(s) will be ignored and no color\n"
        "               transformation will be performed. USE WITH CAUTION preferably\n"
        "               only when you know the file format information is wrong.\n"
        "  --srgb       Force the created texture to have an srgb transfer function.\n"
        "               Like --linear, USE WITH CAUTION. As with @b --linear, no color\n"
        "               transformation will be performed.\n"
        "  --resize <width>x<height>\n"
        "               Resize images to @e width X @e height. This should not be used\n"
        "               with @b--mipmap as it would resize all the images to the same\n"
        "               size. Resampler options can be set via --filter and --fscale.\n"
        "  --scale <value>\n"
        "               Scale images by <value> as they are read. Resampler options can\n"
        "               be set via --filter and --fscale.\n"
        "  --t2         Output in KTX2 format. Default is KTX.\n";
    scApp::usage();
    cerr << endl <<
        "Options can also be set in the environment variable TOKTX_OPTIONS.\n"
        "TOKTX_OPTIONS is parsed first. If conflicting options appear in TOKTX_OPTIONS\n"
        "or the command line, the last one seen wins. However if both --automipmap and\n"
        "--mipmap are seen, it is always flagged as an error. You can, for example,\n"
        "set TOKTX_OPTIONS=--lower_left_maps_to_s0t0 to change the default mapping of\n"
        "the logical image origin to match the GL convention.\n";
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
    return theApp.main(argc, argv);
}

int
toktxApp::main(int argc, _TCHAR *argv[])
{
    KTX_error_code ret;
    ktxTextureCreateInfo createInfo;
    ktxTexture* texture = 0;
    int exitCode = 0;
    unsigned int componentCount = 1, faceSlice, level, layer, levelCount = 1;
    unsigned int levelWidth, levelHeight, levelDepth;
    khr_df_transfer_e chosenOETF, firstImageOETF;
    khr_df_primaries_e chosenPrimaries, firstImagePrimaries;

    processEnvOptions();
    processCommandLine(argc, argv, eDisallowStdin, eFirst);
    validateOptions();

    if (options.cubemap)
      createInfo.numFaces = 6;
    else
      createInfo.numFaces = 1;

    // TO DO: handle array textures
    createInfo.numLayers = options.layers;
    createInfo.isArray = KTX_FALSE;

    // TO DO: handle 3D textures.

    faceSlice = layer = level = 0;
    std::vector<_tstring>::const_iterator it;
    uint32_t i;
    for (it = options.infiles.begin(), i = 0;
         it < options.infiles.end();
         it++, i++)
    {
        const _tstring& infile = *it;

        Image* image;
        try {
            image =
              Image::CreateFromFile(infile,
                                    options.oetf == KHR_DF_TRANSFER_UNSPECIFIED,
                                    options.bcmp);
        } catch (exception& e) {
            cerr << name << ": failed to create image from "
                      << infile << ". " << e.what() << endl;
            exit(2);
        }

        /* Sanity check. */
        assert(image->getWidth() * image->getHeight() * image->getPixelSize()
                  == image->getByteCount());

        if (i == 0) {
            // First file.
            firstImageOETF = image->getOetf();
            firstImagePrimaries = image->getPrimaries();
            chosenPrimaries = image->getPrimaries();
            if (options.oetf == KHR_DF_TRANSFER_UNSPECIFIED) {
                chosenOETF = firstImageOETF;
            } else {
                chosenOETF = options.oetf;
            }
        }

        if (options.scale != 1.0f || options.resize) {
            Image* scaledImage;
            if (options.scale != 1.0f) {
                scaledImage = image->createImage(
                              (uint32_t)(image->getWidth()  * options.scale),
                              (uint32_t)(image->getHeight()  * options.scale));

            } else {
                scaledImage = image->createImage(options.newGeom.width,
                                                 options.newGeom.height);
            }

            try {
                image->resample(*scaledImage,
                                chosenOETF == KHR_DF_TRANSFER_SRGB,
                                options.gmopts.filter.c_str(),
                                options.gmopts.filterScale,
                                basisu::Resampler::Boundary_Op::BOUNDARY_CLAMP);
            } catch (runtime_error e) {
                cerr << name << ": Image::resample() failed! "
                          << e.what() << endl;
                exitCode = 1;
                goto cleanup;
            }
            khr_df_transfer_e oetf = image->getOetf();
            delete image;
            image = scaledImage;
            image->setOetf(oetf);
        }

        if (image->getHeight() > 1 && options.lower_left_maps_to_s0t0) {
            image->yflip();
        }

        if (i == 0) {
            // First file.
            bool srgb;

            componentCount = image->getComponentCount();
            srgb = (chosenOETF == KHR_DF_TRANSFER_SRGB);
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
            if (image->getHeight() == 1 && !options.two_d)
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
                            cerr << name << "--levels value is greater than "
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
            if (requiredFileCount > options.infiles.size()) {
                cerr << name << ": too few files for " << levelCount
                     << " levels, " << createInfo.numLayers
                     << " layers and " << createInfo.numFaces
                     << " faces." << endl;
                exitCode = 1;
                goto cleanup;
            } else if (requiredFileCount < options.infiles.size()) {
                cerr << name << ": too many files for " << levelCount
                     << " levels, " << createInfo.numLayers
                     << " layers and " << createInfo.numFaces
                     << " faces. Extras will be ignored." << endl;
                options.infiles.erase(options.infiles.begin() + requiredFileCount,
                                          options.infiles.end());
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
                        name.c_str(), ktxErrorString(ret));
                exitCode = 2;
                goto cleanup;
            }
        } else {
            // Subsequent files.
            if (image->getOetf() != firstImageOETF) {
                fprintf(stderr, "%s: \"%s\" is encoded with a different transfer"
                                " function (OETF) than preceding files.\n",
                                name.c_str(), infile.c_str());
                exitCode = 1;
                goto cleanup;
            }
            if (image->getPrimaries() != firstImagePrimaries) {
                fprintf(stderr, "%s: \"%s\" has different color primaries than"
                                " preceding files.\n",
                                name.c_str(), infile.c_str());
                exitCode = 1;
                goto cleanup;
            }            // Input file order is layer, faceSlice, level. This seems easier for
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
            cerr << name << ": \"" << infile << "\" intended for a cubemap "
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
                                    chosenOETF == KHR_DF_TRANSFER_SRGB,
                                    options.gmopts.filter.c_str(),
                                    options.gmopts.filterScale,
                                    options.gmopts.wrapMode);
                } catch (runtime_error e) {
                    cerr << name << ": Image::resample() failed! "
                              << e.what() << endl;
                    exitCode = 1;
                    goto cleanup;
                }

                // TODO: add an option for renormalize;
                //if (options.gmopts.mipRenormalize)
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
        writeId(writer, options.test != 0);
        ktxHashList_AddKVPair(&texture->kvDataHead, KTX_WRITER_KEY,
                              (ktx_uint32_t)writer.str().length() + 1,
                              writer.str().c_str());
    }
    if (options.ktx2 && chosenPrimaries != KHR_DF_PRIMARIES_BT709) {
        KHR_DFDSETVAL(((ktxTexture2*)texture)->pDfd + 1, PRIMARIES,
                      chosenPrimaries);
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
       if (options.bcmp || options.bopts.uastc) {
            commandOptions::basisOptions& bopts = options.bopts;
            if (bopts.normalMap && chosenOETF != KHR_DF_TRANSFER_LINEAR) {
                fprintf(stderr, "%s: --normal_map specified but input file(s) are"
                        " not linear.", name.c_str());
                exitCode = 1;
                goto cleanup;
            }
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
                fprintf(stderr, "%s failed to compress KTX file \"%s\"; KTX error: %s\n",
                        name.c_str(), options.outfile.c_str(),
                        ktxErrorString(ret));
                exitCode = 2;
                goto cleanup;
            }
        } else {
            ret = KTX_SUCCESS;
        }
        if (KTX_SUCCESS == ret) {
            if (options.zcmp) {
                ret = ktxTexture2_DeflateZstd((ktxTexture2*)texture,
                                               options.zcmpLevel);
                if (KTX_SUCCESS != ret) {
                    cerr << name << ": Zstd deflation failed; KTX error: "
                         << ktxErrorString(ret) << endl;
                    exitCode = 2;
                    goto cleanup;
                }
            }
            if (!getParamsStr().empty()) {
                ktxHashList_AddKVPair(&texture->kvDataHead, scparamKey.c_str(),
                (ktx_uint32_t)getParamsStr().length(), getParamsStr().c_str());
            }
            ret = ktxTexture_WriteToStdioStream(ktxTexture(texture), f);
            if (KTX_SUCCESS != ret) {
                fprintf(stderr, "%s failed to write KTX file \"%s\"; KTX error: %s\n",
                        name.c_str(), options.outfile.c_str(),
                        ktxErrorString(ret));
                exitCode = 2;
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
                name.c_str(), options.outfile.c_str(), strerror(errno));
        exitCode = 2;
    }

cleanup:
    if (texture) ktxTexture_Destroy(ktxTexture(texture));
    return exitCode;
}


void
toktxApp::validateOptions()
{
    scApp::validateOptions();

    if (options.automipmap + options.genmipmap + options.mipmap > 1) {
        error("only one of --automipmap, --genmipmap and "
              "--mipmap may be specified.");
        usage();
        exit(1);
    }
    if ((options.automipmap || options.genmipmap) && options.levels > 1) {
        error("cannot specify --levels > 1 with --automipmap or --genmipmap.");
        usage();
        exit(1);
    }
    if (options.cubemap && options.lower_left_maps_to_s0t0) {
        error("cubemaps require images to have an upper-left origin. "
              "Ignoring --lower_left_maps_to_s0t0.");
        options.lower_left_maps_to_s0t0 = 0;
    }
    if (options.cubemap && options.depth > 1) {
        error("cubemaps cannot have depth > 1.");
        usage();
        exit(1);
    }
    if (options.layers > 1 && options.depth > 1) {
        error("cannot have 3D array textures.");
        usage();
        exit(1);
    }
    if (options.scale != 1.0 && options.resize) {
        error("only one of --scale and --resize can be specified.");
        usage();
        exit(1);
    }
    if (options.resize && options.mipmap) {
        error("only one of --resize and --mipmap can be specified.");
        usage();
        exit(1);
    }

    if (options.outfile.compare(_T("-")) != 0
            && options.outfile.find_last_of('.') == _tstring::npos)
    {
        options.outfile.append(options.ktx2 ? _T(".ktx2") : _T(".ktx"));
    }

    ktx_uint32_t requiredInputFiles = options.cubemap ? 6 : 1 * options.levels;
    if (requiredInputFiles > options.infiles.size()) {
        error("too few input files.");
        exit(1);
    }
    /* Whether there are enough input files for all the mipmap levels in
     * a full pyramid can only be checked when the first file has been
     * read and the size determined.
     */
}

void
toktxApp::processEnvOptions() {
    _tstring toktx_options;
    _TCHAR* env_options = _tgetenv(_T("TOKTX_OPTIONS"));

    if (env_options != nullptr)
        toktx_options = env_options;
    else
        return;

    if (!toktx_options.empty()) {
        istringstream iss(toktx_options);
        argvector arglist;
        for (_tstring w; iss >> w; )
            arglist.push_back(w);

        argparser optparser(arglist, 0);
        processOptions(optparser);
        if (optparser.optind != arglist.size()) {
            cerr << "Only options are allowed in the TOKTX_OPTIONS "
                 << "environment variable." << endl;
            usage();
            exit(1);
        }
    }
}

/*
 * @brief process a command line option
 *
 * @return
 *
 * @param[in]     parser,     an @c argparser holding the options to process.
 */
bool
toktxApp::processOption(argparser& parser, int opt)
{
    switch (opt) {
      case 0:
        break;
      case 'a':
        options.layers = strtoi(parser.optarg.c_str());
        break;
      case 'd':
        options.depth = strtoi(parser.optarg.c_str());
        break;
      case 'l':
        options.levels = strtoi(parser.optarg.c_str());
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
            usage();
            exit(1);
        }
        break;
      case 'r':
        {
            istringstream iss(parser.optarg);
            char x;
            iss >> options.newGeom.width >> x >> options.newGeom.height;
            if (iss.fail()) {
                cerr << "Bad resize geometry." << endl;
                usage();
                exit(1);
            }
            options.resize = 1;
            break;
        }
      case 's':
        options.scale = strtof(parser.optarg.c_str(), nullptr);
        if (options.scale > 2000.0f) {
            cerr << name << ": Unreasonable scale factor of "
                 << options.scale << "." << endl;
            exit(1);
        }
        break;
      case ':':
      default:
        return scApp::processOption(parser, opt);
    }
    return true;
}

void toktxApp::warning(const char *pFmt, va_list args) {
    if (options.warn) {
        cerr << name << " warning: ";
        vfprintf(stderr, pFmt, args);
        cerr << endl;
    }
}

void warning(const char *pFmt, ...) {
        va_list args;
        va_start(args, pFmt);

        theApp.warning(pFmt, args);
        va_end(args);
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

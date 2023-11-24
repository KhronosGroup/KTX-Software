// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 sts=4 expandtab:

// Copyright 2010-2020 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

// To use, download from http://www.billbaxter.com/projects/imdebug/
// Put imdebug.dll in %SYSTEMROOT% (usually C:\WINDOWS), imdebug.h in
// ../../include, imdebug.lib in ../../build/msvs/<platform>/vs<ver> &
// add ..\imdebug.lib to the libraries list in the project properties.
#define IMAGE_DEBUG 0

#include "scapp.h"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <sstream>
#include <vector>
#include <inttypes.h>
#include <zstd.h>

#include "GL/glcorearb.h"
#include "ktx.h"
#include "../../lib/vkformat_enum.h"
#include "argparser.h"
#include "version.h"
#include "image.hpp"
#include "imageio.h"
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
static void dumpImage(char* name, int width, int height, int components,
                      int componentSize, unsigned char* srcImage);
#endif

using namespace std;

/** @page toktx toktx
@~English

Create a KTX file from JPEG, PNG or netpbm format files.

@section toktx_synopsis SYNOPSIS
    toktx [options] @e outfile [@e infile.{jpg,png,pam,pgm,ppm} ...]

@section toktx_description DESCRIPTION
    Create a Khronos format texture file (KTX) from a set of JPEG (.jpg),
    PNG (.png) or Netpbm format (.pam, .pgm, .ppm) images. It writes the
    destination ktx file to @e outfile, creating parent directories and
    appending ".ktx{,2}" if necessary. If @e outfile is '-' the output will
    be written to stdout.

    @b toktx reads each named @e infile. which must be in .jpg, .png, .pam,
    .ppm or .pgm format. @e infiles prefixed with '@' are read as text files
    listing actual file names to process with one file path per line. Paths
    must be absolute or relative to the current directory when @b toktx is run.
    If '\@@' is used instead, paths must be absolute or relative to the location
    of the list file. File paths must be encoded in UTF-8.

    The target texture type (number of components in the output texture) is
    chosen via @b --target_type. Swizzling of the components of the input
    file is specified with @b --input_swizzle and swizzzle metadata can be
    specified with @b --swizzle. Defaults, shown in the following table, are
    based on the components of the input file and whether the target texture
    format is uncompressed or block-compressed including the universal
    formats. Input components are arbitrarily labeled r, g, b & a.

    |   | Uncompressed Formats |||| Block-compressed formats ||||
    | --------------------- | :-: | :-: | :-: | :-:  | :-: | :-: | :-: | :-: |
    | Input components |  1  (greyscale)  |  2  (greyscale alpha) |  3  |  4  |  1  |  2  |  3  |  4  |
    | Target type | R | RG | RGB | RGBA | RGB | RGBA | RGB | RGBA |
    | Input swizzle | - | - | - | - | rrr1 | rrrg | - | - |
    | Swizzle | rrr1 | rrrg | - | - | - | - | - | - |

    As can be seen from the table one- and two-component inputs are treated
    as luminance{,-alpha} in accordance with the JPEG and PNG specifications.
    For consistency Netpbm inputs are handled the same way. Use of R & RG
    types for uncompressed formats saves space but note that the sRGB versions
    of these formats are not widely supported so a warning will be issued
    prompting you to convert the input to linear.

    The primaries, transfer function (OETF) and the texture's sRGB-ness is set
    based on the input file unless @b --assign_oetf linear or @b --assign_oetf
    srgb is specified. For .jpg files @b toktx always sets BT709/sRGB primaries
    and the sRGB OETF in the output file and creates sRGB format textures.
    Netpbm files always use BT.709/sRGB primaries and the BT.709 OETF. @b toktx
    tranforms these images to the sRGB OETF, sets BT709/sRGB primaries and the
    sRGB OETF in the output file and creates sRGB format textures.

    For .png files the OETF is set as follows:

    <dl>
    <dt>No color-info chunks or sRGB chunk present:</dt>
        <dd>primaries are set to BT.709 and OETF to sRGB.</dd>
    <dt>sRGB chunk present:</dt>
        <dd>primaries are set to BT.709 and OETF to sRGB. gAMA and cHRM chunks
        are ignored.</dd>
    <dt>iCCP chunk present:</dt>
        <dd>General ICC profiles are not yet supported by toktx or the KTX2
        format. In future these images may be transformed to linear or sRGB
        OETF as appropriate for the profile. sRGB chunk must not be present.
        </dd>
    <dt>gAMA and/or cHRM chunks present without sRGB or iCCP:</dt>
        <dd>If gAMA is < 60000 the image is transformed to and the OETF is set
        to sRGB. otherwise the image is transformed to and the OETF is set to
        linear. The color primaries in cHRM are matched to one of the
        standard sets listed in the Khronos Data Format Specification (the
        KHR_DF_PRIMARIES values from khr_df.h) and the primaries
        field of the output file's DFD is set to the matched value. If no match
        is found the primaries field is set to UNSPECIFIED.</dd>
    </dl>

    The following options are always available:
    <dl>
    <dt>\--2d</dt>
    <dd>If the image height is 1, by default a KTX file for a 1D texture is
        created. With this option one for a 2D texture is created instead.</dd>
    <dt>\--automipmap</dt>
    <dd>Causes the KTX file to be marked to request generation of a mipmap
        pyramid when the file is loaded. This option is mutually exclusive
        with @b --genmipmap, @b --levels and @b --mipmap.</dd>
    <dt>\--cubemap</dt>
    <dd>KTX file is for a cubemap. At least 6 @e infiles must be provided,
        more if @b --mipmap or @b --layers is also specified. Provide the
        images in the order +X, -X, +Y, -Y, +Z, -Z where the arrangement is a
        left-handed coordinate system with +Y up. So if you're facing +Z,
        -X will be on your left and +X on your right. If @b --layers &gt; 1 is
        specified, provide the faces for layer 0 first then for layer 1, etc.
        Images must have an upper left origin so --lower_left_maps_to_s0t0
        is ignored with this option.</dd>
    <dt>\--depth &lt;number&gt;</dt>
    <dd>KTX file is for a 3D texture with a depth of @e number where
        @e number &gt; 0. Provide the file(s) for z=0 first then those for
        z=1, etc. It is an error to specify this together with
        @b --layers or @b --cubemap.</dd>
    <dt>\--genmipmap</dt>
    <dd>Causes mipmaps to be generated for each input file. This option is
        mutually exclusive with @b --automipmap and @b --mipmap. When set,
        the following mipmap-generation related options become valid,
        otherwise they are ignored.
        <dl>
        <dt>\--filter &lt;name&gt;</dt>
        <dd>Specifies the filter to use when generating the mipmaps. @e name
            is a string. The default is @e lanczos4. The following names are
            recognized: @e box, @e tent, @e bell, @e b-spline, @e mitchell,
            @e lanczos3, @e lanczos4, @e lanczos6, @e lanczos12, @e blackman,
            @e kaiser, @e gaussian, @e catmullrom, @e quadratic_interp,
            @e quadratic_approx and @e quadratic_mix.</dd>
        <dt>\--fscale &lt;floatVal&gt;</dt>
        <dd>The filter scale to use. The default is 1.0.</dd>
        <dt>\--wmode &lt;mode&gt;</dt>
        <dd>Specify how to sample pixels near the image boundaries. Values
            are @e wrap, @e reflect and @e clamp. The default is @e clamp.</dd>
        </dl>
    </dd>
    <dt>\--layers &lt;number&gt;</dt>
    <dd>KTX file is for an array texture with @e number of layers where
        @e number &gt; 0. Provide the file(s) for layer 0 first then those
        for layer 1, etc. It is an error to specify this together with
        @b --depth.</dd>
    <dt>\--levels &lt;number&gt;</dt>
    <dd>KTX file is for a mipmap pyramid with @e number of levels rather than
        a full pyramid. @e number must be &gt; 1 and  &lt;= the maximum number
        of levels determined from the size of the base level image. Provide the
        base level image first, if using @b --mipmap. This option is mutually
        exclusive with @b --automipmap.</dd>
    <dt>\--mipmap</dt>
    <dd>KTX file is for a mipmap pyramid with one @b infile being explicitly
        provided for each level. Provide the images in the order of layer
        then face or depth slice then level with the base-level image first
        then in order down to the 1x1 image or the level specified by
        @b --levels.  @note This ordering differs from that in the
        created texture as it is felt to be more user-friendly.

        This option is mutually exclusive with @b --automipmap and
        @b --genmipmap.</dd>
    <dt>\--nometadata</dt>
    <dd>Do not write KTXorientation metadata into the output file. Metadata
        is written by default. Use of this option is not recommended.</dd>
    <dt>\--nowarn</dt>
    <dd>Silence warnings which are issued when certain transformations are
        performed on input images.</dd>
    <dt>\--upper_left_maps_to_s0t0</dt>
    <dd>Map the logical upper left corner of the image to s0,t0.
        Although opposite to the OpenGL convention, this is the DEFAULT
        BEHAVIOUR. netpbm and PNG files have an upper left origin so this
        option does not flip the input images. When this option is in effect,
        toktx writes a KTXorientation value of S=r,T=d into the output file
        to inform loaders of the logical orientation. If an OpenGL {,ES}
        loader ignores the orientation value, the image will appear upside
        down.</dd>
    <dt>\--lower_left_maps_to_s0t0</dt>
    <dd>Map the logical lower left corner of the image to s0,t0.
        This causes the input netpbm and PNG images to be flipped vertically
        to a lower-left origin. When this option is in effect, toktx
        writes a KTXorientation value of S=r,T=u into the output file
        to inform loaders of the logical orientation. If a Vulkan loader
        ignores the orientation value, the image will appear upside down.
        This option is ignored with @b --cubemap. </dd>
    <dt>\--assign_oetf &lt;linear|srgb&gt;</dt>
    <dd>Force the created texture to have the specified transfer function. If
        this is specified, implicit or explicit color space information from the
        input file(s) will be ignored and no color transformation will be
        performed. USE WITH CAUTION preferably only when you know the file
        format information is wrong.</dd>
    <dt>\--assign_primaries &lt;bt709|none|srgb&gt;</dt>
    <dd>Force the created texture to have the specified primaries. If this is
        specified, implicit or explicit color space information from the input
        file(s) will be ignored and no color transformation will be performed.
        USE WITH CAUTION preferably only when you know the file format
        information is wrong.</dd>
    <dt>\--convert_oetf &lt;linear|srgb&gt;</dt>
    <dd>Convert the input images to the specified transfer function, if the
        current transfer function is different. If both this and
        @b --assign_oetf are specified, conversion will be performed from the
        assigned transfer function to the transfer function specified by this
        option, if different.
    <dt>\--convert_primaries &lt;primaries&gt;</dt>
    <dd>Convert the image images to the specified color primaries, if
        different from the color primaries of the input file(s) or the one
        specified by --assign-primaries. If both this and --assign-primaries
        are specified, conversion will be performed from the assigned primaries
        to the primaries specified by this option, if different. This option is
        not allowed to be specified when --assign-primaries is set to 'none'.
        Case insensitive.
        Possible options are:
        bt709 | srgb | bt601-ebu | bt601-smpte | bt2020 | ciexyz | aces |
        acescc | ntsc1953 | pal525 | displayp3 | adobergb</dd>
    <dt>\--linear</dt>
    <dd>Deprecated. Use @b --assign_oetf linear.</dd>
    <dt>\--srgb</dt>
    <dd>Deprecated. Use @b --assign_oetf srgb.</dd>
    <dt>\--resize &lt;width&gt;x&lt;height&gt;
    <dd>Resize images to @e width X @e height. This should not be used with
        @b --mipmap as it would resize all the images to the same size.
        Resampler options can be set via @b --filter and  @b --fscale. </dd>
    <dt>\--scale &lt;value&gt;</dt>
    <dd>Scale images by @e value as they are read. Resampler options can
        be set via @b --filter and  @b --fscale. </dd>.
    <dt>\--swizzle &lt;swizzle&gt;
    <dd>Add swizzle metadata to the file being created. @e swizzle
        has the same syntax as the parameter for @b --input_swizzle.
        Not recommended for use with block-cmpressed textures, including
        Basis Universal formats, because something like @c rabb may
        yield drastically different error metrics if done after compression.
    <dt>\--target_type &lt;type&gt;
    <dd>Specify the number of components in the created texture. @e type
        is one of the following strings: @c R, @c RG, @c RGB or @c RGBA.
        Excess input components will be dropped. Output components with
        no mapping from the input will be set to 0 or, if the alpha component,
        1.0.
    <dt>\--t2</dt>
    <dd>Output in KTX2 format. Default is KTX.</dd>
    </dl>
    @snippet{doc} scapp.h scApp options

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
    virtual ~toktxApp() { };

    virtual int main(int argc, char* argv[]);
    virtual void usage();

    friend void warning(const char *pFmt, va_list args);
    friend void warning(const char *pFmt, ...);
    friend void warning(const string& msg);

  protected:
    struct targetImageSpec : public ImageSpec {
        khr_df_transfer_e usedInputTransferFunction;
        khr_df_primaries_e usedInputPrimaries;
        std::unique_ptr<const TransferFunction> srcTransferFunction{};
        std::unique_ptr<const TransferFunction> dstTransferFunction{};
        std::unique_ptr<const ColorPrimaries> srcColorPrimaries{};
        std::unique_ptr<const ColorPrimaries> dstColorPrimaries{};
        targetImageSpec& operator=(const ImageSpec& s) {
            *static_cast<ImageSpec*>(this) = s;
            return *this;
        }
    };

    virtual bool processOption(argparser& parser, int opt);
    void processEnvOptions();
    void validateOptions();
    khr_df_primaries_e parseColorPrimaries(string& argValue);

    unique_ptr<Image> createImage(const targetImageSpec& target, ImageInput& in);
    unique_ptr<Image> convertImageType(unique_ptr<Image> pImage);
    unique_ptr<Image> scaleImage(unique_ptr<Image> pImage,
                                 ktx_uint32_t width, ktx_uint32_t height);
    void genMipmap(unique_ptr<Image> pImage,
                   uint32_t layer, uint32_t faceSlice,
                   ktxTexture* texture);

    ktxTexture* createTexture(const targetImageSpec& target);

    void determineTargetColorSpace(const ImageInput& in,
                                   targetImageSpec& target);
    void determineTargetTypeBitLengthScale(const ImageInput& in,
                                           targetImageSpec& target,
                                           string& defaultSwizzle);

    void determineTargetImageSpec(const ImageInput& in,
                                  targetImageSpec& target,
                                  string& defaultSwizzle);
    void checkSpecsMatch(const ImageInput& current, const ImageSpec& firstSpec);

    void setAstcMode(const targetImageSpec& target);

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
        khr_df_transfer_e assign_oetf;
        khr_df_transfer_e convert_oetf;
        khr_df_primaries_e assign_primaries;
        khr_df_primaries_e convert_primaries;
        int          useStdin;
        int          lower_left_maps_to_s0t0;
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
        string swizzle;
        enum {
            // These values are selected to match the number of components.
            eUnspecified=0, eR=1, eRG, eRGB, eRGBA
        } targetType;

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
            depth = 0;
            layers = 0;
            levels = 1;
            convert_oetf = KHR_DF_TRANSFER_UNSPECIFIED;
            assign_oetf = KHR_DF_TRANSFER_UNSPECIFIED;
            assign_primaries = KHR_DF_PRIMARIES_MAX;
            convert_primaries = KHR_DF_PRIMARIES_MAX;
            // As required by spec. Opposite of OpenGL {,ES}, same as
            // Vulkan, et al.
            lower_left_maps_to_s0t0 = 0;
            scale = 1.0f;
            resize = 0;
            newGeom.width = newGeom.height = 0;
            targetType = eUnspecified;
        }
    } options;

    class cant_create_image : public runtime_error {
        using runtime_error::runtime_error;
    };

    class cant_create_texture : public runtime_error {
        using runtime_error::runtime_error;
    };
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
        { "linear", argparser::option::no_argument, (int*)&options.assign_oetf, KHR_DF_TRANSFER_LINEAR },
        { "srgb", argparser::option::no_argument, (int*)&options.assign_oetf, KHR_DF_TRANSFER_SRGB },
        { "resize", argparser::option::required_argument, NULL, 'r' },
        { "scale", argparser::option::required_argument, NULL, 's' },
        { "swizzle", argparser::option::required_argument, NULL, 1101},
        { "target_type", argparser::option::required_argument, NULL, 1102},
        { "assign_oetf", argparser::option::required_argument, NULL, 1103},
        { "convert_oetf", argparser::option::required_argument, NULL, 1104},
        { "assign_primaries", argparser::option::required_argument, NULL, 1105},
        { "convert_primaries", argparser::option::required_argument, NULL, 1106},
        { "t2", argparser::option::no_argument, &options.ktx2, 1},
    };

    const int lastOptionIndex = sizeof(my_option_list)
                                / sizeof(argparser::option);
    option_list.insert(option_list.begin(), my_option_list,
                       my_option_list + lastOptionIndex);
    short_opts += "f:F:w:d:a:l:r:s:";
}

static toktxApp toktx;
ktxApp& theApp = toktx;

// I really HATE this duplication of text but I cannot find a simple way to
// avoid it that works on all platforms (e.g running man toktx) even if I was
// willing to tolerate markup commands in the usage output.
void
toktxApp::usage()
{
    cerr <<
        "Usage: " << name << " [options] <outfile> [<infile>.{jpg,png,pam,pgm,ppm} ...]\n"
        "\n"
        "  <outfile>    The destination ktx file. Parent directories will be created\n"
        "               and \".ktx\" will appended if necessary. If it is '-' the\n"
        "               output will be written to stdout.\n"
        "  <infile>     One or more image files in .jpg, .png, .pam, .ppm, or .pgm\n"
        "               format. Other formats can be readily converted to these formats\n"
        "               using tools such as ImageMagick and XnView. infiles prefixed\n"
        "               with '@' are read as text files listing actual file names to\n"
        "               process with one file path per line. Paths must be absolute or\n"
        "               relative to the current directory when toktx is run. If '@@'\n"
        "               is used instead, paths must be absolute or relative to the\n"
        "               location of the list file. File paths must be encoded in UTF-8.\n"
        "\n"
        "  The target texture type (number of components in the output texture) is chosen\n"
        "  via --target_type. Swizzling of the components of the input file is specified\n"
        "  with --input_swizzle and swizzle metadata can be specified with --swizzle\n"
        "  Defaults, shown in the following tables, are based on the components of the\n"
        "  input file and whether the target texture format is uncompressed or\n"
        "  block-compressed including the universal formats. Input components are\n"
        "  arbitrarily labeled r, g, b & a.\n"
        "\n"
        "  |                      Uncompressed Formats                           |\n"
        "  |---------------------------------------------------------------------|\n"
        "  | Input components | 1 (greyscale) | 2 (greyscale alpha) |  3  |  4   |\n"
        "  | Target type      |       R       |         RG          | RGB | RGBA |\n"
        "  | Input swizzle    |       -       |         -           |  -  |  -   |\n"
        "  | Swizzle          |     rrr1      |        rrrg         |  -  |  -   |\n"
        "\n"
        "  |                      Block-compressed formats                       |\n"
        "  |---------------------------------------------------------------------|\n"
        "  | Target type      |      RGB      |        RGBA         | RGB | RGBA |\n"
        "  | Input swizzle    |      rrr1     |        rrrg         |  -  |  -   |\n"
        "  | Swizzle          |       -       |         -           |  -  |  -   |\n"
        "\n"
        "  As can be seen from the table one- and two-component inputs are treated as\n"
        "  luminance{,-alpha} in accordance with the JPEG and PNG specifications. For\n"
        "  consistency Netpbm inputs are handled the same way. Use of R & RG types for\n"
        "  uncompressed formats saves space but note that the sRGB versions of these\n"
        "  formats are not widely supported so a warning will be issued prompting you\n"
        "  to convert the input to linear.\n"
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
        "               number > 0. Provide the file(s) for z=0 first then those for\n"
        "               z=1, etc. It is an error to specify this together with\n"
        "               --layers or --cubemap.\n"
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
        "               where number > 0. Provide the file(s) for layer 0 first then\n"
        "               those for layer 1, etc. It is an error to specify this\n"
        "               together with --depth.\n"
        "  --levels <number>\n"
        "               KTX file is for a mipmap pyramid with <number> of levels rather\n"
        "               than a full pyramid. number must be > 1 and <= the maximum number\n"
        "               of levels determined from the size of the base image. This option\n"
        "               is mutually exclusive with @b --automipmap.\n"
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
        "  --assign_oetf <linear|srgb>\n"
        "               Force the created texture to have the specified transfer\n"
        "               function. If this is specified, implicit or explicit color space\n"
        "               information from the input file(s) will be ignored and no color\n"
        "               transformation will be performed. USE WITH CAUTION preferably\n"
        "               only when you know the file format information is wrong.\n"
        "  --assign_primaries <bt709|none|srgb>\n"
        "               Force the created texture to have the specified primaries. If\n"
        "               this is specified, implicit or explicit color space information\n"
        "               from the input file(s) will be ignored and no color\n"
        "               transformation will be performed. USE WITH CAUTION preferably\n"
        "               only when you know the file format information is wrong.\n"
        "  --convert_oetf <linear|srgb>\n"
        "               Convert the input images to the specified transfer function, if\n"
        "               the current transfer function is different. If both this and\n"
        "               --assign_oetf are specified, conversion will be performed from\n"
        "               the assigned transfer function to the transfer function specified\n"
        "               by this option, if different.\n"
        "  --convert_primaries <primaries> \n"
        "               Convert the image image(s) to the specified color primaries,\n"
        "               if different from the color primaries of the input file(s) or the\n"
        "               one specified by --assign-primaries. If both this and\n"
        "               --assign-primaries are specified, conversion will be performed\n"
        "               from the assigned primaries to the primaries specified by this\n"
        "               option, if different. This option is not allowed to be specified\n"
        "               when --assign-primaries is set to 'none'. Case insensitive.\n"
        "               Possible options are: bt709 | srgb | bt601-ebu | bt601-smpte |\n"
        "               bt2020 | ciexyz | aces | acescc | ntsc1953 | pal525 |\n"
        "               displayp3 | adobergb.\n"
        "  --linear     Deprecated. Use --assign_oetf linear.\n"
        "  --srgb       Deprecated. Use --assign_oetf srgb.\n"
        "  --swizzle <swizzle>\n"
        "               Add swizzle metadata to the file being created. swizzle has the\n"
        "               same syntax as the parameter for --input_swizzle. Not recommended\n"
        "               for use with block-cmpressed textures, including Basis Universal\n"
        "               formats, because something like `rabb` may yield drastically\n"
        "               different error metrics if done after compression.\n"
        "  --target_type <type>\n"
        "               Specify the number of components in the created texture. type is\n"
        "               one of the following strings: @c R, @c RG, @c RGB or @c RGBA.\n"
        "               Excess input components will be dropped. Output components with\n"
        "               no mapping from the input will be set to 0 or, if the alpha\n"
        "               component, 1.0.\n"
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

int
toktxApp::main(int argc, char *argv[])
{
    KTX_error_code ret;
    ktxTexture* texture = 0;
    int exitCode = 0;
    unsigned int faceSlice, level, layer, levelCount = 1;
    unsigned int levelWidth=0, levelHeight=0, levelDepth=0;
    string defaultSwizzle;

    processEnvOptions();
    processCommandLine(argc, argv, eAllowStdin, eFirst);
    validateOptions();

    faceSlice = layer = level = 0;
    vector<string>::const_iterator it;
    bool firstImage = true;
    ImageSpec firstImageSpec;
    targetImageSpec target;

    for (it = options.infiles.begin(); it < options.infiles.end(); it++) {
        const string& infile = *it;
        unique_ptr<Image> image;
        uint32_t subimage=0, miplevel=0;

        try {
            ImageSpec config;
            auto in = ImageInput::open(infile, &config,
                          // This lambda provides the trampoline to the
                          // warning method.
                          [this](const std::string& w) { this->warning(w); });

            // Input file order is layer, faceSlice, level. This seems easier
            // for a human to manage than the order in a KTX file. It keeps the
            // base level images and their mip levels together. It also works
            // better with subimages and miplevels.

            // TODO: figure out how to handle 3d input files. slice,level
            // order does not work. Such files will have slices(level0),
            // slices(level1) ...
            if (in->spec().depth() > 1) {
                stringstream message;
                throw cant_create_image(
                    "Input of volumetric images is not (yet) supported."
                );
            }

            do { // subimages
                do {  // miplevels
                    if (in->miplevelCount() > 1 && !options.mipmap) {
                         warning("Ignoring miplevels in %s(%d) because --mipmap not set.",
                                 infile.c_str(), subimage);
                    }
                    in->seekSubimage(subimage, miplevel);
                    const ImageSpec& spec = in->spec();
                    if (!spec.format().sameUnitAllChannels()) {
                        throw cant_create_image(
                            "Components of differing size or type not yet supported.");
                    }
                    if (firstImage) {
                        if (options.cubemap && spec.width() != spec.height())
                        {
                            throw cant_create_image(
                              "--cubemap specified but image is not square."
                            );
                        }
                        firstImageSpec = spec;
                        determineTargetImageSpec(*in, target, defaultSwizzle);
                        texture = createTexture(target);
                        levelWidth = texture->baseWidth;
                        levelHeight = texture->baseHeight;
                        levelDepth = texture->baseDepth;
                        // Figure out how many levels we'll read from files.
                        if (options.mipmap) {
                            levelCount = texture->numLevels;
                        } else {
                            // texture->numLevels will be > 1 for --genmipmap.
                            levelCount = 1;
                        }
                        if (options.astc) {
                            setAstcMode(target);
                        }
                        firstImage = false;
                    } else {
                        // Subsequent images
                        checkSpecsMatch(*in, firstImageSpec);
                        // If cubemap we must have passed square test to get
                        // here so expected sizes will be square so it is
                        // sufficient to test against expected sizes.
                    }
                    image = createImage(target, *in);
                    // Because of potential scale or resize, this test is best
                    // done after the image has been created as they will have
                    // been applied.
                    if (!(image->getWidth() == levelWidth
                        && image->getHeight() == levelHeight))
                        // TODO: Figure out 3d input images.
                    {
                        throw cant_create_image(
                            "Image has incorrect size for next layer, face or mip level."
                        );
                    }

                    //Check astcopts.mode here?

                    if (image->getHeight() > 1 && options.lower_left_maps_to_s0t0) {
                        image->yflip();
                    }
                    if (options.normalize) {
                        image->normalize();
                    }
                    if (options.inputSwizzle.size() > 0
                        // inputSwizzle is handled during BasisU and astc encoding
                        && !options.etc1s && !options.bopts.uastc && !options.astc) {
                        image->swizzle(options.inputSwizzle);
                    }

                    ret = ktxTexture_SetImageFromMemory(texture,
                                                        level,
                                                        layer,
                                                        faceSlice,
                                                        *image,
                                                        image->getByteCount());
                    // Only an error in this program could lead to
                    // ret != SUCCESS hence no user message.
                    assert(ret == KTX_SUCCESS);

                    if (options.genmipmap) {
                        genMipmap(std::move(image), layer, faceSlice, texture);
                    }
#if IMAGE_DEBUG
                    {
                        ktx_size_t offset;
                        ktxTexture_GetImageOffset(texture, level, 0, faceSlice,
                                                  &offset);
                        dumpImage(infile, image->getWidth(), image->getHeight(),
                                  image->getComponentCount(),
                                  image->getComponentSize(),
                                  texture.pData + offset);
                    }
#endif
                    image = nullptr;

                    miplevel++;
                    level++;
                    levelWidth = maximum(levelWidth >> 1, 1U);
                    levelHeight = maximum(levelHeight >> 1, 1U);
                    levelDepth = maximum(levelDepth >> 1, 1U);
                } while (miplevel < levelCount && miplevel < in->miplevelCount());
                subimage++;
                if (level == levelCount) {
                    faceSlice++;
                    level = 0;
                    levelWidth = texture->baseWidth;
                    levelHeight = texture->baseHeight;
                    levelDepth = texture->baseDepth;
                    if (faceSlice == (options.cubemap ? 6 : levelDepth)) {
                        faceSlice = 0;
                        layer++;
                        if (layer == texture->numLayers) {
                            // We're done.
                            break;
                        }
                    }
                }
            } while (subimage < in->subimageCount());
            if (subimage < in->subimageCount()) {
                ; // warn unused
            }
            if (layer == texture->numLayers) {
                // We're done.
                break;
            }
        } catch (cant_create_texture& e) {
            cerr << name << ": failed to create ktxTexture. "
                 << e.what() << endl;
            exit(2);
        } catch (cant_create_image& e) {
            cerr << name << ": could not create image from "
                 << infile << "(" << subimage << "," << miplevel
                 << ")." << endl << e.what() << endl;
            // Some of these exceptions are thrown after the image has
            // been created despite its name. We want the same message
            // to the user hence not creating a different exception.
            if (image != nullptr)
                image = nullptr;
            exitCode = 1;
            goto cleanup;
        } catch (runtime_error& e) {
            cerr << name << ": failed to create image from "
                 << infile << "(" << subimage << "," << miplevel
                 << ")." << endl << e.what() << endl;
            exitCode = 2;
            goto cleanup;
        }
    }
    if (layer != texture->numLayers) {
        cerr << name << ": too few input images for " << levelCount
             << " levels, " << texture->numLayers
             << " layers and " << texture->numFaces
             << " faces." << endl;
        exitCode = 1;
        goto cleanup;
    }
    // We break out of the loop when done so final iterator increment
    // never happens hence -1 here.
    if (it != options.infiles.end() - 1) {
        warning("Ignoring excess input images.");
    }

    /*
     * Add orientation metadata.
     */
    if (options.metadata) {
        ktxHashList* ht = &texture->kvDataHead;
        char orientation[20];
        if (options.ktx2) {
            orientation[0] = 'r';
            if (texture->numDimensions > 1) {
                orientation[1] = options.lower_left_maps_to_s0t0 ? 'u' : 'd';
                if (texture->numDimensions > 2) {
                    orientation[2] = options.lower_left_maps_to_s0t0
                                   ? 'o'  : 'i';
                    orientation[3] = 0;
                } else {
                    orientation[2] = 0;
                }
            } else {
                orientation[1] = 0;
            }
        } else {
            assert(strlen(KTX_ORIENTATION3_FMT) < sizeof(orientation));
            if (texture->numDimensions == 1) {
                snprintf(orientation, sizeof(orientation), KTX_ORIENTATION1_FMT,
                         'r');
            } else if (texture->numDimensions == 2) {
                snprintf(orientation, sizeof(orientation), KTX_ORIENTATION2_FMT,
                         'r', options.lower_left_maps_to_s0t0 ? 'u' : 'd');
            } else
                snprintf(orientation, sizeof(orientation), KTX_ORIENTATION3_FMT,
                         'r', options.lower_left_maps_to_s0t0 ? 'u' : 'd',
                         options.lower_left_maps_to_s0t0 ? 'o' : 'i');
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

        string swizzle;
        // Add Swizzle metadata
        if (options.swizzle.size()) {
            swizzle = options.swizzle;
        } else if (!options.etc1s && !options.bopts.uastc && !options.astc
                   && defaultSwizzle.size()) {
            swizzle = defaultSwizzle;
        }
        if (swizzle.size()) {
            ktxHashList_AddKVPair(&texture->kvDataHead, KTX_SWIZZLE_KEY,
                                  (uint32_t)swizzle.size()+1,
                                  // +1 is for the NUL on the c_str
                                  swizzle.c_str());
        }
    }

    FILE* f;
    if (options.outfile.compare("-") == 0) {
        f = stdout;
#if defined(_WIN32)
        /* Set "stdout" to have binary mode */
        (void)_setmode( _fileno( stdout ), _O_BINARY );
#endif
    }
    else {
        const auto outputPath = filesystem::path(DecodeUTF8Path(options.outfile));
        if (outputPath.has_parent_path())
            filesystem::create_directories(outputPath.parent_path());
        f = fopenUTF8(options.outfile, "wb");
    }

    if (f) {
        if (options.astc || options.etc1s || options.bopts.uastc || options.zcmp) {
            string& swizzle = options.inputSwizzle.size() == 0 && defaultSwizzle.size() && !options.normalMode
                            ? defaultSwizzle
                            : options.inputSwizzle;
            exitCode = encode((ktxTexture2*)texture, swizzle,
                               f == stdout ? "stdout" : options.outfile);
            if (exitCode)
                goto closefileandcleanup;
        }
        ret = ktxTexture_WriteToStdioStream(ktxTexture(texture), f);
        if (KTX_SUCCESS != ret) {
            cerr << name << ": "
                 << "%s failed to write KTX file \"" << options.outfile
                 << "\"; KTX error: " << ktxErrorString(ret) << endl;
            exitCode = 2;
        }
closefileandcleanup:
        fclose(f);
        if (exitCode && (f != stdout)) {
            unlinkUTF8(options.outfile);
        }
    } else {
        cerr << name << ": "
             << "could not open output file \"" << options.outfile
             << "\". " << strerror(errno) << endl;
        exitCode = 2;
    }

cleanup:
    if (texture) ktxTexture_Destroy(ktxTexture(texture));
    return exitCode;
}

unique_ptr<Image>
toktxApp::createImage(const targetImageSpec& target, ImageInput& in)
{
    const ImageSpec& inSpec = in.spec();
    FormatDescriptor inputFormat;
    unique_ptr<Image> image;

    // input plugins that support channel reduction and addition do so
    // in a way which differs from the documented behaviour for --target_type
    // so alway do channel adjustments in this program.
    if (target.format().channelCount() != inSpec.format().channelCount()) {
        // Have plugin deliver all channels.
        inputFormat = inSpec.format();
        if (inSpec.format().anyChannelBitLengthNotEqual(target.format().channelBitLength())) {
            // target.format() is set so all channels have same bit length.
            std::vector<uint32_t> bits;
            bits.resize(1);
            bits[0] = target.format().channelBitLength();
            // TODO: Consider making a function for channelBitCounts.
            // Currently supported input formats all have
            // numChannels = numSamples so this works but is fragile.
            inputFormat.updateSampleBitCounts(bits);
        }
    } else {
        inputFormat = target.format();
    }
    //if (target.format().channelBitLength() == 16) {
    if (inputFormat.channelBitLength() == 16) {
        switch (inputFormat.channelCount()) {
          case 1: {
            image = make_unique<r16image>(inSpec.width(), inSpec.height());
            break;
          } case 2: {
            image = make_unique<rg16image>(inSpec.width(), inSpec.height());
            break;
          } case 3: {
            image = make_unique<rgb16image>(inSpec.width(), inSpec.height());
            break;
          } case 4: {
            image = make_unique<rgba16image>(inSpec.width(), inSpec.height());
            break;
          }
        }
    } else if (target.format().channelBitLength() == 8) {
        switch (inputFormat.channelCount()) {
          case 1: {
            image = make_unique<r8image>(inSpec.width(), inSpec.height());
            break;
          } case 2: {
            image = make_unique<rg8image>(inSpec.width(), inSpec.height());
            break;
          } case 3: {
            image = make_unique<rgb8image>(inSpec.width(), inSpec.height());
            break;
          } case 4: {
            image = make_unique<rgba8image>(inSpec.width(), inSpec.height());
            break;
          }
        }
    } else {
        stringstream message;
        uint32_t ct = inSpec.format().samples[0].channelType;
        khr_df_sample_datatype_qualifiers_e dtq;
        dtq = static_cast<khr_df_sample_datatype_qualifiers_e>(ct);
        message << "Unsupported format "
                << inSpec.format().channelBitLength()
                << "-bit " << dtq << " needed.";
        throw runtime_error(message.str());
        // TODO: uint32, uint64, float etc.
    }

    in.readImage(static_cast<uint8_t*>(*image), image->getByteCount(),
                  0/*subimage*/, 0/*miplevel*/, inputFormat);
    /* Sanity check. */
    assert(image->getWidth() * image->getHeight() * image->getPixelSize()
           == image->getByteCount());



    if (target.dstTransferFunction != nullptr) {
        assert(target.srcTransferFunction != nullptr);
        if (target.dstColorPrimaries != nullptr) {
            assert(target.srcColorPrimaries != nullptr);
            auto primaryTransform = target.srcColorPrimaries->transformTo(*target.dstColorPrimaries);

            // Transform OETF with primary transform
            image->transformColorSpace(*target.srcTransferFunction, *target.dstTransferFunction, &primaryTransform);
        } else {
            // Transform OETF without primary transform
            image->transformColorSpace(*target.srcTransferFunction, *target.dstTransferFunction);
        }
    }
    image->setPrimaries((khr_df_primaries_e)target.format().primaries());
    image->setOetf((khr_df_transfer_e)target.format().transfer());

    if (options.scale != 1.0f) {
        auto scaledWidth = image->getWidth() * options.scale;
        auto scaledHeight = image->getHeight() * options.scale;
        image = scaleImage(std::move(image),
               static_cast<ktx_uint32_t>(scaledWidth),
               static_cast<ktx_uint32_t>(scaledHeight));
    } else if (options.resize
               && (image->getWidth() != target.width()
                   || image->getHeight() != target.height()))
    {
        // --resize is not allowed with --mipmap so createImage will never be
        // called for other than the base level when set. This would be
        // incorrect otherwise. target reflects the resize value, if any.
        image = scaleImage(std::move(image), target.width(), target.height());
    }
    if (options.targetType != commandOptions::eUnspecified) {
        image = convertImageType(std::move(image));
    }
    return image;
}

unique_ptr<Image>
toktxApp::convertImageType(unique_ptr<Image> pImage)
{
    // TODO: These copyTo's should be reversed. The image should have
    // a copy constructor for each componentCount src image.
    if (options.targetType != (int)pImage->getComponentCount()) {
        unique_ptr<Image> newImage;
        string nullSwizzle = "rgba";
        // The casts in the following copyTo* definitions only work
        // because, thanks to the switch, at runtime we always pass
        // the image type being cast to.
        if (pImage->getComponentSize() == 2) {
            switch (options.targetType) {
              case commandOptions::eR:
                newImage = make_unique<r16image>(pImage->getWidth(), pImage->getHeight());
                pImage->copyToR(*newImage, nullSwizzle);
                break;
              case commandOptions::eRG:
                newImage = make_unique<rg16image>(pImage->getWidth(), pImage->getHeight());
                pImage->copyToRG(*newImage, nullSwizzle);
                break;
              case commandOptions::eRGB:
                newImage = make_unique<rgb16image>(pImage->getWidth(), pImage->getHeight());
                pImage->copyToRGB(*newImage, nullSwizzle);
                break;
              case commandOptions::eRGBA:
                newImage = make_unique<rgba16image>(pImage->getWidth(), pImage->getHeight());
                pImage->copyToRGBA(*newImage, nullSwizzle);
                break;
              case commandOptions::eUnspecified:
                assert(false);
            }
        } else {
            switch (options.targetType) {
              case commandOptions::eR:
                newImage = make_unique<r8image>(pImage->getWidth(), pImage->getHeight());
                pImage->copyToR(*newImage, nullSwizzle);
                break;
              case commandOptions::eRG:
                newImage = make_unique<rg8image>(pImage->getWidth(), pImage->getHeight());
                pImage->copyToRG(*newImage, nullSwizzle);
                break;
              case commandOptions::eRGB:
                newImage = make_unique<rgb8image>(pImage->getWidth(), pImage->getHeight());
                pImage->copyToRGB(*newImage, nullSwizzle);
                break;
              case commandOptions::eRGBA:
                newImage = make_unique<rgba8image>(pImage->getWidth(), pImage->getHeight());
                pImage->copyToRGBA(*newImage, nullSwizzle);
                break;
              case commandOptions::eUnspecified:
                assert(false);
            }
        }
        if (newImage) {
             return newImage;
        } else {
            throw runtime_error(
                "Out of memory for image with new target type."
            );
        }
    }
    return pImage;
}

// TODO: This should probably be a method on Image.
unique_ptr<Image>
toktxApp::scaleImage(unique_ptr<Image> pImage, ktx_uint32_t width, ktx_uint32_t height)
{
    try {
        pImage = pImage->resample(width, height,
                        options.gmopts.filter.c_str(),
                        options.gmopts.filterScale,
                        basisu::Resampler::Boundary_Op::BOUNDARY_CLAMP);
    } catch (runtime_error& e) {
        stringstream message;
        message << "Image::resample() failed! " << e.what();
        // A couple of the exceptions have to do with memory but the
        // others are "too large an image" and "unknown filter." The
        // latter are much more likely to occur hence choice of exception.
        throw cant_create_image(message.str());
    }
    return pImage;
}

void
toktxApp::genMipmap(unique_ptr<Image> pImage,
                    uint32_t layer, uint32_t faceSlice,
                    ktxTexture* texture)
{
    unique_ptr<Image> levelImage;
    for (uint32_t glevel = 1; glevel < texture->numLevels; glevel++) {
        auto levelWidth = maximum<uint32_t>(1, pImage->getWidth() >> glevel);
        auto levelHeight = maximum<uint32_t>(1, pImage->getHeight() >> glevel);
        try {
            levelImage = pImage->resample(levelWidth, levelHeight,
                                    options.gmopts.filter.c_str(),
                                    options.gmopts.filterScale,
                                    options.gmopts.wrapMode);
        } catch (runtime_error& e) {
            stringstream message;
            message << "Image::resample() failed! " << e.what();
            cant_create_image(message.str());
        }

        if (options.normalize)
            levelImage->normalize();

        MAYBE_UNUSED ktx_error_code_e ret;
        ret = ktxTexture_SetImageFromMemory(texture,
                                      glevel,
                                      layer,
                                      faceSlice,
                                      *levelImage,
                                      levelImage->getByteCount());
        assert(ret == KTX_SUCCESS);
    }
}

ktxTexture*
toktxApp::createTexture(const targetImageSpec& target)
{
    ktxTextureCreateInfo createInfo;
    ktxTexture* texture = 0;

    memset(&createInfo, 0, sizeof(createInfo));

    if (options.cubemap)
      createInfo.numFaces = 6;
    else
      createInfo.numFaces = 1;

    if (options.layers) {
        createInfo.numLayers = options.layers;
        createInfo.isArray = KTX_TRUE;
    } else {
        createInfo.numLayers = 1;
        createInfo.isArray = KTX_FALSE;
    }

    bool srgb = (target.format().transfer() == KHR_DF_TRANSFER_SRGB);
    uint32_t componentCount = target.format().channelCount();
    switch (componentCount) {
      case 1:
        switch (target.format().channelBitLength()) {
          case 8:
            createInfo.glInternalformat
                            = srgb ? GL_SR8 : GL_R8;
            createInfo.vkFormat
                            = srgb ? VK_FORMAT_R8_SRGB
                                   : VK_FORMAT_R8_UNORM;
            break;
          case 16:
            createInfo.glInternalformat = GL_R16;
            createInfo.vkFormat = VK_FORMAT_R16_UNORM;
            break;
          case 32:
            createInfo.glInternalformat = GL_R32F;
            createInfo.vkFormat = VK_FORMAT_R32_SFLOAT;
            break;
        }
        break;

      case 2:
         switch (target.format().channelBitLength()) {
          case 8:
            createInfo.glInternalformat
                            = srgb ? GL_SRG8 : GL_RG8;
            createInfo.vkFormat
                            = srgb ? VK_FORMAT_R8G8_SRGB
                                   : VK_FORMAT_R8G8_UNORM;
            break;
          case 16:
            createInfo.glInternalformat = GL_RG16;
            createInfo.vkFormat = VK_FORMAT_R16G16_UNORM;
            break;
          case 32:
            createInfo.glInternalformat = GL_RG32F;
            createInfo.vkFormat = VK_FORMAT_R32G32_SFLOAT;
            break;
        }
        break;

      case 3:
         switch (target.format().channelBitLength()) {
          case 8:
            createInfo.glInternalformat
                            = srgb ? GL_SRGB8 : GL_RGB8;
            createInfo.vkFormat
                            = srgb ? VK_FORMAT_R8G8B8_SRGB
                                   : VK_FORMAT_R8G8B8_UNORM;
            break;
          case 16:
            createInfo.glInternalformat = GL_RGB16;
            createInfo.vkFormat = VK_FORMAT_R16G16B16_UNORM;
            break;
          case 32:
            createInfo.glInternalformat = GL_RGB32F;
            createInfo.vkFormat = VK_FORMAT_R32G32B32_SFLOAT;
            break;
        }
        break;

      case 4:
         switch (target.format().channelBitLength()) {
          case 8:
            createInfo.glInternalformat
                            = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
            createInfo.vkFormat
                            = srgb ? VK_FORMAT_R8G8B8A8_SRGB
                                   : VK_FORMAT_R8G8B8A8_UNORM;
            break;
          case 16:
            createInfo.glInternalformat = GL_RGBA16;
            createInfo.vkFormat = VK_FORMAT_R16G16B16A16_UNORM;
            break;
          case 32:
            createInfo.glInternalformat = GL_RGBA32F;
            createInfo.vkFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
            break;
        }
        break;

      default:
        /* If we get here there's a bug. */
        assert(0);
    }
    if ((createInfo.vkFormat == VK_FORMAT_R8_SRGB
        || createInfo.vkFormat == VK_FORMAT_R8G8_SRGB)
        && !(options.astc || options.etc1s || options.bopts.uastc)) {
        // Encoding to BasisU or ASTC will cause conversion to RGB.
        warning("GPU support of sRGB variants of R & RG formats is"
                " limited.\nConsider using '--target_type' or"
                " '--convert_oetf linear' to avoid these formats.");
    }
    createInfo.baseWidth = target.width();
    createInfo.baseHeight = target.height();
    createInfo.baseDepth = options.depth ? options.depth : 1;
    if (options.depth > 0) {
        // In this case, don't care about image->getHeight(). Images are
        // always considered to be 2d. No need to set options.two_d.
        createInfo.numDimensions = 3;
    } else if (target.height() == 1 && !options.two_d)
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
            GLuint max_dim = maximum(target.width(), target.height());
            createInfo.numLevels = log2(max_dim) + 1;
            if (options.levels > 1) {
                if (options.levels > createInfo.numLevels) {
                    stringstream message;
                    message <<  "--levels value " << options.levels
                            << " is greater than the maximum"
                            << " levels possible for the image size "
                            << createInfo.numLevels << ".";
                    throw cant_create_image(message.str());
                }
                // Override the above.
                createInfo.numLevels = options.levels;
            }
        } else {
            createInfo.numLevels = 1;
        }
    }

    ktx_error_code_e ret;
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
        stringstream message;
        message << "libktx error: " << ktxErrorString(ret);
        throw cant_create_texture(message.str());
    }

    // BT709 is the default for DFDs.
    if (options.ktx2 && target.format().primaries() != KHR_DF_PRIMARIES_BT709) {
        KHR_DFDSETVAL(((ktxTexture2*)texture)->pDfd + 1, PRIMARIES,
                      target.format().primaries());
    }

    return texture;
}

static std::unique_ptr<const ColorPrimaries>
createColorPrimaries(khr_df_primaries_e primaries) {
    switch (primaries) {
    case KHR_DF_PRIMARIES_BT709:
        return std::make_unique<ColorPrimariesBT709>();
    case KHR_DF_PRIMARIES_BT601_EBU:
        return std::make_unique<ColorPrimariesBT601_625_EBU>();
    case KHR_DF_PRIMARIES_BT601_SMPTE:
        return std::make_unique<ColorPrimariesBT601_525_SMPTE>();
    case KHR_DF_PRIMARIES_BT2020:
        return std::make_unique<ColorPrimariesBT2020>();
    case KHR_DF_PRIMARIES_CIEXYZ:
        return std::make_unique<ColorPrimariesCIEXYZ>();
    case KHR_DF_PRIMARIES_ACES:
        return std::make_unique<ColorPrimariesACES>();
    case KHR_DF_PRIMARIES_ACESCC:
        return std::make_unique<ColorPrimariesACEScc>();
    case KHR_DF_PRIMARIES_NTSC1953:
        return std::make_unique<ColorPrimariesNTSC1953>();
    case KHR_DF_PRIMARIES_PAL525:
        return std::make_unique<ColorPrimariesPAL525>();
    case KHR_DF_PRIMARIES_DISPLAYP3:
        return std::make_unique<ColorPrimariesDisplayP3>();
    case KHR_DF_PRIMARIES_ADOBERGB:
        return std::make_unique<ColorPrimariesAdobeRGB>();
    default:
        assert(false);
        // We return BT709 by default if some error happened
        return std::make_unique<ColorPrimariesBT709>();
    }
}

void
toktxApp::determineTargetColorSpace(const ImageInput& in, targetImageSpec& target)
{
    // Primaries handling:
    //
    // 1. Use assign_primaries option value, if set.
    // 2. Use primaries info given by plugin.
    // 3. If no primaries info and input is PNG use PNG spec.
    //    recommendation of BT709/sRGB otherwise leave as
    //    UNSPECIFIED.
    const ImageSpec& spec = in.spec();
    // Set Primaries
    target.usedInputPrimaries = spec.format().primaries();
    if (options.assign_primaries != KHR_DF_PRIMARIES_MAX) {
        target.usedInputPrimaries = options.assign_primaries;
        target.format().setPrimaries(options.assign_primaries);
    } else if (spec.format().primaries() != KHR_DF_PRIMARIES_UNSPECIFIED) {
        target.format().setPrimaries(spec.format().primaries());
    } else {
        if (!in.formatName().compare("png")) {
            warning("No color primaries in PNG input file \"{}\", defaulting to BT.709.",
                    in.filename().c_str());
            target.usedInputPrimaries = KHR_DF_PRIMARIES_BT709;
            target.format().setPrimaries(KHR_DF_PRIMARIES_BT709);
        } else {
           // Leave as unspecified.
           target.format().setPrimaries(spec.format().primaries());
        }
    }

    if (options.convert_primaries != KHR_DF_PRIMARIES_MAX) {
        if (target.usedInputPrimaries == KHR_DF_PRIMARIES_UNSPECIFIED) {
            throw cant_create_image(
                "Cannot convert primaries as no information about the color primaries "
                "is available in the input file \"{}\". Use --assign-primaries to specify one.");
        } else if (options.convert_primaries != target.usedInputPrimaries) {
            target.srcColorPrimaries = createColorPrimaries(target.usedInputPrimaries);
            target.dstColorPrimaries = createColorPrimaries(options.convert_primaries);
            target.format().setPrimaries(options.convert_primaries);
        }
    }

    // OETF / Transfer function handling in priority order:
    //
    // 1. Use assign_oetf option value, if set.
    // 2. Use OETF signalled by plugin, if LINEAR or SRGB. If ITU signalled,
    //    set up conversion to SRGB. For all others, throw error.
    // 3. If ICC profile signalled, throw error.
    // 4. If gamma of 1.0 signalled use LINEAR. If gamma of .45454 signalled,
    //    set up for conversion to SRGB. If gamma of 0.0 is signalled,
    //    set SRGB. For any other gamma value, throw error.
    // 5. If no color info is signalled, and input is PNG follow W3C
    //    recommendation of sRGB. For other input formats throw error.
    // 6. Convert OETF based on convert_oetf option value or as described
    //    above.
    //
    target.usedInputTransferFunction = KHR_DF_TRANSFER_UNSPECIFIED;
    if (options.assign_oetf != KHR_DF_TRANSFER_UNSPECIFIED) {
        target.format().setTransfer(options.assign_oetf);
        target.usedInputTransferFunction = options.assign_oetf;
        if (options.assign_oetf == KHR_DF_TRANSFER_SRGB) {
            target.srcTransferFunction
                = std::make_unique<TransferFunctionSRGB>();
        } else {
            assert(options.assign_oetf == KHR_DF_TRANSFER_LINEAR);
            target.srcTransferFunction
                = std::make_unique<TransferFunctionLinear>();
        }
    } else {
        // Set image's OETF as indicated by metadata.
        if (spec.format().transfer() != KHR_DF_TRANSFER_UNSPECIFIED) {
            target.format().setTransfer(spec.format().transfer());
            target.usedInputTransferFunction = spec.format().transfer();
            switch (spec.format().transfer()) {
              case KHR_DF_TRANSFER_LINEAR:
                target.srcTransferFunction =
                    make_unique<TransferFunctionLinear>();
                break;
              case KHR_DF_TRANSFER_SRGB:
                target.srcTransferFunction =
                    make_unique<TransferFunctionSRGB>();
                break;
              case KHR_DF_TRANSFER_ITU:
                target.format().setTransfer(KHR_DF_TRANSFER_SRGB);
                target.srcTransferFunction =
                    make_unique<TransferFunctionITU>();
                break;
              default:
                throw cant_create_image(
                              "Transfer function not supported by KTX."
                              " Use --assign_oetf to specify a different one.");
            }
        } else if (spec.format().iccProfileName().size()) {
            throw cant_create_image(
                        "It has an ICC profile. These are not supported."
                        " Use --assign_oetf to specify handling.");
        } else if (spec.format().oeGamma() >= 0.0f) {
            if (spec.format().oeGamma() > .45450f
                && spec.format().oeGamma() < .45460f) {
                // N.B The previous loader matched oeGamma .45455 to the sRGB
                // OETF and did not do an OETF transformation. In this loader
                // we decode and reencode. Previous behavior can be obtained
                // with the --assign_oetf option to toktx.
                //
                // This change results in 1 bit differences in the LSB of
                // some color values noticeable only when directly comparing
                // images produced before and after this change of loader.
                warning("Converting gamma 2.2f to sRGB. Use --assign-oetf srgb"
                        " to force treating input as sRGB.", in.filename().c_str()
                );
                target.format().setTransfer(KHR_DF_TRANSFER_SRGB);
                target.srcTransferFunction
                    = make_unique<TransferFunctionGamma>(spec.format().oeGamma());
            } else if (spec.format().oeGamma() == 1.0) {
                target.format().setTransfer(KHR_DF_TRANSFER_LINEAR);
                target.srcTransferFunction
                    = make_unique<TransferFunctionLinear>();
            } else if (spec.format().oeGamma() == 0.0f) {
                if (!in.formatName().compare("png")) {
                    if (spec.format().channelBitLength() == 8) {
                        warning("Ignoring reported gamma of 0.0f in %s."
                                "Handling as sRGB.", in.filename().c_str());
                        target.format().setTransfer(KHR_DF_TRANSFER_SRGB);
                        target.usedInputTransferFunction = KHR_DF_TRANSFER_SRGB;
                        target.srcTransferFunction =
                            make_unique<TransferFunctionSRGB>();
                    } else {
                        warning("Ignoring reported gamma of 0.0f in %s."
                                "Handling as linear.", in.filename().c_str());
                        target.format().setTransfer(KHR_DF_TRANSFER_LINEAR);
                        target.usedInputTransferFunction =
                            KHR_DF_TRANSFER_LINEAR;
                        target.srcTransferFunction =
                            make_unique<TransferFunctionLinear>();
                    }
                } else {
                    throw cant_create_image("Its reported gamma is 0.0f."
                            " Use --assign_oetf to specify handling.");
                }
            } else {
                if (options.convert_oetf == KHR_DF_TRANSFER_UNSPECIFIED) {
                    stringstream message;
                    message << "Its encoding gamma, "
                        << spec.format().oeGamma()
                        << ", is not automatically supported by KTX." << endl
                        << "Specify handling with --convert_oetf or"
                        << " --assign_oetf.";
                    throw cant_create_image(message.str());
                } else {
                    target.srcTransferFunction
                        = make_unique<TransferFunctionGamma>(spec.format().oeGamma());
                }
            }
        } else {
            if (!in.formatName().compare("png")) {
                // Follow W3C. Treat unspecified as sRGB.
                target.format().setTransfer(KHR_DF_TRANSFER_SRGB);
                target.usedInputTransferFunction =
                    KHR_DF_TRANSFER_SRGB;
                target.srcTransferFunction =
                    make_unique<TransferFunctionSRGB>();
            } else {
                throw cant_create_image(
                    "It has no color space information."
                    " Use --assign_oetf to specify handling.");
            }
        }
    }

    if (options.convert_oetf != KHR_DF_TRANSFER_UNSPECIFIED) {
        target.format().setTransfer(options.convert_oetf);
    }

    // Need to do color conversion if either the transfer functions don't match or the primaries
    if (target.format().transfer() != target.usedInputTransferFunction ||
        target.format().primaries() != target.usedInputPrimaries) {
        if (target.srcTransferFunction == nullptr)
            throw cant_create_image(
                "No transfer function can be determined from input file."
                " Use --assign-oetf to specify one.");

        switch (target.format().transfer()) {
        case KHR_DF_TRANSFER_LINEAR:
            target.dstTransferFunction = std::make_unique<TransferFunctionLinear>();
            break;
        case KHR_DF_TRANSFER_SRGB:
            target.dstTransferFunction = std::make_unique<TransferFunctionSRGB>();
            break;
        default:
            assert(false);
            break;
        }
    }
}

void
toktxApp::determineTargetTypeBitLengthScale(const ImageInput& in,
                                            targetImageSpec& target,
                                            string& defaultSwizzle)
{
    const FormatDescriptor& format = in.spec().format();
    FormatDescriptor& targetFormat = target.format();
    uint32_t bitLength = format.channelBitLength();
    uint32_t maxValue;

    if (format.largestChannelBitLength() > 8
        && (options.etc1s || options.bopts.uastc)) {
        bitLength = 8;
    } else if (format.largestChannelBitLength() < 8) {
        bitLength = 8;
    }

    // Currently we only support unsigned normalized input formats.
    maxValue = ((1U << bitLength) - 1U);

    // TODO: Support < 8 bit channels for non-block-compressed?

    if (bitLength != format.largestChannelBitLength()) {
        warning("Rescaling %d-bit image in %s to %d bits.",
                format.channelBitLength(),
                in.filename().c_str(),
                targetFormat.channelBitLength());
    }

    uint32_t channelCount = format.channelCount();
    if (options.targetType != commandOptions::eUnspecified) {
        channelCount = options.targetType;
        targetFormat.setModel(KHR_DF_MODEL_RGBSDA);
    } else if (format.model() == KHR_DF_MODEL_YUVSDA) {
        // It's a luminance image. Override.
        assert(format.channelCount() < 3);
        targetFormat.setModel(KHR_DF_MODEL_RGBSDA);
        if (format.channelCount() == 1) {
            defaultSwizzle = "rrr1";
        } else {
            defaultSwizzle = "rrrg";
        }
    }

    // Must be after setting of model.
    if (targetFormat.anyChannelBitLengthNotEqual(bitLength)
        || maxValue != targetFormat.channelUpper()
        || channelCount != targetFormat.channelCount())
    {
        targetFormat.updateSampleInfo(channelCount, bitLength, 0, maxValue,
                                      targetFormat.channelDataType());
    }
}

void
toktxApp::determineTargetImageSpec(const ImageInput& in,
                                   targetImageSpec& target,
                                   string& defaultSwizzle)
{
    target = in.spec();
    if (options.scale != 1.0f) {
        target.setWidth(
                static_cast<ktx_uint32_t>(target.width() * options.scale));
        target.setHeight(
                static_cast<ktx_uint32_t>(target.height() * options.scale));
        target.setDepth(
                static_cast<ktx_uint32_t>(target.depth() * options.scale));
    } else if (options.resize) {
        target.setWidth(options.newGeom.width);
        target.setHeight(options.newGeom.height);
        // Current CLI does not allow for setting depth.
    }
    determineTargetTypeBitLengthScale(in, target, defaultSwizzle);
    determineTargetColorSpace(in, target);
}

void
toktxApp::checkSpecsMatch(const ImageInput& currentFile,
                          const ImageSpec& firstSpec)
{
    const FormatDescriptor firstFormat = firstSpec.format();
    const FormatDescriptor& currentFormat = currentFile.spec().format();
    if (currentFormat.transfer() != firstFormat.transfer()
        && options.convert_oetf == KHR_DF_TRANSFER_UNSPECIFIED)
    {
        stringstream msg;
        if (options.assign_oetf == KHR_DF_TRANSFER_UNSPECIFIED) {
            msg << "Image";
        } else {
            msg << "Image in " << currentFile.filename() << "("
            << currentFile.currentSubimage() << ","
            << currentFile.currentMiplevel() << ")";
        }
        msg << " has a different transfer function (OETF)"
            << " than preceding image(s).";
        if (options.assign_oetf == KHR_DF_TRANSFER_UNSPECIFIED) {
            msg << endl
                << "Use --assign_oetf (not recommended) or --convert_oetf to"
                << " stop this error.";
            throw cant_create_image(msg.str());
        } else {
            warning(msg.str());
        }
        // Don't warn when convert_oetf is set as proper conversions
        // will be done so all images will be in the same space.
    }
    if (currentFormat.primaries() != firstFormat.primaries()) {
        stringstream msg;
        if (options.assign_primaries == KHR_DF_PRIMARIES_UNSPECIFIED) {
            msg << "Image";
        } else{
            msg << "Image in " << currentFile.filename() << "("
            << currentFile.currentSubimage() << ","
            << currentFile.currentMiplevel() << ")";
        }
        msg << " has different primaries than preceding images(s).";
        if (options.assign_primaries == KHR_DF_PRIMARIES_UNSPECIFIED) {
            msg << endl
                << "Use --assign_primaries (not recommended) to"
                << " stop this error.";
            throw cant_create_image(msg.str());
        } else
            warning(msg.str());
        // There is no convert_primaries option.
    }
    if (currentFormat.channelCount() != firstFormat.channelCount()) {
        stringstream msg;
        if (options.targetType == commandOptions::eUnspecified) {
            msg << "Image";
        } else{
            msg << "Image in " << currentFile.filename() << "("
            << currentFile.currentSubimage() << ","
            << currentFile.currentMiplevel() << ")";
        }
        msg << " has a different component count than"
            << " preceding images(s).";
        if (options.targetType == commandOptions::eUnspecified) {
            msg << endl
                << "Use --target_type to stop this error (not recommended).";
            throw cant_create_image(msg.str());
        } else  {
            msg << endl
                << "The components of the level or layer derived "
                << "from this file will likely be significantly "
                << "different"
                << endl
                << "from those in other levels or layers.";
            warning(msg.str());
        }
    }
}

void
toktxApp::setAstcMode(const targetImageSpec& target)
{
    // If no astc mode option is specified and if input is <= 8bit
    // default to LDR otherwise default to HDR
    if (options.astcopts.mode == KTX_PACK_ASTC_ENCODER_MODE_DEFAULT) {
        if (target.format().channelBitLength() <= 8)
            options.astcopts.mode = KTX_PACK_ASTC_ENCODER_MODE_LDR;
        else
            options.astcopts.mode = KTX_PACK_ASTC_ENCODER_MODE_HDR;
    } else {
        if (target.format().channelBitLength() > 8
            && options.astcopts.mode == KTX_PACK_ASTC_ENCODER_MODE_LDR)
        {
            // Input is > 8-bit and user wants LDR, issue quality loss warning.
            stringstream msg;
            msg << "Input file is 16-bit but ASTC LDR option is specified."
                << " Expect quality loss in the output."
                << endl;
            warning(msg.str());
        } else if (target.format().channelBitLength() < 16
                   && options.astcopts.mode == KTX_PACK_ASTC_ENCODER_MODE_HDR)
        {
            // Input is < 8bit and user wants HDR, issue warning.
            stringstream msg;
            msg << "Input file is not 16-bit but HDR option is specified."
                << endl;
            warning(msg.str());
        }
    }
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
    if (options.cubemap && options.depth > 0) {
        error("cubemaps cannot have 3D textures.");
        usage();
        exit(1);
    }
    if (options.layers && options.depth > 0) {
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

    if (options.depth > 1 && options.genmipmap) {
        error("generation of mipmaps for 3d textures is not supported.\n"
              "A PR to add this feature will be gratefully accepted!");
        exit(1);
    }

    if (options.outfile.compare("-") != 0
            && options.outfile.find_last_of('.') == string::npos)
    {
        options.outfile.append(options.ktx2 ? ".ktx2" : ".ktx");
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
    string toktx_options;
    char* env_options = getenv("TOKTX_OPTIONS");

    if (env_options != nullptr)
        toktx_options = env_options;
    else
        return;

    if (!toktx_options.empty()) {
        istringstream iss(toktx_options);
        argvector arglist;
        for (string w; iss >> w; )
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
 * @brief parse a color primaries argument
 */
 khr_df_primaries_e
 toktxApp::parseColorPrimaries(string& argValue) {
        static const std::unordered_map<std::string, khr_df_primaries_e> values{
            { "NONE", KHR_DF_PRIMARIES_UNSPECIFIED },
            { "BT709", KHR_DF_PRIMARIES_BT709 },
            { "SRGB", KHR_DF_PRIMARIES_SRGB },
            { "BT601-EBU", KHR_DF_PRIMARIES_BT601_EBU },
            { "BT601-SMPTE", KHR_DF_PRIMARIES_BT601_SMPTE },
            { "BT2020", KHR_DF_PRIMARIES_BT2020 },
            { "CIEXYZ", KHR_DF_PRIMARIES_CIEXYZ },
            { "ACES", KHR_DF_PRIMARIES_ACES },
            { "ACESCC", KHR_DF_PRIMARIES_ACESCC },
            { "NTSC1953", KHR_DF_PRIMARIES_NTSC1953 },
            { "PAL525", KHR_DF_PRIMARIES_PAL525 },
            { "DISPLAYP3", KHR_DF_PRIMARIES_DISPLAYP3 },
            { "ADOBERGB", KHR_DF_PRIMARIES_ADOBERGB },
        };

        khr_df_primaries_e result = {};

        if (argValue.length()) {
            for_each(argValue.begin(), argValue.end(), [](char & c) {
                c = (char)::toupper(c);
            });

            const auto it = values.find(argValue);
            if (it != values.end()) {
                result = it->second;
            } else {
                cerr << name
                     << "Invalid or unsupported transfer function specified: "
                     << argValue << endl;
                exit(1);
            }
        }

        return result;
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
    // N.B. It is not possible for an optarg string to be a negative number
    // because the leading '-' will make the parser think it is an option
    // leading to a "missing required argument" error before this is ever called.
    switch (opt) {
      case 0:
        break;
      case 'a':
        options.layers = (uint32_t)strtoi(parser.optarg.c_str());
        if (options.layers == 0) {
            cerr << name << ": "
                 << "To create an array texture set --layers > 0." << endl;
            exit(1);
        }
        break;
      case 'd':
        options.depth = (uint32_t)strtoi(parser.optarg.c_str());
        if (options.depth == 0) {
            cerr << name << ": "
                 << "To create a 3d texture set --depth > 0." << endl;
            exit(1);
        }
        break;
      case 'l':
        options.levels = (uint32_t)strtoi(parser.optarg.c_str());
        if (options.levels < 2) {
            cerr << name << ": "
                 << "--levels must be > 1." << endl;
            exit(1);
        }
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
      case 1101:
        validateSwizzle(parser.optarg);
        options.swizzle = parser.optarg;
        break;
      case 1102:
        for_each(parser.optarg.begin(), parser.optarg.end(), [](char & c) {
            c = (char)::toupper(c);
        });
        if (parser.optarg.compare("R") == 0)
          options.targetType = commandOptions::eR;
        else if (parser.optarg.compare("RG") == 0)
          options.targetType = commandOptions::eRG;
        else if (parser.optarg.compare("RGB") == 0)
          options.targetType = commandOptions::eRGB;
        else if (parser.optarg.compare("RGBA") == 0)
          options.targetType = commandOptions::eRGBA;
        else {
            cerr << name << ": unrecognized target_type \"" << parser.optarg
                 << "\"." << endl;
            usage();
            exit(1);
        }
        break;
      case 1103:
        for_each(parser.optarg.begin(), parser.optarg.end(), [](char & c) {
            c = (char)::tolower(c);
        });
        if (parser.optarg.compare("linear") == 0)
            options.assign_oetf = KHR_DF_TRANSFER_LINEAR;
        else if (parser.optarg.compare("srgb") == 0)
            options.assign_oetf = KHR_DF_TRANSFER_SRGB;
        break;
      case 1104:
        for_each(parser.optarg.begin(), parser.optarg.end(), [](char & c) {
            c = (char)::tolower(c);
        });
        if (parser.optarg.compare("linear") == 0)
            options.convert_oetf = KHR_DF_TRANSFER_LINEAR;
        else if (parser.optarg.compare("srgb") == 0)
            options.convert_oetf = KHR_DF_TRANSFER_SRGB;
        break;
      case 1105:
        options.assign_primaries = parseColorPrimaries(parser.optarg);
        break;
      case 1106:
        options.convert_primaries = parseColorPrimaries(parser.optarg);
        break;
      case ':':
      default:
        return scApp::processOption(parser, opt);
    }
    return true;
}

void warning(const char *pFmt, va_list args) {
    toktx.warning(pFmt, args);
}

void warning(const char *pFmt, ...) {
    va_list args;
    va_start(args, pFmt);

    toktx.warning(pFmt, args);
    va_end(args);
}

void warning(const string& msg) {
   toktx.warning(msg);
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
dumpImage(char* name, int width, int height, int components, int componentSize,
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

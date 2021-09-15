// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 sts=4 expandtab:

// Copyright 2019-2020 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

#include <zstd.h>
#include "ktxapp.h"
#include <unordered_map>
#include <algorithm>

template<typename T>
struct clampedOption
{
  clampedOption(T& option, T min_v, T max_v) :
    option(option),
    min(min_v),
    max(max_v)
  {
  }

  void clear()
  {
    option = 0;
  }

  operator T() const
  {
    return option;
  }

  T operator= (T v)
  {
    option = clamp<T>(v, min, max);
    return option;
  }

  T& option;
  T min;
  T max;
};

/**
 * @memberof ktxTexture
 * @ingroup write
 * @~English
 * @brief Creates valid ASTC block dimension from string.
 *
 * @return      Valid ktx_pack_astc_block_dimension_e from string
 */
ktx_pack_astc_block_dimension_e
astcBlockDimension(const char* block_size) {
  static std::unordered_map<std::string, ktx_pack_astc_block_dimension_e>
      astc_blocks_mapping{{"4x4", KTX_PACK_ASTC_BLOCK_DIMENSION_4x4},
                          {"5x4", KTX_PACK_ASTC_BLOCK_DIMENSION_5x4},
                          {"5x5", KTX_PACK_ASTC_BLOCK_DIMENSION_5x5},
                          {"6x5", KTX_PACK_ASTC_BLOCK_DIMENSION_6x5},
                          {"6x6", KTX_PACK_ASTC_BLOCK_DIMENSION_6x6},
                          {"8x5", KTX_PACK_ASTC_BLOCK_DIMENSION_8x5},
                          {"8x6", KTX_PACK_ASTC_BLOCK_DIMENSION_8x6},
                          {"10x5", KTX_PACK_ASTC_BLOCK_DIMENSION_10x5},
                          {"10x6", KTX_PACK_ASTC_BLOCK_DIMENSION_10x6},
                          {"8x8", KTX_PACK_ASTC_BLOCK_DIMENSION_8x8},
                          {"10x8", KTX_PACK_ASTC_BLOCK_DIMENSION_10x8},
                          {"10x10", KTX_PACK_ASTC_BLOCK_DIMENSION_10x10},
                          {"12x10", KTX_PACK_ASTC_BLOCK_DIMENSION_12x10},
                          {"12x12", KTX_PACK_ASTC_BLOCK_DIMENSION_12x12},
                          {"3x3x3", KTX_PACK_ASTC_BLOCK_DIMENSION_3x3x3},
                          {"4x3x3", KTX_PACK_ASTC_BLOCK_DIMENSION_4x3x3},
                          {"4x4x3", KTX_PACK_ASTC_BLOCK_DIMENSION_4x4x3},
                          {"4x4x4", KTX_PACK_ASTC_BLOCK_DIMENSION_4x4x4},
                          {"5x4x4", KTX_PACK_ASTC_BLOCK_DIMENSION_5x4x4},
                          {"5x5x4", KTX_PACK_ASTC_BLOCK_DIMENSION_5x5x4},
                          {"5x5x5", KTX_PACK_ASTC_BLOCK_DIMENSION_5x5x5},
                          {"6x5x5", KTX_PACK_ASTC_BLOCK_DIMENSION_6x5x5},
                          {"6x6x5", KTX_PACK_ASTC_BLOCK_DIMENSION_6x6x5},
                          {"6x6x6", KTX_PACK_ASTC_BLOCK_DIMENSION_6x6x6}};

  auto opt = astc_blocks_mapping.find(block_size);

  if (opt != astc_blocks_mapping.end())
      return opt->second;

  return KTX_PACK_ASTC_BLOCK_DIMENSION_6x6;
}

/**
 * @memberof ktxTexture
 * @ingroup write
 * @~English
 * @brief Creates valid ASTC quality from string.
 *
 * @return      Valid ktx_pack_astc_quality_e from string
 */
ktx_pack_astc_quality_levels_e
astcQualityLevel(const char *quality) {

    static std::unordered_map<std::string,
                              ktx_pack_astc_quality_levels_e> astc_quality_mapping{
        {"fastest", KTX_PACK_ASTC_QUALITY_LEVEL_FASTEST},
        {"fast", KTX_PACK_ASTC_QUALITY_LEVEL_FAST},
        {"medium", KTX_PACK_ASTC_QUALITY_LEVEL_MEDIUM},
        {"thorough", KTX_PACK_ASTC_QUALITY_LEVEL_THOROUGH},
        {"exhaustive", KTX_PACK_ASTC_QUALITY_LEVEL_EXHAUSTIVE}
    };

  auto opt = astc_quality_mapping.find(quality);

  if (opt != astc_quality_mapping.end())
      return opt->second;

  return KTX_PACK_ASTC_QUALITY_LEVEL_MEDIUM;
}

/**
 * @memberof ktxTexture
 * @ingroup write
 * @~English
 * @brief Creates valid ASTC mode from string.
 *
 * @return      Valid ktx_pack_astc_mode_e from string
 */
ktx_pack_astc_encoder_mode_e
astcEncoderMode(const char* mode) {
    if (strcmp(mode, "ldr") == 0)
        return KTX_PACK_ASTC_ENCODER_MODE_LDR;
    else if (strcmp(mode, "hdr") == 0)
        return KTX_PACK_ASTC_ENCODER_MODE_HDR;

  return KTX_PACK_ASTC_ENCODER_MODE_DEFAULT;
}

/*
// Markdown doesn't work in files included by snipped{doc} or include{doc}
// so the table below has to be laboriously done in html.
//! [scApp options]
  <dl>
    <dt>--encode &lt;astc|etc1s|uastc&gt;</dt>
                 <dd>Compress the image data to ASTC, transcodable ETC1S / BasisLZ or
                 high-quality transcodable UASTC format. Implies @b --t2.
                 With each encoding option the following encoder specific options
                 become valid, otherwise they are ignored.</dd>
      <dl>
      <dt>astc:</dt>
                 <dd>Create a texture in high-quality ASTC format.</dd>
        <dt>--astc_blk_d &lt;XxY|XxYxZ&gt;</dt>
                 <dd>Specify which block dimension to use for compressing the textures.
                 e.g. @b --astc_blk_d 6x5 for 2D or @b --astc_blk_d 6x6x6 for 3D.
                 6x6 is default for 2D.
                 <table>
                     <tr><th>Supported 2D block dimensions are:</th></tr>
                         <tr><td>4x4</td> <td>8.00 bpp</td></tr>
                         <tr><td>5x4</td> <td>6.40 bpp</td></tr>
                         <tr><td>5x5</td> <td>5.12 bpp</td></tr>
                         <tr><td>6x5</td> <td>4.27 bpp</td></tr>
                         <tr><td>6x6</td> <td>3.56 bpp</td></tr>
                         <tr><td>8x5</td> <td>3.20 bpp</td></tr>
                         <tr><td>8x6</td> <td>2.67 bpp</td></tr>
                         <tr><td>10x5</td>  <td>2.56 bpp</td></tr>
                         <tr><td>10x6</td>  <td>2.13 bpp</td></tr>
                         <tr><td>8x8</td>   <td>2.00 bpp</td></tr>
                         <tr><td>10x8</td>  <td>1.60 bpp</td></tr>
                         <tr><td>10x10</td> <td>1.28 bpp</td></tr>
                         <tr><td>12x10</td> <td>1.07 bpp</td></tr>
                         <tr><td>12x12</td> <td>0.89 bpp</td></tr>
                     <tr><th>Supported 3D block dimensions are:</th></tr>
                         <tr><td>3x3x3</td> <td>4.74 bpp</td></tr>
                         <tr><td>4x3x3</td> <td>3.56 bpp</td></tr>
                         <tr><td>4x4x3</td> <td>2.67 bpp</td></tr>
                         <tr><td>4x4x4</td> <td>2.00 bpp</td></tr>
                         <tr><td>5x4x4</td> <td>1.60 bpp</td></tr>
                         <tr><td>5x5x4</td> <td>1.28 bpp</td></tr>
                         <tr><td>5x5x5</td> <td>1.02 bpp</td></tr>
                         <tr><td>6x5x5</td> <td>0.85 bpp</td></tr>
                         <tr><td>6x6x5</td> <td>0.71 bpp</td></tr>
                         <tr><td>6x6x6</td> <td>0.59 bpp</td></tr>
                 </table></dd>
        <dt>--astc_mode &lt;ldr|hdr&gt;</dt>
                 <dd>Specify which encoding mode to use. LDR is the default unless the input.
                 image is 16-bit in which case the default is HDR.</dd>
        <dt>--astc_quality &lt;level&gt;</dt>
                 <dd>The quality level configures the quality-performance tradeoff for
                 the compressor; more complete searches of the search space improve
                 image quality at the expense of compression time. Default is 'medium'
                 The quality level can be set to fastest (0) and thorough (100) via the
                 following fixed quality presets:
                 <table>
                     <tr><th>Level      </th> <th> Quality                      </th></tr>
                     <tr><td>fastest    </td> <td>(equivalent to quality =   0) </td></tr>
                     <tr><td>fast       </td> <td>(equivalent to quality =  10) </td></tr>
                     <tr><td>medium     </td> <td>(equivalent to quality =  60) </td></tr>
                     <tr><td>thorough   </td> <td>(equivalent to quality =  98) </td></tr>
                     <tr><td>exhaustive </td> <td>(equivalent to quality = 100) </td></tr>
                 </table>
                 </dd>
      </dl>
      <dl>
      <dt>etc1s:</dt>
                 <dd>Supercompress the image data with ETC1S / BasisLZ.
                 RED images will become RGB with RED in each component. RG images
                 will have R in the RGB part and G in the alpha part of the
                 compressed texture. When set, the following BasisLZ-related
                 options become valid, otherwise they are ignored.</dd>
        <dt>--no_multithreading</dt>
                 <dd>Disable multithreading. Deprecated. For backward compatibility.
                 Use @b --threads 1 instead.</dd>
        <dt>--clevel &lt;level&gt;</dt>
                 <dd>ETC1S / BasisLZ compression level, an encoding speed vs. quality
                 tradeoff. Range is [0,5], default is 1. Higher values are slower,
                 but give higher quality.</dd>
        <dt>--qlevel &lt;level&gt;</dt>
                 <dd>ETC1S / BasisLZ quality level. Range is [1,255]. Lower gives
                 better compression/lower quality/faster. Higher gives less
                 compression/higher quality/slower. @b --qlevel automatically
                 determines values for @b --max_endpoints, @b --max-selectors,
                 @b --endpoint_rdo_threshold and @b --selector_rdo_threshold for the
                 target quality level. Setting these options overrides the values
                 determined by -qlevel which defaults to 128 if neither it nor
                 both of @b --max_endpoints and @b --max_selectors have been set.

                 Note that both of @b --max_endpoints and @b --max_selectors
                 must be set for them to have any effect. If all three options
                 are set, a warning will be issued that @b --qlevel will be ignored.

                 Note also that @b --qlevel will only determine values for
                 @b --endpoint_rdo_threshold and @b --selector_rdo_threshold when
                 its value exceeds 128, otherwise their defaults will be used.</dd>
        <dt>--max_endpoints &lt;arg&gt;</dt>
                 <dd>Manually set the maximum number of color endpoint clusters. Range
                 is [1,16128]. Default is 0, unset.</dd>
        <dt>--endpoint_rdo_threshold &lt;arg&gt;</dt>
                 <dd>Set endpoint RDO quality threshold. The default is 1.25. Lower
                 is higher quality but less quality per output bit (try
                 [1.0,3.0]). This will override the value chosen by @b --qlevel.</dd>
        <dt>--max_selectors &lt;arg&gt;</dt>
                 <dd>Manually set the maximum number of color selector clusters from
                 [1,16128]. Default is 0, unset.</dd>
        <dt>--selector_rdo_threshold &lt;arg&gt;</dt>
                 <dd>Set selector RDO quality threshold. The default is 1.25. Lower
                 is higher quality but less quality per output bit (try
                 [1.0,3.0]). This will override the value chosen by @b --qlevel.</dd>
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
      <dl>
      <dt>uastc:</dt>
                 <dd>Create a texture in high-quality transcodable UASTC format.</dd>
        <dt>--uastc_quality &lt;level&gt;</dt>
                 <dd>This optional parameter selects a speed vs quality
                 tradeoff as shown in the following table:

               <table>
                   <tr><th>Level</th>   <th>Speed</th> <th>Quality</th></tr>
                   <tr><td>0   </td><td> Fastest </td><td> 43.45dB</td></tr>
                   <tr><td>1   </td><td> Faster </td><td> 46.49dB</td></tr>
                   <tr><td>2   </td><td> Default </td><td> 47.47dB</td></tr>
                   <tr><td>3   </td><td> Slower </td><td> 48.01dB</td></tr>
                   <tr><td>4   </td><td> Very slow </td><td> 48.24dB</td></tr>
               </table>

                 You are strongly encouraged to also specify @b --zcmp to losslessly
                 compress the UASTC data. This and any LZ-style compression can
                 be made more effective by conditioning the UASTC texture data
                 using the Rate Distortion Optimization (RDO) post-process stage.
                 When uastc encoding is set the following options become available
                 for controlling RDO:</dd>
        <dt>--uastc_rdo_l [&lt;lambda&gt;]</dt>
                 <dd>Enable UASTC RDO post-processing and optionally set UASTC RDO
                 quality scalar (lambda) to @e lambda.  Lower values yield higher
                 quality/larger LZ compressed files, higher values yield lower
                 quality/smaller LZ compressed files. A good range to try is
                 [.25,10]. For normal maps a good range is [.25,.75]. The full
                 range is [.001,10.0]. Default is 1.0.

                 Note that previous versions used the @b --uastc_rdo_q option which
                 was removed because the RDO algorithm changed.</dd>
        <dt>--uastc_rdo_d &lt;dictsize&gt;</dt>
                 <dd>Set UASTC RDO dictionary size in bytes. Default is 4096. Lower
                 values=faster, but give less compression. Range is [64,65536].</dd>
        <dt>--uastc_rdo_b &lt;scale&gt;</dt>
                 <dd>Set UASTC RDO max smooth block error scale. Range is [1.0,300.0].
                 Default is 10.0, 1.0 is disabled. Larger values suppress more
                 artifacts (and allocate more bits) on smooth blocks.</dd>
        <dt>--uastc_rdo_s &lt;deviation&gt;</dt>
                 <dd>Set UASTC RDO max smooth block standard deviation. Range is
                 [.01,65536.0]. Default is 18.0. Larger values expand the range
                 of blocks considered smooth.</dd>
        <dt>--uastc_rdo_f</dt>
                 <dd>Do not favor simpler UASTC modes in RDO mode.</dd>
        <dt>--uastc_rdo_m</dt>
                 <dd>Disable RDO multithreading (slightly higher compression,
                 deterministic).</dd>
      </dl>
    <dt>--normal_mode</dt>
                 <dd>For ASTC encoder '@b --encode astc' assumes the input texture is
                 a three component linear LDR normal map storing unit length
                 normals as (R=X, G=Y, B=Z). The output will be a two component
                 X+Y normal map stored as (RGB=X, A=Y), optimized for angular
                 error instead of simple PSNR. The Z component can be recovered
                 programmatically in shader code by using the equation:
                 <pre>
      nml.xy = texture(...).ga;              // Load in [0,1]
      nml.xy = nml.xy * 2.0 - 1.0;           // Unpack to [-1,1]
      nml.z = sqrt(1 - dot(nml.xy, nml.xy)); // Compute Z
                 </pre>
                 For ETC1S encoder '@b --encode etc1s' tunes codec parameters for
                 better quality on normal maps (no selector RDO, no endpoint RDO).
                 Only valid for linear textures.</dd>
    <dt>--no_sse</dt>
                 <dd>Forbid use of the SSE instruction set. Ignored if CPU does not
                 support SSE. Only the Basis Universal compressor uses SSE.</dd>
    <dt>--bcmp</dt>
                 <dd>Deprecated. Use '@b --encode etc1s' instead.</dd>
    <dt>--uastc [&lt;level&gt;]</dt>
                 <dd>Deprecated. Use '@b --encode uastc' instead.</dd>
    <dt>--zcmp [&lt;compressionLevel&gt;]</dt>
                 <dd>Supercompress the data with Zstandard. Implies @b --t2. Can be used
                 with data in any format except ETC1S / BasisLZ. Most
                 effective with RDO-conditioned UASTC or uncompressed formats. The
                 optional compressionLevel range is 1 - 22 and the default is 3.
                 Lower values=faster but give less compression. Values above 20
                 should be used with caution as they require more memory.</dd>
    <dt>--threads &lt;count&gt;</dt>
                 <dd>Explicitly set the number of threads to use during compression.
                 By default, ETC1S / BasisLZ and ASTC compression will use the number of
                 threads reported by thread::hardware_concurrency or 1 if value
                 returned is 0.</dd>
    <dt>--verbose</dt>
                 <dd>Print encoder/compressor activity status to stdout. Currently
                 only the astc, etc1s and uastc encoders emit status.</dd>
  </dl>
  @snippet{doc} ktxapp.h ktxApp options

  In case of ambiguity, such as when the last option is one with an optional
  parameter, separate options from file names with " -- ".

  Any specified ETC1S / BasisLZ and supercompression options are recorded in
  the metadata item @c KTXwriterScParams in the output file.
//! [scApp options]
*/

class scApp : public ktxApp {
  protected:
    struct commandOptions : public ktxApp::commandOptions {
        struct basisOptions : public ktxBasisParams {
            // The remaining numeric fields are clamped within the Basis
            // library.
            clampedOption<ktx_uint32_t> threadCount;
            clampedOption<ktx_uint32_t> qualityLevel;
            clampedOption<ktx_uint32_t> maxEndpoints;
            clampedOption<ktx_uint32_t> maxSelectors;
            clampedOption<ktx_uint32_t> uastcRDODictSize;
            clampedOption<float> uastcRDOQualityScalar;
            clampedOption<float> uastcRDOMaxSmoothBlockErrorScale;
            clampedOption<float> uastcRDOMaxSmoothBlockStdDev;

            basisOptions() :
                threadCount(ktxBasisParams::threadCount, 1, 10000),
                qualityLevel(ktxBasisParams::qualityLevel, 1, 255),
                maxEndpoints(ktxBasisParams::maxEndpoints, 1, 16128),
                maxSelectors(ktxBasisParams::maxSelectors, 1, 16128),
                uastcRDODictSize(ktxBasisParams::uastcRDODictSize, 256, 65536),
                uastcRDOQualityScalar(ktxBasisParams::uastcRDOQualityScalar,
                                      0.001f, 50.0f),
                uastcRDOMaxSmoothBlockErrorScale(
                              ktxBasisParams::uastcRDOMaxSmoothBlockErrorScale,
                              1.0f, 300.0f),
                uastcRDOMaxSmoothBlockStdDev(
                              ktxBasisParams::uastcRDOMaxSmoothBlockStdDev,
                              0.01f, 65536.0f)
            {
                uint32_t tc = thread::hardware_concurrency();
                if (tc == 0) tc = 1;
                threadCount.max = tc;
                threadCount = tc;

                structSize = sizeof(ktxBasisParams);
                // - 1 is to match what basisu_tool does (since 1.13).
                compressionLevel = KTX_ETC1S_DEFAULT_COMPRESSION_LEVEL - 1;
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
                uastc = false; // Default to ETC1S.
                uastcRDO = false;
                uastcFlags = KTX_PACK_UASTC_LEVEL_DEFAULT;
                uastcRDODictSize.clear();
                uastcRDOQualityScalar.clear();
                uastcRDODontFavorSimplerModes = false;
                uastcRDONoMultithreading = false;
                noSSE = false;
                verbose = false; // Default to quiet operation.
                for (int i = 0; i < 4; i++) inputSwizzle[i] = 0;
            }
        };

        struct astcOptions : public ktxAstcParams {
            clampedOption<ktx_uint32_t> threadCount;
            clampedOption<ktx_uint32_t> blockDimension;
            clampedOption<ktx_uint32_t> mode;
            clampedOption<ktx_uint32_t> qualityLevel;

            astcOptions() :
                threadCount(ktxAstcParams::threadCount, 1, 10000),
                blockDimension(ktxAstcParams::blockDimension, 0, KTX_PACK_ASTC_BLOCK_DIMENSION_MAX),
                mode(ktxAstcParams::mode, 0, KTX_PACK_ASTC_ENCODER_MODE_MAX),
                qualityLevel(ktxAstcParams::qualityLevel, 0, KTX_PACK_ASTC_QUALITY_LEVEL_MAX)
            {
                uint32_t tc = thread::hardware_concurrency();
                if (tc == 0) tc = 1;
                threadCount.max = tc;
                threadCount = tc;

                structSize = sizeof(ktxAstcParams);
                blockDimension.clear();
                blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_6x6;
                mode.clear();
                qualityLevel.clear();
                normalMap = false;
            }
        };
        int          ktx2;
        int          etc1s;
        int          zcmp;
        int          astc;
        ktx_bool_t   normalMode;
        clamped<ktx_uint32_t> zcmpLevel;
        clamped<ktx_uint32_t> threadCount;
        struct basisOptions bopts;
        struct astcOptions astcopts;

        commandOptions() :
            zcmpLevel(ZSTD_CLEVEL_DEFAULT, 1U, 22U),
            threadCount(std::max(1U, thread::hardware_concurrency()) , 1U, 10000U)
        {
            ktx2 = false;
            etc1s = false;
            zcmp = false;
            astc = false;
            normalMode = false;
        }
    };

    commandOptions& options;
    const string scparamKey = "KTXwriterScParams";
    string scparams;

    virtual bool processOption(argparser& parser, int opt);
    enum HasArg { eNone, eOptional, eRequired };
    void captureOption(const argparser& parser, HasArg hasArg);
    void validateOptions();

  public:
    scApp(string& version, string& defaultVersion, scApp::commandOptions& options);
    const string& getParamsStr() {
        if (!scparams.empty() && *(scparams.end()-1) == ' ')
            scparams.erase(scparams.end()-1);
        return scparams;
    }

    void setEncoder(string encoding) {
        if (encoding == "astc")
            options.astc = 1;
        else if (encoding == "etc1s")
            options.etc1s = 1;
        else if (encoding == "uastc")
            options.bopts.uastc = 1;
    }

    void usage()
    {
        cerr <<
          "  --encode <astc|etc1s|uastc>\n"
          "               Compress the image data to ASTC, transcodable ETC1S / BasisLZ or\n"
          "               high-quality transcodable UASTC format. Implies --t2.\n"
          "               With each encoding option the following encoder specific options\n"
          "               become valid, otherwise they are ignored.\n\n"
          "    astc:\n"
          "               Create a texture in high-quality ASTC format.\n"
          "      --astc_blk_d <XxY|XxYxZ>\n"
          "               Specify which block dimension to use for compressing the textures.\n"
          "               e.g. --astc_blk_d 6x5 for 2D or --astc_blk_d 6x6x6 for 3D.\n"
          "               6x6 is default for 2D.\n\n"
          "                   Supported 2D block dimensions are:\n\n"
          "                       4x4: 8.00 bpp         10x5:  2.56 bpp\n"
          "                       5x4: 6.40 bpp         10x6:  2.13 bpp\n"
          "                       5x5: 5.12 bpp         8x8:   2.00 bpp\n"
          "                       6x5: 4.27 bpp         10x8:  1.60 bpp\n"
          "                       6x6: 3.56 bpp         10x10: 1.28 bpp\n"
          "                       8x5: 3.20 bpp         12x10: 1.07 bpp\n"
          "                       8x6: 2.67 bpp         12x12: 0.89 bpp\n\n"
          "                   Supported 3D block dimensions are:\n\n"
          "                       3x3x3: 4.74 bpp       5x5x4: 1.28 bpp\n"
          "                       4x3x3: 3.56 bpp       5x5x5: 1.02 bpp\n"
          "                       4x4x3: 2.67 bpp       6x5x5: 0.85 bpp\n"
          "                       4x4x4: 2.00 bpp       6x6x5: 0.71 bpp\n"
          "                       5x4x4: 1.60 bpp       6x6x6: 0.59 bpp\n"
          "      --astc_mode <ldr|hdr>\n"
          "               Specify which encoding mode to use. LDR is the default unless the input.\n"
          "               image is 16-bit in which case the default is HDR.\n"
          "      --astc_quality <level>\n"
          "               The quality level configures the quality-performance tradeoff for\n"
          "               the compressor; more complete searches of the search space improve\n"
          "               image quality at the expense of compression time. Default is 'medium'\n"
          "               The quality level can be set to fastest (0) and thorough (100) via the \n"
          "               following fixed quality presets:\n\n"
          "                   Level      |  Quality\n"
          "                   ---------- | -----------------------------\n"
          "                   fastest    | (equivalent to quality =   0)\n"
          "                   fast       | (equivalent to quality =  10)\n"
          "                   medium     | (equivalent to quality =  60)\n"
          "                   thorough   | (equivalent to quality =  98)\n"
          "                   exhaustive | (equivalent to quality = 100)\n"
          "    etc1s:\n"
          "               Supercompress the image data with ETC1S / BasisLZ.\n"
          "               RED images will become RGB with RED in each component. RG images\n"
          "               will have R in the RGB part and G in the alpha part of the\n"
          "               compressed texture. When set, the following BasisLZ-related\n"
          "               options become valid, otherwise they are ignored.\n\n"
          "      --no_multithreading\n"
          "               Disable multithreading. Deprecated. For backward compatibility.\n"
          "               Use --threads 1 instead.\n"
          "      --clevel <level>\n"
          "               ETC1S / BasisLZ compression level, an encoding speed vs. quality\n"
          "               tradeoff. Range is [0,5], default is 1. Higher values are slower,\n"
          "               but give higher quality.\n"
          "      --qlevel <level>\n"
          "               ETC1S / BasisLZ quality level. Range is [1,255]. Lower gives\n"
          "               better compression/lower quality/faster. Higher gives less\n"
          "               compression/higher quality/slower. --qlevel automatically\n"
          "               determines values for --max_endpoints, --max-selectors,\n"
          "               --endpoint_rdo_threshold and --selector_rdo_threshold for the\n"
          "               target quality level. Setting these options overrides the values\n"
          "               determined by -qlevel which defaults to 128 if neither it nor\n"
          "               both of --max_endpoints and --max_selectors have been set.\n"
          "\n"
          "               Note that both of --max_endpoints and --max_selectors\n"
          "               must be set for them to have any effect. If all three options\n"
          "               are set, a warning will be issued that --qlevel will be ignored.\n"
          "\n"
          "               Note also that --qlevel will only determine values for\n"
          "               --endpoint_rdo_threshold and --selector_rdo_threshold when\n"
          "               its value exceeds 128, otherwise their defaults will be used.\n"
          "      --max_endpoints <arg>\n"
          "               Manually set the maximum number of color endpoint clusters. Range\n"
          "               is [1,16128]. Default is 0, unset.\n"
          "      --endpoint_rdo_threshold <arg>\n"
          "               Set endpoint RDO quality threshold. The default is 1.25. Lower\n"
          "               is higher quality but less quality per output bit (try\n"
          "               [1.0,3.0]). This will override the value chosen by --qlevel.\n"
          "      --max_selectors <arg>\n"
          "               Manually set the maximum number of color selector clusters from\n"
          "               [1,16128]. Default is 0, unset.\n"
          "      --selector_rdo_threshold <arg>\n"
          "               Set selector RDO quality threshold. The default is 1.25. Lower\n"
          "               is higher quality but less quality per output bit (try\n"
          "               [1.0,3.0]). This will override the value chosen by --qlevel.\n"
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
          "               to do selector RDO.\n\n"
          "    uastc:\n"
          "               Create a texture in high-quality transcodable UASTC format.\n"
          "      --uastc_quality <level>\n"
          "               This optional parameter selects a speed vs quality\n"
          "               tradeoff as shown in the following table:\n"
          "\n"
          "                   Level |  Speed    | Quality\n"
          "                   ----- | --------- | -------\n"
          "                     0   |  Fastest  | 43.45dB\n"
          "                     1   |  Faster   | 46.49dB\n"
          "                     2   |  Default  | 47.47dB\n"
          "                     3   |  Slower   | 48.01dB\n"
          "                     4   | Very slow | 48.24dB\n"
          "\n"
          "               You are strongly encouraged to also specify --zcmp to losslessly\n"
          "               compress the UASTC data. This and any LZ-style compression can\n"
          "               be made more effective by conditioning the UASTC texture data\n"
          "               using the Rate Distortion Optimization (RDO) post-process stage.\n"
          "               When uastc encoding is set the following options become available\n"
          "               for controlling RDO:\n\n"
          "      --uastc_rdo_l [<lambda>]\n"
          "               Enable UASTC RDO post-processing and optionally set UASTC RDO\n"
          "               quality scalar (lambda) to @e lambda.  Lower values yield higher\n"
          "               quality/larger LZ compressed files, higher values yield lower\n"
          "               quality/smaller LZ compressed files. A good range to try is\n"
          "               [.25,10]. For normal maps a good range is [.25,.75]. The full\n"
          "               range is [.001,10.0]. Default is 1.0.\n"
          "\n"
          "               Note that previous versions used the --uastc_rdo_q option which\n"
          "               was removed because the RDO algorithm changed.\n"
          "      --uastc_rdo_d <dictsize>\n"
          "               Set UASTC RDO dictionary size in bytes. Default is 4096. Lower\n"
          "               values=faster, but give less compression. Range is [64,65536].\n"
          "      --uastc_rdo_b <scale>\n"
          "               Set UASTC RDO max smooth block error scale. Range is [1.0,300.0].\n"
          "               Default is 10.0, 1.0 is disabled. Larger values suppress more\n"
          "               artifacts (and allocate more bits) on smooth blocks.\n"
          "      --uastc_rdo_s <deviation>\n"
          "               Set UASTC RDO max smooth block standard deviation. Range is\n"
          "               [.01,65536.0]. Default is 18.0. Larger values expand the range\n"
          "               of blocks considered smooth.<dd>\n"
          "      --uastc_rdo_f\n"
          "               Do not favor simpler UASTC modes in RDO mode.\n"
          "      --uastc_rdo_m\n"
          "               Disable RDO multithreading (slightly higher compression,\n"
          "               deterministic).\n\n"
          "  --normal_mode\n"
          "               For ASTC encoder '--encode astc' assumes the input texture is\n"
          "               a three component linear LDR normal map storing unit length\n"
          "               normals as (R=X, G=Y, B=Z). The output will be a two component\n"
          "               X+Y normal map stored as (RGB=X, A=Y), optimized for angular\n"
          "               error instead of simple PSNR. The Z component can be recovered\n"
          "               programmatically in shader code by using the equation:\n\n"
          "                   nml.xy = texture(...).ga;              // Load in [0,1]\n"
          "                   nml.xy = nml.xy * 2.0 - 1.0;           // Unpack to [-1,1]\n"
          "                   nml.z = sqrt(1 - dot(nml.xy, nml.xy)); // Compute Z\n\n"
          "               For ETC1S encoder '--encode etc1s' tunes codec parameters for \n"
          "               better quality on normal maps (no selector RDO, no endpoint RDO).\n"
          "               Only valid for linear textures.\n"
          "  --no_sse\n"
          "               Forbid use of the SSE instruction set. Ignored if CPU does not\n"
          "               support SSE. Only the Basis Universal compressor uses SSE.\n"
          "  --bcmp\n"
          "               Deprecated. Use '--encode etc1s' instead.\n"
          "  --uastc [<level>]\n"
          "               Deprecated. Use '--encode uastc' instead.\n"
          "  --zcmp [<compressionLevel>]\n"
          "               Supercompress the data with Zstandard. Implies --t2. Can be used\n"
          "               with data in any format except ETC1S / BasisLZ. Most\n"
          "               effective with RDO-conditioned UASTC or uncompressed formats. The\n"
          "               optional compressionLevel range is 1 - 22 and the default is 3.\n"
          "               Lower values=faster but give less compression. Values above 20\n"
          "               should be used with caution as they require more memory.\n"
          "  --threads <count>\n"
          "               Explicitly set the number of threads to use during compression.\n"
          "               By default, ETC1S / BasisLZ and ASTC compression will use the number of\n"
          "               threads reported by thread::hardware_concurrency or 1 if value\n"
          "               returned is 0.\n"
          "  --verbose\n"
          "               Print encoder/compressor activity status to stdout. Currently\n"
          "               only the astc, etc1s and uastc encoders emit status.\n"
          "\n";
          ktxApp::usage();
          cerr << endl <<
          "In case of ambiguity, such as when the last option is one with an optional\n"
          "parameter, options can be separated from file names with \" -- \".\n"
          "\n"
          "Any specified ETC1S / BasisLZ and supercompression options are recorded in\n"
          "the metadata item @c KTXwriterScParams in the output file.\n"
          << endl;
    }
};

scApp::scApp(string& version, string& defaultVersion,
             scApp::commandOptions& options)
      : ktxApp(version, defaultVersion, options), options(options)
{
  argparser::option my_option_list[] = {
      { "zcmp", argparser::option::optional_argument, NULL, 'z' },
      { "no_multithreading", argparser::option::no_argument, NULL, 'N' },
      { "threads", argparser::option::required_argument, NULL, 't' },
      { "clevel", argparser::option::required_argument, NULL, 'c' },
      { "qlevel", argparser::option::required_argument, NULL, 'q' },
      { "max_endpoints", argparser::option::required_argument, NULL, 'e' },
      { "endpoint_rdo_threshold", argparser::option::required_argument, NULL, 'E' },
      { "max_selectors", argparser::option::required_argument, NULL, 'u' },
      { "selector_rdo_threshold", argparser::option::required_argument, NULL, 'S' },
      { "normal_mode", argparser::option::no_argument, NULL, 'n' },
      { "separate_rg_to_color_alpha", argparser::option::no_argument, NULL, 1000 },
      { "no_endpoint_rdo", argparser::option::no_argument, NULL, 1001 },
      { "no_selector_rdo", argparser::option::no_argument, NULL, 1002 },
      { "no_sse", argparser::option::no_argument, NULL, 1011 },
      { "uastc_quality", argparser::option::required_argument, NULL, 1003 },
      { "uastc_rdo_l", argparser::option::optional_argument, NULL, 1004 },
      { "uastc_rdo_d", argparser::option::required_argument, NULL, 1005 },
      { "uastc_rdo_b", argparser::option::optional_argument, NULL, 1006 },
      { "uastc_rdo_s", argparser::option::optional_argument, NULL, 1007 },
      { "uastc_rdo_f", argparser::option::no_argument, NULL, 1008 },
      { "uastc_rdo_m", argparser::option::no_argument, NULL, 1009 },
      { "verbose", argparser::option::no_argument, NULL, 1010 },
      { "astc_blk_d", argparser::option::required_argument, NULL, 1012 },
      { "astc_mode", argparser::option::required_argument, NULL, 1013 },
      { "astc_quality", argparser::option::required_argument, NULL, 1014 },
      { "encode", argparser::option::required_argument, NULL, 1015 },
      // Deprecated options
      { "bcmp", argparser::option::no_argument, NULL, 'b' },
      { "uastc", argparser::option::optional_argument, NULL, 1016 }
  };
  const int lastOptionIndex = sizeof(my_option_list)
                              / sizeof(argparser::option);
  option_list.insert(option_list.begin(), my_option_list,
                     my_option_list + lastOptionIndex);
  short_opts += "z;Nt:c:q:e:E:u:S:nb";
}

void
scApp::captureOption(const argparser& parser, HasArg hasArg)
{
    uint32_t indexDecrement = 1;
    bool captureArg = false;

    if ((hasArg == eOptional && parser.optarg.size() > 0) || hasArg == eRequired)
        indexDecrement = 2;
    scparams += parser.argv[parser.optind - indexDecrement] + " ";
    if (captureArg)
        scparams += parser.optarg + " ";
}

void
scApp::validateOptions() {
    if ((options.bopts.maxEndpoints == 0) ^ (options.bopts.maxSelectors == 0)) {
        cerr << name << ": Both or neither of --max_endpoints and"
             << " --max_selectors must be specified." << endl;
        usage();
        exit(1);
    }
    if (options.bopts.qualityLevel
        && (options.bopts.maxEndpoints + options.bopts.maxSelectors)) {
        cerr << name << ": Warning: ignoring --qlevel as it, --max_endpoints"
             << " and --max_selectors are all set." << endl;
    }
}

// Derived classes' processOption will have to explicitly call this one
// and should call it after processing their own options.
bool
scApp::processOption(argparser& parser, int opt)
{
    bool hasArg = false;
    bool capture = true;

    switch (opt) {
      case 'z':
        if (options.etc1s) {
            cerr << "Only one of '--encode etc1s|--bcmp'  and --zcmp can be specified."
                 << endl;
            usage();
            exit(1);
        }
        options.zcmp = 1;
        options.ktx2 = 1;
        if (parser.optarg.size() > 0) {
            options.zcmpLevel = strtoi(parser.optarg.c_str());
            hasArg = true;
        }
        break;
      case 'c':
        options.bopts.compressionLevel = strtoi(parser.optarg.c_str());
        hasArg = true;
        break;
      case 'e':
        options.bopts.maxEndpoints = strtoi(parser.optarg.c_str());
        hasArg = true;
        break;
      case 'E':
        options.bopts.endpointRDOThreshold = strtof(parser.optarg.c_str(), nullptr);
        hasArg = true;
        break;
      case 'N':
        options.threadCount = 1;
        capture = false;
        break;
      case 'n':
        options.normalMode = true;
        break;
      case 1001:
        options.bopts.noEndpointRDO = 1;
        break;
      case 1002:
        options.bopts.noSelectorRDO = 1;
        break;
      case 'q':
        options.bopts.qualityLevel = strtoi(parser.optarg.c_str());
        hasArg = true;
        break;
      case 1000:
        options.bopts.separateRGToRGB_A = 1;
        break;
      case 'u':
        options.bopts.maxSelectors = strtoi(parser.optarg.c_str());
        hasArg = true;
        break;
      case 'S':
        options.bopts.selectorRDOThreshold = strtof(parser.optarg.c_str(), nullptr);
        hasArg = true;
        break;
      case 't':
        options.threadCount = strtoi(parser.optarg.c_str());
        capture = false;
        break;
      case 1003:
        {
            ktx_uint32_t level = strtoi(parser.optarg.c_str());
            level = clamp<ktx_uint32_t>(level, 0, KTX_PACK_UASTC_MAX_LEVEL);
            // Ensure the last one wins in case of multiple of these args.
            options.bopts.uastcFlags = (unsigned int)~KTX_PACK_UASTC_LEVEL_MASK;
            options.bopts.uastcFlags |= level;
            hasArg = true;
        }
        break;
      case 1004:
        options.bopts.uastcRDO = true;
        if (parser.optarg.size() > 0) {
            options.bopts.uastcRDOQualityScalar =
                                strtof(parser.optarg.c_str(), nullptr);
            hasArg = true;
        }
        break;
      case 1005:
        options.bopts.uastcRDODictSize = strtoi(parser.optarg.c_str());
        hasArg = true;
        break;
      case 1006:
        options.bopts.uastcRDOMaxSmoothBlockErrorScale =
                               strtof(parser.optarg.c_str(), nullptr);
        hasArg = true;
        break;
      case 1007:
        options.bopts.uastcRDOMaxSmoothBlockStdDev =
                               strtof(parser.optarg.c_str(), nullptr);
        hasArg = true;
        break;
      case 1008:
        options.bopts.uastcRDODontFavorSimplerModes = true;
        break;
      case 1009:
        options.bopts.uastcRDONoMultithreading = true;
        break;
      case 1010:
        options.bopts.verbose = true;
        options.astcopts.verbose = true;
        capture = false;
        break;
      case 1011:
        options.bopts.noSSE = true;
        capture = true;
        break;
      case 1012: // astc_blk_d
        options.astcopts.blockDimension = astcBlockDimension(parser.optarg.c_str());
        hasArg = true;
        break;
      case 1013: // astc_mode
        options.astcopts.mode = astcEncoderMode(parser.optarg.c_str());
        hasArg = true;
        break;
      case 1014: // astc_quality
        options.astcopts.qualityLevel = astcQualityLevel(parser.optarg.c_str());
        hasArg = true;
        break;
      case 'b':
        if (options.zcmp) {
            cerr << "Only one of --bcmp and --zcmp can be specified.\n"
                 << "--bcmp is deprecated, use '--encode etc1s' instead."
                 << endl;
            usage();
            exit(1);
        }
        if (options.bopts.uastc) {
            cerr << "Only one of --bcmp and '--encode etc1s|--uastc' can be specified.\n"
                 << "--bcmp is deprecated, use '--encode etc1s' instead."
                 << endl;
            usage();
            exit(1);
        }
        options.etc1s = 1;
        options.ktx2 = 1;
        break;
      case 1015:
        setEncoder(parser.optarg);
        options.ktx2 = 1;
        break;
      case 1016:
        if (options.etc1s) {
             cerr << "Only one of `--encode etc1s|--bcmp` and `--uastc [<level>]` can be specified."
                  << endl;
             usage();
             exit(1);
        }
        options.bopts.uastc = 1;
        options.ktx2 = 1;
        if (parser.optarg.size() > 0) {
            ktx_uint32_t level = strtoi(parser.optarg.c_str());
            level = clamp<ktx_uint32_t>(level, 0, KTX_PACK_UASTC_MAX_LEVEL);
            // Ensure the last one wins in case of multiple of these args.
            options.bopts.uastcFlags = (unsigned int)~KTX_PACK_UASTC_LEVEL_MASK;
            options.bopts.uastcFlags |= level;
            hasArg = true;
        }
        break;
      default:
        return false;
    }

    if (capture) {
        scparams += parser.argv[parser.optind - (hasArg ? 2 : 1)] + " ";
        if (hasArg)
            scparams += parser.optarg + " ";
    }

    return true;
}

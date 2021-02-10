// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 sts=4 expandtab:

// Copyright 2019-2020 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

#include <zstd.h>
#include "ktxapp.h"

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

/*
// Markdown doesn't work in files included by snipped{doc} or include{doc}
// so the table below has to be laboriously done in html.
//! [scApp options]
  <dl>
    <dt>--bcmp</dt>
    <dd>Supercompress the image data with ETC1S / BasisLZ. Implies @b --t2.
        RED images will become RGB with RED in each component. RG images will
        have R in the RGB part and G in the alpha part of the compressed
        texture. When set, the following BasisLZ-related options become valid
        otherwise they are ignored.
      <dl>
      <dt>--no_multithreading</dt>
      <dd>Disable multithreading. Deprecated. For backward compatibility
          only. Use @b --threads 1 instead.</dd>
      <dt>--threads &lt;count&gt;</dt>
      <dd>Explicitly set the number of threads to use during compression.
          By default, ETC1S / BasisLZ compression will use the number of threads
          reported by @c thread::hardware_concurrency or 1 if value
          returned is 0.</dd>
      <dt>--clevel &lt;level&gt;</dt>
      <dd>ETC1S / BasisLZ compression level, an encoding speed vs. quality tradeoff.
          Range is 0 - 5, default is 1. Higher values are slower, but give
          higher quality.</dd>
      <dt>--qlevel &lt;level&gt;</dt>
      <dd>ETC1S / BasisLZ quality level. Range is 1 - 255.  Lower gives better
          compression/lower quality/faster. Higher gives less compression
          /higher quality/slower. @b --qlevel automatically determines values
          for @b --max_endpoints, @b --max-selectors,
          @b --endpoint_rdo_threshold and @b --selector_rdo_threshold for the
          target quality level. Setting these options overrides the values
          determined by @b --qlevel which defaults to 128 if neither it nor
          both of @b --max_endpoints and @b --max_selectors have been set.

          @note Both of @b --max_endpoints and @b --max_selectors must be set
          for them to have any effect. If all three options are set, a
          warning will be issued that @b --qlevel will be ignored.
          @note @b --qlevel will only determine values for
          @b --endpoint_rdo_threshold and @b --selector_rdo_threshold
          when its value exceeds 128, otherwise their defaults will be used.
      <dt>--max_endpoints &lt;arg&gt;</dt>
      <dd>Manually set the maximum number of color endpoint clusters
          from 1-16128. Default is 0, unset. If this is set,
          @b --max_selectors must also be set, otherwise the value
          will be ignored.</dd>
      <dt>--endpoint_rdo_threshold &lt;arg&gt;</dt>
      <dd>Set endpoint RDO quality threshold. The default is 1.25. Lower is
          higher quality but less quality per output bit (try 1.0-3.0).
          This will override the value chosen by @c --qual.</dd>
      <dt>--max_selectors &lt;arg&gt;</dt>
      <dd>Manually set the maximum number of color selector clusters
          from 1-16128. Default is 0, unset. If this is set,
          @b --max_selectors must also be set, otherwise the value
          will be ignored.</dd>
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
    <dt>--uastc [&lt;level&gt;]</dt>
    <dd>Create a texture in high-quality transcodable UASTC format. Implies
        @b --t2. The optional parameter @e level selects a speed vs quality
        tradeoff as shown in the following table:

        <table>
        <tr><th>Level</th>   <th>Speed</th> <th>Quality</th></tr>
        <tr><td>0   </td><td> Fastest </td><td> 43.45dB</td></tr>
        <tr><td>1   </td><td> Faster </td><td> 46.49dB</td></tr>
        <tr><td>2   </td><td> Default </td><td> 47.47dB</td></tr>
        <tr><td>3   </td><td> Slower </td><td> 48.01dB</td></tr>
        <tr><td>4   </td><td> Very slow </td><td> 48.24dB</td></tr>
        </table>

        When set the following options become available for controlling the
        optional Rate Distortion Optimization (RDO) post-process stage that
        conditions the encoded UASTC texture data so it can be more effectively
        LZ compressed:
      <dl>
        <dt>--uastc_rdo_q [&lt;quality&gt;]</dt>
        <dd>Enable UASTC RDO post-processing and optionally set UASTC RDO
            quality scalar to @e quality.  Lower values yield higher
            quality/larger LZ compressed files, higher values yield lower
            quality/smaller LZ compressed files. A good range to try is
            [.2-4]." Full range is .001 to 10.0. Default is 1.0.</dd>
        <dt>--uastc_rdo_d &lt;dictsize&gt;</dt>
        <dd>Set UASTC RDO dictionary size in bytes. Default is 32768. Lower
            values=faster, but give less compression. Possible range is 256 to
            65536.</dd>
      </dl>
    </dd>
    <dt>--verbose</dt>
    <dd>Print encoder/compressor activity status to stdout. Currently only
        the Basis Universal compressor emits status.</dd>
    <dt>--zcmp [&lt;compressionLevel&gt;]</dt>
    <dd>Supercompress the data with Zstandard. Implies @b --t2. Can be used
        with data in any format except ETC1S / BasisLZ (@b --bcmp). Most
        effective with RDO-conditioned UASTC or uncompressed formats. The
        optional @e compressionLevel range is 1 - 22 and the default is 3.
        Lower values=faster but give less compression. Values above 20 should
        be used with caution as they require more memory.</dd>
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

            basisOptions() :
                threadCount(ktxBasisParams::threadCount, 1, 10000),
                qualityLevel(ktxBasisParams::qualityLevel, 1, 255),
                maxEndpoints(ktxBasisParams::maxEndpoints, 1, 16128),
                maxSelectors(ktxBasisParams::maxSelectors, 1, 16128),
                uastcRDODictSize(ktxBasisParams::uastcRDODictSize, 256, 65536),
                uastcRDOQualityScalar(ktxBasisParams::uastcRDOQualityScalar,
                                      0.001f, 10.0f)
            {
                uint32_t tc = thread::hardware_concurrency();
                if (tc == 0) tc = 1;
                threadCount.max = tc;
                threadCount = tc;

                structSize = sizeof(ktxBasisParams);
                compressionLevel = KTX_ETC1S_DEFAULT_COMPRESSION_LEVEL;
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
                verbose = false; // Default to quiet operation.
            }
        };

        int          ktx2;
        int          bcmp;
        int          zcmp;
        clamped<ktx_uint32_t> zcmpLevel;
        struct basisOptions bopts;

        commandOptions() : zcmpLevel(ZSTD_CLEVEL_DEFAULT, 1U, 22U) {
            ktx2 = false;
            bcmp = false;
            zcmp = false;
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

    void usage()
    {
        cerr <<
          "  --bcmp       Supercompress the image data with ETC1S / BasisLZ. Implies --t2.\n"
          "               RED images will become RGB with RED in each component. RG images\n"
          "               will have R in the RGB part and G in the alpha part of the\n"
          "               compressed texture. When set, the following BasisLZ-related\n"
          "               options become valid, otherwise they are ignored.\n\n"
          "      --no_multithreading\n"
          "               Disable multithreading. Deprecated. For backward compatibility.\n"
          "               Use --threads 1 instead.\n"
          "      --threads <count>\n"
          "               Explicitly set the number of threads to use during compression.\n"
          "               By default, ETC1S / BasisLZ compression will use the number of threads\n"
          "               reported by thread::hardware_concurrency or 1 if value returned\n"
          "               is 0.\n"
          "      --clevel <level>\n"
          "               ETC1S / BasisLZ compression level, an encoding speed vs. quality tradeoff.\n"
          "               Range is 0 - 5, default is 1. Higher values are slower, but give\n"
          "               higher quality.\n"
          "      --qlevel <level>\n"
          "               ETC1S / BasisLZ quality level. Range is 1 - 255.  Lower gives better\n"
          "               compression/lower quality/faster. Higher gives less compression\n"
          "               /higher quality/slower. --qlevel automatically determines values\n"
          "               for --max_endpoints, --max-selectors, --endpoint_rdo_threshold\n"
          "               and --selector_rdo_threshold for the target quality level.\n"
          "               Setting these options overrides the values determined by\n"
          "               --qlevel which defaults to 128 if neither it nor both of\n"
          "               --max_endpoints and --max_selectors have been set.\n"
          "\n"
          "               Note that both of --max_endpoints and --max_selectors\n"
          "               must be set for them to have any effect. If all three options\n"
          "               are set, a warning will be issued that --qlevel will be ignored.\n"
          "\n"
          "               Note also that --qlevel will only determine values for\n"
          "               --endpoint_rdo_threshold and --selector_rdo_threshold when\n"
          "               its value exceeds 128, otherwise their defaults will be used.\n"
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
          "               to do selector RDO.\n\n"
          "  --uastc [<level>]\n"
          "               Create a texture in high-quality transcodable UASTC format.\n"
          "               Implies --t2. The optional parameter <level> selects a speed\n"
          "               vs quality tradeoff as shown in the following table:\n"
          "\n"
          "                 Level |  Speed    | Quality\n"
          "                 ----- | -------   | -------\n"
          "                   0   |  Fastest  | 43.45dB\n"
          "                   1   |  Faster   | 46.49dB\n"
          "                   2   |  Default  | 47.47dB\n"
          "                   3   |  Slower   | 48.01dB\n"
          "                   4   | Very slow | 48.24dB\n"
          "\n"
          "               When set the following options become available for controlling\n"
          "               the optional Rate Distortion Optimization (RDO) post-process stage\n"
          "               that conditions the encoded UASTC texture data so it can be more\n"
          "               effectively LZ compressed:\n\n"
          "      --uastc_rdo_q [<quality>]\n"
          "               Enable UASTC RDO post-processing and optionally set UASTC RDO\n"
          "               quality scalar to <quality>.  Lower values yield higher\n"
          "               quality/larger LZ compressed files, higher values yield lower\n"
          "               quality/smaller LZ compressed files. A good range to try is [.2-4].\n"
          "               Full range is .001 to 10.0. Default is 1.0.\n"
          "      --uastc_rdo_d <dictsize>\n"
          "               Set UASTC RDO dictionary size in bytes. Default is 32768. Lower\n"
          "               values=faster, but give less compression. Possible range is 256\n"
          "               to 65536.\n\n"
          "  --verbose    Print encoder/compressor activity status to stdout. Currently only\n"
          "               the Basis Universal compressor emits status."
          "  --zcmp [<compressionLevel>]\n"
          "               Supercompress the data with Zstandard. Implies --t2. Can be used\n"
          "               with data in any format except ETC1S / BasisLZ (--bcmp). Most\n"
          "               effective with RDO-conditioned UASTC or uncompressed formats. The\n"
          "               optional compressionLevel range is 1 - 22 and the default is 3.\n"
          "               Lower values=faster but give less compression. Values above 20\n"
          "               should be used with caution as they require more memory.\n";
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
      : options(options), ktxApp(version, defaultVersion, options)
{
  argparser::option my_option_list[] = {
      { "bcmp", argparser::option::no_argument, NULL, 'b' },
      { "zcmp", argparser::option::optional_argument, NULL, 'z' },
      { "no_multithreading", argparser::option::no_argument, NULL, 'N' },
      { "threads", argparser::option::required_argument, NULL, 't' },
      { "clevel", argparser::option::required_argument, NULL, 'c' },
      { "qlevel", argparser::option::required_argument, NULL, 'q' },
      { "max_endpoints", argparser::option::required_argument, NULL, 'e' },
      { "endpoint_rdo_threshold", argparser::option::required_argument, NULL, 'E' },
      { "max_selectors", argparser::option::required_argument, NULL, 'u' },
      { "selector_rdo_threshold", argparser::option::required_argument, NULL, 'S' },
      { "normal_map", argparser::option::no_argument, NULL, 'n' },
      { "separate_rg_to_color_alpha", argparser::option::no_argument, NULL, 1000 },
      { "no_endpoint_rdo", argparser::option::no_argument, NULL, 1001 },
      { "no_selector_rdo", argparser::option::no_argument, NULL, 1002 },
      { "uastc", argparser::option::optional_argument, NULL, 1003 },
      { "uastc_rdo_q", argparser::option::optional_argument, NULL, 1004 },
      { "uastc_rdo_d", argparser::option::required_argument, NULL, 1005 },
      { "verbose", argparser::option::no_argument, NULL, 1006 }
  };
  const int lastOptionIndex = sizeof(my_option_list)
                              / sizeof(argparser::option);
  option_list.insert(option_list.begin(), my_option_list,
                     my_option_list + lastOptionIndex);
  short_opts += "bz;Nt:c:q:e:E:u:S:n";
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
      case 'b':
        if (options.zcmp) {
            cerr << "Only one of --bcmp and --zcmp can be specified."
                 << endl;
            usage();
            exit(1);
        }
        if (options.bopts.uastc) {
            cerr << "Only one of --bcmp and --uastc can be specified."
                 << endl;
            usage();
            exit(1);
        }
        options.bcmp = 1;
        options.ktx2 = 1;
        break;
      case 'z':
        if (options.bcmp) {
            cerr << "Only one of --bcmp and --zcmp can be specified."
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
        options.bopts.threadCount = 1;
        capture = false;
        break;
      case 'n':
        options.bopts.normalMap = 1;
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
        options.bopts.threadCount = strtoi(parser.optarg.c_str());
        capture = false;
        break;
      case 1003:
        if (options.bcmp) {
             cerr << "Only one of --bcmp and --uastc can be specified."
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
            options.bopts.uastcFlags = ~KTX_PACK_UASTC_LEVEL_MASK;
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
        options.bopts.verbose = true;
        capture = false;
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


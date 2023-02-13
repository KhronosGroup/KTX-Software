// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include "command.h"
#include "formats.h"
#include "sbufstream.h"
#include "utility.h"
#include "validate.h"
#include "ktx.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>

#include <cxxopts.hpp>
#include <fmt/ostream.h>
#include <fmt/printf.h>


// -------------------------------------------------------------------------------------------------

namespace ktx {

enum class TranscodeTarget {
    etc_rgb,
    etc_rgba,
    eac_r11,
    eac_rg11,
    bc1,
    bc3,
    bc4,
    bc5,
    bc7,
    astc,
    r8,
    rg8,
    rgb8,
    rgba8,
};

[[nodiscard]] constexpr inline auto operator+(TranscodeTarget value) noexcept {
    return to_underlying(value);
}

struct Swizzle {
    enum Channel {
        red = 0,
        green = 1,
        blue = 2,
        alpha = 3,
        zero,
        one,
    };

    int32_t numChannels = 0; /// Specifies how many output channels to keep
    std::array<Channel, 4> channels{red, green, blue, alpha};
};

// -------------------------------------------------------------------------------------------------

/** @page ktxtools_extract ktx extract
@~English

Extracts a KTX2 file.

@warning TODO Tools P5: This page is incomplete

@section ktxtools_extract_synopsis SYNOPSIS
    ktx extract [options] @e input_file

@section ktxtools_extract_description DESCRIPTION

    The following options are available:
    <dl>
        <dt>-f, --flag</dt>
        <dd>Flag description</dd>
    </dl>
    @snippet{doc} ktx/command.h command options

@section ktxtools_extract_exitstatus EXIT STATUS
    @b ktx @b extract exits
        0 - Success
        1 - Command line error
        2 - IO error
        3 - Invalid input or state

@section ktxtools_extract_history HISTORY

@par Version 4.0
 - Initial version

@section ktxtools_extract_author AUTHOR
    - Mátyás Császár [Vader], RasterGrid www.rastergrid.com
    - Daniel Rákos, RasterGrid www.rastergrid.com
*/
class CommandExtract : public Command {
    enum {
        all = -1,
    };

    struct Options {
        std::string outputPath;
        std::optional<TranscodeTarget> transcode;
        std::string uri;
        std::optional<int32_t> level = 0; /// -1 indicates all
        std::optional<int32_t> layer = 0; /// -1 indicates all
        std::optional<int32_t> face = 0; /// -1 indicates all
        std::optional<int32_t> depth = 0; /// -1 indicates all
        bool raw = false;
    } options;

public:
    virtual int main(int argc, _TCHAR* argv[]) override;

protected:
    virtual void initOptions(cxxopts::Options& opts) override;
    virtual void processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) override;

private:
    std::optional<khr_df_model_channels_e> getChannelType(const KTXTexture2& texture, uint32_t index);
    TranscodeTarget detectTranscodeTarget(const KTXTexture2& texture);
    void executeExtract();
    void writeRawFile(const std::string& filepath, const char* data, std::size_t size, const std::optional<Swizzle>& swizzle);

private:
    template <typename... Args>
    void warning(Args&&... args) {
        fmt::print(std::cerr, "{} warning: ", processName);
        fmt::print(std::cerr, std::forward<Args>(args)...);
        fmt::print(std::cerr, "\n");
    }

    template <typename... Args>
    void error(Args&&... args) {
        fmt::print(std::cerr, "{} error: ", processName);
        fmt::print(std::cerr, std::forward<Args>(args)...);
        fmt::print(std::cerr, "\n");
    }

    template <typename... Args>
    void fatal(int return_code, Args&&... args) {
        fmt::print(std::cerr, "{} fatal: ", processName);
        fmt::print(std::cerr, std::forward<Args>(args)...);
        fmt::print(std::cerr, "\n");
        throw FatalError(return_code);
    }
};

// -------------------------------------------------------------------------------------------------

int CommandExtract::main(int argc, _TCHAR* argv[]) {
    try {
        parseCommandLine("ktx extract", "Export a selected image from a KTX2 file.\n", argc, argv);
        executeExtract();
        return RETURN_CODE_SUCCESS;
    } catch (const FatalError& error) {
        return error.return_code;
    }
}

void CommandExtract::initOptions(cxxopts::Options& opts) {
    opts.add_options()
            ("output", "Output filepath for single, output directory for multiple image export.", cxxopts::value<std::string>(), "<filepath>")
            ("transcode", "Target transcode format. Case-insensitive.\n"
                          "Possible options are: "
                          "etc-rgb | etc-rgba | eac-r11 | eac-rg11 | bc1 | bc3 | bc4 | bc5 | bc7 | astc | r8 | rg8 | rgb8 | rgba8",
                          cxxopts::value<std::string>(), "<target>")
            ("uri", "KTX Fragment URI.", cxxopts::value<std::string>(), "<uri>")
            ("level", "Level to extract. Defaults to 0. Case-insensitive.", cxxopts::value<std::string>(), "[0-9]+ | all")
            ("layer", "Layer to extract. Defaults to 0. Case-insensitive.", cxxopts::value<std::string>(), "[0-9]+ | all")
            ("face", "Face to extract. Defaults to 0. Case-insensitive.", cxxopts::value<std::string>(), "[0-5] | all")
            ("depth", "Depth slice to extract. Defaults to 0. Case-insensitive.", cxxopts::value<std::string>(), "[0-9]+ | all")
            ("all", "Extract all image slices. Unset by default.", cxxopts::value<bool>())
            ("raw", "Extract raw image data. Unset by default.", cxxopts::value<bool>());

    Command::initOptions(opts);
    opts.parse_positional({"input-file", "output"});
    opts.positional_help("<input-file> <output>");
}

void CommandExtract::processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) {
    Command::processOptions(opts, args);

    options.outputPath = args["output"].as<std::string>();

    if (args["transcode"].count()) {
        static const std::unordered_map<std::string, TranscodeTarget> values{
                {"etc-rgb", TranscodeTarget::etc_rgb},
                {"etc-rgba", TranscodeTarget::etc_rgba},
                {"eac-r11", TranscodeTarget::eac_r11},
                {"eac-rg11", TranscodeTarget::eac_rg11},
                {"bc1", TranscodeTarget::bc1},
                {"bc3", TranscodeTarget::bc3},
                {"bc4", TranscodeTarget::bc4},
                {"bc5", TranscodeTarget::bc5},
                {"bc7", TranscodeTarget::bc7},
                {"astc", TranscodeTarget::astc},
                {"r8", TranscodeTarget::r8},
                {"rg8", TranscodeTarget::rg8},
                {"rgb8", TranscodeTarget::rgb8},
                {"rgba8", TranscodeTarget::rgba8},
        };

        const auto transcodeStr = to_lower_copy(args["transcode"].as<std::string>());
        const auto it = values.find(transcodeStr);
        if (it == values.end())
            fatal(RETURN_CODE_INVALID_ARGUMENTS, "Invalid transcode target: \"{}\"", transcodeStr);
        else
            options.transcode = it->second;
    }

    if (args["uri"].count())
        // TODO Tools P4: Validate and parse fragment URI, Handle error conditions
        options.uri = args["uri"].as<std::string>();

    const auto parseDimensionIndex = [&](const std::string& name) -> std::optional<int32_t> {
        if (!args[name].count())
            return std::nullopt;
        const auto str = to_lower_copy(args[name].as<std::string>());
        try {
            return str == "all" ? all : std::stoi(str);
        } catch (const std::invalid_argument& e) {
            fatal(RETURN_CODE_INVALID_ARGUMENTS, "Invalid {} value \"{}\": {}", name, str, e.what());
        } catch (const std::out_of_range& e) {
            fatal(RETURN_CODE_INVALID_ARGUMENTS, "Out of range {} value \"{}\": {}", name, str, e.what());
        }
        return std::nullopt;
    };

    options.level = parseDimensionIndex("level");
    options.layer = parseDimensionIndex("layer");
    options.face = parseDimensionIndex("face");
    options.depth = parseDimensionIndex("depth");
    options.raw = args["raw"].as<bool>();

    if (args["all"].as<bool>()) {
        if (options.level)
            fatal(RETURN_CODE_INVALID_ARGUMENTS, "Conflicting arguments 'level' and 'all'. Level cannot be specified if the 'all' flag is set");
        if (options.layer)
            fatal(RETURN_CODE_INVALID_ARGUMENTS, "Conflicting arguments 'layer' and 'all'. Layer cannot be specified if the 'all' flag is set");
        if (options.face)
            fatal(RETURN_CODE_INVALID_ARGUMENTS, "Conflicting arguments 'face' and 'all'. Face cannot be specified if the 'all' flag is set");
        if (options.depth)
            fatal(RETURN_CODE_INVALID_ARGUMENTS, "Conflicting arguments 'depth' and 'all'. Depth cannot be specified if the 'all' flag is set");

        options.level = all;
        options.layer = all;
        options.face = all;
        options.depth = all;
    }
}

std::optional<khr_df_model_channels_e> CommandExtract::getChannelType(const KTXTexture2& texture, uint32_t index) {
    const auto* bdfd = (texture->pDfd + 1);

    if (KHR_DFDSAMPLECOUNT(bdfd) <= index)
        return std::nullopt;

    return khr_df_model_channels_e(KHR_DFDSVAL(bdfd, index, CHANNELID));
}

TranscodeTarget CommandExtract::detectTranscodeTarget(const KTXTexture2& texture) {
    switch (texture->supercompressionScheme) {
    case KTX_SS_ZLIB: [[fallthrough]];
    case KTX_SS_ZSTD: [[fallthrough]];
    case KTX_SS_BASIS_LZ: [[fallthrough]];
    case KTX_SS_NONE:
        break;
    default:
        // If the supercompression is unknown, generate an error and exit.
        fatal(RETURN_CODE_INVALID_FILE, "Unknown supercompression scheme {}", toString(texture->supercompressionScheme));
    }

    const auto* bdfd = (texture->pDfd + 1);
    const auto sample0 = getChannelType(texture, 0);
    const auto sample1 = getChannelType(texture, 1);

    if (texture->supercompressionScheme == KTX_SS_BASIS_LZ) {
        if (sample0 == KHR_DF_CHANNEL_ETC1S_RGB && sample1 == KHR_DF_CHANNEL_ETC1S_AAA)
            return TranscodeTarget::rgba8;
        else if (sample0 == KHR_DF_CHANNEL_ETC1S_RGB)
            return TranscodeTarget::rgb8;
        else if (sample0 == KHR_DF_CHANNEL_ETC1S_RRR && sample1 == KHR_DF_CHANNEL_ETC1S_GGG)
            return TranscodeTarget::rg8;
        else if (sample0 == KHR_DF_CHANNEL_ETC1S_RRR)
            return TranscodeTarget::r8;
        else
            fatal(RETURN_CODE_INVALID_FILE, "Not supported channel types for Basis-LZ transcoding: {}, {}",
                    sample0 ? toString(KHR_DF_MODEL_ETC1S, *sample0) : "-",
                    sample1 ? toString(KHR_DF_MODEL_ETC1S, *sample1) : "-");
    } else if (khr_df_model_e(KHR_DFDVAL(bdfd, MODEL)) == KHR_DF_MODEL_UASTC) {
        // Sample 0 is always present for UASTC
        switch (*sample0) {
        case KHR_DF_CHANNEL_UASTC_RGBA:
            return TranscodeTarget::rgba8;
        case KHR_DF_CHANNEL_UASTC_RGB:
            return TranscodeTarget::rgb8;
        case KHR_DF_CHANNEL_UASTC_RG: [[fallthrough]];
        case KHR_DF_CHANNEL_UASTC_RRRG:
            return TranscodeTarget::rg8;
        case KHR_DF_CHANNEL_UASTC_RRR:
            return TranscodeTarget::r8;
        default:
            fatal(RETURN_CODE_INVALID_FILE, "Not supported channel type for UASTC transcoding: {}",
                    sample0 ? toString(KHR_DF_MODEL_UASTC, *sample0) : "-");
        }
    } else {
        // If neither the supercompression is BasisLZ, nor the DFD color model is UASTC,
        // generate an error and exit.
        fatal(RETURN_CODE_INVALID_FILE, "Requested transcoding but input file is neither BasisLZ, nor UASTC");
    }

    assert(false && "Internal error");
    return TranscodeTarget::rgba8;
}

void CommandExtract::executeExtract() {
    std::ifstream file(inputFile, std::ios::in | std::ios::binary);

    {
        // TODO Tools P5: Refactor and make a reusable tools validate and print function for this validation
        // TODO Tools P4: Skip validate create/transcode tests as the tools will create/transcode them anyways
        std::ostringstream messagesOS;
        const auto validationResult = validateIOStream(file, false, false, [&](const ValidationReport& issue) {
            fmt::print(messagesOS, "{}-{:04}: {}\n", toString(issue.type), issue.id, issue.message);
            fmt::print(messagesOS, "    {}\n", issue.details);
        });

        const auto validationMessages = std::move(messagesOS).str();
        if (!validationMessages.empty()) {
            fmt::print(std::cerr, "Validation {}\n", validationResult == 0 ? "successful" : "failed");
            fmt::print(std::cerr, "\n");
            fmt::print(std::cerr, "{}", validationMessages);
        }

        if (validationResult != 0)
            throw FatalError(RETURN_CODE_INVALID_FILE);

        file.seekg(0);
        if (!file)
            fatal(RETURN_CODE_IO_FAILURE, "Could not rewind the input file \"{}\": {}", inputFile, errnoMessage());
    }

    KTXTexture2 texture{nullptr};
    StreambufStream<std::streambuf*> ktx2Stream{file.rdbuf(), std::ios::in | std::ios::binary};
    auto ret = ktxTexture2_CreateFromStream(ktx2Stream.stream(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, texture.pHandle());
    if (ret != KTX_SUCCESS)
        fatal(RETURN_CODE_INVALID_FILE, "Failed to create KTX2 texture: {}", ktxErrorString(ret));

    // TODO Tools P4: Command line argument validation
    // if (options.level.value_or(0) > texture->numLevels)
    //     ■ If the "level" option is greater than the maximum level index, generate an error end
    //             exit.
    //     fatal(RETURN_CODE_INVALID_ARGUMENTS, )
    // if (options.layer && !texture->isArray)
    //     ■ If the texture type is not one of Array types and the "layer" option is set,
    //             generate an error and exit.
    // if (...)
    //     ■ If the texture type is one of Array types and the "layer" option is not set or
    //             is out of [0, layerCount) range, generate an error and exit.
    // if (...)
    //     ■ If the texture type is not Cubemap or Cubemap Array and the "face"
    //             option is set, generate an error and exit.
    // if (texture->isCubemap ...)
    //     ■ If the texture type is Cubemap or Cubemap Array and the "face" option is
    //             not set or out of [0-5] range, generate an error and exit.
    // if (...)
    //     ■ If the texture type is not 3D or 3D Array and the "depth" option is set,
    //             generate an error and exit.

    std::optional<Swizzle> swizzle;
    const auto isBasisLZ = texture->supercompressionScheme == KTX_SS_BASIS_LZ;

    if (options.transcode || isBasisLZ) {
        if (!ktxTexture2_NeedsTranscoding(texture))
            fatal(RETURN_CODE_INVALID_ARGUMENTS, "Cannot transcode the input file.");

        const auto sample0 = getChannelType(texture, 0);
        const auto sample1 = getChannelType(texture, 1);

        ktx_transcode_fmt_e targetFormat = KTX_TTF_RGBA32;
        TranscodeTarget target;

        if (options.transcode)
            target = *options.transcode;
        else
            target = detectTranscodeTarget(texture);

        switch (target) {
        case TranscodeTarget::etc_rgb:
            targetFormat = KTX_TTF_ETC1_RGB;
            break;
        case TranscodeTarget::etc_rgba:
            targetFormat = KTX_TTF_ETC2_RGBA;
            break;
        case TranscodeTarget::eac_r11:
            targetFormat = KTX_TTF_ETC2_EAC_R11;
            break;
        case TranscodeTarget::eac_rg11:
            targetFormat = KTX_TTF_ETC2_EAC_RG11;
            break;
        case TranscodeTarget::bc1:
            targetFormat = KTX_TTF_BC1_RGB;
            break;
        case TranscodeTarget::bc3:
            targetFormat = KTX_TTF_BC3_RGBA;
            break;
        case TranscodeTarget::bc4:
            targetFormat = KTX_TTF_BC4_R;
            break;
        case TranscodeTarget::bc5:
            targetFormat = KTX_TTF_BC5_RG;
            break;
        case TranscodeTarget::bc7:
            targetFormat = KTX_TTF_BC7_RGBA;
            break;
        case TranscodeTarget::astc:
            targetFormat = KTX_TTF_ASTC_4x4_RGBA;
            break;
        case TranscodeTarget::r8:
            targetFormat = KTX_TTF_RGBA32;
            swizzle = Swizzle{1, {Swizzle::red}};
            break;
        case TranscodeTarget::rg8:
            // BASIS_LZ will be decoded into RGBA based on their original slices requiring a swizzle:
            //         R    -> RRR1
            //         RG   -> RRRG
            //         RGB  -> RGB1
            //         RGBA -> RGBA
            targetFormat = KTX_TTF_RGBA32;
            if (isBasisLZ) {
                if (sample0 == KHR_DF_CHANNEL_ETC1S_RGB && sample1 == KHR_DF_CHANNEL_ETC1S_AAA)
                    swizzle = Swizzle{2, {Swizzle::red, Swizzle::green}};
                else if (sample0 == KHR_DF_CHANNEL_ETC1S_RGB)
                    swizzle = Swizzle{2, {Swizzle::red, Swizzle::green}};
                else if (sample0 == KHR_DF_CHANNEL_ETC1S_RRR && sample1 == KHR_DF_CHANNEL_ETC1S_GGG)
                    swizzle = Swizzle{2, {Swizzle::red, Swizzle::alpha}};
                else if (sample0 == KHR_DF_CHANNEL_ETC1S_RRR)
                    swizzle = Swizzle{2, {Swizzle::red, Swizzle::zero}};
                else
                    fatal(RETURN_CODE_INVALID_FILE, "Not supported channel types for Basis-LZ transcoding: {}, {}",
                            sample0 ? toString(KHR_DF_MODEL_ETC1S, *sample0) : "-",
                            sample1 ? toString(KHR_DF_MODEL_ETC1S, *sample1) : "-");
            } else { // UASTC
                if (sample0 == KHR_DF_CHANNEL_UASTC_RGBA) {
                    swizzle = Swizzle{2, {Swizzle::red, Swizzle::green}};
                } else if (sample0 == KHR_DF_CHANNEL_UASTC_RGB) {
                    swizzle = Swizzle{2, {Swizzle::red, Swizzle::green}};
                } else if (sample0 == KHR_DF_CHANNEL_UASTC_RRRG) {
                    swizzle = Swizzle{2, {Swizzle::red, Swizzle::alpha}};
                } else if (sample0 == KHR_DF_CHANNEL_UASTC_RG) {
                    swizzle = Swizzle{2, {Swizzle::red, Swizzle::green}};
                } else if (sample0 == KHR_DF_CHANNEL_UASTC_RRR) {
                    swizzle = Swizzle{2, {Swizzle::red, Swizzle::zero}};
                } else {
                    fatal(RETURN_CODE_INVALID_FILE, "Not supported channel type for UASTC transcoding: {}",
                            sample0 ? toString(KHR_DF_MODEL_UASTC, *sample0) : "-");
                }
            }
            break;
        case TranscodeTarget::rgb8:
            targetFormat = KTX_TTF_RGBA32;
            if (isBasisLZ) {
                if (sample0 == KHR_DF_CHANNEL_ETC1S_RGB && sample1 == KHR_DF_CHANNEL_ETC1S_AAA)
                    swizzle = Swizzle{3, {Swizzle::red, Swizzle::green, Swizzle::blue}};
                else if (sample0 == KHR_DF_CHANNEL_ETC1S_RGB)
                    swizzle = Swizzle{3, {Swizzle::red, Swizzle::green, Swizzle::blue}};
                else if (sample0 == KHR_DF_CHANNEL_ETC1S_RRR && sample1 == KHR_DF_CHANNEL_ETC1S_GGG)
                    swizzle = Swizzle{3, {Swizzle::red, Swizzle::alpha, Swizzle::zero}};
                else if (sample0 == KHR_DF_CHANNEL_ETC1S_RRR)
                    swizzle = Swizzle{3, {Swizzle::red, Swizzle::zero, Swizzle::zero}};
                else
                    fatal(RETURN_CODE_INVALID_FILE, "Not supported channel types for Basis-LZ transcoding: {}, {}",
                            sample0 ? toString(KHR_DF_MODEL_ETC1S, *sample0) : "-",
                            sample1 ? toString(KHR_DF_MODEL_ETC1S, *sample1) : "-");
            } else { // UASTC
                if (sample0 == KHR_DF_CHANNEL_UASTC_RGBA) {
                    swizzle = Swizzle{3, {Swizzle::red, Swizzle::green, Swizzle::blue}};
                } else if (sample0 == KHR_DF_CHANNEL_UASTC_RGB) {
                    swizzle = Swizzle{3, {Swizzle::red, Swizzle::green, Swizzle::blue}};
                } else if (sample0 == KHR_DF_CHANNEL_UASTC_RRRG) {
                    swizzle = Swizzle{3, {Swizzle::red, Swizzle::alpha, Swizzle::zero}};
                } else if (sample0 == KHR_DF_CHANNEL_UASTC_RG) {
                    swizzle = Swizzle{3, {Swizzle::red, Swizzle::green, Swizzle::zero}};
                } else if (sample0 == KHR_DF_CHANNEL_UASTC_RRR) {
                    swizzle = Swizzle{3, {Swizzle::red, Swizzle::zero, Swizzle::zero}};
                } else {
                    fatal(RETURN_CODE_INVALID_FILE, "Not supported channel type for UASTC transcoding: {}",
                            sample0 ? toString(KHR_DF_MODEL_UASTC, *sample0) : "-");
                }
            }
            break;
        case TranscodeTarget::rgba8:
            targetFormat = KTX_TTF_RGBA32;
            if (isBasisLZ) {
                if (sample0 == KHR_DF_CHANNEL_ETC1S_RGB && sample1 == KHR_DF_CHANNEL_ETC1S_AAA)
                    swizzle = Swizzle{4, {Swizzle::red, Swizzle::green, Swizzle::blue, Swizzle::alpha}};
                else if (sample0 == KHR_DF_CHANNEL_ETC1S_RGB)
                    swizzle = Swizzle{4, {Swizzle::red, Swizzle::green, Swizzle::blue, Swizzle::one}};
                else if (sample0 == KHR_DF_CHANNEL_ETC1S_RRR && sample1 == KHR_DF_CHANNEL_ETC1S_GGG)
                    swizzle = Swizzle{4, {Swizzle::red, Swizzle::alpha, Swizzle::zero, Swizzle::one}};
                else if (sample0 == KHR_DF_CHANNEL_ETC1S_RRR)
                    swizzle = Swizzle{4, {Swizzle::red, Swizzle::zero, Swizzle::zero, Swizzle::one}};
                else
                    fatal(RETURN_CODE_INVALID_FILE, "Not supported channel types for Basis-LZ transcoding: {}, {}",
                            sample0 ? toString(KHR_DF_MODEL_ETC1S, *sample0) : "-",
                            sample1 ? toString(KHR_DF_MODEL_ETC1S, *sample1) : "-");
            } else { // UASTC
                if (sample0 == KHR_DF_CHANNEL_UASTC_RGBA) {
                    swizzle = Swizzle{4, {Swizzle::red, Swizzle::green, Swizzle::blue, Swizzle::alpha}};
                } else if (sample0 == KHR_DF_CHANNEL_UASTC_RGB) {
                    swizzle = Swizzle{4, {Swizzle::red, Swizzle::green, Swizzle::blue, Swizzle::one}};
                } else if (sample0 == KHR_DF_CHANNEL_UASTC_RRRG) {
                    swizzle = Swizzle{4, {Swizzle::red, Swizzle::alpha, Swizzle::zero, Swizzle::one}};
                } else if (sample0 == KHR_DF_CHANNEL_UASTC_RG) {
                    swizzle = Swizzle{4, {Swizzle::red, Swizzle::green, Swizzle::zero, Swizzle::one}};
                } else if (sample0 == KHR_DF_CHANNEL_UASTC_RRR) {
                    swizzle = Swizzle{4, {Swizzle::red, Swizzle::zero, Swizzle::zero, Swizzle::one}};
                } else {
                    fatal(RETURN_CODE_INVALID_FILE, "Not supported channel type for UASTC transcoding: {}",
                            sample0 ? toString(KHR_DF_MODEL_UASTC, *sample0) : "-");
                }
            }
            break;
        }

        ktxTexture2_TranscodeBasis(texture, targetFormat, 0);
        if (ret != KTX_SUCCESS)
            fatal(RETURN_CODE_INVALID_FILE, "Failed to transcode KTX2 texture: {}", ktxErrorString(ret));
    }

    const auto isMultiOutput =
            options.level == all ||
            options.face == all ||
            options.layer == all ||
            options.depth == all;

    try {
        if (isMultiOutput) {
            if (std::filesystem::exists(options.outputPath) && !std::filesystem::is_directory(options.outputPath))
                fatal(RETURN_CODE_INVALID_ARGUMENTS, "Specified output path must be a directory for multi-output extract: \"{}\"", options.outputPath);
            std::filesystem::create_directories(options.outputPath);
        } else {
            std::filesystem::create_directories(std::filesystem::path(options.outputPath).parent_path());
        }
    } catch (const std::filesystem::filesystem_error& e) {
        fatal(RETURN_CODE_IO_FAILURE, "Failed to create the output directory \"{}\": {}", e.path1().generic_string(), e.what());
    }

    for (uint32_t levelIndex = 0; levelIndex < texture->numLevels; ++levelIndex) {
        if (options.level != all && options.level.value_or(0) != static_cast<int32_t>(levelIndex))
            continue; // Skip

        const auto imageSize = ktxTexture_GetImageSize(texture, levelIndex);
        // const auto imageWidth = std::max(1u, texture->baseWidth >> levelIndex);
        // const auto imageHeight = std::max(1u, texture->baseHeight >> levelIndex);
        const auto imageDepth = std::max(1u, texture->baseDepth >> levelIndex);

        if (options.depth != all && options.depth.value_or(0) >= static_cast<int32_t>(imageDepth))
            fatal(RETURN_CODE_INVALID_ARGUMENTS, "Requested depth slice {} on level {} is not available. The last depth slice on this level is {}.",
                    options.depth.value_or(0), levelIndex, imageDepth - 1);

        for (uint32_t faceIndex = 0; faceIndex < texture->numFaces; ++faceIndex) {
            if (options.face != all && options.face.value_or(0) != static_cast<int32_t>(faceIndex))
                continue; // Skip

            for (uint32_t layerIndex = 0; layerIndex < texture->numLayers; ++layerIndex) {
                if (options.layer != all && options.layer.value_or(0) != static_cast<int32_t>(layerIndex))
                    continue; // Skip

                ktx_size_t imageOffset;
                ktxTexture_GetImageOffset(texture, levelIndex, layerIndex, faceIndex, &imageOffset);
                const char* imageData = reinterpret_cast<const char*>(texture->pData) + imageOffset;

                if (!options.depth && options.raw) {
                    // If the texture type is 3D / 3D Array and the "depth" option is not set,
                    // the whole 3D block of pixel data is selected according to the "level" and "layer"
                    // option. This extraction path requires the "raw" option to be enabled.
                    if (isMultiOutput) {
                        const auto outputFilepath = fmt::format("{}/output{}{}{}{}.raw",
                                options.outputPath,
                                texture->numLevels > 1 ? fmt::format("_level{}", levelIndex) : "",
                                texture->isCubemap ? fmt::format("_face{}", faceIndex) : "",
                                texture->isArray ? fmt::format("_layer{}", layerIndex) : ""
                                // Depth is not part of the name as the whole 3D image is raw exported
                        );
                        writeRawFile(outputFilepath, imageData, imageSize, swizzle);
                    } else {
                        writeRawFile(options.outputPath, imageData, imageSize, swizzle);
                    }
                    continue;
                }

                // Iterate z_slice_of_blocks (this code currently assumes block z size is 1)
                // TODO Tools P5: 3D-Block Compressed formats are not supported
                for (uint32_t depthIndex = 0; depthIndex < imageDepth; ++depthIndex) {
                    if (options.depth != all && static_cast<uint32_t>(options.depth.value_or(0)) != depthIndex)
                        continue; // Skip

                    const auto depthSliceSize = imageSize / imageDepth;
                    const auto depthSliceData = imageData + depthSliceSize * depthIndex;

                    std::string outputFilepath;
                    if (isMultiOutput) {
                        outputFilepath = fmt::format("{}/output{}{}{}{}.raw",
                                options.outputPath,
                                texture->numLevels > 1 ? fmt::format("_level{}", levelIndex) : "",
                                texture->isCubemap ? fmt::format("_face{}", faceIndex) : "",
                                texture->isArray ? fmt::format("_layer{}", layerIndex) : "",
                                texture->baseDepth > 1 ? fmt::format("_depth{}", depthIndex) : ""
                        );
                    } else {
                        outputFilepath = options.outputPath;
                    }

                    if (options.raw) {
                        // If the "raw" option is set, return the original data without any headers.
                        writeRawFile(outputFilepath, depthSliceData, depthSliceSize, swizzle);
                    } else {
                        // TODO Tools P3: Upload to imageio, save with imageio, Use depthSliceData and depthSliceSize
                        // ○ If the "Raw Formats Conversion" table has an entry for the used KTX format,
                        //         return a PNG or EXR file converted using the supplied rules; otherwise generate
                        //         an error and exit
                    }
                }
            }
        }
    }

    if (ret != KTX_SUCCESS)
        fatal(RETURN_CODE_INVALID_FILE, "Failed to iterate KTX2 texture: {}", ktxErrorString(ret));
}

void CommandExtract::writeRawFile(const std::string& filepath, const char* data, std::size_t size, const std::optional<Swizzle>& swizzle) {
    std::ofstream file(filepath, std::ios::out | std::ios::binary);
    if (!file)
        fatal(RETURN_CODE_IO_FAILURE, "Failed to open output file \"{}\": {}", filepath, errnoMessage());

    if (!swizzle) {
        file.write(data, size);
    } else {
        // TODO Tools P5: Extend swizzle support for non-R8G8B8A8 formats (will be necessary for png/exr export)
        // Swizzle on raw data is currently only supported and used for R8G8B8A8 format
        // as only transcode to r8, rg8, rgb8 and rgba8 could request it
        const auto end = data + size;
        while (data + swizzle->numChannels <= end) {
            char pixel[4];
            for (int i = 0; i < swizzle->numChannels; ++i)
                switch (swizzle->channels[i]) {
                case Swizzle::red:
                    pixel[i] = *(data + 0);
                    break;
                case Swizzle::green:
                    pixel[i] = *(data + 1);
                    break;
                case Swizzle::blue:
                    pixel[i] = *(data + 2);
                    break;
                case Swizzle::alpha:
                    pixel[i] = *(data + 3);
                    break;
                case Swizzle::zero:
                    pixel[i] = static_cast<char>(0x00);
                    break;
                case Swizzle::one:
                    pixel[i] = static_cast<char>(0xFF);
                    break;
                }
            file.write(pixel, swizzle->numChannels);
            data += 4;
        }
    }

    if (!file)
        fatal(RETURN_CODE_IO_FAILURE, "Failed to write output file \"{}\": {}", filepath, errnoMessage());
}

} // namespace ktx

KTX_COMMAND_ENTRY_POINT(ktxExtract, ktx::CommandExtract)

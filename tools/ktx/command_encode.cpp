// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include "command.h"
#include "compress_utils.h"
#include "encode_utils.h"
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

// -------------------------------------------------------------------------------------------------

/** @page ktxtools_encode ktx encode
@~English

Encodes a KTX2 file.

@warning TODO Tools P5: This page is incomplete

@section ktxtools_encode_synopsis SYNOPSIS
    ktx encode [options] @e input_file

@section ktxtools_encode_description DESCRIPTION

    The following options are available:
    <dl>
        <dt>-f, --flag</dt>
        <dd>Flag description</dd>
    </dl>
    @snippet{doc} ktx/command.h command options

@section ktxtools_encode_exitstatus EXIT STATUS
    @b ktx @b encode exits
        0 - Success
        1 - Command line error
        2 - IO error
        3 - Invalid input or state

@section ktxtools_encode_history HISTORY

@par Version 4.0
 - Initial version

@section ktxtools_encode_author AUTHOR
    - Mátyás Császár [Vader], RasterGrid www.rastergrid.com
    - Daniel Rákos, RasterGrid www.rastergrid.com
*/
class CommandEncode : public Command {
    enum {
        all = -1,
    };

    struct OptionsEncode {
        void init(cxxopts::Options& opts);
        void process(cxxopts::Options& opts, cxxopts::ParseResult& args, Reporter& report);
    };

    Combine<OptionsEncode, OptionsCodec<true>, OptionsCompress, OptionsSingleInSingleOut, OptionsGeneric> options;

public:
    virtual int main(int argc, _TCHAR* argv[]) override;
    virtual void initOptions(cxxopts::Options& opts) override;
    virtual void processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) override;

private:
    void executeEncode();
};

// -------------------------------------------------------------------------------------------------

int CommandEncode::main(int argc, _TCHAR* argv[]) {
    try {
        parseCommandLine("ktx encode", "Encode a KTX2 file.", argc, argv);
        executeEncode();
        return RETURN_CODE_SUCCESS;
    } catch (const FatalError& error) {
        return error.return_code;
    } catch (const std::exception& e) {
        fmt::print(std::cerr, "{} fatal: {}\n", commandName, e.what());
        return RETURN_CODE_RUNTIME_ERROR;
    }
}

void CommandEncode::OptionsEncode::init(cxxopts::Options& opts) {
    opts.add_options()
        ("codec", "Target codec.\n"
                  "Possible options are: basis-lz | uastc", cxxopts::value<std::string>(), "<target>");
}

void CommandEncode::OptionsEncode::process(cxxopts::Options&, cxxopts::ParseResult&, Reporter&) {
}

void CommandEncode::initOptions(cxxopts::Options& opts) {
    options.init(opts);
}

void CommandEncode::processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) {
    options.process(opts, args, *this);

    if (options.codec == EncodeCodec::BasisLZ) {
        if (options.zstd.has_value())
            fatal_usage("Cannot encode to BasisLZ and supercompress with Zstd.");

        if (options.zlib.has_value())
            fatal_usage("Cannot encode to BasisLZ and supercompress with ZLIB.");
    }
}

void CommandEncode::executeEncode() {
    std::ifstream file(options.inputFilepath, std::ios::in | std::ios::binary);
    validateToolInput(file, options.inputFilepath, *this);

    KTXTexture2 texture{nullptr};
    StreambufStream<std::streambuf*> ktx2Stream{file.rdbuf(), std::ios::in | std::ios::binary};
    auto ret = ktxTexture2_CreateFromStream(ktx2Stream.stream(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, texture.pHandle());
    if (ret != KTX_SUCCESS)
        fatal(rc::INVALID_FILE, "Failed to create KTX2 texture: {}", ktxErrorString(ret));

    if (texture->supercompressionScheme != KTX_SS_NONE)
        fatal(rc::INVALID_FILE, "Cannot encode KTX2 file with {} supercompression.",
            toString(ktxSupercmpScheme(texture->supercompressionScheme)));

    switch (texture->vkFormat) {
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R8_SRGB:
    case VK_FORMAT_R8G8_UNORM:
    case VK_FORMAT_R8G8_SRGB:
    case VK_FORMAT_R8G8B8_UNORM:
    case VK_FORMAT_R8G8B8_SRGB:
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SRGB:
        // Allowed formats
        break;
    default:
        fatal_usage("Only R8, RG8, RGB8, or RGBA8 UNORM and SRGB formats can be encoded, "
            "but format is {}.", toString(VkFormat(texture->vkFormat)));
        break;
    }

    // Modify KTXwriter metadata
    const auto writer = fmt::format("{} {}", commandName, version(options.testrun));
    ktxHashList_DeleteKVPair(&texture->kvDataHead, KTX_WRITER_KEY);
    ktxHashList_AddKVPair(&texture->kvDataHead, KTX_WRITER_KEY,
            static_cast<uint32_t>(writer.size() + 1), // +1 to include the \0
            writer.c_str());

    ktx_uint32_t oetf = ktxTexture2_GetOETF(texture);
    if (options.basisOpts.normalMap && oetf != KHR_DF_TRANSFER_LINEAR)
        fatal(rc::INVALID_FILE,
            "--normal-mode specified but the input file uses non-linear transfer function {}.",
            toString(khr_df_transfer_e(oetf)));

    ret = ktxTexture2_CompressBasisEx(texture, &options.basisOpts);
    if (ret != KTX_SUCCESS)
        fatal(rc::IO_FAILURE, "Failed to encode KTX2 file with codec \"{}\". KTX Error: {}", ktxErrorString(ret));

    if (options.zstd) {
        ret = ktxTexture2_DeflateZstd((ktxTexture2*)texture, *options.zstd);
        if (ret != KTX_SUCCESS)
            fatal(rc::IO_FAILURE, "Zstd deflation failed. KTX Error: {}", ktxErrorString(ret));
    }

    if (options.zlib) {
        ret = ktxTexture2_DeflateZLIB((ktxTexture2*)texture, *options.zlib);
        if (ret != KTX_SUCCESS)
            fatal(rc::IO_FAILURE, "ZLIB deflation failed. KTX Error: {}", ktxErrorString(ret));
    }

    // Save output file
    if (std::filesystem::path(options.outputFilepath).has_parent_path())
        std::filesystem::create_directories(std::filesystem::path(options.outputFilepath).parent_path());
    FILE* f = _tfopen(options.outputFilepath.c_str(), "wb");
    if (!f)
        fatal(2, "Could not open output file \"{}\": ", options.outputFilepath, errnoMessage());

    ret = ktxTexture_WriteToStdioStream(texture, f);
    fclose(f);

    if (KTX_SUCCESS != ret) {
        if (f != stdout)
            std::filesystem::remove(options.outputFilepath);
        fatal(rc::IO_FAILURE, "Failed to write KTX file \"{}\": KTX error: {}",
            options.outputFilepath, ktxErrorString(ret));
    }
}

} // namespace ktx

KTX_COMMAND_ENTRY_POINT(ktxEncode, ktx::CommandEncode)

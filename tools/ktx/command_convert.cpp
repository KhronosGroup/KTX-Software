// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#include "formats.h"
#include "ktx.h"
#include "ktxint.h"
#include "command.h"
#include "platform_utils.h"
#include "sbufstream.h"
#include "validate.h"

// #define TINYDDS_IMPLEMENTATION
// #define BASISU_NOTE_UNUSED(x) (void)(x)
#include "basis_universal/encoder/3rdparty/tinydds.h"

#include <exception>
#include <filesystem>
#include <regex>
#include <cxxopts.hpp>
#include <fmt/ostream.h>
#include <fmt/printf.h>

extern "C" const char* glInternalformatString(GLenum);

// -------------------------------------------------------------------------------------------------

namespace ktx {

// -------------------------------------------------------------------------------------------------

/** @page ktx_convert ktx convert
@~English

Convert another texture file type to a KTX2 file

@section ktx\_convert\_synopsis SYNOPSIS
    ktx convert [option...] @e input-file [@e output-path]

@section ktx\_convert\_description DESCRIPTION
    @b ktx @b convert converts the file specified as the @e input-file argument
    to KTX v2 and saves it in the @e output-path. If the @e input-file is '-'
    the file will be read from the stdin. If the @e output-path is '-' the output
    file will be written to the stdout. If @e output-path is a directory, the
    output is saved to a file in that directory whose name is the stem of
    the value given in @e input-file with the extension  @c ktx2. If no @e output-path
    is provided, the output is saved to a file whose name is the parent path and stem of
    the value given in  @e input-file with the extension @c ktx2.

    The input file must be of a supported file type. Currently the only supported
    file types are KTX v1 and DDS. Generates an error if the input file type is
    unrecognized.

    To encode or supercompress the converted file, pipe it to @b ktx @b encode or
    @b ktx @b deflate via stdout.

    Unrecognized metadata with keys beginning "KTX" or "ktx" found in an input
    KTX v1 file, is dropped and a warning is generated.

    @note When converting a KTX v1 file, payloads with format
    @c GL_COMPRESSED_RGBA_ASTC_\*_KHR are mapped to the equivalent
    @c VK_FORMAT_ASTC_\*_SFLOAT_BLOCK. In order to display images from the
    converted file, applications using Vulkan must therefore enable the
    @c VK_EXT_texture_compression_astc_hdr extension and its
    @c textureCompressionASTC_HDR feature. Using these formats, Vulkan
    implementations will render both HDR and LDR blocks within the images. With
    the alternative mapping to @c VK_FORMAT_ASTC_\*_UNORM_BLOCK they will render
    HDR blocks in the error color.

    @note When converting a DDS file, the conversion is lossless (no
    decoding-encoding cycle is performed and bits are just copied as they are).
    Not all formats supported by DDS are currently supported. Currently
    supported formats include all BC1-BC7 GPU block compressed formats, all LDR
    uncompressed formats, and all HDR uncompressed formats.

@section ktx\_convert\_options OPTIONS
    The following options are available:
    <dl>
        <dt>-t, \--input-type &lt;type&gt;</dt>
        <dd>Type of input file. Currently @b type must be either @c ktx or @c dds. Case insensitive.</dd>
        <dt>-d, \--drop-bad-orientation</dt>
        <dd>Some in-the-wild KTX v1 files have orientation metadata with the key
            "KTXOrientation" instead of KTXorientaion. By default such metadata is
            rewritten with the correct name. This option causes such bad metadata
            to be dropped. Ignored unless @b type is @c ktx.
        </dd>
    </dl>
    @snippet{doc} ktx/command.h command options_generic

@section ktx_convert_exitstatus EXIT STATUS
    @snippet{doc} ktx/command.h command exitstatus

@section ktx_convert_history HISTORY

@par Version 4.0
 - Initial version

@section ktx_convert_author AUTHOR
    - Mark Callow
*/

class OutputStreamEx : public OutputStream {
public:
    OutputStreamEx(const std::string& filepath, Reporter& report)
        : OutputStream(filepath, report) { }
#if defined(__cpp_lib_char8_t)
    OutputStreamEx(const std::u8string& filepath, Reporter& report)
        : OutputStream(filepath, report) { }
#endif

    /// Writes input KTX1 texture as KTX2 texture. Calls `ktxTexture1_WriteKTX2ToStream` on the input KTX1 texture
    void writeKTX2(ktxTexture1* texture, Reporter& report) {
        StreambufStream<std::streambuf*> stream(activeStream->rdbuf(), std::ios::out | std::ios::binary);
        const auto ret = ktxTexture1_WriteKTX2ToStream(texture, stream.stream());
        if (KTX_SUCCESS != ret) {
            if (!isStdout())
                std::filesystem::remove(DecodeUTF8Path(filepath).c_str());
            report.fatal(rc::IO_FAILURE, "Failed to write KTX file \"{}\": KTX error: {}.",
                         filepath, ktxErrorString(ret));
        }
    }

    /// Writes input KTX2 texture. Simply calls `ktxTexture2_WriteToStream` on the input KTX2 texture
    void writeKTX2(ktxTexture2* texture, Reporter& report) {
        StreambufStream<std::streambuf*> stream(activeStream->rdbuf(), std::ios::out | std::ios::binary);
        const auto ret = ktxTexture2_WriteToStream(texture, stream.stream());
        if (KTX_SUCCESS != ret) {
            if (!isStdout())
                std::filesystem::remove(DecodeUTF8Path(filepath).c_str());
            report.fatal(rc::IO_FAILURE, "Failed to write KTX2 file \"{}\": KTX error: {}.",
                         filepath, ktxErrorString(ret));
        }
    }
};

class CommandConvert : public Command {
    enum class input_type_e { ktx, dds };

    struct OptionsConvert {
        inline static const char* kDropBadOrientation = "drop-bad-orientation";
        inline static const char* kInputType = "input-type";

        bool dropBadOrientation = false;
        std::optional<input_type_e> inputType;

        void init(cxxopts::Options& opts) {
            const std::string kDropBadOrientationFlags = std::string("d,") + kDropBadOrientation;
            const std::string kInputTypeFlags = std::string("t,") + kInputType;
            opts.add_options()
                (kInputTypeFlags, "Specify the type of input file. Currently must be ktx or dds.",
                  cxxopts::value<std::string>(), "<type>")
                (kDropBadOrientationFlags, "Drop bad orientation metadata, such as \"KTXOrientation\","
                    " instead of fixing it.");
        }

        std::optional<input_type_e> parseInputType(cxxopts::ParseResult& args, const char* argName, Reporter& report) const {
            static const std::unordered_map<std::string, input_type_e> values{
                {"KTX", input_type_e::ktx},
                {"DDS", input_type_e::dds},
            };
            std::optional<input_type_e> result = {};
            if (args[argName].count()) {
                const std::string typeStr = to_upper_copy(args[argName].as<std::string>());
                const auto it = values.find(typeStr);
                if (it != values.end()) {
                    result = it->second;
                } else {
                    report.fatal_usage("Invalid or unsupported type specified as --{} argument: \"{}\".",
                                       argName, args[argName].as<std::string>());
                }
            }
            return result;
        }

        void process(cxxopts::Options&, cxxopts::ParseResult& args, Reporter& report) {
            inputType = parseInputType(args, kInputType, report);
            if (!inputType.has_value())
                report.fatal_usage("--{} <type> must be specified", kInputType);

            dropBadOrientation = args[kDropBadOrientation].as<bool>();
        }
    };

    Combine<OptionsConvert, OptionsSingleInSingleOut<true>, OptionsGeneric> options;

public:
    virtual int main(int argc, char* argv[]) override;
    virtual void initOptions(cxxopts::Options& opts) override;
    virtual void processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) override;

private:
    void convertKtx(InputStream&, OutputStreamEx&);
    void convertDDS(InputStream&, OutputStreamEx&);
    void executeConvert();

    static void tinydds_error(void* user, char const* msg);
    static void* tinydds_alloc(void* user, size_t size);
    static void tinydds_free(void* user, void* memory);
    static size_t tinydds_read(void* user, void* buffer, size_t byteCount);
    static bool tinydds_seek(void* user, int64_t offset);
    static int64_t tinydds_tell(void* user);
};

// -------------------------------------------------------------------------------------------------

int CommandConvert::main(int argc, char* argv[]) {
    try {
        parseCommandLine("ktx convert",
                "Convert the non-KTX2 texture file specified as the input-file argument,\n"
                "    optionally supercompress the result, and save it as the output-file.",
                argc, argv);
        executeConvert();
        return +rc::SUCCESS;
    } catch (const FatalError& error) {
        return +error.returnCode;
    } catch (const std::exception& e) {
        fmt::print(std::cerr, "{} fatal: {}\n", commandName, e.what());
        return +rc::RUNTIME_ERROR;
    }
}

void CommandConvert::initOptions(cxxopts::Options& opts) {
    options.init(opts);
}

void CommandConvert::processOptions(cxxopts::Options& opts, cxxopts::ParseResult& args) {
    options.process(opts, args, *this);
}

void CommandConvert::executeConvert() {
    InputStream inputStream(options.inputFilepath, *this);

    // In c++20 options.{input,output}Filepath should be changed to u8string
    // and u8path() replaced with path.
#if defined(__cpp_lib_char8_t)
   auto outputFilepath = std::filesystem::path(to_u8string(options.outputFilepath));
#else
    auto outputFilepath = std::filesystem::u8path(options.outputFilepath);
#endif
    bool usingInputName = false;
    // If no output path given or output is a directory, use input path/filename
    // changing extension to or adding ".ktx2".
    if (outputFilepath.empty()) {
        outputFilepath = options.inputFilepath;
        usingInputName = true;
    } else if (!outputFilepath.has_filename() || std::filesystem::is_directory(outputFilepath)) {
        // is_directory() above handles case where outputFilepath is not '/' terminated but
        // exists and is a directory.
#if defined(__cpp_lib_char8_t)
        auto inputFilepath = std::filesystem::path(to_u8string(options.inputFilepath));
#else
        auto inputFilepath = std::filesystem::u8path(options.inputFilepath);
#endif
        outputFilepath /= inputFilepath.filename();
        usingInputName = true;
    }
    if (usingInputName) {
        outputFilepath.replace_extension("ktx2");
    }

    // Create or open output file
    if (outputFilepath.has_parent_path()) {
        std::filesystem::create_directories(outputFilepath.parent_path());
    }
    OutputStreamEx outputStream(outputFilepath.u8string(), *this);

    try {
        if (options.inputType == input_type_e::ktx)
            convertKtx(inputStream, outputStream);
        else if (options.inputType == input_type_e::dds)
            convertDDS(inputStream, outputStream);
    } catch (const FatalError& error) {
        outputStream.removeOnDestruct();
        throw error;
    }


    if (!outputStream.isStdout()) {
        outputStream.flush();
        std::ostringstream messagesOS;
        InputStream converted(outputFilepath.u8string(), *this);
        const auto validationResult = validateIOStream(converted,
            fmtInFile(outputFilepath.u8string()), false, false, [&](const ValidationReport& issue) {
            fmt::print(messagesOS, "{}-{:04}: {}\n", toString(issue.type), issue.id, issue.message);
            fmt::print(messagesOS, "    {}\n", issue.details);
        });

        if (validationResult) {
            fatal(ReturnCode(validationResult),
                  "Validation of converted file \"{}\" failed. This is likely due to an internal"
                  " issue in the tool. If, after looking at the validation messages below,"
                  " you agree, please open an issue at"
                  " https://github.com/KhronosGroup/KTX-Software/issues. The file has not been"
                  " deleted so you can attach it to the issue.\n\n{}",
                  outputStream.str(), messagesOS.str());
        }
    }
}

void CommandConvert::convertKtx(InputStream& inputStream, OutputStreamEx& outputStream) {
    std::unique_ptr<ktxTexture1, decltype(ktxTexture1_Destroy)*> texture_raii{nullptr,
                                                                          ktxTexture1_Destroy};
    ktxTexture1* texture = nullptr;
    StreambufStream<std::streambuf*> ktxStream{inputStream->rdbuf(), std::ios::in | std::ios::binary};
    auto ret = ktxTexture1_CreateFromStream(ktxStream.stream(),
                                            KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);
    texture_raii.reset(texture);
    if (ret != KTX_SUCCESS) {
        if (ret == KTX_UNSUPPORTED_TEXTURE_TYPE) {
            inputStream->seekg(0);
            KTX_header header;
            inputStream->read(reinterpret_cast<char*>(&header), KTX_HEADER_SIZE);
            fatal(rc::NOT_SUPPORTED,
                  "Format of input file, {}, is unsupported or has no equivalent VkFormat.",
                  ::glInternalformatString(header.glInternalformat));
        } else {
            fatal(rc::INVALID_FILE, "Failed to create KTX texture: {}", ktxErrorString(ret));
        }
    }

    // Some in-the-wild KTX files have incorrect KTXOrientation
    // Warn about dropping invalid metadata.
    ktxHashListEntry* pEntry;
    for (pEntry = texture->kvDataHead;
         pEntry != NULL;
         pEntry = ktxHashList_Next(pEntry)) {
        unsigned int keyLen;
        char* rawKey;

        ktxHashListEntry_GetKey(pEntry, &keyLen, &rawKey);
        auto key = std::string_view(rawKey);
        std::regex re("ktx|KTX");
        std::smatch ktxMatch;
        if (std::regex_search(static_cast<std::string>(key), re)) {
            if (key.compare(KTX_ORIENTATION_KEY)
                && key.compare(KTX_WRITER_KEY)) {
                if (key.compare("KTXOrientation") == 0
                    && !options.dropBadOrientation) {
                        unsigned int orientLen;
                        char* orientation;
                        ktxHashListEntry_GetValue(pEntry,
                                            &orientLen,
                                            (void**)&orientation);
                        ktxHashList_AddKVPair(&texture->kvDataHead,
                                              KTX_ORIENTATION_KEY,
                                              orientLen,
                                              orientation);
               } else {
                   warning("Dropping unrecognized KTX metadata \"{}\"", key);
               }
               auto next_entry = ktxHashList_Next(pEntry);
               ktxHashList_DeleteEntry(&texture->kvDataHead,
                                       pEntry);
               pEntry = next_entry;
            }
        }
    }

    // Add required writer metadata.
    const auto writer = fmt::format("{} {}", commandName, version(options.testrun));
    ktxHashList_AddKVPair(&texture->kvDataHead, KTX_WRITER_KEY,
                         static_cast<uint32_t>(writer.size() + 1),
                         writer.c_str());

    outputStream.writeKTX2(texture, *this);
}

inline VkFormat dds_to_vkformat(TinyDDS_Format dds_format) {
    // clang-format off
  switch (dds_format) {
        /* LDR uncompressed formats */
    case TDDS_UNDEFINED: return VK_FORMAT_UNDEFINED;
    case TDDS_R8_UNORM: return VK_FORMAT_R8_UNORM;
    case TDDS_R8_SNORM: return VK_FORMAT_R8_SNORM;
    case TDDS_R8_UINT: return VK_FORMAT_R8_UINT;
    case TDDS_R8_SINT: return VK_FORMAT_R8_SINT;
    case TDDS_R8G8_UNORM: return VK_FORMAT_R8G8_UNORM;
    case TDDS_R8G8_SNORM: return VK_FORMAT_R8G8_SNORM;
    case TDDS_R8G8_UINT: return VK_FORMAT_R8G8_UINT;
    case TDDS_R8G8_SINT: return VK_FORMAT_R8G8_SINT;
    case TDDS_R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
    case TDDS_R8G8B8A8_SNORM: return VK_FORMAT_R8G8B8A8_SNORM;
    case TDDS_R8G8B8A8_UINT: return VK_FORMAT_R8G8B8A8_UINT;
    case TDDS_R8G8B8A8_SINT: return VK_FORMAT_R8G8B8A8_SINT;
    case TDDS_R8G8B8A8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
    case TDDS_B8G8R8A8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
    case TDDS_B8G8R8A8_SRGB: return VK_FORMAT_B8G8R8A8_SRGB;
	// TDDS_R9G9B9E5_UFLOAT = TIF_DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
	// TDDS_R10G10B10A2_UNORM = TIF_DXGI_FORMAT_R10G10B10A2_UNORM,
	// TDDS_R10G10B10A2_UINT = TIF_DXGI_FORMAT_R10G10B10A2_UINT,
	// TDDS_R11G11B10_UFLOAT = TIF_DXGI_FORMAT_R11G11B10_FLOAT,
	// TDDS_R16_UNORM = TIF_DXGI_FORMAT_R16_UNORM,
	// TDDS_R16_SNORM = TIF_DXGI_FORMAT_R16_SNORM,
	// TDDS_R16_UINT = TIF_DXGI_FORMAT_R16_UINT,
	// TDDS_R16_SINT = TIF_DXGI_FORMAT_R16_SINT,
	// TDDS_R16_SFLOAT = TIF_DXGI_FORMAT_R16_FLOAT,

	// TDDS_R16G16_UNORM = TIF_DXGI_FORMAT_R16G16_UNORM,
	// TDDS_R16G16_SNORM = TIF_DXGI_FORMAT_R16G16_SNORM,
	// TDDS_R16G16_UINT = TIF_DXGI_FORMAT_R16G16_UINT,
	// TDDS_R16G16_SINT = TIF_DXGI_FORMAT_R16G16_SINT,
	// TDDS_R16G16_SFLOAT = TIF_DXGI_FORMAT_R16G16_FLOAT,

	// TDDS_R16G16B16A16_UNORM = TIF_DXGI_FORMAT_R16G16B16A16_UNORM,
	// TDDS_R16G16B16A16_SNORM = TIF_DXGI_FORMAT_R16G16B16A16_SNORM,
	// TDDS_R16G16B16A16_UINT = TIF_DXGI_FORMAT_R16G16B16A16_UINT,
	// TDDS_R16G16B16A16_SINT = TIF_DXGI_FORMAT_R16G16B16A16_SINT,
	// TDDS_R16G16B16A16_SFLOAT = TIF_DXGI_FORMAT_R16G16B16A16_FLOAT,

         /* HDR uncompressed formats */
    case TDDS_R32_UINT: return VK_FORMAT_R32_UINT;
    case TDDS_R32_SINT: return VK_FORMAT_R32_SINT;
    case TDDS_R32_SFLOAT: return VK_FORMAT_R32_SFLOAT;
    case TDDS_R32G32_UINT: return VK_FORMAT_R32G32_UINT;
    case TDDS_R32G32_SINT: return VK_FORMAT_R32G32_SINT;
    case TDDS_R32G32_SFLOAT: return VK_FORMAT_R32G32_SFLOAT;
    case TDDS_R32G32B32_UINT: return VK_FORMAT_R32G32B32_UINT;
    case TDDS_R32G32B32_SINT: return VK_FORMAT_R32G32B32_SINT;
    case TDDS_R32G32B32_SFLOAT: return VK_FORMAT_R32G32B32_SFLOAT;
    case TDDS_R32G32B32A32_UINT: return VK_FORMAT_R32G32B32A32_UINT;
    case TDDS_R32G32B32A32_SINT: return VK_FORMAT_R32G32B32A32_SINT;
    case TDDS_R32G32B32A32_SFLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;

        /* compressed formats */
    case TDDS_BC1_RGBA_UNORM_BLOCK: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
    case TDDS_BC1_RGBA_SRGB_BLOCK: return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
    case TDDS_BC2_UNORM_BLOCK: return VK_FORMAT_BC2_UNORM_BLOCK;
    case TDDS_BC2_SRGB_BLOCK: return VK_FORMAT_BC2_SRGB_BLOCK;
    case TDDS_BC3_UNORM_BLOCK: return VK_FORMAT_BC3_UNORM_BLOCK;
    case TDDS_BC3_SRGB_BLOCK: return VK_FORMAT_BC3_SRGB_BLOCK;
    case TDDS_BC4_UNORM_BLOCK: return VK_FORMAT_BC4_UNORM_BLOCK;
    case TDDS_BC4_SNORM_BLOCK: return VK_FORMAT_BC4_SNORM_BLOCK;
    case TDDS_BC5_UNORM_BLOCK: return VK_FORMAT_BC5_UNORM_BLOCK;
    case TDDS_BC5_SNORM_BLOCK: return VK_FORMAT_BC5_SNORM_BLOCK;
    case TDDS_BC6H_UFLOAT_BLOCK: return VK_FORMAT_BC6H_UFLOAT_BLOCK;
    case TDDS_BC6H_SFLOAT_BLOCK: return VK_FORMAT_BC6H_SFLOAT_BLOCK;
    case TDDS_BC7_UNORM_BLOCK: return VK_FORMAT_BC7_UNORM_BLOCK;
    case TDDS_BC7_SRGB_BLOCK: return VK_FORMAT_BC7_SRGB_BLOCK;

    // TODO: I have no idea what other formats even mean let alone what they translate to in Vulkan...
    //       Most are probably legacy formats, but, again, I have no idea how to handle them.
    default: return VK_FORMAT_UNDEFINED;
  }
  // clang-format off
}

struct TinyDDS_CustomData {
    CommandConvert* parent;
    ktxStream* str;
};

void TinyDDS_Deleter(TinyDDS_ContextHandle* handle) { TinyDDS_DestroyContext(*handle); }

void CommandConvert::convertDDS(InputStream& inputStream, OutputStreamEx& outputStream) {
    std::unique_ptr<TinyDDS_ContextHandle, decltype(TinyDDS_Deleter)*> dds_raii{nullptr,
                                                                                TinyDDS_Deleter};
    std::unique_ptr<ktxTexture2, decltype(ktxTexture2_Destroy)*> texture_raii{nullptr,
                                                                              ktxTexture2_Destroy};
    StreambufStream<std::streambuf*> ktxStream{inputStream->rdbuf(),
                                               std::ios::in | std::ios::binary};

    TinyDDS_Callbacks clbks;
    clbks.errorFn = tinydds_error;
    clbks.allocFn = tinydds_alloc;
    clbks.freeFn = tinydds_free;
    clbks.readFn = tinydds_read;
    clbks.seekFn = tinydds_seek;
    clbks.tellFn = tinydds_tell;

    // Set custom user data to be passed as pointer to TinyDDS clbks (see above)
    TinyDDS_CustomData user_data;
    user_data.parent = this;
    user_data.str = ktxStream.stream();

    // Create a TinyDDS context
    TinyDDS_ContextHandle dds_handle = TinyDDS_CreateContext(&clbks, &user_data);
    if (dds_handle == NULL)
        fatal(rc::RUNTIME_ERROR, "Failed to create TinyDDS context from input DDS stream");
    dds_raii.reset(&dds_handle);

    // Then read the header
    if (!TinyDDS_ReadHeader(dds_handle)) {
        fatal(rc::INVALID_FILE, "Failed to read DDS header");
    }

    // Get some specs that will be passed to ktxTexture2 creation struct
    uint32_t width, height, depth, slices;
    if (!TinyDDS_Dimensions(dds_handle, &width, &height, &depth, &slices))
        fatal(rc::INVALID_FILE, "Failed to retrieve texture dimensions from DDS input");

    // libktx expects dimensions to be >= 1 while TinyDDS can report, for instance, depth of 0
    width = std::max(width, 1u);
    height = std::max(height, 1u);
    depth = std::max(depth, 1u);

    uint32_t num_levels = TinyDDS_NumberOfMipmaps(dds_handle);

    // TODO: is TinyDDS up-to-date with latest DDS spec?
    //       see: https://github.com/microsoft/DirectXTex
    TinyDDS_Format dds_format = TinyDDS_GetFormat(dds_handle);
    if (dds_format == TDDS_UNDEFINED)
        fatal(rc::RUNTIME_ERROR, "Failed to retrieve DDS format (TDDS_UNDEFINED)");

    VkFormat vkformat = dds_to_vkformat(dds_format);
    if (vkformat == VK_FORMAT_UNDEFINED)
        fatal(rc::RUNTIME_ERROR, "Failed to retrieve Vulkan format from DDS format {}",
              static_cast<uint32_t>(dds_format));

    ktxTextureCreateInfo create_info;
    create_info.glInternalformat = 0;  // Ignored as this is not a KTX1 texture
    create_info.vkFormat = vkformat;
    create_info.pDfd = nullptr;
    create_info.baseWidth = width;
    create_info.baseHeight = height;
    create_info.baseDepth = depth;
    create_info.numDimensions = 2;
    create_info.numLevels = num_levels;
    create_info.numLayers = 1;                // TODO: dds arrays support
    create_info.numFaces = 1;                 // TODO: dds cubemaps support
    create_info.isArray = KTX_FALSE;          // TODO: dds arrays support
    create_info.generateMipmaps = KTX_FALSE;  // TODO: always false?

#if 0
    std::cout << "calling ktxTexture2_Create with: "
              << "vkFormat=" << create_info.vkFormat << "; "
              << "baseWidth=" << create_info.baseWidth << "; "
              << "baseHeight=" << create_info.baseHeight << "; "
              << "baseDepth=" << create_info.baseDepth << "; "
              << "numDimensions=" << create_info.numDimensions << "; "
              << "numLevels=" << create_info.numLevels << "; "
              << "numLayers=" << create_info.numLayers << "; "
              << "numFaces=" << create_info.numFaces << "; "
              << "isArray=" << create_info.isArray << "; "
              << "generateMipmaps=" << create_info.generateMipmaps << "; "
              << std::endl;
#endif

    ktxTexture2* texture = nullptr;
    auto result = ktxTexture2_Create(&create_info, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &texture);
    texture_raii.reset(texture);

    if (result != KTX_SUCCESS) {
        fatal(rc::RUNTIME_ERROR, "ktxTexture2_Create returned ktx_error_code: {}",
              static_cast<uint32_t>(result));
    }

    // Loop over all images and set them
    for (uint32_t level_idx = 0; level_idx < texture->numLevels; ++level_idx) {
        const uint32_t depth = std::max(texture->baseDepth >> level_idx, 1u);
        for (ktx_uint32_t layer_idx = 0; layer_idx < texture->numLayers; ++layer_idx) {
            for (uint32_t face_idx = 0; face_idx < texture->numFaces; ++face_idx) {
                for (uint32_t slice_idx = 0; slice_idx < depth; ++slice_idx) {
                    // Before anything, be absolutely certain that what we are about to write is of
                    // the exact same size (in bytes) of what libktx expects us to write for this
                    // mip level.
                    const size_t data_size = TinyDDS_ImageSize(dds_handle, level_idx);
                    const size_t expected_size = ktxTexture2_GetImageSize(texture, level_idx);
                    if (data_size != expected_size) {
                        fatal(rc::RUNTIME_ERROR,
                              "libktx expects {} bytes to be written for this mip level {} but {} "
                              "bytes are instead attempted to be written",
                              expected_size, level_idx, data_size);
                    }
                    // Get raw data (whether compressed, uncompressed, we don't care). The data
                    // should just match the set vkFormat, that's all. Remember that this is a
                    // lossless conversion so we do not decode/encode anything at all.
                    auto data_ptr = (const ktx_uint8_t*)TinyDDS_ImageRawData(dds_handle, level_idx);
                    auto status =
                        ktxTexture_SetImageFromMemory(ktxTexture(texture), level_idx, 0,
                                                      face_idx + slice_idx, data_ptr, data_size);
                    if (status != KTX_SUCCESS) {
                        fatal(rc::RUNTIME_ERROR,
                              "ktxTexture_SetImageFromMemory returned KTX exit error code: {}",
                              static_cast<uint32_t>(status));
                    }
                }  // slices
            }  // faces
        }  // layers
    }  // mip levels

    // Add required writer metadata.
    const auto writer = fmt::format("{} {}", commandName, version(options.testrun));
    ktxHashList_AddKVPair(&texture->kvDataHead, KTX_WRITER_KEY,
                          static_cast<uint32_t>(writer.size() + 1), writer.c_str());
    outputStream.writeKTX2(texture, *this);
}


void CommandConvert::tinydds_error(void* user, char const* msg) {
    ((TinyDDS_CustomData*)user)->parent->warning(msg);
}

void* CommandConvert::tinydds_alloc(void*, size_t size) { return new unsigned char[size]; }

void CommandConvert::tinydds_free(void*, void* memory) { delete[] (unsigned char*)memory; }

size_t CommandConvert::tinydds_read(void* user, void* buffer, size_t byteCount) {
    const auto str = ((TinyDDS_CustomData*)user)->str;
    if (auto result = str->read(str, buffer, byteCount); result != KTX_SUCCESS) return 0;
    return byteCount;
}

bool CommandConvert::tinydds_seek(void* user, int64_t offset) {
    const auto str = ((TinyDDS_CustomData*)user)->str;
    if (auto result = str->setpos(str, offset); result != KTX_SUCCESS) return false;
    return true;
}

int64_t CommandConvert::tinydds_tell(void* user) {
    const auto str = ((TinyDDS_CustomData*)user)->str;
    ktx_off_t curr_pos;
    // This callback is only called initially, or after successful call to read()
    // To be absolutely safe, do a `fatal` on failure
    if (auto result = str->getpos(str, &curr_pos); result != KTX_SUCCESS)
        ((TinyDDS_CustomData*)user)
            ->parent->fatal(rc::RUNTIME_ERROR, "Call to streambuf->getpos() failed");
    return curr_pos;
}

} // namespace ktx

KTX_COMMAND_ENTRY_POINT(ktxConvert, ktx::CommandConvert)

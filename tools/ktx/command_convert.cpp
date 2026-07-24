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
#include "dxgiformat.h"

// Implementation is already included in Basis Universal
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

    // Callbacks for TinyDDS
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

/// Get the corresponding VkFormat from the provided TinyDDS DXGI format. Not
/// all formats have an equivalent in Vulkan. All BC1-BC7 formats are supported.
/// The usual LDR and HDR uncompressed formats are also supported. For any
/// non-supported format, this returns VK_FORMAT_UNDEFINED.
inline VkFormat dds_to_vkformat(TinyDDS_Format dds_format) {
    auto dxgi_format = static_cast<DXGI_FORMAT>(dds_format);
    switch (dxgi_format) {
#include "dxgiFormat2vkFormat.inl"
    default:
        return VK_FORMAT_UNDEFINED;
    }
}

struct TinyDDS_CustomData {
    CommandConvert* parent;
    ktxStream* str;
};

/// Deleter that wraps `TinyDDS_DestroyContext` to be passed to std::unique_ptr
void TinyDDS_Deleter(TinyDDS_ContextHandle* handle) { TinyDDS_DestroyContext(*handle); }

#if 0  // This is just kept here in case we decide to use our own tinydds.h
/// Mimics `ktxTexture2_GetImageSize` because TinyDDS' `TinyDDS_ImageSize`
/// returns the size of an entire 3D texture, or entire array, or entire
/// cubemap.
///
/// Calculate & return the size in bytes of
/// an image at the specified mip level. For arrays, this is the size of a
/// layer, for cubemaps, the size of a face and for 3D textures, the size of a
/// depth slice.
size_t TinyDDS_GetImageSize(TinyDDS_ContextHandle handle, int mipmaplevel) {
    const TinyDDS_Context* ctx = (TinyDDS_Context*)handle;
    if (ctx == NULL) return 0;
    if (!ctx->headerValid) {
        ctx->callbacks.errorFn(ctx->user, "Header data hasn't been read yet or is invalid");
        return 0;
    }
    size_t w = std::max(ctx->header.width >> mipmaplevel, 1u);
    size_t h = std::max(ctx->header.height >> mipmaplevel, 1u);
    size_t d = std::max(ctx->header.depth >> mipmaplevel, 1u);
    const size_t s = ctx->headerDx10.arraySize ? ctx->headerDx10.arraySize : 1;
    if (d > 1 && s > 1) {
        ctx->callbacks.errorFn(ctx->user, "Volume texture arrays are not supoprted by DDS");
        return 0;
    }
    if (TinyDDS_IsCompressed(ctx->format)) {
        // padd to block boundaries
        w = (w + 3) / 4;
        h = (h + 3) / 4;
    }
    // 1 bit special case
    if (ctx->format == TDDS_R1_UNORM) w = (w + 7) / 8;
    const size_t formatSize = TinyDDS_FormatSize(ctx->format);
    return w * h * formatSize;
}
#endif

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
    if (!TinyDDS_ReadHeader(dds_handle)) fatal(rc::INVALID_FILE, "Failed to read DDS header");

    // Check endianess (this will always return false but there is a TODO pending in its
    // implementation)
    if (TinyDDS_NeedsEndianCorrecting(dds_handle))
        fatal(rc::INVALID_FILE, "Handling of big endian DDS data is not yet supported");

    // Get some specs that will be passed to ktxTexture2 creation struct
    uint32_t base_width, base_height, base_depth, layers;
    if (!TinyDDS_Dimensions(dds_handle, &base_width, &base_height, &base_depth, &layers))
        fatal(rc::INVALID_FILE, "Failed to retrieve texture dimensions from DDS input");

    // libktx expects dimensions to be >= 1 while TinyDDS can report, for instance, depth of 0
    base_width = std::max(base_width, 1u);
    base_height = std::max(base_height, 1u);
    base_depth = std::max(base_depth, 1u);
    layers = std::max(layers, 1u);

    uint32_t num_levels = TinyDDS_NumberOfMipmaps(dds_handle);
    if (num_levels == 0)
        fatal(rc::INVALID_FILE, "Expected at least one mipmap level but retrieved 0");

    uint32_t num_dims = 2;
    if (base_height > 1 && base_depth > 1)
        num_dims = 3;
    else if (base_height <= 1)
        num_dims = 1;

    // DDS doesn't support volume/cubemap texture arrays
    if (layers > 1 && (num_dims == 3 || TinyDDS_IsCubemap(dds_handle)))
        fatal(rc::INVALID_FILE, "DDS does not support Volume/Cubemap texture arrays");

    uint32_t num_faces = 1u;
    if (TinyDDS_IsCubemap(dds_handle))
        num_faces = 6;  // from the source code of tinydds.h it seems that DDS
                        // cubemap textures can only be of exactly 6 faces.

    // TODO: is TinyDDS up-to-date with latest DDS spec?
    //       see: https://github.com/microsoft/DirectXTex
    TinyDDS_Format dds_format = TinyDDS_GetFormat(dds_handle);
    if (dds_format == TDDS_UNDEFINED)
        fatal(rc::RUNTIME_ERROR, "Failed to retrieve DDS format (TDDS_UNDEFINED)");
    if (dds_format >= TDDS_SYNTHESISED_DXGIFORMATS)
        fatal(rc::RUNTIME_ERROR, "Unsupported synthesised DXGI format");

    VkFormat vkformat = dds_to_vkformat(dds_format);
    if (vkformat == VK_FORMAT_UNDEFINED)
        fatal(rc::RUNTIME_ERROR, "Failed to retrieve Vulkan format from DDS format {}",
              static_cast<uint32_t>(dds_format));

    ktxTextureCreateInfo create_info;
    create_info.glInternalformat = 0;  // Ignored as this is not a KTX1 texture
    create_info.vkFormat = vkformat;
    create_info.pDfd = nullptr;
    create_info.baseWidth = base_width;
    create_info.baseHeight = base_height;
    create_info.baseDepth = base_depth;
    create_info.numDimensions = num_dims;
    create_info.numLevels = num_levels;
    create_info.numLayers = layers;
    create_info.numFaces = num_faces;
    create_info.isArray = layers > 1;
    create_info.generateMipmaps = KTX_FALSE;

#if 1
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
              << "generateMipmaps=" << create_info.generateMipmaps << std::endl;
#endif

    ktxTexture2* texture = nullptr;
    auto result = ktxTexture2_Create(&create_info, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &texture);
    texture_raii.reset(texture);

    if (result != KTX_SUCCESS)
        fatal(rc::RUNTIME_ERROR, "ktxTexture2_Create returned ktx_error_code: {}",
              static_cast<uint32_t>(result));

    // Loop over all images and set them
    for (uint32_t level_idx = 0; level_idx < texture->numLevels; ++level_idx) {
        const uint32_t depth = std::max(texture->baseDepth >> level_idx, 1u);
        // Get raw data (whether compressed, uncompressed, we don't care). The data should just
        // match the set vkFormat, that's all. Remember that this is a lossless conversion so we do
        // not decode/encode anything at all.
        //
        // For cubemaps, this points to first face
        // For 3d textures, this points to first depth slice
        // For 2d texture arrays, this points to first array layer
        //   => in all of these cases, an offset needs to be added
        //
        auto data_ptr = (const ktx_uint8_t*)TinyDDS_ImageRawData(dds_handle, level_idx);
        if (data_ptr == NULL)
            fatal(rc::RUNTIME_ERROR,
                  "Failed to retrieve raw image data from DDS file at mipmap level {}", level_idx);
        // Before anything, be absolutely certain that what we are about to write is of
        // the exact same size (in bytes) of what libktx expects us to write for this
        // mip level.
        //
        // This doesn't return the image size in the same manner that libktx does.
        // For cubemaps, this returns size of all faces (i.e., size of a face multiplied 6)
        // For volumes, this returns size of all volume image
        // For arrays, this returns size of all array layers
        //    => hence why a division is needed (you can safely multiply these because
        //       DDS does not support volume/cubemap arrays.
        const size_t data_size = TinyDDS_ImageSize(dds_handle, level_idx) /
                                 (texture->numLayers * texture->numFaces * depth);
        const size_t expected_size = ktxTexture2_GetImageSize(texture, level_idx);
        if (data_size != expected_size)
            fatal(rc::RUNTIME_ERROR,
                  "libktx expects {} bytes to be written for this mip level {} but {} "
                  "bytes are instead attempted to be written",
                  expected_size, level_idx, data_size);
        for (ktx_uint32_t layer_idx = 0; layer_idx < texture->numLayers; ++layer_idx) {
            for (uint32_t face_idx = 0; face_idx < texture->numFaces; ++face_idx) {
                for (uint32_t slice_idx = 0; slice_idx < depth; ++slice_idx) {
                    // DDS doesn't support volume/cubemap texture arrays, so the following addition
                    // is an addition on mutually exclusive indices
                    size_t offset = data_size * (layer_idx + face_idx + slice_idx);
                    auto status = ktxTexture_SetImageFromMemory(ktxTexture(texture), level_idx,
                                                                layer_idx, face_idx + slice_idx,
                                                                data_ptr + offset, data_size);
                    if (status != KTX_SUCCESS)
                        fatal(rc::RUNTIME_ERROR,
                              "ktxTexture_SetImageFromMemory returned KTX exit error code: {}",
                              static_cast<uint32_t>(status));
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

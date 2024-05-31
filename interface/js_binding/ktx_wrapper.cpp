/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab textwidth=80: */

/*
 * Copyright 2019-2024 Khronos Group, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <emscripten/bind.h>
#include <ktx.h>
#include "vkformat_enum.h"
#include <iostream>

using namespace emscripten;

namespace ktx
{
    class texture
    {
    private:
        texture(ktxTexture* ptr)
            : m_ptr{ ptr, &destroy }
        {
        }

        texture() : m_ptr{ nullptr, &destroy }
        {
        }

        texture(ktxTexture2* ptr)
            : m_ptr{ reinterpret_cast<ktxTexture*>(ptr), &destroy }
        {
        }

        static void destroy(ktxTexture* ptr)
        {
            ktxTexture_Destroy(ptr);
        }

        std::unique_ptr<ktxTexture, decltype(&destroy)> m_ptr;

    public:
        operator ktxTexture2*() const {
            return reinterpret_cast<ktxTexture2*>(m_ptr.get());
        }

        bool isTexture2() const {
            return (m_ptr->classId == ktxTexture2_c);

        }

        //texture(texture&) = delete;
        texture(texture&&) = default;

        texture(const val& data) : m_ptr{ nullptr, &destroy }
        {
            std::vector<uint8_t> bytes{};
            bytes.resize(data["byteLength"].as<size_t>());
            val memory = emscripten::val::module_property("HEAP8")["buffer"];
            val memoryView = data["constructor"].new_(memory, reinterpret_cast<uintptr_t>(bytes.data()), data["length"].as<uint32_t>());
            // Yes, this code IS copying the data. Sigh! According to Alon
            // Zakai:
            //     "There isn't a way to let compiled code access a new
            //     ArrayBuffer. The compiled code has hardcoded access to the
            //     wasm Memory it was instantiated with - all the pointers it
            //     can understand are indexes into that Memory. It can't refer
            //     to anything else, I'm afraid."
            //
            //     "In the future using different address spaces or techniques
            //     with reference types may open up some possibilities here."
            memoryView.call<void>("set", data);

            // Load the image data now, otherwise we'd have to  copy it from
            // JS into a buffer only for it to be copied from that buffer into
            // the texture later.
            ktxTexture* ptr = nullptr;
            KTX_error_code result = ktxTexture_CreateFromMemory(
                                        bytes.data(),
                                        bytes.size(),
                                        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                        &ptr);
            if (result != KTX_SUCCESS)
            {
                std::cout << "ERROR: Failed to create from memory: " << ktxErrorString(result) << std::endl;
            }
            m_ptr = texture(ptr).m_ptr;
        }

        texture(const texture& in) : m_ptr{ nullptr, &destroy }
        {
            if (!in.isTexture2())
                 return;

            ktxTexture2* ptr = nullptr;
            ktx_error_code_e result;
            result = ktxTexture2_CreateCopy(in, &ptr);
            if (result != KTX_SUCCESS)
            {
                std::cout << "ERROR: Failed to copy construct: " << ktxErrorString(result) << std::endl;
            }
            m_ptr = texture(ptr).m_ptr;
        }

        // This is needed because of a runtime embind error:"Cannot register
        // multiple constructors with identical number of parameters (1)."
        // See BINDINGS below for more details.
        texture createCopy()
        {
            texture textureCopy(*this);
            return textureCopy;
        }

        val findKeyValue(const std::string key)
        {
            ktxHashList* ht = &m_ptr->kvDataHead;
            ktx_error_code_e result;
            unsigned int valueLen = 0;
            ktx_uint8_t* pValue = nullptr;
            result = ktxHashList_FindValue(ht, key.c_str(), &valueLen, (void**)&pValue);
            if (result == KTX_SUCCESS) {
                return val(emscripten::typed_memory_view((ktx_size_t)valueLen, pValue));
            } else {
                std::cout << "ERROR: failed to findKeyValue: " << ktxErrorString(result) << std::endl;
                return val::null();
            }
        }

        uint32_t getDataSize() const 
        {
          return ktxTexture_GetDataSize(m_ptr.get());
        }

        uint32_t baseWidth() const
        {
            return m_ptr->baseWidth;
        }

        uint32_t baseHeight() const
        {
            return m_ptr->baseHeight;
        }

        bool needsTranscoding() const
        {
            return ktxTexture_NeedsTranscoding(m_ptr.get());
        }

        khr_df_transfer_e getOETF() const
        {
            if (isTexture2())
                return ktxTexture2_GetOETF_e(*this);
            else
                return KHR_DF_TRANSFER_UNSPECIFIED;
        }

        khr_df_primaries_e getPrimaries() const
        {
            if (isTexture2())
                return ktxTexture2_GetPrimaries_e(*this);
            else
                return KHR_DF_PRIMARIES_UNSPECIFIED;
        }

        bool isSrgb() const
        {
            return (getOETF() == KHR_DF_TRANSFER_SRGB);
        }

        bool isPremultiplied() const
        {
            return (isTexture2()
                    && ktxTexture2_GetPremultipliedAlpha(*this));
        }

        // @copydoc ktxTexture2::GetNumComponents
        uint32_t numComponents() const
        {
            if (!isTexture2())
            {
                std::cout << "ERROR: numComponents is only supported for KTX2" << std::endl;
                return 0;
            }

            return ktxTexture2_GetNumComponents(*this);
        }

        enum ktxSupercmpScheme supercompressionScheme() const
        {
            if (isTexture2())
                return static_cast<ktxTexture2*>(*this)->supercompressionScheme;
            else
                return KTX_SS_NONE;
        }

        uint32_t vkFormat() const
        {
            if (isTexture2()) {
                return static_cast<ktxTexture2*>(*this)->vkFormat;
            } else {
                std::cout << "ERROR: vkFormat is only supported for KTX2" << std::endl;
                return VK_FORMAT_UNDEFINED;
            }
        }

        val getImage(uint32_t level, uint32_t layer, uint32_t faceSlice)
        {
            ktx_error_code_e result;
            ktx_size_t imageByteOffset, imageByteLength;
            ktxTexture* ptr = m_ptr.get();

            if (faceSlice == KTX_FACESLICE_WHOLE_LEVEL) {
                result = ktxTexture_GetImageOffset(ptr, level, layer, 0, &imageByteOffset);
                if (result != KTX_SUCCESS) {
                    std::cout << "ERROR: getImage.ktxTexture_GetImageOffset: " << ktxErrorString(result) << std::endl;
                    return val::null();
                }
                imageByteLength = ktxTexture_GetLevelSize(ptr, level);
            } else {
                result = ktxTexture_GetImageOffset(ptr, level, layer, faceSlice, &imageByteOffset);
                if (result != KTX_SUCCESS) {
                    std::cout << "ERROR: getImage.ktxTexture_GetImageOffset: " << ktxErrorString(result) << std::endl;
                    return val::null();
                }
                imageByteLength = ktxTexture_GetImageSize(ptr, level);
            }
            if (imageByteLength >  ptr->dataSize) {
                std::cout << "ERROR: getImage: not enough data in texture." << std::endl;
                return val::null();
            }
            return val(emscripten::typed_memory_view(imageByteLength,
                                                     ptr->pData + imageByteOffset));
        }

        struct ktxOrientation orientation() const
        {
            return m_ptr->orientation;
        }

        ktx_error_code_e transcodeBasis(const val& targetFormat, const val& decodeFlags)
        {
            if (!isTexture2())
            {
                std::cout << "ERROR: transcodeBasis is only supported for KTX2" << std::endl;
                return KTX_INVALID_OPERATION;
            }

            KTX_error_code result = ktxTexture2_TranscodeBasis(
                *this,
                targetFormat.as<ktx_texture_transcode_fmt_e>(),
                decodeFlags.as<ktx_transcode_flags>());

            if (result != KTX_SUCCESS)
            {
                std::cout << "ERROR: Failed to transcode: " << ktxErrorString(result) << std::endl;
            }
            return result;
        }

        // NOTE: WebGLTexture objects are completely opaque so the option of passing in the texture
        // to use is not viable.
        val glUpload()
        {
            GLuint texname = 0;
            GLenum target = 0;
            GLenum error = 0;
            KTX_error_code result = ktxTexture_GLUpload(m_ptr.get(), &texname, &target, &error);
            if (result != KTX_SUCCESS)
            {
                std::cout << "ERROR: Failed to GL upload: " << ktxErrorString(result) << std::endl;
            }

            val ret = val::object();
            // Find the WebGLTexture for texture.
            val texObject = val::module_property("GL")["textures"][texname];
            ret.set("object", texObject);
            ret.set("target", target);
            ret.set("error", error);
            return ret;
        }

#if KTX_FEATURE_WRITE
        texture(const ktxTextureCreateInfo& createInfo,
                ktxTextureCreateStorageEnum storageAllocation)
                : m_ptr{ nullptr, &destroy }
        {
            ktxTexture* ptr = nullptr;
//#define DUMP_CREATEINFO
#ifdef DUMP_CREATEINFO
    printf("createInfo.vkFormat = %d\n", createInfo.vkFormat);
    printf("createInfo.baseWidth = %d\n", createInfo.baseWidth);
    printf("createInfo.baseHeight = %d\n", createInfo.baseHeight);
    printf("createInfo.baseDepth = %d\n", createInfo.baseDepth);
    printf("createInfo.numDimensions = %d\n", createInfo.numDimensions);
    printf("createInfo.numLevels = %d\n", createInfo.numLevels);
    printf("createInfo.numLayers = %d\n", createInfo.numLayers);
    printf("createInfo.numFaces = %d\n", createInfo.numFaces);
    printf("createInfo.isArray = %d\n", createInfo.isArray);
    printf("createInfo.generateMipmaps = %d\n", createInfo.generateMipmaps);
#endif
            KTX_error_code result = ktxTexture2_Create(&createInfo,
                                                       storageAllocation,
                                                       (ktxTexture2**)&ptr);

            if (result != KTX_SUCCESS) {
                std::cout << "ERROR: failed to create texture: " << ktxErrorString(result) << std::endl;
            }
            m_ptr = texture(ptr).m_ptr;
        }

        ktx_error_code_e setImageFromMemory(ktx_uint32_t level,
                                            ktx_uint32_t layer,
                                            ktx_uint32_t faceSlice,
                                            const val& jsimage)
        {
            std::vector<uint8_t> image{};
            image.resize(jsimage["byteLength"].as<size_t>());
            val memory = emscripten::val::module_property("HEAP8")["buffer"];
            val memoryView = jsimage["constructor"].new_(memory,
                                               reinterpret_cast<uintptr_t>(image.data()),
                                               jsimage["length"].as<uint32_t>());
            // Yes, this code IS copying the data. Sigh! According to Alon
            // Zakai:
            //     "There isn't a way to let compiled code access a new
            //     ArrayBuffer. The compiled code has hardcoded access to the
            //     wasm Memory it was instantiated with - all the pointers it
            //     can understand are indexes into that Memory. It can't refer
            //     to anything else, I'm afraid."
            //
            //     "In the future using different address spaces or techniques
            //     with reference types may open up some possibilities here."
            memoryView.call<void>("set", jsimage);
            ktx_error_code_e result;
            result = ktxTexture_SetImageFromMemory(m_ptr.get(),
                                                   level, layer, faceSlice,
                                                   image.data(), image.size());

            if (result != KTX_SUCCESS) {
                std::cout << "ERROR: Failed to setImageFromMemory: " << ktxErrorString(result) << std::endl;
            }
            return result;

        }

        ktx_error_code_e compressAstc(const ktxAstcParams& params_input)
        {
            KTX_error_code result = KTX_SUCCESS;
            ktxAstcParams params = params_input;
            params.structSize = sizeof(ktxAstcParams);
            params.threadCount = 1;
            params.verbose = false;

//#define DUMP_ASTC_PARAMS
#ifdef DUMP_ASTC_PARAMS
    printf("params.structSize %d\n", params.structSize);
    printf("params.verbose %d\n", params.verbose);
    printf("params.threadCount %d\n", params.threadCount);
    printf("params.blockDimension %d\n", params.blockDimension);
    printf("params.mode %d\n", params.mode);
    printf("params.qualityLevel %d\n", params.qualityLevel);
    printf("params.normalMap %d\n", params.normalMap);
    printf("params.inputSwizzle %.4s\n", params.inputSwizzle);
#endif

            result = ktxTexture2_CompressAstcEx(*this, &params);
            if (result != KTX_SUCCESS) {
                std::cout << "ERROR: failed to compressAstc: " << ktxErrorString(result) << std::endl;
            }
            return result;
        }

        ktx_error_code_e compressBasis(const ktxBasisParams& params_input)
        {
            KTX_error_code result = KTX_SUCCESS;
            ktxBasisParams params = params_input;
            params.structSize = sizeof(ktxBasisParams);
            params.threadCount = 1;
            params.verbose = false;
            params.noSSE = true;

//#define DUMP_PARAMS
#ifdef DUMP_PARAMS
    printf("params.structSize %d\n", params.structSize);
    printf("params.uastc %d\n", params.uastc);
    printf("params.verbose %d\n", params.verbose);
    printf("params.noSSE %d\n", params.noSSE);
    printf("params.threadCount %d\n", params.threadCount);
    printf("params.inputSwizzle %.4s\n", params.inputSwizzle);
    printf("params.preSwizzle %d\n", params.preSwizzle);
    printf("params.ETC1S.compressionLevel %d\n", params.compressionLevel);
    printf("params.ETC1S.qualityLevel %d\n", params.qualityLevel);
    printf("params.ETC1S.maxEndpoints %d\n", params.maxEndpoints);
    printf("params.ETC1S.endpointRDOThreshold %f\n", params.endpointRDOThreshold);
    printf("params.ETC1S.maxSelectors %d\n", params.maxSelectors);
    printf("params.ETC1S.selectorRDOThreshold %f\n", params.selectorRDOThreshold);
    printf("params.ETC1S.normalMap %d\n", params.normalMap);
    printf("params.ETC1S.separateRGToRGB_A %d\n", params.separateRGToRGB_A);
    printf("params.ETC1S.noEndpointRDO %d\n", params.noEndpointRDO);
    printf("params.ETC1S.noSelectorRDO %d\n", params.noSelectorRDO);

    printf("params.UASTC.uastcFlags %d\n", params.uastcFlags);
    printf("params.UASTC.uastcRDO %d\n", params.uastcRDO);
    printf("params.UASTC.uastcRDOQualityScalar %f\n", params.uastcRDOQualityScalar);
    printf("params.UASTC.uastcRDODictSize %d\n", params.uastcRDODictSize);
    printf("params.UASTC.uastcRDOMaxSmoothBlockErrorScale %f\n", params.uastcRDOMaxSmoothBlockErrorScale);
    printf("params.UASTC.uastcRDOMaxSmoothBlockStdDev %f\n", params.uastcRDOMaxSmoothBlockStdDev);
    printf("params.UASTC.uastcRDODontFavorSimplerModes %d\n", params.uastcRDODontFavorSimplerModes);
    printf("params.UASTC.uastcRDONoMultithreading %d\n", params.uastcRDONoMultithreading);
#endif

            result = ktxTexture2_CompressBasisEx(*this, &params);
            if (result != KTX_SUCCESS) {
                std::cout << "ERROR: failed to compressBasis: " << ktxErrorString(result) << std::endl;
            }
            return result;
        }

        ktx_error_code_e deflateZstd(int compression_level)
        {
            ktx_error_code_e result;

            if (!isTexture2())
            {
                std::cout << "ERROR: deflateZstd is only supported for KTX2" << std::endl;
                return KTX_INVALID_OPERATION;
            }

            result = ktxTexture2_DeflateZstd(*this, compression_level);
            if (result != KTX_SUCCESS) {
                std::cout << "ERROR: failed to deflateZstd: " << ktxErrorString(result) << std::endl;
            }
            return result;
        }

        ktx_error_code_e deflateZLIB(int compression_level)
        {
            ktx_error_code_e result;

            if (!isTexture2())
            {
                std::cout << "ERROR: deflateZLIB is only supported for KTX2" << std::endl;
                return KTX_INVALID_OPERATION;
            }

            result = ktxTexture2_DeflateZLIB(*this, compression_level);
            if (result != KTX_SUCCESS) {
                std::cout << "ERROR: failed to deflateZLIB: " << ktxErrorString(result) << std::endl;
            }
            return result;
        }

        ktx_error_code_e addKVPair(const std::string& key, const std::string& value)
        {
            ktxHashList* ht = &m_ptr->kvDataHead;
            ktx_error_code_e result;
            result = ktxHashList_AddKVPair(ht, key.c_str(),
                                           value.size() + 1,
                                           value.c_str());

            if (result != KTX_SUCCESS) {
                std::cout << "ERROR: failed to addKVPair (string): " << ktxErrorString(result) << std::endl;
            }
            return result;
        }

        ktx_error_code_e addKVPair(const std::string& key, const val& jsvalue)
        {
            std::vector<uint8_t> value{};
            value.resize(jsvalue["byteLength"].as<size_t>());
            val memory = val::module_property("HEAP8")["buffer"];
            val memoryView = jsvalue["constructor"].new_(memory,
                                               reinterpret_cast<uintptr_t>(value.data()),
                                               jsvalue["length"].as<uint32_t>());
            memoryView.call<void>("set", jsvalue);

            ktxHashList* ht = &m_ptr->kvDataHead;
            ktx_error_code_e result;
            result = ktxHashList_AddKVPair(ht, key.c_str(),
                                           value.size(),
                                           value.data());

            if (result != KTX_SUCCESS) {
                std::cout << "ERROR: failed to addKVPair (vector): " << ktxErrorString(result) << std::endl;
            }
            return result;
        }

        ktx_error_code_e deleteKVPair(const std::string& key)
        {
            ktxHashList* ht = &m_ptr->kvDataHead;
            ktx_error_code_e result;
            result = ktxHashList_DeleteKVPair(ht, key.c_str());

            if (result != KTX_SUCCESS) {
                std::cout << "ERROR: failed to deleteKVPair: " << ktxErrorString(result) << std::endl;
            }
            return result;
        }

        // Should only be used when creating new ktx textures.
        // TODO: How to prevent use on ktxTexture objects
        // created with CreateFromMemory? Should we?
        void setOETF(khr_df_transfer_e oetf)
        {
            if (isTexture2())
                KHR_DFDSETVAL(static_cast<ktxTexture2*>(*this)->pDfd+1, TRANSFER, oetf);
        }

        void setPrimaries(khr_df_primaries_e primaries)
        {
            if (isTexture2())
                KHR_DFDSETVAL(static_cast<ktxTexture2*>(*this)->pDfd+1, PRIMARIES, primaries);
        }

        val writeToMemory() const
        {
            ktx_error_code_e result;
            ktx_size_t ktx_data_size = 0;
            ktx_uint8_t* ktx_data = nullptr;
            result = ktxTexture_WriteToMemory(m_ptr.get(), &ktx_data, &ktx_data_size);
            if (result == KTX_SUCCESS) {
                return val(emscripten::typed_memory_view(ktx_data_size, ktx_data));
            } else {
                std::cout << "ERROR: failed to writeToMemory: " << ktxErrorString(result) << std::endl;
                return val::null();
            }
        }
#endif
    };
}

/** @mainpage

 Javascript bindings are provided to:

 @li @subpage libktx_js "libktx (in libktx.js)"
 @li @subpage msc_basis_transcoder "Basis Universal Transcoder (in msc_basis_transcoder.js)


 ---
 @par This page last modified $Date$
*/

/** @page libktx_js libktx Binding

## WebIDL for the binding

@code{.idl}
interface ktxOrientation {
    readonly attribute OrientationX x;
    readonly attribute OrientationY y;
    readonly attribute OrientationZ z;
};

interface ktxUploadResult {
    readonly attribute WebGLTexture texture;
    readonly attribute GLenum target;
    readonly attribute GLenum error;
};

interface ktxTextureCreateInfo {
    attribute long vkFormat;
    attribute long baseWidth;
    attribute long baseHeight;
    attribute long baseDepth;
    attribute long numDimensions;
    attribute long numLevels;
    attribute long numLayers;
    attribute long numFaces;
    attribute boolean isArray;
    attribute boolean generateMipmaps;
};

interface ktxAstcParams {  // ktx only.
    void ktxAstcParams();

    attribute boolean verbose;
    attribute long threadCount;
    attribute AstcBlockDimension blockDimension;
    attribute AstcMode mode;
    attribute long qualityLevel;
    attribute boolean normalMap;
    attribute DOMString inputSwizzle;
};

interface ktxBasisParams {
    void ktxBasisParams();

    attribute boolean uastc,
    attribute boolean verbose,
    attribute boolean noSSE,
    attribute long threadCount,
    attribute DOMString inputSwizzle,
    attribute boolean preSwizzle,

    // ETC1S/Basis-LZ parameters.

    attribute long compressionLevel,
    attribute long qualityLevel,
    attribute long maxEndpoints,
    attribute float endpointRDOThreshold,
    attribute long maxSelectors,
    attribute float selectorRDOThreshold,
    attribute boolean normalMap,
    attribute boolean noEndpointRDO,
    attribute boolean noSelectorRDO,

    // UASTC parameters.

    attribute UastcFlags uastcFlags,
    attribute boolean uastcRDO,
    attribute float uastcRDOQualityScalar,
    attribute long uastcRDODictSize,
    attribute float uastcRDOMaxSmoothBlockErrorScale,
    attribute float uastcRDOMaxSmoothBlockStdDev,
    attribute boolean uastcRDODontFavorSimplerModes,
    attribute boolean uastcRDONoMultithreading
};

interface ktxTexture {
    void constructor(ArrayBufferView fileData);
    void constructor(ktxTextureCreateInfo createInfo, // ktx only
                     CreateStorageEnum? storage);

    ErrorCode? compressAstc(ktxAstcParams params); // ktx only.
    ErrorCode? compressBasis(ktxBasisParams params); // ktx only .
    ktxTexture createCopy();
    ErrorCode defateZLIB(); // ktx only.
    ErrorCode deflateZstd(); // ktx only.
    ArrayBufferView getImage(long level, long layer, long faceSlice);
    UploadResult glUpload();
    ErrorCode? setImageFromMemory(long level, long layer, long faceSlice,
                                  ArrayBufferView imageData); // ktx only.
    ErrorCode? transcodeBasis(TranscodeTarget? target, TranscodeFlagBits
                              decodeFlags);
    ArrayBufferView writeToMemory(); // ktx only.
    ErrorCode? addKVPair(DOMString key, DOMString value);
    deleteKVPair(DOMString key);
    DOMString findKeyValue(DOMString key);

    readonly attribute long baseWidth;
    readonly attribute long baseHeight;
    readonly attribute boolean isSRGB;
    readonly attribute boolean isPremultiplied;
    readonly attribute boolean needsTranscoding;
    readonly attribute long numComponents;
    readonly attribute long vkFormat;
    readonly attribute SupercmpScheme supercompressionScheme;
    readonly attribute ktxOrientation orientation;

    attribute dfTransfer OETF;
    attribute dfPrimaries primaries;
};

enum ErrorCode = {
    "SUCCESS",
    "FILE_DATA_ERROR",
    "FILE_ISPIPE",
    "FILE_OPEN_FAILED",
    "FILE_OVERFLOW",
    "FILE_READ_ERROR",
    "FILE_SEEK_ERROR",
    "FILE_UNEXPECTED_ERROR",
    "FILE_WRITE_ERROR",
    "GL_ERROR",
    "INVALID_OPERATION",
    "INVALID_VALUE",
    "NOT_FOUND",
    "OUT_OF_MEMORY",
    "TRANSCODE_FAILED",
    "UNKNOWN_FILE_FORMAT",
    "UNSUPPORTED_TEXTURE_TYPE",
    "UNSUPPORTED_FEATURE",
    "LIBRARY_NOT_LINKED"
};

// Some targets may not be available depending on options used when compiling
// the web assembly. ktxTexture.transcodeBasis will report this.
enum TranscodeTarget = {
    "ETC1_RGB",
    "BC1_RGB",
    "BC4_R",
    "BC5_RG",
    "BC3_RGBA",
    "BC1_OR_3",
    "PVRTC1_4_RGB",
    "PVRTC1_4_RGBA",
    "BC7_RGBA",
    "ETC2_RGBA",
    "ASTC_4x4_RGBA",
    "RGBA32",
    "RGB565",
    "BGR565",
    "RGBA4444",
    "PVRTC2_4_RGB",
    "PVRTC2_4_RGBA",
    "ETC",
    "EAC_R11",
    "EAC_RG11"
};

enum TranscodeFlagBits {
   "TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS"
};

enum OrientationX {
    "LEFT",
    "RIGHT"
};
enum OrientationY {
    "UP",
    "DOWN"
};
enum OrientationZ {
    "IN",
    "OUT"
};

enum SupercmpScheme {
    "NONE",
    "BASIS_LZ",
    "ZSTD"
    "ZLIB"
};

enum dfPrimaries = {
    // These are the values needed with HTML5/WebGL.
    "UNSPECIFIED",
    "BT709",
    "SRGB"
    "DISPLAYP3"
};

enum dfTransfer = {
    // These are the values needed for KTX with HTML5/WebGL.
    "UNSPECIFIED",
    "LINEAR",
    "SRGB",
    // DisplayP3 uses the SRGB transfer function.
};

enum VkFormat = {
    "R8G8B8A8_SRGB",
    "R8G8B8A8_UNORM"
    // Full list omitted as its length will distract from the documentation
    // purpose of this IDL. Any VkFormat valid for KTX can be used. As shown
    // here, omit the VK_FORMAT_ prefix and enclose in quotes.
}

enum AstcQualityLevel = {
    "FASTEST",
    "FAST",
    "MEDIUM",
    "THOROUGH",
    "EXHAUSTIVE",
};

enum AstcBlockDimension = {
    // 2D formats
    "d4x4",
    "d5x4",
    "d5x5",
    "d6x5",
    "d6x6",
    "d8x5",
    "d8x6",
    "d10x5",
    "d10x6",
    "d8x8",
    "d10x8",
    "d10x10",
    "d12x10",
    "d12x12",
    // 3D formats
    "d3x3x3",
    "d4x3x3",
    "d4x4x3",
    "d4x4x4",
    "d5x4x4",
    "d5x5x4",
    "d5x5x5",
    "d6x5x5",
    "d6x6x5",
    "d6x6x6"
};

enum AstcMode = {
    "DEFAULT",
    "LDR",
    "HDR"
};

enum UastcFlags = {
    "LEVEL_FASTEST",
    "LEVEL_FASTER",
    "LEVEL_DEFAULT",
    "LEVEL_SLOWER",
    "LEVEL_VERYSLOW",
};
@endcode

## How to use

Put libktx.js and libktx.wasm in a directory on your server. Create a script
tag with libktx.js as the @c src in your .html as shown below, changing the
path as necessary for the relative locations of your .html file and the
script source. libktx.js will automatically load libktx.wasm.
@code{.html}
    <script src="libktx.js"></script>
@endcode

@note For the read-only version of the library, use libktx_read.js and
libktx_read.wasm instead.

### Create an instance of the ktx module

To avoid polluting the global @c window name space all methods, variables and
tokens related to libktx are wrapped in a function that returns a promise.
The promise is resolved with a module instance when it is safe to run the
compiled code. To use any of the features your code must call the function,
wait for the promise resolution and use the returned instance. Before doing so,
your code must create your WebGL context. The context is needed during module
initialization so that the @c glUpload function can provide WebGLTexture object
handles on the same context.

The function is called @e createKtxModule. In previous releases it was called
@e LIBKTX. It has been renamed to clarify what it is actually doing. Old scripts
must be updated to the new name.

@note In libktx_read.js the function is called @e createKtxReadModule.

Add the following to the top of your script to call the function, wait for the
instance of the ktx module, make it available in the window name space, make
your WebGL context the current context in Emscripten's OpenGL emulation and
call your @c main().

This snippet shows WebGL context creation as well.

@code{.js}
    const canvas = document.querySelector('#glcanvas');
    gl = canvas.getContext('webgl2');

    // If we don't have a GL context, give up now
    if (!gl) {
      alert('Unable to initialize WebGL. Your browser or machine may not support it.');
    } else {
      createKtxModule({preinitializedWebGLContext: gl}).then(instance => {
        window.ktx = instance;
        // Make existing WebGL context current for Emscripten OpenGL.
        ktx.GL.makeContextCurrent(
                    ktx.GL.createContext(document.getElementById("glcanvas"),
                                            { majorVersion: 2.0 })
                    );
        main()
      });
    }
@endcode

@e Call @c main() after the module instance has been created. Start the rest of your
code there.

### Download a KTX file and create a texture

To download an existing texture and create a WebGL texture from it, execute a
function like @c loadTexture the following:

@code{.js}
    var myTexture;

    main() {
        loadTexture(gl, "myTextureUrl");
    }

    function loadTexture(gl, url)
    {
      // Create placeholder which will be replace once the data arrives.
      myTexture = createPlaceholderTexture(gl, [0, 0, 255, 255]);
      gl.bindTexture(myTexture.target, myTexture.object);

      var xhr = new XMLHttpRequest();
      xhr.open('GET', url);
      xhr.responseType = "arraybuffer";
      xhr.onload = function(){
        const { ktxTexture, TranscodeTarget, OrientationX, OrientationY } = ktx;
        var ktxdata = new Uint8Array(this.response);
        ktexture = new ktxTexture(ktxdata);
        const tex = uploadTextureToGl(gl, ktexture);
        setTexParameters(tex, ktexture);
        gl.bindTexture(tex.target, tex.object);
        gl.deleteTexture(texture.object);
        texture = tex;
        elem('format').innerText = tex.format;
        ktexture.delete();
      };

      //xhr.onprogress = runProgress;
      //xhr.onloadstart = openProgress;
      xhr.send();
    }
@endcode

This is the function for creating the place holder texture.

@code{.js}
function createPlaceholderTexture(gl, color)
{
    const placeholder = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, placeholder);

    const level = 0;
    const internalFormat = gl.RGBA;
    const width = 1;
    const height = 1;
    const border = 0;
    const srcFormat = gl.RGBA;
    const srcType = gl.UNSIGNED_BYTE;
    const pixel = new Uint8Array(color);

    gl.texImage2D(gl.TEXTURE_2D, level, internalFormat,
                  width, height, border, srcFormat, srcType,
                  pixel);
    return {
      target: gl.TEXTURE_2D,
      object: placeholder,
      format: formatString,
    };
}
@endcode


Uploading the KTX texture to the WebGL context is done like this. This function
returns the created WebGL texture object and matching texture target.

@code{.js}
function uploadTextureToGl(gl, ktexture) {
  const { TranscodeTarget  } = ktx;
  var formatString;

  if (ktexture.needsTranscoding) {
    var format;
    if (astcSupported) {
      formatString = 'ASTC';
      format = TranscodeTarget.ASTC_4x4_RGBA;
    } else if (dxtSupported) {
      formatString = ktexture.numComponents == 4 ? 'BC3' : 'BC1';
      format = TranscodeTarget.BC1_OR_3;
    } else if (pvrtcSupported) {
      formatString = 'PVRTC1';
      format = TranscodeTarget.PVRTC1_4_RGBA;
    } else if (etcSupported) {
      formatString = 'ETC';
      format = TranscodeTarget.ETC;
    } else {
      formatString = 'RGBA4444';
      format = TranscodeTarget.RGBA4444;
    }
    if (ktexture.transcodeBasis(format, 0) != ktx.ErrorCode.SUCCESS) {
        alert('Texture transcode failed. See console for details.');
        return undefined;
    }
  }

  const result = ktexture.glUpload();
  if (result.error != gl.NO_ERROR) {
    alert('WebGL error when uploading texture, code = '
          + result.error.toString(16));
    return undefined;
  }
  if (result.object === undefined) {
    alert('Texture upload failed. See console for details.');
    return undefined;
  }
  if (result.target != gl.TEXTURE_2D) {
    alert('Loaded texture is not a TEXTURE2D.');
    return undefined;
  }

  return {
    target: result.target,
    object: result.object,
    format: formatString,
  }
}

@endcode

This is the function to correctly set the TexParameters for the loaded
texture. It expects that the WebGLTexture object in the @c texture
parameter was created from the content of the ktexture parameter.

@code{.js}
    function setTexParameters(texture, ktexture) {
      gl.bindTexture(texture.target, texture.object);

      if (ktexture.numLevels > 1 || ktexture.generateMipmaps) {
         // Enable bilinear mipmapping.
         gl.texParameteri(texture.target,
                          gl.TEXTURE_MIN_FILTER, gl.LINEAR_MIPMAP_NEAREST);
      } else {
        gl.texParameteri(texture.target, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
      }
      gl.texParameteri(texture.target, gl.TEXTURE_MAG_FILTER, gl.LINEAR);

      gl.bindTexture(texture.target, null);
    }
@endcode

@note It is not clear if glUpload can be used with, e.g. THREE.js. It may
be necessary to expose the ktxTexture_IterateLevelFaces or
ktxTexture_IterateLoadLevelFaces API to JS with those calling a
callback in JS to upload each image to WebGL.

*/

EMSCRIPTEN_BINDINGS(ktx)
{
    enum_<ktx_error_code_e>("ErrorCode")
        .value("SUCCESS", KTX_SUCCESS)
        .value("FILE_DATA_ERROR", KTX_FILE_DATA_ERROR)
        .value("FILE_ISPIPE", KTX_FILE_ISPIPE)
        .value("FILE_OPEN_FAILED", KTX_FILE_OPEN_FAILED)
        .value("FILE_OVERFLOW", KTX_FILE_OVERFLOW)
        .value("FILE_READ_ERROR", KTX_FILE_READ_ERROR)
        .value("FILE_SEEK_ERROR", KTX_FILE_SEEK_ERROR)
        .value("FILE_UNEXPECTED_ERROR", KTX_FILE_UNEXPECTED_EOF)
        .value("FILE_WRITE_ERROR", KTX_FILE_WRITE_ERROR)
        .value("GL_ERROR", KTX_GL_ERROR)
        .value("INVALID_OPERATION", KTX_INVALID_OPERATION)
        .value("INVALID_VALUE", KTX_INVALID_VALUE)
        .value("NOT_FOUND", KTX_NOT_FOUND)
        .value("OUT_OF_MEMORY", KTX_OUT_OF_MEMORY)
        .value("TRANSCODE_FAILED", KTX_TRANSCODE_FAILED)
        .value("UNKNOWN_FILE_FORMAT", KTX_UNKNOWN_FILE_FORMAT)
        .value("UNSUPPORTED_TEXTURE_TYPE", KTX_UNSUPPORTED_TEXTURE_TYPE)
        .value("UNSUPPORTED_FEATURE", KTX_UNSUPPORTED_FEATURE)
        .value("LIBRARY_NOT_LINKED", KTX_LIBRARY_NOT_LINKED)
        .value("DECOMPRESS_LENGTH_ERROR", KTX_DECOMPRESS_LENGTH_ERROR)
        .value("DECOMPRESS_CHECKSUM_ERROR", KTX_DECOMPRESS_CHECKSUM_ERROR)
        ;

    enum_<ktx_texture_transcode_fmt_e>("TranscodeTarget")
        .value("ETC1_RGB", KTX_TTF_ETC1_RGB)
        .value("BC1_RGB", KTX_TTF_BC1_RGB)
        .value("BC4_R", KTX_TTF_BC4_R)
        .value("BC5_RG", KTX_TTF_BC5_RG)
        .value("BC3_RGBA", KTX_TTF_BC3_RGBA)
        .value("BC1_OR_3", KTX_TTF_BC1_OR_3)
        .value("PVRTC1_4_RGB", KTX_TTF_PVRTC1_4_RGB)
        .value("PVRTC1_4_RGBA", KTX_TTF_PVRTC1_4_RGBA)
        .value("BC7_RGBA", KTX_TTF_BC7_RGBA)
        .value("ETC2_RGBA", KTX_TTF_ETC2_RGBA)
        .value("ASTC_4x4_RGBA", KTX_TTF_ASTC_4x4_RGBA)
        .value("RGBA32", KTX_TTF_RGBA32)
        .value("RGB565", KTX_TTF_RGB565)
        .value("BGR565", KTX_TTF_BGR565)
        .value("RGBA4444", KTX_TTF_RGBA4444)
        .value("RGBA8888", KTX_TTF_RGBA32)
        .value("PVRTC2_4_RGB", KTX_TTF_PVRTC2_4_RGB)
        .value("PVRTC2_4_RGBA", KTX_TTF_PVRTC2_4_RGBA)
        .value("ETC", KTX_TTF_ETC)
        .value("EAC_R11", KTX_TTF_ETC2_EAC_R11)
        .value("EAC_RG11", KTX_TTF_ETC2_EAC_RG11)
    ;

    enum_<ktx_transcode_flag_bits_e>("TranscodeFlagBits")
        .value("TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS",
               KTX_TF_TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS)
    ;

    enum_<ktxSupercmpScheme>("SupercmpScheme")
        .value("NONE", KTX_SS_NONE)
        .value("BASIS_LZ", KTX_SS_BASIS_LZ)
        .value("ZSTD", KTX_SS_ZSTD)
        .value("ZLIB", KTX_SS_ZLIB)
    ;

    enum_<ktxOrientationX>("OrientationX")
        .value("LEFT", KTX_ORIENT_X_LEFT)
        .value("RIGHT", KTX_ORIENT_X_RIGHT)
    ;

    enum_<ktxOrientationY>("OrientationY")
        .value("UP", KTX_ORIENT_Y_UP)
        .value("DOWN", KTX_ORIENT_Y_DOWN)
    ;

    enum_<ktxOrientationZ>("OrientationZ")
        .value("IN", KTX_ORIENT_Z_IN)
        .value("OUT", KTX_ORIENT_Z_OUT)
    ;

    value_object<ktxOrientation>("Orientation")
        .field("x", &ktxOrientation::x)
        .field("y", &ktxOrientation::y)
        .field("z", &ktxOrientation::z)
    ;

    enum_<khr_df_primaries_e>("dfPrimaries")
        // These are the values needed with HTML5/WebGL.
        .value("UNSPECIFIED", KHR_DF_PRIMARIES_UNSPECIFIED)
        .value("BT709", KHR_DF_PRIMARIES_BT709)
        .value("SRGB", KHR_DF_PRIMARIES_SRGB)
        .value("DISPLAYP3", KHR_DF_PRIMARIES_DISPLAYP3)
    ;

    enum_<khr_df_transfer_e>("dfTransfer")
        // These are the values needed for KTX with HTML5/WebGL.
        .value("UNSPECIFIED", KHR_DF_TRANSFER_UNSPECIFIED)
        .value("LINEAR", KHR_DF_TRANSFER_LINEAR)
        .value("SRGB", KHR_DF_TRANSFER_SRGB)
        // DisplayP3 uses the SRGB transfer function.
    ;

    class_<ktx::texture>("ktxTexture")
        .constructor<const val>()
        // This compiles but get runtime error "Cannot register multiple
        // constructors with identical number of parameters (1) ...! Overload
        // resolution is currently only performed using the parameter count,
        // not actual type info!
        //.constructor<const ktx::texture&>()
        // So make copy constructor a function.
        .function("createCopy", &ktx::texture::createCopy,
                  return_value_policy::take_ownership())
        .property("dataSize", &ktx::texture::getDataSize)
        .property("baseWidth", &ktx::texture::baseWidth)
        .property("baseHeight", &ktx::texture::baseHeight)
#if KTX_FEATURE_WRITE
        .property("oetf", &ktx::texture::getOETF, &ktx::texture::setOETF)
        .property("primaries", &ktx::texture::getPrimaries,
                               &ktx::texture::setPrimaries)
#else
        .property("oetf", &ktx::texture::getOETF)
        .property("primaries", &ktx::texture::getPrimaries)
#endif
        .property("isSrgb", &ktx::texture::isSrgb)
        .property("isPremultiplied", &ktx::texture::isPremultiplied)
        .property("needsTranscoding", &ktx::texture::needsTranscoding)
        .property("numComponents", &ktx::texture::numComponents)
        .property("orientation", &ktx::texture::orientation)
        .property("supercompressScheme", &ktx::texture::supercompressionScheme)
        .property("vkFormat", &ktx::texture::vkFormat)
        .function("findKeyValue", &ktx::texture::findKeyValue)
        .function("getImage", &ktx::texture::getImage)
        .function("glUpload", &ktx::texture::glUpload)
        .function("transcodeBasis", &ktx::texture::transcodeBasis)
#if KTX_FEATURE_WRITE
        .constructor<const ktxTextureCreateInfo&, ktxTextureCreateStorageEnum>()
        .function("compressAstc", &ktx::texture::compressAstc)
        .function("compressBasis", &ktx::texture::compressBasis)
        .function("deflateZstd", &ktx::texture::deflateZstd)
        .function("deflateZLIB", &ktx::texture::deflateZLIB)
        .function("addKVPairString",
                  select_overload<ktx_error_code_e(const std::string&, const std::string&)>(&ktx::texture::addKVPair))
        .function("addKVPairByte",
                  select_overload<ktx_error_code_e(const std::string&, const val&)>(&ktx::texture::addKVPair))
        .function("deleteKVPair", &ktx::texture::deleteKVPair)
        .function("setImageFromMemory", &ktx::texture::setImageFromMemory)
        .function("writeToMemory", &ktx::texture::writeToMemory)
#endif
    ;

#if KTX_FEATURE_WRITE
    enum_<ktxTextureCreateStorageEnum>("CreateStorageEnum")
        .value("NO_STORAGE", KTX_TEXTURE_CREATE_NO_STORAGE)
        .value("ALLOC_STORAGE", KTX_TEXTURE_CREATE_ALLOC_STORAGE)
    ;

    enum_<VkFormat>("VkFormat")
#include "vk_format.inl"
    ;

    class_<ktxTextureCreateInfo>("ktxTextureCreateInfo")
      .constructor<>()
      // This and similar getters and setters below are needed so the, in
      // this case VkFormat, enum value is correctly retrieved from and
      // written to the uint32_t field of the, in this case,
      // ktxTextureCreateInfo, struct. Without these the JS side would have
      // to use, e.g, `VkFormat.R8G8B8A8_SRGB.value` to set this property.
      .property("vkFormat", +[](const ktxTextureCreateInfo& info) {
        return info.vkFormat;
      },
      +[](ktxTextureCreateInfo& info, VkFormat format) {
        info.vkFormat = format;
      })
      .property("baseWidth", &ktxTextureCreateInfo::baseWidth)
      .property("baseHeight", &ktxTextureCreateInfo::baseHeight)
      .property("baseDepth", &ktxTextureCreateInfo::baseDepth)
      .property("numDimensions", &ktxTextureCreateInfo::numDimensions)
      .property("numLevels", &ktxTextureCreateInfo::numLevels)
      .property("numLayers", &ktxTextureCreateInfo::numLayers)
      .property("numFaces", &ktxTextureCreateInfo::numFaces)
      .property("isArray", &ktxTextureCreateInfo::isArray)
      .property("generateMipmaps", &ktxTextureCreateInfo::generateMipmaps)
    ;

    enum_<ktx_pack_astc_quality_levels_e>("AstcQualityLevel")
      .value("FASTEST", KTX_PACK_ASTC_QUALITY_LEVEL_FASTEST)
      .value("FAST", KTX_PACK_ASTC_QUALITY_LEVEL_FAST)
      .value("MEDIUM", KTX_PACK_ASTC_QUALITY_LEVEL_MEDIUM)
      .value("THOROUGH", KTX_PACK_ASTC_QUALITY_LEVEL_THOROUGH)
      .value("EXHAUSTIVE", KTX_PACK_ASTC_QUALITY_LEVEL_EXHAUSTIVE)
    ;

    enum_<ktx_pack_astc_block_dimension_e>("AstcBlockDimension")
      // 2D formats
      .value("d4x4", KTX_PACK_ASTC_BLOCK_DIMENSION_4x4) //: 8.00 bpp
      .value("d5x4", KTX_PACK_ASTC_BLOCK_DIMENSION_5x4) //: 6.40 bpp
      .value("d5x5", KTX_PACK_ASTC_BLOCK_DIMENSION_5x5) //: 5.12 bpp
      .value("d6x5", KTX_PACK_ASTC_BLOCK_DIMENSION_6x5) //: 4.27 bpp
      .value("d6x6", KTX_PACK_ASTC_BLOCK_DIMENSION_6x6) //: 3.56 bpp
      .value("d8x5", KTX_PACK_ASTC_BLOCK_DIMENSION_8x5) //: 3.20 bpp
      .value("d8x6", KTX_PACK_ASTC_BLOCK_DIMENSION_8x6) //: 2.67 bpp
      .value("d10x5", KTX_PACK_ASTC_BLOCK_DIMENSION_10x5) //: 2.56 bpp
      .value("d10x6", KTX_PACK_ASTC_BLOCK_DIMENSION_10x6) //: 2.13 bpp
      .value("d8x8", KTX_PACK_ASTC_BLOCK_DIMENSION_8x8) //: 2.00 bpp
      .value("d10x8", KTX_PACK_ASTC_BLOCK_DIMENSION_10x8) //: 1.60 bpp
      .value("d10x10", KTX_PACK_ASTC_BLOCK_DIMENSION_10x10) //: 1.28 bpp
      .value("d12x10", KTX_PACK_ASTC_BLOCK_DIMENSION_12x10) //: 1.07 bpp
      .value("d12x12", KTX_PACK_ASTC_BLOCK_DIMENSION_12x12) //: 0.89 bpp
      // 3D formats
      .value("d3x3x3", KTX_PACK_ASTC_BLOCK_DIMENSION_3x3x3) //: 4.74 bpp
      .value("d4x3x3", KTX_PACK_ASTC_BLOCK_DIMENSION_4x3x3) //: 3.56 bpp
      .value("d4x4x3", KTX_PACK_ASTC_BLOCK_DIMENSION_4x4x3) //: 2.67 bpp
      .value("d4x4x4", KTX_PACK_ASTC_BLOCK_DIMENSION_4x4x4) //: 2.00 bpp
      .value("d5x4x4", KTX_PACK_ASTC_BLOCK_DIMENSION_5x4x4) //: 1.60 bpp
      .value("d5x5x4", KTX_PACK_ASTC_BLOCK_DIMENSION_5x5x4) //: 1.28 bpp
      .value("d5x5x5", KTX_PACK_ASTC_BLOCK_DIMENSION_5x5x5) //: 1.02 bpp
      .value("d6x5x5", KTX_PACK_ASTC_BLOCK_DIMENSION_6x5x5) //: 0.85 bpp
      .value("d6x6x5", KTX_PACK_ASTC_BLOCK_DIMENSION_6x6x5) //: 0.71 bpp
      .value("d6x6x6", KTX_PACK_ASTC_BLOCK_DIMENSION_6x6x6) //: 0.59 bpp
    ;

    enum_<ktx_pack_astc_encoder_mode_e>("AstcMode")
      .value("DEFAULT", KTX_PACK_ASTC_ENCODER_MODE_DEFAULT)
      .value("LDR", KTX_PACK_ASTC_ENCODER_MODE_LDR)
      .value("HDR", KTX_PACK_ASTC_ENCODER_MODE_HDR)
    ;

    class_<ktxAstcParams>("ktxAstcParams")
      .constructor<>()
      .property("structSize", &ktxAstcParams::structSize)
      .property("verbose", &ktxAstcParams::verbose)
      .property("threadCount", &ktxAstcParams::threadCount)
      .property("blockDimension", +[](const ktxAstcParams& p) {
        return p.blockDimension;
      },
      +[](ktxAstcParams& p, ktx_pack_astc_block_dimension_e d) {
          p.blockDimension = d;
      })
      .property("mode", +[](const ktxAstcParams& p) {
        return p.mode;
      },
      +[](ktxAstcParams& p, ktx_pack_astc_encoder_mode_e m) {
          p.mode = m;
      })
      .property("qualityLevel", +[](const ktxAstcParams& p) {
        return p.qualityLevel;
      },
      +[](ktxAstcParams& p, ktx_pack_astc_quality_levels_e q) {
          p.qualityLevel = q;
      })
      .property("normalMap", &ktxAstcParams::normalMap)
      // char arrays are not currently bindable. Interface vis std::string.
      .property<std::string>("inputSwizzle", +[](const ktxAstcParams& p) {
        return std::string(p.inputSwizzle, 4);
      },
      +[](ktxAstcParams& p, std::string s) {
        for (uint32_t i = 0; i < 4; i++) {
          p.inputSwizzle[i] = s[i];
        }
      })
    ;

    enum_<ktx_pack_uastc_flag_bits_e>("UastcFlags")
      .value("LEVEL_FASTEST", KTX_PACK_UASTC_LEVEL_FASTEST)
      .value("LEVEL_FASTER", KTX_PACK_UASTC_LEVEL_FASTER)
      .value("LEVEL_DEFAULT", KTX_PACK_UASTC_LEVEL_DEFAULT)
      .value("LEVEL_SLOWER", KTX_PACK_UASTC_LEVEL_SLOWER)
      .value("LEVEL_VERYSLOW", KTX_PACK_UASTC_LEVEL_VERYSLOW)
    ;

    class_<ktxBasisParams>("ktxBasisParams")
      .constructor<>()
      .property("structSize", &ktxBasisParams::structSize)
      .property("uastc", &ktxBasisParams::uastc)
      .property("verbose", &ktxBasisParams::verbose)
      .property("noSSE", &ktxBasisParams::noSSE)
      .property("threadCount", &ktxBasisParams::threadCount)
      .property<std::string>("inputSwizzle", +[](const ktxBasisParams& p) {
        return std::string(p.inputSwizzle, 4);
      },
      +[](ktxBasisParams& p, std::string s) {
        for (uint32_t i = 0; i < 4; i++) {
          p.inputSwizzle[i] = s[i];
        }
      })
      .property("preSwizzle", &ktxBasisParams::preSwizzle)

      /* ETC1S params */

      .property("compressionLevel", &ktxBasisParams::compressionLevel)
      .property("qualityLevel", &ktxBasisParams::qualityLevel)
      .property("maxEndpoints", &ktxBasisParams::maxEndpoints)
      .property("endpointRDOThreshold", &ktxBasisParams::endpointRDOThreshold)
      .property("maxSelectors", &ktxBasisParams::maxSelectors)
      .property("selectorRDOThreshold", &ktxBasisParams::selectorRDOThreshold)
      .property("normalMap", &ktxBasisParams::normalMap)
      .property("noEndpointRDO", &ktxBasisParams::noEndpointRDO)
      .property("noSelectorRDO", &ktxBasisParams::noSelectorRDO)

      /* UASTC params */

      .property("uastcFlags", +[](const ktxBasisParams& p) {
        return p.uastcFlags;
      },
      +[](ktxBasisParams& p, ktx_pack_uastc_flag_bits_e f) {
          p.uastcFlags = f;
      })
      .property("uastcRDO", &ktxBasisParams::uastcRDO)
      .property("uastcRDOQualityScalar", &ktxBasisParams::uastcRDOQualityScalar)
      .property("uastcRDODictSize", &ktxBasisParams::uastcRDODictSize)
      .property("uastcRDOMaxSmoothBlockErrorScale", &ktxBasisParams::uastcRDOMaxSmoothBlockErrorScale)
      .property("uastcRDOMaxSmoothBlockStdDev", &ktxBasisParams::uastcRDOMaxSmoothBlockStdDev)
      .property("uastcRDODontFavorSimplerModes", &ktxBasisParams::uastcRDODontFavorSimplerModes)
      .property("uastcRDONoMultithreading", &ktxBasisParams::uastcRDONoMultithreading);
    ;

    constant("AnimDataKey", std::string(KTX_ANIMDATA_KEY));
    constant("OrientationKey", std::string(KTX_ORIENTATION_KEY));
    constant("SwizzleKey", std::string(KTX_SWIZZLE_KEY));
    constant("WriterKey", std::string(KTX_WRITER_KEY));
    constant("FaceSliceWholeLevel", KTX_FACESLICE_WHOLE_LEVEL);
    constant("Etc1SDefaultCompressionLevel", KTX_ETC1S_DEFAULT_COMPRESSION_LEVEL);
#endif
}

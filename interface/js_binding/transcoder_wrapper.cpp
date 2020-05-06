/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab textwidth=80: */

/*
 * ©2019 Khronos Group, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <emscripten/bind.h>
#include "basisu_image_transcoders.h"
#include "basisu_transcoder_config.h"

using namespace emscripten;
using namespace basist;

namespace msc {
    class BasisTranscoderState: public basisu_transcoder_state {
    };

    class TranscodedImage {
      public:
        TranscodedImage(size_t size) : image(size) { }

        uint8_t* data() { return image.data(); }
        size_t size() { return image.size(); }

        val get_typed_memory_view() {
           return val(typed_memory_view(image.size(), image.data()));
        }

      protected:
        std::vector<uint8_t> image;
    };

    class ImageTranscoderHelper {
        // block size calculations
        static inline uint32_t getWidthInBlocks(uint32_t w, uint32_t bw)
        {
            return (w + (bw - 1)) / bw;
        }

        static inline uint32_t getHeightInBlocks(uint32_t h, uint32_t bh)
        {
            return (h + (bh - 1)) / bh;
        }        //

      public:
        static size_t getTranscodedImageByteLength(transcoder_texture_format format,
                                                   uint32_t width, uint32_t height)
        {
            uint32_t blockByteLength =
                      basis_get_bytes_per_block_or_pixel(format);
            if (basis_transcoder_format_is_uncompressed(format)) {
                return width * height * blockByteLength;
            } else if (format == transcoder_texture_format::cTFPVRTC1_4_RGB
                       || format == transcoder_texture_format::cTFPVRTC1_4_RGBA) {
                // For PVRTC1, Basis only writes (or requires)
                // blockWidth * blockHeight * blockByteLength. But GL requires
                // extra padding for very small textures:
                // https://www.khronos.org/registry/OpenGL/extensions/IMG/IMG_texture_compression_pvrtc.txt
                const uint32_t paddedWidth = (width + 3) & ~3;
                const uint32_t paddedHeight = (height + 3) & ~3;
                return (std::max(8U, paddedWidth)
                        * std::max(8U, paddedHeight) * 4 + 7) / 8;
            } else {
                uint32_t blockWidth = getWidthInBlocks(width, basis_get_block_width(format));
                uint32_t blockHeight = getHeightInBlocks(height, basis_get_block_height(format));
                return blockWidth * blockHeight * blockByteLength;
            }
        }
    };


    class BasisUImageTranscoder : public ktxBasisImageTranscoder {
      public:
        BasisUImageTranscoder() : ktxBasisImageTranscoder(buildSelectorCodebook())
        { }

        // Yes, code in the following functions handling data coming in from
        // ArrayBuffers IS copying the data. Sigh! According to Alon Zakai:
        //
        //     "There isn't a way to let compiled code access a new ArrayBuffer.
        //     The compiled code has hardcoded access to the wasm Memory it was
        //     instantiated with - all the pointers it can understand are
        //     indexes into that Memory. It can't refer to anything else,
        //     I'm afraid."
        //
        //     "In the future using different address spaces or techniques with
        //     reference types may open up some possibilities here."

        bool decode_palettes(uint32_t num_endpoints, const val& jsEndpoints,
                             uint32_t num_selectors, const val& jsSelectors)
        {
            std::vector<uint8_t> cEndpoints{}, cSelectors{};
            val memory = val::module_property("HEAP8")["buffer"];
            cEndpoints.resize(jsEndpoints["byteLength"].as<size_t>());
            val endpointsView = jsEndpoints["constructor"].new_(memory,
                                            reinterpret_cast<uintptr_t>(cEndpoints.data()),
                                            jsEndpoints["length"].as<uint32_t>());
            endpointsView.call<void>("set", jsEndpoints);

            cSelectors.resize(jsSelectors["byteLength"].as<size_t>());
            val selectorsView = jsSelectors["constructor"].new_(memory,
                                            reinterpret_cast<uintptr_t>(cSelectors.data()),
                                            jsSelectors["length"].as<uint32_t>());
            selectorsView.call<void>("set", jsSelectors);

            return ktxBasisImageTranscoder::decode_palettes(num_endpoints,
                                                       cEndpoints.data(),
                                                       cEndpoints.size(),
                                                       num_selectors,
                                                       cSelectors.data(),
                                                       cSelectors.size());
        }

        bool decode_tables(const val& jsTableData)
        {
            std::vector<uint8_t> cTableData{};
            val memory = val::module_property("HEAP8")["buffer"];

            cTableData.resize(jsTableData["byteLength"].as<size_t>());
            val TableDataView = jsTableData["constructor"].new_(memory,
                                            reinterpret_cast<uintptr_t>(cTableData.data()),
                                            jsTableData["length"].as<uint32_t>());
            TableDataView.call<void>("set", jsTableData);

            return ktxBasisImageTranscoder::decode_tables(cTableData.data(),
                                                          cTableData.size());
        }

        // @~English
        // @brief Transcode a single Basis supercompressed image.
        //
        // Most applications should use this transcoder in preference to the
        // low-level slice transcoder.
        //
        // @param[in] imageFlags   flags from the first uint32_t in an imageDesc
        //                         from a KTX2 file's supercompressionGlobalData
        //                         or a sliceDesc in a .basis file.
        // @param[in] jsRgbSlice   emscripten::val of a .subarray pointing to the
        //                         rgbSlice of the data to be transcoded within
        //                         the ArrayBuffer holding the file data.
        // @param[in] jsAlphaSlice emscripten::val of a .subarray pointing to the
        //                         alphaSlice of the data to be transcoded
        //                         within the ArrayBuffer holding the file data.
        // @param[in] targetFormat the format to which to transcode the image.
        //                         Only real formats are accepted, i.e., any
        //                         format accepted by ktxTexture2_TranscodeBasis
        //                         except @c KTX_TTF_ETC and @c KTX_TTF_BC1_OR_3.
        // @param[in] level the mip level of the image being transcoded.
        // @param[in] width the pixel width of a level @p level image.
        // @param[in] height the pixel height of a level @p level image.
        // @param[in] num_blocks_x number of blocks in the x dimension of mip
        //                         level @p level to be transcoded. This is the
        //                         number of blocks in base block-compressed
        //                         format used by Basis Universal. When the
        //                         format is ETC1, as indicated by @c eBuIsETC1S
        //                         being set in @c globalFlags in the
        //                         supercompression global data, the block width
        //                         to use for calculating this from @p width is 4.
        // @param[in] num_blocks_y number of blocks in the y dimension of mip
        //                         level @p level to be transcoded. When
        //                         @c eBuIsETC1S is set in @c globalFlags in the
        //                         supercompression global data the block height
        //                         to use for calculating this from @p height is 4.
        // @param[in] isVideo @c true if the image comes from a file containing
        //                    an animation sequence, @c false otherwise.
        // @param[in] transcodeAlphaToOpaqueFormats if @p targetFormat is a
        //                                          format lacking an alpha
        //                                          component, transcode the
        //                                          alpha slice into the RGB
        //                                          components.
        // @return An emscripten::val possibly 2 entries, @c result and @c
        //         transcodedImage. If error is not KTX_SUCCESS (0), it will be
        //         set to one of the following values:
        // @exception KTX_INVALID_VALUE  A non-real format was specified as
        //                               @p targetFormat.
        // @exception KTX_TRANSCODE_FAILED   Something went wrong during transcoding.
        //                                   The image will be corrupted.
        //
        emscripten::val transcode_image(uint32_t imageFlags,
                                     const val& jsRgbSlice,
                                     const val& jsAlphaSlice,
                                     const val& jsTargetFormat,
                                     uint32_t level,
                                     uint32_t width, uint32_t height,
                                     uint32_t num_blocks_x,
                                     uint32_t num_blocks_y,
                                     bool isVideo = false,
                                     bool transcodeAlphaToOpaqueFormats = false)
        {
            val memory = val::module_property("HEAP8")["buffer"];

            // Must reconstruct an imageDesc which is an array of 5 uint32_ts as
            // found in the supercompressionGlobalData of a KTX2 file:
            //
            //    uint32_t imageFlags
            //    uint32_t rgbSliceByteOffset
            //    uint32_t rgbSliceByteLength
            //    uint32_t alphaSliceByteOffset
            //    uint32_t alphaSliceByteLength
            //
            // Maybe there is a better way to pass the information in from Java
            // that can avoid the need for reconstruction.

            // First of all copy in the deflated data.
            std::vector <uint8_t> deflatedImage;
            size_t rgbSliceByteLength = jsRgbSlice["byteLength"].as<size_t>();
            size_t alphaSliceByteLength = jsAlphaSlice["byteLength"].as<size_t>();
            deflatedImage.resize(rgbSliceByteLength + alphaSliceByteLength);

            val memoryView = jsRgbSlice["constructor"].new_(memory,
                                         reinterpret_cast<uintptr_t>(deflatedImage.data()),
                                         rgbSliceByteLength);
            memoryView.call<void>("set", jsRgbSlice);
            if (alphaSliceByteLength != 0) {
                val memoryView = jsAlphaSlice["constructor"].new_(memory,
                                             reinterpret_cast<uintptr_t>(&deflatedImage[rgbSliceByteLength]),
                                             alphaSliceByteLength);
                memoryView.call<void>("set", jsAlphaSlice);
            }

            transcoder_texture_format cTargetFormat
                            = jsTargetFormat.as<transcoder_texture_format>();

            ktxBasisImageDesc imageDesc;
            imageDesc.imageFlags = imageFlags;
            imageDesc.rgbSliceByteOffset = 0;
            imageDesc.rgbSliceByteLength = rgbSliceByteLength;
            imageDesc.alphaSliceByteOffset = alphaSliceByteLength == 0 ?
                                                  0 : rgbSliceByteLength;
            imageDesc.alphaSliceByteLength = alphaSliceByteLength;

            //std::vector<uint8_t> dst;
            //dst.resize(getTranscodedImageByteLength(static_cast<transcoder_texture_format>(cTargetFormat),
            size_t tiByteLength =
            ImageTranscoderHelper::getTranscodedImageByteLength(cTargetFormat,
                                                                width,
                                                                height);
            TranscodedImage* dst = new TranscodedImage(tiByteLength);

            KTX_error_code error =
                ktxBasisImageTranscoder::transcode_image(
                                              imageDesc,
                                              cTargetFormat,
                                              dst->data(),
                                              dst->size(),
                                              level,
                                              deflatedImage.data(),
                                              width, height,
                                              num_blocks_x, num_blocks_y,
                                              isVideo,
                                              transcodeAlphaToOpaqueFormats);

            val ret = val::object();
            ret.set("error", static_cast<uint32_t>(error));
            if (error == KTX_SUCCESS) {
                ret.set("transcodedImage", dst);
            }
            return std::move(ret);
        }

      protected:
        static basist::etc1_global_selector_codebook* pGlobal_codebook;

        static basist::etc1_global_selector_codebook*
        buildSelectorCodebook()
        {
           if (!pGlobal_codebook) {
                pGlobal_codebook = new basist::etc1_global_selector_codebook(
                                                     g_global_selector_cb_size,
                                                     g_global_selector_cb);
            }
            return pGlobal_codebook;
        }
    };

    class UastcImageTranscoder : public ktxUastcImageTranscoder {
      public:
        UastcImageTranscoder() : ktxUastcImageTranscoder() { }

        emscripten::val transcode_image(
                                   const val& jsTargetFormat,
                                   uint32_t level,
                                   const val& jsInImage,
                                   uint32_t width, uint32_t height,
                                   uint32_t num_blocks_x,
                                   uint32_t num_blocks_y,
                                   bool hasAlpha = false,
                                   uint32_t transcode_flags = 0
                                   /*basisu_transcoder_state* pState = nullptr*/)
        {


            transcoder_texture_format cTargetFormat
                            = jsTargetFormat.as<transcoder_texture_format>();
            // Copy in the deflated image.
            std::vector <uint8_t> deflatedImage;
            size_t deflatedImageByteLength
                                     = jsInImage["byteLength"].as<size_t>();
            deflatedImage.resize(deflatedImageByteLength);
            val memory = val::module_property("HEAP8")["buffer"];
            val memoryView = jsInImage["constructor"].new_(memory,
                              reinterpret_cast<uintptr_t>(deflatedImage.data()),
                              deflatedImageByteLength);
            memoryView.call<void>("set", jsInImage);

            size_t tiByteLength =
            ImageTranscoderHelper::getTranscodedImageByteLength(cTargetFormat,
                                                                width,
                                                                height);
            TranscodedImage* dst = new TranscodedImage(tiByteLength);

            ktx_error_code_e error =
                ktxUastcImageTranscoder::transcode_image(
                                              cTargetFormat,
                                              dst->data(),
                                              dst->size(),
                                              level,
                                              deflatedImage.data(),
                                              deflatedImage.size(),
                                              width, height,
                                              num_blocks_x, num_blocks_y,
                                              hasAlpha,
                                              transcode_flags
                                              /*pState*/);

            val ret = val::object();
            ret.set("error", static_cast<uint32_t>(error));
            if (error == KTX_SUCCESS) {
                ret.set("transcodedImage", dst);
            }
            return std::move(ret);
        }
    };

    basist::etc1_global_selector_codebook* BasisUImageTranscoder::pGlobal_codebook;
}

/** @page msc_basis_transcoder Basis Image Transcoder binding

 ## WebIDL for the binding

@code{.unparsed}
interface BasisTranscoderState {
    void BasisTranscoderState();
};

interface TranscodedImage {
    ArrayBufferView get_typed_memory_view();
};

interface TranscodeResult {

    uint32_t error;
    TranscodedImage transcodedImage;
};

interface BasisUImageTranscoder {
    void BasisUImageTranscoder();
    uint32_t getBytesPerBlock(const TranscodeTarget format);
    bool decode_palettes(uint32_t num_endpoints,
                         const ArrayBufferView endpoints,
                         uint32_t num_selectors,
                         const ArrayBufferView selectors);
    bool decode_tables(const ArrayBufferView tableData);
    TranscodeResult transcode_image(uint32_t imageFlags,
                                    const ArrayBufferView rgbSlice,
                                    const ArrayBufferView alphaSlice,
                                    const TranscodeTarget targetFormat,
                                    uint32_t level,
                                    uint32_t width, uint32_t height,
                                    uint32_t num_blocks_x,
                                    uint32_t num_blocks_y,
                                    bool isVideo = false,
                                    bool transcodeAlphaToOpaqueFormats = false);
};

interface UastcImageTranscoder {
    void UastcImageTranscoder();
    uint32_t getBytesPerBlock(const TranscodeTarget format);
    TranscodeResult transcode_image(const val& jsTargetFormat,
                                    uint32_t level,
                                    const ArrayBufferView jsInImage,
                                    uint32_t width, uint32_t height,
                                    uint32_t num_blocks_x,
                                    uint32_t num_blocks_y,
                                    bool hasAlpha = false,
                                    uint32_t transcode_flags = 0);
};

// Some targets may not be available depending on options used when compiling
// the web assembly.
enum TranscodeTarget = {
    "ETC1_RGB",
    "BC1_RGB",
    "BC4_R",
    "BC5_RG",
    "BC3_RGBA",
    "BC1_OR_3",
    "PVRTC1_4_RGB",
    "PVRTC1_4_RGBA",
    "BC7_M6_RGB",
    "BC7_M5_RGBA",
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

enum TranscodeFlagBits = {
    "TRANSCODE_ALPHA_DAT_TO_OPAQUE_FORMATS",
    "HIGH_QUALITY"
};

@endcode

## How to use

Put msc_basis_transcoder.js and msc_basis_transcoder.wasm in a
directory on your server. Create a script tag with
msc_basis_tranacoder.js as the @c src as shown below, changing
the path as necessary for the relative locations of your .html
file and the script source. msc_basis_transcoder.js will
automatically load msc_basis_transcoder.wasm.

### Create an instance of the MSC_TRANSCODER module

Add this to the .html file to initialize the transcoder and make it available on the
main window.
@code{.unparsed}
    &lt;script src="msc_transcoder_wrapper.js">&lt;/script>
    &lt;script type="text/javascript">
      MSC_TRANSCODER().then(module => {
        window.MSC_TRANSCODER = module;
        // Call a function to begin loading or transcoding..
    &lt;/script>
@endcode

@e After the module is initialized, invoke code that will directly or indirectly cause
a function with code like the following to be executed.

## Somewhere in the loader/transcoder

Assume a KTX file is fetched via an XMLHttpRequest which deposits the data into
a Uint8Array, "buData"...

@note The names of the data items used in the following code are those
from the KTX2 specification but the actual data is not specific to that
container format.

@code{.unparsed}
    const {
        InitTranscoderGlobal,
        BasisUImageTranscoder,
        UastcImageTranscoder,
        BasisTranscoderState,
        TranscodeTarget
    } = MSC_TRANSCODER;

    InitTranscoderGlobal();

    // Determine from the KTX2 header information in buData if
    // the data format  is BasisU or Uastc.
    // supercompressionScheme value == 1, it's BasisU.
    // DFD colorModel == 166, it's UASTC.

    // Determine appropriate transcode format from available targets,
    // info about the texture, e.g. texture.numComponents, and
    // expected use. Use values from TranscodeTarget.
    var targetFormat = ...

    if (Uastc) {
        transcodeUastc(targetFormat);
    } else {
        transcodeEtc1s(targetFormat);
    }
@endcode

This is the function for transcoding etc1s.

@code{.unparsed}
transcodeEtc1s(targetFormat) {
    // Locate the supercompression global data and compresssed
    // mip level data within buData.

    var bit = new BasisUImageTranscoder();

    // Find the index of the starts of the endpoints, selectors and tables
    // data within buData...
    var endpointsStart = ...
    var selectorsStart = ...
    var tablesStart = ...
    // The numbers of endpoints & selectors and their byteLengths are items
    // within buData. In KTX2 they are in the header of the
    // supercompressionGlobalData.

    var endpoints = new Uint8Array(buData, endpointsStart,
                                   endpointsByteLength);
    var selectors = new Uint8Array(buData, selectorsStart,
                                   selectorsByteLength);

    bit.decodePalettes(numEndpoints, endpoints,
                              numSelectors, selectors);

    var tables = new UInt8Array(buData, tablesStart, tablesByteLength);
    bit.decodeTables(tables);

    // Determine if the file contains a video sequence...
    var isVideo = ...

    // Calculate the total number of images in the data
    var numImages = ...

    // Set up a subarray pointing at the deflated image descriptions
    // in buData. This is for KTX2 containers. .basis containers will
    // require slightly different code. The image descriptions are
    // located in supercompressionGlobalData.
    var imageDescsStart = ...:
    // An imageDesc has 5 uint32 values.
    var imageDescs = new Uint32Data(buData, imageDescsStart,
                                    numImages * 5 * 4);
    var curImageIndex = 0;

    // Pseudo code ...
    foreach level {
       var width = width of image at this level
       var height = height of image at this level
       var bw = 4; // for ETC1S based Basis compressed data.
       var bh = 4; //            ditto
       var num_blocks_x = Math.ceil(width / bw);
       var num_blocks_y = Math.ceil(height / bh);
       foreach image in level {
           // In KTX2 container locate the imageDesc for this image.
           var imageDesc = imageDescs[curImageIndex++];
           // Determine the location in the ArrayBuffer of the start
           // of the deflated data for level.
           var levelData = ...;
           // Make a .subarray of the rgb slice data.
           var rgbSliceStart = levelData + imageDesc[1];
           var rgbSliceByteLength = imageDesc[2];
           var rgbSlice = new UInt8Array(buData, rgbSliceStart,
           rgbSliceByteLength);
           // Do the same for the alpha slice. Length 0 is okay.
           var alphaSliceStart = levelData + imageDesc[3];
           var alphaSliceByteLength = imageDesc[4];
           var alphaSlice = new Uint8Array(buData, alphaSliceStart,
                                           alphaSliceByteLength);
           const {transcodedImage, error} = bit.transcodeImage(
                                     imageDesc[0], // imageFlags,
                                     rgbSlice,
                                     alphaSlice,
                                     targetFormat,
                                     level,
                                     width, height,
                                     num_blocks_x,
                                     num_blocks_y,
                                     isVideo,
                                     false);
            if (!error) {
                let imgData = transcodedImage.get_typed_memory_view();

                // Upload data in imgData to WebGL...

                // Do not call delete() until data has been uploaded
                // or otherwise copied.
                transcodedImage.delete();
            }
        }
    }
}
@endcode

This is the function for transcoding Uastc.

@code{.unparsed}
transcodeUastc(targetFormat) {
    var uit = new UastcImageTranscoder();

    // Determine if the data is supercompressed.
    var zstd = (supercompressionScheme == 2);

    // Determine if the data has alpha.
    var hasAlpha = (Channel ID of sample in DFD == 1);

    var dctx;
    if (zstd) {
        // Initialize the zstd decoder. Zstd JS wrapper + wasm is
        // a separate package.
        dctx = ZSTD_createDCtx();
    }

    // Pseudo code ...
    foreach level {
        // Determine the location in the ArrayBuffer buData of the
        // start of the deflated data for the level.
        var levelData = ...
        if (zstd) {
            // Inflate the level data
            levelData = ZSTD_decompressDCtx(dctx, levelData, ... );
        }

        var width = width of image at this level
        var height = height of image at this level
        var depth = depth of texture at this level
        var bw = 4; // for UASTC 4x4 block-compressed data.
        var bh = 4; //            ditto
        var num_blocks_x = Math.ceil(width / bw);
        var num_blocks_y = Math.ceil(height / bh);
        levelImageCount = number of layers * number of faces * depth;

        foreach image in level {
            inImage = Uint8Array(levelData, imageStart, imageEnd);
            const {transcodedImage, error} = uit.transcodeImage(
                                                        targetFormat,
                                                        level,
                                                        inImage,
                                                        width, height,
                                                        num_blocks_x,
                                                        num_blocks_y,
                                                        hasAlpha,
                                                        0);
            if (!error) {
                let imgData = transcodedImage.get_typed_memory_view();

                // Upload data in imgData to WebGL...

                // Do not call delete() until data has been uploaded
                // or otherwise copied.
                transcodedImage.delete();
            }
        }
    }
}
@endcode

*/

EMSCRIPTEN_BINDINGS(ktx_wrappers)
{
    enum_<ktx_texture_transcode_fmt_e>("TranscodeTarget")
        .value("ETC1_RGB", KTX_TTF_ETC1_RGB)
#if BASISD_SUPPORT_DXT1
        .value("BC1_RGB", KTX_TTF_BC1_RGB)
#endif
#if BASISD_SUPPORT_DXT5A
        .value("BC4_R", KTX_TTF_BC4_R)
        .value("BC5_RG", KTX_TTF_BC5_RG)
#endif
#if BASISD_SUPPORT_DXT1 && BASISD_SUPPORT_DXT5A
        .value("BC3_RGBA", KTX_TTF_BC3_RGBA)
        .value("BC1_OR_3", KTX_TTF_BC1_OR_3)
#endif
#if BASISD_SUPPORT_PVRTC1
        .value("PVRTC1_4_RGB", KTX_TTF_PVRTC1_4_RGB)
        .value("PVRTC1_4_RGBA", KTX_TTF_PVRTC1_4_RGBA)
#endif
#if BASISD_SUPPORT_BC7_MODE6_OPAQUE_ONLY
        .value("BC7_M6_RGB", KTX_TTF_BC7_M6_RGB)
#endif
#if BASISD_SUPPORT_BC7_MODE5
        .value("BC7_M5_RGBA", KTX_TTF_BC7_M5_RGBA)
#endif
#if BASISD_SUPPORT_ETC2_EAC_A8
        .value("ETC2_RGBA", KTX_TTF_ETC2_RGBA)
#endif
#if BASISD_SUPPORT_ASTC
        .value("ASTC_4x4_RGBA", KTX_TTF_ASTC_4x4_RGBA)
#endif
        .value("RGBA32", KTX_TTF_RGBA32)
        .value("RGB565", KTX_TTF_RGB565)
        .value("BGR565", KTX_TTF_BGR565)
        .value("RGBA4444", KTX_TTF_RGBA4444)
#if BASISD_SUPPORT_PVRTC2
        .value("PVRTC2_4_RGB", KTX_TTF_PVRTC2_4_RGB)
        .value("PVRTC2_4_RGBA", KTX_TTF_PVRTC2_4_RGBA)
#endif
#if BASISD_SUPPORT_ETC2_EAC_RG11
        .value("ETC", KTX_TTF_ETC)
        .value("EAC_R11", KTX_TTF_ETC2_EAC_R11)
        .value("EAC_RG11", KTX_TTF_ETC2_EAC_RG11)
#endif
    ;

    enum_<ktx_transcode_flag_bits_e>("TranscodeFlagBits")
        .value("TRANSCODE_ALPHA_DAT_TO_OPAQUE_FORMATS",
               KTX_TF_TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS)
        .value("HIGH_QUALITY", KTX_TF_HIGH_QUALITY)
    ;

    function("InitTranscoderGlobal", basisu_transcoder_init);

    class_<msc::BasisUImageTranscoder>("BasisUImageTranscoder")
        .constructor()
        .class_function("getBytesPerBlock", basis_get_bytes_per_block_or_pixel)
        .function("decodePalettes", &msc::BasisUImageTranscoder::decode_palettes)
        .function("decodeTables", &msc::BasisUImageTranscoder::decode_tables)
        .function("transcodeImage", &msc::BasisUImageTranscoder::transcode_image)
        ;

    class_<msc::UastcImageTranscoder>("UastcImageTranscoder")
        .constructor()
        .class_function("getBytesPerBlock", basis_get_bytes_per_block_or_pixel)
        .function("transcodeImage", &msc::UastcImageTranscoder::transcode_image)
        ;

    class_<basisu_transcoder_state>("BasisTranscoderState")
        .constructor()
        ;

    class_<msc::TranscodedImage>("TranscodedImage")
        .function( "get_typed_memory_view", &msc::TranscodedImage::get_typed_memory_view )
    ;

}

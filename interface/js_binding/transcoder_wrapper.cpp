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
#include "basis_image_transcoder.h"

using namespace emscripten;
using namespace basist;

namespace msc {
    class BasisTranscoder : public ktxBasisImageTranscoder {
      public:
        BasisTranscoder() : ktxBasisImageTranscoder(buildSelectorCodebook())
        { }

        bool decode_palettes(uint32_t num_endpoints, const val& endpoints_val,
                             uint32_t num_selectors, const val& selectors_val)
        {
            std::vector<uint8_t> endpoints{}, selectors{};
            val memory = val::module_property("HEAP8")["buffer"];

            endpoints.resize(endpoints_val["byteLength"].as<size_t>());
            val endpointsView = endpoints_val["constructor"].new_(memory,
                                            reinterpret_cast<uintptr_t>(endpoints.data()),
                                            endpoints_val["length"].as<uint32_t>());
            endpointsView.call<void>("set", endpoints_val);

            selectors.resize(selectors_val["byteLength"].as<size_t>());
            val selectorsView = selectors_val["constructor"].new_(memory,
                                            reinterpret_cast<uintptr_t>(selectors.data()),
                                            selectors_val["length"].as<uint32_t>());
            selectorsView.call<void>("set", selectors_val);

            return ktxBasisImageTranscoder::decode_palettes(num_endpoints,
                                                       endpoints.data(),
                                                       endpoints.size(),
                                                       num_selectors,
                                                       selectors.data(),
                                                       selectors.size());
        }

        bool decode_tables(const val& table_data_val)
        {
            std::vector<uint8_t> table_data{};
            val memory = val::module_property("HEAP8")["buffer"];

            table_data.resize(table_data_val["byteLength"].as<size_t>());
            val table_dataView = table_data_val["constructor"].new_(memory,
                                            reinterpret_cast<uintptr_t>(table_data.data()),
                                            table_data_val["length"].as<uint32_t>());
            table_dataView.call<void>("set", table_data_val);

            return ktxBasisImageTranscoder::decode_tables(table_data.data(),
                                                          table_data.size());
        }

        //
        // @~English
        // @brief Transcode a single Basis supercompressed image.
        //
        // Most applications should use this transcoder in preference to the
        // low-level slice transcoder.
        //
        // @param[in] imageDescVal emscripten::val of the JS object holding the
        //                         image descriptor for the to the i
        //                         @c BasisImageDesc of the image to be
        //                         transcoded. This comes from the @c
        //                         supercompressionGlobalData area of a KTX2 file.
        // @param[in] targetFormat the format to which to transcode the image.
        //                         Only real formats are accepted, i.e., any
        //                         format accepted by ktxTexture2_TranscodeBasis
        //                         except @c KTX_TTF_ETC and @c KTX_TTF_BC1_OR_3.
        // @param[in] writePtr emscripten::val of the JS object to which to
        //                     write the transcoded image.
        // @param[in] bufferByteLength the length of the buffer pointed to by @p
        //                             @p writePtr.
        // @param[in] level the mip level of the image being transcoded.
        // @param[in] levelDataPtr emscripten::val of the JS object from which
        //                         to read the supercompressed data for mip
        //                         level @p level. It must represent the entire
        //                         level.
        // @param[in] width the pixel width of a level @p level image.
        // @param[in] height the pixel height of a level @p level image.
        // @param[in] num_blocks_x number of blocks in the x dimension for mip
        //                         level @p level in the pre-deflation input. When
        //                         @c eBuIsETC1S is set in @c globalFlags in the
        //                         supercompression global data, the block width
        //                         to use for calculating this from @p width is 4.
        // @param[in] num_blocks_y number of blocks in the y dimension for mip
        //                         level @p level in the pre-deflation input. When
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
        // @exception KTX_INVALID_VALUE  A non-real format was specified as
        //                               @p targetFormat.
        // @exception KTX_TRANSCODE_FAILED   Something went wrong during transcoding.
        //                                   The image will be corrupted.
        //
        //
        KTX_error_code transcode_image(const val& imageDescVal,
                                     uint32_t targetFormatVal,
                                     const val& dst,
                                     size_t bufferByteLength,
                                     uint32_t level,
                                     const val& levelDataVal,
                                     uint32_t width, uint32_t height,
                                     uint32_t num_blocks_x,
                                     uint32_t num_blocks_y,
                                     bool isVideo = false,
                                     bool transcodeAlphaToOpaqueFormats = false)
        {
            const ktx_transcode_fmt_e targetFormat
                    = static_cast<ktx_transcode_fmt_e>(targetFormatVal);

            // FIXME: I'm not sure about this. Probably it is just data in an
            // arrayBuffer rather than an abject of some type and we need to
            // coerce to c++.
            val memory = val::module_property("HEAP8")["buffer"];
            ktxBasisImageDesc imageDesc;
            val memoryView = imageDescVal["constructor"].new_(memory,
                                                              imageDescVal,
                                                              reinterpret_cast<uintptr_t>(&imageDesc),
                                                              imageDescVal["length"].as<uint32_t>());
            memoryView.call<void>("set", imageDescVal);

            // If there is actual data copying going on here, we will want to
            // limit this to just the slices of the current image rather than
            // the whole miplevel.
            std::vector<uint8_t> levelData{};
            levelData.resize(levelDataVal["byteLength"].as<size_t>());
            memoryView = levelDataVal["constructor"].new_(memory,
                                        reinterpret_cast<uintptr_t>(levelData.data()),
                                        levelDataVal["length"].as<uint32_t>());
            memoryView.call<void>("set", levelDataVal);


            // Similarly, if there is any data copying going on here will want
            // to limit this to just the target image portion of the total image
            // data.
            std::vector<uint8_t> dst_data;
            dst_data.resize(bufferByteLength);

            KTX_error_code result;
            result = ktxBasisImageTranscoder::transcode_image(imageDesc, targetFormat,
                                                    dst_data.data(),
                                                    bufferByteLength,
                                                    level,
                                                    levelData.data(),
                                                    width, height,
                                                    num_blocks_x, num_blocks_y,
                                                    isVideo,
                                                    transcodeAlphaToOpaqueFormats);

            if (result == KTX_SUCCESS) {
                memoryView = val::global("Uint8Array").new_(memory,
                                         reinterpret_cast<uintptr_t>(dst_data.data()),
                                         dst_data.size());
                dst.call<void>("set", memoryView);
            }
            return result;
        }

        // @brief Transcode a BasisU encoded image slice.
        //
        // This is for the majority of target formats for which there is no need
        // to provide a buffer of already transcoded alpha data to be included
        // in the final image. Despite their names, @p isAlphaSlice and
        // @p miplevel are only used for slices from video files.
        //
        // FIXME: transcoder_state is only needed when transcoding multiple mip
        // levels in parallel on different threads. Is it needed in JavaScript?
        //
        // This is customized for use transcoding data from KTX2 files and
        // uploading to WebGL:
        // - It does not expose the @p output_row_pitch_in_blocks_or_pixels
        //   parameter of the base class function meaning data will always be
        //   tightly packed.
        // - It does not expose the @p output_rows_in_pixels parameter of the
        //   base class meaning there is no way to adjust the height of destination
        //   image. It will always have the height of the original image. Since
        //   this parameter is ignored by the base class function except when
        //   transcoding to RGBA32 it has little utility.
        bool transcode_slice(const val& dst, uint32_t num_blocks_x, uint32_t num_blocks_y,
                             const val& imagedata, uint32_t imagedata_size,
                             uint32_t target_fmt, uint32_t output_block_or_pixel_stride_in_bytes,
                             bool bc1_allow_threecolor_blocks,
                             const bool isVideo, const bool isAlphaSlice,
                             const uint32_t miplevel,
                             const uint32_t orig_width, const uint32_t orig_height,
                             const bool transcode_alpha = false,
                             const val& transcoder_state = static_cast<val>(m_def_state))
        {
            std::vector<uint8_t> image_data{};
            image_data.resize(imagedata["byteLength"].as<size_t>());
            val memory = val::module_property("HEAP8")["buffer"];
            val memoryView = imagedata["constructor"].new_(memory,
                                        reinterpret_cast<uintptr_t>(image_data.data()),
                                        imagedata["length"].as<uint32_t>());
            memoryView.call<void>("set", imagedata);

            const block_format target_format =
                                  static_cast<block_format>(target_fmt);
            const uint32_t bytes_per_slice = num_blocks_x * num_blocks_y
                                             * output_block_or_pixel_stride_in_bytes;

            std::vector<uint8_t> dst_data;
            dst_data.resize(bytes_per_slice);

            bool status = ktxBasisImageTranscoder::transcode_slice(dst_data.data(),
                                                 num_blocks_x, num_blocks_y,
                                                 image_data.data(), image_data.size(),
                                                 target_format,
                                                 output_block_or_pixel_stride_in_bytes,
                                                 bc1_allow_threecolor_blocks,
                                                 isVideo, isAlphaSlice,
                                                 miplevel,
                                                 orig_width, orig_height,
                                                 0, /* output_row_pitch_in_blocks_or_pixels */
                                                 nullptr, transcode_alpha);

            memoryView = val::global("Uint8Array").new_(memory,
                                                 reinterpret_cast<uintptr_t>(dst_data.data()),
                                                 dst_data.size());

            dst.call<void>("set", memoryView);

            return status;
        }


        // @brief Transcode a BasisU encoded image slice while merging in
        //        pre-transcoded alpha data.
        //
        // This is for target formats which require a temporary buffer
        // containing previously transcoded alpha data.
        //
        // @sa transcode_slice
        bool transcode_slice(const val& dst, uint32_t num_blocks_x, uint32_t num_blocks_y,
                             const val& imagedata, uint32_t imagedata_size,
                             const uint32_t target_fmt, uint32_t output_block_or_pixel_stride_in_bytes,
                             bool bc1_allow_threecolor_blocks,
                             const bool isVideo, const bool isAlphaSlice,
                             const uint32_t miplevel,
                             const uint32_t orig_width, const uint32_t orig_height,
                             uint32_t output_row_pitch_in_blocks_or_pixels,
                             bool transcode_alpha,
                             const val& alphadata,
                             const val& transcoder_state = static_cast<val>(m_def_state),
                             uint32_t output_rows_in_pixels = 0)
        {
            std::vector<uint8_t> image_data{};
            std::vector<uint8_t> alpha_data{};
            image_data.resize(imagedata["byteLength"].as<size_t>());
            alpha_data.resize(alphadata["byteLength"].as<size_t>());
            val memory = val::module_property("HEAP8")["buffer"];
            val memoryView = imagedata["constructor"].new_(memory,
                                        reinterpret_cast<uintptr_t>(image_data.data()),
                                        imagedata["length"].as<uint32_t>());
            memoryView.call<void>("set", imagedata);

            memoryView = alphadata["constructor"].new_(memory,
                                        reinterpret_cast<uintptr_t>(alpha_data.data()),
                                        alphadata["length"].as<uint32_t>());
            memoryView.call<void>("set", alphadata);

            const block_format target_format =
                                  static_cast<block_format>(target_fmt);
            const uint32_t bytes_per_slice = num_blocks_x * num_blocks_y
                                             * output_block_or_pixel_stride_in_bytes;

            std::vector<uint8_t> dst_data;
            std::vector<uint8_t> alphadst_data;
            dst_data.resize(bytes_per_slice);
            alphadst_data.resize(bytes_per_slice);

            bool status = ktxBasisImageTranscoder::transcode_slice(dst_data.data(),
                                                 num_blocks_x, num_blocks_y,
                                                 image_data.data(), image_data.size(),
                                                 target_format,
                                                 output_block_or_pixel_stride_in_bytes,
                                                 bc1_allow_threecolor_blocks,
                                                 isVideo, isAlphaSlice,
                                                 miplevel,
                                                 orig_width, orig_height,
                                                 0, /* output_row_pitch_in_blocks_or_pixels */
                                                 // FIXME HOW TO CONVERT
                                                 // transcoder_state val back to
                                                 // the pointer to the C++
                                                 // object?
                                                 nullptr,
                                                 transcode_alpha,
                                                 alpha_data.data());

            memoryView = val::global("Uint8Array").new_(memory,
                                                 reinterpret_cast<uintptr_t>(dst_data.data()),
                                                 dst_data.size());

            dst.call<void>("set", memoryView);

            return status;
        }

        // @brief Write opaque blocks into the alpha part of a transcoded
        //        texture.
        //
        // Used when transcoding an RGB texture to an RGBA target.
        static void write_opaque_alpha_blocks(uint32_t num_blocks_x, uint32_t num_blocks_y,
                                              const val& dst,
                                              uint32_t output_blocks_buf_size_in_blocks,
                                              block_format target_fmt, uint32_t block_stride_in_bytes,
                                              uint32_t output_row_pitch_in_blocks)
        {
            const block_format target_format =
                                  static_cast<block_format>(target_fmt);
            const uint32_t bufferByteLength = num_blocks_x * num_blocks_y
                                              * block_stride_in_bytes;

            std::vector<uint8_t> dst_data;
            dst_data.resize(bufferByteLength);

            basisu_transcoder::write_opaque_alpha_blocks(num_blocks_x,
                                                         num_blocks_y,
                                                         dst_data.data(),
                                                         output_blocks_buf_size_in_blocks,
                                                         target_format,
                                                         block_stride_in_bytes,
                                                         output_row_pitch_in_blocks);

            val memory = val::module_property("HEAP8")["buffer"];
            val memoryView = val::global("Uint8Array").new_(memory,
                                                 reinterpret_cast<uintptr_t>(dst_data.data()),
                                                 dst_data.size());

            dst.call<void>("set", memoryView);
        }

        // Declare constants.
        #define BLOCK_FORMAT(c) static const uint32_t c;
        #define TRANSCODE_FORMAT(c) static const uint32_t c;
        #define DECODE_FLAG(f)
        #include "constlist.inl"

      protected:
        static basisu_transcoder_state m_def_state;
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

#if 0
        basist::etc1_global_selector_codebook*
        convertSelectorCodebook(const val& data)
        {
            std::vector<uint8_t> bytes{};
            bytes.resize(data["byteLength"].as<size_t>());
            val memory = val::module_property("HEAP8")["buffer"];
            val memoryView = data["constructor"].new_(memory, reinterpret_cast<uintptr_t>(bytes.data()), data["length"].as<uint32_t>());
            memoryView.call<void>("set", data);
            return (basist::etc1_global_selector_codebook*)bytes.data();
        }
#endif
    };

    basist::etc1_global_selector_codebook* BasisTranscoder::pGlobal_codebook;

    // Define constants.
    #undef BLOCK_FORMAT
    #undef TRANSCODE_FORMAT
    #define BLOCK_FORMAT(c) const uint32_t BasisTranscoder::c = static_cast<uint32_t>(block_format::c);
    #define TRANSCODE_FORMAT(c) const uint32_t BasisTranscoder::c = static_cast<uint32_t>(::c);
    #include "constlist.inl"
}

#undef BLOCK_FORMAT
#undef TRANSCODE_FORMAT
#define BLOCK_FORMAT(c) .class_property(#c, &msc::BasisTranscoder::c)
#define TRANSCODE_FORMAT(c) .class_property(#c, &msc::BasisTranscoder::c)

/** @page transcoder_js_binding Using the JS Binding to the Low-Level Basis Transcoder

== Recommended Way

    // Fetch a URL of a file with Basis Universal compressed data
    // into an ArrayBuffer, texdata.

    // Parse the file, locating the supercompression global data and compresssed
    // mip level data.

    Module.BasisTranscoder.initTranscoder();
    var transcoder = new Module.BasisTranscoder();
    // Determine appropriate transcode format from available targets,
    // info about the texture, e.g. texture.numComponents, and
    // expected use.
    var targetFormat = ...

    // Determine total size of data transcoded to this format
    var bufferByteLength = ...
    var transcodedData = new UInt8ArrayBuffer[bufferByteLength];
    // Alternatively you could make a buffer for each level or image.

    // Determine if the file contains a video sequence...
    var isVideo = ...

    // Pseudo code ...
    foreach level
       var width = width of image at this level
       var height = height of image at this level
       var bw = 4; // for ETC1S based Basis compressed data.
       var bh = 4; //            ditto
       var num_blocks_x = (width + (bw - 1)) / bw;
       var levelData = location of level within texdata
       var num_blocks_y = (height + (bh - 1)) / bh;
       foreach image in level
           // Determine offset for this image within transcodedData
           var transcodedImageOffset = ...
           var imageDesc;
           // In a .ktx2 file the following information is found in
           // the supercompressionGlobalData field of a BasisU
           // compressed file.
           imageDesc[0] = rgb slice offset within levelData;
           imageDesc[1] = rgb slice byte length;
           // [2] and [3] must be 0 if no alpha slice.
           imageDesc[2] = alpha slice offset with levelData;
           imageDesc[3] = alpha slice byte length;
           transcoder.transcodeImage(imageDesc,
                                     targetFormat,
                                     transcodedData + transcodedImageOffset,
                                     bufferByteLength - transcodedImageOffseet,
                                     level,
                                     levelData,
                                     width, height,
                                     num_blocks_x,
                                     num_blocks_y,
                                     isVideo = false);

== Low-level Way

Here the application must transcode each slice individually and combine the
slices to make a texture image ready for upload.

    // Fetch a URL of a file with Basis Universal compressed data
    // into an ArrayBuffer, texdata.

    // Parse the file, locating the supercompression global data and compresssed
    // mip level data.

    Module.BasisTranscoder.initTranscoder();
    var transcoder = new Module.BasisTranscoder();
    // Determine appropriate transcode format from available targets,
    // info about the texture, e.g. texture.numComponents, and
    // expected use.
    var targetFormat = ...

    // Determine total size of data transcoded to this format
    var bufferByteLength = ...
    var transcodedData = new UInt8ArrayBuffer[bufferByteLength];
    // Alternatively you could make a buffer for each level or image.

    // Determine if the file contains a video sequence...
    var isVideo = ...

    // Determine if the encoded data has alpha...
    var hasAlpha = ...

    var bytes_per_block = 

    // Pseudo code ...
    foreach level
       var width = width of image at this level
       var height = height of image at this level
       var bw = 4; // for ETC1S based Basis compressed data.
       var bh = 4; //            ditto
       var num_blocks_x = (width + (bw - 1)) / bw;
       var levelData = location of level within texdata
       var num_blocks_y = (height + (bh - 1)) / bh;
       var bytes_per_block = transcoder.getBytesPerBlock(targetFormat)
       foreach image in level
           // Determine offset for this image within transcodedData
           var transcodedImageOffset = ...
           // When using the low-level decoder the application must
           // transcode each slice and combine to make final result.
           // The actual operations are dependent on the target
           // format and the input data.
           //
           // This example does it only for ETC2_EAC_A8 target format.

           // In a .ktx2 file the following information is found in
           // the supercompressionGlobalData field of a BasisU
           // compressed file.
           var rgbSliceOffset = rgb slice offset within levelData;
           var rgbSLiceByteLength - rgb slice byte length;
           // Will be 0 if no alpha slice.
           var alphaSliceOffset = alpha slice offset with levelData;
           var alphaSliceByteLength = alpha slice byte length;

            if (hasAlpha) {
                status = transcoder.transcode_slice(
                                transcodedData + transcodedImageOffset,
                                num_blocks_x, num_blocks_y,
                                levelData + image.alphaSliceByteOffset,
                                image.alphaSliceByteLength,
                                transcoder.cETC2_EAC_A8, bytes_per_block,
                                true,
                                isVideo, true, level, width, height);
            } else {
                basisu_transcoder::write_opaque_alpha_blocks(
                    num_blocks_x, num_blocks_y,
                    transcodedData + transcodedImageOffset,
                    (uint32_t)(bufferByteLength / bytes_per_block),
                    transcoder.cETC2_EAC_A8, bytes_per_block,
                    0);
                status = true;
            }
            if (status) {
                // Now decode the color data.
              status = transcode_slice(
                      transcodedData + transcodedImageOffset + 8,
                      num_blocks_x, num_blocks_y,
                      levelDataPtr + image.rgbSliceByteOffset,
                      image.rgbSliceByteLength,
                      transcoder::cETC1, bytes_per_block,
                      true,
                      isVideo, false, level, width, height);
            }
            if (!status) {
                 result = TRANSCODE_FAILED;
            }
*/

EMSCRIPTEN_BINDINGS(ktx_wrappers)
{
    class_<msc::BasisTranscoder>("BasisTranscoder")
        #include "constlist.inl"
        .constructor()
        .class_function("initTranscoder", basisu_transcoder_init)
        .class_function("getBytesPerBlock", basis_get_bytes_per_block)
        .class_function("writeOpaqueAlphaBlocks",
                        &msc::BasisTranscoder::write_opaque_alpha_blocks)
        .function("decodePalettes", &msc::BasisTranscoder::decode_palettes)
        .function("decodeTables", &msc::BasisTranscoder::decode_tables)
        .function("transcodeImage", &msc::BasisTranscoder::transcode_image)
        .function("transcodeSlice",
                  select_overload<bool(const val&, uint32_t, uint32_t, const val&,
                                  uint32_t, uint32_t, uint32_t, bool, bool, bool,
                                  uint32_t, uint32_t, uint32_t, bool, const val&)>
                  (&msc::BasisTranscoder::transcode_slice))
        .function("transcodeSlice",
                  select_overload<bool(const val&, uint32_t, uint32_t, const val&,
                                  uint32_t, uint32_t, uint32_t, bool, bool, bool,
                                  uint32_t, uint32_t, uint32_t, uint32_t,
                                  bool, const val&, const val&, uint32_t)>
                  (&msc::BasisTranscoder::transcode_slice))
        ;

    class_<basisu_transcoder_state>("BasisTranscoderState")
        .constructor()
        ;
}

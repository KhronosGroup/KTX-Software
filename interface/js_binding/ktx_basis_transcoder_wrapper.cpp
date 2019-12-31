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

namespace ktx_basis_transcoder_wrapper {
    class ktxBasisTranscoder : public ktxBasisImageTranscoder {
      public:
        ktxBasisTranscoder() : ktxBasisImageTranscoder(buildSelectorCodebook())
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
        //                         @c ktxBasisImageDesc of the image to be
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
                             const bool transcode_alpha = false)
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
                             const val& xcoder_state,
                             bool transcode_alpha,
                             const val& alphadata,
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
                                                 nullptr, /* pState */
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

        static const uint32_t cETC1 = static_cast<uint32_t>(block_format::cETC1);
        static const uint32_t KTX_TTF_ETC1_RGB = ::KTX_TTF_ETC1_RGB;
#if BASISD_SUPPORT_DXT1
        static const uint32_t cBC1 = static_cast<uint32_t>(block_format::cBC1);
        static const uint32_t KTX_TTF_BC1_RGB = ::KTX_TTF_BC1_RGB;
#endif
#if BASISD_SUPPORT_DXT5A
        static const uint32_t cBC4 = static_cast<uint32_t>(block_format::cBC4);
        static const uint32_t KTX_TTF_BC4_R = ::KTX_TTF_BC4_R;
        static const uint32_t KTX_TTF_BC5_RG = ::KTX_TTF_BC5_RG;
#endif
#if BASISD_SUPPORT_DXT1 && BASISD_SUPPORT_DXT5A
        static const uint32_t KTX_TTF_BC3_RGBA = ::KTX_TTF_BC3_RGBA;
#endif
#if BASISD_SUPPORT_PVRTC1
        static const uint32_t cPVRTC1_4_RGB = static_cast<uint32_t>(block_format::cPVRTC1_4_RGB);
        static const uint32_t cPVRTC1_4_RGBA = static_cast<uint32_t>(block_format::cPVRTC1_4_RGBA);
        static const uint32_t KTX_TTF_PVRTC1_4_RGB = ::KTX_TTF_PVRTC1_4_RGB;
        static const uint32_t KTX_TTF_PVRTC1_4_RGBA = ::KTX_TTF_PVRTC1_4_RGBA;
#endif
#if BASISD_SUPPORT_BC7_MODE6_OPAQUE_ONLY
        static const uint32_t cBC7_M6_OPAQUE_ONLY = static_cast<uint32_t>(block_format::cBC7_M6_OPAQUE_ONLY);
        static const uint32_t KTX_TTF_BC7_M6_RGB = ::KTX_TTF_BC7_M6_RGB;
#endif
#if BASISD_SUPPORT_BC7_MODE5
        static const uint32_t cBC7_M5_COLOR = static_cast<uint32_t>(block_format::cBC7_M5_COLOR);
        static const uint32_t cBC7_M5_ALPHA = static_cast<uint32_t>(block_format::cBC7_M5_ALPHA);
        static const uint32_t KTX_TTF_BC7_M5_RGBA = ::KTX_TTF_BC7_M5_RGBA;
#endif
#if BASISD_SUPPORT_ETC2_EAC_A8
        static const uint32_t cETC2_EAC_A8 = static_cast<uint32_t>(block_format::cETC2_EAC_A8);
        static const uint32_t KTX_TTF_ETC2_RGBA = ::KTX_TTF_ETC2_RGBA;
#endif
#if BASISD_SUPPORT_ASTC
        static const uint32_t cASTC_4x4 = static_cast<uint32_t>(block_format::cASTC_4x4);
        static const uint32_t KTX_TTF_ASTC_4x4_RGBA = ::KTX_TTF_ASTC_4x4_RGBA;
#endif
#if BASISD_SUPPORT_ATC
        static const uint32_t cATC_RGB = static_cast<uint32_t>(block_format::cATC_RGB);
        static const uint32_t cATC_RGBA_INTERPOLATED_ALPHA = static_cast<uint32_t>(block_format::cATC_RGBA_INTERPOLATED_ALPHA);
#endif
#if BASISD_SUPPORT_FXT1
        static const uint32_t cFXT1_RGB = static_cast<uint32_t>(block_format::cFXT1_RGB);
#endif
        static const uint32_t cIndices = static_cast<uint32_t>(block_format::cIndices);

        static const uint32_t cRGB32 = static_cast<uint32_t>(block_format::cRGB32);
        static const uint32_t cRGBA32 = static_cast<uint32_t>(block_format::cRGBA32);
        static const uint32_t cA32 = static_cast<uint32_t>(block_format::cA32);

        static const uint32_t cRGB565 = static_cast<uint32_t>(block_format::cRGB565);
        static const uint32_t cBGR565 = static_cast<uint32_t>(block_format::cBGR565);

        static const uint32_t cRGBA4444_COLOR = static_cast<uint32_t>(block_format::cRGBA4444_COLOR);
        static const uint32_t cRGBA4444_ALPHA = static_cast<uint32_t>(block_format::cRGBA4444_ALPHA);
        static const uint32_t cRGBA4444_COLOR_OPAQUE = static_cast<uint32_t>(block_format::cRGBA4444_COLOR_OPAQUE);

        static const uint32_t KTX_TTF_RGBA32 = ::KTX_TTF_RGBA32;
        static const uint32_t KTX_TTF_RGB565 = ::KTX_TTF_RGB565;
        static const uint32_t KTX_TTF_BGR565 = ::KTX_TTF_BGR565;
        static const uint32_t KTX_TTF_RGBA4444 = ::KTX_TTF_RGBA4444;
#if BASISD_SUPPORT_PVRTC2
        static const uint32_t cPVRTC2_4_RGB = static_cast<uint32_t>(block_format::cPVRTC2_4_RGB);
        static const uint32_t cPVRTC2_4_RGBA = static_cast<uint32_t>(block_format::cPVRTC2_4_RGBA);
        static const uint32_t KTX_TTF_PVRTC2_4_RGB = ::KTX_TTF_PVRTC2_4_RGB;
        static const uint32_t KTX_TTF_PVRTC2_4_RGBA = ::KTX_TTF_PVRTC2_4_RGBA;
#endif
#if BASISD_SUPPORT_ETC2_EAC_RG11
        static const uint32_t cETC2_EAC_R11 = static_cast<uint32_t>(block_format::cPVRTC2_4_RGB);
        static const uint32_t KTX_TTF_ETC2_EAC_R11 = ::KTX_TTF_ETC2_EAC_R11;
        static const uint32_t KTX_TTF_ETC2_EAC_RG11 = ::KTX_TTF_ETC2_EAC_RG11;
#endif
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

    basist::etc1_global_selector_codebook* ktxBasisTranscoder::pGlobal_codebook;
    const uint32_t ktxBasisTranscoder::cETC1;
}

EMSCRIPTEN_BINDINGS(ktx_wrappers)
{
    class_<ktx_basis_transcoder_wrapper::ktxBasisTranscoder>("BasisLowLevelTranscoder")
    .class_property("cPVRTC1_4_RGB", &ktx_basis_transcoder_wrapper::ktxBasisTranscoder::cETC1)
#if 0
        .class_property("KTX_TTF_ETC1_RGB", &ktx_wrappers::texture::KTX_TTF_ETC1_RGB)
        .class_property("KTX_TTF_ETC2_RGBA", &ktx_wrappers::texture::KTX_TTF_ETC2_RGBA)
        .class_property("KTX_TTF_BC1_RGB", &ktx_wrappers::texture::KTX_TTF_BC1_RGB)
        .class_property("KTX_TTF_BC3_RGBA", &ktx_wrappers::texture::KTX_TTF_BC3_RGBA)
        .class_property("KTX_TTF_BC4_R", &ktx_wrappers::texture::KTX_TTF_BC4_R)
        .class_property("KTX_TTF_BC5_RG", &ktx_wrappers::texture::KTX_TTF_BC5_RG)
        .class_property("KTX_TTF_BC7_M6_RGB", &ktx_wrappers::texture::KTX_TTF_BC7_M6_RGB)
        .class_property("KTX_TTF_BC7_M5_RGBA", &ktx_wrappers::texture::KTX_TTF_BC7_M5_RGBA)
        .class_property("KTX_TTF_PVRTC1_4_RGB", &ktx_wrappers::texture::KTX_TTF_PVRTC1_4_RGB)
        .class_property("KTX_TTF_PVRTC1_4_RGBA", &ktx_wrappers::texture::KTX_TTF_PVRTC1_4_RGBA)
        .class_property("KTX_TTF_ASTC_4x4_RGBA", &ktx_wrappers::texture::KTX_TTF_ASTC_4x4_RGBA)
        .class_property("KTX_TTF_PVRTC2_4_RGB", &ktx_wrappers::texture::KTX_TTF_PVRTC2_4_RGB)
        .class_property("KTX_TTF_PVRTC2_4_RGBA", &ktx_wrappers::texture::KTX_TTF_PVRTC2_4_RGBA)
        .class_property("KTX_TTF_ETC2_EAC_R11", &ktx_wrappers::texture::KTX_TTF_ETC2_EAC_R11)
        .class_property("KTX_TTF_ETC2_EAC_RG11", &ktx_wrappers::texture::KTX_TTF_ETC2_EAC_RG11)
        .class_property("KTX_TTF_RGBA32", &ktx_wrappers::texture::KTX_TTF_RGBA32)
        .class_property("KTX_TTF_RGB565", &ktx_wrappers::texture::KTX_TTF_RGB565)
        .class_property("KTX_TTF_BGR565", &ktx_wrappers::texture::KTX_TTF_BGR565)
        .class_property("KTX_TTF_RGBA4444", &ktx_wrappers::texture::KTX_TTF_RGBA4444)
        .class_property("KTX_TTF_ETC", &ktx_wrappers::texture::KTX_TTF_ETC)
        .class_property("KTX_TTF_BC1_OR_3", &ktx_wrappers::texture::KTX_TTF_BC1_OR_3)
#endif
        .constructor()
        .class_function("initTranscoder", basisu_transcoder_init)
        .class_function("writeOpaqueAlphaBlocks",
                        &ktx_basis_transcoder_wrapper::ktxBasisTranscoder::write_opaque_alpha_blocks)
        .function("decodePalettes", &ktx_basis_transcoder_wrapper::ktxBasisTranscoder::decode_palettes)
        .function("decodeTables", &ktx_basis_transcoder_wrapper::ktxBasisTranscoder::decode_tables)
        .function("transcodeImage", &ktx_basis_transcoder_wrapper::ktxBasisTranscoder::transcode_image)
        .function("transcodeSlice",
                  select_overload<bool(const val&, uint32_t, uint32_t, const val&,
                                  uint32_t, uint32_t, uint32_t, bool, bool, bool,
                                  uint32_t, uint32_t, uint32_t, bool)>
                  (&ktx_basis_transcoder_wrapper::ktxBasisTranscoder::transcode_slice))
#if 0
        .function("transcodeSlice",
                  select_overload<bool(const val&, uint32_t, uint32_t, const val&,
                                  uint32_t, uint32_t, uint32_t, bool,
                                  uint32_t, uint32_t, bool)>
                  (&ktx_basis_transcoder_wrapper::ktxBasisTranscoder::transcode_slice))
#endif
        .function("transcodeSlice",
                  select_overload<bool(const val&, uint32_t, uint32_t, const val&,
                                  uint32_t, uint32_t, uint32_t, bool, bool, bool,
                                  uint32_t, uint32_t, uint32_t, uint32_t,
                                  const val&, bool, const val&, uint32_t)>
                  (&ktx_basis_transcoder_wrapper::ktxBasisTranscoder::transcode_slice))
#if 0
        .function("transcodeSlice",
                  select_overload<bool(const val&, uint32_t, uint32_t, const val&,
                                  uint32_t, uint32_t, uint32_t, bool,
                                  uint32_t, uint32_t,
                                  bool, const val&)>
                  (&ktx_basis_transcoder_wrapper::ktxBasisTranscoder::transcode_slice))
#endif
        ;
}

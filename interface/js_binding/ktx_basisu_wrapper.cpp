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
#include <basisu_transcoder.h>

using namespace emscripten;
using namespace basist;

namespace ktx_basisu_wrapper {
    class lowlevel_transcoder : public basisu_lowlevel_transcoder {
      public:
        lowlevel_transcoder() : basisu_lowlevel_transcoder(buildSelectorCodebook())
        { }

#if 0
        lowlevel_transcoder(const val& data)
            : basisu_lowlevel_transcoder(convertSelectorCodebook(data))
        { }
#endif

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

            return basisu_lowlevel_transcoder::decode_palettes(num_endpoints,
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

            return basisu_lowlevel_transcoder::decode_tables(table_data.data(),
                                                             table_data.size());
        }

        // @brief Transcode a BasisU encoded non-video slice.
        //
        // This is for the majority of target formats for which there is no need
        // to provide a buffer of alpha data to be included in the final image.
        bool transcode_slice(const val& dst,
                             uint32_t num_blocks_x, uint32_t num_blocks_y,
                             const val& imagedata, uint32_t imagedata_size,
                             uint32_t target_fmt,
                             uint32_t output_block_or_pixel_stride_in_bytes,
                             bool bc1_allow_threecolor_blocks,
                             const uint32_t orig_width, const uint32_t orig_height,
                             const bool transcode_alpha = false)
        {
            return transcode_slice(dst, num_blocks_x, num_blocks_y,
                                   imagedata, imagedata_size,
                                   target_fmt,
                                   output_block_or_pixel_stride_in_bytes,
                                   bc1_allow_threecolor_blocks,
                                   // video_flag, alpha_flag, level_index,
                                   false, false, 0,
                                   orig_width, orig_height,
                                   transcode_alpha);
        }

        // @brief Transcode a BasisU encoded non-video slice while merging in
        //        pre-decoded alpha data.
        //
        // This is for target formats which require a temporary buffer
        // containing previously transcode alpha data.
        bool transcode_slice(const val& dst,
                             uint32_t num_blocks_x, uint32_t num_blocks_y,
                             const val& imagedata, uint32_t imagedata_size,
                             uint32_t target_fmt,
                             uint32_t output_block_or_pixel_stride_in_bytes,
                             bool bc1_allow_threecolor_blocks,
                             const uint32_t orig_width, const uint32_t orig_height,
                             const bool transcode_alpha,
                             const val& alphadata)
        {
            val* xcoder_state = 0;

            return transcode_slice(dst, num_blocks_x, num_blocks_y,
                                   imagedata, imagedata_size,
                                   target_fmt,
                                   output_block_or_pixel_stride_in_bytes,
                                   bc1_allow_threecolor_blocks,
                                   // video_flag, alpha_flag, level_index,
                                   false, false, 0,
                                   orig_width, orig_height,
                                   0,
                                   *xcoder_state,
                                   transcode_alpha,
                                   alphadata);
        }

        // @brief Transcode a BasisU encoded non-video slice.
        //
        // This is for the majority of target formats for which there is no need
        // to provide a buffer of alpha data to be included in the final image.
        // Despite their names, alpha_flag and level_index are only used for
        // video slices.
        //
        // This is customized for use transcoding data from KTX2 files and
        // uploading to WebGL:
        // - It does not provide a way to change the output row pitch. Output
        //   data will always be tightly packed.
        // - It does not provide a way to adjust the height of destination
        //   image. output_rows_in_pixels is ignored by the base class function
        //   except when transcoding to RGBA32 and only to specify that output
        //   height should be num_blocks_y * 4. We prefer output to the original
        //   height.
        bool transcode_slice(const val& dst, uint32_t num_blocks_x, uint32_t num_blocks_y,
                             const val& imagedata, uint32_t imagedata_size,
                             uint32_t target_fmt, uint32_t output_block_or_pixel_stride_in_bytes,
                             bool bc1_allow_threecolor_blocks,
                             const bool video_flag, const bool alpha_flag,
                             const uint32_t level_index,
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

            bool status = basisu_lowlevel_transcoder::transcode_slice(dst_data.data(),
                                                 num_blocks_x, num_blocks_y,
                                                 image_data.data(), image_data.size(),
                                                 target_format,
                                                 output_block_or_pixel_stride_in_bytes,
                                                 bc1_allow_threecolor_blocks,
                                                 video_flag, alpha_flag,
                                                 level_index,
                                                 orig_width, orig_height,
                                                 0, /* output_row_pitch_in_blocks_or_pixels */
                                                 nullptr, transcode_alpha);

            memoryView = val::global("Uint8Array").new_(memory,
                                                 reinterpret_cast<uintptr_t>(dst_data.data()),
                                                 dst_data.size());

            dst.call<void>("set", memoryView);

            return status;
        }


        // @brief Transcode a BasisU encoded video slice while merging in
        //        pre-decoded alpha data.
        //
        // This is for target formats which require a temporary buffer
        // containing previously transcode alpha data.
        //
        // @sa transcode_slice
        bool transcode_slice(const val& dst, uint32_t num_blocks_x, uint32_t num_blocks_y,
                             const val& imagedata, uint32_t imagedata_size,
                             const uint32_t target_fmt, uint32_t output_block_or_pixel_stride_in_bytes,
                             bool bc1_allow_threecolor_blocks,
                             const bool video_flag, const bool alpha_flag,
                             const uint32_t level_index,
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

            bool status = basisu_lowlevel_transcoder::transcode_slice(dst_data.data(),
                                                 num_blocks_x, num_blocks_y,
                                                 image_data.data(), image_data.size(),
                                                 target_format,
                                                 output_block_or_pixel_stride_in_bytes,
                                                 bc1_allow_threecolor_blocks,
                                                 video_flag, alpha_flag,
                                                 level_index,
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

      protected:
        static basist::etc1_global_selector_codebook*
        buildSelectorCodebook()
        {
            static basist::etc1_global_selector_codebook* pGlobal_codebook;
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
        static const uint32_t cETC1 = static_cast<uint32_t>(block_format::cETC1);
        static const uint32_t cBC1 = static_cast<uint32_t>(block_format::cBC1);
        static const uint32_t cBC4 = static_cast<uint32_t>(block_format::cBC4);
        static const uint32_t cPVRTC1_4_RGB = static_cast<uint32_t>(block_format::cPVRTC1_4_RGB);
        static const uint32_t cPVRTC1_4_RGBA = static_cast<uint32_t>(block_format::cPVRTC1_4_RGBA);
#if !BASISD_SUPPORT_BC7_MODE6_OPAQUE_ONLY
        static const uint32_t cBC7_M6_OPAQUE_ONLY = static_cast<uint32_t>(block_format::cBC7_M6_OPAQUE_ONLY);
#endif
#if !BASISD_SUPPORT_BC7_MODE5
        static const uint32_t cBC7_M5_COLOR = static_cast<uint32_t>(block_format::cBC7_M5_COLOR);
        static const uint32_t cBC7_M5_ALPHA = static_cast<uint32_t>(block_format::cBC7_M5_ALPHA);
#endif
        static const uint32_t cETC2_EAC_A8 = static_cast<uint32_t>(block_format::cETC2_EAC_A8);
        static const uint32_t cASTC_4x4 = static_cast<uint32_t>(block_format::cASTC_4x4);

#if !BASISD_SUPPORT_ATC
        static const uint32_t cATC_RGB = static_cast<uint32_t>(block_format::cATC_RGB);
        static const uint32_t cATC_RGBA_INTERPOLATED_ALPHA = static_cast<uint32_t>(block_format::cATC_RGBA_INTERPOLATED_ALPHA);
#endif
#if !BASISD_SUPPORT_FXT1
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

#if !BASISD_SUPPORT_PVRTC2
        static const uint32_t cPVRTC2_4_RGB = static_cast<uint32_t>(block_format::cPVRTC2_4_RGB);
        static const uint32_t cPVRTC2_4_RGBA = static_cast<uint32_t>(block_format::cPVRTC2_4_RGBA);
#endif
#if !BASISD_SUPPORT_ETC2_EAC_RG11
        static const uint32_t cETC2_EAC_R11 = static_cast<uint32_t>(block_format::cPVRTC2_4_RGB);
#endif
    };
}

EMSCRIPTEN_BINDINGS(ktx_wrappers)
{
    class_<ktx_basisu_wrapper::lowlevel_transcoder>("BasisLowLevelTranscoder")
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
                        &ktx_basisu_wrapper::lowlevel_transcoder::write_opaque_alpha_blocks)
        .function("decodePalettes", &ktx_basisu_wrapper::lowlevel_transcoder::decode_palettes)
        .function("decodeTables", &ktx_basisu_wrapper::lowlevel_transcoder::decode_tables)
        .function("transcodeSlice",
                  select_overload<bool(const val&, uint32_t, uint32_t, const val&,
                                  uint32_t, uint32_t, uint32_t, bool, bool, bool,
                                  uint32_t, uint32_t, uint32_t, bool)>
                  (&ktx_basisu_wrapper::lowlevel_transcoder::transcode_slice))
        .function("transcodeSlice",
                  select_overload<bool(const val&, uint32_t, uint32_t, const val&,
                                  uint32_t, uint32_t, uint32_t, bool,
                                  uint32_t, uint32_t, bool)>
                  (&ktx_basisu_wrapper::lowlevel_transcoder::transcode_slice))
        .function("transcodeSlice",
                  select_overload<bool(const val&, uint32_t, uint32_t, const val&,
                                  uint32_t, uint32_t, uint32_t, bool, bool, bool,
                                  uint32_t, uint32_t, uint32_t, uint32_t,
                                  const val&, bool, const val&, uint32_t)>
                  (&ktx_basisu_wrapper::lowlevel_transcoder::transcode_slice))
        .function("transcodeSlice",
                  select_overload<bool(const val&, uint32_t, uint32_t, const val&,
                                  uint32_t, uint32_t, uint32_t, bool,
                                  uint32_t, uint32_t,
                                  bool, const val&)>
                  (&ktx_basisu_wrapper::lowlevel_transcoder::transcode_slice))
        ;
}

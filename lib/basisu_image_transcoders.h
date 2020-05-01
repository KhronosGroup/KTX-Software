/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab textwidth=70: */

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

/**
 * @internal
 * @file basis_image_transcoder.h
 * @~English
 *
 * @brief Declare BasisImageTranscoder class.
 */

#ifndef _BASIS_IMAGE_TRANSCODER_H_
#define _BASIS_IMAGE_TRANSCODER_H_

#include <ktx.h>
#include "basis_sgd.h"
#include "basisu/transcoder/basisu_transcoder.h"

using namespace basist;

/**
 * @class ktxBasisImageTranscoder
 * Transcode a single Basis Universal image.
 */
class ktxBasisImageTranscoder : protected basisu_lowlevel_etc1s_transcoder {
  public:
      /**
       * @~English
       * Constructor
       * @param[in] gcb the etc1_global_selector_codebook to use.
       */
      ktxBasisImageTranscoder(basist::etc1_global_selector_codebook* gcb)
           : basisu_lowlevel_etc1s_transcoder(gcb) { }
      // This is documented with the implementation.
      KTX_error_code transcode_image(const ktxBasisImageDesc& image,
                                     transcoder_texture_format targetFormat,
                                     uint8_t* dstBufferPtr,
                                     size_t bufferByteLength,
                                     uint32_t level,
                                     const uint8_t* levelDataPtr,
                                     uint32_t width, uint32_t height,
                                     uint32_t num_blocks_x,
                                     uint32_t num_blocks_y,
                                     bool isVideo = false,
                                     bool transcodeAlphaToOpaqueFormats = false,
                                     basisu_transcoder_state* pState = nullptr);

    KTX_error_code transcode_image(const ktxBasisImageDesc& image,
                                   ktx_transcode_fmt_e targetFormat,
                                   uint8_t* dstBufferPtr,
                                   size_t dstBufferByteLength,
                                   uint32_t level,
                                   const ktx_uint8_t* levelDataPtr,
                                   uint32_t width, uint32_t height,
                                   uint32_t num_blocks_x,
                                   uint32_t num_blocks_y,
                                   bool isVideo = false,
                                   bool transcodeAlphaToOpaqueFormats = false,
                                   basisu_transcoder_state* pState = nullptr) {
        if (targetFormat >= KTX_TTF_ETC) {
            // Only real format values can be accepted here.
            return KTX_INVALID_VALUE;
        }
        return transcode_image(image,
                               (transcoder_texture_format)targetFormat,
                               dstBufferPtr, dstBufferByteLength,
                               level, levelDataPtr,
                               width, height,
                               num_blocks_x, num_blocks_y,
                               isVideo, transcodeAlphaToOpaqueFormats,
                               pState);
    }

    bool decode_palettes(uint32_t num_endpoints, const uint8_t* pEndpoints_data,
                         uint32_t endpoints_data_size,
                         uint32_t num_selectors, const uint8_t* pSelectors_data,
                         uint32_t selectors_data_size) {
        return basisu_lowlevel_etc1s_transcoder::decode_palettes(num_endpoints,
                                                        pEndpoints_data,
                                                        endpoints_data_size,
                                                        num_selectors,
                                                        pSelectors_data,
                                                        selectors_data_size);
    }

    bool decode_tables(const uint8_t* pTable_data,
                        uint32_t table_data_size) {
        return basisu_lowlevel_etc1s_transcoder::decode_tables(pTable_data,
                                                        table_data_size);
    }
};

/**
 * @class ktxUastcImageTranscoder
 * Transcode a single Basis Universal image.
 */
class ktxUastcImageTranscoder : protected basisu_lowlevel_uastc_transcoder {
  public:
    // This is documented with the implementation.
    KTX_error_code transcode_image(transcoder_texture_format targetFormat,
                                   uint8_t* dstBufferPtr,
                                   size_t dstBufferByteLength,
                                   uint32_t level,
                                   const uint8_t* inImagePtr,
                                   const uint32_t inImageByteLength,
                                   uint32_t width, uint32_t height,
                                   uint32_t num_blocks_x,
                                   uint32_t num_blocks_y,
                                   bool hasAlpha = false,
                                   uint32_t decode_flags = 0,
                                   basisu_transcoder_state* pState = nullptr);

   KTX_error_code transcode_image(ktx_transcode_fmt_e targetFormat,
                                  uint8_t* dstBufferPtr,
                                  size_t dstBufferByteLength,
                                  uint32_t level,
                                  const uint8_t* inImagePtr,
                                  const uint32_t inImageByteLength,
                                  uint32_t width, uint32_t height,
                                  uint32_t num_blocks_x,
                                  uint32_t num_blocks_y,
                                  bool hasAlpha = false,
                                  ktx_transcode_flags transcode_flags = 0,
                                  basisu_transcoder_state* pState = nullptr) {
       if (targetFormat >= KTX_TTF_ETC) {
           // Only real format values can be accepted here.
           return KTX_INVALID_VALUE;
       }
       return transcode_image((transcoder_texture_format)targetFormat,
                              dstBufferPtr, dstBufferByteLength,
                              level,
                              inImagePtr, inImageByteLength,
                              width, height,
                              num_blocks_x, num_blocks_y,
                              hasAlpha, (uint32_t)transcode_flags,
                              pState);
   }
};

#endif /* _BASIS_IMAGE_TRANSCODER_H_ */

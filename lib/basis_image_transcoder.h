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
class ktxBasisImageTranscoder : public basisu_lowlevel_etc1s_transcoder {
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
                                   ktx_transcode_fmt_e targetFormat,
                                   ktx_uint8_t* writePtr,
                                   ktx_size_t bufferByteLength,
                                   uint32_t level,
                                   ktx_uint8_t* levelDataPtr,
                                   uint32_t width, uint32_t height,
                                   uint32_t num_blocks_x,
                                   uint32_t num_blocks_y,
                                   bool isVideo = false,
                                   bool transcodeAlphaToOpaqueFormats = false,
                                   basisu_transcoder_state* pState = nullptr);
};

#endif /* _BASIS_IMAGE_TRANSCODER_H_ */

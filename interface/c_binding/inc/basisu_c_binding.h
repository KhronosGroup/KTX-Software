// Copyright (c) 2019 Andreas Atteneder, All Rights Reserved.

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <basisu_transcoder.h>

using namespace basist;

extern basist::etc1_global_selector_codebook *g_pGlobal_codebook;

class basis_file
{
    int m_magic = 0;
    basisu_transcoder m_transcoder;
    const uint8_t *m_file;
    size_t byteLength;
    
public:
    basis_file()
    :
    m_transcoder(g_pGlobal_codebook)
    {}
    
    bool open(const uint8_t *buffer, size_t newByteLength);
    void close();
    uint32_t getHasAlpha();
    uint32_t getNumImages();
    uint32_t getNumLevels(uint32_t image_index);
    uint32_t getImageWidth(uint32_t image_index, uint32_t level_index);
    uint32_t getImageHeight(uint32_t image_index, uint32_t level_index);
    uint32_t getImageTranscodedSizeInBytes(uint32_t image_index, uint32_t level_index, uint32_t format);
    uint32_t startTranscoding();
    uint32_t transcodeImage(void* dst, size_t dst_size, uint32_t image_index, uint32_t level_index, uint32_t format, uint32_t pvrtc_wrap_addressing, uint32_t get_alpha_for_opaque_formats);
};
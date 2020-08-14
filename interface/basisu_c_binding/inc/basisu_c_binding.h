// Copyright 2019 Andreas Atteneder, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <basisu_transcoder.h>

using namespace basist;

extern basist::etc1_global_selector_codebook *g_pGlobal_codebook;

class basis_file
{
    int m_magic = 0;
    basisu_transcoder m_transcoder;
    const uint8_t *m_file;
    uint32_t byteLength;
    
public:
    basis_file()
    :
    m_transcoder(g_pGlobal_codebook)
    {}
    
    bool open(const uint8_t *buffer, uint32_t newByteLength);
    void close();
    uint32_t getHasAlpha();
    uint32_t getNumImages();
    uint32_t getNumLevels(uint32_t image_index);
    uint32_t getImageWidth(uint32_t image_index, uint32_t level_index);
    uint32_t getImageHeight(uint32_t image_index, uint32_t level_index);
    uint32_t getImageTranscodedSizeInBytes(uint32_t image_index, uint32_t level_index, uint32_t format);
    uint32_t startTranscoding();
    uint32_t transcodeImage(void* dst, uint32_t dst_size, uint32_t image_index, uint32_t level_index, uint32_t format, uint32_t pvrtc_wrap_addressing, uint32_t get_alpha_for_opaque_formats);
};

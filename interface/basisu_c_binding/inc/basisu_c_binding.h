// Copyright 2019 Andreas Atteneder, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#if defined(KHRONOS_STATIC)
  #define KTX_BASISU_API
#elif defined(_WIN32)
  #if !defined(KTX_BASISU_API)
    #define KTX_BASISU_API __declspec(dllimport)
  #endif
#elif defined(__ANDROID__)
  #define KTX_BASISU_API __attribute__((visibility("default")))
#else
  #define KTX_BASISU_API
#endif

#include <basisu_transcoder.h>

using namespace basist;

class basis_file
{
    unsigned int m_magic = 0;
    basisu_transcoder m_transcoder;
    const uint8_t *m_file;
    uint32_t byteLength;
    basisu_file_info fileinfo;

public:
    basis_file()
    :
    m_transcoder()
    {}
    
    bool open(const uint8_t *buffer, uint32_t newByteLength);
    void close();
    uint32_t getHasAlpha();
    uint32_t getNumImages();
    uint32_t getNumLevels(uint32_t image_index);
    uint32_t getImageWidth(uint32_t image_index, uint32_t level_index);
    uint32_t getImageHeight(uint32_t image_index, uint32_t level_index);
    uint32_t getYFlip();
    uint32_t getIsEtc1s();
    basis_texture_type getTextureType();
    uint32_t getImageTranscodedSizeInBytes(uint32_t image_index, uint32_t level_index, uint32_t format);
    uint32_t startTranscoding();
    uint32_t transcodeImage(void* dst, uint32_t dst_size, uint32_t image_index, uint32_t level_index, uint32_t format, uint32_t pvrtc_wrap_addressing, uint32_t get_alpha_for_opaque_formats);
};

extern "C" {
KTX_BASISU_API void ktx_basisu_basis_init();
#ifdef KTX_BASISU_C_BINDINGS
KTX_BASISU_API basis_file* ktx_basisu_create_basis();
KTX_BASISU_API uint32_t ktx_basisu_open_basis( basis_file* basis, const uint8_t * data, uint32_t length );
KTX_BASISU_API void ktx_basisu_close_basis( basis_file* basis );
KTX_BASISU_API void ktx_basisu_delete_basis( basis_file* basis );
KTX_BASISU_API uint32_t ktx_basisu_getHasAlpha( basis_file* basis );
KTX_BASISU_API uint32_t ktx_basisu_getNumImages( basis_file* basis );
KTX_BASISU_API uint32_t ktx_basisu_getNumLevels( basis_file* basis, uint32_t image_index);
KTX_BASISU_API uint32_t ktx_basisu_getImageWidth( basis_file* basis, uint32_t image_index, uint32_t level_index);
KTX_BASISU_API uint32_t ktx_basisu_getImageHeight( basis_file* basis, uint32_t image_index, uint32_t level_index);
KTX_BASISU_API uint32_t ktx_basisu_get_y_flip( basis_file* basis );
KTX_BASISU_API uint32_t ktx_basisu_get_is_etc1s( basis_file* basis );
KTX_BASISU_API basis_texture_type ktx_basisu_get_texture_type( basis_file* basis );
KTX_BASISU_API uint32_t ktx_basisu_getImageTranscodedSizeInBytes( basis_file* basis, uint32_t image_index, uint32_t level_index, uint32_t format);
KTX_BASISU_API uint32_t ktx_basisu_startTranscoding( basis_file* basis );
KTX_BASISU_API uint32_t ktx_basisu_transcodeImage( basis_file* basis, void* dst, uint32_t dst_size, uint32_t image_index, uint32_t level_index, uint32_t format, uint32_t pvrtc_wrap_addressing, uint32_t get_alpha_for_opaque_formats);
#endif
}

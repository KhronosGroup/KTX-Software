// Copyright 2019 Andreas Atteneder, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef DLL_EXPORT_FLAG
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

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

extern "C" {
DLL_EXPORT void ktx_basisu_basis_init();
#ifdef KTX_BASISU_C_BINDINGS
DLL_EXPORT basis_file* ktx_basisu_create_basis();
DLL_EXPORT bool ktx_basisu_open_basis( basis_file* basis, const uint8_t * data, uint32_t length );
DLL_EXPORT void ktx_basisu_close_basis( basis_file* basis );
DLL_EXPORT void ktx_basisu_delete_basis( basis_file* basis );
DLL_EXPORT bool ktx_basisu_getHasAlpha( basis_file* basis );
DLL_EXPORT uint32_t ktx_basisu_getNumImages( basis_file* basis );
DLL_EXPORT uint32_t ktx_basisu_getNumLevels( basis_file* basis, uint32_t image_index);
DLL_EXPORT uint32_t ktx_basisu_getImageWidth( basis_file* basis, uint32_t image_index, uint32_t level_index);
DLL_EXPORT uint32_t ktx_basisu_getImageHeight( basis_file* basis, uint32_t image_index, uint32_t level_index);
DLL_EXPORT uint32_t ktx_basisu_getImageTranscodedSizeInBytes( basis_file* basis, uint32_t image_index, uint32_t level_index, uint32_t format);
DLL_EXPORT bool ktx_basisu_startTranscoding( basis_file* basis );
DLL_EXPORT bool ktx_basisu_transcodeImage( basis_file* basis, void* dst, uint32_t dst_size, uint32_t image_index, uint32_t level_index, uint32_t format, uint32_t pvrtc_wrap_addressing, uint32_t get_alpha_for_opaque_formats);
#endif
}

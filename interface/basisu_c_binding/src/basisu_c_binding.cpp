// Copyright 2019 Andreas Atteneder, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <basisu_transcoder.h>

#include "basisu_c_binding.h"
#include "unused.h"

using namespace basist;

basist::etc1_global_selector_codebook *g_pGlobal_codebook;

#define MAGIC 0xDEADBEE1

bool basis_file::open(const uint8_t *buffer, uint32_t newByteLength) {
    m_file = buffer;
    byteLength = newByteLength;

    if (!m_transcoder.validate_header(buffer, newByteLength)) {
        m_file = nullptr;
        byteLength = 0;
        return false;
    }
    
    // Initialized after validation
    m_magic = MAGIC;
    return true;
}

void basis_file::close() {
    assert(m_magic == MAGIC);
    m_file = nullptr;
    byteLength = 0;
}

uint32_t basis_file::getHasAlpha() {
    assert(m_magic == MAGIC);
    if (m_magic != MAGIC)
        return 0;
    
    basisu_image_level_info li;
    if (!m_transcoder.get_image_level_info(m_file, byteLength, li, 0, 0))
        return 0;
    
    return li.m_alpha_flag;
}

uint32_t basis_file::getNumImages() {
    assert(m_magic == MAGIC);
    if (m_magic != MAGIC)
        return 0;
    
    return m_transcoder.get_total_images(m_file, byteLength);
}

uint32_t basis_file::getNumLevels(uint32_t image_index) {
    assert(m_magic == MAGIC);
    if (m_magic != MAGIC)
        return 0;
    
    basisu_image_info ii;
    if (!m_transcoder.get_image_info(m_file, byteLength, ii, image_index))
        return 0;
    
    return ii.m_total_levels;
}

uint32_t basis_file::getImageWidth(uint32_t image_index, uint32_t level_index) {
    assert(m_magic == MAGIC);
    if (m_magic != MAGIC)
        return 0;
    
    uint32_t orig_width, orig_height, total_blocks;
    if (!m_transcoder.get_image_level_desc(m_file, byteLength,
                                           image_index, level_index,
                                           orig_width, orig_height,
                                           total_blocks))
        return 0;
    
    return orig_width;
}

uint32_t basis_file::getImageHeight(uint32_t image_index, uint32_t level_index) {
    assert(m_magic == MAGIC);
    if (m_magic != MAGIC)
        return 0;
    
    uint32_t orig_width, orig_height, total_blocks;
    if (!m_transcoder.get_image_level_desc(m_file, byteLength,
                                           image_index, level_index,
                                           orig_width, orig_height,
                                           total_blocks))
        return 0;
    
    return orig_height;
}

uint32_t basis_file::getImageTranscodedSizeInBytes(uint32_t image_index, uint32_t level_index, uint32_t format) {
    assert(m_magic == MAGIC);
    if (m_magic != MAGIC)
        return 0;
    
    if (format >= (uint32_t) basist::transcoder_texture_format::cTFTotalTextureFormats)
        return 0;
    
    uint32_t orig_width, orig_height, total_blocks;
    if (!m_transcoder.get_image_level_desc(m_file, (uint32_t)byteLength,
                                           image_index, level_index, orig_width,
                                           orig_height, total_blocks))
        return 0;

    const transcoder_texture_format transcoder_format = static_cast<transcoder_texture_format>(format);

    if (basis_transcoder_format_is_uncompressed(transcoder_format))
    {
        // Uncompressed formats are just plain raster images.
        const uint32_t bytes_per_pixel = basis_get_uncompressed_bytes_per_pixel(transcoder_format);
        const uint32_t bytes_per_line = orig_width * bytes_per_pixel;
        const uint32_t bytes_per_slice = bytes_per_line * orig_height;
        return bytes_per_slice;
    }
    else
    {
        // Compressed formats are 2D arrays of blocks.
        const uint32_t bytes_per_block = basis_get_bytes_per_block_or_pixel(transcoder_format);

        if (transcoder_format == transcoder_texture_format::cTFPVRTC1_4_RGB || transcoder_format == transcoder_texture_format::cTFPVRTC1_4_RGBA)
        {
            // For PVRTC1, Basis only writes (or requires) total_blocks * bytes_per_block. But GL requires extra padding for very small textures: 
            // https://www.khronos.org/registry/OpenGL/extensions/IMG/IMG_texture_compression_pvrtc.txt
            const uint32_t width = (orig_width + 3) & ~3;
            const uint32_t height = (orig_height + 3) & ~3;
            const uint32_t size_in_bytes = (std::max(8U, width) * std::max(8U, height) * 4 + 7) / 8;
            return size_in_bytes;
        }

        return total_blocks * bytes_per_block;
    }
}

uint32_t basis_file::startTranscoding() {
    assert(m_magic == MAGIC);
    if (m_magic != MAGIC)
        return 0;
    
    return m_transcoder.start_transcoding(m_file, byteLength);
}

uint32_t basis_file::transcodeImage(void* dst, uint32_t dst_size, uint32_t image_index, uint32_t level_index, uint32_t format, uint32_t pvrtc_wrap_addressing, uint32_t get_alpha_for_opaque_formats) {
    assert(m_magic == MAGIC);
    if (m_magic != MAGIC)
        return 0;
    
    if (format >= (uint32_t) basist::transcoder_texture_format::cTFTotalTextureFormats)
        return 0;
    
    const transcoder_texture_format transcoder_format = static_cast<transcoder_texture_format>(format);

    uint32_t orig_width, orig_height, total_blocks;
    if (!m_transcoder.get_image_level_desc(m_file, byteLength, image_index, level_index, orig_width, orig_height, total_blocks))
        return 0;

  uint32_t flags = get_alpha_for_opaque_formats ? cDecodeFlagsTranscodeAlphaDataToOpaqueFormats : 0;

    uint32_t status;

    if (basis_transcoder_format_is_uncompressed(transcoder_format))
    {
        const uint32_t MAYBE_UNUSED bytes_per_pixel = basis_get_uncompressed_bytes_per_pixel(transcoder_format);
        const uint32_t MAYBE_UNUSED bytes_per_line = orig_width * bytes_per_pixel;
        const uint32_t MAYBE_UNUSED bytes_per_slice = bytes_per_line * orig_height;

        assert(bytes_per_slice <= dst_size);

        status = m_transcoder.transcode_image_level(
            m_file, byteLength, image_index, level_index,
            dst, orig_width * orig_height,
            transcoder_format,
            flags,
            orig_width,
            nullptr,
            orig_height);
    }
    else
    {
        uint32_t bytes_per_block = basis_get_bytes_per_block_or_pixel(transcoder_format);

        uint32_t required_size = total_blocks * bytes_per_block;

        if (transcoder_format == transcoder_texture_format::cTFPVRTC1_4_RGB || transcoder_format == transcoder_texture_format::cTFPVRTC1_4_RGBA)
        {
            // For PVRTC1, Basis only writes (or requires) total_blocks * bytes_per_block. But GL requires extra padding for very small textures: 
            // https://www.khronos.org/registry/OpenGL/extensions/IMG/IMG_texture_compression_pvrtc.txt
            // The transcoder will clear the extra bytes followed the used blocks to 0.
            const uint32_t width = (orig_width + 3) & ~3;
            const uint32_t height = (orig_height + 3) & ~3;
            required_size = (std::max(8U, width) * std::max(8U, height) * 4 + 7) / 8;
            assert(required_size >= total_blocks * bytes_per_block);
        }

        assert(required_size <= dst_size);

        status = m_transcoder.transcode_image_level(
            m_file, byteLength, image_index, level_index,
            dst, dst_size / bytes_per_block,
            static_cast<basist::transcoder_texture_format>(format),
            flags);
    }

    return status;
}

extern "C" {

DLL_EXPORT void ktx_basisu_basis_init()
{
    basisu_transcoder_init();
    
    if (!g_pGlobal_codebook)
        g_pGlobal_codebook = new basist::etc1_global_selector_codebook(g_global_selector_cb_size, g_global_selector_cb);
}

#ifdef KTX_BASISU_C_BINDINGS

DLL_EXPORT basis_file* ktx_basisu_create_basis() {
    basis_file* new_basis = new basis_file();
    return new_basis;
}
    
DLL_EXPORT bool ktx_basisu_open_basis( basis_file* basis, const uint8_t * data, uint32_t length ) {
    return basis->open(data,length);
}

DLL_EXPORT void ktx_basisu_close_basis( basis_file* basis ) {
    basis->close();
}
    
DLL_EXPORT void ktx_basisu_delete_basis( basis_file* basis ) {
    delete basis;
}

DLL_EXPORT bool ktx_basisu_getHasAlpha( basis_file* basis ) {
    assert(basis!=nullptr);
    return basis->getHasAlpha();
}

DLL_EXPORT uint32_t ktx_basisu_getNumImages( basis_file* basis ) {
    return basis->getNumImages();
}

DLL_EXPORT uint32_t ktx_basisu_getNumLevels( basis_file* basis, uint32_t image_index) {
    return basis->getNumLevels(image_index);
}

DLL_EXPORT uint32_t ktx_basisu_getImageWidth( basis_file* basis, uint32_t image_index, uint32_t level_index) {
    return basis->getImageWidth(image_index,level_index);
}

DLL_EXPORT uint32_t ktx_basisu_getImageHeight( basis_file* basis, uint32_t image_index, uint32_t level_index) {
    return basis->getImageHeight(image_index,level_index);
}

DLL_EXPORT uint32_t ktx_basisu_getImageTranscodedSizeInBytes( basis_file* basis, uint32_t image_index, uint32_t level_index, uint32_t format) {
    return basis->getImageTranscodedSizeInBytes(image_index,level_index,format);
}

DLL_EXPORT bool ktx_basisu_startTranscoding( basis_file* basis ) {
    return basis->startTranscoding();
}

DLL_EXPORT bool ktx_basisu_transcodeImage( basis_file* basis, void* dst, uint32_t dst_size, uint32_t image_index, uint32_t level_index, uint32_t format, uint32_t pvrtc_wrap_addressing, uint32_t get_alpha_for_opaque_formats) {
    return basis->transcodeImage(dst,dst_size,image_index,level_index,format,pvrtc_wrap_addressing,get_alpha_for_opaque_formats);
}

#endif

} // END extern "C" 

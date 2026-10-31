// Copyright 2026 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef _BASIS_TRANSCODE_H_
#define _BASIS_TRANSCODE_H_

/*
 * Internal interface shared between the Basis transcode paths:
 * ktxTexture2_TranscodeBasis (basis_transcode.cpp) and the streaming
 * ktxLevelProcessor (level_processor.cpp).
 */

#include <ktx.h>
#include "basis_sgd.h"
#include "vkformat_enum.h"

/*
 * Check that a texture is in a format ktxTexture2_TranscodeBasis can
 * transcode (ETC1S/BasisLZ or UASTC LDR/HDR) and carries the
 * supercompression global data its scheme requires. Returns KTX_SUCCESS
 * or KTX_INVALID_OPERATION.
 */
KTX_error_code
ktxTexture2_validateBasisSource(const ktxTexture2* This);

/*
 * Resolve a requested transcode target against a Basis-compressed source
 * texture: detect the source's alpha content, map the automatic selection
 * formats (KTX_TTF_ETC, KTX_TTF_BC1_OR_3, PVRTC RGBA variants) to their
 * concrete targets, resolve the target VkFormat (applying the source's
 * transfer function), validate PVRTC1 power-of-two dimensions and check
 * that the transcoder for the source/target pair is included in the build.
 *
 * On success *pOutputFormat holds the concrete target, *pVkFormat the
 * resolved VkFormat and *pAlphaContent the source's alpha content.
 */
KTX_error_code
ktxTexture2_resolveBasisTargetFormat(const ktxTexture2* This,
                                     ktx_transcode_fmt_e* pOutputFormat,
                                     VkFormat* pVkFormat,
                                     alpha_content_e* pAlphaContent);

/*
 * Perform the transcoder's process-wide initialization if it has not been
 * done yet. Thread-safe; a no-op once initialized.
 */
void
ktxInitBasisTranscoder(void);

/*
 * Per-level transcoding state for one Basis source and one target: the
 * resolved target, the target-layout prototype, the SGD image
 * descriptions and, for BasisLZ/ETC1S, the endpoint and selector palettes
 * and Huffman tables decoded once from the SGD. It does not depend on
 * where a level's bytes come from, so ktxTexture2_TranscodeBasis (bytes
 * from the loaded texture) and ktxLevelProcessor (bytes provided by the
 * caller) share it and with it every codec path.
 *
 * It borrows the source and the prototype, which must outlive it.
 */
typedef struct ktxBasisLevelTranscoder ktxBasisLevelTranscoder;

/*
 * Create the per-level transcoding state. @p outputFormat must already be
 * resolved (see ktxTexture2_resolveBasisTargetFormat) and @p prototype
 * must have been created for the resolved VkFormat. Validates the layout
 * of the SGD for the source's scheme and decodes the BasisLZ/ETC1S
 * palettes and tables. Returns KTX_FILE_DATA_ERROR for an invalid SGD
 * layout, KTX_OUT_OF_MEMORY on allocation failure. Palettes or tables
 * that fail to decode are reported by the level transcodes, before they
 * transcode anything, as KTX_TRANSCODE_FAILED.
 */
KTX_error_code
ktxBasisLevelTranscoder_create(const ktxTexture2* source,
                               ktxTexture2* prototype,
                               ktx_transcode_fmt_e outputFormat,
                               ktx_transcode_flags transcodeFlags,
                               alpha_content_e alphaContent,
                               ktxBasisLevelTranscoder** pTranscoder);

/*
 * Transcode every image of @p level (all layers, faces and depth slices)
 * into @p dst, packed as described by the prototype's layout for the
 * level with the first image at @p dst[0].
 *
 * @p levelData is the level's data in its unsupercompressed form: as
 * stored for BasisLZ, UASTC HDR 6x6 intermediate and unsupercompressed
 * UASTC, inflated for Zstd or ZLIB supercompressed UASTC. For UASTC HDR
 * 4x4 to ASTC HDR 4x4 the blocks are copied unchanged.
 *
 * Returns KTX_INVALID_VALUE for an invalid level or a @p dstCapacity
 * smaller than the level, KTX_FILE_DATA_ERROR when @p levelData is too
 * short for the level or describes images outside it, and
 * KTX_TRANSCODE_FAILED when the transcoder rejects an image.
 */
KTX_error_code
ktxBasisLevelTranscoder_transcodeLevel(ktxBasisLevelTranscoder* This,
                                       ktx_uint32_t level,
                                       const ktx_uint8_t* levelData,
                                       ktx_size_t levelDataSize,
                                       ktx_uint8_t* dst,
                                       ktx_size_t dstCapacity);

void
ktxBasisLevelTranscoder_destroy(ktxBasisLevelTranscoder* This);

#endif /* _BASIS_TRANSCODE_H_ */

/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2026 The Khronos Group Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file level_processor.cpp
 * @~English
 *
 * @brief ktxLevelProcessor: per-level processing of serialized KTX2 sources.
 *
 * The streaming counterpart of ktxTexture2_TranscodeBasis discussed in
 * https://github.com/KhronosGroup/KTX-Software/issues/1224: a consumer
 * constructs a metadata-only ktxTexture2 from the serialized file prefix,
 * fetches level payloads separately (see ktxTexture2_GetLevelFileInfo) and
 * processes them one level at a time, so peak memory is O(one level)
 * instead of O(whole file).
 *
 * This file implements the processor object, its validation and the
 * target-layout queries, which delegate to a private prototype texture
 * created with KTX_TEXTURE_CREATE_NO_STORAGE — the same prototype
 * technique ktxTexture2_TranscodeBasis uses, minus the full-mip-chain
 * allocation. ktxLevelProcessor_ProcessLevel is a stub in this commit;
 * the codec work lands separately.
 */

#include <assert.h>
#include <stdlib.h>

#include <KHR/khr_df.h>
#include "dfdutils/dfd.h"

#include "ktx.h"
#include "ktxint.h"
#include "texture2.h"
#include "vkformat_enum.h"
#include "basis_sgd.h"
#include "basis_transcode.h"
#include "level_processor.h"

/**
 * @memberof ktxLevelProcessor
 * @~English
 * @brief Create a processor for an ETC1S/UASTC source and a chosen target.
 *
 * Validates that @p source is Basis-transcodable and carries the global
 * data its scheme requires, resolves @p outputFormat exactly as
 * ktxTexture2_TranscodeBasis would (automatic selections such as
 * @c KTX_TTF_BC1_OR_3 are mapped using the source's alpha content, the
 * source's transfer function selects the sRGB or linear target variant)
 * and builds the private target-layout prototype the query functions
 * delegate to.
 *
 * The processor borrows @p source: the source must remain alive and
 * unmodified until ktxLevelProcessor_Destroy(). The source must retain
 * its serialized-source state, i.e. be a texture constructed from the
 * serialized file (typically only its metadata prefix) without
 * KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT and not loaded since:
 * ktxLevelProcessor_ProcessLevel() consumes the serialized level payload,
 * which only such a source can describe (see
 * ktxTexture2_GetLevelFileInfo()). A texture whose image data has been
 * loaded, or one created with ktxTexture2_Create(), is rejected with
 * KTX_INVALID_OPERATION.
 *
 * A processor is not safe for concurrent use; separate processor
 * instances may be used concurrently.
 *
 * Creating a processor also performs the transcoder's process-wide
 * initialization, shared with ktxTexture2_TranscodeBasis(), if it has not
 * been done yet. It is done once, not per processor, so a processor should
 * be reused for all the levels of a source rather than created per level.
 *
 * Video sources are recognized, as by ktxTexture2_TranscodeBasis(), from
 * the KTXanimData key in the source's key/value data, so the source must
 * have been constructed with its key/value data parsed (i.e. without
 * KTX_TEXTURE_CREATE_SKIP_KVDATA_BIT or KTX_TEXTURE_CREATE_RAW_KVDATA_BIT)
 * when the source contains key/value data. A source with unparsed key/value
 * data is rejected with KTX_INVALID_OPERATION because its video status is
 * unknown. These flags do not prevent processor creation when the source
 * has no key/value data.
 *
 * @param[in]  source         pointer to the Basis-compressed source texture.
 * @param[in]  outputFormat   a value from the ktx_texture_transcode_fmt_e
 *                            enum specifying the target format.
 * @param[in]  transcodeFlags bitfield of flags modifying the transcode
 *                            operation.
 * @param[out] newProcessor   pointer to location to write the new
 *                            processor's handle.
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p source or @p newProcessor is NULL or
 *                              @p outputFormat is invalid.
 * @exception KTX_INVALID_OPERATION
 *                              The source is not in a Basis-transcodable
 *                              format, is missing the supercompression
 *                              global data its scheme requires, has no
 *                              serialized-source state (it was created
 *                              with ktxTexture2_Create() or its image data
 *                              has been loaded), has unparsed key/value
 *                              data, or @p outputFormat is
 *                              PVRTC1 and the source does not have
 *                              power-of-two dimensions.
 * @exception KTX_UNSUPPORTED_FEATURE
 *                              The source is a video texture (not supported
 *                              in the initial version), a currently
 *                              unsupported @p transcodeFlags bit was
 *                              requested, or the transcoder for the
 *                              source/target pair is not included in the
 *                              library.
 * @exception KTX_FILE_DATA_ERROR
 *                              The source's DFD describes an invalid alpha
 *                              channel arrangement.
 * @exception KTX_OUT_OF_MEMORY Not enough memory to create the processor.
 */
KTX_error_code
ktxLevelProcessor_CreateBasis(const ktxTexture2* source,
                              ktx_transcode_fmt_e outputFormat,
                              ktx_transcode_flags transcodeFlags,
                              ktxLevelProcessor** newProcessor)
{
    if (source == nullptr || newProcessor == nullptr)
        return KTX_INVALID_VALUE;
    if (source->pDfd == nullptr || source->_private == nullptr)
        return KTX_INVALID_VALUE;

    // Same source-family gate and global-data check as
    // ktxTexture2_TranscodeBasis. Unknown colorModels (e.g. XUASTC once the
    // encoder emits it) fail here.
    KTX_error_code result = ktxTexture2_validateBasisSource(source);
    if (result != KTX_SUCCESS)
        return result;

    // ProcessLevel consumes the serialized level payload, which only a
    // source that retains its serialized-source state can describe (see
    // ktxTexture2_GetLevelFileInfo). Image data cannot start at file
    // offset 0, so a zero base marks a texture without that state.
    if (source->_private->_firstLevelFileOffset == 0)
        return KTX_INVALID_OPERATION;

    // SKIP/RAW KVD leaves isVideo false even if KTXanimData is present.
    // Require known video status before accepting an independent-level source.
    if (!source->_private->_kvDataIsParsed)
        return KTX_INVALID_OPERATION;

    // ETC1S video needs inter-frame transcoder state; agreed follow-up.
    if (source->isVideo)
        return KTX_UNSUPPORTED_FEATURE;

    if (transcodeFlags & KTX_TF_PVRTC_DECODE_TO_NEXT_POW2)
        return KTX_UNSUPPORTED_FEATURE;

    VkFormat vkFormat;
    alpha_content_e alphaContent;
    result =
        ktxTexture2_resolveBasisTargetFormat(source, &outputFormat, &vkFormat,
                                             &alphaContent);
    if (result != KTX_SUCCESS)
        return result;

    // Process-wide transcoder initialization, shared with
    // ktxTexture2_TranscodeBasis. Done once, not per processor.
    ktxInitBasisTranscoder();

    // The prototype provides the target layout for the query functions.
    // NO_STORAGE: the processor never allocates the complete output mip
    // chain — level destination buffers are caller-provided.
    ktxTextureCreateInfo createInfo;
    createInfo.glInternalformat = 0;
    createInfo.vkFormat = (ktx_uint32_t)vkFormat;
    createInfo.baseWidth = source->baseWidth;
    createInfo.baseHeight = source->baseHeight;
    createInfo.baseDepth = source->baseDepth;
    createInfo.generateMipmaps = source->generateMipmaps;
    createInfo.isArray = source->isArray;
    createInfo.numDimensions = source->numDimensions;
    createInfo.numFaces = source->numFaces;
    createInfo.numLayers = source->numLayers;
    createInfo.numLevels = source->numLevels;
    createInfo.pDfd = nullptr;

    ktxTexture2* prototype;
    result = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_NO_STORAGE,
                                &prototype);
    if (result != KTX_SUCCESS) {
        assert(result == KTX_OUT_OF_MEMORY); // The only run time error
        return result;
    }

    ktxLevelProcessor* processor =
        (ktxLevelProcessor*)malloc(sizeof(ktxLevelProcessor));
    if (processor == nullptr) {
        ktxTexture_Destroy(ktxTexture(prototype));
        return KTX_OUT_OF_MEMORY;
    }

    processor->source = source;
    processor->prototype = prototype;
    processor->outputFormat = outputFormat;
    processor->transcodeFlags = transcodeFlags;
    processor->alphaContent = alphaContent;
    *newProcessor = processor;
    return KTX_SUCCESS;
}

/**
 * @memberof ktxLevelProcessor
 * @~English
 * @brief Return the resolved output VkFormat.
 *
 * The concrete format processed levels will be in, with automatic
 * selections (e.g. @c KTX_TTF_BC1_OR_3, @c KTX_TTF_ETC) already resolved
 * against the source's alpha content and the source's transfer function
 * applied to select the sRGB or linear variant.
 *
 * @param[in] processor pointer to the processor. Must not be NULL.
 *
 * @return The resolved VkFormat as a ktx_uint32_t.
 */
ktx_uint32_t
ktxLevelProcessor_GetOutputVkFormat(const ktxLevelProcessor* processor)
{
    return processor->prototype->vkFormat;
}

/**
 * @memberof ktxLevelProcessor
 * @~English
 * @brief Return the size of one complete processed level.
 *
 * The number of bytes ktxLevelProcessor_ProcessLevel() writes for
 * @p level: all of the level's array layers, faces and depth slices in the
 * target format, with no inter-level padding.
 *
 * @param[in]  processor pointer to the processor.
 * @param[in]  level     mip level of interest.
 * @param[out] pSize     pointer to location to store the size.
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p processor or @p pSize is NULL or
 *                              @p level is not less than the source's
 *                              numLevels.
 */
KTX_error_code
ktxLevelProcessor_GetLevelSize(const ktxLevelProcessor* processor,
                               ktx_uint32_t level, ktx_size_t* pSize)
{
    if (processor == nullptr || pSize == nullptr)
        return KTX_INVALID_VALUE;
    if (level >= processor->prototype->numLevels)
        return KTX_INVALID_VALUE;

    *pSize = ktxTexture_calcLevelSize(ktxTexture(processor->prototype),
                                      level, KTX_FORMAT_VERSION_TWO);
    return KTX_SUCCESS;
}

/**
 * @memberof ktxLevelProcessor
 * @~English
 * @brief Return the size of one image within a processed level.
 *
 * The size in the target format of a single image — one face, array
 * layer or depth slice — of @p level.
 *
 * @param[in]  processor pointer to the processor.
 * @param[in]  level     mip level of interest.
 * @param[out] pSize     pointer to location to store the size.
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p processor or @p pSize is NULL or
 *                              @p level is not less than the source's
 *                              numLevels.
 */
KTX_error_code
ktxLevelProcessor_GetImageSize(const ktxLevelProcessor* processor,
                               ktx_uint32_t level, ktx_size_t* pSize)
{
    if (processor == nullptr || pSize == nullptr)
        return KTX_INVALID_VALUE;
    if (level >= processor->prototype->numLevels)
        return KTX_INVALID_VALUE;

    *pSize = ktxTexture2_GetImageSize(processor->prototype, level);
    return KTX_SUCCESS;
}

/**
 * @memberof ktxLevelProcessor
 * @~English
 * @brief Return an image's offset within a processed level's buffer.
 *
 * The offset of the image for (@p layer, @p faceSlice) relative to the
 * start of the destination buffer ktxLevelProcessor_ProcessLevel() fills
 * for @p level, so a consumer can upload an individual cubemap face,
 * array layer or depth slice from @c dst + offset.
 *
 * @param[in]  processor pointer to the processor.
 * @param[in]  level     mip level of the image.
 * @param[in]  layer     array layer of the image.
 * @param[in]  faceSlice cube map face or depth slice of the image.
 * @param[out] pOffset   pointer to location to store the offset.
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p processor or @p pOffset is NULL or
 *                              @p level, @p layer or @p faceSlice exceed
 *                              the dimensions of the texture.
 */
KTX_error_code
ktxLevelProcessor_GetImageOffset(const ktxLevelProcessor* processor,
                                 ktx_uint32_t level, ktx_uint32_t layer,
                                 ktx_uint32_t faceSlice, ktx_size_t* pOffset)
{
    if (processor == nullptr || pOffset == nullptr)
        return KTX_INVALID_VALUE;

    // Validate here, like the sibling queries, so all three report an
    // out-of-range argument as KTX_INVALID_VALUE.
    ktxTexture2* prototype = processor->prototype;
    if (level >= prototype->numLevels || layer >= prototype->numLayers)
        return KTX_INVALID_VALUE;
    if (prototype->isCubemap) {
        if (faceSlice >= prototype->numFaces)
            return KTX_INVALID_VALUE;
    } else if (faceSlice >= MAX(1, prototype->baseDepth >> level)) {
        return KTX_INVALID_VALUE;
    }

    // Delegate to the prototype, then rebase from mip-chain-relative to
    // level-relative: the level's destination buffer starts at its first
    // image.
    ktx_size_t imageOffset;
    KTX_error_code result = ktxTexture2_GetImageOffset(
        prototype, level, layer, faceSlice, &imageOffset);
    if (result != KTX_SUCCESS)
        return result;

    *pOffset = imageOffset - ktxTexture2_levelDataOffset(prototype, level);
    return KTX_SUCCESS;
}

/**
 * @memberof ktxLevelProcessor
 * @~English
 * @brief Process one complete level into a caller-provided buffer.
 *
 * @note Not yet implemented in this commit of the API skeleton; the codec
 * paths land in a follow-up commit of this PR series. Argument validation
 * matches the final contract so bindings can already be written against
 * it.
 *
 * @p src must hold exactly the serialized level payload described by
 * ktxTexture2_GetLevelFileInfo() for @p level — in whatever source
 * representation applies (BasisLZ, raw or Zstd-supercompressed UASTC, or
 * UASTC HDR 6x6 intermediate) — and @p srcSize must equal the byteLength
 * that query reports. The complete level (all layers, faces and depth
 * slices) is written to @p dst using the packing described by
 * ktxLevelProcessor_GetLevelSize() / GetImageSize() / GetImageOffset(),
 * with no inter-level padding. For non-video sources levels may be
 * processed in any order and a level may be processed more than once.
 *
 * @param[in]  processor   pointer to the processor.
 * @param[in]  level       mip level to process.
 * @param[in]  src         pointer to the level's serialized payload.
 * @param[in]  srcSize     size of @p src in bytes; must equal the level's
 *                         byteLength from ktxTexture2_GetLevelFileInfo().
 * @param[out] dst         pointer to the destination buffer.
 * @param[in]  dstCapacity capacity of @p dst; must be at least the value
 *                         from ktxLevelProcessor_GetLevelSize().
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p processor, @p src or @p dst is NULL,
 *                              @p level is not less than the source's
 *                              numLevels, @p srcSize does not equal the
 *                              level's stored byteLength, or
 *                              @p dstCapacity is smaller than the level's
 *                              processed size.
 * @exception KTX_INVALID_OPERATION
 *                              The source no longer retains its
 *                              serialized-source state: its image data
 *                              has been loaded since the processor was
 *                              created.
 * @exception KTX_FILE_DATA_ERROR
 *                              The source's serialized Level Index is
 *                              corrupt.
 * @exception KTX_UNSUPPORTED_FEATURE
 *                              Processing is not yet implemented (this
 *                              commit only).
 */
KTX_error_code
ktxLevelProcessor_ProcessLevel(ktxLevelProcessor* processor,
                               ktx_uint32_t level,
                               const ktx_uint8_t* src, ktx_size_t srcSize,
                               ktx_uint8_t* dst, ktx_size_t dstCapacity)
{
    if (processor == nullptr || src == nullptr || dst == nullptr)
        return KTX_INVALID_VALUE;
    if (level >= processor->prototype->numLevels)
        return KTX_INVALID_VALUE;

    // The source must still describe the serialized payload: loading its
    // image data after the processor was created discards that layout
    // (and, for Zstd/Zlib sources, rewrites the level index to the
    // inflated sizes), so the query's KTX_INVALID_OPERATION is propagated.
    ktxLevelFileInfo fileInfo;
    KTX_error_code result =
        ktxTexture2_GetLevelFileInfo(processor->source, level, &fileInfo);
    if (result != KTX_SUCCESS)
        return result;
    if (srcSize != fileInfo.byteLength)
        return KTX_INVALID_VALUE;

    ktx_size_t levelSize;
    result = ktxLevelProcessor_GetLevelSize(processor, level, &levelSize);
    if (result != KTX_SUCCESS)
        return result;
    if (dstCapacity < levelSize)
        return KTX_INVALID_VALUE;

    // Codec paths follow in the next commit of this PR series.
    return KTX_UNSUPPORTED_FEATURE;
}

/**
 * @memberof ktxLevelProcessor
 * @~English
 * @brief Destroy a processor.
 *
 * Frees the processor and its private prototype. The borrowed source
 * texture is not affected. NULL is accepted and ignored.
 *
 * @param[in] processor pointer to the processor to destroy, or NULL.
 */
void
ktxLevelProcessor_Destroy(ktxLevelProcessor* processor)
{
    if (processor == nullptr)
        return;
    if (processor->prototype)
        ktxTexture_Destroy(ktxTexture(processor->prototype));
    free(processor);
}

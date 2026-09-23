/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2026 The Khronos Group Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file
 * @~English
 *
 * @brief Private definition of ktxLevelProcessor.
 *
 * Kept out of level_processor.cpp so the public documentation of the
 * ktxLevelProcessor class comes from the declaration in ktx.h only.
 */

#ifndef _LEVEL_PROCESSOR_H_
#define _LEVEL_PROCESSOR_H_

#include <ktx.h>
#include "basis_transcode.h"

/*
 * A processor for the levels of one serialized Basis source.
 *
 * The processor borrows source: per the lifetime agreed in
 * KhronosGroup/KTX-Software#1224 the source must remain alive and
 * unmodified until ktxLevelProcessor_Destroy.
 */
struct ktxLevelProcessor {
    const ktxTexture2* source;
    // Target-layout prototype (NO_STORAGE), owned by the processor.
    ktxTexture2* prototype;
    // Per-level transcoding state shared with ktxTexture2_TranscodeBasis:
    // resolved target, SGD image descriptions, decoded ETC1S palettes and
    // tables. Owned by the processor.
    ktxBasisLevelTranscoder* transcoder;
    // Zstd sources only: decompression context reused across levels.
    struct ZSTD_DCtx_s* dctx;
    // Zstd and ZLIB sources only: buffer for a level's inflated data,
    // reused across levels and grown when a larger level arrives.
    ktx_uint8_t* inflatedData;
    ktx_size_t inflatedDataCapacity;
};

#endif /* _LEVEL_PROCESSOR_H_ */

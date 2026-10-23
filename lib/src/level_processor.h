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
#include "basis_sgd.h"

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
    // Concrete target after automatic-selection mapping.
    ktx_transcode_fmt_e outputFormat;
    ktx_transcode_flags transcodeFlags;
    alpha_content_e alphaContent;
};

#endif /* _LEVEL_PROCESSOR_H_ */

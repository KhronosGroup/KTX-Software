/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab textwidth=70: */

/*
 * Copyright 2024 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file
 * @~English
 *
 * @brief Get the DXGI format  that matches a VkFormat
 *
 */

#ifndef _VK2DXGI_H_
#define _VK2DXGI_H_

#include "vkformat_enum.h"
#include "dxsdk/include/dxgiformat.h" // TODO: Fix location

// DXGI_FORMAT enum is deliberately forced to uint32.
static uint32_t vkFormat2dxgiFormat( VkFormat vkFormat )
{
    switch ( vkFormat )
    {
#include "vkFormat2dxgiFormat.inl"
        default: return DXGI_FORMAT_UNKNOWN;
    }
}

#endif /* _VK2DXGI_H_ */


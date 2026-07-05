/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab textwidth=70: */

/*
 * Copyright 2026 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file
 * @~English
 *
 * @brief Get the MTLPixelFormat that matches a VkFormat
 *
 */

#ifndef _VK2MTL_H_
#define _VK2MTL_H_

#include "vkformat_enum.h"
#include "metalpixel_format.h"

static uint32_t vkFormat2mtlFormat( VkFormat vkFormat )
{
    switch ( vkFormat )
    {
#include "vkFormat2mtlFormat.inl"
        default: return MTLPixelFormatInvalid;
    }
}

#endif /* _VK2MTL_H_ */


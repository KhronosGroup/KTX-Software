/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/* Copyright 2019-2020 Mark Callow
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @~English
 * @brief Find the VkFormat matching a DFD.
 */

#include <KHR/khr_df.h>
#include "dfd.h"

/**
 * @~English
 * @brief Return a VkFormat matching a DFD.
 *
 * @param[in] dfd    Pointer to the DFD.
 *
 * @return      The matching VkFormat enum or VK_FORMAT_UNDEFINED if not
 *              matched.
 */
enum VkFormat dfd2vk(uint32_t *dfd)
{
#include "dfd2vk.inl"
}


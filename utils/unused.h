/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/* Copyright 2019-2018 The Khronos Group Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _UNUSED_H
#define _UNUSED_H

#if __GNUC__ || __clang__
  #define MAYBE_UNUSED __attribute__((unused))
#else
  #define MAYBE_UNUSED
#endif

#define U_ASSERT_ONLY MAYBE_UNUSED

#endif /* UNUSED_H */

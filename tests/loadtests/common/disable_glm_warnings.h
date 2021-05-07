/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2021 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined DISABLE_GLM_WARNINGS_H

// Temporarily disable the warnings caused by the GLM code.
#if defined(_MSC_VER)
  #pragma warning(push)
  #pragma warning(disable: 4201)
#elif defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
  #pragma clang diagnostic ignored "-Wnested-anon-types"
#endif

#endif /* DISABLE_GLM_WARNINGS_H */

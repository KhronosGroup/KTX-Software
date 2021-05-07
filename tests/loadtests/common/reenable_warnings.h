/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2021 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined REENABLE_WARNINGS_H

// Reenable warnings disabled by, e.g. disable_glm_warnings.h.
#if defined(_MSC_VER)
  #pragma warning(pop)
#elif defined(__clang__)
  #pragma clang diagnostic pop
#endif

#endif /* REENABLE_WARNINGS_H */

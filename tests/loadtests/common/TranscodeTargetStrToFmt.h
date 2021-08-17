/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow, <khronos at callow dot im>.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TRANSCODE_TARGET_STR_TO_FMT_
#define _TRANSCODE_TARGET_STR_TO_FMT_

#include <string>

#if !defined(_tstring)
  #if defined(_UNICODE)
    #define _tstring std::wstring
  #else
    #define _tstring std::string
  #endif
#endif

ktx_transcode_fmt_e TranscodeTargetStrToFmt(_tstring format);

#endif /* _TRANSCODE_TARGET_STR_TO_FMT_ */

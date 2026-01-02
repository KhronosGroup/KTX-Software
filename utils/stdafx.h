// Copyright 2010-2020 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

// stdafx.h

#pragma once

#if defined(_WIN32)
  // _CRT_SECURE_NO_WARNINGS must be defined before <windows.h>,
  // <stdio.h> and and <iostream>
  #define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <stdio.h>
#ifdef _WIN32
  #include <io.h>
  #if _MSC_VER < 1900
    #define snprintf _snprintf
  #endif
#else
  #include <unistd.h>
  #define _setmode(x, y) 0
#endif

#include <fcntl.h>
#include <errno.h>
#include <string.h>

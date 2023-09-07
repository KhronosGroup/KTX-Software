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
  #include <tchar.h>
  #if _MSC_VER < 1900
    #define snprintf _snprintf
  #endif
#else
  #include <unistd.h>

  #define _setmode(x, y) 0
  #define _tmain main
  #define _tcsncmp strncmp
  #define _tcscmp strcmp
  #define _tgetenv getenv
  #define _tcscpy strcpy
  #define _tcsncpy strncpy
  #define _tcsnpcpy strnpcpy
  #define _stscanf sscanf
  #define _tcslen strlen
  #define _tcscat strcat
  #define _tcsrchr strrchr
  #define _tcschr strchr
  #define _tfopen fopen
  #define _trename rename
  #define _tunlink unlink
  #if !defined(_TCHAR)
    #define _TCHAR char
    #define _T(x) x
  #endif
#endif
#if !defined(_tstring)
  #if defined(_UNICODE)
    #define _tstring std::wstring
  #else
    #define _tstring std::string
  #endif
#endif
#include <fcntl.h>
#include <errno.h>
#include <string.h>



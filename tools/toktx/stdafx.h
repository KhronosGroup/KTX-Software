// stdafx.h 

#pragma once

#include "targetver.h"

#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <stdio.h>
#ifdef WIN32
#include <io.h>
#include <tchar.h>
#else
#include <unistd.h>

#define _tmain main
#define _TCHAR char
#define _tcsncmp strncmp
#define _tcscmp strcmp
#define _snprintf snprintf
#define _tgetenv getenv
#define _tcscpy strcpy
#define _stscanf sscanf
#define _tcslen strlen
#define _tcscat strcat
#define _tcsrchr strrchr
#define _tcschr strchr
#define _setmode(x, y) 0
#define _unlink unlink
#define _T
#define WIN32 0

#endif
#include <fcntl.h> 
#include <fcntl.h>
#include <errno.h>
#include <string.h>



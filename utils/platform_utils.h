// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "stdafx.h"
#include <string>
#include <iostream>
#include <memory>
#include <vector>
#include <fmt/ostream.h>
#include <fmt/printf.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#endif

#if defined(_WIN32) && !defined(_UNICODE)
// For Windows, we convert the UTF-8 path to a UTF-16 path to force using the APIs
// that correctly handle unicode characters
inline std::wstring DecodeUTF8Path(std::string path) {
    std::wstring result;
    int len = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), static_cast<int>(path.length()), NULL, 0);
    if (len > 0)
    {
        result.resize(len);
        MultiByteToWideChar(CP_UTF8, 0, path.c_str(), static_cast<int>(path.length()), &result[0], len);
    }
    return result;
}
#else
// For other platforms there is no need for any conversion, they support UTF-8 natively
inline std::string DecodeUTF8Path(std::string path) {
    return path;
}
#endif

inline void InitUTF8CLI(int& argc, _TCHAR* argv[]) {
#if defined(_WIN32) && !defined(_UNICODE)
    // Windows does not support UTF-8 argv so we have to manually acquire it
    static std::vector<std::unique_ptr<_TCHAR[]>> utf8Argv(argc);
    LPWSTR commandLine = GetCommandLineW();
    LPWSTR* wideArgv = CommandLineToArgvW(commandLine, &argc);
    for (int i = 0; i < argc; ++i) {
        int byteSize = WideCharToMultiByte(CP_UTF8, 0, wideArgv[i], -1, nullptr, 0, nullptr, nullptr);
        utf8Argv[i] = std::make_unique<_TCHAR[]>(byteSize);
        WideCharToMultiByte(CP_UTF8, 0, wideArgv[i], -1, utf8Argv[i].get(), byteSize, nullptr, nullptr);
        argv[i] = utf8Argv[i].get();
    }
#else
    // Nothing to do for other platforms
    (void)argc;
    (void)argv;
#endif
}

// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
// For Windows, we convert the UTF-8 path to a UTF-16 path to force using the APIs
// that correctly handle unicode characters
static std::wstring DecodeUTF8Path(std::string path) {
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
static std::string DecodeUTF8Path(std::string path) {
    return path;
}
#endif

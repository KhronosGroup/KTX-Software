// Copyright 2022-2023 The Khronos Group Inc.
// Copyright 2022-2023 RasterGrid Kft.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "stdafx.h"
#include <string>
#include <iostream>
#include <memory>
#include <vector>

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
// For Windows, we convert the UTF-8 path to a UTF-16 path to force using
// the APIs that correctly handle unicode characters.
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
// For other platforms there is no need for any conversion, they
// support UTF-8 natively.
inline std::string DecodeUTF8Path(std::string path) {
    return path;
}
#endif

inline void InitUTF8CLI(int& argc, char* argv[]) {
#if defined(_WIN32)
    // Windows does not support UTF-8 argv so we have to manually acquire it
    static std::vector<std::unique_ptr<char[]>> utf8Argv(argc);
    // argc may be different from wargc if the caller of this is the
    // secondary receiver of the command line args, e.g. in a gtest program
    // where gtest removes its own args first.
    int wargc;
    LPWSTR commandLine = GetCommandLineW();
    LPWSTR* wideArgv = CommandLineToArgvW(commandLine, &wargc);
    for (int i = 0; i < argc; ++i) {
        int byteSize = WideCharToMultiByte(CP_UTF8, 0, wideArgv[i], -1, nullptr, 0, nullptr, nullptr);
        utf8Argv[i] = std::make_unique<char[]>(byteSize);
        WideCharToMultiByte(CP_UTF8, 0, wideArgv[i], -1, utf8Argv[i].get(),
                            byteSize, nullptr, nullptr);
        argv[i] = utf8Argv[i].get();
        if (i == 0) {
            // Skip over the removed args.
            i += (wargc - argc);
        }
    }
#else
    // Nothing to do for other platforms
    (void)argc;
    (void)argv;
#endif
}

inline FILE* fopenUTF8(const std::string& path, const std::string& mode) {
#if defined(_WIN32)
    FILE* fp;
    // Returned errno_t value is also set in the global errno.
    (void)_wfopen_s(&fp, DecodeUTF8Path(path).c_str(), DecodeUTF8Path(mode).c_str());
    return fp;
#else
    return fopen(path.c_str(), mode.c_str());
#endif
}

inline int unlinkUTF8(const std::string& path) {
#if defined(_WIN32)
    return _wunlink(DecodeUTF8Path(path).c_str());
#else
    return unlink(path.c_str());
#endif
}

#if defined(__cpp_lib_char8_t)
    // Casting from u8string to string is not allowed in C++20. Neither
    // can char8_t or std::u8string be streamed to ostreams. This provides
    // an explicit conversion. Note that this does not perform any encoding.
    //
    // To display streamed UTF-8 strings correctly on Windows PowerShell or
    // Command Prompt they must be set to display UTF-8 text. For PowerShell
    // run the following command before executing the program or add it to
    // your $PROFILE:
    //   $OutputEncoding = [Console]::OutputEncoding = [System.Text.UTF8Encoding]::new()
    // See https://stackoverflow.com/questions/57131654/using-utf-8-encoding-chcp-65001-in-command-prompt-windows-powershell-window
    // for more details and how to change the encoding for Command.
    //
    // Note that the console spawned by Visual Studio when running a console
    // application does not load $PROFILE so displayed utf-8 will be
    // mojibake.
    //
    // Note also that fmt::print works correctly without changing the console
    // encoding because it uses the Windows wide char APIs to write to the
    // console.
    inline std::string from_u8string(const std::u8string& s) {
        return std::string(s.begin(), s.end());
    }

    #if defined(_WIN32) && !defined(_UNICODE)
        // For Windows, we convert the UTF-8 path to a UTF-16 path to force using
        // the APIs that correctly handle unicode characters.
        inline std::wstring DecodeUTF8Path(std::u8string u8path) {
            std::wstring result;
            std::string path = from_u8string(u8path);
            int len =
                MultiByteToWideChar(CP_UTF8, 0, path.c_str(), static_cast<int>(path.length()), NULL, 0);
            if (len > 0) {
                result.resize(len);
                MultiByteToWideChar(CP_UTF8, 0, path.c_str(), static_cast<int>(path.length()), &result[0],
                                    len);
            }
            return result;
        }
    #else
        // For other platforms convert to a regular string.
        inline std::string DecodeUTF8Path(std::u8string path) { return from_u8string(path); }
    #endif

    inline void InitUTF8CLI(int& argc, char* argv[], std::vector<std::u8string>& u8argv) {
        u8argv.resize(argc);
    #if defined(_WIN32)
        // Windows does not support UTF-8 argv so we have to manually acquire it
        (void)argv;  // Unused
        // See note in non-char8_t InitUTF8CLI about argc vs wargc.
        int wargc;
        LPWSTR commandLine = GetCommandLineW();
        LPWSTR* wideArgv = CommandLineToArgvW(commandLine, &wargc);
        for (int i = 0; i < argc; ++i) {
            int byteSize =
                WideCharToMultiByte(CP_UTF8, 0, wideArgv[i], -1, nullptr, 0, nullptr, nullptr);
            byteSize--; // Returned byteSize includes the terminating NUL.
            u8argv[i].resize(byteSize);
            WideCharToMultiByte(CP_UTF8, 0, wideArgv[i], -1, (LPSTR)u8argv[i].data(),
                                byteSize, nullptr, nullptr);
            if (i == 0) {
                // Skip over the removed args.
                i += (wargc - argc);
            }
        }
    #else
        for (int i = 0; i < argc; ++i) {
            u8argv[i] = std::u8string(reinterpret_cast<const char8_t*>(argv[i]));
        }
    #endif
    }

    inline FILE* fopenUTF8(const std::u8string& path, const std::string& mode) {
        #if defined(_WIN32)
            FILE* fp;
            // Returned errno_t value is also set in the global errno.
            (void)_wfopen_s(&fp, DecodeUTF8Path(path).c_str(), DecodeUTF8Path(mode).c_str());
            return fp;
        #else
            return fopen(from_u8string(path).c_str(), mode.c_str());
        #endif
    }

    inline int unlinkUTF8(const std::u8string& path) {
        #if defined(_WIN32)
            return _wunlink(DecodeUTF8Path(path).c_str());
        #else
            return unlink(DecodeUTF8Path(path).c_str());
        #endif
    }
#else
    #define from_u8string(s) (s)
#endif

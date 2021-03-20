# Copyright 2016, Simon Werta (@webmaster128).
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 2.8.12)

set(cputypetest_code "
//
// https://gist.github.com/webmaster128/e08067641df1dd784eb195282fd0912f
//
// The resulting values must not be defined as macros, which
// happens e.g. for 'i386', which is a macro in clang.
// For safety reasons, we undefine everything we output later
//
// For CMake literal compatibility, this file must have no double quotes
//
#ifdef _WIN32
    #ifdef _WIN64

#undef x86_64
x86_64

    #else

#undef x86
x86

    #endif
#elif defined __APPLE__
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        #if TARGET_CPU_X86

#undef x86
x86

        #elif TARGET_CPU_X86_64

#undef x86_64
x86_64

        #elif TARGET_CPU_ARM

#undef armv7
armv7

        #elif TARGET_CPU_ARM64

#undef armv8
armv8

        #else
            #error Unsupported cpu
        #endif
    #elif TARGET_OS_MAC

#undef x86_64
x86_64

    #else
        #error Unsupported platform
    #endif
#elif defined __linux
    #ifdef __ANDROID__
        #ifdef __i386__

#undef x86
x86

        #elif defined __arm__

#undef armv7
armv7

        #elif defined __aarch64__

#undef armv8
armv8

        #else
            #error Unsupported cpu
        #endif
    #else
        #ifdef __LP64__

#undef x86_64
x86_64

        #else

#undef x86
x86

        #endif
    #endif
#else
    #error Unsupported cpu
#endif
")
file(WRITE "${CMAKE_BINARY_DIR}/cputypetest.c" "${cputypetest_code}")

cmake_policy(SET CMP0054 NEW)

function(set_target_processor_type out)
    if(${ANDROID_ABI} AND ${ANDROID_ABI} STREQUAL "armeabi-v7a")
        set(${out} armv7 PARENT_SCOPE)
    elseif(${ANDROID_ABI} AND ${ANDROID_ABI} STREQUAL "arm64-v8a")
        set(${out} armv8 PARENT_SCOPE)
    elseif(${ANDROID_ABI} AND ${ANDROID_ABI} STREQUAL "x86")
        set(${out} x86_32 PARENT_SCOPE)
    elseif(${ANDROID_ABI} AND ${ANDROID_ABI} STREQUAL "x86_64")
        set(${out} x86_64 PARENT_SCOPE)

    else()
        if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
            set(C_PREPROCESS ${CMAKE_C_COMPILER} /EP /nologo)
            execute_process(
                COMMAND ${C_PREPROCESS} "${CMAKE_BINARY_DIR}/cputypetest.c"
                OUTPUT_VARIABLE processor
                OUTPUT_STRIP_TRAILING_WHITESPACE
                # Specify this to block MSVC's output of the source file name
                # so as not to trigger PowerShell's stop-on-error in CI.
                # Unfortunately it suppresses all compile errors too hence
                # the special case for MSVC.
                ERROR_QUIET
            )
        else()
            set(C_PREPROCESS ${CMAKE_C_COMPILER} -I /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include -E -P)
            execute_process(
                COMMAND ${C_PREPROCESS} "${CMAKE_BINARY_DIR}/cputypetest.c"
                OUTPUT_VARIABLE processor
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
        endif()

        string(STRIP "${processor}" processor)
        set(${out} ${processor} PARENT_SCOPE)
    endif()
endfunction(set_target_processor_type)

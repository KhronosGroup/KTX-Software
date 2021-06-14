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
#if defined _WIN32
#   if defined _WIN64
#       error ARCH_FOUND x86_64
#   else
#       error ARCH_FOUND x86
#   endif
#elif defined __APPLE__
#   include <TargetConditionals.h>
#   if TARGET_OS_IPHONE
#       if TARGET_CPU_X86
#           error ARCH_FOUND x86
#       elif TARGET_CPU_X86_64
#           error ARCH_FOUND x86_64
#       elif TARGET_CPU_ARM
#           error ARCH_FOUND armv7
#       elif TARGET_CPU_ARM64
#           error ARCH_FOUND armv8
#       else
#           error Unsupported cpu
#       endif
#   elif TARGET_OS_MAC
#       if defined __x86_64__
#           error ARCH_FOUND x86_64
#       elif defined __aarch64__
#           error ARCH_FOUND arm64
#       else
#           error Unsupported platform
#       endif
#   else
#       error Unsupported platform
#   endif
#elif defined __linux
#   ifdef __ANDROID__
#       ifdef __i386__
#           error ARCH_FOUND x86
#       elif defined __arm__
#           error ARCH_FOUND armv7
#       elif defined __aarch64__
#           error ARCH_FOUND armv8
#       else
#           error Unsupported cpu
#       endif
#   else
#       ifdef __LP64__
#           error ARCH_FOUND x86_64
#       else
#           error ARCH_FOUND x86
#       endif
#   endif
#elif defined __EMSCRIPTEN__
#   error ARCH_FOUND wasm
#else
#   error Unsupported cpu
#endif
")
file(WRITE "${CMAKE_BINARY_DIR}/cputypetest.c" "${cputypetest_code}")

cmake_policy(SET CMP0054 NEW)

function(set_target_processor_type out)
    if(ANDROID_ABI AND "${ANDROID_ABI}" STREQUAL "armeabi-v7a")
        set(${out} armv7 PARENT_SCOPE)
    elseif(ANDROID_ABI AND "${ANDROID_ABI}" STREQUAL "arm64-v8a")
        set(${out} armv8 PARENT_SCOPE)
    elseif(ANDROID_ABI AND "${ANDROID_ABI}" STREQUAL "x86")
        set(${out} x86 PARENT_SCOPE)
    elseif(ANDROID_ABI AND "${ANDROID_ABI}" STREQUAL "x86_64")
        set(${out} x86_64 PARENT_SCOPE)
    else()
        if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
            if("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "ARM")
                set(processor "arm")
            elseif("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "ARM64")
                set(processor "arm64")
            else()
                try_compile(res_var
                    ${CMAKE_BINARY_DIR}/CMakeTemp
                    ${CMAKE_BINARY_DIR}/cputypetest.c
                    OUTPUT_VARIABLE processor)

                string(REGEX MATCH "ARCH_FOUND ([_a-z0-9]+)" processor "${processor}")
                string(REPLACE "ARCH_FOUND " "" processor "${processor}")
            endif()
        else()
            if(APPLE AND NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
                # No other Apple systems are x64_64. When configuring for iOS, etc.
                # CMAKE_C_COMPILER points at the HOST compiler - I can't find
                # definitive documentation of what is supposed to happen - the
                # test program above returns x86_64. Since we don't care what
                # type of ARM processor arbitrarily set armv8 for these systems.
                set(processor armv8)
            else()
                try_compile(res_var
                    ${CMAKE_BINARY_DIR}/CMakeTemp
                    ${CMAKE_BINARY_DIR}/cputypetest.c
                    OUTPUT_VARIABLE processor)

                string(REGEX MATCH "ARCH_FOUND ([_a-z0-9]+)" processor "${processor}")
                string(REPLACE "ARCH_FOUND " "" processor "${processor}")
            endif()
        endif()
        set(${out} ${processor} PARENT_SCOPE)
    endif()
endfunction(set_target_processor_type)

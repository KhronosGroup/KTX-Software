# Copyright 2016, Simon Werta (@webmaster128).
# SPDX-License-Identifier: Apache-2.0

include_guard(DIRECTORY)

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

#if defined(__x86_64__) || defined(_M_X64)
x86_64
#elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
x86_32
#elif defined(__ARM_ARCH_2__)
armv2
#elif defined(__ARM_ARCH_3__) || defined(__ARM_ARCH_3M__)
armv3
#elif defined(__ARM_ARCH_4T__) || defined(__TARGET_ARM_4T)
armv4T
#elif defined(__ARM_ARCH_5_) || defined(__ARM_ARCH_5E_)
ARM5
#elif defined(__ARM_ARCH_6T2_) || defined(__ARM_ARCH_6T2_)
armv6T2
#elif defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) || defined(__ARM_ARCH_6ZK__)
armv6
#elif defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
armv7
#elif defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
armv7A
#elif defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
armv7R
#elif defined(__ARM_ARCH_7M__)
armv7M
#elif defined(__ARM_ARCH_7S__)
armv7S
#elif defined(__aarch64__) || defined(_M_ARM64)
arm64
#elif defined(mips) || defined(__mips__) || defined(__mips)
mips
#elif defined(__sh__)
superh
#elif defined(__powerpc) || defined(__powerpc__) || defined(__powerpc64__) || defined(__POWERPC__) || defined(__ppc__) || defined(__PPC__) || defined(_ARCH_PPC)
powerpc
#elif defined(__PPC64__) || defined(__ppc64__) || defined(_ARCH_PPC64)
powerpc64
#elif defined(__sparc__) || defined(__sparc)
sparc
#elif defined(__m68k__)
m68k
#elif defined __EMSCRIPTEN__
wasm
#else
    #error Unsupported cpu
#endif

")

file(WRITE "${CMAKE_BINARY_DIR}/cputypetest.c" "${cputypetest_code}")

cmake_policy(SET CMP0054 NEW)

function(set_target_processor_type out)
    if(ANDROID_ABI AND "${ANDROID_ABI}" STREQUAL "armeabi-v7a")
        set(${out} armv7 PARENT_SCOPE)
    elseif(ANDROID_ABI AND "${ANDROID_ABI}" STREQUAL "arm64-v8a")
        set(${out} arm64 PARENT_SCOPE)
    elseif(ANDROID_ABI AND "${ANDROID_ABI}" STREQUAL "x86")
        set(${out} x86 PARENT_SCOPE)
    elseif(ANDROID_ABI AND "${ANDROID_ABI}" STREQUAL "x86_64")
        set(${out} x86_64 PARENT_SCOPE)

    else()
        if(MSVC) # MSVC is true for all msvc-style compilers, including clang-cl
            if("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "ARM" OR "${CMAKE_GENERATOR_PLATFORM}" STREQUAL "arm")
                set(processor "arm")
            elseif("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "ARM64" OR "${CMAKE_GENERATOR_PLATFORM}" STREQUAL "arm64")
                set(processor "arm64")
            else()
                set(C_PREPROCESS ${CMAKE_C_COMPILER} /EP /nologo)
                # Versions of MSVC prior to VS2019 have a supporting dll which
                # must be found along the search path. MSVC in VS 2019 just
                # works. Whether due to not having this supporting dll or using
                # a different way to locate it, is unknown. To make earlier
                # versions work set the WORKING_DIR to the location of the
                # support dll to ensure it is found.
                string(REGEX REPLACE "/VC/.*$" "/Common7/IDE"
                       compiler_support_dir
                       ${CMAKE_C_COMPILER})
                execute_process(
                    COMMAND ${C_PREPROCESS} "${CMAKE_BINARY_DIR}/cputypetest.c"
                    WORKING_DIRECTORY ${compiler_support_dir}
                    OUTPUT_VARIABLE processor
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    # Specify this to block MSVC's output of the source file name
                    # so as not to trigger PowerShell's stop-on-error in CI.
                    # Unfortunately it suppresses all compile errors too hence
                    # the special case for MSVC. Which was convenient to have
                    # when we found the issue with earlier versions of VS.
                    ERROR_QUIET
                )
            endif()
        else()
            # Apple's clang is a single compiler whose target is determined by
            # its -arch <architecture> or --target=<architecture> options
            # defaulting to the processor of the Mac it is running on. CMake
            # does not set this directly when generating for Xcode, so probing
            # cputypetest.c with CMAKE_{C,CXX}_COMPILER yields the host arch
            # (x86_64 or arm64).
            #
            # For iOS/tvOS/visionOS, CMake only sets CMAKE_SYSTEM_NAME during
            # configuration. The actual ARM target is selected later by Xcode
            # or by an -sdk argument to xcodebuild.
            #
            # For macOS the caller may set CMAKE_OSX_ARCHITECTURES to a single
            # arch (e.g. "x86_64" or "arm64") or to a list ("arm64;x86_64") to
            # request a universal build. The literal "$(ARCHS_STANDARD)" is
            # rejected by the root CMakeLists.txt because CMake stores it
            # unexpanded and breaks $<TARGET_OBJECTS:...> paths.
            #
            # Multi-arch (universal) note: CPU_ARCHITECTURE here is just one
            # representative arch — the universal-aware code paths in
            # lib/CMakeLists.txt key off `universal_build` and the per-arch
            # `arch_arm64`/`arch_x86_64` flags instead. In particular,
            # BASISU_SSE is forced off for universal builds and the x86_64
            # slice gets -msse4.1 re-enabled per-arch via -Xarch_x86_64.
            if(APPLE AND NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
                # Building for iOS, iPadOS, etc. Since we don't care what
                # type of ARM processor, arbitrarily set armv8.
                # It should be arm64 but there is a check in tests/CMakeLists.txt
                # that is dropping loadtests for Apple Silicon arm64.
                set(processor armv8)
            elseif(APPLE AND CMAKE_OSX_ARCHITECTURES)
                # Single-arch builds use that arch directly. For multi-arch
                # universal builds, just pick the first listed arch as a
                # representative value (callers must not depend on it for
                # arch-sensitive decisions; see the multi-arch note above).
                list(GET CMAKE_OSX_ARCHITECTURES 0 processor)
            else()
                # This will distinguish between M1 and Intel Macs
                set(C_PREPROCESS ${CMAKE_C_COMPILER} -E -P)
                execute_process(
                    COMMAND ${C_PREPROCESS} "${CMAKE_BINARY_DIR}/cputypetest.c"
                    OUTPUT_VARIABLE processor
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                )
            endif()
        endif()

        string(STRIP "${processor}" processor)
        #message(STATUS "*** set_target_processor_type ***: processor is ${processor}")
        set(${out} ${processor} PARENT_SCOPE)
    endif()
endfunction(set_target_processor_type)

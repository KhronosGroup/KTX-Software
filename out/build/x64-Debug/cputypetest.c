
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


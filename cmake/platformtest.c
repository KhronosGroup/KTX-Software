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
            #error Unsupported platform
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
            #error Unsupported platform
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
#elif defined EMSCRIPTEN
#undef WASM
WASM
#else
    #error Unsupported platform
#endif

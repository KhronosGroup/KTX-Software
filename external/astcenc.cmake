####################################################
# ASTC Encoder.
####################################################
include(FetchContent)
include(${KTX_ROOT_DIR}/cmake/compiler_query_genexs.cmake)

if (TARGET astc::astcenc)
    message(STATUS "Using prebuilt astc")
    return()
endif()

set(ASTC_VERSION 5.3.0)
message(STATUS "ASTC VERSION: ${ASTC_VERSION}")


# Determine most of the ASTC-related settings automatically.

# On Linux and Windows only one architecture is supported at once. On
# macOS simultaneous multiple architectures are supported by the ASTC
# encoder but not by the BasisU encoder. The latter supports only SSE
# SIMD that is enabled by compile time defines which makes it not amenable
# to a standard Xcode universal binary build. To ensure BASISU_SUPPORT_SSE
# is disabled when building for multiple architectures use only the
# value of CMAKE_OSX_ARCHITECTURES to decide if a universal build
# has been requested. Do not expose the astc-encoder's
# ASTCENC_UNIVERSAL_BUILD configuration option.

set(universal_build OFF)
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    # Check CMAKE_OSX_ARCHITECTURES for multiple architectures.
    list(FIND CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD)" archs_standard)
    list(LENGTH CMAKE_OSX_ARCHITECTURES architecture_count)
    if(NOT ${archs_standard} EQUAL -1 OR architecture_count GREATER 1)
        set(universal_build ON)
    endif()
    # Set ordinary variable to override astc-encoder option's ON default
    # and hide the option.
    set(ASTCENC_UNIVERSAL_BUILD ${universal_build})
endif()

# If we detect the user is doing a universal build defer to astc-encoder
# to pick the architectures. If a universal build has not been requested,
# present options to the user to turn off SIMD and, if running on x86_64
# to choose which SIMD ISA to use. On arm64 always use Neon. On unknown
# CPUs use None.
#
# On x86_64, if neither ASTCENC_ISA_SSE41 nor ASTCENC_ISA_SSE2 are defined,
# choose ASTCENC_ISA_AVX2. If ASTCENC_ISA_AVX2 fails to compile user must
# choose another x86_64 option.

if(NOT ${universal_build})
    set(ASTCENC_ISA_NATIVE OFF)
    set(ASTCENC_ISA_NEON OFF)
    if(NOT CPU_ARCHITECTURE STREQUAL x86_64)
        set(ASTCENC_ISA_AVX2 OFF)
        set(ASTCENC_ISA_SSE41 OFF)
        set(ASTCENC_ISA_SSE2 OFF)
    endif()

    # "" in the CACHE sets causes use of the existing documentation string.
    if(${ASTCENC_ISA_NONE})
        set(ASTCENC_LIB_TARGET astcenc-none-static)
        if(CPU_ARCHITECTURE STREQUAL x86_64)
            set(ASTCENC_ISA_AVX2 OFF CACHE BOOL "" FORCE)
            set(ASTCENC_ISA_SSE41 OFF CACHE BOOL "" FORCE)
            set(ASTCENC_ISA_SSE2 OFF CACHE BOOL "" FORCE)
        endif()
    else()
        if(CPU_ARCHITECTURE STREQUAL x86_64)
            if (${ASTCENC_ISA_SSE41})
                set(ASTCENC_LIB_TARGET astcenc-sse4.1-static)
                set(ASTCENC_ISA_AVX2 OFF CACHE BOOL "" FORCE)
                set(ASTCENC_ISA_SSE2 OFF CACHE BOOL "" FORCE)
            elseif (${ASTCENC_ISA_SSE2})
                set(ASTCENC_LIB_TARGET astcenc-sse2-static)
                set(ASTCENC_ISA_AVX2 OFF CACHE BOOL "" FORCE)
                set(ASTCENC_ISA_SSE41 OFF CACHE BOOL "" FORCE)
            else()
                set(ASTCENC_LIB_TARGET astcenc-avx2-static)
                set(ASTCENC_ISA_AVX2 ON CACHE BOOL "" FORCE)
                set(ASTCENC_ISA_SSE41 OFF CACHE BOOL "" FORCE)
                set(ASTCENC_ISA_SSE2 OFF CACHE BOOL "" FORCE)
            endif()
        elseif(CPU_ARCHITECTURE STREQUAL armv8 OR CPU_ARCHITECTURE STREQUAL arm64)
            set(ASTCENC_LIB_TARGET astcenc-neon-static)
            set(ASTCENC_ISA_NEON ON)
            set(ASTCENC_ISA_NONE OFF CACHE BOOL "" FORCE)
        else()
            message(STATUS "Unsupported ISA for ASTC on ${CPU_ARCHITECTURE} arch, using ASTCENC_ISA_NONE.")
            set(ASTCENC_ISA_NONE ON CACHE BOOL "" FORCE)
            set(ASTCENC_LIB_TARGET astcenc-none-static)
        endif()
    endif()
endif()

# setting ASTCENC_DECOMPRESSOR to ON breaks the build, so force it to OFF
# and hide it from cmake-gui (by using type INTERNAL)
set(ASTCENC_DECOMPRESSOR OFF CACHE INTERNAL "")
set(ASTCENC_CLI OFF) # Only build as library not the CLI astcencoder
# Force static build for astc-encoder
set(BUILD_SHARED_LIBS OFF)

# Declare package
FetchContent_Declare(
    astcenc
    GIT_REPOSITORY https://github.com/ARM-software/astc-encoder
    GIT_TAG ${ASTC_VERSION}
    FIND_PACKAGE_ARGS
)

# Populate astc
FetchContent_MakeAvailable(astcenc)

set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_RESET})
set_property(TARGET ${ASTCENC_LIB_TARGET} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_compile_definitions(
    ${ASTCENC_LIB_TARGET} PRIVATE
    $<$<AND:${is_msvccl},$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,19.40.33811>>:_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR>
    $<$<AND:${is_clangcl},$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,17.0.3>>:_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR>
)

add_library(astc::astcenc ALIAS ${ASTCENC_LIB_TARGET})

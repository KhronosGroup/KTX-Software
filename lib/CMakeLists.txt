# Copyright 2015-2025 The Khronos Group Inc.
# Copyright 2022-2023 RasterGrid Kft.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.22)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../cmake/modules/")
set( KTX_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/.. )
#set( KTX_ROOT_DIR .. )

# N.B. When used before the project() command, PROJECT_IS_TOP_LEVEL will not
# do what you likely at first expect. It will be TRUE if this is included
# in another project.

if(NOT DEFINED KTX_VERSION)
    include(${KTX_ROOT_DIR}/cmake/version.cmake)
endif()

project(libktx
    VERSION ${KTX_VERSION}
    DESCRIPTION "Libraries and tools to create and read KTX image texture files."
)

if(APPLE)
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CMAKE_SYSTEM_NAME STREQUAL "tvOS" OR CMAKE_SYSTEM_NAME STREQUAL "visionOS")
        set( APPLE_LOCKED_OS ON )
    else()
        set( APPLE_MAC_OS ON )
    endif()
endif()

if(POLICY CMP0127)
    # cmake_dependent_option() supports full Condition Syntax. Introduced in
    # 3.22. Not all build environments have 3.22+. Set policy to avoid warning.
    # Seems the parens in the match string trigger the warning.
    cmake_policy(SET CMP0127 NEW)
endif()

# Beware of using cmake_print_variables for variables with function
# dependent values, e.g. ARGN and CMAKE_CURRENT_FUNCTION_LIST_DIR.
include(CMakePrintHelpers)
include(CMakeDependentOption)

include(${KTX_ROOT_DIR}/cmake/codesign.cmake) # Needs APPLE_*_OS values.
include(${KTX_ROOT_DIR}/cmake/cputypetest.cmake)

# OPTIONS

option( LIBKTX_VERSION_READ_ONLY "Build the read only library" OFF)
option( LIBKTX_VERSION_FULL "Build the full library" ON)

option( LIBKTX_FEATURE_KTX1 "Enable KTX 1 support." ON )
# TODO: Remove this option?
option( LIBKTX_FEATURE_KTX2 "Enable KTX 2 support." ON )
option( LIBKTX_FEATURE_VK_UPLOAD "Enable Vulkan texture upload." ON )
option( LIBKTX_FEATURE_GL_UPLOAD "Enable OpenGL texture upload." ON )
option( LIBKTX_FEATURE_ETC_UNPACK "ETC decoding support." ON )

# Intentionally override the BASISU options, only making them visible
# when they are useful.
CMAKE_DEPENDENT_OPTION( BASISU_SSE
    "Compile with SSE support so applications can choose to use it."
    ON
    "NOT CMAKE_OSX_ARCHITECTURES STREQUAL \"$(ARCHS_STANDARD)\"; CPU_ARCHITECTURE STREQUAL x86_64"
    OFF
)
CMAKE_DEPENDENT_OPTION( BASISU_OPENCL
    "Compile with OpenCL support so applications can choose to use it."
    OFF
    "NOT APPLE_LOCKED_OS"
    OFF
)

option( LIBKTX_WERROR "Make all warnings in libktx code into errors." OFF)

CMAKE_DEPENDENT_OPTION( LIBKTX_EMBED_BITCODE
    "Embed bitcode in binaries."
    OFF
    "APPLE"
    OFF
)

CMAKE_DEPENDENT_OPTION( BUILD_SHARED_LIBS
    "Create shared libraries (static otherwise)."
    ON
    "NOT (APPLE_LOCKED_OS OR EMSCRIPTEN)"
    OFF
)

# Used to reset BUILD_SHARED_LIBS after forcing static builds
set( BUILD_SHARED_LIBS_RESET ${BUILD_SHARED_LIBS} )

# Platform specific settings

set(bitness 64)

if(NOT DEFINED CPU_ARCHITECTURE)
    set_target_processor_type(CPU_ARCHITECTURE)
endif()
if (CPU_ARCHITECTURE STREQUAL x86)
    message(FATAL_ERROR "This project cannot be built for x86 cpu.")
endif()

# Windows Store Compatibility.
if (${CMAKE_SYSTEM_NAME} STREQUAL "WindowsStore" AND LIBKTX_FEATURE_GL_UPLOAD)
    message(INFO "Disabling OpenGL upload in libktx for Universal Windows Platform.")
    set(LIBKTX_FEATURE_GL_UPLOAD OFF)
endif()

if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN AND NOT ANDROID)
    set(LINUX TRUE)
endif()

if(EMSCRIPTEN)
    set( LIBKTX_FEATURE_VK_UPLOAD OFF )
endif()

if(NOT BUILD_SHARED_LIBS)
    set(LIB_TYPE STATIC)
else()
    set(LIB_TYPE SHARED)
endif()

if(MINGW)
    # Check if the Threads package is provided; if using Mingw it MIGHT be
    find_package(Threads)
elseif(LINUX)
    find_package(Threads REQUIRED)
endif()

# Source Lists

set(LIBKTX_MAIN_SRC
    include/KHR/khr_df.h
    include/ktx.h
    src/astc_codec.cpp
    src/basis_sgd.h
    src/basis_transcode.cpp
    src/miniz_wrapper.cpp
    src/checkheader.c
    ../external/dfdutils/createdfd.c
    ../external/dfdutils/colourspaces.c
    ../external/dfdutils/dfd.h
    ../external/dfdutils/interpretdfd.c
    ../external/dfdutils/printdfd.c
    ../external/dfdutils/queries.c
    ../external/dfdutils/vk2dfd.c
    ../external/dfdutils/vk2dfd.inl
    ../external/dfdutils/vulkan/vk_platform.h
    ../external/dfdutils/vulkan/vulkan_core.h
    src/etcunpack.cxx
    src/filestream.c
    src/filestream.h
    src/formatsize.h
    src/gl_format.h
    src/glformat_str.c
    src/hashlist.c
    src/info.c
    src/ktxint.h
    src/memstream.c
    src/memstream.h
    src/strings.c
    src/swap.c
    src/texture.c
    src/texture.h
    src/texture2.c
    src/texture2.h
    src/texture_funcs.inl
    src/uthash.h
    src/vk2gl.h
    src/vk_format.h
    src/vkFormat2glFormat.inl
    src/vkFormat2glInternalFormat.inl
    src/vkFormat2glType.inl
    src/vkformat_check.c
    src/vkformat_check_variant.c
    src/vkformat_enum.h
    src/vkformat_str.c
    src/vkformat_typesize.c
    ../utils/unused.h

)

if (LIBKTX_FEATURE_ETC_UNPACK)
    list( APPEND LIBKTX_MAIN_SRC ../external/etcdec/etcdec.cxx )
endif()

if(LIBKTX_FEATURE_GL_UPLOAD)
    list(APPEND LIBKTX_MAIN_SRC
        src/gl_funclist.inl
        src/gl_funcs.c
        src/gl_funcs.h
        src/glloader.c
    )
endif()

include(${KTX_ROOT_DIR}/cmake/fp-settings.cmake)
# To improve output determinism, enable precise floating point operations.
# Set via add_compile_options so they will be set for basisu_encoder too.
# The root KTX-Software project also sets these. Rather than figuring out
# what the parent project is, let CMake's option de-duplication do its work.
get_fp_compile_options(fp_options)
add_compile_options( ${fp_options} )

include(${KTX_ROOT_DIR}/cmake/compiler_query_genexs.cmake)

# Macro for common lib settings

macro(common_libktx_settings target enable_write library_type)

    if(TARGET mkvk)
        # Creating vulkan headers only required after Vulkan Spec/SDK updates.
        add_dependencies(${target} mkvk)
    endif()

    set_target_properties(${target} PROPERTIES
        PUBLIC_HEADER
#          "${CMAKE_CURRENT_SOURCE_DIR}/include/ktx.h;${CMAKE_CURRENT_SOURCE_DIR}/include/KHR/khr_df.h"
          # Omit khr_df.h. Its installation has to be handled separately to
          # workaround CMake's failure to preserve the directory hierarchy.
          include/ktx.h
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
    )

    if(APPLE_LOCKED_OS)
        set_target_properties(${target} PROPERTIES
            FRAMEWORK TRUE
        )
    endif()

    if( NOT ${library_type} STREQUAL STATIC )
        # Must not call this macro for static libs on Windows. To keep
        # the if test simple, never call it for static libs. On macOS
        # and iOS Xcode knows libs aren't signed so it would ignore the
        # settings made by this macro.
        set_code_sign(${target} "NOPPS")
    endif()

    if(MSVC)
        target_compile_options( ${target} PRIVATE /W4;$<$<BOOL:${LIBKTX_WERROR}>:/WX> )
        target_compile_options( ${target} PRIVATE $<IF:$<CONFIG:Debug>,/Gz,/O2> )
        # Enable UTF-8 support
        target_compile_options( ${target} PRIVATE $<$<C_COMPILER_ID:MSVC>:/utf-8> )
        target_compile_options( ${target} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8> )
    elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU"
           OR ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
        target_compile_options( ${target} PRIVATE -Wall -Wextra $<$<BOOL:${LIBKTX_WERROR}>:-Werror>)
        target_compile_options( ${target} PRIVATE $<IF:$<CONFIG:Debug>,-O0$<SEMICOLON>-g,-O3> )
        if(EMSCRIPTEN)
            target_link_options( ${target} PRIVATE $<IF:$<CONFIG:Debug>,-gsource-map,-O3> )
        else()
            target_link_options( ${target} PRIVATE $<IF:$<CONFIG:Debug>,-g,-O3> )
        endif()
        if(APPLE)
            target_compile_options(${target} PRIVATE $<$<BOOL:${LIBKTX_EMBED_BITCODE}>:-fembed-bitcode>)
        endif()
    else()
        message(FATAL_ERROR "${CMAKE_CXX_COMPILER_ID} not yet supported.")
    endif()

    # To influence the FP settings of apps depending on us do this. Unsure if that is
    # a wise thing hence commented.
    #target_compile_options( ${target} PUBLIC ${fp_options} )

    target_compile_definitions(
        ${target}
    PUBLIC
        "$<$<CONFIG:Debug>:_DEBUG;DEBUG>"
    PRIVATE
        LIBKTX
        SUPPORT_SOFTWARE_ETC_UNPACK=$<IF:$<BOOL:${LIBKTX_FEATURE_ETC_UNPACK}>,1,0>
    )

    # C/C++ Standard
    # Need c11 for Unicode string literals. Need c++17 because some basisu
    # include files have c++17 features.
    target_compile_features(${target} PUBLIC c_std_11 cxx_std_17)

    # Compiler Warning Flags
    if(EMSCRIPTEN)
        target_compile_options(${target} PRIVATE
            -Wno-nested-anon-types
            -Wno-gnu-anonymous-struct
        )
    else()
        target_compile_options(${target} PRIVATE
            # clang options
            $<$<CXX_COMPILER_ID:AppleClang,Clang>:
                -Wno-nested-anon-types
                -Wno-gnu-anonymous-struct
            >
            $<$<CXX_COMPILER_ID:GNU>:
                -Wno-cast-function-type
            >
            # not clang options
            $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
                -Wno-pedantic
            >
        )
    endif()

    target_include_directories(
        ${target}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        $<BUILD_INTERFACE:${KTX_ROOT_DIR}/external>
        $<BUILD_INTERFACE:${KTX_ROOT_DIR}/utils>
    )

    target_include_directories(
        ${target}
    SYSTEM PRIVATE
        $<BUILD_INTERFACE:${KTX_ROOT_DIR}/other_include>
    )

    if( ${library_type} STREQUAL STATIC )
        target_compile_definitions(${target} PUBLIC KHRONOS_STATIC)
    endif()

    # To reduce size, don't support transcoding to ancient formats.
    target_compile_definitions(${target} PRIVATE BASISD_SUPPORT_FXT1=0)

    if(WIN32)
        target_compile_definitions(
            ${target}
        PRIVATE
            # Only set dllexport when building a shared library.
            $<$<STREQUAL:${library_type},SHARED>:KTX_API=__declspec\(dllexport\)>
            # Code compiled with the versions shown defaults to a constexpr
            # std::mutex constructor and requires a mscvp140.dll of at least
            # version 14.40.33810.00 otherwise code creating a mutex
            # crashes mysteriously. Since many JVM installations bundle
            # their own version of the VC++ redistributables chances are
            # high they will not have a modern enough version so JNI modules
            # linked with libktx will crash when multiple threads are used,
            # as they are in the BasisU and ASTC encoders.
            #
            # To avoid this set a define to prevent the compiler using
            # constexpr mutex constructors. Remove this eventually after
            # in-use JVM installations have at least this VC runtime. Remove
            # also from ASTCENC_LIB_TARGET settings around line 1169.
            $<$<AND:${is_msvccl},$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,19.40.33811>>:_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR>
            $<$<AND:${is_clangcl},$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,17.0.3>>:_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR>
        PUBLIC # only for basisu_c_binding.
            BASISU_NO_ITERATOR_DEBUG_LEVEL
        )
        # The generator automatically sets the needed VCLinker
        # option when a .def file is seen in sources.
        # C++ names are mangled differently according to compiler and ABI.
        # Only internalexport_write has a c++ name.
        if(MINGW)
            target_sources(
                ${target}
            PRIVATE
                def/internalexport.def
                $<${enable_write}:def/internalexport_write_mingw.def>
            )
            # Need these flags if mingw happens to target the ucrt (new) rather
            # than the legacy msvcrt. Otherwise tests will fail to run because
            # the necessary dlls will be missing. If we statically link
            # them instead it's fine. This does not cause any abberations if
            # the mingw toolchain targets msvcrt instead.
            target_link_options(${target} PUBLIC -static-libgcc -static-libstdc++)
        else()
            target_sources(
                ${target}
            PRIVATE
                def/internalexport.def
                $<${enable_write}:def/internalexport_write.def>
            )
        endif()
    elseif(EMSCRIPTEN)
        target_compile_definitions(${target} PRIVATE
            # To reduce size, don't support transcoding to formats not
            # supported # by WebGL.
            BASISD_SUPPORT_ATC=0
            BASISD_SUPPORT_PVRTC2=0
            # Don't support higher quality mode to avoid 64k table.
            BASISD_SUPPORT_ASTC_HIGHER_OPAQUE_QUALITY=0
            KTX_OMIT_VULKAN=1
        )
        target_link_options(${target} INTERFACE
#            "SHELL:-s ASSERTIONS=2"
#            "SHELL:-s SAFE_HEAP=1"
#            "SHELL:-s STACK_OVERFLOW_CHECK=2"
            "SHELL:-s ALLOW_MEMORY_GROWTH=1"
            "SHELL:-s MALLOC=emmalloc"
            "SHELL:-s FULL_ES3=1"
            "SHELL:-s GL_ENABLE_GET_PROC_ADDRESS=1" # For Emscripten 3.1.51+
        )
    endif()

    if(LIBKTX_FEATURE_KTX1)
        target_compile_definitions(${target} PUBLIC KTX_FEATURE_KTX1)
        target_sources(
            ${target}
        PRIVATE
            src/texture1.c
            src/texture1.h
        )
    endif()

    if(LIBKTX_FEATURE_KTX2)
        target_compile_definitions(${target} PUBLIC KTX_FEATURE_KTX2)
    endif()

    if(MINGW AND TARGET Threads::Threads AND CMAKE_USE_PTHREADS_INIT)
        target_compile_definitions(${target} PRIVATE WIN32_HAS_PTHREADS)
    endif()

    if(LIBKTX_FEATURE_VK_UPLOAD)
        target_sources(
            ${target}
        PRIVATE
            include/ktxvulkan.h
            src/vk_funcs.c
            src/vk_funcs.h
            src/vkloader.c
        )
        target_include_directories(
            ${target}
        PRIVATE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../external/dfdutils>
            $<INSTALL_INTERFACE:../external/dfdutils>
        )

        get_target_property( libktx_public_header ${target} PUBLIC_HEADER )
        list(APPEND libktx_public_header include/ktxvulkan.h)
        set_target_properties(${target} PROPERTIES
            PUBLIC_HEADER "${libktx_public_header}"
        )
    else()
        target_compile_definitions( ${target} PRIVATE KTX_OMIT_VULKAN=1 )
    endif()

    # Adding write capability to target ktx
    if(${enable_write})
        target_compile_definitions(
            ${target}
        PUBLIC
            KTX_FEATURE_WRITE
        PRIVATE
            BASISU_SUPPORT_OPENCL=$<IF:$<BOOL:${BASISU_OPENCL}>,1,0>
        )
        target_compile_options(
            ${target}
        PRIVATE
            $<$<AND:$<BOOL:${BASISU_SUPPORT_SSE}>,$<CXX_COMPILER_ID:AppleClang,Clang,GNU>>:
                -msse4.1
            >
        )
        if(EMSCRIPTEN)
            target_link_options(
                ${target}
            INTERFACE
                # Default 64kb not enough for encode_uastc.
                "SHELL:-s STACK_SIZE=96kb"
            )
        endif()
        target_link_libraries(
            ${target}
        PRIVATE
            $<$<BOOL:${BASISU_SUPPORT_OPENCL}>:${OpenCL_LIBRARY}>
        )
    endif()
endmacro(common_libktx_settings)

macro(add_lib_dependencies target lib)
    if(NOT BUILD_SHARED_LIBS)
        # Make a single static library to simplify linking.
        target_sources(${target} PRIVATE $<TARGET_OBJECTS:${lib}>)
        target_include_directories(${target} PRIVATE $<TARGET_PROPERTY:${lib},INTERFACE_INCLUDE_DIRECTORIES>)
    else()
        target_link_libraries(${target} PRIVATE ${lib})
    endif()
endmacro(add_lib_dependencies)

####################################################
# Basis Universal Encoder.
####################################################

set(BASISU_TOOL FALSE)
set(BASISU_EXAMPLES FALSE)
if(NOT ${CPU_ARCHITECTURE} STREQUAL "x86_64")
    # Basisu sets this TRUE if MSVC is TRUE.
    set(BASISU_SSE FALSE)
endif()
#set(BASISU_ZSTD FALSE)
add_subdirectory(../external/basis_universal basisu-encoder)

###################################################
# ktx libraries
###################################################

if(LIBKTX_VERSION_FULL)
    add_library( ktx ${LIB_TYPE}
        ${LIBKTX_MAIN_SRC}
        src/basis_encode.cpp
        src/writer1.c
        src/writer2.c
    )
    common_libktx_settings(ktx 1 ${LIB_TYPE})
    add_lib_dependencies(ktx basisu_encoder)
endif()

# Read-only library
if(LIBKTX_VERSION_READ_ONLY)
    add_library( ktx_read ${LIB_TYPE}
        ${LIBKTX_MAIN_SRC}
    )
    common_libktx_settings(ktx_read 0 ${LIB_TYPE})
    add_lib_dependencies(ktx_read basisu_encoder)
endif()

create_version_header(src ktx)
if(PROJECT_IS_TOP_LEVEL)
    create_version_file()
endif()

# In C++ apps that use statically linked Libraries all compilatiom units must
# be compiled with matching symbol visibility settings to avoid warnings from
# clang. Many 3rd party libraries, including libassimp which is used by the
# load test apps that statically link also to several internal libraries, use
# "hidden" to avoid conflicts with other libraries.
#
# TODO: set "hidden" as a global option. I do not want to take the time right
# now to deal with the fallout from hiding globals in libktx. Apart from
# having to mark all the public symbols of libktx for clang and gcc with
# __attribute__((visibility("default"))) there will be ramifications to
# texturetests and unittests. Marking the public symbols is easy for those
# already tagged with KTX_API. But all the symbols exported via
# internalexport.def and internalexport_write.def have to be tagged with
# KTX_API which may also require additional inclusion of ktx.h to get the
# definition.
set (STATIC_APP_LIB_SYMBOL_VISIBILITY hidden)
if(NOT BUILD_SHARED_LIBS AND
   (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    set_source_files_properties(
        lib/astc_encode.cpp
        PROPERTIES COMPILE_OPTIONS "-fvisibility=hidden"
    )
endif()


####################################################
# ASTC Encoder.
####################################################

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
add_subdirectory(../external/astc-encoder astc-encoder)
set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_RESET})
set_property(TARGET ${ASTCENC_LIB_TARGET} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_compile_definitions(
    ${ASTCENC_LIB_TARGET}
PRIVATE
    # ASTC encoder uses std::mutex. For more info. see comment about same
    # setting in common_libktx_settings starting about line 355. To be eventually
    # removed as noted in that comment.
    $<$<AND:${is_msvccl},$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,19.40.33811>>:_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR>
    $<$<AND:${is_clangcl},$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,17.0.3>>:_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR>
)

if(NOT PROJECT_IS_TOP_LEVEL AND ${CMAKE_PROJECT_NAME} STREQUAL "KTX-Software")
    # ktxdiff needs access to this variable to find the ASTC
    # library to link with so set it in parent.
    set(ASTCENC_LIB_TARGET ${ASTCENC_LIB_TARGET} PARENT_SCOPE)
endif()

if(LIBKTX_VERSION_FULL)
    add_lib_dependencies(ktx ${ASTCENC_LIB_TARGET})
endif()
if(LIBKTX_VERSION_READ_ONLY)
    add_lib_dependencies(ktx_read ${ASTCENC_LIB_TARGET})
endif()

# Install

if(LIBKTX_VERSION_FULL)
    list(APPEND LIBKTX_INSTALL_TARGETS ktx)
endif()
# ktx_read has never been included in the install packages and no
# issues have been filed. Only install it if building static libs
# or full version is not being built.
if(LIBKTX_VERSION_READ_ONLY AND NOT (BUILD_SHARED_LIBRARIES OR LIBKTX_VERSION_FULL))
    list(APPEND LIBKTX_INSTALL_TARGETS ktx_read)
endif()
if(NOT BUILD_SHARED_LIBS AND NOT APPLE)
    list(APPEND LIBKTX_INSTALL_TARGETS ${ASTCENC_LIB_TARGET} basisu_encoder)
endif()

if(APPLE OR LINUX)
    # Have library's name links as separate component
    set(KTX_NAMELINKS ON)
    install(TARGETS ${LIBKTX_INSTALL_TARGETS}
        EXPORT KTXTargets
        ARCHIVE
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT library
        FRAMEWORK
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT library
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT library
            NAMELINK_SKIP
        PUBLIC_HEADER
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            COMPONENT dev
    )
    install(TARGETS ${LIBKTX_INSTALL_TARGETS}
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT library
            NAMELINK_ONLY
    )
else()
    # No name links on Windows
    set(KTX_NAMELINKS OFF)
    install(TARGETS ${LIBKTX_INSTALL_TARGETS}
        EXPORT KTXTargets
        ARCHIVE
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT dev
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT library
        RUNTIME
            DESTINATION ${CMAKE_INSTALL_BINDIR}
            COMPONENT library
        PUBLIC_HEADER
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            COMPONENT dev
    )
endif()

# Use of this to install KHR/khr_df.h is due to CMake's failure to
# preserve the include source folder hierarchy.
# See https://gitlab.kitware.com/cmake/cmake/-/issues/16739.
if (APPLE_LOCKED_OS)
    set_source_files_properties(
          include/KHR/khr_df.h
          PROPERTIES MACOSX_PACKAGE_LOCATION Headers/KHR
    )
else()
    include(GNUInstallDirs)
    install(FILES include/KHR/khr_df.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/KHR
        COMPONENT dev
    )
endif()

install(EXPORT KTXTargets
    FILE KtxTargets.cmake
    NAMESPACE KTX::
    DESTINATION lib/cmake/ktx
    COMPONENT dev
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "KtxConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

install( FILES
    "cmake/KtxConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/KtxConfigVersion.cmake"
    DESTINATION lib/cmake/ktx
    COMPONENT dev
)

# vim:ai:ts=4:sts=2:sw=2:expandtab

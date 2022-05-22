# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.15)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules/")

find_package(Bash REQUIRED)

include(cmake/version.cmake)

project(KTX-Software
    VERSION ${KTX_VERSION}
    DESCRIPTION "Libraries and tools to create and read KTX image texture files."
)

include(CTest)

include(GNUInstallDirs)
include(CMakeDependentOption)

include(cmake/xcode.cmake)
include(cmake/cputypetest.cmake)

find_package(OpenCL)

# OPTIONS

if( IOS OR EMSCRIPTEN )
    set( LIB_TYPE_DEFAULT ON )
else()
    set( LIB_TYPE_DEFAULT OFF )
endif()

CMAKE_DEPENDENT_OPTION( KTX_FEATURE_TOOLS
                        "Create KTX tools"
                        ON
                        "NOT IOS;NOT ANDROID;NOT EMSCRIPTEN"
                        OFF
)
option( KTX_FEATURE_DOC "Create KTX documentation" OFF )
option( KTX_FEATURE_LOADTEST_APPS "Create load tests apps that load and display various KTX textures" OFF )
option( KTX_FEATURE_STATIC_LIBRARY "Create static libraries (shared otherwise)" ${LIB_TYPE_DEFAULT} )
option( KTX_FEATURE_TESTS "Create unit tests" ON )
option( KTX_FEATURE_JNI "Create Java bindings for libktx" OFF )

if(POLICY CMP0127)
    # cmake_dependent_option() supports full Condition Syntax. Introduced in
    # 3.22. Not all build environments have 3.22+. Set policy to avoid warning.
    # Seems the parens in the match string trigger the warning.
    cmake_policy(SET CMP0127 NEW)
endif()
set_target_processor_type(CPU_ARCHITECTURE)
CMAKE_DEPENDENT_OPTION( BASISU_SUPPORT_SSE
    "Compile with SSE support so applications can choose to use it."
    ON
    "NOT CMAKE_OSX_ARCHITECTURES STREQUAL \"$(ARCHS_STANDARD)\"; CPU_ARCHITECTURE STREQUAL x86_64 OR CPU_ARCHITECTURE STREQUAL x86"
    OFF
)
CMAKE_DEPENDENT_OPTION( BASISU_SUPPORT_OPENCL
    "Compile with OpenCL support so applications can choose to use it."
    OFF
    "OpenCL_FOUND OR WIN32"
    OFF
)

if(BASISU_SUPPORT_OPENCL AND WIN32 AND NOT OpenCL_FOUND)
    # To avoid fiddly setting up of OpenCL on Windows CI VMs, use copy in repo.
    set(OpenCL_INCLUDE_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/basisu/opencl"
        # FORCE to override *-NOTFOUND set by the failed find.
        CACHE PATH "" FORCE
    )
    set(OpenCL_LIBRARY
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/basisu/opencl/lib/OpenCL64.lib"
        CACHE FILEPATH "" FORCE
    )
    set(OpenCL_INCLUDE_DIRS ${OpenCL_INCLUDE_DIR})
    set(OpenCL_LIBRARIES ${OpenCL_LIBRARY})
endif()


CMAKE_DEPENDENT_OPTION( KTX_EMBED_BITCODE
    "Embed bitcode in binaries"
    OFF
    "APPLE AND IOS"
    OFF
)

if(APPLE)
    set(XCODE_CODE_SIGN_IDENTITY "Development" CACHE STRING "Xcode code sign ID")
    set(XCODE_DEVELOPMENT_TEAM "" CACHE STRING "Xcode development team ID")
    set(PRODUCTBUILD_IDENTITY_NAME "" CACHE STRING "productbuild identity name")
    set(PRODUCTBUILD_KEYCHAIN_PATH "" CACHE FILEPATH "pkgbuild keychain file")
    if(IOS)
        set(XCODE_PROVISIONING_PROFILE_SPECIFIER "" CACHE STRING "Xcode provisioning profile specifier")
        set(MOLTENVK_SDK "$ENV{VULKAN_SDK}/../MoltenVK" CACHE STRING "MoltenVK directory inside the Vulkan SDK")
    endif()
endif()

option( KTX_FEATURE_KTX1 "Enable KTX 1 support" ON )
option( KTX_FEATURE_KTX2 "Enable KTX 2 support" ON )
option( KTX_FEATURE_VULKAN "Enable Vulkan texture upload" ON )
option( KTX_FEATURE_GL_UPLOAD "Enable OpenGL texture upload" ON )

# Platform specific settings

if (WIN32 AND ${CMAKE_SYSTEM_NAME} STREQUAL "WindowsStore")
    # Disable OpenGL upload on Universal Windows Platform
    set(KTX_FEATURE_GL_UPLOAD OFF)
endif()

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE STRING "macOS Deployment Target")
    if(IOS)
        set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "11.0" CACHE STRING "iOS Deployment Target")
        set(CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH NO)
    endif()
endif()

if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN AND NOT ANDROID)
    set(LINUX TRUE)
endif()

if(EMSCRIPTEN)
    set( KTX_FEATURE_VULKAN OFF )
endif()

set(bitness 64)
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8 OR FORCE32)
        set(bitness 32)
endif()

if(KTX_FEATURE_STATIC_LIBRARY)
    set(LIB_TYPE STATIC)
else()
    if(IOS OR EMSCRIPTEN)
        message(SEND_ERROR "Library type cannot be shared for the current platform. Set KTX_FEATURE_STATIC_LIBRARY to ON!")
    endif()
    set(LIB_TYPE SHARED)
endif()

# Depends on the settings, so it must be included after
include(cmake/mkvk.cmake)

# Global compile & link options including optimization flags
if(MSVC)
    # With /W4 VS2015 (V19.0) issues many warnings that VS2017 & 2019 don't
    # so only increase warning level for recent versions.
    add_compile_options($<$<VERSION_GREATER_EQUAL:${CMAKE_CXX_COMPILER_VERSION},19.16>:/W4>)
    add_compile_options( $<IF:$<CONFIG:Debug>,/Gz,/O2> )
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU"
       OR ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    add_compile_options(-Wall -Wextra)
    add_compile_options( $<IF:$<CONFIG:Debug>,-O0,-O3> )
    if(EMSCRIPTEN)
        add_link_options( $<IF:$<CONFIG:Debug>,-gsource-map,-O3> )
    else()
        add_link_options( $<IF:$<CONFIG:Debug>,-g,-O3> )
    endif()
else()
    message(ERROR "${CMAKE_CXX_COMPILER_ID} not yet supported.")
endif()

set(KTX_MAIN_SRC
    include/ktx.h
    lib/basis_sgd.h
    lib/basis_transcode.cpp
    lib/basisu/transcoder/basisu_containers.h
    lib/basisu/transcoder/basisu_containers_impl.h
    lib/basisu/transcoder/basisu_file_headers.h
    lib/basisu/transcoder/basisu_transcoder_internal.h
    lib/basisu/transcoder/basisu_transcoder_uastc.h
    lib/basisu/transcoder/basisu_transcoder.cpp
    lib/basisu/transcoder/basisu_transcoder.h
    lib/basisu/transcoder/basisu.h
    lib/basisu/zstd/zstd.c
    lib/checkheader.c
    lib/dfdutils/createdfd.c
    lib/dfdutils/colourspaces.c
    lib/dfdutils/dfd.h
    lib/dfdutils/interpretdfd.c
    lib/dfdutils/printdfd.c
    lib/dfdutils/queries.c
    lib/dfdutils/vk2dfd.c
    lib/dfdutils/vk2dfd.inl
    lib/dfdutils/vulkan/vk_platform.h
    lib/dfdutils/vulkan/vulkan_core.h
    lib/etcdec.cxx
    lib/etcunpack.cxx
    lib/filestream.c
    lib/filestream.h
    lib/formatsize.h
    lib/gl_format.h
    lib/hashlist.c
    lib/info.c
    lib/ktxint.h
    lib/memstream.c
    lib/memstream.h
    lib/strings.c
    lib/swap.c
    lib/texture.c
    lib/texture.h
    lib/texture2.c
    lib/texture2.h
    lib/uthash.h
    lib/vk_format.h
    lib/vkformat_check.c
    lib/vkformat_enum.h
    lib/vkformat_str.c
    )

if(KTX_FEATURE_GL_UPLOAD)
    list(APPEND KTX_MAIN_SRC
        lib/gl_funcs.c
        lib/gl_funcs.h
        lib/glloader.c
    )
endif()

# Main library
add_library( ktx ${LIB_TYPE}
    ${KTX_MAIN_SRC}
    )

# Read-only library
add_library( ktx_read ${LIB_TYPE}
    ${KTX_MAIN_SRC}
    )

macro(commom_lib_settings lib write)

    if(TARGET mkvk)
        # Creating vulkan headers is only required when Vulkan SDK updates.
        add_dependencies(${lib} mkvk)
    endif()

    set_target_properties(${lib} PROPERTIES
        PUBLIC_HEADER ${CMAKE_SOURCE_DIR}/include/ktx.h
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
    )
    set_xcode_code_sign(${lib})

    target_compile_definitions(
        ${lib}
    PUBLIC
        "$<$<CONFIG:Debug>:_DEBUG;DEBUG>"
    PRIVATE
        LIBKTX
    )

    # C/C++ Standard
    target_compile_features(${lib} PUBLIC c_std_99 cxx_std_11)

    # Compiler Warning Flags
    if(EMSCRIPTEN)
        target_compile_options(${lib} PRIVATE
            -Wno-nested-anon-types
            -Wno-gnu-anonymous-struct
        )
    else()
        target_compile_options(${lib} PRIVATE
            # clang options
            $<$<CXX_COMPILER_ID:AppleClang,Clang>:
                -Wno-nested-anon-types
                -Wno-gnu-anonymous-struct
            >
            # not clang options
            $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
                -Wno-pedantic
            >
        )
    endif()

    target_include_directories(
        ${lib}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/basisu/transcoder>
        $<INSTALL_INTERFACE:lib/basisu/transcoder>

        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/basisu/zstd>
        $<INSTALL_INTERFACE:lib/basisu/zstd>

        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/other_include>
        $<INSTALL_INTERFACE:other_include>

        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils>
        $<INSTALL_INTERFACE:utils>
    )

    if( LIB_TYPE STREQUAL STATIC )
        # KHRONOS_STATIC overrides the value of KTX_API set below.
        target_compile_definitions(${lib} PUBLIC KHRONOS_STATIC)
    endif()

    # To reduce size, don't support transcoding to ancient formats.
    target_compile_definitions(${lib} PRIVATE BASISD_SUPPORT_FXT1=0)

    # TODO: make options for all formats and good per-platform defaults
    # - BASISD_SUPPORT_UASTC
    # - BASISD_SUPPORT_DXT1 (BC1)
    # - BASISD_SUPPORT_DXT5A (BC3/4/5)
    # - BASISD_SUPPORT_BC7
    # - BASISD_SUPPORT_BC7_MODE5
    # - BASISD_SUPPORT_PVRTC1
    # - BASISD_SUPPORT_ETC2_EAC_A8
    # - BASISD_SUPPORT_ASTC
    # - BASISD_SUPPORT_ATC
    # - BASISD_SUPPORT_ASTC_HIGHER_OPAQUE_QUALITY
    # - BASISD_SUPPORT_ETC2_EAC_RG11
    # - BASISD_SUPPORT_FXT1
    # - BASISD_SUPPORT_PVRTC2

    if(WIN32)
        target_compile_definitions(
            ${lib}
        PRIVATE
            # KHRONOS_STATIC defined earlier for the static library overrides
            # so no need for a generator expression to conditionally set this.
            KTX_API=__declspec\(dllexport\)
        PUBLIC # only for basisu_c_binding.
            BASISU_NO_ITERATOR_DEBUG_LEVEL
        )
        # The generator automatically sets the needed VCLinker
        # option when a .def file is seen in sources.
        # The def files that we add have a different syntax depending on the ABI
        if(MINGW)
            target_sources(
                ${lib}
            PRIVATE
                lib/internalexport_mingw.def
                $<${write}:lib/internalexport_write_mingw.def>
            )
        else()
            target_sources(
                ${lib}
            PRIVATE
                lib/internalexport.def
                $<${write}:lib/internalexport_write.def>
            )
        endif()
    elseif(EMSCRIPTEN)
        target_compile_definitions(${lib} PRIVATE
            # To reduce size, don't support transcoding to formats not
            # supported # by WebGL.
            BASISD_SUPPORT_ATC=0
            BASISD_SUPPORT_PVRTC2=0
            # Don't support higher quality mode to avoid 64k table.
            BASISD_SUPPORT_ASTC_HIGHER_OPAQUE_QUALITY=0
            KTX_OMIT_VULKAN=1
        )
    endif()

    if(KTX_FEATURE_KTX1)
        target_compile_definitions(${lib} PUBLIC KTX_FEATURE_KTX1)
        target_sources(
            ${lib}
        PRIVATE
            lib/texture1.c
            lib/texture1.h
        )
    endif()

    if(KTX_FEATURE_KTX2)
        target_compile_definitions(${lib} PUBLIC KTX_FEATURE_KTX2)
    endif()

    if(WIN32)
        # By wrapping in generator expression we force multi configuration
        # generators (like Visual Studio) to take the exact path and not
        # change it.
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${KTX_BUILD_DIR}/$<CONFIG>>)
        
        if(MINGW)
            # Check if the Threads package is provided; if using Mingw it MIGHT be
            find_package(Threads)
            if(Threads_FOUND)
                target_compile_definitions(ktx PRIVATE WIN32_HAS_PTHREADS)
                target_link_libraries(ktx PRIVATE Threads::Threads)
            endif()
        endif()

    elseif(APPLE)
        if(KTX_EMBED_BITCODE)
            target_compile_options(${lib} PRIVATE "-fembed-bitcode")
        endif()
        if(NOT IOS)
            # Set a common RUNTIME_OUTPUT_DIR for all targets, so that
            # INSTALL RPATH is functional in build directory as well.
            # BUILD_WITH_INSTALL_RPATH is necessary for working code signing.
            set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${KTX_BUILD_DIR}/$<CONFIG>)
        endif()

    elseif(LINUX)
        find_package(Threads REQUIRED)
        target_link_libraries(
            ${lib}
        PUBLIC
            dl
            Threads::Threads
        )
    endif()

    if(KTX_FEATURE_VULKAN)
        target_sources(
            ${lib}
        PRIVATE
            include/ktxvulkan.h
            lib/vk_funcs.c
            lib/vk_funcs.h
            lib/vkloader.c
        )
        target_include_directories(
            ${lib}
        PRIVATE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/dfdutils>
            $<INSTALL_INTERFACE:lib/dfdutils>
        )

        get_target_property( KTX_PUBLIC_HEADER ${lib} PUBLIC_HEADER )
        list(APPEND KTX_PUBLIC_HEADER ${CMAKE_SOURCE_DIR}/include/ktxvulkan.h)
        set_target_properties(${lib} PROPERTIES
            PUBLIC_HEADER "${KTX_PUBLIC_HEADER}"
        )
    else()
        target_compile_definitions( ${lib} PRIVATE KTX_OMIT_VULKAN=1 )
    endif()

endmacro(commom_lib_settings)

set(KTX_BUILD_DIR "${CMAKE_BINARY_DIR}")

commom_lib_settings(ktx 1)
commom_lib_settings(ktx_read 0)

if(KTX_FEATURE_JNI)
    add_subdirectory(interface/java_binding)
endif()

create_version_header(lib ktx)
create_version_file()

target_compile_definitions(
    ktx_read
PRIVATE
    # We're reading the files ourselves so don't need Basis KTX v2 support.
    BASISD_SUPPORT_KTX2_ZSTD=0
    BASISD_SUPPORT_KTX2=0
)

# Adding write capability to target ktx

set(BASISU_ENCODER_CXX_SRC
    lib/basisu/encoder/basisu_backend.cpp
    lib/basisu/encoder/basisu_backend.h
    lib/basisu/encoder/basisu_basis_file.cpp
    lib/basisu/encoder/basisu_basis_file.h
    lib/basisu/encoder/basisu_bc7enc.cpp
    lib/basisu/encoder/basisu_bc7enc.h
    lib/basisu/encoder/basisu_comp.cpp
    lib/basisu/encoder/basisu_comp.h
    lib/basisu/encoder/basisu_enc.cpp
    lib/basisu/encoder/basisu_enc.h
    lib/basisu/encoder/basisu_etc.cpp
    lib/basisu/encoder/basisu_etc.h
    lib/basisu/encoder/basisu_frontend.cpp
    lib/basisu/encoder/basisu_frontend.h
    lib/basisu/encoder/basisu_gpu_texture.cpp
    lib/basisu/encoder/basisu_gpu_texture.h
    lib/basisu/encoder/basisu_kernels_declares.h
    lib/basisu/encoder/basisu_kernels_imp.h
    lib/basisu/encoder/basisu_kernels_sse.cpp
    lib/basisu/encoder/basisu_miniz.h
    lib/basisu/encoder/basisu_opencl.cpp
    lib/basisu/encoder/basisu_opencl.h
    lib/basisu/encoder/basisu_pvrtc1_4.cpp
    lib/basisu/encoder/basisu_pvrtc1_4.h
    lib/basisu/encoder/basisu_resample_filters.cpp
    lib/basisu/encoder/basisu_resampler_filters.h
    lib/basisu/encoder/basisu_resampler.cpp
    lib/basisu/encoder/basisu_resampler.h
    lib/basisu/encoder/basisu_ssim.cpp
    lib/basisu/encoder/basisu_ssim.h
    lib/basisu/encoder/basisu_uastc_enc.cpp
    lib/basisu/encoder/basisu_uastc_enc.h
    lib/basisu/encoder/cppspmd_flow.h
    lib/basisu/encoder/cppspmd_math.h
    lib/basisu/encoder/cppspmd_math_declares.h
    lib/basisu/encoder/cppspmd_sse.h
    lib/basisu/encoder/cppspmd_type_aliases.h
)

target_sources(
    ktx
PRIVATE
    lib/basis_encode.cpp
    lib/astc_encode.cpp
    ${BASISU_ENCODER_C_SRC}
    ${BASISU_ENCODER_CXX_SRC}
    lib/writer1.c
    lib/writer2.c
)

# Turn off these warnings until Rich fixes the occurences.
# It it not clear to me if generator expressions can be used here
# hence the long-winded way.
#message(STATUS
#        "CMAKE_CXX_COMPILER_ID = ${CMAKE_CXX_COMPILER_ID} "
#        "CMAKE_CXX_COMPILER_VERSION = ${CMAKE_CXX_COMPILER_VERSION}"
#)
if(MSVC)
# Currently no need to disable any warnings in basisu code. Rich fixed them.
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    set_source_files_properties(
        # It's too much work to discriminate which files need which warnings
        # disabled.
        ${BASISU_ENCODER_CXX_SRC}
        PROPERTIES COMPILE_OPTIONS "-Wno-sign-compare;-Wno-unused-variable;-Wno-class-memaccess;-Wno-misleading-indentation;-Wno-extra;-Wno-deprecated-copy;-Wno-parentheses;-Wno-strict-aliasing"
    )
    set_source_files_properties(
        lib/basisu/transcoder/basisu_transcoder.cpp
        PROPERTIES COMPILE_OPTIONS "-Wno-sign-compare;-Wno-unused-function;-Wno-unused-variable;-Wno-class-memaccess;-Wno-maybe-uninitialized"
    )
    if (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "11.0")
        set_source_files_properties(
            lib/basisu/zstd/zstd.c
            PROPERTIES COMPILE_OPTIONS "-Wno-unused-but-set-variable"
        )
    endif()
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    if (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "12.0.5")
        set_source_files_properties( lib/basisu/encoder/basisu_kernels_sse.cpp
            PROPERTIES COMPILE_OPTIONS "-Wno-unused-parameter;-Wno-deprecated-copy;-Wno-uninitialized-const-reference"
            )
    else()
        set_source_files_properties( lib/basisu/encoder/basisu_kernels_sse.cpp
            PROPERTIES COMPILE_OPTIONS "-Wno-unused-parameter"
        )
    endif()
    if (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "13.1")
        set_source_files_properties(
            # We haven't fixed zstd.c because the fix would have to be applied
            # every time the upstream script is used to create an updated
            # single file decoder.
            lib/basisu/zstd/zstd.c
            PROPERTIES COMPILE_OPTIONS "-Wno-unused-but-set-variable"
        )
    endif()
    if (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "15.0")
        # These are for Emscripten which is ahead of xcode in its clang
        # version. Also future proofing for when xcode catches up.
        set_source_files_properties(
            ${BASISU_ENCODER_CXX_SRC}
            PROPERTIES COMPILE_OPTIONS "-Wno-sign-compare;-Wno-unused-variable;-Wno-unused-parameter"
        )
        set_source_files_properties(
            lib/basisu/transcoder/basisu_transcoder.cpp
            PROPERTIES COMPILE_OPTIONS "-Wno-sign-compare;-Wno-unused-function;-Wno-unused-variable"
        )
        set_source_files_properties(
            lib/basisu/zstd/zstd.c
            PROPERTIES COMPILE_OPTIONS "-Wno-unused-but-set-variable;-Wno-bitwise-instead-of-logical"
        )
    endif()
else()
    message(ERROR "${CMAKE_CXX_COMPILER_ID} not yet supported.")
endif()

target_include_directories(
    ktx
PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/basisu>
    $<INSTALL_INTERFACE:lib/basisu>
    $<$<BOOL:${BASISU_SUPPORT_OPENCL}>:${OpenCL_INCLUDE_DIRS}>
)
target_compile_definitions(
    ktx
PUBLIC
    KTX_FEATURE_WRITE
PRIVATE
    # BASISD_SUPPORT_KTX2 has to be 1 to compile the encoder. We
    # don't use it. Hopefully it doesn't add too much code. We're using
    # the zstd encoder in basisu by explicitly including the file in our
    # source list. We don't need the related code in the encoder.
    BASISD_SUPPORT_KTX2_ZSTD=0
    BASISD_SUPPORT_KTX2=1
    $<$<BOOL:${BASISU_SUPPORT_SSE}>:BASISU_SUPPORT_SSE=1>
    $<$<NOT:$<BOOL:${BASISU_SUPPORT_SSE}>>:BASISU_SUPPORT_SSE=0>
    $<$<BOOL:${BASISU_SUPPORT_OPENCL}>:BASISU_SUPPORT_OPENCL=1>
    $<$<NOT:$<BOOL:${BASISU_SUPPORT_OPENCL}>>:BASISU_SUPPORT_OPENCL=0>
)
target_compile_options(
    ktx
PRIVATE
    $<$<AND:$<BOOL:${BASISU_SUPPORT_SSE}>,$<CXX_COMPILER_ID:AppleClang,Clang,GNU>>:
        -msse4.1
    >
)
target_link_libraries(
    ktx
PRIVATE
    $<$<BOOL:${BASISU_SUPPORT_OPENCL}>:${OpenCL_LIBRARIES}>
)

if(EMSCRIPTEN)
    set(
        KTX_EMC_LINK_FLAGS
        --bind
        "SHELL:--source-map-base ./"
        "SHELL:-s ALLOW_MEMORY_GROWTH=1"
        "SHELL:-s ASSERTIONS=0"
        "SHELL:-s MALLOC=emmalloc"
        "SHELL:-s MODULARIZE=1"
        "SHELL:-s FULL_ES3=1"
    )

    add_executable( ktx_js interface/js_binding/ktx_wrapper.cpp )
    target_link_libraries( ktx_js ktx_read )
    target_include_directories( ktx_js PRIVATE $<TARGET_PROPERTY:ktx_read,INTERFACE_INCLUDE_DIRECTORIES> )
    target_link_options(
        ktx_js
    PUBLIC
        ${KTX_EMC_LINK_FLAGS}
        "SHELL:-s EXPORT_NAME=LIBKTX"
        "SHELL:-s EXPORTED_RUNTIME_METHODS=[\'GL\']"
        "SHELL:-s GL_PREINITIALIZED_CONTEXT=1"
    )
    set_target_properties( ktx_js PROPERTIES OUTPUT_NAME "libktx")

    add_custom_command(
        TARGET ktx_js
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:ktx_js>/$<TARGET_FILE_PREFIX:ktx_js>$<TARGET_FILE_BASE_NAME:ktx_js>.js" "${PROJECT_SOURCE_DIR}/tests/webgl"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:ktx_js>/$<TARGET_FILE_PREFIX:ktx_js>$<TARGET_FILE_BASE_NAME:ktx_js>.wasm" "${PROJECT_SOURCE_DIR}/tests/webgl"
        COMMENT "Copy libktx.js and libktx.wasm to tests/webgl"
    )

    install(TARGETS ktx_js
        RUNTIME
            DESTINATION .
            COMPONENT ktx_js
    )
    install(FILES ${CMAKE_BINARY_DIR}/libktx.wasm
        DESTINATION .
        COMPONENT ktx_js
    )

    add_executable( msc_basis_transcoder_js interface/js_binding/transcoder_wrapper.cpp )
    target_link_libraries( msc_basis_transcoder_js ktx_read )
    target_include_directories( msc_basis_transcoder_js
        PRIVATE
        lib
        lib/basisu/transcoder
    )

    # Re-use ktx's compile options
    target_compile_options(msc_basis_transcoder_js
    PRIVATE
       $<TARGET_PROPERTY:ktx_read,INTERFACE_COMPILE_OPTIONS>
       )

    target_link_options(
        msc_basis_transcoder_js
    PUBLIC
        ${KTX_EMC_LINK_FLAGS}
        "SHELL:-s EXPORT_NAME=MSC_TRANSCODER"
        # Re-use ktx's link options
        $<TARGET_PROPERTY:ktx_read,INTERFACE_LINK_OPTIONS>
        )
    set_target_properties( msc_basis_transcoder_js PROPERTIES OUTPUT_NAME "msc_basis_transcoder")

    add_custom_command(
        TARGET ktx_js
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:msc_basis_transcoder_js>/$<TARGET_FILE_PREFIX:msc_basis_transcoder_js>$<TARGET_FILE_BASE_NAME:msc_basis_transcoder_js>.js" "${PROJECT_SOURCE_DIR}/tests/webgl"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:msc_basis_transcoder_js>/$<TARGET_FILE_PREFIX:msc_basis_transcoder_js>$<TARGET_FILE_BASE_NAME:msc_basis_transcoder_js>.wasm" "${PROJECT_SOURCE_DIR}/tests/webgl"
        COMMENT "Copy msc_basis_transcoder.js and msc_basis_transcoder.wasm to tests/webgl"
    )

    install(TARGETS msc_basis_transcoder_js
        RUNTIME
            DESTINATION .
            COMPONENT msc_basis_transcoder_js
    )
    install(FILES ${CMAKE_BINARY_DIR}/msc_basis_transcoder.wasm
        DESTINATION .
        COMPONENT msc_basis_transcoder_js
    )
endif()

add_library( objUtil STATIC
    utils/argparser.cpp
    utils/argparser.h
    utils/ktxapp.h
    utils/sbufstream.h
    utils/scapp.h
    utils/unused.h
    )
target_include_directories(
    objUtil
PUBLIC
    utils
)

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
set_target_properties(objUtil PROPERTIES
    CXX_VISIBILITY_PRESET ${STATIC_APP_LIB_SYMBOL_VISIBILITY}
)

add_subdirectory(interface/basisu_c_binding)

# Only one architecture is supported at once, if neither of
# ISA_SSE41 and ISA_SSE2  are defined ISA_AVX2 is chosen.
# If ISA_AVX2 fails to compile user must chose other x86 options.
# On arm based systems ISA_NEON is default

list(FIND CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD)" ASTC_BUILD_UNIVERSAL)

if(${ASTC_BUILD_UNIVERSAL} EQUAL -1)
    if (${ISA_NONE})
        set(ASTC_LIB_NAME astcenc-none-static)
    else()
        if(CPU_ARCHITECTURE STREQUAL x86_64 OR CPU_ARCHITECTURE STREQUAL x86)
            if (${ISA_SSE41})
                set(ASTC_LIB_NAME astcenc-sse4.1-static)
            elseif (${ISA_SSE2})
                set(ASTC_LIB_NAME astcenc-sse2-static)
            else()
                set(ISA_AVX2 ON)
                set(ASTC_LIB_NAME astcenc-avx2-static)
            endif()
            if(CPU_ARCHITECTURE STREQUAL x86)
                set(ISA_NONE ON)
                set(ISA_AVX2 OFF)
                set(ASTCENC_POPCNT 0)
                set(ASTC_LIB_NAME astcenc-none-static)
            endif()
        elseif(CPU_ARCHITECTURE STREQUAL armv8 OR CPU_ARCHITECTURE STREQUAL arm64)
            set(ASTC_LIB_NAME astcenc-neon-static)
            set(ISA_NEON ON)
        else()
            message(STATUS "Unsupported ISA for ${CPU_ARCHITECTURE} arch, using ISA_NONE.")
            set(ISA_NONE ON)
        endif()
    endif()
else()
    set(ASTC_LIB_NAME astcenc-static)
endif()

# astcenc
set(CLI OFF) # Only build as library not the CLI astcencoder
add_subdirectory(lib/astc-encoder)
set_property(TARGET ${ASTC_LIB_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(ktx PRIVATE ${ASTC_LIB_NAME})

# Tools
if(KTX_FEATURE_TOOLS)
    add_subdirectory(tools)
endif()

# Tests
add_subdirectory(tests)

# Documentation
if(KTX_FEATURE_DOC)
    include(cmake/docs.cmake)
endif()

set(KTX_INSTALL_TARGETS ktx)

if(KTX_FEATURE_STATIC_LIBRARY)
    list(APPEND KTX_INSTALL_TARGETS ${ASTC_LIB_NAME})
    if(BASISU_SUPPORT_OPENCL)
        list(APPEND ${OpenCL_LIBRARIES})
    endif()
endif()

# Install

if(APPLE OR LINUX)
    # Have library's name links as separate component
    set(KTX_NAMELINKS ON)
    install(TARGETS ${KTX_INSTALL_TARGETS}
        EXPORT KTXTargets
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT library
            NAMELINK_SKIP
        PUBLIC_HEADER
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            COMPONENT dev
    )
    install(TARGETS ${KTX_INSTALL_TARGETS}
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT library
            NAMELINK_ONLY
    )
else()
    # No name links on Windows
    set(KTX_NAMELINKS OFF)
    install(TARGETS ${KTX_INSTALL_TARGETS}
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

install(EXPORT KTXTargets
    FILE KtxTargets.cmake
    NAMESPACE KTX::
    DESTINATION lib/cmake/ktx
    COMPONENT library
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
    COMPONENT library
)


# CPack

include(CPackComponent)

set(CPACK_PACKAGE_NAME "KTX-Software")
set(CPACK_PACKAGE_VENDOR "Khronos Group")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.khronos.org/KTX-Software")
set(CPACK_PACKAGE_CONTACT "khronos@callow.im" )

set(CPACK_RESOURCE_FILE_WELCOME "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Welcome.rtf")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ReadMe.rtf")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/License.rtf")

# Custom package file name
if( APPLE AND CMAKE_OSX_ARCHITECTURES )
    # CMAKE_SYSTEM_PROCESSOR typically set to build host processor here.
    # CMAKE_OSX_ARCHITECTURES governs target of build.
    if( ${CMAKE_OSX_ARCHITECTURES} STREQUAL "$(ARCHS_STANDARD)" )
        set(processor_name "universal")
    else()
        set(processor_name ${CMAKE_OSX_ARCHITECTURES})
    endif()
elseif( CMAKE_SYSTEM_PROCESSOR )
    set(processor_name ${CMAKE_SYSTEM_PROCESSOR})
elseif( IOS )
    # CMAKE_SYSTEM_PROCESSOR not set when building for iOS.
    set(processor_name "arm64")
endif()
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${KTX_VERSION_FULL}-${CMAKE_SYSTEM_NAME}-${processor_name}")

if(APPLE)
    install(FILES tools/package/mac/ktx-uninstall
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COMPONENT tools
    )
    set(CPACK_GENERATOR productbuild)
    # Trick the productbuild generator into creating more useful package IDs
    set(CPACK_PACKAGE_NAME "ktx")
    set(CPACK_PACKAGE_VENDOR "khronos")
    # Install at root level
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local")

    set(CPACK_PRODUCTBUILD_IDENTITY_NAME ${PRODUCTBUILD_IDENTITY_NAME})
    set(CPACK_PRODUCTBUILD_KEYCHAIN_PATH ${PRODUCTBUILD_KEYCHAIN_PATH})

    # Contains the `summary.html` file, shown at end of installation
    set(CPACK_PRODUCTBUILD_RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tools/package/mac/Resources")
    set(CPACK_PRODUCTBUILD_BACKGROUND "ktx_logo_190_xp.png")
    set(CPACK_PRODUCTBUILD_BACKGROUND_MIME_TYPE "image/png")
    set(CPACK_PRODUCTBUILD_BACKGROUND_ALIGNMENT "bottomleft")

elseif(LINUX)
    set(CPACK_GENERATOR DEB RPM TBZ2)
    set(CPACK_PACKAGE_CHECKSUM SHA1)
    set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ReadMe.txt")
elseif(WIN32)
    # Add logo to top left of installer pages. Despite the installer-
    # independent variable name, this does nothing for PRODUCTBUILD
    # hence setting here only. Format has to be old BMP (use
    # BMP3:<output file> with ImageMagick `convert`) and the final
    # separator must be as shown. Recommended size is 150x57 pixels but
    # to my eye on my screen 200x111 is much less aliased than the 150x83
    # proportionally scaled logo.
    set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/win\\\\ktx_logo_200_bmp3.bmp")
    # Set the icon for the installer and uninstaller
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/win/ktx_app.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/win/ktx_app.ico")
    # Set icon in Windows' add/remove control panel. Must be an .exe file.
    set(CPACK_NSIS_INSTALLED_ICON_NAME uninstall.exe)
    set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)
    set(CPACK_NSIS_URL_INFO_ABOUT ${CPACK_PACKAGE_HOMEPAGE_URL})
    set(CPACK_NSIS_CONTACT ${CPACK_PACKAGE_CONTACT})
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_UNINSTALL_NAME "Uninstall")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "KTX-Software")
    ## Alternative: add major version at end
    #set(CPACK_PACKAGE_INSTALL_DIRECTORY "KTX-Software-${PROJECT_VERSION_MAJOR}")
    set(CPACK_PACKAGE_CHECKSUM SHA1)
elseif(EMSCRIPTEN)
    set(CPACK_GENERATOR ZIP)
    set(CPACK_ARCHIVE_KTX_JS_FILE_NAME "${CMAKE_PROJECT_NAME}-${KTX_VERSION_FULL}-Web-libktx")
    set(CPACK_ARCHIVE_MSC_BASIS_TRANSCODER_JS_FILE_NAME "${CMAKE_PROJECT_NAME}-${KTX_VERSION_FULL}-Web-msc_basis_transcoder")

    set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
    set(CPACK_PACKAGE_CHECKSUM SHA1)
else()
    set(CPACK_PACKAGE_CHECKSUM SHA1)
endif()

cpack_add_component(library
    DISPLAY_NAME "Library"
    DESCRIPTION "Main KTX library."
    REQUIRED
)
if(KTX_FEATURE_JNI)
cpack_add_component(jni
    DISPLAY_NAME "Java wrapper"
    DESCRIPTION "Java wrapper and native interface for KTX library."
    DEPENDS library
    DISABLED
)
endif()
# if(KTX_NAMELINKS)
#     cpack_add_component(Namelinks
#         DEPENDS library
#         HIDDEN
#     )
# endif()
cpack_add_component(tools
    DISPLAY_NAME "Command line tools"
    DESCRIPTION "Command line tools for creating, converting and inspecting KTX files."
    DEPENDS library
)
cpack_add_component(dev
    DISPLAY_NAME "Development"
    DESCRIPTION "Additional resources for development (header files and documentation)."
    DEPENDS library
    DISABLED
)

if(KTX_FEATURE_LOADTEST_APPS)
    cpack_add_component(GlLoadTestApps
        GROUP LoadTestApps
        DISPLAY_NAME "OpenGL Test Applications"
        DISABLED
    )
    cpack_add_component(VkLoadTestApp
        GROUP LoadTestApps
        DISPLAY_NAME "Vulkan Test Application"
        DISABLED
    )
    cpack_add_component_group(LoadTestApps
        DISPLAY_NAME "Load Test Applications"
    )
endif()

if(EMSCRIPTEN)
    set(CPACK_COMPONENTS_ALL
        ktx_js
        msc_basis_transcoder_js
    )
else()
    set(CPACK_COMPONENTS_ALL
        library
        tools
        dev
        jni
        # Headers
    )
endif()
# if(KTX_NAMELINKS)
#     list(APPEND CPACK_COMPONENTS_ALL
#         Namelinks
#     )
# endif()

include(CPack)

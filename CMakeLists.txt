# Copyright 2015-2023 The Khronos Group Inc.
# Copyright 2022-2023 RasterGrid Kft.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.15)

include(CMakePrintHelpers)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules/")

find_package(Bash REQUIRED)

include(cmake/version.cmake)

if(POLICY CMP0149)
    # Ignore CMAKE_SYSTEM_VERSION and select either latest available
    # Windows SDK or that specified in WindowsSDKVersion environment
    # variable  Needed because OLD policy picks SDK that matches
    # system version, CI uses Windows Server 2022 and its matching
    # SDK lacks the arm64 glu32.lib which causes builds to fail.
    # MUST be set before project() command.
    cmake_policy(SET CMP0149 NEW)
endif()

project(KTX-Software
    VERSION ${KTX_VERSION}
    DESCRIPTION "Libraries and tools to create and read KTX image texture files."
)

include(CTest)

include(GNUInstallDirs)
include(CMakeDependentOption)

include(cmake/codesign.cmake)
include(cmake/cputypetest.cmake)

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
option( KTX_FEATURE_DOC "Create KTX documentation." OFF )
option( KTX_FEATURE_STATIC_LIBRARY "Create static libraries (shared otherwise)." ${LIB_TYPE_DEFAULT} )
option( KTX_FEATURE_JNI "Create Java bindings for libktx." OFF )
option( KTX_FEATURE_PY "Create Python source distribution." OFF )
option( KTX_FEATURE_TESTS "Create unit tests." ON )
option( KTX_FEATURE_TOOLS_CTS "Enable KTX CLI Tools CTS tests (requires CTS submodule)." OFF )

if(KTX_FEATURE_TOOLS_CTS AND NOT KTX_FEATURE_TOOLS)
    message(WARNING "KTX_FEATURE_TOOLS is not set -> disabling KTX_FEATURE_TOOLS_CTS.")
    set(KTX_FEATURE_TOOLS_CTS "OFF")
elseif(KTX_FEATURE_TOOLS_CTS AND NOT KTX_FEATURE_TESTS)
    message(WARNING "KTX_FEATURE_TESTS is not set -> disabling KTX_FEATURE_TOOLS_CTS.")
    set(KTX_FEATURE_TOOLS_CTS "OFF")
endif()

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

if(BASISU_SUPPORT_OPENCL)
    find_package(OpenCL)
endif()

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
    "Embed bitcode in binaries."
    OFF
    "APPLE AND IOS"
    OFF
)

option( KTX_FEATURE_KTX1 "Enable KTX 1 support." ON )
option( KTX_FEATURE_KTX2 "Enable KTX 2 support." ON )
option( KTX_FEATURE_VK_UPLOAD "Enable Vulkan texture upload." ON )
option( KTX_FEATURE_GL_UPLOAD "Enable OpenGL texture upload." ON )
# When a variable like this is set via CMakeUserPresets.json, it no longer
# shows the selectable options provided by the STRINGS property.
set( KTX_FEATURE_LOADTEST_APPS
     ""
     CACHE
     STRING
     "Load test apps test the upload feature by displaying various KTX textures. Select which to create. \"OpenGL\" includes OpenGL ES."
)
set_property( CACHE KTX_FEATURE_LOADTEST_APPS
              PROPERTY STRINGS OFF OpenGL Vulkan OpenGL+Vulkan
)
if(WIN32 OR APPLE)
    if(NOT IOS)
        CMAKE_DEPENDENT_OPTION( KTX_LOADTEST_APPS_USE_LOCAL_DEPENDENCIES
            "Look for locally installed dependencies. Fail build if not found. When OFF use binaries from the KTX-Software repo, if available for the target processor. Useful when building for Windows arm64 using vcpkg packages."
            OFF
            KTX_FEATURE_LOADTEST_APPS
            OFF
        )
    endif()
else()
    set( KTX_LOADTEST_APPS_USE_LOCAL_DEPENDENCIES ON )
endif()

option( KTX_GENERATE_VK_FILES
    "Include targets for generating VkFormat related files. For project developers only."
    OFF
)
mark_as_advanced(FORCE KTX_GENERATE_VK_FILES)

# Platform specific settings

if(APPLE)
    # Signing
    set(XCODE_CODE_SIGN_IDENTITY "Development" CACHE STRING "Xcode code sign ID")
    set(XCODE_DEVELOPMENT_TEAM "" CACHE STRING "Xcode development team ID")
    set(PRODUCTBUILD_IDENTITY_NAME "" CACHE STRING "productbuild identity name")
    set(PRODUCTBUILD_KEYCHAIN_PATH "" CACHE FILEPATH "pkgbuild keychain file")
    if(IOS)
        set(XCODE_PROVISIONING_PROFILE_SPECIFIER "" CACHE STRING "Xcode provisioning profile specifier")
        set(MOLTENVK_SDK "$ENV{VULKAN_SDK}/../MoltenVK" CACHE STRING "MoltenVK directory inside the Vulkan SDK")
    endif()

    # Deployment
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE STRING "macOS Deployment Target")
    if(IOS)
        set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "11.0" CACHE STRING "iOS Deployment Target")
        set(CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH NO)
    endif()
endif()

if(WIN32)
    if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.18.0")
        include(KtxDependentVariable)
        # Signing
        set(CODE_SIGN_KEY_VAULT "" CACHE STRING "Name of the vault to search for signing certificate. Enables related code signing variables when set.")
        set_property(CACHE CODE_SIGN_KEY_VAULT
            PROPERTY STRINGS "" Azure Machine User)

        # Common signing variables
        KTX_DEPENDENT_VARIABLE( CODE_SIGN_TIMESTAMP_URL STRING
          "URL of timestamp server for code signing. Signed code should be timestamped so the signature will remain valid even after the certificate expires."
          ""
          "CODE_SIGN_KEY_VAULT"
          ""
        )
        # Variables for signing with local certificate.
        KTX_DEPENDENT_VARIABLE( LOCAL_KEY_VAULT_SIGNING_IDENTITY STRING
            "Subject Name of Windows code signing certificate. Displayed in 'Issued To' field of cert{lm,mgr}. Overriden by LOCAL_KEY_VAULT_CERTIFICATE_THUMBPRINT."
            ""
            "CODE_SIGN_KEY_VAULT;${CODE_SIGN_KEY_VAULT} MATCHES Machine OR ${CODE_SIGN_KEY_VAULT} MATCHES User"
            ""
        )
        KTX_DEPENDENT_VARIABLE( LOCAL_KEY_VAULT_CERTIFICATE_THUMBPRINT STRING
            "Thumbprint of the certificate to use. Use this instead of LOCAL_KEY_VAULT_SIGNING_IDENTITY when you have multiple certificates with the same identity. Overrides LOCAL_KEY_VAULT_SIGNING_IDENTITY."
            ""
            "CODE_SIGN_KEY_VAULT;${CODE_SIGN_KEY_VAULT} MATCHES Machine OR ${CODE_SIGN_KEY_VAULT} MATCHES User"
            ""
        )

        # Variables for signing with certificate from Azure
        KTX_DEPENDENT_VARIABLE( AZURE_KEY_VAULT_URL STRING
            "The URL of your Azure key vault."
            ""
            "CODE_SIGN_KEY_VAULT;${CODE_SIGN_KEY_VAULT} MATCHES Azure"
            ""
        )
        KTX_DEPENDENT_VARIABLE( AZURE_KEY_VAULT_CERTIFICATE STRING
            "The name of the certificate in Azure Key Vault."
            ""
            "CODE_SIGN_KEY_VAULT;${CODE_SIGN_KEY_VAULT} MATCHES Azure"
            ""
        )
        KTX_DEPENDENT_VARIABLE( AZURE_KEY_VAULT_CLIENT_ID STRING
            "The id of an application (Client) registered with Azure that has permission to access the certificate."
            ""
            "CODE_SIGN_KEY_VAULT;${CODE_SIGN_KEY_VAULT} MATCHES Azure"
            ""
        )
        KTX_DEPENDENT_VARIABLE( AZURE_KEY_VAULT_TENANT_ID STRING
            "The id of the Azure Active Directory (Tenant) holding the Client."
            ""
            "CODE_SIGN_KEY_VAULT;${CODE_SIGN_KEY_VAULT} MATCHES Azure"
            ""
        )
        KTX_DEPENDENT_VARIABLE( AZURE_KEY_VAULT_CLIENT_SECRET STRING
            "The secret to authenticate access to the Client."
            ""
            "CODE_SIGN_KEY_VAULT;${CODE_SIGN_KEY_VAULT} MATCHES Azure"
            ""
        )
    else()
        # KTX_DEPENDENT_VARIABLE won't work. Force disable signing.
        unset(CODE_SIGN_KEY_VAULT CACHE)
    endif()

    # Windows Store Compatibility
    if (${CMAKE_SYSTEM_NAME} STREQUAL "WindowsStore")
        # Disable OpenGL upload on Universal Windows Platform
        set(KTX_FEATURE_GL_UPLOAD OFF)
    endif()
endif()

if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN AND NOT ANDROID)
    set(LINUX TRUE)
endif()

if(EMSCRIPTEN)
    set( KTX_FEATURE_VK_UPLOAD OFF )
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
if(KTX_GENERATE_VK_FILES)
    include(cmake/mkvk.cmake)
endif()

# Global compile & link options including optimization flags

option( KTX_WERROR "Make all warnings in KTX code into errors." OFF)

if(MSVC)
    # ";" argument separator is problematic. Can't use a GenEx `$<IF:`
    # because `/W4;/WX` is returned as a single string.
    add_compile_options( /W4;$<$<BOOL:${KTX_WERROR}>:/WX> )
    add_compile_options( $<IF:$<CONFIG:Debug>,/Gz,/O2> )
    # Enable UTF-8 support
    add_compile_options( $<$<C_COMPILER_ID:MSVC>:/utf-8> )
    add_compile_options( $<$<CXX_COMPILER_ID:MSVC>:/utf-8> )
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU"
       OR ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    add_compile_options( -Wall -Wextra $<$<BOOL:${KTX_WERROR}>:-Werror>)
    add_compile_options( $<IF:$<CONFIG:Debug>,-O0,-O3> )
    if(EMSCRIPTEN)
        add_link_options( $<IF:$<CONFIG:Debug>,-gsource-map,-O3> )
    else()
        add_link_options( $<IF:$<CONFIG:Debug>,-g,-O3> )
    endif()
else()
    message(FATAL_ERROR "${CMAKE_CXX_COMPILER_ID} not yet supported.")
endif()

# To improve output determinism enable precise floating point operations globally
# This code was based on lib/astc-encoder/Source/cmake_core.cmake

# For Visual Studio prior to 2022 (compiler < 19.30) /fp:strict
# For Visual Studio 2022 (compiler >= 19.30) /fp:precise
# For Visual Studio 2022 ClangCL seems to have accidentally enabled contraction by default,
# so behaves differently to CL.exe. Use the -Xclang argument to workaround and allow access
# GNU-style switch to control contraction and force disable.

# On CMake 3.25 or older CXX_COMPILER_FRONTEND_VARIANT is not always set
if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "")
    set(CMAKE_CXX_COMPILER_FRONTEND_VARIANT "${CMAKE_CXX_COMPILER_ID}")
endif()

# Compiler accepts MSVC-style command line options
set(is_msvc_fe "$<STREQUAL:${CMAKE_CXX_COMPILER_FRONTEND_VARIANT},MSVC>")
# Compiler accepts GNU-style command line options
set(is_gnu_fe1 "$<STREQUAL:${CMAKE_CXX_COMPILER_FRONTEND_VARIANT},GNU>")
# Compiler accepts AppleClang-style command line options, which is also GNU-style
set(is_gnu_fe2 "$<STREQUAL:${CMAKE_CXX_COMPILER_FRONTEND_VARIANT},AppleClang>")
# Compiler accepts GNU-style command line options
set(is_gnu_fe "$<OR:${is_gnu_fe1},${is_gnu_fe2}>")

# Compiler is Visual Studio cl.exe
set(is_msvccl "$<AND:${is_msvc_fe},$<CXX_COMPILER_ID:MSVC>>")
# Compiler is Visual Studio clangcl.exe
set(is_clangcl "$<AND:${is_msvc_fe},$<CXX_COMPILER_ID:Clang>>")
# Compiler is upstream clang with the standard frontend
set(is_clang "$<AND:${is_gnu_fe},$<CXX_COMPILER_ID:Clang,AppleClang>>")

if(${is_msvccl} AND $<VERSION_LESS:$<CXX_COMPILER_VERSION>,19.30>)
    add_compile_options(/fp:strict)
endif()
if(${is_msvccl} AND $<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,19.30>)
    add_compile_options(/fp:precise)
endif()
if(${is_clangcl})
    add_compile_options(/fp:precise)
endif()
if(${is_clangcl} AND $<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,14.0.0>)
    add_compile_options(-Xclang -ffp-contract=off)
endif()
if(${is_clang} AND $<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,10.0.0>)
    add_compile_options(-ffp-model=precise)
endif()
if(${is_gnu_fe})
    add_compile_options(-ffp-contract=off)
endif()

set(KTX_BUILD_DIR "${CMAKE_BINARY_DIR}")

set(KTX_MAIN_SRC
    include/KHR/khr_df.h
    include/ktx.h
    lib/basis_sgd.h
    lib/basis_transcode.cpp
    lib/miniz_wrapper.cpp
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

if(KTX_FEATURE_GL_UPLOAD)
    list(APPEND KTX_MAIN_SRC
        lib/gl_funcs.c
        lib/gl_funcs.h
        lib/glloader.c
    )
endif()

if(WIN32)
    # By wrapping in generator expression we force multi configuration
    # generators (like Visual Studio) to take the exact path and not
    # change it.
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${KTX_BUILD_DIR}/$<CONFIG>>)
elseif(APPLE OR LINUX)
    if(NOT IOS)
        # Set a common RUNTIME_OUTPUT_DIR and LIBRARY_OUTPUT_DIR for all targets,
        # so that INSTALL RPATH is functional in build directory as well.
        # BUILD_WITH_INSTALL_RPATH is necessary for working code signing.
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${KTX_BUILD_DIR}/$<CONFIG>>)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $<1:${KTX_BUILD_DIR}/$<CONFIG>>)
    endif()
endif()

# Main library
add_library( ktx ${LIB_TYPE}
    ${KTX_MAIN_SRC}
    )

# Read-only library
add_library( ktx_read ${LIB_TYPE}
    ${KTX_MAIN_SRC}
    )

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
          "${CMAKE_CURRENT_SOURCE_DIR}/include/ktx.h"
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES

    )
    if(IOS)
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

    target_compile_definitions(
        ${target}
    PUBLIC
        "$<$<CONFIG:Debug>:_DEBUG;DEBUG>"
    PRIVATE
        LIBKTX
    )

    # C/C++ Standard
    target_compile_features(${target} PUBLIC c_std_99 cxx_std_11)

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
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/basisu/transcoder>
        $<INSTALL_INTERFACE:lib/basisu/transcoder>

        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/basisu/zstd>
        $<INSTALL_INTERFACE:lib/basisu/zstd>

        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils>
        $<INSTALL_INTERFACE:utils>
    )

    target_include_directories(
        ${target}
        SYSTEM
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/other_include>
        $<INSTALL_INTERFACE:other_include>
    )

    if( ${library_type} STREQUAL STATIC )
        target_compile_definitions(${target} PUBLIC KHRONOS_STATIC)
    endif()

    # To reduce size, don't support transcoding to ancient formats.
    target_compile_definitions(${target} PRIVATE BASISD_SUPPORT_FXT1=0)

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
            ${target}
        PRIVATE
            # Only set dllexport when building a shared library.
            $<$<STREQUAL:${library_type},SHARED>:KTX_API=__declspec\(dllexport\)>
        PUBLIC # only for basisu_c_binding.
            BASISU_NO_ITERATOR_DEBUG_LEVEL
        )
        # The generator automatically sets the needed VCLinker
        # option when a .def file is seen in sources.
        # The def files that we add have a different syntax depending on the ABI
        if(MINGW)
            target_sources(
                ${target}
            PRIVATE
                lib/internalexport_mingw.def
                $<${enable_write}:lib/internalexport_write_mingw.def>
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
                lib/internalexport.def
                $<${enable_write}:lib/internalexport_write.def>
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
    endif()

    if(KTX_FEATURE_KTX1)
        target_compile_definitions(${target} PUBLIC KTX_FEATURE_KTX1)
        target_sources(
            ${target}
        PRIVATE
            lib/texture1.c
            lib/texture1.h
        )
    endif()

    if(KTX_FEATURE_KTX2)
        target_compile_definitions(${target} PUBLIC KTX_FEATURE_KTX2)
    endif()

    if(WIN32)
        if(MINGW)
            # Check if the Threads package is provided; if using Mingw it MIGHT be
            find_package(Threads)
            if(Threads_FOUND AND CMAKE_USE_PTHREADS_INIT)
                target_compile_definitions(${target} PRIVATE WIN32_HAS_PTHREADS)
                target_link_libraries(${target} PRIVATE Threads::Threads)
            endif()
        endif()

    elseif(APPLE)
        if(KTX_EMBED_BITCODE)
            target_compile_options(${target} PRIVATE "-fembed-bitcode")
        endif()

    elseif(LINUX)
        find_package(Threads REQUIRED)
        target_link_libraries(
            ${target}
            PRIVATE
            dl
            Threads::Threads
        )
    endif()

    if(KTX_FEATURE_VK_UPLOAD)
        target_sources(
            ${target}
        PRIVATE
            include/ktxvulkan.h
            lib/vk_funcs.c
            lib/vk_funcs.h
            lib/vkloader.c
        )
        target_include_directories(
            ${target}
        PRIVATE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/dfdutils>
            $<INSTALL_INTERFACE:lib/dfdutils>
        )

        get_target_property( KTX_PUBLIC_HEADER ${target} PUBLIC_HEADER )
        list(APPEND KTX_PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/include/ktxvulkan.h)
        set_target_properties(${target} PROPERTIES
            PUBLIC_HEADER "${KTX_PUBLIC_HEADER}"
        )
    else()
        target_compile_definitions( ${target} PRIVATE KTX_OMIT_VULKAN=1 )
    endif()

    # Adding write capability to target ktx
    if(${enable_write})
        target_sources(
            ${target}
        PRIVATE
            lib/basis_encode.cpp
            lib/astc_encode.cpp
            ${BASISU_ENCODER_C_SRC}
            ${BASISU_ENCODER_CXX_SRC}
            lib/writer1.c
            lib/writer2.c
        )

        target_include_directories(
            ${target}
        PRIVATE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/basisu>
            $<INSTALL_INTERFACE:lib/basisu>
            $<$<BOOL:${BASISU_SUPPORT_OPENCL}>:${OpenCL_INCLUDE_DIRS}>
        )
        target_compile_definitions(
            ${target}
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
            ${target}
        PRIVATE
            $<$<AND:$<BOOL:${BASISU_SUPPORT_SSE}>,$<CXX_COMPILER_ID:AppleClang,Clang,GNU>>:
                -msse4.1
            >
        )
        target_link_libraries(
            ${target}
        PRIVATE
            $<$<BOOL:${BASISU_SUPPORT_OPENCL}>:${OpenCL_LIBRARIES}>
        )
    endif()
endmacro(common_libktx_settings)

common_libktx_settings(ktx 1 ${LIB_TYPE})
common_libktx_settings(ktx_read 0 ${LIB_TYPE})

if(KTX_FEATURE_JNI)
    add_subdirectory(interface/java_binding)
endif()

if(KTX_FEATURE_PY)
    add_subdirectory(interface/python_binding)
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

# Turn off these warnings until Rich fixes the occurences.
# It it not clear to me if generator expressions can be used here
# hence the long-winded way.
#message(STATUS
#        "CMAKE_CXX_COMPILER_ID = ${CMAKE_CXX_COMPILER_ID} "
#        "CMAKE_CXX_COMPILER_VERSION = ${CMAKE_CXX_COMPILER_VERSION}"
#)
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
# Currently no need to disable any warnings in basisu code. Rich fixed them.
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
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
    if (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "11")
        if (${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS "12" )
        # Version 11 raises several stringop-overflow warnings in some
        # very hard to decipher code. They appear to be bogus based on
        # the facts that we have never seen a crash and version 12 no
        # longer raises the warnings.
        get_source_file_property(cur_options
            lib/basisu/encoder/basisu_comp.cpp
            COMPILE_OPTIONS
        )
        set_source_files_properties(
            lib/basisu/encoder/basisu_comp.cpp
            PROPERTIES COMPILE_OPTIONS "${cur_options};-Wno-stringop-overflow"
        )
        endif()
    endif()
    if (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "12.0")
        # Version 12 newly raises this warning on basisu_uastc_enc.cpp.
        # There seems no way for the index calculated by the code at
        # line 326, where the error is raised, to be > the array length.
        # Also we have never seen any crashes.
        set_source_files_properties(
            lib/basisu/encoder/basisu_uastc_enc.cpp
            PROPERTIES COMPILE_OPTIONS "-Wno-stringop-overflow"
        )
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        # Versions equivalency from https://en.wikipedia.org/wiki/Xcode#Xcode_11.x_-_14.x_(since_SwiftUI_framework)_2
        if (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "14")
            set( clang_version ${CMAKE_CXX_COMPILER_VERSION})
        elseif (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "13.1.0")
            set( clang_version "13.0.0")
        elseif (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "13.0.0")
            set( clang_version "12.0.0")
        elseif (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "12.0.5")
            set( clang_version "11.1.0")
        elseif (${CMAKE_CXX_COMPILER_VERSION} VERSION_EQUAL "12.0.0")
            set( clang_version "10.0.0")
        else()
            message(FATAL_ERROR "Unsupported AppleClang version")
        endif()
    else()
        set( clang_version ${CMAKE_CXX_COMPILER_VERSION} )
    endif()
    # BEWARE: set_source_files_properties is not additive; it replaces.
    if (${clang_version} VERSION_GREATER_EQUAL "12.0.0")
        set_source_files_properties( lib/basisu/encoder/basisu_kernels_sse.cpp
            PROPERTIES COMPILE_OPTIONS "-Wno-unused-parameter;-Wno-deprecated-copy;-Wno-uninitialized-const-reference"
        )
    else()
        set_source_files_properties( lib/basisu/encoder/basisu_kernels_sse.cpp
            PROPERTIES COMPILE_OPTIONS "-Wno-unused-parameter"
        )
    endif()
    if (${clang_version} VERSION_GREATER_EQUAL "14.0")
        # These are for Emscripten which is ahead of xcode in its clang
        # version. Also future proofing for when xcode catches up.
        set_source_files_properties(
            ${BASISU_ENCODER_CXX_SRC}
            PROPERTIES COMPILE_OPTIONS "-Wno-sign-compare;-Wno-unused-variable;-Wno-unused-parameter;-Wno-deprecated-copy-with-user-provided-copy"
        )
        set_source_files_properties(
            lib/basisu/transcoder/basisu_transcoder.cpp
            PROPERTIES COMPILE_OPTIONS "-Wno-sign-compare;-Wno-unused-function;-Wno-unused-variable"
        )
        set_source_files_properties(
            lib/basisu/zstd/zstd.c
            PROPERTIES COMPILE_OPTIONS "-Wno-unused-function"
        )
    endif()
else()
    message(FATAL_ERROR "${CMAKE_CXX_COMPILER_ID} not yet supported.")
endif()

# Retrieve the final set of properties for use by the transcodetests.
# We do this because the CMake feature that would allow the transcodetests
# target to retrieve these from the ktx target are not available until
# v18 and we still need to work with v16 in the Emscripten Docker image.
get_source_file_property(transcoder_options
    lib/basisu/transcoder/basisu_transcoder.cpp
    COMPILE_OPTIONS
)
get_source_file_property(zstd_options lib/basisu/zstd/zstd.c COMPILE_OPTIONS)

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
        "SHELL:-s GL_ENABLE_GET_PROC_ADDRESS=1" # For Emscripten 3.1.51+
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
    utils/stdafx.h
    utils/platform_utils.h
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
if(KTX_FEATURE_STATIC_LIBRARY AND
   (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    set_source_files_properties(
        lib/astc_encode.cpp
        PROPERTIES COMPILE_OPTIONS "-fvisibility=hidden"
    )
endif()

add_subdirectory(interface/basisu_c_binding)

# Only one architecture is supported at once, if neither ASTCENC_ISA_SSE41
# nor ASTCENC_ISA_SSE2 are defined, ASTCENC_ISA_AVX2 is chosen. If
# ASTCENC_ISA_AVX2 fails to compile user must chose other x86 options.
# On arm based systems ASTCENC_ISA_NEON is the default. If we detect
# the user is doing a universal build then SIMD is disabled.

list(FIND CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD)" ASTCENC_BUILD_UNIVERSAL)
list(LENGTH CMAKE_OSX_ARCHITECTURES ARCHITECTURE_COUNT)

if(${ASTCENC_BUILD_UNIVERSAL} EQUAL -1 AND ARCHITECTURE_COUNT LESS_EQUAL 1)
    if (${ASTCENC_ISA_NONE})
        set(ASTCENC_LIB_TARGET astcenc-none-static)
    else()
        if(CPU_ARCHITECTURE STREQUAL x86_64 OR CPU_ARCHITECTURE STREQUAL x86)
            if (${ASTCENC_ISA_SSE41})
                set(ASTCENC_LIB_TARGET astcenc-sse4.1-static)
            elseif (${ASTCENC_ISA_SSE2})
                set(ASTCENC_LIB_TARGET astcenc-sse2-static)
            else()
                set(ASTCENC_ISA_AVX2 ON)
                set(ASTCENC_LIB_TARGET astcenc-avx2-static)
            endif()
            if(CPU_ARCHITECTURE STREQUAL x86)
                set(ASTCENC_ISA_NONE ON)
                set(ASTCENC_ISA_AVX2 OFF)
                set(ASTCENC_POPCNT 0)
                set(ASTCENC_LIB_TARGET astcenc-none-static)
            endif()
        elseif(CPU_ARCHITECTURE STREQUAL armv8 OR CPU_ARCHITECTURE STREQUAL arm64)
            set(ASTCENC_LIB_TARGET astcenc-neon-static)
            set(ASTCENC_ISA_NEON ON)
        else()
            message(STATUS "Unsupported ISA for ASTC on ${CPU_ARCHITECTURE} arch, using ASTCENC_ISA_NONE.")
            set(ASTCENC_LIB_TARGET astcenc-none-static)
            set(ASTCENC_ISA_NONE ON)
        endif()
    endif()
else()
    set(ASTCENC_LIB_TARGET astcenc-static)
endif()

# astcenc
set(ASTCENC_CLI OFF) # Only build as library not the CLI astcencoder
add_subdirectory(lib/astc-encoder)
set_property(TARGET ${ASTCENC_LIB_TARGET} PROPERTY POSITION_INDEPENDENT_CODE ON)

if(KTX_FEATURE_STATIC_LIBRARY AND APPLE)
    # Make a single static library to simplify linking.
    add_dependencies(ktx ${ASTCENC_LIB_TARGET})
    add_custom_command( TARGET ktx
        POST_BUILD
        COMMAND libtool -static -o $<TARGET_FILE:ktx> $<TARGET_FILE:ktx> $<TARGET_FILE:${ASTCENC_LIB_TARGET}>
    )

    # Don't know libtool equivalent on Windows or Emscripten. Applications
    # will have to link with  both ktx and ${ASTCENC_LIB_TARGET}. Static libs
    # are unlikely to be used on Windows so not a problem there. For Emscripten
    # everything is built into the JS module so not an issue there either.
else()
    target_link_libraries(ktx PRIVATE ${ASTCENC_LIB_TARGET})
endif()

# Other external projects
# `NOT TARGET`s here are a precaution. They would only be exercised if
# this CMakeLists is included in another project which is unlikely
# except for building the ktx library.
if((KTX_FEATURE_TOOLS OR KTX_FEATURE_TESTS) AND NOT TARGET fmt::fmt)
    set(FMT_SYSTEM_HEADERS ON)
    add_subdirectory(other_projects/fmt)
endif()
if(KTX_FEATURE_TOOLS AND NOT TARGET cxxopts::cxxopts)
    add_subdirectory(other_projects/cxxopts)
endif()

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
if(KTX_FEATURE_STATIC_LIBRARY AND NOT APPLE)
    list(APPEND KTX_INSTALL_TARGETS ${ASTCENC_LIB_TARGET})
endif()

# Install

if(APPLE OR LINUX)
    # Have library's name links as separate component
    set(KTX_NAMELINKS ON)
    install(TARGETS ${KTX_INSTALL_TARGETS}
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

# Use of this to install KHR/khr_df.h is due to CMake's failure to
# preserve the include source folder hierarchy.
# See https://gitlab.kitware.com/cmake/cmake/-/issues/16739.
if (IOS)
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
# This does not use the ${CPU_ARCHITECTURE} set earlier because the hope is
# that the BasisU SIMD code will be changed to no longer need compile time
# definitions and we can remove the complex cputypetest.cmake.
if( APPLE AND CMAKE_OSX_ARCHITECTURES )
    # CMAKE_SYSTEM_PROCESSOR typically set to build host processor here.
    # CMAKE_OSX_ARCHITECTURES governs target of build.
    list(LENGTH CMAKE_OSX_ARCHITECTURES archs_len)
    list(GET CMAKE_OSX_ARCHITECTURES 0 arch0)
    # Cannot use CMAKE_OSX_ARCHITECTURES instead of arch0 due to if() being
    # confused by potentially multiple architectures between OR and STREQUAL.
    # It's okay in the else() clause but as we've already set arch0...
    if( ${archs_len} GREATER 1 OR ${arch0} STREQUAL "$(ARCHS_STANDARD)" )
        set(processor_name "universal")
    else()
        set(processor_name ${arch0})
    endif()
elseif( CMAKE_CXX_COMPILER_ARCHITECTURE_ID )
    # When targeting Windows arm64 CMAKE_SYSTEM_PROCESSOR will incorrectly
    # return AMD64. See: https://gitlab.kitware.com/cmake/cmake/-/issues/15170.
    # We assume that when building for Windows arm64 that we are using MSVC
    # or ClangCL so we can detect the processor arch name with
    # CMAKE_CXX_COMPILER_ARCHITECTURE_ID
    set(processor_name ${CMAKE_CXX_COMPILER_ARCHITECTURE_ID})
elseif( CMAKE_SYSTEM_PROCESSOR )
    if( ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64" )
        # Use consistent package name for all platforms.
        set(processor_name "arm64")
    else()
        set(processor_name ${CMAKE_SYSTEM_PROCESSOR})
    endif()
elseif( IOS )
    # CMAKE_SYSTEM_PROCESSOR not set when building for iOS.
    set(processor_name "arm64")
endif()
string(TOLOWER "${processor_name}" processor_name)
#message(STATUS "processor_name is ${processor_name}, CPU_ARCHITECTURE is ${CPU_ARCHITECTURE}")
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${KTX_VERSION_FULL}-${CMAKE_SYSTEM_NAME}-${processor_name}")

if(APPLE)
    if(NOT IOS)
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
    else()
        set(CPACK_GENERATOR ZIP)
        set(CPACK_ARCHIVE_KTX_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}")
        set(CPACK_ARCHIVE_COMPONENT_INSTALL OFF)
        set(CPACK_PACKAGE_CHECKSUM SHA1)
    endif()
elseif(LINUX)
    set(CPACK_GENERATOR DEB RPM TBZ2)
    set(CPACK_PACKAGE_CHECKSUM SHA1)
    set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ReadMe.txt")

    # This fixes the directory permission derived from Ubuntu ${BUILD_DIR} of 0775 to 0755.
    # otherwise when installing the RPM, it conflicts with the base filesystem.rpm that is
    # setting the directory permissions and owns them to 0755.
    # See GitHub Issue #827.
    # NOTE: Alternatively It would be better build the RPM through a docker or RedHat OS in the CI
    set(CPACK_RPM_DEFAULT_DIR_PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE)
elseif(WIN32)
    set(CPACK_GENERATOR "NSIS")
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
    if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.20")
        set(CPACK_NSIS_BRANDING_TEXT "KTX for Windows")
    endif()
    ## Alternative: add major version at end
    #set(CPACK_PACKAGE_INSTALL_DIRECTORY "KTX-Software-${PROJECT_VERSION_MAJOR}")
    if (CODE_SIGN_KEY_VAULT)
        set_nsis_installer_codesign_cmd()
    else()
        # We're not signing the package so provide a checksum file.
        set(CPACK_PACKAGE_CHECKSUM SHA1)
    endif()
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
#cpack_add_component(Namelinks
#    DEPENDS library
#    HIDDEN
#)
cpack_add_component(jni
    DISPLAY_NAME "Java wrapper"
    DESCRIPTION "Java wrapper and native interface for KTX library."
    DEPENDS library
    DISABLED
)
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

if(EMSCRIPTEN)
    set(CPACK_COMPONENTS_ALL
        ktx_js
        msc_basis_transcoder_js
    )
else()
    set(CPACK_COMPONENTS_ALL
        library
        dev
    )
    if(KTX_FEATURE_TOOLS)
        list(APPEND CPACK_COMPONENTS_ALL
            tools
        )
    endif()
    if(KTX_FEATURE_JNI)
        list(APPEND CPACK_COMPONENTS_ALL
            jni
        )
    endif()
#    if(${KTX_FEATURE_LOADTEST_APPS} MATCHES "OpenGL")
#        list(APPEND CPACK_COMPONENTS_ALL
#            GLLoadTestApps
#        )
#    endif()
#    if(${KTX_FEATURE_LOADTEST_APP} MATCHES "Vulkan")
#        list(APPEND CPACK_COMPONENTS_ALL
#            VkLoadTestApp
#        )
#    endif()
endif()
# if(KTX_NAMELINKS)
#     list(APPEND CPACK_COMPONENTS_ALL
#         Namelinks
#     )
# endif()

include(CPack)

# vim:ai:ts=4:sts=2:sw=2:expandtab

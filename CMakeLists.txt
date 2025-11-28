# Copyright 2015-2023 The Khronos Group Inc.
# Copyright 2022-2023 RasterGrid Kft.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.22)

# N.B. Beware of using cmake_print_variables for function dependent
# variables, e.g. ARGN and CMAKE_CURRENT_FUNCTION_LIST_DIR.
include(CMakePrintHelpers)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules/")

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

# N.B IOS and, in the Darwin case, CMAKE_SYSTEM_NAME are not set
# until after the project() command. The latter is most strange.
if(APPLE)
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CMAKE_SYSTEM_NAME STREQUAL "tvOS" OR CMAKE_SYSTEM_NAME STREQUAL "visionOS")
        set( APPLE_LOCKED_OS ON )
    else()
        set( APPLE_MAC_OS ON )
    endif()
endif()

include(CMakeDependentOption)

include(cmake/codesign.cmake) # Needs APPLE_LOCKED_OS value.
include(cmake/cputypetest.cmake)

# OPTIONS

option( KTX_FEATURE_DOC "Create KTX documentation." OFF )
option( KTX_FEATURE_JNI "Create Java bindings for libktx." OFF )
option( KTX_FEATURE_PY "Create Python source distribution." OFF )
option( KTX_FEATURE_TESTS "Create unit tests." ON )
option( KTX_FEATURE_TOOLS_CTS "Enable KTX CLI Tools CTS tests (requires CTS submodule)." OFF )
option( KTX_FEATURE_JS "Enable JavaScript bindings in Emscripten builds." ON )

if(KTX_FEATURE_TOOLS_CTS AND NOT KTX_FEATURE_TOOLS)
    message(WARNING "KTX_FEATURE_TOOLS is not set -> disabling KTX_FEATURE_TOOLS_CTS.")
    set(KTX_FEATURE_TOOLS_CTS "OFF")
endif()

if(POLICY CMP0127)
    # cmake_dependent_option() supports full Condition Syntax. Introduced in
    # 3.22. Not all build environments have 3.22+. Set policy to avoid warning.
    # Seems the parens in the match string trigger the warning.
    cmake_policy(SET CMP0127 NEW)
endif()

CMAKE_DEPENDENT_OPTION( KTX_EMBED_BITCODE
    "Embed bitcode in binaries."
    OFF
    "APPLE AND APPLE_LOCKED_OS"
    OFF
)

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

if(NOT KTX_FEATURE_LOADTEST_APPS MATCHES OFF)
    set(VCPKG_MANIFEST_FEATURES loadtests)
    if (KTX_FEATURE_LOADTEST_APPS MATCHES OpenGL)
        list(APPEND VCPKG_MANIFEST_FEATURES glloadtests)
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        # Explicitly set the triplet to avoid potential trouble.
        # Automatic triplet selection in CI, which runs on x86_64, selects
        # x64-ios, which is the simulator. Works locally on arm64 so
        # presumably running on an x86_64 is the reason.
        set(VCPKG_TARGET_TRIPLET arm64-ios)
    endif()
endif()

option( KTX_GENERATE_VK_FILES
    "Include targets for generating VkFormat related files. For project developers only."
    OFF
)
mark_as_advanced(FORCE KTX_GENERATE_VK_FILES)

# Platform specific settings

set(bitness 64)

option( KTX_WERROR "Make all warnings in KTX code into errors." OFF)

if(APPLE)
    # Deployment.
    # MUST be set before project() else it is ignored.
    # When changing the target you must also edit the triplet files in
    # vcpkg-triplets to reflect the new target.
    if(APPLE_MAC_OS)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "macOS Deployment Target")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CMAKE_SYSTEM_NAME STREQUAL "tvOS")
        set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "iOS/tvOS Deployment Target")
        set(CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH NO)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "visionOS")
        set(CMAKE_OSX_DEPLOYMENT_TARGET "1.0" CACHE STRING "visionOS Deployment Target")
    endif()
endif()

# After most option settings so settings can be used to affect vcpkg.
project(KTX-Software
    VERSION ${KTX_VERSION}
    DESCRIPTION "Libraries and tools to create and read KTX image texture files."
)

include(GNUInstallDirs) # Must be after project.
include(CTest)          #           "

set_target_processor_type(CPU_ARCHITECTURE) # Must be after project.
if (CPU_ARCHITECTURE STREQUAL x86)
    message(FATAL_ERROR "This project cannot be built for x86 cpu.")
endif()

# Uses IOS, so included after project to avoid modifying it.
if(KTX_GENERATE_VK_FILES)
    include(cmake/mkvk.cmake)
endif()

# EMSCRIPTEN is not set until project so all these must be after.
if(KTX_FEATURE_TESTS AND (APPLE_LOCKED_OS OR ANDROID OR EMSCRIPTEN))
    message(WARNING "Building unit tests for Android, Apple locked OSes or the web is not supported -> disabling KTX_FEATURE_TESTS.")
    set(KTX_FEATURE_TESTS "OFF")
endif()
if(KTX_FEATURE_TOOLS_CTS AND NOT KTX_FEATURE_TESTS)
    message(WARNING "KTX_FEATURE_TESTS is not set -> disabling KTX_FEATURE_TOOLS_CTS.")
    set(KTX_FEATURE_TOOLS_CTS "OFF")
endif()
if(APPLE_LOCKED_OS OR EMSCRIPTEN)
    set( LIB_TYPE_DEFAULT OFF )
else()
    set( LIB_TYPE_DEFAULT ON )
endif()

option(BUILD_SHARED_LIBS "Create shared libraries (static otherwise)." ${LIB_TYPE_DEFAULT} )

if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN AND NOT ANDROID)
    set(LINUX TRUE)
endif()

if(EMSCRIPTEN)
    set( KTX_FEATURE_VK_UPLOAD OFF )
endif()

if(NOT BUILD_SHARED_LIBS)
    set(LIB_TYPE STATIC)
else()
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS" OR EMSCRIPTEN)
        message(SEND_ERROR "Library type cannot be shared for the current platform. Set BUILD_SHARED_LIBS to OFF!")
    endif()
    set(LIB_TYPE SHARED)
endif()

# Global compile & link options including optimization flags
if(MSVC)
    add_compile_options( /W4;$<$<BOOL:${KTX_WERROR}>:/WX> )
    add_compile_options( $<IF:$<CONFIG:Debug>,/Gz,/O2> )
    # Enable UTF-8 support
    add_compile_options( $<$<C_COMPILER_ID:MSVC>:/utf-8> )
    add_compile_options( $<$<CXX_COMPILER_ID:MSVC>:/utf-8> )
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU"
       OR ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    add_compile_options( -Wall -Wextra $<$<BOOL:${KTX_WERROR}>:-Werror>)
    add_compile_options( $<IF:$<CONFIG:Debug>,-O0$<SEMICOLON>-g,-O3> )
    if(EMSCRIPTEN)
        add_link_options( $<IF:$<CONFIG:Debug>,-gsource-map,-O3> )
    else()
        add_link_options( $<IF:$<CONFIG:Debug>,-g,-O3> )
    endif()
else()
    message(FATAL_ERROR "${CMAKE_CXX_COMPILER_ID} not yet supported.")
endif()

# To improve output determinism, enable precise floating point operations globally
include(cmake/fp-settings.cmake)
get_fp_compile_options(fp_options)
add_compile_options( ${fp_options} )

# Set output directories
set(KTX_BUILD_DIR "${CMAKE_BINARY_DIR}")

if(APPLE OR LINUX OR WIN32)
    # Three reasons for setting CMAKE_RUNTIME_OUTPUT_DIRECTORY.
    # - Normalize the output directory so it is the same regardless of
    #   single or multi-config generator. Note that wrapping this in a
    #   generator expression forces multi-configuration generators (like
    #   Visual Studio, Xcode or Ninja multi-config) to take the exact path
    #   and not add  a further $<CONFIG> to it;
    # - Build all executables into a common output directory.
    # - On Windows, build dlls into the same directory as the executables,
    #   so they can find their dlls when run in the build directory.
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${KTX_BUILD_DIR}/$<CONFIG>>)

    # Reason for setting CMAKE_LIBRARY_OUTPUT_DIRECTORY.
    # - On GNU/Linux and macOS build .{so,dylib}s into the same directory
    #   as the executables so that the INSTALL RPATH we need to add to
    #   installed binaries is functional in the build directory as well.
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $<1:${KTX_BUILD_DIR}/$<CONFIG>>)

    # For GNU/Linux and macOS BUILD_WITH_INSTALL_RPATH is necessary to have
    # the install rpath set duing build. See below for more explanation.
    set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)

    # Notes about install names, rpaths and code signing
    # --------------------------------------------------
    # Changing the install name of a library by setting INSTALL_NAME_DIR
    # on a library target will invalidate the signature as it causes
    # cmake to run `install_name_tool` after building and signing. The
    # default install name is the library target's name prefixed by `@rpath/`.
    # With this, `dyld` will load the library provided it is in one of the
    # directories specified by a program's INSTALL_RPATH or is in the
    # directory part of a full-path name passed to `dlopen`. Because it would
    # also invalidate the signature INSTALL_RPATH cannot be altered
    # during install. Hence BUILD_WITH_INSTALL_RPATH.
    #
    # On macOS, a bad signature in either executable or shared library is
    # indicated by the executable exiting with "Killed: 9".
    #
    # Although GNU/Linux code is not currently being signed, handle its
    # INSTALL_RPATH the same way for symmetry.
endif()

# Ensure the following are set as KTX-Software needs and hide them from users.
set( LIBKTX_WERROR ${KTX_WERROR} )
set( LIBKTX_EMBED_BITCODE ${KTX_EMBED_BITCODE} )
set( LIBKTX_VERSION_FULL ON )
# Want to expose this one to users so must be a cache variable. Note
# that this is a different default from the one set in the libktx project.
set( LIBKTX_VERSION_READ_ONLY ON CACHE BOOL "Build the read-only library")
add_subdirectory(lib)

if(KTX_FEATURE_JNI)
    add_subdirectory(interface/java_binding)
endif()

if(KTX_FEATURE_PY)
    add_subdirectory(interface/python_binding)
endif()

create_version_file()

if(EMSCRIPTEN AND KTX_FEATURE_JS)
    set(
        KTX_EM_COMMON_LINK_FLAGS
        --bind
        "SHELL:-s MODULARIZE=1"
        "SHELL:-s EXPORTED_RUNTIME_METHODS=[\'GL,HEAP8\']"
        "SHELL:-s GL_PREINITIALIZED_CONTEXT=1"
    )

    set(
        KTX_EM_COMMON_KTX_LINK_FLAGS
        --pre-js ${CMAKE_CURRENT_SOURCE_DIR}/interface/js_binding/class_compat.js
        --extern-post-js ${CMAKE_CURRENT_SOURCE_DIR}/interface/js_binding/module_create_compat.js
        ${KTX_EM_COMMON_LINK_FLAGS}
    )

    set(
        KTX_JS_COMMON_SOURCE
        interface/js_binding/ktx_wrapper.cpp
        interface/js_binding/class_compat.js
        interface/js_binding/module_create_compat.js
    )

    add_executable( ktx_js
        ${KTX_JS_COMMON_SOURCE}
        interface/js_binding/vk_format.inl
    )
    target_compile_definitions(ktx_js PUBLIC KTX_FEATURE_WRITE)
    target_link_libraries( ktx_js ktx )
    target_include_directories(
        ktx_js
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/other_include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/src
        $<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES>
    )
    target_link_options(
        ktx_js
    PUBLIC
        ${KTX_EM_COMMON_KTX_LINK_FLAGS}
        "SHELL:-s EXPORT_NAME=createKtxModule"
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

    add_executable( ktx_js_read
        ${KTX_JS_COMMON_SOURCE}
    )
    target_link_libraries( ktx_js_read ktx_read )
    target_include_directories(
        ktx_js_read
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/other_include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/src
        $<TARGET_PROPERTY:ktx_read,INTERFACE_INCLUDE_DIRECTORIES>
    )
    target_link_options(
        ktx_js_read
    PUBLIC
        ${KTX_EM_COMMON_KTX_LINK_FLAGS}
        "SHELL:-s EXPORT_NAME=createKtxReadModule"
    )
    set_target_properties( ktx_js_read PROPERTIES OUTPUT_NAME "libktx_read")

    add_custom_command(
        TARGET ktx_js_read
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:ktx_js_read>/$<TARGET_FILE_PREFIX:ktx_js_read>$<TARGET_FILE_BASE_NAME:ktx_js_read>.js" "${PROJECT_SOURCE_DIR}/tests/webgl"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:ktx_js_read>/$<TARGET_FILE_PREFIX:ktx_js_read>$<TARGET_FILE_BASE_NAME:ktx_js_read>.wasm" "${PROJECT_SOURCE_DIR}/tests/webgl"
        COMMENT "Copy libktx_read.js and libktx_read.wasm to tests/webgl"
    )

    install(TARGETS ktx_js_read
        RUNTIME
            DESTINATION .
            COMPONENT ktx_js_read
    )
    install(FILES ${CMAKE_BINARY_DIR}/libktx_read.wasm
        DESTINATION .
        COMPONENT ktx_js_read
    )

    add_executable( msc_basis_transcoder_js interface/js_binding/transcoder_wrapper.cpp )
    target_link_libraries( msc_basis_transcoder_js ktx_read )
    target_include_directories( msc_basis_transcoder_js
        PRIVATE
        lib
        external
    )

    # Re-use ktx's compile options
    target_compile_options(msc_basis_transcoder_js
    PRIVATE
       $<TARGET_PROPERTY:ktx_read,INTERFACE_COMPILE_OPTIONS>
       )

    target_link_options(
        msc_basis_transcoder_js
    PUBLIC
        ${KTX_EM_COMMON_LINK_FLAGS}
        "SHELL:-s EXPORT_NAME=MSC_TRANSCODER"
        # Re-use ktx's link options
        $<TARGET_PROPERTY:ktx_read,INTERFACE_LINK_OPTIONS>
    )
    set_target_properties( msc_basis_transcoder_js PROPERTIES OUTPUT_NAME "msc_basis_transcoder")

    target_link_libraries(msc_basis_transcoder_js basisu_encoder)

    add_custom_command(
        TARGET msc_basis_transcoder_js
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
target_compile_features(
    objUtil
PUBLIC
    cxx_std_11
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
if(NOT BUILD_SHARED_LIBS AND
   (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    set_source_files_properties(
        lib/astc_encode.cpp
        PROPERTIES COMPILE_OPTIONS "-fvisibility=hidden"
    )
endif()

add_subdirectory(interface/basisu_c_binding)

# Other external projects
# `NOT TARGET`s here are a precaution. They would only be exercised if
# this CMakeLists is included in another project which is unlikely
# except for building the ktx library.
if((KTX_FEATURE_TOOLS OR KTX_FEATURE_TESTS) AND NOT TARGET fmt::fmt)
    # Always build fmt static
    set(BUILD_SHARED_LIBS OFF)
    set(FMT_INSTALL OFF)
    set(FMT_SYSTEM_HEADERS ON)
    add_subdirectory(external/fmt)
    set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_RESET})
endif()
if(KTX_FEATURE_TOOLS AND NOT TARGET cxxopts::cxxopts)
    add_subdirectory(external/cxxopts)
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
elseif( APPLE_LOCKED_OS )
    # CMAKE_SYSTEM_PROCESSOR not set when building for iOS/tvOS/visionOS.
    set(processor_name "arm64")
endif()
string(TOLOWER "${processor_name}" processor_name)
#message(STATUS "processor_name is ${processor_name}, CPU_ARCHITECTURE is ${CPU_ARCHITECTURE}")
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${KTX_VERSION_FULL}-${CMAKE_SYSTEM_NAME}-${processor_name}")

if(APPLE)
    if(APPLE_MAC_OS)
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
    set(CPACK_ARCHIVE_KTX_JS_READ_FILE_NAME "${CMAKE_PROJECT_NAME}-${KTX_VERSION_FULL}-Web-libktx_read")
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
        ktx_js_read
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

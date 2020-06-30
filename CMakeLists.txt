cmake_minimum_required(VERSION 3.13)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules/")

include(cmake/version.cmake)

project(KTX-Software
    VERSION ${KTX_VERSION}
    DESCRIPTION "Libraries and tools to create and read KTX image texture files."
)

include(CTest)

include(GNUInstallDirs)
include(CMakeDependentOption)

include(cmake/mkvk.cmake)
include(cmake/xcode.cmake)

find_package(Bash REQUIRED)

# OPTIONS

CMAKE_DEPENDENT_OPTION(KTX_FEATURE_TOOLS "Create KTX tools" ON "NOT IOS;NOT ANDROID;NOT EMSCRIPTEN" OFF)
option( KTX_FEATURE_DOC "Create KTX documentation" OFF )
option( KTX_FEATURE_LOADTEST_APPS "Create load tests apps that load and display various KTX textures" OFF )


set(VULKAN_INSTALL_DIR "" CACHE PATH "Path to installation of Vulkan SDK (obtainable from https://vulkan.lunarg.com/sdk/home )")

if(APPLE)
    set(XCODE_CODE_SIGN_IDENTITY "Development" CACHE STRING "Xcode code sign ID")
    set(XCODE_DEVELOPMENT_TEAM "" CACHE STRING "Xcode development team ID")
    set(PRODUCTBUILD_IDENTITY_NAME "" CACHE STRING "productbuild identity name")
    set(PRODUCTBUILD_KEYCHAIN_PATH "" CACHE FILEPATH "pkgbuild keychain file")
endif()

# Might become options in the future

# option( KTX_FEATURE_KTX1 "KTX version 1" ON )
# option( KTX_FEATURE_KTX2 "KTX version 2" ON )
# option( KTX_FEATURE_WRITE "Create KTX files" ON )

set( KTX_FEATURE_WRITE ON )
set( KTX_FEATURE_KTX1 ON )
set( KTX_FEATURE_KTX2 ON )
set( KTX_FEATURE_VULKAN ON )

# Platform specific settings

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE STRING "macOS Deployment Target")
    if(IOS)
        set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "10.0" CACHE STRING "iOS Deployment Target")
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

if(EMSCRIPTEN OR IOS)
    set(LIB_TYPE STATIC)
else()
    set(LIB_TYPE SHARED)
endif()

# Main library
add_library( ktx ${LIB_TYPE}
    include/ktx.h
    lib/basis_sgd.h
    lib/basis_transcode.cpp
    lib/basisu/transcoder/basisu_file_headers.h
    lib/basisu/transcoder/basisu_global_selector_cb.h
    lib/basisu/transcoder/basisu_global_selector_palette.h
    lib/basisu/transcoder/basisu_transcoder_internal.h
    lib/basisu/transcoder/basisu_transcoder_uastc.h
    lib/basisu/transcoder/basisu_transcoder.cpp
    lib/basisu/transcoder/basisu_transcoder.h
    lib/basisu/transcoder/basisu.h
    lib/checkheader.c
    lib/dfdutils/createdfd.c
    lib/dfdutils/dfd.h
    lib/dfdutils/dfd2vk.inl
    lib/dfdutils/interpretdfd.c
    lib/dfdutils/printdfd.c
    lib/dfdutils/queries.c
    lib/dfdutils/vk2dfd.c
    lib/dfdutils/vulkan/vk_platform.h
    lib/dfdutils/vulkan/vulkan_core.h
    lib/etcdec.cxx
    lib/etcunpack.cxx
    lib/filestream.c
    lib/filestream.h
    lib/formatsize.h
    lib/gl_format.h
    lib/gl_funcs.c
    lib/gl_funcs.h
    lib/glloader.c
    lib/hashlist.c
    lib/info.c
    lib/ktxint.h
    lib/memstream.c
    lib/memstream.h
    lib/stream.h
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

set_target_properties(ktx PROPERTIES
    PUBLIC_HEADER ${CMAKE_SOURCE_DIR}/include/ktx.h
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
)
set_xcode_code_sign(ktx)

## Creating vulkan headers is only required when Vulkan SDK updates.
# add_dependencies(ktx mkvk)

create_version_header(lib ktx)

target_compile_definitions(ktx PRIVATE LIBKTX)

# C/C++ Standard
target_compile_features(ktx PUBLIC c_std_99 cxx_std_11)

# Compiler Warning Flags
if(EMSCRIPTEN)
    target_compile_options(ktx PRIVATE
        -Wno-nested-anon-types
        -Wno-gnu-anonymous-struct
    )
else()
    target_compile_options(ktx PRIVATE
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

# Optimization Flags
if(EMSCRIPTEN)
    target_compile_options(ktx PUBLIC
        $<IF:$<CONFIG:Debug>,-O0,-Oz>
    )
    target_link_options( ktx PUBLIC
        $<IF:$<CONFIG:Debug>,-g4,-Oz>
    )
if(MSVC)
    target_compile_options(ktx PUBLIC
        $<IF:$<CONFIG:Debug>,/Gz,/O2>
    )
else()
    target_compile_options(ktx PUBLIC
        $<IF:$<CONFIG:Debug>,-O0,-O3>
    )
    target_link_options( ktx PUBLIC
        $<IF:$<CONFIG:Debug>,-g,-O3>
    )
endif()


else()
    # set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -O0 -g")
    # set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -O0 -g")

    # set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O3 -flto")
    # set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -flto")
endif()

target_include_directories(
    ktx
PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/basisu/transcoder>
    $<INSTALL_INTERFACE:lib/basisu/transcoder>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/other_include>
    $<INSTALL_INTERFACE:other_include>
)

if( LIB_TYPE STREQUAL STATIC )
    target_compile_definitions(ktx PUBLIC KHRONOS_STATIC)
endif()


target_compile_definitions(ktx PUBLIC "$<$<CONFIG:Debug>:_DEBUG;DEBUG>")

# To reduce size, don't support transcoding to ancient formats.
target_compile_definitions(ktx PUBLIC BASISD_SUPPORT_FXT1=0)

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
        ktx
    PUBLIC
        "KTX_APICALL=__declspec(dllexport)"
        BASISU_NO_ITERATOR_DEBUG_LEVEL
    )
    target_sources(
        ktx
    PRIVATE
        # The msvs generator automatically sets the needed VCLinker
        # option when a .def file is seen in sources.
        lib/internalexport.def
    )
endif()

if(KTX_FEATURE_KTX1)
    target_compile_definitions(ktx PUBLIC KTX_FEATURE_KTX1)
    target_sources(
        ktx
    PRIVATE
        lib/texture1.c
        lib/texture1.h
    )
endif()

if(KTX_FEATURE_KTX2)
    target_compile_definitions(ktx PUBLIC KTX_FEATURE_KTX2)
endif()

if(KTX_FEATURE_WRITE)

    target_sources(
        ktx
    PRIVATE
        lib/basis_encode.cpp
        lib/basisu/apg_bmp.c
        lib/basisu/apg_bmp.h
        lib/basisu/basisu_astc_decomp.cpp
        lib/basisu/basisu_astc_decomp.h
        lib/basisu/basisu_backend.cpp
        lib/basisu/basisu_backend.h
        lib/basisu/basisu_basis_file.cpp
        lib/basisu/basisu_basis_file.h
        lib/basisu/basisu_bc7enc.cpp
        lib/basisu/basisu_bc7enc.h
        lib/basisu/basisu_comp.cpp
        lib/basisu/basisu_comp.h
        lib/basisu/basisu_enc.cpp
        lib/basisu/basisu_enc.h
        lib/basisu/basisu_etc.cpp
        lib/basisu/basisu_etc.h
        lib/basisu/basisu_frontend.cpp
        lib/basisu/basisu_frontend.h
        lib/basisu/basisu_global_selector_palette_helpers.cpp
        lib/basisu/basisu_global_selector_palette_helpers.h
        lib/basisu/basisu_gpu_texture.cpp
        lib/basisu/basisu_gpu_texture.h
        lib/basisu/basisu_miniz.h
        lib/basisu/basisu_pvrtc1_4.cpp
        lib/basisu/basisu_pvrtc1_4.h
        lib/basisu/basisu_resample_filters.cpp
        lib/basisu/basisu_resampler_filters.h
        lib/basisu/basisu_resampler.cpp
        lib/basisu/basisu_resampler.h
        lib/basisu/basisu_ssim.cpp
        lib/basisu/basisu_ssim.h
        lib/basisu/basisu_uastc_enc.cpp
        lib/basisu/basisu_uastc_enc.h
        lib/basisu/jpgd.cpp
        lib/basisu/jpgd.h
        lib/basisu/lodepng.cpp
        lib/basisu/lodepng.h
        lib/writer1.c
        lib/writer2.c
    )
    target_include_directories(
        ktx
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib>
        $<INSTALL_INTERFACE:lib>

        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/basisu>
        $<INSTALL_INTERFACE:lib/basisu>
    )

endif()

set(KTX_BUILD_DIR "${CMAKE_BINARY_DIR}")

if(WIN32)
    target_link_libraries(
        ktx
        ${CMAKE_CURRENT_SOURCE_DIR}/other_lib/win/Release-x64/zstd_static.lib
    )
    # By wrapping in generator expression we force multi configuration generators (like Visual Studio)
    # to take the exact path and not change it.
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${KTX_BUILD_DIR}/$<CONFIG>>)

elseif(APPLE)
    if(IOS)
        target_link_libraries(
            ktx
            ${CMAKE_CURRENT_SOURCE_DIR}/other_lib/ios/Release-iphoneos/libzstd.a
            )
    else()
        # Set a common RUNTIME_OUTPUT_DIR for all target, so that INSTALL RPATH
        # is functional in build directory as well. BUILD_WITH_INSTALL_RPATH is necessary
        # for working code signing.
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${KTX_BUILD_DIR}/$<CONFIG>>)
        target_link_libraries(
            ktx
            ${CMAKE_CURRENT_SOURCE_DIR}/other_lib/mac/Release/libzstd.a
            )
    endif()
elseif(EMSCRIPTEN)
    target_sources(
        ktx
    PRIVATE
        lib/zstddeclib.c
    )
elseif(LINUX)
    find_package(zstd REQUIRED)
    find_package(Threads REQUIRED)
    target_link_libraries(
        ktx
        dl
        Threads::Threads
        zstd::zstd
    )
elseif(ANDROID)
    # add_library(zstd STATIC IMPORTED)
    # set_target_properties(zstd PROPERTIES
    # IMPORTED_LOCATION /Users/Shared/SDK/zstd/build/cmake/install-android/lib/libzstd.a
    # )
    # target_link_libraries(
    #     ktx
    #     zstd
    # )
    set(ZSTD_BUILD_PROGRAMS OFF)
    set(ZSTD_BUILD_TESTS OFF)
    add_subdirectory(/Users/Shared/SDK/zstd/build/cmake zstd)
    target_link_libraries(
        ktx
        libzstd_static
    )
endif()

if(KTX_FEATURE_VULKAN)
    target_sources(
        ktx
    PRIVATE
        include/ktxvulkan.h
        lib/vk_funcs.c
        lib/vk_funcs.h
        lib/vkloader.c
    )
    target_include_directories(
        ktx
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/dfdutils>
        $<INSTALL_INTERFACE:lib/dfdutils>
    )

    get_target_property( KTX_PUBLIC_HEADER ktx PUBLIC_HEADER )
    list(APPEND KTX_PUBLIC_HEADER ${CMAKE_SOURCE_DIR}/include/ktxvulkan.h)
    set_target_properties(ktx PROPERTIES
        PUBLIC_HEADER "${KTX_PUBLIC_HEADER}"
    )
else()
    target_compile_definitions( ktx PRIVATE KTX_OMIT_VULKAN=1 )
endif()

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
    target_link_libraries( ktx_js ktx )
    target_include_directories( ktx_js PRIVATE $<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES> )
    target_link_options(
        ktx_js
    PUBLIC
        ${KTX_EMC_LINK_FLAGS}
        "SHELL:-s EXPORT_NAME=LIBKTX"
    )
    set_target_properties( ktx_js PROPERTIES OUTPUT_NAME "libktx")
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
    target_link_libraries( msc_basis_transcoder_js ktx )
    target_include_directories( msc_basis_transcoder_js
        PRIVATE
        lib
        lib/basisu/transcoder
    )

    # Re-use ktx's compile options
    target_compile_options(msc_basis_transcoder_js PRIVATE $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_OPTIONS>)

    target_link_options(
        msc_basis_transcoder_js
    PUBLIC
        ${KTX_EMC_LINK_FLAGS}
        "SHELL:-s EXPORT_NAME=MSC_TRANSCODER"
        # Re-use ktx's link options
        $<TARGET_PROPERTY:ktx,INTERFACE_LINK_OPTIONS>
        )
    set_target_properties( msc_basis_transcoder_js PROPERTIES OUTPUT_NAME "msc_basis_transcoder")

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

add_library( objUtil OBJECT
    utils/argparser.cpp
    utils/argparser.h
    utils/ktxapp.h
    utils/scapp.h
    utils/unused.h
    )
target_include_directories(
    objUtil
PUBLIC
    utils
)

add_subdirectory(interface/basisu_c_binding)

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


# Install

if(APPLE OR LINUX)
    # Have library's name links as separate component
    set(KTX_NAMELINKS ON)
    install(TARGETS ktx
        EXPORT KTXTargets
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT library
            NAMELINK_SKIP
        PUBLIC_HEADER
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            COMPONENT dev
    )
    install(TARGETS ktx
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT library
            NAMELINK_ONLY
    )
else()
    # No name links on Windows
    set(KTX_NAMELINKS OFF)
    install(TARGETS ktx
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
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${KTX_VERSION_FULL}-${CMAKE_SYSTEM_NAME}")

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

elseif(LINUX)
    set(CPACK_GENERATOR DEB RPM TBZ2)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libzstd1 (>= 1.3.1)")
    set(CPACK_PACKAGE_CHECKSUM SHA1)
    set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ReadMe.txt")
elseif(WIN32)
    # Use the Icon file for the installer.exe
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/win/ktx_app.ico")
    set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)
    set(CPACK_NSIS_URL_INFO_ABOUT ${CPACK_PACKAGE_HOMEPAGE_URL})
    set(CPACK_NSIS_CONTACT ${CPACK_PACKAGE_CONTACT})
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "KTX-Software")
    ## Alternative: add major version at end
    #set(CPACK_PACKAGE_INSTALL_DIRECTORY "KTX-Software-${PROJECT_VERSION_MAJOR}")
    set(CPACK_PACKAGE_CHECKSUM SHA1)
    # Custom Package name to make sure it has the "-win64" suffix.
    set(CPACK_VERSION_FULL ${KTX_VERSION_FULL})
    set(CPACK_PROJECT_CONFIG_FILE ${CMAKE_SOURCE_DIR}/cmake/package.txt)
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
    DESCRIPTION "Main KTX library"
    REQUIRED
)
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
    DESCRIPTION "Additional resources for development (header files and documentation)"
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
        # Headers
    )
endif()
# if(KTX_NAMELINKS)
#     list(APPEND CPACK_COMPONENTS_ALL
#         Namelinks
#     )
# endif()

include(CPack)

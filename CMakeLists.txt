cmake_minimum_required(VERSION 3.13)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules/")

project(KTX-Software)

include(CTest)

include(GNUInstallDirs)
include(CPackComponent)

include(cmake/version.cmake)
include(cmake/mkvk.cmake)

find_package(Bash REQUIRED)

# OPTIONS

option( KTX_FEATURE_DOC "Create KTX documentation" OFF )
option( KTX_FEATURE_LOADTEST_APPS "Create load tests apps that load and display various KTX textures" OFF )

# Might become options in the future

# option( KTX_FEATURE_KTX1 "KTX version 1" ON )
# option( KTX_FEATURE_KTX2 "KTX version 2" ON )
# option( KTX_FEATURE_WRITE "Create KTX files" ON )

set( KTX_FEATURE_WRITE ON )
set( KTX_FEATURE_KTX1 ON )
set( KTX_FEATURE_KTX2 ON )
set( KTX_FEATURE_VULKAN ON )
set( KTX_FEATURE_TOOLS ON )


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
    
if(IOS OR ANDROID OR EMSCRIPTEN)
    # Tools are not supported on mobile platforms
    set( KTX_FEATURE_TOOLS OFF )
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

if(LINUX)
    set_target_properties(ktx PROPERTIES
        PUBLIC_HEADER include/ktx.h
        # TODO: fill in correct version here
        VERSION 3.0.0
        SOVERSION 3
    )
endif()

add_dependencies(ktx mkvk)
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
    include
    lib/basisu/transcoder
    other_include
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
        lib
        lib/basisu
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
        lib/dfdutils
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

if(KTX_FEATURE_TOOLS)
    add_subdirectory(tools)
endif()

add_subdirectory(tests)

if(KTX_FEATURE_DOC)
    include(cmake/docs.cmake)
endif()

if(LINUX)
    install(TARGETS ktx
        LIBRARY
            DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            COMPONENT Libraries
            NAMELINK_SKIP
        PUBLIC_HEADER
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            COMPONENT Headers
    )
    install(TARGETS ktx
        LIBRARY
            DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            COMPONENT Namelinks
            NAMELINK_ONLY
    )
else()
    install(TARGETS ktx)
endif()

# Packing

if(APPLE)
    set(CPACK_GENERATOR productbuild)
elseif(LINUX)
    set(CPACK_GENERATOR DEB)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libzstd1 (>= 1.3.1)")

    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "KTX-Software")
    set(CPACK_PACKAGE_VENDOR "Khronos Group")
    set(CPACK_PACKAGE_CONTACT "khronos@callow.im" )
    
    # set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/ReadMe.txt")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")
    
    # TODO: fill in correct version here
    set(CPACK_PACKAGE_VERSION_MAJOR "3")
    set(CPACK_PACKAGE_VERSION_MINOR "0")
    set(CPACK_PACKAGE_VERSION_PATCH "0")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")
    
    
    cpack_add_component(Libraries)
    cpack_add_component(Namelinks GROUP Development DEPENDS Libraries)
    cpack_add_component(Headers GROUP Development DEPENDS Libraries)
    
    cpack_add_component_group(Development)
    
    include(CPack)
endif()

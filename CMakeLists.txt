cmake_minimum_required(VERSION 3.12)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules/")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules/")

project(KTX-Software)

include(CTest)

find_package(OpenGL)
if(NOT EMSCRIPTEN)
    find_package(Vulkan)
endif()

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE STRING "macOS Deployment Target")
    if(IOS)
        set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "10.0" CACHE STRING "iOS Deployment Target")
    endif()
endif()

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()


# OPTIONS

option( KTX_FEATURE_DOC "Create KTX documentation" OFF )

# Might become options in the future

# option( KTX_FEATURE_KTX1 "KTX version 1" ON )
# option( KTX_FEATURE_KTX2 "KTX version 2" ON )
# option( KTX_FEATURE_WRITE "Create KTX files" ON )

set( KTX_FEATURE_WRITE ON )
set( KTX_FEATURE_KTX1 ON )
set( KTX_FEATURE_KTX2 ON )
set( KTX_FEATURE_VULKAN ON )

if(EMSCRIPTEN)
    set( KTX_FEATURE_VULKAN OFF )
    set( KTX_FEATURE_VULKAN OFF )
endif()

set( KTX_GL_CONTEXT_PROFILE "CORE" CACHE STRING "Open GL context profile" )
set_property(CACHE KTX_GL_CONTEXT_PROFILE PROPERTY STRINGS
    CORE
    COMPATIBILITY
    ES
)

set( KTX_GL_CONTEXT_MAJOR_VERSION 3 CACHE STRING "OpenGL major version" )
set( KTX_GL_CONTEXT_MINOR_VERSION 3 CACHE STRING "OpenGL minor version" )

set(bitness 64)
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8 OR FORCE32)
	set(bitness 32)
endif()

# Begin auto-generated code

list(APPEND mkvkformatfiles_input
    "lib/dfdutils/vulkan/vulkan_core.h"
    "lib/mkvkformatfiles")
list(APPEND mkvkformatfiles_output
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/vkformat_enum.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/vkformat_check.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/vkformat_str.c")

if(WIN32)
    add_custom_command(OUTPUT ${mkvkformatfiles_output}
        COMMAND ${CMAKE_COMMAND} -E make_directory lib
        COMMAND call call "setup_env.bat" && set VULKAN_SDK=${VULKAN_SDK} && bash -c "lib/mkvkformatfiles lib"
        DEPENDS ${mkvkformatfiles_input}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT Generating VkFormat-related source files
        VERBATIM
    )
else()
    add_custom_command(OUTPUT ${mkvkformatfiles_output}
        COMMAND ${CMAKE_COMMAND} -E make_directory lib
        COMMAND VULKAN_SDK=${VULKAN_SDK} lib/mkvkformatfiles lib
        DEPENDS ${mkvkformatfiles_input}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT Generating VkFormat-related source files
        VERBATIM
    )
endif()

add_custom_target(mkvkformatfiles
    DEPENDS ${mkvkformatfiles_output}
    SOURCES ${mkvkformatfiles_input}
)


list(APPEND makevkswitch_input
    "lib/vkformat_enum.h"
    "lib/dfdutils/makevkswitch.pl")
set(makevkswitch_output
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/dfdutils/vk2dfd.inl")
add_custom_command(
    OUTPUT ${makevkswitch_output}
    COMMAND ${CMAKE_COMMAND} -E make_directory lib/dfdutils
    COMMAND perl lib/dfdutils/makevkswitch.pl lib/vkformat_enum.h lib/dfdutils/vk2dfd.inl
    DEPENDS ${makevkswitch_input}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMENT Generating VkFormat/DFD switch body
    VERBATIM
)
add_custom_target(makevkswitch
    DEPENDS ${makevkswitch_output}
    SOURCES ${makevkswitch_input}
)


list(APPEND makedfd2vk_input
    "lib/vkformat_enum.h"
    "lib/dfdutils/makedfd2vk.pl")
list(APPEND makedfd2vk_output
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/dfdutils/dfd2vk.inl")

add_custom_command(
    OUTPUT ${makedfd2vk_output}
    COMMAND ${CMAKE_COMMAND} -E make_directory lib/dfdutils
    COMMAND lib/dfdutils/makedfd2vk.pl lib/vkformat_enum.h lib/dfdutils/dfd2vk.inl
    DEPENDS ${makedfd2vk_input}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT Generating DFD/VkFormat switch body
    VERBATIM
)

add_custom_target(makedfd2vk
    DEPENDS ${makedfd2vk_output}
    SOURCES ${makedfd2vk_input}
)

add_custom_target(mkvk SOURCES)

set_target_properties(mkvk PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(mkvkformatfiles PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(makevkswitch PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(makedfd2vk PROPERTIES EXCLUDE_FROM_ALL "FALSE")

add_dependencies(mkvk
    mkvkformatfiles
    makevkswitch
    makedfd2vk
)

# version.h

list(APPEND version_h_output
    "${PROJECT_SOURCE_DIR}/lib/version.h")

if(WIN32)
    add_custom_command(
        OUTPUT ${version_h_output}
        COMMAND call call "setup_env.bat" && bash -c "\"./mkversion\" \"-o\" \"version.h\" \"lib\""
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT Generate version.h
        VERBATIM
    )
else()
    add_custom_command(
        OUTPUT ${version_h_output}
        COMMAND ./mkversion -o version.h lib
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT Generate version.h
        VERBATIM
    )
endif()

add_custom_target(version_h
    DEPENDS ${version_h_output}
)

# End auto-generated code

if(EMSCRIPTEN)
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
    lib/version.h
    lib/vk_format.h
    lib/vkformat_check.c
    lib/vkformat_enum.h
    lib/vkformat_str.c
    )
add_dependencies(ktx version_h mkvk)

target_compile_definitions(ktx PUBLIC LIBKTX)
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


target_compile_definitions(ktx PUBLIC "$<$<CONFIG:Debug>:_DEBUG>")

# To reduce size, don't support transcoding to ancient formats.
target_compile_definitions(ktx PUBLIC BASISD_SUPPORT_FXT1=0 )

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
    PRIVATE
        lib
        lib/basisu
    )

endif()

if(WIN32)
    target_link_libraries(
        ktx
        ${CMAKE_CURRENT_SOURCE_DIR}/other_lib/win/Release-x64/zstd_static.lib
        )
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

if(OPENGL_FOUND)
    target_include_directories(
        ktx
    PRIVATE
        ${OPENGL_INCLUDE_DIR}
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

    if(Vulkan_FOUND)
        if(NOT Vulkan_INCLUDE_DIRS AND Vulkan_INCLUDE_DIR)
            # Fallback for inconsistent variable name on Windows
            set(Vulkan_INCLUDE_DIRS ${Vulkan_INCLUDE_DIR})
        endif()

        target_compile_definitions(ktx PUBLIC USE_VULKAN_SDK)
        target_include_directories(
            ktx
        PUBLIC
            ${Vulkan_INCLUDE_DIRS}
        )
    elseif(APPLE)
        # TODO: Make it more elegant than fallback to VULKAN_SDK variable
        target_include_directories(
            ktx
        PUBLIC
            ${VULKAN_SDK}/include
        )
    endif()
else()
    target_compile_definitions( ktx PRIVATE KTX_OMIT_VULKAN=1 )
endif()

if(EMSCRIPTEN)

    add_executable( ktx_js interface/js_binding/ktx_wrapper.cpp )
    target_link_libraries( ktx_js ktx )
    target_include_directories( ktx_js PRIVATE $<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES> )
    target_link_options( ktx_js PUBLIC --bind )
    
    add_executable( msc_basis_transcoder_js interface/js_binding/transcoder_wrapper.cpp )
    target_link_libraries( msc_basis_transcoder_js ktx )
    target_include_directories( msc_basis_transcoder_js
        PRIVATE
        lib
        lib/basisu/transcoder
    )
    target_link_options( msc_basis_transcoder_js PUBLIC --bind )

endif()

if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()
set(CMAKE_CXX_STANDARD 11)

if(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Gz")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Gz")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} /O2")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} /O2")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2 -flto")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2 -flto")
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

if(NOT IOS AND NOT ANDROID AND NOT EMSCRIPTEN)
    add_subdirectory(tools)
endif()

add_subdirectory(tests)

if(KTX_FEATURE_DOC)
    include(cmake/docs.cmake)
endif()

install(TARGETS ktx)

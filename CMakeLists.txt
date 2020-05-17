cmake_minimum_required(VERSION 3.14)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules/")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules/")

project(KTX-Software)

include(CTest)

find_package(Doxygen)
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


# Might become options in the future

# option( KTX_FEATURE_KTX1 "KTX version 1" ON )
# option( KTX_FEATURE_KTX2 "KTX version 2" ON )
# option( KTX_FEATURE_WRITE "Create KTX files" ON )

set( KTX_FEATURE_WRITE ON )
set( KTX_FEATURE_KTX1 ON )
set( KTX_FEATURE_KTX2 ON )
set( KTX_FEATURE_VULKAN ON )
set( KTX_FEATURE_DOC ON )

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
        COMMAND set VULKAN_SDK=${VULKAN_SDK} & lib/mkvkformatfiles lib
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

set( KTX_DEFS LIBKTX )

if(EMSCRIPTEN)
    set(LIB_TYPE STATIC)
    list( APPEND KTX_DEFS KHRONOS_STATIC )
else()
    set(LIB_TYPE SHARED)
endif()

# Main library
add_library( ktx ${LIB_TYPE}
    $<TARGET_OBJECTS:objKtxOpenGLLoader>
    include/ktx.h
    lib/basis_sgd.h
    lib/basis_transcode.cpp
    lib/basisu_image_transcoders.cpp
    lib/basisu_image_transcoders.h
    lib/basisu_transcoder_config.h
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
    lib/filestream.c
    lib/filestream.h
    lib/formatsize.h
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

add_library( objKtxOpenGLLoader OBJECT
    lib/etcdec.cxx
    lib/etcunpack.cxx
    lib/gl_format.h
    lib/gl_funcs.c
    lib/gl_funcs.h
    lib/glloader.c
)

# list(APPEND KTX_DEFS
#     BASISD_SUPPORT_FXT1=1
#     BASISD_SUPPORT_BC7=1
#     BASISD_SUPPORT_PVRTC2=1
#     BASISD_SUPPORT_ETC2_EAC_RG11=1
#     BASISD_SUPPORT_ASTC_HIGHER_OPAQUE_QUALITY=1
#     )

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND KTX_DEFS
        _DEBUG)
endif()

if(KTX_FEATURE_KTX1)
    list( APPEND KTX_DEFS
        KTX_FEATURE_KTX1
    )
    target_sources(
        ktx
        PRIVATE
        lib/texture1.c
        lib/texture1.h
    )
endif()

if(KTX_FEATURE_KTX2)
    list( APPEND KTX_DEFS
        KTX_FEATURE_KTX2
    )
endif()

if(KTX_FEATURE_WRITE)

    add_library( objBasisuWrite OBJECT
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
    )
    target_include_directories(
        objBasisuWrite
        PUBLIC
        lib/basisu
    )

    add_library( objKtxWrite OBJECT
        lib/basis_encode.cpp
        lib/writer1.c
        lib/writer2.c
    )
    add_dependencies(objKtxWrite version_h mkvk)
    target_include_directories(
        objKtxWrite
        PUBLIC
        include
        PRIVATE
        lib
        other_include
        )

    target_sources(
        ktx
        PRIVATE
        $<TARGET_OBJECTS:objKtxWrite>
        $<TARGET_OBJECTS:objBasisuWrite>
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

target_include_directories(
    ktx
    PUBLIC
    include
    PRIVATE
    other_include
    lib/basisu/transcoder
)

target_include_directories(
    objKtxOpenGLLoader
    PUBLIC
    include
    other_include
)

if(OPENGL_FOUND)
    target_include_directories(
        objKtxOpenGLLoader
        PRIVATE
        ${OPENGL_INCLUDE_DIR}
    )
endif()

target_compile_definitions(
    objKtxOpenGLLoader
    PRIVATE
    ${KTX_DEFS}
)

if(KTX_FEATURE_VULKAN)
    add_library( objKtxVulkanLoader OBJECT
        include/ktxvulkan.h
        lib/vk_funcs.c
        lib/vk_funcs.h
        lib/vkloader.c
    )
    target_sources( ktx PRIVATE $<TARGET_OBJECTS:objKtxVulkanLoader> )
    target_include_directories(
        objKtxVulkanLoader
        PUBLIC
        include
        other_include
    )
    target_compile_definitions(
        objKtxVulkanLoader
        PRIVATE
        ${KTX_DEFS}
    )

    if(Vulkan_FOUND)
        list(APPEND KTX_DEFS
            USE_VULKAN_SDK
        )
        target_include_directories(
            objKtxVulkanLoader
            PRIVATE
            ${Vulkan_INCLUDE_DIRS}
        )
    elseif(APPLE)
        # TODO: Make it more elegant than fallback to VULKAN_SDK variable
        target_include_directories(
            objKtxVulkanLoader
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
    target_include_directories( ktx_js
        PRIVATE
        lib
        lib/basisu/transcoder
    )
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

target_compile_definitions(
    ktx
    PRIVATE
    ${KTX_DEFS}
)
if(KTX_FEATURE_WRITE)
    target_compile_definitions(
        objKtxWrite
        PRIVATE
        ${KTX_DEFS}
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

if(NOT IOS AND NOT ANDROID AND NOT EMSCRIPTEN)
    add_subdirectory(tools)
endif()

add_subdirectory(tests)

if(KTX_FEATURE_DOC)
    if(DOXYGEN_FOUND)
        include(cmake/docs.cmake)
    else()
        message("Doxygen not found -> skipping documentation")
    endif()
endif()

install(TARGETS ktx)

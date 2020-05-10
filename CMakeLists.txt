cmake_minimum_required(VERSION 3.14)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules/")

project(KTX-Software)

include(CTest)

find_package(Doxygen)

if(NOT ANDROID)
    find_package(OpenGL)
endif()

if(NOT IOS AND NOT ANDROID)
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
    "lib/vkformat_enum.h"
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
    "lib/dfdutils/vk2dfd.inl")
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
    "lib/dfdutils/dfd2vk.inl")

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
    "lib/version.h")

add_custom_command(
    OUTPUT ${version_h_output}
    COMMAND ./mkversion -o version.h lib
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT Generating DFD/VkFormat switch body
    VERBATIM
)

add_custom_target(version_h
    DEPENDS ${version_h_output}
)

# End auto-generated code

# Main library
add_library( ktx SHARED
    $<TARGET_OBJECTS:objKtxOpenGLLoader>
    $<TARGET_OBJECTS:objKtxVulkanLoader>
    lib/basis_transcode.cpp
    lib/basisu_image_transcoders.cpp
    lib/checkheader.c
    lib/filestream.c
    lib/hashlist.c
    lib/info.c
    lib/memstream.c
    lib/strings.c
    lib/swap.c
    lib/texture.c
    lib/texture2.c
    lib/vk_funcs.c
    lib/vkformat_check.c
    lib/vkformat_str.c

    lib/dfdutils/createdfd.c
    lib/dfdutils/interpretdfd.c
    lib/dfdutils/printdfd.c
    lib/dfdutils/queries.c
    lib/dfdutils/vk2dfd.c

    lib/dfdutils/dfd2vk.inl
    lib/basisu/transcoder/basisu_transcoder.cpp
    )
add_dependencies(ktx version_h mkvk)

add_library( objKtxOpenGLLoader OBJECT
    lib/etcdec.cxx
    lib/etcunpack.cxx
    lib/glloader.c
    lib/gl_funcs.c
)

add_library( objKtxVulkanLoader OBJECT
    lib/vkloader.c
)

set(KTX_DEFS LIBKTX)

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
        lib/basisu/basisu_astc_decomp.cpp
        lib/basisu/basisu_backend.cpp
        lib/basisu/basisu_basis_file.cpp
        lib/basisu/basisu_bc7enc.cpp
        lib/basisu/basisu_comp.cpp
        lib/basisu/basisu_enc.cpp
        lib/basisu/basisu_etc.cpp
        lib/basisu/basisu_frontend.cpp
        lib/basisu/basisu_global_selector_palette_helpers.cpp
        lib/basisu/basisu_gpu_texture.cpp
        lib/basisu/basisu_pvrtc1_4.cpp
        lib/basisu/basisu_resample_filters.cpp
        lib/basisu/basisu_resampler.cpp
        lib/basisu/basisu_ssim.cpp
        lib/basisu/basisu_tool.cpp
        lib/basisu/basisu_uastc_enc.cpp
        lib/basisu/jpgd.cpp
        lib/basisu/lodepng.cpp
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

if(Vulkan_FOUND)
    list(APPEND KTX_DEFS
        USE_VULKAN_SDK
    )    
endif()

if(APPLE)
target_link_libraries(
    ktx
    ${CMAKE_CURRENT_SOURCE_DIR}/other_lib/mac/Release/libzstd.a
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
    ${Vulkan_INCLUDE_DIRS}
    other_include
    lib/basisu/transcoder
)

target_include_directories(
    objKtxOpenGLLoader
    PUBLIC
    include
    other_include
    ${OPENGL_INCLUDE_DIR}
)

target_compile_definitions(
    objKtxOpenGLLoader
    PRIVATE
    ${KTX_DEFS}
)

target_include_directories(
    objKtxVulkanLoader
    PUBLIC
    include
    other_include
    ${Vulkan_INCLUDE_DIRS}
)

target_compile_definitions(
    objKtxVulkanLoader
    PRIVATE
    ${KTX_DEFS}
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
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

add_library( objUtil OBJECT utils/argparser.cpp )
target_include_directories(
    objUtil
    PUBLIC
    utils
    )

add_subdirectory(interface/basisu_c_binding)

if(NOT IOS AND NOT ANDROID)
    add_subdirectory(tools)
    add_subdirectory(tests)
endif()

if(DOXYGEN_FOUND)
    include(cmake/docs.cmake)
else()
    message("Doxygen not found -> skipping documentation")
endif()

install(TARGETS ktx)

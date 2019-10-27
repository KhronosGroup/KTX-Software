cmake_minimum_required(VERSION 3.14)

project(KTX-Software)

include(CTest)

if(NOT ANDROID)
    find_package(OpenGL)
endif()

if(NOT IOS AND NOT ANDROID)
    find_package(Vulkan)
endif()

if(IOS)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.0" CACHE STRING "Minimum iOS deployment version")
endif()

option( KTX_FEATURE_KTX1 "KTX version 1" ON )
option( KTX_FEATURE_KTX2 "KTX version 2" ON )

option( KTX_FEATURE_UPLOAD_OPENGL "Directly upload and create textures for OpenGL" ON )

set( KTX_GL_CONTEXT_PROFILE "CORE" CACHE STRING "Open GL context profile" )
set_property(CACHE KTX_GL_CONTEXT_PROFILE PROPERTY STRINGS
    CORE
    COMPATIBILITY
    ES
)

set( KTX_GL_CONTEXT_MAJOR_VERSION 3 CACHE STRING "OpenGL major version" )
set( KTX_GL_CONTEXT_MINOR_VERSION 3 CACHE STRING "OpenGL minor version" )

option( KTX_FEATURE_UPLOAD_VULKAN "Directly upload and create textures for Vulkan" ON )

option( KTX_FEATURE_WRITE "Create KTX files" ON )

if( KTX_FEATURE_UPLOAD_OPENGL OR KTX_FEATURE_UPLOAD_VULKAN )
    set( KTX_FEATURE_UPLOAD ON )
endif()

set( VULKAN_SDK "" CACHE PATH "Path to Vulkan SDK")

# Begin auto-generated code

list(APPEND mkvkformatfiles_input
    "lib/dfdutils/vulkan/vulkan_core.h"
    "lib/mkvkformatfiles")
list(APPEND mkvkformatfiles_output
    "lib/vkformat_enum.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/vkformat_check.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/vkformat_str.c")

add_custom_command(OUTPUT ${mkvkformatfiles_output}
    COMMAND ${CMAKE_COMMAND} -E make_directory lib
    COMMAND VULKAN_SDK=${VULKAN_SDK} lib/mkvkformatfiles lib
    DEPENDS ${mkvkformatfiles_input}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT Generating VkFormat-related source files
    VERBATIM
)

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
    COMMAND lib/dfdutils/makevkswitch.pl lib/vkformat_enum.h lib/dfdutils/vk2dfd.inl
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

# End auto-generated code

set(KTX_LIB_READ_SRC
    lib/basis_transcode.cpp
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
    lib/dfdutils/vk2dfd.c

    lib/dfdutils/dfd2vk.inl
    )

set(BASISU_SRC
    lib/basisu/transcoder/basisu_transcoder.cpp
)

set(KTX_DEFS LIBKTX)

if (CMAKE_BUILD_TYPE EQUAL "DEBUG")
    list(APPEND KTX_DEFS
        _DEBUG)
endif()

if(KTX_FEATURE_KTX1)
    list( APPEND KTX_DEFS
        KTX_FEATURE_KTX1
    )
    list(APPEND KTX_LIB_READ_SRC
        lib/texture1.c
    )
endif()

if(KTX_FEATURE_KTX2)
    list( APPEND KTX_DEFS
        KTX_FEATURE_KTX2
    )
endif()

set(KTX_LIB_SRC)
set(KTX_LIB_WRITE_SRC "")
set(KTX_LIB_READ_LIBS)

if(KTX_FEATURE_WRITE)

    set(BASISU_WRITE_SRC
        lib/basisu/basisu_astc_decomp.cpp
        lib/basisu/basisu_backend.cpp
        lib/basisu/basisu_basis_file.cpp
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
        lib/basisu/lodepng.cpp
    )
    add_library( objBasisuWrite OBJECT ${BASISU_WRITE_SRC} )

    list(APPEND KTX_LIB_WRITE_SRC
        lib/basis_encode.cpp
        lib/writer1.c
        lib/writer2.c
    )
    add_library( objKtxWrite OBJECT ${KTX_LIB_WRITE_SRC} )

    add_dependencies(objKtxWrite mkvk)
    target_include_directories(
        objKtxWrite
        PUBLIC
        include
        PRIVATE
        lib
        other_include
        "${VULKAN_SDK}/include"
        )
    target_compile_definitions(
        objKtxWrite
        PRIVATE
        ${KTX_DEFS}
        )
    list(APPEND KTX_LIB_SRC
        $<TARGET_OBJECTS:objKtxWrite>
        $<TARGET_OBJECTS:objBasisuWrite>
    )
endif()

if(KTX_FEATURE_UPLOAD OR KTX_FEATURE_WRITE)
    list(APPEND KTX_LIB_READ_SRC
        lib/etcunpack.cxx
        lib/etcdec.cxx
        
        lib/texture1_vvtbl.c
        lib/texture2_vvtbl.c
    )

    if(KTX_FEATURE_UPLOAD_OPENGL)
        list(APPEND KTX_LIB_READ_SRC
            lib/glloader.c
        )
        if (ANDROID)
            list(APPEND KTX_LIB_READ_LIBS GLESv2)
        else()
            list(APPEND KTX_LIB_READ_LIBS
                ${OPENGL_LIBRARIES}
            )
        endif()
        list(APPEND KTX_DEFS
            GL_CONTEXT_PROFILE=SDL_GL_CONTEXT_PROFILE_${KTX_GL_CONTEXT_PROFILE}
            GL_CONTEXT_MAJOR_VERSION=${KTX_GL_CONTEXT_MAJOR_VERSION}
            GL_CONTEXT_MINOR_VERSION=${KTX_GL_CONTEXT_MINOR_VERSION}
        )
    endif()
    
    if(KTX_FEATURE_UPLOAD_VULKAN)
        list(APPEND KTX_LIB_READ_SRC
            lib/vkloader.c
            lib/dfdutils/dfd2vk.c
        )
        list(APPEND KTX_LIB_READ_LIBS
            ${Vulkan_LIBRARIES}
        )
    endif()

    list(APPEND KTX_DEFS
        KTX_OPENGL
        # KTX_OPENGL_ES1
        # KTX_OPENGL_ES2
        # KTX_OPENGL_ES3
    )
endif()

if(KTX_FEATURE_UPLOAD)
    list(APPEND KTX_DEFS
        KTX_FEATURE_UPLOAD
    )
endif()

if(NOT KTX_FEATURE_UPLOAD_VULKAN)
    list(APPEND KTX_DEFS
        KTX_OMIT_VULKAN
    )
endif()

if(VULKAN_SDK)
    list(APPEND KTX_DEFS
        USE_VULKAN_SDK
    )    
endif()

add_library( objBasisu OBJECT ${BASISU_SRC} )
add_library( objKtxRead OBJECT ${KTX_LIB_READ_SRC} )

add_library( ktx SHARED
    $<TARGET_OBJECTS:objKtxRead>
    $<TARGET_OBJECTS:objBasisu>
    ${KTX_LIB_SRC}
    )

add_dependencies(objKtxRead mkvk)

target_link_libraries( ktx ${KTX_LIB_READ_LIBS} )

target_include_directories(
    objBasisu
    PUBLIC
    lib/basisu/transcoder
)

target_include_directories(
    objKtxRead
    PUBLIC
    include
    PRIVATE
    "${VULKAN_SDK}/include"
    other_include
)

set(CMAKE_C_FLAGS "-fPIC")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2 -flto")
set(CMAKE_CXX_FLAGS "-std=c++11 -fPIC")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2 -flto")
set(CMAKE_CXX_STANDARD 11)

target_compile_definitions(
    objKtxRead
    PRIVATE
    ${KTX_DEFS}
)

add_library( objUtil OBJECT utils/argparser.cpp )
target_include_directories(
    objUtil
    PUBLIC
    utils
    )

if(NOT IOS AND NOT ANDROID)
    add_subdirectory(tools)
    add_subdirectory(tests)
endif()


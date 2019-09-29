cmake_minimum_required(VERSION 3.14)

project(KTX-Software)

include(CTest)

find_package(OpenGL)
find_package(Vulkan)

option( KTX_FEATURE_KTX1 "KTX version 1" OFF )
option( KTX_FEATURE_KTX2 "KTX version 2" ON )

option( KTX_FEATURE_UPLOAD_OPENGL "Directly upload and create textures for OpenGL" OFF )
option( KTX_FEATURE_UPLOAD_VULKAN "Directly upload and create textures for Vulkan" OFF )

option( KTX_FEATURE_WRITE "Create KTX files" OFF )

option( KTX_UNITY_LIB "Build a library for use in Unity 3D" OFF )

if( KTX_FEATURE_UPLOAD_OPENGL OR KTX_FEATURE_UPLOAD_VULKAN )
    set( KTX_FEATURE_UPLOAD ON )
endif()

set( VULKAN_SDK "" CACHE PATH "Path to Vulkan SDK")

set( UNITY_PACKAGE_PATH "" CACHE PATH "Path locating the Unity package source. When installing, native libraries will get injected there" )

set(KTX_LIB_READ_SRC
    lib/basis_transcode.cpp
    lib/checkheader.c
    lib/errstr.c
    lib/filestream.c
    lib/hashlist.c
    lib/info.c
    lib/memstream.c
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

    lib/sgd_helper.cpp
    )

set(BASISU_SRC
    lib/basisu/transcoder/basisu_transcoder.cpp
)

set(KTX_DEFS "")

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

    target_include_directories(
        objKtxWrite
        PUBLIC
        include
        PRIVATE
        other_include
        "${VULKAN_SDK}/include"
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
        list(APPEND KTX_LIB_READ_LIBS
            ${OPENGL_LIBRARIES}
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
    other_include
    "${VULKAN_SDK}/include"
)

set(CMAKE_CXX_FLAGS -std=c++11)
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

add_subdirectory(tools)

if(KTX_UNITY_LIB)
    add_subdirectory(interface/unity)
endif()

add_subdirectory(tests)

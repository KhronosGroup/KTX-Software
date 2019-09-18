cmake_minimum_required(VERSION 3.14)

project(KTX-Software)

# option( KTX_FEATURE_KTX1 "KTX version 1" OFF )
option( KTX_FEATURE_KTX2 "KTX version 2" ON )

# option( KTX_FEATURE_UPLOAD "Directly upload and create textures" OFF )

# option( KTX_FEATURE_WRITE "Create KTX files" OFF )

set( VULKAN_SDK "" CACHE PATH "Path to Vulkan SDK")

set(KTX_LIB_SRC
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
    )

set(BASISU_SRC
    lib/basisu/transcoder/basisu_transcoder.cpp
)

set(KTX_DEFS "")

# if(KTX_FEATURE_KTX1)
#     list( APPEND KTX_DEFS
#         KTX_FEATURE_KTX1
#     )
#     list(APPEND KTX_LIB_SRC
#         lib/texture1.c
#     )
# endif()

if(KTX_FEATURE_KTX2)
    list( APPEND KTX_DEFS
        KTX_FEATURE_KTX2
    )
endif()

# if(KTX_FEATURE_WRITE)
#     list(APPEND KTX_LIB_SRC
#         lib/basis_encode.cpp
#         lib/vkloader.c
#         lib/writer1.c
#         lib/writer2.c
#         lib/dfdutils/dfd2vk.c
#     )
# endif()

# if(KTX_FEATURE_UPLOAD)
#     list(APPEND KTX_LIB_SRC
#         lib/glloader.c
#         lib/texture1_vvtbl.c
#         lib/texture2_vvtbl.c
#     )
# endif()

if(VULKAN_SDK)
    list(APPEND KTX_DEFS
        USE_VULKAN_SDK
    )
endif()

add_library(ktx SHARED ${KTX_LIB_SRC} ${BASISU_SRC})

target_include_directories(
    ktx
    PUBLIC
    include
    PRIVATE
    other_include
    "${VULKAN_SDK}/include"
)
set(CMAKE_CXX_FLAGS -std=c++11)
set(CMAKE_CXX_STANDARD 11)

target_compile_definitions(
    ktx
    PRIVATE
    KTX_OMIT_VULKAN
    ${KTX_DEFS}
)

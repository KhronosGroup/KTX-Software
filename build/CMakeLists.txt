cmake_minimum_required(VERSION 2.8)
project(KTX)

# determine GL platform
find_path(GLES2_PATH GLES2/gl2.h)
find_path(GLES3_PATH GLES2/gl3.h)

if(GLES2_PATH)
    add_definitions(-DKTX_OPENGL_ES2=1)
elseif(GLES3_PATH)
    add_definitions(-DKTX_OPENGL_ES3=1)
else(GLES3_PATH)
    add_definitions(-DKTX_OPENGL=1)
endif(GLES2_PATH)

# libktx
file(GLOB ktx_SRC ../lib/*.c*)
include_directories(../include/)
add_library(ktx ${ktx_SRC})

# toktx
file(GLOB toktx_SRC ../tools/toktx/*.c*)
add_executable(toktx ${toktx_SRC})
target_link_libraries(toktx ktx)
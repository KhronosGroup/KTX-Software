# -*- tab-width: 4; -*-
# vi: set sw=2 ts=4:
cmake_minimum_required(VERSION 2.8)
project(KTX)

set(DEPTH ../${DEPTH})

# Add GL_PATH because it is not in the standard compiler search path
# on all OS platforms.
#include_directories(${DEPTH}/include "${glheader}")

# toktx
file(GLOB toktx_SRC ${DEPTH}/tools/toktx/*.c*)
add_executable(toktx ${toktx_SRC})
target_link_libraries(toktx ktx)


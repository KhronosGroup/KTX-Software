# -*- tab-width: 4; -*-
# vi: set sw=2 ts=4:
cmake_minimum_required(VERSION 2.8)
project(KTX)

set(DEPTH ../..)

find_path(GLCORE_PATH GL/glcorearb.h PATHS $ENV{HI_BSLIB_HOME}/include)
find_path(GLEW_PATH GL/glew.h PATHS $ENV{HI_BSLIB_HOME}/include)

add_subdirectory(libktx libktx)
add_subdirectory(toktx toktx)

add_dependencies(toktx libktx)


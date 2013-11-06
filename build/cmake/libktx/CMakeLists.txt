# -*- tab-width: 4; -*-
# vi: set sw=2 ts=4:
cmake_minimum_required(VERSION 2.8)
project(KTX)

set(DEPTH ../${DEPTH})
  
# glheader is set in the parent.

# Add GL{,ES[123]}_PATH because it is not in the standard compiler
# search path on all OS platforms.
#include_directories(SYSTEM "${glheader}")

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

file(GLOB ktx_SRC ${DEPTH}/lib/*.c*)
add_library(ktx ${ktx_SRC})


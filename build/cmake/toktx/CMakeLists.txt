# -*- tab-width: 4; -*-
# vi: set sw=2 ts=4:
cmake_minimum_required(VERSION 2.8)
project(KTX)

set(DEPTH ../${DEPTH})

# toktx is compiled using OpenGL includes so it has access to
# the enums for all defined GL formats and types.
if(TOKTX_GL_HEADER)
  set(glheader ${${TOKTX_GL_HEADER}_PATH})
  if(${glheader} STREQUAL ".*-NOTFOUND")
	message(FATAL_ERROR "Selected \"${TOKTX_GL_HEADER}_PATH\" path not found. Please set it or select a different TOKTX_GL_HEADER.")
  endif()
else()
  if(GLCORE_PATH AND GLEW_PATH)
	set(glheader "GLCORE")
	message("Both glcorearb.h and glew.h found. Set TOKTX_GL_HEADER to override\n"
            "automatic choice of \"${glheader}\"")
  elseif(GLCORE_PATH)
	set(glheader "GLCORE")
  elseif(GLEW_PATH)
	set(glheader "GLEW")
  else()
	message(FATAL_ERROR "Can't find desktop GL include file to build toktx. Please set GLCORE_PATH or GLEW_PATH")
  endif()
  set(TOKTX_GL_HEADER ${glheader} CACHE STRING
      "Choose GL header to use for building toktx. Options are GLCORE and GLEW.")
endif()

add_definitions(-DKTX_OPENGL=1)
if(TOKTX_GL_HEADER STREQUAL "GLEW")
  add_definitions(-DKTX_USE_GETPROC=1)
endif()


# Add GL_PATH because it is not in the standard compiler search path
# on all OS platforms.
include_directories(${DEPTH}/include "${${TOKTX_GL_HEADER}_PATH}")

# toktx
file(GLOB toktx_SRC ${DEPTH}/tools/toktx/*.c*)
add_executable(toktx ${toktx_SRC})
target_link_libraries(toktx ktx)


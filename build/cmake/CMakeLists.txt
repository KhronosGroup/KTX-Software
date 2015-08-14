# -*- tab-width: 4; -*-
# vi: set sw=2 ts=4:
cmake_minimum_required(VERSION 2.8)
project(KTX)

set(DEPTH ../..)

# GL include paths
find_path(GLCORE_PATH GL/glcorearb.h PATHS $ENV{HI_BSLIB_HOME}/include)
find_path(GLEW_PATH GL/glew.h PATHS $ENV{HI_BSLIB_HOME}/include)
find_path(GLES1_PATH GLES/gl.h PATHS $ENV{HI_BSLIB_HOME}/include)
find_path(GLES2_PATH GLES2/gl2.h PATHS $ENV{HI_BSLIB_HOME}/include)
find_path(GLES3_PATH GLES3/gl3.h PATHS $ENV{HI_BSLIB_HOME}/include)

if(KTX_GL_PLATFORM)
  set(glheader ${${KTX_GL_PLATFORM}_PATH})
  if(${glheader} STREQUAL ".*-NOTFOUND")
	message(FATAL_ERROR "Selected \"${KTX_GL_PLATFORM}_PATH\" path not found. Please set it or select a different KTX_GL_PLATFORM.")
  endif()
else()
  set(num_paths 0)
  foreach(name "GLES1" "GLES2" "GLES3" "GLCORE" "GLEW")
    set(path ${${name}_PATH})
	if(path)
	  set(glheader ${${path}})
      set(glplatform ${name})
	  math(EXPR num_paths "${num_paths}+1")
	endif()
  endforeach()

  if(${num_paths} EQUAL 0)
    message(FATAL_ERROR "Can't find a GL platform for building. Please set one of GL{CORE,EW,ES[123]}_PATH to a folder containing a GL header directory root.")
  elseif(${num_paths} GREATER 1)
	# Arbitrary priority
	if(GLEW_PATH)
	  set(glheader ${GLEW_PATH})
	  set(glplatform "GLEW")
	elseif(GLCORE_PATH)
	  set(glheader ${GLCORE_PATH})
	  set(glplatform "GLCORE")
	elseif(GLES3_PATH)
	  set(glheader ${GLES3_PATH})
	  set(glplatform "GLES3")
	elseif(GLES2_PATH)
	  set(glheader ${GLES2_PATH})
	  set(glplatform "GLES2")
	elseif(GLES1_PATH)
	  set(glheader ${GLES1_PATH})
	  set(glplatform "GLES1")
	endif()
	set(KTX_GL_PLATFORM ${glplatform} CACHE STRING
		# Must be a single string!
		"Choose the GL platform to use for build. Initial automatic choice is based on platforms found on system.")
	set_property(CACHE KTX_GL_PLATFORM PROPERTY STRINGS
	 			 "GLEW;GLCORE;GLES1;GLES2;GLES3")
	message("Multiple GL platforms found for build. Set KTX_GL_PLATFORM\n"
            "to override automatic choice of \"${KTX_GL_PLATFORM}\"")
  endif()
endif()
if (KTX_GL_PLATFORM STREQUAL "GLCORE" OR KTX_GL_PLATFORM STREQUAL "GLEW")
    add_definitions(-DKTX_OPENGL=1)
elseif (KTX_GL_PLATFORM STREQUAL "GLES1")
    add_definitions(-DKTX_OPENGL_ES1=1)
elseif (KTX_GL_PLATFORM STREQUAL "GLES2")
    add_definitions(-DKTX_OPENGL_ES2=1)
elseif (KTX_GL_PLATFORM STREQUAL "GLES3")
    add_definitions(-DKTX_OPENGL_ES3=1)
endif ()
if(KTX_GL_PLATFORM STREQUAL "GLEW")
  add_definitions(-DKTX_USE_GETPROC=1)
endif()

# Add GL{,ES[123]}_PATH because it is not in the standard compiler
# search path on all OS platforms.
include_directories(SYSTEM "${glheader}")
include_directories(${DEPTH}/include)

add_subdirectory(libktx libktx)
add_subdirectory(toktx toktx)

add_dependencies(toktx ktx)


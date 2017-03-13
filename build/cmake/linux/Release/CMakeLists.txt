cmake_minimum_required(VERSION 2.8.8 FATAL_ERROR)
cmake_policy(VERSION 2.8.8)
project(readtests)
set(configuration "Release")
enable_language(ASM)
set(builddir "${CMAKE_CURRENT_BINARY_DIR}")
set(obj "${builddir}/obj")

set(CMAKE_C_OUTPUT_EXTENSION_REPLACE 1)
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)

set(CMAKE_NINJA_FORCE_RESPONSE_FILE 1)


#libktx.gyp:vulkan_headers#target
set(TARGET "vulkan_headers")
set(TOOLSET "target")
add_custom_target(vulkan_headers SOURCES)
set_target_properties(vulkan_headers PROPERTIES EXCLUDE_FROM_ALL "FALSE")
unset(TOOLSET)
unset(TARGET)

#libktx.gyp:libvulkan#target
set(TARGET "libvulkan")
set(TOOLSET "target")
add_custom_target(libvulkan SOURCES)
set_target_properties(libvulkan PROPERTIES EXCLUDE_FROM_ALL "FALSE")
add_dependencies(libvulkan
  vulkan_headers
)
unset(TOOLSET)
unset(TARGET)

#libktx.gyp:libktx.es3#target
set(TARGET "libktx.es3")
set(TOOLSET "target")
list(APPEND libktx_es3__c_srcs
  "../../../../lib/checkheader.c"
  "../../../../lib/errstr.c"
  "../../../../lib/glloader.c"
  "../../../../lib/hashtable.c"
  "../../../../lib/ktxfilestream.c"
  "../../../../lib/ktxmemstream.c"
  "../../../../lib/ktxreader.c"
  "../../../../lib/swap.c"
  "../../../../lib/writer.c"
  "../../../../lib/vkloader.c")
list(APPEND libktx_es3__cxx_srcs
  "../../../../lib/etcdec.cxx"
  "../../../../lib/etcunpack.cxx")
list(APPEND libktx_es3__other_srcs
  "../../../../include/ktx.h"
  "../../../../lib/gl_format.h"
  "../../../../lib/gl_funcptrs.h"
  "../../../../lib/gles1_funcptrs.h"
  "../../../../lib/gles2_funcptrs.h"
  "../../../../lib/gles3_funcptrs.h"
  "../../../../lib/ktxfilestream.h"
  "../../../../lib/ktxint.h"
  "../../../../lib/ktxmemstream.h"
  "../../../../lib/ktxstream.h"
  "../../../../lib/ktxreader.h"
  "../../../../lib/uthash.h"
  "../../../../include/ktxvulkan.h"
  "../../../../lib/vk_format.h")
add_library(libktx_es3 STATIC ${libktx_es3__c_srcs} ${libktx_es3__cxx_srcs} ${libktx_es3__other_srcs})
set_target_properties(libktx_es3 PROPERTIES EXCLUDE_FROM_ALL "TRUE")
set_source_files_properties(${libktx_es3__other_srcs} PROPERTIES HEADER_FILE_ONLY "TRUE")
set_target_properties(libktx_es3 PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${obj}.${TOOLSET}/")
set_target_properties(libktx_es3 PROPERTIES PREFIX "lib")
set_target_properties(libktx_es3 PROPERTIES ARCHIVE_OUTPUT_NAME "ktx.es3")
set_target_properties(libktx_es3 PROPERTIES SUFFIX ".a")
set_source_files_properties(${obj}.${TOOLSET}/libktx.es3.a PROPERTIES GENERATED "TRUE")
list(APPEND libktx_es3__include_dirs
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include"
  "/$ENV{VULKAN_SDK}/include")
set_property(TARGET libktx_es3 APPEND PROPERTY INCLUDE_DIRECTORIES ${libktx_es3__include_dirs})
set_target_properties(libktx_es3 PROPERTIES COMPILE_DEFINITIONS "KTX_OPENGL_ES3=1;NDEBUG;")
set_target_properties(libktx_es3 PROPERTIES COMPILE_FLAGS "-O3 ")
set_target_properties(libktx_es3 PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(libktx_es3 PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(libktx_es3 PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
add_dependencies(libktx_es3
  vulkan_headers
)
unset(TOOLSET)
unset(TARGET)

#libktx.gyp:libktx.es1#target
set(TARGET "libktx.es1")
set(TOOLSET "target")
list(APPEND libktx_es1__c_srcs
  "../../../../lib/checkheader.c"
  "../../../../lib/errstr.c"
  "../../../../lib/glloader.c"
  "../../../../lib/hashtable.c"
  "../../../../lib/ktxfilestream.c"
  "../../../../lib/ktxmemstream.c"
  "../../../../lib/ktxreader.c"
  "../../../../lib/swap.c"
  "../../../../lib/writer.c")
list(APPEND libktx_es1__cxx_srcs
  "../../../../lib/etcdec.cxx"
  "../../../../lib/etcunpack.cxx")
list(APPEND libktx_es1__other_srcs
  "../../../../include/ktx.h"
  "../../../../lib/gl_format.h"
  "../../../../lib/gl_funcptrs.h"
  "../../../../lib/gles1_funcptrs.h"
  "../../../../lib/gles2_funcptrs.h"
  "../../../../lib/gles3_funcptrs.h"
  "../../../../lib/ktxfilestream.h"
  "../../../../lib/ktxint.h"
  "../../../../lib/ktxmemstream.h"
  "../../../../lib/ktxstream.h"
  "../../../../lib/ktxreader.h"
  "../../../../lib/uthash.h")
add_library(libktx_es1 STATIC ${libktx_es1__c_srcs} ${libktx_es1__cxx_srcs} ${libktx_es1__other_srcs})
set_target_properties(libktx_es1 PROPERTIES EXCLUDE_FROM_ALL "TRUE")
set_source_files_properties(${libktx_es1__other_srcs} PROPERTIES HEADER_FILE_ONLY "TRUE")
set_target_properties(libktx_es1 PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${obj}.${TOOLSET}/")
set_target_properties(libktx_es1 PROPERTIES PREFIX "lib")
set_target_properties(libktx_es1 PROPERTIES ARCHIVE_OUTPUT_NAME "ktx.es1")
set_target_properties(libktx_es1 PROPERTIES SUFFIX ".a")
set_source_files_properties(${obj}.${TOOLSET}/libktx.es1.a PROPERTIES GENERATED "TRUE")
list(APPEND libktx_es1__include_dirs
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include")
set_property(TARGET libktx_es1 APPEND PROPERTY INCLUDE_DIRECTORIES ${libktx_es1__include_dirs})
set_target_properties(libktx_es1 PROPERTIES COMPILE_DEFINITIONS "KTX_OPENGL_ES1=1;NDEBUG;")
set_target_properties(libktx_es1 PROPERTIES COMPILE_FLAGS "-O3 ")
set_target_properties(libktx_es1 PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(libktx_es1 PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(libktx_es1 PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
unset(TOOLSET)
unset(TARGET)

#libktx.gyp:libktx.doc#target
set(TARGET "libktx.doc")
set(TOOLSET "target")
list(APPEND libktx_doc__buildDoc__input
  "${CMAKE_CURRENT_LIST_DIR}/../../../../ktxDoxy"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../LICENSE.md"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include/ktx.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/checkheader.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/errstr.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/etcdec.cxx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/etcunpack.cxx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/gl_format.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/gl_funcptrs.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/gles1_funcptrs.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/gles2_funcptrs.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/gles3_funcptrs.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/glloader.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/hashtable.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/ktxfilestream.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/ktxfilestream.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/ktxint.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/ktxmemstream.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/ktxmemstream.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/ktxstream.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/ktxreader.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/ktxreader.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/swap.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/uthash.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/writer.c")
list(APPEND libktx_doc__buildDoc__output
  "${CMAKE_CURRENT_LIST_DIR}/../../../../build/doc/html"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../build/doc/latex"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../build/doc/man")
add_custom_command(OUTPUT ${libktx_doc__buildDoc__output}
  COMMAND ${CMAKE_COMMAND} -E make_directory build/doc
  COMMAND doxygen ktxDoxy
  DEPENDS ${libktx_doc__buildDoc__input}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT Generating documentation with Doxygen
  VERBATIM
)
add_custom_target(libktx_doc__buildDoc
  DEPENDS ${libktx_doc__buildDoc__output}
  SOURCES ${libktx_doc__buildDoc__input}
)
list(APPEND libktx_doc__touchTimestamp__input
  "${CMAKE_CURRENT_LIST_DIR}/../../../../ktxDoxy"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../LICENSE.md"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include/ktx.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/checkheader.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/errstr.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/etcdec.cxx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/etcunpack.cxx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/gl_format.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/gl_funcptrs.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/gles1_funcptrs.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/gles2_funcptrs.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/gles3_funcptrs.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/glloader.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/hashtable.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/ktxfilestream.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/ktxfilestream.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/ktxint.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/ktxmemstream.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/ktxmemstream.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/ktxstream.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/ktxreader.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/ktxreader.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/swap.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/uthash.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/writer.c")
set(libktx_doc__touchTimestamp__output "${CMAKE_CURRENT_LIST_DIR}/../../../../build/doc/.gentimestamp")
add_custom_command(OUTPUT ${libktx_doc__touchTimestamp__output}
  COMMAND ${CMAKE_COMMAND} -E make_directory build/doc
  COMMAND touch build/doc/.gentimestamp
  DEPENDS ${libktx_doc__touchTimestamp__input}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT setting generation timestamp
  VERBATIM
)
add_custom_target(libktx_doc__touchTimestamp
  DEPENDS ${libktx_doc__touchTimestamp__output}
  SOURCES ${libktx_doc__touchTimestamp__input}
)
add_custom_target(libktx_doc SOURCES)
set_target_properties(libktx_doc PROPERTIES EXCLUDE_FROM_ALL "TRUE")
set_target_properties(libktx_doc__buildDoc PROPERTIES EXCLUDE_FROM_ALL "TRUE")
set_target_properties(libktx_doc__touchTimestamp PROPERTIES EXCLUDE_FROM_ALL "TRUE")
add_dependencies(libktx_doc
  libktx_doc__buildDoc
  libktx_doc__touchTimestamp
)
unset(TOOLSET)
unset(TARGET)

#libktx.gyp:libgl#target
set(TARGET "libgl")
set(TOOLSET "target")
add_custom_target(libgl SOURCES)
set_target_properties(libgl PROPERTIES EXCLUDE_FROM_ALL "FALSE")
unset(TOOLSET)
unset(TARGET)

#libktx.gyp:libktx.gl#target
set(TARGET "libktx.gl")
set(TOOLSET "target")
list(APPEND libktx_gl__c_srcs
  "../../../../lib/checkheader.c"
  "../../../../lib/errstr.c"
  "../../../../lib/glloader.c"
  "../../../../lib/hashtable.c"
  "../../../../lib/ktxfilestream.c"
  "../../../../lib/ktxmemstream.c"
  "../../../../lib/ktxreader.c"
  "../../../../lib/swap.c"
  "../../../../lib/writer.c"
  "../../../../lib/vkloader.c")
list(APPEND libktx_gl__cxx_srcs
  "../../../../lib/etcdec.cxx"
  "../../../../lib/etcunpack.cxx")
list(APPEND libktx_gl__other_srcs
  "../../../../include/ktx.h"
  "../../../../lib/gl_format.h"
  "../../../../lib/gl_funcptrs.h"
  "../../../../lib/gles1_funcptrs.h"
  "../../../../lib/gles2_funcptrs.h"
  "../../../../lib/gles3_funcptrs.h"
  "../../../../lib/ktxfilestream.h"
  "../../../../lib/ktxint.h"
  "../../../../lib/ktxmemstream.h"
  "../../../../lib/ktxstream.h"
  "../../../../lib/ktxreader.h"
  "../../../../lib/uthash.h"
  "../../../../include/ktxvulkan.h"
  "../../../../lib/vk_format.h")
link_directories()
add_library(libktx_gl SHARED ${libktx_gl__c_srcs} ${libktx_gl__cxx_srcs} ${libktx_gl__other_srcs})
set_target_properties(libktx_gl PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_source_files_properties(${libktx_gl__other_srcs} PROPERTIES HEADER_FILE_ONLY "TRUE")
set_target_properties(libktx_gl PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${builddir}/lib.${TOOLSET}")
set_target_properties(libktx_gl PROPERTIES PREFIX "lib")
set_target_properties(libktx_gl PROPERTIES LIBRARY_OUTPUT_NAME "ktx.gl")
set_target_properties(libktx_gl PROPERTIES SUFFIX ".so")
set_source_files_properties(${builddir}/lib.${TOOLSET}/libktx.gl.so PROPERTIES GENERATED "TRUE")
list(APPEND libktx_gl__include_dirs
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include"
  "/$ENV{VULKAN_SDK}/include")
set_property(TARGET libktx_gl APPEND PROPERTY INCLUDE_DIRECTORIES ${libktx_gl__include_dirs})
set_target_properties(libktx_gl PROPERTIES COMPILE_DEFINITIONS "KTX_OPENGL=1;NDEBUG;")
set_target_properties(libktx_gl PROPERTIES COMPILE_FLAGS "-O3 -fPIC ")
set_target_properties(libktx_gl PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(libktx_gl PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(libktx_gl PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
add_dependencies(libktx_gl
  libgl
  libvulkan
  vulkan_headers
)
target_link_libraries(libktx_gl
  "-lGL"
  "-lvulkan"
)
unset(TOOLSET)
unset(TARGET)

#ktxtools.gyp:toktx#target
set(TARGET "toktx")
set(TOOLSET "target")
list(APPEND toktx__cxx_srcs
  "../../../../tools/toktx/image.cpp"
  "../../../../tools/toktx/toktx.cpp")
list(APPEND toktx__other_srcs
  "../../../../tools/toktx/image.h"
  "../../../../tools/toktx/stdafx.h"
  "../../../../tools/toktx/targetver.h")
add_executable(toktx ${toktx__cxx_srcs} ${toktx__other_srcs})
set_target_properties(toktx PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_source_files_properties(${toktx__other_srcs} PROPERTIES HEADER_FILE_ONLY "TRUE")
set_target_properties(toktx PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${builddir}")
set_target_properties(toktx PROPERTIES PREFIX "")
set_target_properties(toktx PROPERTIES RUNTIME_OUTPUT_NAME "toktx")
set_target_properties(toktx PROPERTIES SUFFIX "")
set_source_files_properties(${builddir}/toktx PROPERTIES GENERATED "TRUE")
list(APPEND toktx__include_dirs
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include")
set_property(TARGET toktx APPEND PROPERTY INCLUDE_DIRECTORIES ${toktx__include_dirs})
set_target_properties(toktx PROPERTIES COMPILE_DEFINITIONS "NDEBUG;")
set_target_properties(toktx PROPERTIES COMPILE_FLAGS "-O3 ")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "YES")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
target_link_libraries(toktx
  libktx_gl
)
unset(TOOLSET)
unset(TARGET)

#ktxtools.gyp:toktx-tests#target
set(TARGET "toktx-tests")
set(TOOLSET "target")
set(toktx-tests__toktx-tests__input "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/toktx-tests")
set(toktx-tests__toktx-tests__output "${CMAKE_CURRENT_LIST_DIR}/../../../../tools/toktx/testsrun")
add_custom_command(OUTPUT ${toktx-tests__toktx-tests__output}
  COMMAND ${CMAKE_COMMAND} -E make_directory tools/toktx
  COMMAND tests/toktx-tests "${builddir}/toktx"
  DEPENDS ${toktx-tests__toktx-tests__input}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT Running toktx tests
  VERBATIM
)
add_custom_target(toktx-tests__toktx-tests
  DEPENDS ${toktx-tests__toktx-tests__output}
  SOURCES ${toktx-tests__toktx-tests__input}
)
add_custom_target(toktx-tests SOURCES)
set_target_properties(toktx-tests PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(toktx-tests__toktx-tests PROPERTIES EXCLUDE_FROM_ALL "FALSE")
add_dependencies(toktx-tests__toktx-tests
  toktx
)
add_dependencies(toktx-tests
  toktx
  toktx-tests__toktx-tests
)
unset(TOOLSET)
unset(TARGET)

#ktxtests.gyp:testimages#target
set(TARGET "testimages")
set(TOOLSET "target")
list(APPEND testimages__copies__input
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/conftestimage_R11_EAC.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/conftestimage_RG11_EAC.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/conftestimage_SIGNED_R11_EAC.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/conftestimage_SIGNED_RG11_EAC.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/cubemap_space.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/cubemap_vulkan.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/cubemap_yokohama.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/etc1.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/etc2-rgba1.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/etc2-rgba8.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/etc2-rgb.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/etc2-sRGBa1.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/etc2-sRGBa8.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/etc2-sRGB.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/hi_mark.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/hi_mark_sq.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/luminance_sized_reference.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/luminance_unsized_reference.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/no-npot.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/orient-down-metadata.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/orient-down-metadata-sized.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/orient-up.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/orient-up-metadata-sized.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/orient-up-sized.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/pattern_02_bc2.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/rgb-amg-reference.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/rgba-reference.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/rgb-mipmap-reference.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/rgb-reference.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/texturearray_bc3.ktx")
list(APPEND testimages__copies__output
  "${builddir}/testimages/conftestimage_R11_EAC.ktx"
  "${builddir}/testimages/conftestimage_RG11_EAC.ktx"
  "${builddir}/testimages/conftestimage_SIGNED_R11_EAC.ktx"
  "${builddir}/testimages/conftestimage_SIGNED_RG11_EAC.ktx"
  "${builddir}/testimages/cubemap_space.ktx"
  "${builddir}/testimages/cubemap_vulkan.ktx"
  "${builddir}/testimages/cubemap_yokohama.ktx"
  "${builddir}/testimages/etc1.ktx"
  "${builddir}/testimages/etc2-rgba1.ktx"
  "${builddir}/testimages/etc2-rgba8.ktx"
  "${builddir}/testimages/etc2-rgb.ktx"
  "${builddir}/testimages/etc2-sRGBa1.ktx"
  "${builddir}/testimages/etc2-sRGBa8.ktx"
  "${builddir}/testimages/etc2-sRGB.ktx"
  "${builddir}/testimages/hi_mark.ktx"
  "${builddir}/testimages/hi_mark_sq.ktx"
  "${builddir}/testimages/luminance_sized_reference.ktx"
  "${builddir}/testimages/luminance_unsized_reference.ktx"
  "${builddir}/testimages/no-npot.ktx"
  "${builddir}/testimages/orient-down-metadata.ktx"
  "${builddir}/testimages/orient-down-metadata-sized.ktx"
  "${builddir}/testimages/orient-up.ktx"
  "${builddir}/testimages/orient-up-metadata-sized.ktx"
  "${builddir}/testimages/orient-up-sized.ktx"
  "${builddir}/testimages/pattern_02_bc2.ktx"
  "${builddir}/testimages/rgb-amg-reference.ktx"
  "${builddir}/testimages/rgba-reference.ktx"
  "${builddir}/testimages/rgb-mipmap-reference.ktx"
  "${builddir}/testimages/rgb-reference.ktx"
  "${builddir}/testimages/texturearray_bc3.ktx")
add_custom_command(
OUTPUT ${testimages__copies__output}
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/conftestimage_R11_EAC.ktx ${builddir}/testimages/conftestimage_R11_EAC.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/conftestimage_RG11_EAC.ktx ${builddir}/testimages/conftestimage_RG11_EAC.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/conftestimage_SIGNED_R11_EAC.ktx ${builddir}/testimages/conftestimage_SIGNED_R11_EAC.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/conftestimage_SIGNED_RG11_EAC.ktx ${builddir}/testimages/conftestimage_SIGNED_RG11_EAC.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/cubemap_space.ktx ${builddir}/testimages/cubemap_space.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/cubemap_vulkan.ktx ${builddir}/testimages/cubemap_vulkan.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/cubemap_yokohama.ktx ${builddir}/testimages/cubemap_yokohama.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/etc1.ktx ${builddir}/testimages/etc1.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/etc2-rgba1.ktx ${builddir}/testimages/etc2-rgba1.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/etc2-rgba8.ktx ${builddir}/testimages/etc2-rgba8.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/etc2-rgb.ktx ${builddir}/testimages/etc2-rgb.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/etc2-sRGBa1.ktx ${builddir}/testimages/etc2-sRGBa1.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/etc2-sRGBa8.ktx ${builddir}/testimages/etc2-sRGBa8.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/etc2-sRGB.ktx ${builddir}/testimages/etc2-sRGB.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/hi_mark.ktx ${builddir}/testimages/hi_mark.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/hi_mark_sq.ktx ${builddir}/testimages/hi_mark_sq.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/luminance_sized_reference.ktx ${builddir}/testimages/luminance_sized_reference.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/luminance_unsized_reference.ktx ${builddir}/testimages/luminance_unsized_reference.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/no-npot.ktx ${builddir}/testimages/no-npot.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/orient-down-metadata.ktx ${builddir}/testimages/orient-down-metadata.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/orient-down-metadata-sized.ktx ${builddir}/testimages/orient-down-metadata-sized.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/orient-up.ktx ${builddir}/testimages/orient-up.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/orient-up-metadata-sized.ktx ${builddir}/testimages/orient-up-metadata-sized.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/orient-up-sized.ktx ${builddir}/testimages/orient-up-sized.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/pattern_02_bc2.ktx ${builddir}/testimages/pattern_02_bc2.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/rgb-amg-reference.ktx ${builddir}/testimages/rgb-amg-reference.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/rgba-reference.ktx ${builddir}/testimages/rgba-reference.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/rgb-mipmap-reference.ktx ${builddir}/testimages/rgb-mipmap-reference.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/rgb-reference.ktx ${builddir}/testimages/rgb-reference.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/texturearray_bc3.ktx ${builddir}/testimages/texturearray_bc3.ktx
DEPENDS ${testimages__copies__input}
WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
COMMENT Copying for testimages
VERBATIM
)
add_custom_target(testimages__copies
  DEPENDS ${testimages__copies__output}
  SOURCES ${testimages__copies__input}
)
add_custom_target(testimages SOURCES)
set_target_properties(testimages PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(testimages__copies PROPERTIES EXCLUDE_FROM_ALL "FALSE")
add_dependencies(testimages
  testimages__copies
)
unset(TOOLSET)
unset(TARGET)

#ktxtests.gyp:libsdl#target
set(TARGET "libsdl")
set(TOOLSET "target")
list(APPEND libsdl__copies__input
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_lib/linux/${configuration}-x64/libSDL2-2.0.so.0"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_lib/linux/${configuration}-x64/libSDL2-2.0.so.0.4.0")
list(APPEND libsdl__copies__output
  "${builddir}/libSDL2-2.0.so.0"
  "${builddir}/libSDL2-2.0.so.0.4.0")
add_custom_command(
OUTPUT ${libsdl__copies__output}
COMMAND ${CMAKE_COMMAND} -E copy other_lib/linux/${configuration}-x64/libSDL2-2.0.so.0 ${builddir}/libSDL2-2.0.so.0
COMMAND ${CMAKE_COMMAND} -E copy other_lib/linux/${configuration}-x64/libSDL2-2.0.so.0.4.0 ${builddir}/libSDL2-2.0.so.0.4.0
DEPENDS ${libsdl__copies__input}
WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
COMMENT Copying for libsdl
VERBATIM
)
add_custom_target(libsdl__copies
  DEPENDS ${libsdl__copies__output}
  SOURCES ${libsdl__copies__input}
)
add_custom_target(libsdl SOURCES)
set_target_properties(libsdl PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(libsdl__copies PROPERTIES EXCLUDE_FROM_ALL "FALSE")
add_dependencies(libsdl
  libsdl__copies
)
unset(TOOLSET)
unset(TARGET)

#ktxtests.gyp:appfwSDL#target
set(TARGET "appfwSDL")
set(TOOLSET "target")
set(appfwSDL__frag2spirv__input "")
set(RULE_INPUT_PATH "tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.frag")
set(RULE_INPUT_DIRNAME "tests/loadtests/appfwSDL/VulkanAppSDL/shaders")
set(RULE_INPUT_NAME "textoverlay.frag")
set(RULE_INPUT_ROOT "textoverlay")
set(RULE_INPUT_EXT ".frag")
set(appfwSDL__frag2spirv_0_0 "${builddir}/shaders/${RULE_INPUT_NAME}.spv")
add_custom_command(OUTPUT
  ${appfwSDL__frag2spirv_0_0}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${builddir}/shaders
  COMMAND glslangValidator -V -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
  DEPENDS ${appfwSDL__frag2spirv__input} ../../../../tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.frag
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT Compiling ${RULE_INPUT_NAME}.
  VERBATIM
)
unset(RULE_INPUT_PATH)
unset(RULE_INPUT_DIRNAME)
unset(RULE_INPUT_NAME)
unset(RULE_INPUT_ROOT)
unset(RULE_INPUT_EXT)
add_custom_target(appfwSDL__frag2spirv DEPENDS
  ${appfwSDL__frag2spirv_0_0}
SOURCES ${appfwSDL__frag2spirv__input}
  ../../../../tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.frag
)
set(appfwSDL__vert2spirv__input "")
set(RULE_INPUT_PATH "tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.vert")
set(RULE_INPUT_DIRNAME "tests/loadtests/appfwSDL/VulkanAppSDL/shaders")
set(RULE_INPUT_NAME "textoverlay.vert")
set(RULE_INPUT_ROOT "textoverlay")
set(RULE_INPUT_EXT ".vert")
set(appfwSDL__vert2spirv_0_0 "${builddir}/shaders/${RULE_INPUT_NAME}.spv")
add_custom_command(OUTPUT
  ${appfwSDL__vert2spirv_0_0}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${builddir}/shaders
  COMMAND glslangValidator -V -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
  DEPENDS ${appfwSDL__vert2spirv__input} ../../../../tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.vert
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT Compiling ${RULE_INPUT_NAME}.
  VERBATIM
)
unset(RULE_INPUT_PATH)
unset(RULE_INPUT_DIRNAME)
unset(RULE_INPUT_NAME)
unset(RULE_INPUT_ROOT)
unset(RULE_INPUT_EXT)
add_custom_target(appfwSDL__vert2spirv DEPENDS
  ${appfwSDL__vert2spirv_0_0}
SOURCES ${appfwSDL__vert2spirv__input}
  ../../../../tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.vert
)
set(appfwSDL__c_srcs "../../../../tests/loadtests/appfwSDL/VulkanAppSDL/SDL_vulkan.c")
list(APPEND appfwSDL__cxx_srcs
  "../../../../tests/loadtests/appfwSDL/main.cpp"
  "../../../../tests/loadtests/appfwSDL/AppBaseSDL.cpp"
  "../../../../tests/loadtests/appfwSDL/GLAppSDL.cpp"
  "../../../../tests/loadtests/appfwSDL/VulkanAppSDL/VulkanAppSDL.cpp"
  "../../../../tests/loadtests/appfwSDL/VulkanAppSDL/VulkanContext.cpp"
  "../../../../tests/loadtests/appfwSDL/VulkanAppSDL/VulkanSwapchain.cpp"
  "../../../../tests/loadtests/appfwSDL/VulkanAppSDL/vulkandebug.cpp"
  "../../../../tests/loadtests/appfwSDL/VulkanAppSDL/vulkantools.cpp")
list(APPEND appfwSDL__other_srcs
  "../../../../tests/loadtests/appfwSDL/AppBaseSDL.h"
  "../../../../tests/loadtests/appfwSDL/GLAppSDL.h"
  "../../../../tests/loadtests/appfwSDL/VulkanAppSDL/SDL_vulkan.h"
  "../../../../tests/loadtests/appfwSDL/VulkanAppSDL/VulkanAppSDL.h"
  "../../../../tests/loadtests/appfwSDL/VulkanAppSDL/vulkancheckres.h"
  "../../../../tests/loadtests/appfwSDL/VulkanAppSDL/VulkanContext.h"
  "../../../../tests/loadtests/appfwSDL/VulkanAppSDL/VulkanSwapchain.h"
  "../../../../tests/loadtests/appfwSDL/VulkanAppSDL/vulkandebug.h"
  "../../../../tests/loadtests/appfwSDL/VulkanAppSDL/vulkantextoverlay.hpp"
  "../../../../tests/loadtests/appfwSDL/VulkanAppSDL/vulkantools.h"
  "../../../../tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.frag"
  "../../../../tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.vert")
add_library(appfwSDL STATIC ${appfwSDL__c_srcs} ${appfwSDL__cxx_srcs} ${appfwSDL__other_srcs})
set_target_properties(appfwSDL PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(appfwSDL__frag2spirv PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(appfwSDL__vert2spirv PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_source_files_properties(${appfwSDL__other_srcs} PROPERTIES HEADER_FILE_ONLY "TRUE")
set_target_properties(appfwSDL PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${obj}.${TOOLSET}/")
set_target_properties(appfwSDL PROPERTIES PREFIX "lib")
set_target_properties(appfwSDL PROPERTIES ARCHIVE_OUTPUT_NAME "appfwSDL")
set_target_properties(appfwSDL PROPERTIES SUFFIX ".a")
set_source_files_properties(${obj}.${TOOLSET}/libappfwSDL.a PROPERTIES GENERATED "TRUE")
list(APPEND appfwSDL__include_dirs
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/appfwSDL"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/appfwSDL/VulkanAppSDL"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include/SDL2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include"
  "/$ENV{VULKAN_SDK}/include")
set_property(TARGET appfwSDL APPEND PROPERTY INCLUDE_DIRECTORIES ${appfwSDL__include_dirs})
set_target_properties(appfwSDL PROPERTIES COMPILE_DEFINITIONS "NDEBUG;")
set_source_files_properties(${appfwSDL__c_srcs} PROPERTIES COMPILE_FLAGS "-O3 ")
set_source_files_properties(${appfwSDL__cxx_srcs} PROPERTIES COMPILE_FLAGS "-O3 -std=c++11 ")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
add_dependencies(appfwSDL__frag2spirv
  libsdl
  vulkan_headers
)
add_dependencies(appfwSDL__vert2spirv
  libsdl
  vulkan_headers
)
add_dependencies(appfwSDL
  libsdl
  vulkan_headers
  appfwSDL__frag2spirv
  appfwSDL__vert2spirv
)
unset(TOOLSET)
unset(TARGET)

#ktxtests.gyp:gl3loadtests#target
set(TARGET "gl3loadtests")
set(TOOLSET "target")
list(APPEND gl3loadtests__c_srcs
  "../../../../tests/loadtests/common/at.c"
  "../../../../tests/loadtests/glloadtests/shader-based/sample_01_draw_texture.c"
  "../../../../tests/loadtests/glloadtests/shader-based/sample_02_cube_textured.c"
  "../../../../tests/loadtests/glloadtests/shader-based/shaderfuncs.c"
  "../../../../tests/loadtests/glloadtests/shader-based/shaders.c")
list(APPEND gl3loadtests__cxx_srcs
  "../../../../tests/loadtests/glloadtests/LoadTests.cpp"
  "../../../../tests/loadtests/glloadtests/shader-based/LoadTestsGL3.cpp")
list(APPEND gl3loadtests__other_srcs
  "../../../../tests/loadtests/common/at.h"
  "../../../../tests/loadtests/geom/cube.h"
  "../../../../tests/loadtests/geom/frame.h"
  "../../../../tests/loadtests/glloadtests/LoadTests.h"
  "../../../../tests/loadtests/glloadtests/shader-based/mygl.h")
link_directories( ../../../../other_lib/linux/${configuration}-x64
)
add_executable(gl3loadtests ${gl3loadtests__c_srcs} ${gl3loadtests__cxx_srcs} ${gl3loadtests__other_srcs})
set_target_properties(gl3loadtests PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_source_files_properties(${gl3loadtests__other_srcs} PROPERTIES HEADER_FILE_ONLY "TRUE")
set_target_properties(gl3loadtests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${builddir}")
set_target_properties(gl3loadtests PROPERTIES PREFIX "")
set_target_properties(gl3loadtests PROPERTIES RUNTIME_OUTPUT_NAME "gl3loadtests")
set_target_properties(gl3loadtests PROPERTIES SUFFIX "")
set_source_files_properties(${builddir}/gl3loadtests PROPERTIES GENERATED "TRUE")
list(APPEND gl3loadtests__include_dirs
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/glloadtests"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/common"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/geom"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/appfwSDL"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/appfwSDL/VulkanAppSDL"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include")
set_property(TARGET gl3loadtests APPEND PROPERTY INCLUDE_DIRECTORIES ${gl3loadtests__include_dirs})
set_target_properties(gl3loadtests PROPERTIES COMPILE_DEFINITIONS "GL_CONTEXT_PROFILE=SDL_GL_CONTEXT_PROFILE_CORE;GL_CONTEXT_MAJOR_VERSION=3;GL_CONTEXT_MINOR_VERSION=3;NDEBUG;")
set_target_properties(gl3loadtests PROPERTIES COMPILE_FLAGS "-O3 ")
set_target_properties(gl3loadtests PROPERTIES LINK_FLAGS "-Wl,-rpath,. ")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "YES")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.khronos.${PRODUCT_NAME}")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_INFOPLIST_FILE "tests/loadtests/glloadtests/resources_mac/Info.plist")
add_dependencies(gl3loadtests
  libgl
  testimages
  libsdl
  vulkan_headers
)
target_link_libraries(gl3loadtests
  appfwSDL
  libktx_gl
  "-lX11-xcb"
  "-lSDL2-2.0"
  "-lSDL2main"
  "-ldl"
  "-lpthread"
  "-lGL"
)
unset(TOOLSET)
unset(TARGET)

#ktxtests.gyp:libassimp#target
set(TARGET "libassimp")
set(TOOLSET "target")
add_custom_target(libassimp SOURCES)
set_target_properties(libassimp PROPERTIES EXCLUDE_FROM_ALL "FALSE")
unset(TOOLSET)
unset(TARGET)

#ktxtests.gyp:vkloadtests#target
set(TARGET "vkloadtests")
set(TOOLSET "target")
set(vkloadtests__frag2spirv__input "")
set(RULE_INPUT_PATH "tests/loadtests/vkloadtests/shaders/cube/cube.frag")
set(RULE_INPUT_DIRNAME "tests/loadtests/vkloadtests/shaders/cube")
set(RULE_INPUT_NAME "cube.frag")
set(RULE_INPUT_ROOT "cube")
set(RULE_INPUT_EXT ".frag")
set(vkloadtests__frag2spirv_0_0 "${builddir}/shaders/${RULE_INPUT_NAME}.spv")
add_custom_command(OUTPUT
  ${vkloadtests__frag2spirv_0_0}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${builddir}/shaders
  COMMAND glslangValidator -V -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
  DEPENDS ${vkloadtests__frag2spirv__input} ../../../../tests/loadtests/vkloadtests/shaders/cube/cube.frag
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT Compiling ${RULE_INPUT_NAME}.
  VERBATIM
)
unset(RULE_INPUT_PATH)
unset(RULE_INPUT_DIRNAME)
unset(RULE_INPUT_NAME)
unset(RULE_INPUT_ROOT)
unset(RULE_INPUT_EXT)
set(RULE_INPUT_PATH "tests/loadtests/vkloadtests/shaders/cubemap/reflect.frag")
set(RULE_INPUT_DIRNAME "tests/loadtests/vkloadtests/shaders/cubemap")
set(RULE_INPUT_NAME "reflect.frag")
set(RULE_INPUT_ROOT "reflect")
set(RULE_INPUT_EXT ".frag")
set(vkloadtests__frag2spirv_1_0 "${builddir}/shaders/${RULE_INPUT_NAME}.spv")
add_custom_command(OUTPUT
  ${vkloadtests__frag2spirv_1_0}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${builddir}/shaders
  COMMAND glslangValidator -V -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
  DEPENDS ${vkloadtests__frag2spirv__input} ../../../../tests/loadtests/vkloadtests/shaders/cubemap/reflect.frag
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT Compiling ${RULE_INPUT_NAME}.
  VERBATIM
)
unset(RULE_INPUT_PATH)
unset(RULE_INPUT_DIRNAME)
unset(RULE_INPUT_NAME)
unset(RULE_INPUT_ROOT)
unset(RULE_INPUT_EXT)
set(RULE_INPUT_PATH "tests/loadtests/vkloadtests/shaders/cubemap/skybox.frag")
set(RULE_INPUT_DIRNAME "tests/loadtests/vkloadtests/shaders/cubemap")
set(RULE_INPUT_NAME "skybox.frag")
set(RULE_INPUT_ROOT "skybox")
set(RULE_INPUT_EXT ".frag")
set(vkloadtests__frag2spirv_2_0 "${builddir}/shaders/${RULE_INPUT_NAME}.spv")
add_custom_command(OUTPUT
  ${vkloadtests__frag2spirv_2_0}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${builddir}/shaders
  COMMAND glslangValidator -V -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
  DEPENDS ${vkloadtests__frag2spirv__input} ../../../../tests/loadtests/vkloadtests/shaders/cubemap/skybox.frag
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT Compiling ${RULE_INPUT_NAME}.
  VERBATIM
)
unset(RULE_INPUT_PATH)
unset(RULE_INPUT_DIRNAME)
unset(RULE_INPUT_NAME)
unset(RULE_INPUT_ROOT)
unset(RULE_INPUT_EXT)
set(RULE_INPUT_PATH "tests/loadtests/vkloadtests/shaders/texture/texture.frag")
set(RULE_INPUT_DIRNAME "tests/loadtests/vkloadtests/shaders/texture")
set(RULE_INPUT_NAME "texture.frag")
set(RULE_INPUT_ROOT "texture")
set(RULE_INPUT_EXT ".frag")
set(vkloadtests__frag2spirv_3_0 "${builddir}/shaders/${RULE_INPUT_NAME}.spv")
add_custom_command(OUTPUT
  ${vkloadtests__frag2spirv_3_0}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${builddir}/shaders
  COMMAND glslangValidator -V -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
  DEPENDS ${vkloadtests__frag2spirv__input} ../../../../tests/loadtests/vkloadtests/shaders/texture/texture.frag
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT Compiling ${RULE_INPUT_NAME}.
  VERBATIM
)
unset(RULE_INPUT_PATH)
unset(RULE_INPUT_DIRNAME)
unset(RULE_INPUT_NAME)
unset(RULE_INPUT_ROOT)
unset(RULE_INPUT_EXT)
set(RULE_INPUT_PATH "tests/loadtests/vkloadtests/shaders/texturearray/instancing.frag")
set(RULE_INPUT_DIRNAME "tests/loadtests/vkloadtests/shaders/texturearray")
set(RULE_INPUT_NAME "instancing.frag")
set(RULE_INPUT_ROOT "instancing")
set(RULE_INPUT_EXT ".frag")
set(vkloadtests__frag2spirv_4_0 "${builddir}/shaders/${RULE_INPUT_NAME}.spv")
add_custom_command(OUTPUT
  ${vkloadtests__frag2spirv_4_0}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${builddir}/shaders
  COMMAND glslangValidator -V -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
  DEPENDS ${vkloadtests__frag2spirv__input} ../../../../tests/loadtests/vkloadtests/shaders/texturearray/instancing.frag
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT Compiling ${RULE_INPUT_NAME}.
  VERBATIM
)
unset(RULE_INPUT_PATH)
unset(RULE_INPUT_DIRNAME)
unset(RULE_INPUT_NAME)
unset(RULE_INPUT_ROOT)
unset(RULE_INPUT_EXT)
add_custom_target(vkloadtests__frag2spirv DEPENDS
  ${vkloadtests__frag2spirv_0_0}
  ${vkloadtests__frag2spirv_1_0}
  ${vkloadtests__frag2spirv_2_0}
  ${vkloadtests__frag2spirv_3_0}
  ${vkloadtests__frag2spirv_4_0}
SOURCES ${vkloadtests__frag2spirv__input}
  ../../../../tests/loadtests/vkloadtests/shaders/cube/cube.frag
  ../../../../tests/loadtests/vkloadtests/shaders/cubemap/reflect.frag
  ../../../../tests/loadtests/vkloadtests/shaders/cubemap/skybox.frag
  ../../../../tests/loadtests/vkloadtests/shaders/texture/texture.frag
  ../../../../tests/loadtests/vkloadtests/shaders/texturearray/instancing.frag
)
set(vkloadtests__vert2spirv__input "")
set(RULE_INPUT_PATH "tests/loadtests/vkloadtests/shaders/cube/cube.vert")
set(RULE_INPUT_DIRNAME "tests/loadtests/vkloadtests/shaders/cube")
set(RULE_INPUT_NAME "cube.vert")
set(RULE_INPUT_ROOT "cube")
set(RULE_INPUT_EXT ".vert")
set(vkloadtests__vert2spirv_0_0 "${builddir}/shaders/${RULE_INPUT_NAME}.spv")
add_custom_command(OUTPUT
  ${vkloadtests__vert2spirv_0_0}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${builddir}/shaders
  COMMAND glslangValidator -V -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
  DEPENDS ${vkloadtests__vert2spirv__input} ../../../../tests/loadtests/vkloadtests/shaders/cube/cube.vert
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT Compiling ${RULE_INPUT_NAME}.
  VERBATIM
)
unset(RULE_INPUT_PATH)
unset(RULE_INPUT_DIRNAME)
unset(RULE_INPUT_NAME)
unset(RULE_INPUT_ROOT)
unset(RULE_INPUT_EXT)
set(RULE_INPUT_PATH "tests/loadtests/vkloadtests/shaders/cubemap/reflect.vert")
set(RULE_INPUT_DIRNAME "tests/loadtests/vkloadtests/shaders/cubemap")
set(RULE_INPUT_NAME "reflect.vert")
set(RULE_INPUT_ROOT "reflect")
set(RULE_INPUT_EXT ".vert")
set(vkloadtests__vert2spirv_1_0 "${builddir}/shaders/${RULE_INPUT_NAME}.spv")
add_custom_command(OUTPUT
  ${vkloadtests__vert2spirv_1_0}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${builddir}/shaders
  COMMAND glslangValidator -V -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
  DEPENDS ${vkloadtests__vert2spirv__input} ../../../../tests/loadtests/vkloadtests/shaders/cubemap/reflect.vert
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT Compiling ${RULE_INPUT_NAME}.
  VERBATIM
)
unset(RULE_INPUT_PATH)
unset(RULE_INPUT_DIRNAME)
unset(RULE_INPUT_NAME)
unset(RULE_INPUT_ROOT)
unset(RULE_INPUT_EXT)
set(RULE_INPUT_PATH "tests/loadtests/vkloadtests/shaders/cubemap/skybox.vert")
set(RULE_INPUT_DIRNAME "tests/loadtests/vkloadtests/shaders/cubemap")
set(RULE_INPUT_NAME "skybox.vert")
set(RULE_INPUT_ROOT "skybox")
set(RULE_INPUT_EXT ".vert")
set(vkloadtests__vert2spirv_2_0 "${builddir}/shaders/${RULE_INPUT_NAME}.spv")
add_custom_command(OUTPUT
  ${vkloadtests__vert2spirv_2_0}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${builddir}/shaders
  COMMAND glslangValidator -V -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
  DEPENDS ${vkloadtests__vert2spirv__input} ../../../../tests/loadtests/vkloadtests/shaders/cubemap/skybox.vert
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT Compiling ${RULE_INPUT_NAME}.
  VERBATIM
)
unset(RULE_INPUT_PATH)
unset(RULE_INPUT_DIRNAME)
unset(RULE_INPUT_NAME)
unset(RULE_INPUT_ROOT)
unset(RULE_INPUT_EXT)
set(RULE_INPUT_PATH "tests/loadtests/vkloadtests/shaders/texture/texture.vert")
set(RULE_INPUT_DIRNAME "tests/loadtests/vkloadtests/shaders/texture")
set(RULE_INPUT_NAME "texture.vert")
set(RULE_INPUT_ROOT "texture")
set(RULE_INPUT_EXT ".vert")
set(vkloadtests__vert2spirv_3_0 "${builddir}/shaders/${RULE_INPUT_NAME}.spv")
add_custom_command(OUTPUT
  ${vkloadtests__vert2spirv_3_0}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${builddir}/shaders
  COMMAND glslangValidator -V -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
  DEPENDS ${vkloadtests__vert2spirv__input} ../../../../tests/loadtests/vkloadtests/shaders/texture/texture.vert
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT Compiling ${RULE_INPUT_NAME}.
  VERBATIM
)
unset(RULE_INPUT_PATH)
unset(RULE_INPUT_DIRNAME)
unset(RULE_INPUT_NAME)
unset(RULE_INPUT_ROOT)
unset(RULE_INPUT_EXT)
set(RULE_INPUT_PATH "tests/loadtests/vkloadtests/shaders/texturearray/instancing.vert")
set(RULE_INPUT_DIRNAME "tests/loadtests/vkloadtests/shaders/texturearray")
set(RULE_INPUT_NAME "instancing.vert")
set(RULE_INPUT_ROOT "instancing")
set(RULE_INPUT_EXT ".vert")
set(vkloadtests__vert2spirv_4_0 "${builddir}/shaders/${RULE_INPUT_NAME}.spv")
add_custom_command(OUTPUT
  ${vkloadtests__vert2spirv_4_0}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${builddir}/shaders
  COMMAND glslangValidator -V -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
  DEPENDS ${vkloadtests__vert2spirv__input} ../../../../tests/loadtests/vkloadtests/shaders/texturearray/instancing.vert
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT Compiling ${RULE_INPUT_NAME}.
  VERBATIM
)
unset(RULE_INPUT_PATH)
unset(RULE_INPUT_DIRNAME)
unset(RULE_INPUT_NAME)
unset(RULE_INPUT_ROOT)
unset(RULE_INPUT_EXT)
add_custom_target(vkloadtests__vert2spirv DEPENDS
  ${vkloadtests__vert2spirv_0_0}
  ${vkloadtests__vert2spirv_1_0}
  ${vkloadtests__vert2spirv_2_0}
  ${vkloadtests__vert2spirv_3_0}
  ${vkloadtests__vert2spirv_4_0}
SOURCES ${vkloadtests__vert2spirv__input}
  ../../../../tests/loadtests/vkloadtests/shaders/cube/cube.vert
  ../../../../tests/loadtests/vkloadtests/shaders/cubemap/reflect.vert
  ../../../../tests/loadtests/vkloadtests/shaders/cubemap/skybox.vert
  ../../../../tests/loadtests/vkloadtests/shaders/texture/texture.vert
  ../../../../tests/loadtests/vkloadtests/shaders/texturearray/instancing.vert
)
list(APPEND vkloadtests__copies__input
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/vkloadtests/models/cube.obj"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/vkloadtests/models/sphere.obj"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/vkloadtests/models/teapot.dae"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/vkloadtests/models/torusknot.obj")
list(APPEND vkloadtests__copies__output
  "${builddir}/models/cube.obj"
  "${builddir}/models/sphere.obj"
  "${builddir}/models/teapot.dae"
  "${builddir}/models/torusknot.obj")
add_custom_command(
OUTPUT ${vkloadtests__copies__output}
COMMAND ${CMAKE_COMMAND} -E copy tests/loadtests/vkloadtests/models/cube.obj ${builddir}/models/cube.obj
COMMAND ${CMAKE_COMMAND} -E copy tests/loadtests/vkloadtests/models/sphere.obj ${builddir}/models/sphere.obj
COMMAND ${CMAKE_COMMAND} -E copy tests/loadtests/vkloadtests/models/teapot.dae ${builddir}/models/teapot.dae
COMMAND ${CMAKE_COMMAND} -E copy tests/loadtests/vkloadtests/models/torusknot.obj ${builddir}/models/torusknot.obj
DEPENDS ${vkloadtests__copies__input}
WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
COMMENT Copying for vkloadtests
VERBATIM
)
add_custom_target(vkloadtests__copies
  DEPENDS ${vkloadtests__copies__output}
  SOURCES ${vkloadtests__copies__input}
)
list(APPEND vkloadtests__cxx_srcs
  "../../../../tests/loadtests/vkloadtests/Texture.cpp"
  "../../../../tests/loadtests/vkloadtests/TextureArray.cpp"
  "../../../../tests/loadtests/vkloadtests/TextureCubemap.cpp"
  "../../../../tests/loadtests/vkloadtests/TexturedCube.cpp"
  "../../../../tests/loadtests/vkloadtests/VulkanLoadTests.cpp"
  "../../../../tests/loadtests/vkloadtests/VulkanLoadTestSample.cpp")
list(APPEND vkloadtests__other_srcs
  "../../../../tests/loadtests/common/vecmath.hpp"
  "../../../../tests/loadtests/vkloadtests/Texture.h"
  "../../../../tests/loadtests/vkloadtests/TextureArray.h"
  "../../../../tests/loadtests/vkloadtests/TextureCubemap.h"
  "../../../../tests/loadtests/vkloadtests/TexturedCube.h"
  "../../../../tests/loadtests/vkloadtests/shaders/cube/cube.frag"
  "../../../../tests/loadtests/vkloadtests/shaders/cube/cube.vert"
  "../../../../tests/loadtests/vkloadtests/shaders/cubemap/reflect.frag"
  "../../../../tests/loadtests/vkloadtests/shaders/cubemap/reflect.vert"
  "../../../../tests/loadtests/vkloadtests/shaders/cubemap/skybox.frag"
  "../../../../tests/loadtests/vkloadtests/shaders/cubemap/skybox.vert"
  "../../../../tests/loadtests/vkloadtests/shaders/texture/texture.frag"
  "../../../../tests/loadtests/vkloadtests/shaders/texture/texture.vert"
  "../../../../tests/loadtests/vkloadtests/shaders/texturearray/instancing.frag"
  "../../../../tests/loadtests/vkloadtests/shaders/texturearray/instancing.vert"
  "../../../../tests/loadtests/vkloadtests/utils/VulkanMeshLoader.hpp"
  "../../../../tests/loadtests/vkloadtests/VulkanLoadTests.h"
  "../../../../tests/loadtests/vkloadtests/VulkanLoadTestSample.h")
link_directories( ../../../../other_lib/linux/${configuration}-x64
)
add_executable(vkloadtests ${vkloadtests__cxx_srcs} ${vkloadtests__other_srcs})
set_target_properties(vkloadtests PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(vkloadtests__frag2spirv PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(vkloadtests__vert2spirv PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(vkloadtests__copies PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_source_files_properties(${vkloadtests__other_srcs} PROPERTIES HEADER_FILE_ONLY "TRUE")
set_target_properties(vkloadtests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${builddir}")
set_target_properties(vkloadtests PROPERTIES PREFIX "")
set_target_properties(vkloadtests PROPERTIES RUNTIME_OUTPUT_NAME "vkloadtests")
set_target_properties(vkloadtests PROPERTIES SUFFIX "")
set_source_files_properties(${builddir}/vkloadtests PROPERTIES GENERATED "TRUE")
list(APPEND vkloadtests__include_dirs
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/common"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/geom"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/appfwSDL"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/appfwSDL/VulkanAppSDL"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include"
  "/$ENV{VULKAN_SDK}/include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include")
set_property(TARGET vkloadtests APPEND PROPERTY INCLUDE_DIRECTORIES ${vkloadtests__include_dirs})
set_target_properties(vkloadtests PROPERTIES COMPILE_DEFINITIONS "NDEBUG;")
set_target_properties(vkloadtests PROPERTIES COMPILE_FLAGS "-std=c++11 -O3 ")
set_target_properties(vkloadtests PROPERTIES LINK_FLAGS "-Wl,-rpath,. ")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "YES")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_OTHER_LDFLAGS "-lassimp -lz")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.khronos.${PRODUCT_NAME}")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_INFOPLIST_FILE "tests/loadtests/glloadtests/resources_mac/Info.plist")
add_dependencies(vkloadtests__frag2spirv
  appfwSDL
  libktx_gl
  libassimp
  libvulkan
  testimages
  libsdl
  vulkan_headers
)
add_dependencies(vkloadtests__vert2spirv
  appfwSDL
  libktx_gl
  libassimp
  libvulkan
  testimages
  libsdl
  vulkan_headers
)
add_dependencies(vkloadtests__copies
  appfwSDL
  libktx_gl
  libassimp
  libvulkan
  testimages
  libsdl
  vulkan_headers
)
add_dependencies(vkloadtests
  libassimp
  libvulkan
  testimages
  libsdl
  vulkan_headers
  vkloadtests__frag2spirv
  vkloadtests__vert2spirv
  vkloadtests__copies
)
target_link_libraries(vkloadtests
  appfwSDL
  libktx_gl
  "-lpthread"
  "-lX11-xcb"
  "-lSDL2-2.0"
  "-lSDL2main"
  "-ldl"
  "-lassimp"
  "-lvulkan"
)
unset(TOOLSET)
unset(TARGET)

#ktxtests.gyp:gtest_main#target
set(TARGET "gtest_main")
set(TOOLSET "target")
set(gtest_main__cxx_srcs "../../../../tests/gtest/src/gtest_main.cc")
add_library(gtest_main STATIC ${gtest_main__cxx_srcs})
set_target_properties(gtest_main PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(gtest_main PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${obj}.${TOOLSET}/")
set_target_properties(gtest_main PROPERTIES PREFIX "lib")
set_target_properties(gtest_main PROPERTIES ARCHIVE_OUTPUT_NAME "gtest_main")
set_target_properties(gtest_main PROPERTIES SUFFIX ".a")
set_source_files_properties(${obj}.${TOOLSET}/libgtest_main.a PROPERTIES GENERATED "TRUE")
list(APPEND gtest_main__include_dirs
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/gtest/include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/gtest")
set_property(TARGET gtest_main APPEND PROPERTY INCLUDE_DIRECTORIES ${gtest_main__include_dirs})
set_target_properties(gtest_main PROPERTIES COMPILE_DEFINITIONS "NDEBUG;")
set_target_properties(gtest_main PROPERTIES COMPILE_FLAGS "-O3 ")
set_target_properties(gtest_main PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(gtest_main PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(gtest_main PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(gtest_main PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(gtest_main PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
unset(TOOLSET)
unset(TARGET)

#ktxtests.gyp:gtest#target
set(TARGET "gtest")
set(TOOLSET "target")
set(gtest__cxx_srcs "../../../../tests/gtest/src/gtest-all.cc")
add_library(gtest STATIC ${gtest__cxx_srcs})
set_target_properties(gtest PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(gtest PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${obj}.${TOOLSET}/")
set_target_properties(gtest PROPERTIES PREFIX "lib")
set_target_properties(gtest PROPERTIES ARCHIVE_OUTPUT_NAME "gtest")
set_target_properties(gtest PROPERTIES SUFFIX ".a")
set_source_files_properties(${obj}.${TOOLSET}/libgtest.a PROPERTIES GENERATED "TRUE")
list(APPEND gtest__include_dirs
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/gtest/include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/gtest")
set_property(TARGET gtest APPEND PROPERTY INCLUDE_DIRECTORIES ${gtest__include_dirs})
set_target_properties(gtest PROPERTIES COMPILE_DEFINITIONS "GTEST_HAS_PTHREAD=0;_HAS_EXCEPTIONS=1;NDEBUG;")
set_target_properties(gtest PROPERTIES COMPILE_FLAGS "-O3 ")
set_target_properties(gtest PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(gtest PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(gtest PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(gtest PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(gtest PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
unset(TOOLSET)
unset(TARGET)

#ktxtests.gyp:unittests#target
set(TARGET "unittests")
set(TOOLSET "target")
set(unittests__cxx_srcs "../../../../tests/unittests/unittests.cc")
add_executable(unittests ${unittests__cxx_srcs})
set_target_properties(unittests PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(unittests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${builddir}")
set_target_properties(unittests PROPERTIES PREFIX "")
set_target_properties(unittests PROPERTIES RUNTIME_OUTPUT_NAME "unittests")
set_target_properties(unittests PROPERTIES SUFFIX "")
set_source_files_properties(${builddir}/unittests PROPERTIES GENERATED "TRUE")
list(APPEND unittests__include_dirs
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/gtest/include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include")
set_property(TARGET unittests APPEND PROPERTY INCLUDE_DIRECTORIES ${unittests__include_dirs})
set_target_properties(unittests PROPERTIES COMPILE_DEFINITIONS "NDEBUG;")
set_target_properties(unittests PROPERTIES COMPILE_FLAGS "-O3 ")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "YES")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
target_link_libraries(unittests
-Wl,--start-group
  gtest
  gtest_main
-Wl,--end-group
  libktx_gl
)
unset(TOOLSET)
unset(TARGET)

#ktxtests.gyp:readtests#target
set(TARGET "readtests")
set(TOOLSET "target")
set(readtests__cxx_srcs "../../../../tests/readtests/readtests.cc")
add_executable(readtests ${readtests__cxx_srcs})
set_target_properties(readtests PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(readtests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${builddir}")
set_target_properties(readtests PROPERTIES PREFIX "")
set_target_properties(readtests PROPERTIES RUNTIME_OUTPUT_NAME "readtests")
set_target_properties(readtests PROPERTIES SUFFIX "")
set_source_files_properties(${builddir}/readtests PROPERTIES GENERATED "TRUE")
list(APPEND readtests__include_dirs
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/gtest/lib"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/gtest/include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include")
set_property(TARGET readtests APPEND PROPERTY INCLUDE_DIRECTORIES ${readtests__include_dirs})
set_target_properties(readtests PROPERTIES COMPILE_DEFINITIONS "NDEBUG;")
set_target_properties(readtests PROPERTIES COMPILE_FLAGS "-O3 ")
set_target_properties(readtests PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(readtests PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(readtests PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(readtests PROPERTIES XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "YES")
set_target_properties(readtests PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(readtests PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
target_link_libraries(readtests
-Wl,--start-group
  gtest
  gtest_main
-Wl,--end-group
  libktx_gl
)
unset(TOOLSET)
unset(TARGET)

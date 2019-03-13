cmake_minimum_required(VERSION 2.8.8 FATAL_ERROR)
cmake_policy(VERSION 2.8.8)
project(texturetests)
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
  "../../../../lib/filestream.c"
  "../../../../lib/glloader.c"
  "../../../../lib/hashlist.c"
  "../../../../lib/hashtable.c"
  "../../../../lib/memstream.c"
  "../../../../lib/swap.c"
  "../../../../lib/texture.c"
  "../../../../lib/writer.c"
  "../../../../lib/writer_v1.c"
  "../../../../lib/vkloader.c"
  "../../../../lib/vk_funcs.c")
list(APPEND libktx_es3__cxx_srcs
  "../../../../lib/etcdec.cxx"
  "../../../../lib/etcunpack.cxx")
list(APPEND libktx_es3__other_srcs
  "../../../../include/ktx.h"
  "../../../../lib/filestream.h"
  "../../../../lib/gl_format.h"
  "../../../../lib/gl_funcptrs.h"
  "../../../../lib/gles1_funcptrs.h"
  "../../../../lib/gles2_funcptrs.h"
  "../../../../lib/gles3_funcptrs.h"
  "../../../../lib/ktxgl.h"
  "../../../../lib/ktxint.h"
  "../../../../lib/memstream.h"
  "../../../../lib/stream.h"
  "../../../../lib/uthash.h"
  "../../../../include/ktxvulkan.h"
  "../../../../lib/vk_format.h"
  "../../../../lib/vk_funclist.inl"
  "../../../../lib/vk_funcs.h")
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
set_target_properties(libktx_es3 PROPERTIES COMPILE_FLAGS "-pedantic -std=c99 -O3 ")
set_target_properties(libktx_es3 PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(libktx_es3 PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(libktx_es3 PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(libktx_es3 PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(libktx_es3 PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(libktx_es3 PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
set_target_properties(libktx_es3 PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
add_dependencies(libktx_es3
  vulkan_headers
)
unset(TOOLSET)
unset(TARGET)

#libktx.gyp:libvulkan.lazy#target
set(TARGET "libvulkan.lazy")
set(TOOLSET "target")
add_custom_target(libvulkan_lazy SOURCES)
set_target_properties(libvulkan_lazy PROPERTIES EXCLUDE_FROM_ALL "TRUE")
unset(TOOLSET)
unset(TARGET)

#libktx.gyp:libktx.es1#target
set(TARGET "libktx.es1")
set(TOOLSET "target")
list(APPEND libktx_es1__c_srcs
  "../../../../lib/checkheader.c"
  "../../../../lib/errstr.c"
  "../../../../lib/filestream.c"
  "../../../../lib/glloader.c"
  "../../../../lib/hashlist.c"
  "../../../../lib/hashtable.c"
  "../../../../lib/memstream.c"
  "../../../../lib/swap.c"
  "../../../../lib/texture.c"
  "../../../../lib/writer.c"
  "../../../../lib/writer_v1.c")
list(APPEND libktx_es1__cxx_srcs
  "../../../../lib/etcdec.cxx"
  "../../../../lib/etcunpack.cxx")
list(APPEND libktx_es1__other_srcs
  "../../../../include/ktx.h"
  "../../../../lib/filestream.h"
  "../../../../lib/gl_format.h"
  "../../../../lib/gl_funcptrs.h"
  "../../../../lib/gles1_funcptrs.h"
  "../../../../lib/gles2_funcptrs.h"
  "../../../../lib/gles3_funcptrs.h"
  "../../../../lib/ktxgl.h"
  "../../../../lib/ktxint.h"
  "../../../../lib/memstream.h"
  "../../../../lib/stream.h"
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
set_target_properties(libktx_es1 PROPERTIES COMPILE_FLAGS "-pedantic -std=c99 -O3 ")
set_target_properties(libktx_es1 PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(libktx_es1 PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(libktx_es1 PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(libktx_es1 PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(libktx_es1 PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(libktx_es1 PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
set_target_properties(libktx_es1 PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
unset(TOOLSET)
unset(TARGET)

#libktx.gyp:libktx.doc#target
set(TARGET "libktx.doc")
set(TOOLSET "target")
list(APPEND libktx_doc__buildLibktxDoc__input
  "${CMAKE_CURRENT_LIST_DIR}/../../../../libktx.doxy"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../runDoxygen"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/mainpage.md"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../LICENSE.md"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../TODO.md"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include/ktx.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/checkheader.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/errstr.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/etcdec.cxx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/etcunpack.cxx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/filestream.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/filestream.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/gl_format.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/gl_funcptrs.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/gles1_funcptrs.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/gles2_funcptrs.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/gles3_funcptrs.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/glloader.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/hashlist.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/hashtable.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/ktxgl.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/ktxint.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/memstream.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/memstream.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/stream.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/swap.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/texture.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/uthash.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/writer.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/writer_v1.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include/ktxvulkan.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/vk_format.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/vkloader.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/vk_funclist.inl"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/vk_funcs.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/vk_funcs.h")
set(libktx_doc__buildLibktxDoc__output "${CMAKE_CURRENT_LIST_DIR}/../../../../build/docs/.libktx_gentimestamp")
add_custom_command(OUTPUT ${libktx_doc__buildLibktxDoc__output}
  COMMAND ${CMAKE_COMMAND} -E make_directory build/docs
  COMMAND ./runDoxygen -t build/docs/.libktx_gentimestamp -o build/docs/html libktx.doxy
  DEPENDS ${libktx_doc__buildLibktxDoc__input}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT Generating libktx documentation with Doxygen
  VERBATIM
)
add_custom_target(libktx_doc__buildLibktxDoc
  DEPENDS ${libktx_doc__buildLibktxDoc__output}
  SOURCES ${libktx_doc__buildLibktxDoc__input}
)
add_custom_target(libktx_doc SOURCES)
set_target_properties(libktx_doc PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(libktx_doc__buildLibktxDoc PROPERTIES EXCLUDE_FROM_ALL "FALSE")
add_dependencies(libktx_doc
  libktx_doc__buildLibktxDoc
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
  "../../../../lib/filestream.c"
  "../../../../lib/glloader.c"
  "../../../../lib/hashlist.c"
  "../../../../lib/hashtable.c"
  "../../../../lib/memstream.c"
  "../../../../lib/swap.c"
  "../../../../lib/texture.c"
  "../../../../lib/writer.c"
  "../../../../lib/writer_v1.c"
  "../../../../lib/vkloader.c"
  "../../../../lib/vk_funcs.c")
list(APPEND libktx_gl__cxx_srcs
  "../../../../lib/etcdec.cxx"
  "../../../../lib/etcunpack.cxx")
list(APPEND libktx_gl__other_srcs
  "../../../../include/ktx.h"
  "../../../../lib/filestream.h"
  "../../../../lib/gl_format.h"
  "../../../../lib/gl_funcptrs.h"
  "../../../../lib/gles1_funcptrs.h"
  "../../../../lib/gles2_funcptrs.h"
  "../../../../lib/gles3_funcptrs.h"
  "../../../../lib/ktxgl.h"
  "../../../../lib/ktxint.h"
  "../../../../lib/memstream.h"
  "../../../../lib/stream.h"
  "../../../../lib/uthash.h"
  "../../../../include/ktxvulkan.h"
  "../../../../lib/vk_format.h"
  "../../../../lib/vk_funclist.inl"
  "../../../../lib/vk_funcs.h")
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
set_target_properties(libktx_gl PROPERTIES COMPILE_DEFINITIONS "KTX_OPENGL=1;KTX_USE_FUNCPTRS_FOR_VULKAN;NDEBUG;")
set_target_properties(libktx_gl PROPERTIES COMPILE_FLAGS "-pedantic -std=c99 -O3 -fPIC ")
set_target_properties(libktx_gl PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(libktx_gl PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(libktx_gl PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(libktx_gl PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(libktx_gl PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(libktx_gl PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
set_target_properties(libktx_gl PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
add_dependencies(libktx_gl
  vulkan_headers
  libgl
)
target_link_libraries(libktx_gl
  "-lGL"
)
unset(TOOLSET)
unset(TARGET)

#ktxtools.gyp:toktx#target
set(TARGET "toktx")
set(TOOLSET "target")
list(APPEND toktx__cxx_srcs
  "../../../../utils/argparser.cpp"
  "../../../../tools/toktx/image.cpp"
  "../../../../tools/toktx/toktx.cpp")
list(APPEND toktx__other_srcs
  "../../../../utils/argparser.h"
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
  "${CMAKE_CURRENT_LIST_DIR}/../../../../utils"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include")
set_property(TARGET toktx APPEND PROPERTY INCLUDE_DIRECTORIES ${toktx__include_dirs})
set_target_properties(toktx PROPERTIES COMPILE_DEFINITIONS "NDEBUG;")
set_target_properties(toktx PROPERTIES COMPILE_FLAGS "-pedantic -O3 ")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "YES")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
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

#ktxtools.gyp:toktx.doc#target
set(TARGET "toktx.doc")
set(TOOLSET "target")
list(APPEND toktx_doc__buildToktxDoc__input
  "${CMAKE_CURRENT_LIST_DIR}/../../../../ktxtools.doxy"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../runDoxygen"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tools/toktx/toktx.cpp")
set(toktx_doc__buildToktxDoc__output "${CMAKE_CURRENT_LIST_DIR}/../../../../build/docs/.toktxdoc_gentimestamp")
add_custom_command(OUTPUT ${toktx_doc__buildToktxDoc__output}
  COMMAND ${CMAKE_COMMAND} -E make_directory build/docs
  COMMAND ./runDoxygen -t build/docs/.toktxdoc_gentimestamp -o build/docs/html ktxtools.doxy
  DEPENDS ${toktx_doc__buildToktxDoc__input}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT Generating toktx documentation with Doxygen
  VERBATIM
)
add_custom_target(toktx_doc__buildToktxDoc
  DEPENDS ${toktx_doc__buildToktxDoc__output}
  SOURCES ${toktx_doc__buildToktxDoc__input}
)
add_custom_target(toktx_doc SOURCES)
set_target_properties(toktx_doc PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(toktx_doc__buildToktxDoc PROPERTIES EXCLUDE_FROM_ALL "FALSE")
add_dependencies(toktx_doc
  toktx_doc__buildToktxDoc
)
unset(TOOLSET)
unset(TARGET)

#ktxdoc.gyp:ktxpkg.doc#target
set(TARGET "ktxpkg.doc")
set(TOOLSET "target")
list(APPEND ktxpkg_doc__buildPackageDoc__input
  "${CMAKE_CURRENT_LIST_DIR}/../../../../ktxpkg.doxy"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../LICENSE.md"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../runDoxygen"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../TODO.md"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../pkgdoc/mainpage.md")
set(ktxpkg_doc__buildPackageDoc__output "${CMAKE_CURRENT_LIST_DIR}/../../../../build/docs/.package_gentimestamp")
add_custom_command(OUTPUT ${ktxpkg_doc__buildPackageDoc__output}
  COMMAND ${CMAKE_COMMAND} -E make_directory build/docs
  COMMAND ./runDoxygen -t build/docs/.package_gentimestamp -o build/docs/html ktxpkg.doxy
  DEPENDS ${ktxpkg_doc__buildPackageDoc__input}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT Generating package documentation with Doxygen
  VERBATIM
)
add_custom_target(ktxpkg_doc__buildPackageDoc
  DEPENDS ${ktxpkg_doc__buildPackageDoc__output}
  SOURCES ${ktxpkg_doc__buildPackageDoc__input}
)
add_custom_target(ktxpkg_doc SOURCES)
set_target_properties(ktxpkg_doc PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(ktxpkg_doc__buildPackageDoc PROPERTIES EXCLUDE_FROM_ALL "FALSE")
add_dependencies(ktxpkg_doc__buildPackageDoc
  libktx_doc
  toktx_doc
)
add_dependencies(ktxpkg_doc
  libktx_doc
  toktx_doc
  ktxpkg_doc__buildPackageDoc
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
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/cubemap_yokohama_astc_8x8_unorm.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/cubemap_yokohama_bc3_unorm.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/cubemap_yokohama_etc2_unorm.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/etc1.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/etc2-rgb.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/etc2-rgba1.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/etc2-rgba8.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/etc2-sRGB.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/etc2-sRGBa1.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/etc2-sRGBa8.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/hi_mark.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/hi_mark_sq.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/luminance-reference-metadata.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/metalplate-amg-rgba8.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/no-npot.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/not4_rgb888_srgb.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/orient-down-metadata.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/orient-up-metadata.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/orient-up.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/pattern_02_bc2.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/rgb-amg-reference.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/rgb-mipmap-reference.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/rgb-reference-metadata.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/rgb-reference.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/rgba-reference.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/texturearray_astc_8x8_unorm.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/texturearray_bc3_unorm.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/texturearray_etc2_unorm.ktx")
list(APPEND testimages__copies__output
  "${builddir}/testimages/conftestimage_R11_EAC.ktx"
  "${builddir}/testimages/conftestimage_RG11_EAC.ktx"
  "${builddir}/testimages/conftestimage_SIGNED_R11_EAC.ktx"
  "${builddir}/testimages/conftestimage_SIGNED_RG11_EAC.ktx"
  "${builddir}/testimages/cubemap_yokohama_astc_8x8_unorm.ktx"
  "${builddir}/testimages/cubemap_yokohama_bc3_unorm.ktx"
  "${builddir}/testimages/cubemap_yokohama_etc2_unorm.ktx"
  "${builddir}/testimages/etc1.ktx"
  "${builddir}/testimages/etc2-rgb.ktx"
  "${builddir}/testimages/etc2-rgba1.ktx"
  "${builddir}/testimages/etc2-rgba8.ktx"
  "${builddir}/testimages/etc2-sRGB.ktx"
  "${builddir}/testimages/etc2-sRGBa1.ktx"
  "${builddir}/testimages/etc2-sRGBa8.ktx"
  "${builddir}/testimages/hi_mark.ktx"
  "${builddir}/testimages/hi_mark_sq.ktx"
  "${builddir}/testimages/luminance-reference-metadata.ktx"
  "${builddir}/testimages/metalplate-amg-rgba8.ktx"
  "${builddir}/testimages/no-npot.ktx"
  "${builddir}/testimages/not4_rgb888_srgb.ktx"
  "${builddir}/testimages/orient-down-metadata.ktx"
  "${builddir}/testimages/orient-up-metadata.ktx"
  "${builddir}/testimages/orient-up.ktx"
  "${builddir}/testimages/pattern_02_bc2.ktx"
  "${builddir}/testimages/rgb-amg-reference.ktx"
  "${builddir}/testimages/rgb-mipmap-reference.ktx"
  "${builddir}/testimages/rgb-reference-metadata.ktx"
  "${builddir}/testimages/rgb-reference.ktx"
  "${builddir}/testimages/rgba-reference.ktx"
  "${builddir}/testimages/texturearray_astc_8x8_unorm.ktx"
  "${builddir}/testimages/texturearray_bc3_unorm.ktx"
  "${builddir}/testimages/texturearray_etc2_unorm.ktx")
add_custom_command(
OUTPUT ${testimages__copies__output}
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/conftestimage_R11_EAC.ktx ${builddir}/testimages/conftestimage_R11_EAC.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/conftestimage_RG11_EAC.ktx ${builddir}/testimages/conftestimage_RG11_EAC.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/conftestimage_SIGNED_R11_EAC.ktx ${builddir}/testimages/conftestimage_SIGNED_R11_EAC.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/conftestimage_SIGNED_RG11_EAC.ktx ${builddir}/testimages/conftestimage_SIGNED_RG11_EAC.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/cubemap_yokohama_astc_8x8_unorm.ktx ${builddir}/testimages/cubemap_yokohama_astc_8x8_unorm.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/cubemap_yokohama_bc3_unorm.ktx ${builddir}/testimages/cubemap_yokohama_bc3_unorm.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/cubemap_yokohama_etc2_unorm.ktx ${builddir}/testimages/cubemap_yokohama_etc2_unorm.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/etc1.ktx ${builddir}/testimages/etc1.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/etc2-rgb.ktx ${builddir}/testimages/etc2-rgb.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/etc2-rgba1.ktx ${builddir}/testimages/etc2-rgba1.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/etc2-rgba8.ktx ${builddir}/testimages/etc2-rgba8.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/etc2-sRGB.ktx ${builddir}/testimages/etc2-sRGB.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/etc2-sRGBa1.ktx ${builddir}/testimages/etc2-sRGBa1.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/etc2-sRGBa8.ktx ${builddir}/testimages/etc2-sRGBa8.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/hi_mark.ktx ${builddir}/testimages/hi_mark.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/hi_mark_sq.ktx ${builddir}/testimages/hi_mark_sq.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/luminance-reference-metadata.ktx ${builddir}/testimages/luminance-reference-metadata.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/metalplate-amg-rgba8.ktx ${builddir}/testimages/metalplate-amg-rgba8.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/no-npot.ktx ${builddir}/testimages/no-npot.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/not4_rgb888_srgb.ktx ${builddir}/testimages/not4_rgb888_srgb.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/orient-down-metadata.ktx ${builddir}/testimages/orient-down-metadata.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/orient-up-metadata.ktx ${builddir}/testimages/orient-up-metadata.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/orient-up.ktx ${builddir}/testimages/orient-up.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/pattern_02_bc2.ktx ${builddir}/testimages/pattern_02_bc2.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/rgb-amg-reference.ktx ${builddir}/testimages/rgb-amg-reference.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/rgb-mipmap-reference.ktx ${builddir}/testimages/rgb-mipmap-reference.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/rgb-reference-metadata.ktx ${builddir}/testimages/rgb-reference-metadata.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/rgb-reference.ktx ${builddir}/testimages/rgb-reference.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/rgba-reference.ktx ${builddir}/testimages/rgba-reference.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/texturearray_astc_8x8_unorm.ktx ${builddir}/testimages/texturearray_astc_8x8_unorm.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/texturearray_bc3_unorm.ktx ${builddir}/testimages/texturearray_bc3_unorm.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/texturearray_etc2_unorm.ktx ${builddir}/testimages/texturearray_etc2_unorm.ktx
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
set(libsdl__copies__input "${CMAKE_CURRENT_LIST_DIR}/../../../../other_lib/linux/${configuration}-x64/libSDL2-2.0.so")
set(libsdl__copies__output "${builddir}/libSDL2-2.0.so")
add_custom_command(
OUTPUT ${libsdl__copies__output}
COMMAND ${CMAKE_COMMAND} -E copy other_lib/linux/${configuration}-x64/libSDL2-2.0.so ${builddir}/libSDL2-2.0.so
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
  COMMAND glslc "-fshader-stage=fragment" -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
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
  COMMAND glslc "-fshader-stage=vertex" -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
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
  "../../../../tests/loadtests/appfwSDL/VulkanAppSDL/VulkanAppSDL.h"
  "../../../../tests/loadtests/appfwSDL/VulkanAppSDL/vulkancheckres.h"
  "../../../../tests/loadtests/appfwSDL/VulkanAppSDL/VulkanContext.h"
  "../../../../tests/loadtests/appfwSDL/VulkanAppSDL/VulkanSwapchain.h"
  "../../../../tests/loadtests/appfwSDL/VulkanAppSDL/vulkandebug.h"
  "../../../../tests/loadtests/appfwSDL/VulkanAppSDL/vulkantextoverlay.hpp"
  "../../../../tests/loadtests/appfwSDL/VulkanAppSDL/vulkantools.h"
  "../../../../tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.frag"
  "../../../../tests/loadtests/appfwSDL/VulkanAppSDL/shaders/textoverlay.vert")
add_library(appfwSDL STATIC ${appfwSDL__cxx_srcs} ${appfwSDL__other_srcs})
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
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include"
  "/$ENV{VULKAN_SDK}/include")
set_property(TARGET appfwSDL APPEND PROPERTY INCLUDE_DIRECTORIES ${appfwSDL__include_dirs})
set_target_properties(appfwSDL PROPERTIES COMPILE_DEFINITIONS "NDEBUG;")
set_target_properties(appfwSDL PROPERTIES COMPILE_FLAGS "-pedantic -O3 -std=c++11 ")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
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
list(APPEND gl3loadtests__cxx_srcs
  "../../../../tests/loadtests/common/LoadTestSample.cpp"
  "../../../../tests/loadtests/common/SwipeDetector.cpp"
  "../../../../tests/loadtests/glloadtests/GLLoadTests.cpp"
  "../../../../tests/loadtests/glloadtests/shader-based/DrawTexture.cpp"
  "../../../../tests/loadtests/glloadtests/shader-based/GL3LoadTests.cpp"
  "../../../../tests/loadtests/glloadtests/shader-based/GL3LoadTestSample.cpp"
  "../../../../tests/loadtests/glloadtests/shader-based/TextureArray.cpp"
  "../../../../tests/loadtests/glloadtests/shader-based/TexturedCube.cpp"
  "../../../../tests/loadtests/glloadtests/shader-based/shaders.cpp")
list(APPEND gl3loadtests__other_srcs
  "../../../../tests/loadtests/common/LoadTestSample.h"
  "../../../../tests/loadtests/common/SwipeDetector.h"
  "../../../../tests/loadtests/geom/cube.h"
  "../../../../tests/loadtests/geom/frame.h"
  "../../../../tests/loadtests/glloadtests/GLLoadTests.h"
  "../../../../tests/loadtests/common/ltexceptions.h"
  "../../../../tests/loadtests/glloadtests/shader-based/DrawTexture.h"
  "../../../../tests/loadtests/glloadtests/shader-based/GL3LoadTestSample.h"
  "../../../../tests/loadtests/glloadtests/shader-based/TextureArray.h"
  "../../../../tests/loadtests/glloadtests/shader-based/TexturedCube.h"
  "../../../../tests/loadtests/glloadtests/shader-based/mygl.h")
link_directories( ../../../../other_lib/linux/${configuration}-x64
)
add_executable(gl3loadtests ${gl3loadtests__cxx_srcs} ${gl3loadtests__other_srcs})
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
set_target_properties(gl3loadtests PROPERTIES COMPILE_FLAGS "-pedantic -O3 ")
set_target_properties(gl3loadtests PROPERTIES LINK_FLAGS "-Wl,-rpath,. ")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "YES")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.khronos.${PRODUCT_NAME}")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_INFOPLIST_FILE "tests/loadtests/glloadtests/resources/mac/Info.plist")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
add_dependencies(gl3loadtests
  libgl
  testimages
  libsdl
  vulkan_headers
)
target_link_libraries(gl3loadtests
  appfwSDL
  libktx_gl
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
  COMMAND glslc "-fshader-stage=fragment" -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
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
  COMMAND glslc "-fshader-stage=fragment" -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
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
  COMMAND glslc "-fshader-stage=fragment" -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
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
  COMMAND glslc "-fshader-stage=fragment" -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
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
  COMMAND glslc "-fshader-stage=fragment" -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
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
set(RULE_INPUT_PATH "tests/loadtests/vkloadtests/shaders/texturemipmap/instancinglod.frag")
set(RULE_INPUT_DIRNAME "tests/loadtests/vkloadtests/shaders/texturemipmap")
set(RULE_INPUT_NAME "instancinglod.frag")
set(RULE_INPUT_ROOT "instancinglod")
set(RULE_INPUT_EXT ".frag")
set(vkloadtests__frag2spirv_5_0 "${builddir}/shaders/${RULE_INPUT_NAME}.spv")
add_custom_command(OUTPUT
  ${vkloadtests__frag2spirv_5_0}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${builddir}/shaders
  COMMAND glslc "-fshader-stage=fragment" -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
  DEPENDS ${vkloadtests__frag2spirv__input} ../../../../tests/loadtests/vkloadtests/shaders/texturemipmap/instancinglod.frag
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
  ${vkloadtests__frag2spirv_5_0}
SOURCES ${vkloadtests__frag2spirv__input}
  ../../../../tests/loadtests/vkloadtests/shaders/cube/cube.frag
  ../../../../tests/loadtests/vkloadtests/shaders/cubemap/reflect.frag
  ../../../../tests/loadtests/vkloadtests/shaders/cubemap/skybox.frag
  ../../../../tests/loadtests/vkloadtests/shaders/texture/texture.frag
  ../../../../tests/loadtests/vkloadtests/shaders/texturearray/instancing.frag
  ../../../../tests/loadtests/vkloadtests/shaders/texturemipmap/instancinglod.frag
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
  COMMAND glslc "-fshader-stage=vertex" -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
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
  COMMAND glslc "-fshader-stage=vertex" -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
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
  COMMAND glslc "-fshader-stage=vertex" -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
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
  COMMAND glslc "-fshader-stage=vertex" -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
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
  COMMAND glslc "-fshader-stage=vertex" -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
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
set(RULE_INPUT_PATH "tests/loadtests/vkloadtests/shaders/texturemipmap/instancinglod.vert")
set(RULE_INPUT_DIRNAME "tests/loadtests/vkloadtests/shaders/texturemipmap")
set(RULE_INPUT_NAME "instancinglod.vert")
set(RULE_INPUT_ROOT "instancinglod")
set(RULE_INPUT_EXT ".vert")
set(vkloadtests__vert2spirv_5_0 "${builddir}/shaders/${RULE_INPUT_NAME}.spv")
add_custom_command(OUTPUT
  ${vkloadtests__vert2spirv_5_0}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${builddir}/shaders
  COMMAND glslc "-fshader-stage=vertex" -o "${builddir}/shaders/${RULE_INPUT_NAME}.spv" "${RULE_INPUT_PATH}"
  DEPENDS ${vkloadtests__vert2spirv__input} ../../../../tests/loadtests/vkloadtests/shaders/texturemipmap/instancinglod.vert
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
  ${vkloadtests__vert2spirv_5_0}
SOURCES ${vkloadtests__vert2spirv__input}
  ../../../../tests/loadtests/vkloadtests/shaders/cube/cube.vert
  ../../../../tests/loadtests/vkloadtests/shaders/cubemap/reflect.vert
  ../../../../tests/loadtests/vkloadtests/shaders/cubemap/skybox.vert
  ../../../../tests/loadtests/vkloadtests/shaders/texture/texture.vert
  ../../../../tests/loadtests/vkloadtests/shaders/texturearray/instancing.vert
  ../../../../tests/loadtests/vkloadtests/shaders/texturemipmap/instancinglod.vert
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
  "../../../../utils/argparser.cpp"
  "../../../../tests/loadtests/common/LoadTestSample.cpp"
  "../../../../tests/loadtests/common/SwipeDetector.cpp"
  "../../../../tests/loadtests/vkloadtests/InstancedSampleBase.cpp"
  "../../../../tests/loadtests/vkloadtests/Texture.cpp"
  "../../../../tests/loadtests/vkloadtests/TextureArray.cpp"
  "../../../../tests/loadtests/vkloadtests/TextureCubemap.cpp"
  "../../../../tests/loadtests/vkloadtests/TexturedCube.cpp"
  "../../../../tests/loadtests/vkloadtests/TextureMipmap.cpp"
  "../../../../tests/loadtests/vkloadtests/VulkanLoadTests.cpp"
  "../../../../tests/loadtests/vkloadtests/VulkanLoadTestSample.cpp")
list(APPEND vkloadtests__other_srcs
  "../../../../utils/argparser.h"
  "../../../../tests/loadtests/common/LoadTestSample.h"
  "../../../../tests/loadtests/common/SwipeDetector.h"
  "../../../../tests/loadtests/common/ltexceptions.h"
  "../../../../tests/loadtests/common/vecmath.hpp"
  "../../../../tests/loadtests/vkloadtests/resources/mac/Info.plist"
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
  "../../../../tests/loadtests/vkloadtests/shaders/texturemipmap/instancinglod.frag"
  "../../../../tests/loadtests/vkloadtests/shaders/texturemipmap/instancinglod.vert"
  "../../../../tests/loadtests/vkloadtests/InstancedSampleBase.h"
  "../../../../tests/loadtests/vkloadtests/Texture.h"
  "../../../../tests/loadtests/vkloadtests/TextureArray.h"
  "../../../../tests/loadtests/vkloadtests/TextureCubemap.h"
  "../../../../tests/loadtests/vkloadtests/TexturedCube.h"
  "../../../../tests/loadtests/vkloadtests/TextureMipmap.h"
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
  "${CMAKE_CURRENT_LIST_DIR}/../../../../utils"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/common"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/geom"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/appfwSDL"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/appfwSDL/VulkanAppSDL"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include"
  "/$ENV{VULKAN_SDK}/include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include")
set_property(TARGET vkloadtests APPEND PROPERTY INCLUDE_DIRECTORIES ${vkloadtests__include_dirs})
set_target_properties(vkloadtests PROPERTIES COMPILE_DEFINITIONS "NDEBUG;")
set_target_properties(vkloadtests PROPERTIES COMPILE_FLAGS "-pedantic -std=c++11 -O3 ")
set_target_properties(vkloadtests PROPERTIES LINK_FLAGS "-Wl,-rpath,. ")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "YES")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_OTHER_LDFLAGS "-lassimp -lz")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.khronos.${PRODUCT_NAME}")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_INFOPLIST_FILE "tests/loadtests/vkloadtests/resources/mac/Info.plist")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
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
set_target_properties(gtest_main PROPERTIES COMPILE_FLAGS "-pedantic -O3 ")
set_target_properties(gtest_main PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(gtest_main PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(gtest_main PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(gtest_main PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(gtest_main PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(gtest_main PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(gtest_main PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES")
set_target_properties(gtest_main PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
set_target_properties(gtest_main PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
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
set_target_properties(gtest PROPERTIES COMPILE_FLAGS "-pedantic -O3 ")
set_target_properties(gtest PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(gtest PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(gtest PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(gtest PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(gtest PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(gtest PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(gtest PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES")
set_target_properties(gtest PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
set_target_properties(gtest PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
unset(TOOLSET)
unset(TARGET)

#ktxtests.gyp:unittests#target
set(TARGET "unittests")
set(TOOLSET "target")
set(unittests__cxx_srcs "../../../../tests/unittests/unittests.cc")
set(unittests__other_srcs "../../../../tests/unittests/wthelper.h")
add_executable(unittests ${unittests__cxx_srcs} ${unittests__other_srcs})
set_target_properties(unittests PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_source_files_properties(${unittests__other_srcs} PROPERTIES HEADER_FILE_ONLY "TRUE")
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
set_target_properties(unittests PROPERTIES COMPILE_FLAGS "-pedantic -O3 ")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "YES")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
target_link_libraries(unittests
-Wl,--start-group
  gtest
  gtest_main
-Wl,--end-group
  libktx_gl
)
unset(TOOLSET)
unset(TARGET)

#ktxtests.gyp:texturetests#target
set(TARGET "texturetests")
set(TOOLSET "target")
set(texturetests__cxx_srcs "../../../../tests/texturetests/texturetests.cc")
set(texturetests__other_srcs "../../../../tests/unittests/wthelper.h")
add_executable(texturetests ${texturetests__cxx_srcs} ${texturetests__other_srcs})
set_target_properties(texturetests PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_source_files_properties(${texturetests__other_srcs} PROPERTIES HEADER_FILE_ONLY "TRUE")
set_target_properties(texturetests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${builddir}")
set_target_properties(texturetests PROPERTIES PREFIX "")
set_target_properties(texturetests PROPERTIES RUNTIME_OUTPUT_NAME "texturetests")
set_target_properties(texturetests PROPERTIES SUFFIX "")
set_source_files_properties(${builddir}/texturetests PROPERTIES GENERATED "TRUE")
list(APPEND texturetests__include_dirs
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/gtest/include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/unittests"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include")
set_property(TARGET texturetests APPEND PROPERTY INCLUDE_DIRECTORIES ${texturetests__include_dirs})
set_target_properties(texturetests PROPERTIES COMPILE_DEFINITIONS "NDEBUG;")
set_target_properties(texturetests PROPERTIES COMPILE_FLAGS "-pedantic -O3 ")
set_target_properties(texturetests PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(texturetests PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(texturetests PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(texturetests PROPERTIES XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "YES")
set_target_properties(texturetests PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(texturetests PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(texturetests PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(texturetests PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES")
set_target_properties(texturetests PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
set_target_properties(texturetests PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
target_link_libraries(texturetests
-Wl,--start-group
  gtest
  gtest_main
-Wl,--end-group
  libktx_gl
)
unset(TOOLSET)
unset(TARGET)

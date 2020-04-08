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

#libktx.gyp:version.h#target
set(TARGET "version.h")
set(TOOLSET "target")
list(APPEND version_h__mkversion__input
  "${CMAKE_CURRENT_LIST_DIR}/../../../../mkversion"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../.git")
set(version_h__mkversion__output "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/version.h")
add_custom_command(OUTPUT ${version_h__mkversion__output}
  COMMAND ${CMAKE_COMMAND} -E make_directory lib
  COMMAND ./mkversion -o version.h lib
  DEPENDS ${version_h__mkversion__input}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT version_h__mkversion
  VERBATIM
)
add_custom_target(version_h__mkversion
  DEPENDS ${version_h__mkversion__output}
  SOURCES ${version_h__mkversion__input}
)
add_custom_target(version_h SOURCES)
set_target_properties(version_h PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(version_h__mkversion PROPERTIES EXCLUDE_FROM_ALL "FALSE")
add_dependencies(version_h
  version_h__mkversion
)
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
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/apg_bmp.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/apg_bmp.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_astc_decomp.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_astc_decomp.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_backend.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_backend.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_basis_file.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_basis_file.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_bc7enc.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_bc7enc.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_comp.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_comp.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_enc.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_enc.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_etc.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_etc.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_frontend.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_frontend.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_global_selector_palette_helpers.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_global_selector_palette_helpers.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_gpu_texture.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_gpu_texture.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_pvrtc1_4.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_pvrtc1_4.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_resampler.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_resampler.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_resample_filters.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_resampler_filters.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/jpgd.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/jpgd.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/lodepng.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/lodepng.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/transcoder/basisu.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/transcoder/basisu_file_headers.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/transcoder/basisu_transcoder.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/transcoder/basisu_transcoder.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/transcoder/basisu_transcoder_internal.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/transcoder/basisu_transcoder_tables_astc.inc"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/transcoder/basisu_transcoder_tables_astc_0_255.inc"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/transcoder/basisu_transcoder_tables_atc_55.inc"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/transcoder/basisu_transcoder_tables_atc_56.inc"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/transcoder/basisu_transcoder_tables_bc7_m5_alpha.inc"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/transcoder/basisu_transcoder_tables_bc7_m5_color.inc"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/transcoder/basisu_transcoder_tables_dxt1_5.inc"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/transcoder/basisu_transcoder_tables_dxt1_6.inc"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/transcoder/basisu_transcoder_tables_pvrtc2_45.inc"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/transcoder/basisu_transcoder_tables_pvrtc2_alpha_33.inc"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_uastc_enc.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/basisu_uastc_enc.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basis_sgd.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basis_encode.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basis_image_transcoder.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basis_transcode.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basis_transcoder_config.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/checkheader.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/dfdutils/createdfd.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/dfdutils/dfd.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/dfdutils/dfd2vk.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/dfdutils/dfd2vk.inl"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/dfdutils/interpretdfd.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/dfdutils/printdfd.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/dfdutils/queries.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/dfdutils/vk2dfd.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/dfdutils/vk2dfd.inl"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/etcdec.cxx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/etcunpack.cxx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/filestream.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/filestream.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/formatsize.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/gl_format.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/gl_funcptrs.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/gles1_funcptrs.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/gles2_funcptrs.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/gles3_funcptrs.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/glloader.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/hashlist.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/info.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/ktxgl.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/ktxint.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/memstream.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/memstream.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/stream.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/strings.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/swap.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/texture.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/texture.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/texture_funcs.inl"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/texture1.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/texture1.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/texture2.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/texture2.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/uthash.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/vkformat_enum.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/vkformat_check.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/vkformat_str.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/writer1.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/writer2.c"
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
add_dependencies(libktx_doc__buildLibktxDoc
  version_h
)
add_dependencies(libktx_doc
  version_h
  libktx_doc__buildLibktxDoc
)
unset(TOOLSET)
unset(TARGET)

#libktx.gyp:mkvkformatfiles#target
set(TARGET "mkvkformatfiles")
set(TOOLSET "target")
list(APPEND mkvkformatfiles__run_mkvkformatfiles__input
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/dfdutils/vulkan/vulkan_core.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/mkvkformatfiles")
list(APPEND mkvkformatfiles__run_mkvkformatfiles__output
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/vkformat_enum.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/vkformat_check.c"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/vkformat_str.c")
add_custom_command(OUTPUT ${mkvkformatfiles__run_mkvkformatfiles__output}
  COMMAND ${CMAKE_COMMAND} -E make_directory lib
  COMMAND lib/mkvkformatfiles lib
  DEPENDS ${mkvkformatfiles__run_mkvkformatfiles__input}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT Generating VkFormat-related source files
  VERBATIM
)
add_custom_target(mkvkformatfiles__run_mkvkformatfiles
  DEPENDS ${mkvkformatfiles__run_mkvkformatfiles__output}
  SOURCES ${mkvkformatfiles__run_mkvkformatfiles__input}
)
list(APPEND mkvkformatfiles__run_makevkswitch__input
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/vkformat_enum.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/dfdutils/makevkswitch.pl")
set(mkvkformatfiles__run_makevkswitch__output "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/dfdutils/vk2dfd.inl")
add_custom_command(OUTPUT ${mkvkformatfiles__run_makevkswitch__output}
  COMMAND ${CMAKE_COMMAND} -E make_directory lib/dfdutils
  COMMAND lib/dfdutils/makevkswitch.pl lib/vkformat_enum.h lib/dfdutils/vk2dfd.inl
  DEPENDS ${mkvkformatfiles__run_makevkswitch__input}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT Generating VkFormat/DFD switch body
  VERBATIM
)
add_custom_target(mkvkformatfiles__run_makevkswitch
  DEPENDS ${mkvkformatfiles__run_makevkswitch__output}
  SOURCES ${mkvkformatfiles__run_makevkswitch__input}
)
list(APPEND mkvkformatfiles__run_makedfdtovk__input
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/vkformat_enum.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/dfdutils/makedfd2vk.pl")
set(mkvkformatfiles__run_makedfdtovk__output "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/dfdutils/dfd2vk.inl")
add_custom_command(OUTPUT ${mkvkformatfiles__run_makedfdtovk__output}
  COMMAND ${CMAKE_COMMAND} -E make_directory lib/dfdutils
  COMMAND lib/dfdutils/makedfd2vk.pl lib/vkformat_enum.h lib/dfdutils/dfd2vk.inl
  DEPENDS ${mkvkformatfiles__run_makedfdtovk__input}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT Generating DFD/VkFormat switch body
  VERBATIM
)
add_custom_target(mkvkformatfiles__run_makedfdtovk
  DEPENDS ${mkvkformatfiles__run_makedfdtovk__output}
  SOURCES ${mkvkformatfiles__run_makedfdtovk__input}
)
add_custom_target(mkvkformatfiles SOURCES)
set_target_properties(mkvkformatfiles PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(mkvkformatfiles__run_mkvkformatfiles PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(mkvkformatfiles__run_makevkswitch PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(mkvkformatfiles__run_makedfdtovk PROPERTIES EXCLUDE_FROM_ALL "FALSE")
add_dependencies(mkvkformatfiles
  mkvkformatfiles__run_mkvkformatfiles
  mkvkformatfiles__run_makevkswitch
  mkvkformatfiles__run_makedfdtovk
)
unset(TOOLSET)
unset(TARGET)

#libktx.gyp:libgles3#target
set(TARGET "libgles3")
set(TOOLSET "target")
add_custom_target(libgles3 SOURCES)
set_target_properties(libgles3 PROPERTIES EXCLUDE_FROM_ALL "FALSE")
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
  "../../../../lib/basisu/apg_bmp.c"
  "../../../../lib/checkheader.c"
  "../../../../lib/dfdutils/createdfd.c"
  "../../../../lib/dfdutils/dfd2vk.c"
  "../../../../lib/dfdutils/interpretdfd.c"
  "../../../../lib/dfdutils/printdfd.c"
  "../../../../lib/dfdutils/queries.c"
  "../../../../lib/dfdutils/vk2dfd.c"
  "../../../../lib/filestream.c"
  "../../../../lib/glloader.c"
  "../../../../lib/hashlist.c"
  "../../../../lib/info.c"
  "../../../../lib/memstream.c"
  "../../../../lib/strings.c"
  "../../../../lib/swap.c"
  "../../../../lib/texture.c"
  "../../../../lib/texture1.c"
  "../../../../lib/texture2.c"
  "../../../../lib/vkformat_check.c"
  "../../../../lib/vkformat_str.c"
  "../../../../lib/writer1.c"
  "../../../../lib/writer2.c"
  "../../../../lib/vkloader.c"
  "../../../../lib/vk_funcs.c")
list(APPEND libktx_gl__cxx_srcs
  "../../../../lib/basisu/basisu_astc_decomp.cpp"
  "../../../../lib/basisu/basisu_backend.cpp"
  "../../../../lib/basisu/basisu_basis_file.cpp"
  "../../../../lib/basisu/basisu_bc7enc.cpp"
  "../../../../lib/basisu/basisu_comp.cpp"
  "../../../../lib/basisu/basisu_enc.cpp"
  "../../../../lib/basisu/basisu_etc.cpp"
  "../../../../lib/basisu/basisu_frontend.cpp"
  "../../../../lib/basisu/basisu_global_selector_palette_helpers.cpp"
  "../../../../lib/basisu/basisu_gpu_texture.cpp"
  "../../../../lib/basisu/basisu_pvrtc1_4.cpp"
  "../../../../lib/basisu/basisu_resampler.cpp"
  "../../../../lib/basisu/basisu_resample_filters.cpp"
  "../../../../lib/basisu/jpgd.cpp"
  "../../../../lib/basisu/lodepng.cpp"
  "../../../../lib/basisu/transcoder/basisu_transcoder.cpp"
  "../../../../lib/basisu/basisu_uastc_enc.cpp"
  "../../../../lib/basis_encode.cpp"
  "../../../../lib/basis_transcode.cpp"
  "../../../../lib/etcdec.cxx"
  "../../../../lib/etcunpack.cxx")
list(APPEND libktx_gl__other_srcs
  "../../../../include/ktx.h"
  "../../../../lib/basisu/apg_bmp.h"
  "../../../../lib/basisu/basisu_astc_decomp.h"
  "../../../../lib/basisu/basisu_backend.h"
  "../../../../lib/basisu/basisu_basis_file.h"
  "../../../../lib/basisu/basisu_bc7enc.h"
  "../../../../lib/basisu/basisu_comp.h"
  "../../../../lib/basisu/basisu_enc.h"
  "../../../../lib/basisu/basisu_etc.h"
  "../../../../lib/basisu/basisu_frontend.h"
  "../../../../lib/basisu/basisu_global_selector_palette_helpers.h"
  "../../../../lib/basisu/basisu_gpu_texture.h"
  "../../../../lib/basisu/basisu_pvrtc1_4.h"
  "../../../../lib/basisu/basisu_resampler.h"
  "../../../../lib/basisu/basisu_resampler_filters.h"
  "../../../../lib/basisu/jpgd.h"
  "../../../../lib/basisu/lodepng.h"
  "../../../../lib/basisu/transcoder/basisu.h"
  "../../../../lib/basisu/transcoder/basisu_file_headers.h"
  "../../../../lib/basisu/transcoder/basisu_transcoder.h"
  "../../../../lib/basisu/transcoder/basisu_transcoder_internal.h"
  "../../../../lib/basisu/transcoder/basisu_transcoder_tables_astc.inc"
  "../../../../lib/basisu/transcoder/basisu_transcoder_tables_astc_0_255.inc"
  "../../../../lib/basisu/transcoder/basisu_transcoder_tables_atc_55.inc"
  "../../../../lib/basisu/transcoder/basisu_transcoder_tables_atc_56.inc"
  "../../../../lib/basisu/transcoder/basisu_transcoder_tables_bc7_m5_alpha.inc"
  "../../../../lib/basisu/transcoder/basisu_transcoder_tables_bc7_m5_color.inc"
  "../../../../lib/basisu/transcoder/basisu_transcoder_tables_dxt1_5.inc"
  "../../../../lib/basisu/transcoder/basisu_transcoder_tables_dxt1_6.inc"
  "../../../../lib/basisu/transcoder/basisu_transcoder_tables_pvrtc2_45.inc"
  "../../../../lib/basisu/transcoder/basisu_transcoder_tables_pvrtc2_alpha_33.inc"
  "../../../../lib/basisu/basisu_uastc_enc.h"
  "../../../../lib/basis_sgd.h"
  "../../../../lib/basis_image_transcoder.h"
  "../../../../lib/basis_transcoder_config.h"
  "../../../../lib/dfdutils/dfd.h"
  "../../../../lib/dfdutils/dfd2vk.inl"
  "../../../../lib/dfdutils/vk2dfd.inl"
  "../../../../lib/filestream.h"
  "../../../../lib/formatsize.h"
  "../../../../lib/gl_format.h"
  "../../../../lib/gl_funcptrs.h"
  "../../../../lib/gles1_funcptrs.h"
  "../../../../lib/gles2_funcptrs.h"
  "../../../../lib/gles3_funcptrs.h"
  "../../../../lib/ktxgl.h"
  "../../../../lib/ktxint.h"
  "../../../../lib/memstream.h"
  "../../../../lib/stream.h"
  "../../../../lib/texture.h"
  "../../../../lib/texture_funcs.inl"
  "../../../../lib/texture1.h"
  "../../../../lib/texture2.h"
  "../../../../lib/uthash.h"
  "../../../../lib/vkformat_enum.h"
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
set_target_properties(libktx_gl PROPERTIES COMPILE_DEFINITIONS "KTX_OPENGL=1;KTX_USE_FUNCPTRS_FOR_VULKAN;LIBKTX=1;NDEBUG;")
set_source_files_properties(${libktx_gl__c_srcs} PROPERTIES COMPILE_FLAGS "-Wpedantic -O3 -fPIC -std=c99 ")
set_source_files_properties(${libktx_gl__cxx_srcs} PROPERTIES COMPILE_FLAGS "-Wpedantic -O3 -fPIC -std=c++11 -Wno-pedantic ")
set_target_properties(libktx_gl PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(libktx_gl PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(libktx_gl PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(libktx_gl PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "")
set_target_properties(libktx_gl PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(libktx_gl PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(libktx_gl PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES")
set_target_properties(libktx_gl PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(libktx_gl PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic")
set_target_properties(libktx_gl PROPERTIES XCODE_ATTRIBUTE_GCC_TREAT_WARNINGS_AS_ERRORS "YES")
set_target_properties(libktx_gl PROPERTIES XCODE_ATTRIBUTE_WARNING_CFLAGS "-Wno-nested-anon-types -Wno-gnu-anonymous-struct ")
set_target_properties(libktx_gl PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
set_target_properties(libktx_gl PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
add_dependencies(libktx_gl
  vulkan_headers
  version_h
  libgl
)
target_link_libraries(libktx_gl
  "-lGL"
)
unset(TOOLSET)
unset(TARGET)

#libktx.gyp:install.lib#target
set(TARGET "install.lib")
set(TOOLSET "target")
list(APPEND install_lib__copies__input
  "${builddir}/lib.target/libktx.gl.so"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include/ktx.h"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include/ktxvulkan.h")
list(APPEND install_lib__copies__output
  "/tmp/ktx.dst//usr/local/lib/libktx.gl.so"
  "/tmp/ktx.dst//usr/local/include/ktx.h"
  "/tmp/ktx.dst//usr/local/include/ktxvulkan.h")
set(install_lib__copies__input_dirs "${CMAKE_CURRENT_LIST_DIR}/../../../../build/docs/man/man3")
set(install_lib__copies__output_dirs "/tmp/ktx.dst//usr/local/share/man/man3")
add_custom_command(
OUTPUT ${install_lib__copies__output} ${install_lib__copies__output_dirs}
COMMAND ${CMAKE_COMMAND} -E copy ${builddir}/lib.target/libktx.gl.so /tmp/ktx.dst//usr/local/lib/libktx.gl.so
COMMAND ${CMAKE_COMMAND} -E copy include/ktx.h /tmp/ktx.dst//usr/local/include/ktx.h
COMMAND ${CMAKE_COMMAND} -E copy include/ktxvulkan.h /tmp/ktx.dst//usr/local/include/ktxvulkan.h
COMMAND ${CMAKE_COMMAND} -E copy_directory build/docs/man/man3/ /tmp/ktx.dst//usr/local/share/man/man3
DEPENDS ${install_lib__copies__input} ${install_lib__copies__input_dirs}
WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
COMMENT Copying for install_lib
VERBATIM
)
add_custom_target(install_lib__copies
  DEPENDS ${install_lib__copies__output} ${install_lib__copies__output_dirs}
  SOURCES ${install_lib__copies__input}
)
add_custom_target(install_lib SOURCES)
set_target_properties(install_lib PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(install_lib__copies PROPERTIES EXCLUDE_FROM_ALL "FALSE")
add_dependencies(install_lib__copies
  libktx_gl
  libktx_doc
)
add_dependencies(install_lib
  libktx_doc
  install_lib__copies
  libktx_gl
)
unset(TOOLSET)
unset(TARGET)

#ktxtools.gyp:toktx#target
set(TARGET "toktx")
set(TOOLSET "target")
list(APPEND toktx__mkversion__input
  "${CMAKE_CURRENT_LIST_DIR}/../../../../mkversion"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../.git")
set(toktx__mkversion__output "${CMAKE_CURRENT_LIST_DIR}/../../../../tools/toktx/version.h")
add_custom_command(OUTPUT ${toktx__mkversion__output}
  COMMAND ${CMAKE_COMMAND} -E make_directory tools/toktx
  COMMAND ./mkversion -o version.h tools/toktx
  DEPENDS ${toktx__mkversion__input}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT toktx__mkversion
  VERBATIM
)
add_custom_target(toktx__mkversion
  DEPENDS ${toktx__mkversion__output}
  SOURCES ${toktx__mkversion__input}
)
list(APPEND toktx__cxx_srcs
  "../../../../utils/argparser.cpp"
  "../../../../tools/toktx/image.cc"
  "../../../../tools/toktx/jpgimage.cc"
  "../../../../tools/toktx/npbmimage.cc"
  "../../../../tools/toktx/pngimage.cc"
  "../../../../tools/toktx/toktx.cc")
list(APPEND toktx__other_srcs
  "../../../../utils/argparser.h"
  "../../../../tools/toktx/image.hpp"
  "../../../../tools/toktx/stdafx.h"
  "../../../../tools/toktx/targetver.h")
add_executable(toktx ${toktx__cxx_srcs} ${toktx__other_srcs})
set_target_properties(toktx PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(toktx__mkversion PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_source_files_properties(${toktx__other_srcs} PROPERTIES HEADER_FILE_ONLY "TRUE")
set_target_properties(toktx PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${builddir}")
set_target_properties(toktx PROPERTIES PREFIX "")
set_target_properties(toktx PROPERTIES RUNTIME_OUTPUT_NAME "toktx")
set_target_properties(toktx PROPERTIES SUFFIX "")
set_source_files_properties(${builddir}/toktx PROPERTIES GENERATED "TRUE")
list(APPEND toktx__include_dirs
  "${CMAKE_CURRENT_LIST_DIR}/../../../../utils"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include")
set_property(TARGET toktx APPEND PROPERTY INCLUDE_DIRECTORIES ${toktx__include_dirs})
set_target_properties(toktx PROPERTIES COMPILE_DEFINITIONS "NDEBUG;")
set_target_properties(toktx PROPERTIES COMPILE_FLAGS "-Wpedantic -O3 -std=c++11 -Wno-pedantic ")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "YES")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_GCC_TREAT_WARNINGS_AS_ERRORS "YES")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_WARNING_CFLAGS "-Wno-nested-anon-types -Wno-gnu-anonymous-struct ")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
set_target_properties(toktx PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
add_dependencies(toktx__mkversion
  libktx_gl
)
add_dependencies(toktx
  toktx__mkversion
)
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

#ktxtools.gyp:ktxsc#target
set(TARGET "ktxsc")
set(TOOLSET "target")
list(APPEND ktxsc__mkversion__input
  "${CMAKE_CURRENT_LIST_DIR}/../../../../mkversion"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../.git")
set(ktxsc__mkversion__output "${CMAKE_CURRENT_LIST_DIR}/../../../../tools/ktxsc/version.h")
add_custom_command(OUTPUT ${ktxsc__mkversion__output}
  COMMAND ${CMAKE_COMMAND} -E make_directory tools/ktxsc
  COMMAND ./mkversion -o version.h tools/ktxsc
  DEPENDS ${ktxsc__mkversion__input}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT ktxsc__mkversion
  VERBATIM
)
add_custom_target(ktxsc__mkversion
  DEPENDS ${ktxsc__mkversion__output}
  SOURCES ${ktxsc__mkversion__input}
)
list(APPEND ktxsc__cxx_srcs
  "../../../../utils/argparser.cpp"
  "../../../../tools/ktxsc/ktxsc.cpp")
list(APPEND ktxsc__other_srcs
  "../../../../utils/argparser.h"
  "../../../../tools/ktxsc/stdafx.h")
add_executable(ktxsc ${ktxsc__cxx_srcs} ${ktxsc__other_srcs})
set_target_properties(ktxsc PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(ktxsc__mkversion PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_source_files_properties(${ktxsc__other_srcs} PROPERTIES HEADER_FILE_ONLY "TRUE")
set_target_properties(ktxsc PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${builddir}")
set_target_properties(ktxsc PROPERTIES PREFIX "")
set_target_properties(ktxsc PROPERTIES RUNTIME_OUTPUT_NAME "ktxsc")
set_target_properties(ktxsc PROPERTIES SUFFIX "")
set_source_files_properties(${builddir}/ktxsc PROPERTIES GENERATED "TRUE")
list(APPEND ktxsc__include_dirs
  "${CMAKE_CURRENT_LIST_DIR}/../../../../utils"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include")
set_property(TARGET ktxsc APPEND PROPERTY INCLUDE_DIRECTORIES ${ktxsc__include_dirs})
set_target_properties(ktxsc PROPERTIES COMPILE_DEFINITIONS "NDEBUG;")
set_target_properties(ktxsc PROPERTIES COMPILE_FLAGS "-Wpedantic -O3 -std=c++11 ")
set_target_properties(ktxsc PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(ktxsc PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(ktxsc PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(ktxsc PROPERTIES XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "YES")
set_target_properties(ktxsc PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "")
set_target_properties(ktxsc PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(ktxsc PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(ktxsc PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES")
set_target_properties(ktxsc PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(ktxsc PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic")
set_target_properties(ktxsc PROPERTIES XCODE_ATTRIBUTE_GCC_TREAT_WARNINGS_AS_ERRORS "YES")
set_target_properties(ktxsc PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
set_target_properties(ktxsc PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
add_dependencies(ktxsc__mkversion
  libktx_gl
)
add_dependencies(ktxsc
  ktxsc__mkversion
)
target_link_libraries(ktxsc
  libktx_gl
)
unset(TOOLSET)
unset(TARGET)

#ktxtools.gyp:ktxinfo#target
set(TARGET "ktxinfo")
set(TOOLSET "target")
list(APPEND ktxinfo__mkversion__input
  "${CMAKE_CURRENT_LIST_DIR}/../../../../mkversion"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../.git")
set(ktxinfo__mkversion__output "${CMAKE_CURRENT_LIST_DIR}/../../../../tools/ktxinfo/version.h")
add_custom_command(OUTPUT ${ktxinfo__mkversion__output}
  COMMAND ${CMAKE_COMMAND} -E make_directory tools/ktxinfo
  COMMAND ./mkversion -o version.h tools/ktxinfo
  DEPENDS ${ktxinfo__mkversion__input}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT ktxinfo__mkversion
  VERBATIM
)
add_custom_target(ktxinfo__mkversion
  DEPENDS ${ktxinfo__mkversion__output}
  SOURCES ${ktxinfo__mkversion__input}
)
list(APPEND ktxinfo__cxx_srcs
  "../../../../utils/argparser.cpp"
  "../../../../tools/ktxinfo/ktxinfo.cpp")
list(APPEND ktxinfo__other_srcs
  "../../../../utils/argparser.h"
  "../../../../tools/ktxinfo/stdafx.h")
add_executable(ktxinfo ${ktxinfo__cxx_srcs} ${ktxinfo__other_srcs})
set_target_properties(ktxinfo PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(ktxinfo__mkversion PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_source_files_properties(${ktxinfo__other_srcs} PROPERTIES HEADER_FILE_ONLY "TRUE")
set_target_properties(ktxinfo PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${builddir}")
set_target_properties(ktxinfo PROPERTIES PREFIX "")
set_target_properties(ktxinfo PROPERTIES RUNTIME_OUTPUT_NAME "ktxinfo")
set_target_properties(ktxinfo PROPERTIES SUFFIX "")
set_source_files_properties(${builddir}/ktxinfo PROPERTIES GENERATED "TRUE")
list(APPEND ktxinfo__include_dirs
  "${CMAKE_CURRENT_LIST_DIR}/../../../../utils"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include")
set_property(TARGET ktxinfo APPEND PROPERTY INCLUDE_DIRECTORIES ${ktxinfo__include_dirs})
set_target_properties(ktxinfo PROPERTIES COMPILE_DEFINITIONS "NDEBUG;")
set_target_properties(ktxinfo PROPERTIES COMPILE_FLAGS "-Wpedantic -O3 -std=c++11 ")
set_target_properties(ktxinfo PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(ktxinfo PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(ktxinfo PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(ktxinfo PROPERTIES XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "YES")
set_target_properties(ktxinfo PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "")
set_target_properties(ktxinfo PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(ktxinfo PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(ktxinfo PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES")
set_target_properties(ktxinfo PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(ktxinfo PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic")
set_target_properties(ktxinfo PROPERTIES XCODE_ATTRIBUTE_GCC_TREAT_WARNINGS_AS_ERRORS "YES")
set_target_properties(ktxinfo PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
set_target_properties(ktxinfo PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
add_dependencies(ktxinfo__mkversion
  libktx_gl
)
add_dependencies(ktxinfo
  ktxinfo__mkversion
)
target_link_libraries(ktxinfo
  libktx_gl
)
unset(TOOLSET)
unset(TARGET)

#ktxtools.gyp:ktx2ktx2#target
set(TARGET "ktx2ktx2")
set(TOOLSET "target")
list(APPEND ktx2ktx2__mkversion__input
  "${CMAKE_CURRENT_LIST_DIR}/../../../../mkversion"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../.git")
set(ktx2ktx2__mkversion__output "${CMAKE_CURRENT_LIST_DIR}/../../../../tools/ktx2ktx2/version.h")
add_custom_command(OUTPUT ${ktx2ktx2__mkversion__output}
  COMMAND ${CMAKE_COMMAND} -E make_directory tools/ktx2ktx2
  COMMAND ./mkversion -o version.h tools/ktx2ktx2
  DEPENDS ${ktx2ktx2__mkversion__input}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT ktx2ktx2__mkversion
  VERBATIM
)
add_custom_target(ktx2ktx2__mkversion
  DEPENDS ${ktx2ktx2__mkversion__output}
  SOURCES ${ktx2ktx2__mkversion__input}
)
list(APPEND ktx2ktx2__cxx_srcs
  "../../../../utils/argparser.cpp"
  "../../../../tools/ktx2ktx2/ktx2ktx2.cpp")
list(APPEND ktx2ktx2__other_srcs
  "../../../../utils/argparser.h"
  "../../../../tools/ktx2ktx2/stdafx.h")
add_executable(ktx2ktx2 ${ktx2ktx2__cxx_srcs} ${ktx2ktx2__other_srcs})
set_target_properties(ktx2ktx2 PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(ktx2ktx2__mkversion PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_source_files_properties(${ktx2ktx2__other_srcs} PROPERTIES HEADER_FILE_ONLY "TRUE")
set_target_properties(ktx2ktx2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${builddir}")
set_target_properties(ktx2ktx2 PROPERTIES PREFIX "")
set_target_properties(ktx2ktx2 PROPERTIES RUNTIME_OUTPUT_NAME "ktx2ktx2")
set_target_properties(ktx2ktx2 PROPERTIES SUFFIX "")
set_source_files_properties(${builddir}/ktx2ktx2 PROPERTIES GENERATED "TRUE")
list(APPEND ktx2ktx2__include_dirs
  "${CMAKE_CURRENT_LIST_DIR}/../../../../utils"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include")
set_property(TARGET ktx2ktx2 APPEND PROPERTY INCLUDE_DIRECTORIES ${ktx2ktx2__include_dirs})
set_target_properties(ktx2ktx2 PROPERTIES COMPILE_DEFINITIONS "NDEBUG;")
set_target_properties(ktx2ktx2 PROPERTIES COMPILE_FLAGS "-Wpedantic -O3 -std=c++11 ")
set_target_properties(ktx2ktx2 PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(ktx2ktx2 PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(ktx2ktx2 PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(ktx2ktx2 PROPERTIES XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "YES")
set_target_properties(ktx2ktx2 PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "")
set_target_properties(ktx2ktx2 PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(ktx2ktx2 PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(ktx2ktx2 PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES")
set_target_properties(ktx2ktx2 PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(ktx2ktx2 PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic")
set_target_properties(ktx2ktx2 PROPERTIES XCODE_ATTRIBUTE_GCC_TREAT_WARNINGS_AS_ERRORS "YES")
set_target_properties(ktx2ktx2 PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
set_target_properties(ktx2ktx2 PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
add_dependencies(ktx2ktx2__mkversion
  libktx_gl
)
add_dependencies(ktx2ktx2
  ktx2ktx2__mkversion
)
target_link_libraries(ktx2ktx2
  libktx_gl
)
unset(TOOLSET)
unset(TARGET)

#ktxtools.gyp:ktx2check#target
set(TARGET "ktx2check")
set(TOOLSET "target")
list(APPEND ktx2check__mkversion__input
  "${CMAKE_CURRENT_LIST_DIR}/../../../../mkversion"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../.git")
set(ktx2check__mkversion__output "${CMAKE_CURRENT_LIST_DIR}/../../../../tools/ktx2check/version.h")
add_custom_command(OUTPUT ${ktx2check__mkversion__output}
  COMMAND ${CMAKE_COMMAND} -E make_directory tools/ktx2check
  COMMAND ./mkversion -o version.h tools/ktx2check
  DEPENDS ${ktx2check__mkversion__input}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT ktx2check__mkversion
  VERBATIM
)
add_custom_target(ktx2check__mkversion
  DEPENDS ${ktx2check__mkversion__output}
  SOURCES ${ktx2check__mkversion__input}
)
list(APPEND ktx2check__cxx_srcs
  "../../../../utils/argparser.cpp"
  "../../../../tools/ktx2check/ktx2check.cpp")
list(APPEND ktx2check__other_srcs
  "../../../../utils/argparser.h"
  "../../../../utils/ktxapp.h"
  "../../../../tools/ktx2check/stdafx.h")
add_executable(ktx2check ${ktx2check__cxx_srcs} ${ktx2check__other_srcs})
set_target_properties(ktx2check PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(ktx2check__mkversion PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_source_files_properties(${ktx2check__other_srcs} PROPERTIES HEADER_FILE_ONLY "TRUE")
set_target_properties(ktx2check PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${builddir}")
set_target_properties(ktx2check PROPERTIES PREFIX "")
set_target_properties(ktx2check PROPERTIES RUNTIME_OUTPUT_NAME "ktx2check")
set_target_properties(ktx2check PROPERTIES SUFFIX "")
set_source_files_properties(${builddir}/ktx2check PROPERTIES GENERATED "TRUE")
list(APPEND ktx2check__include_dirs
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../utils"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include")
set_property(TARGET ktx2check APPEND PROPERTY INCLUDE_DIRECTORIES ${ktx2check__include_dirs})
set_target_properties(ktx2check PROPERTIES COMPILE_DEFINITIONS "NDEBUG;")
set_target_properties(ktx2check PROPERTIES COMPILE_FLAGS "-Wpedantic -O3 -std=c++11 ")
set_target_properties(ktx2check PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(ktx2check PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(ktx2check PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(ktx2check PROPERTIES XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "YES")
set_target_properties(ktx2check PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "")
set_target_properties(ktx2check PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(ktx2check PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(ktx2check PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES")
set_target_properties(ktx2check PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(ktx2check PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic")
set_target_properties(ktx2check PROPERTIES XCODE_ATTRIBUTE_GCC_TREAT_WARNINGS_AS_ERRORS "YES")
set_target_properties(ktx2check PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
set_target_properties(ktx2check PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
add_dependencies(ktx2check__mkversion
  libktx_gl
)
add_dependencies(ktx2check
  ktx2check__mkversion
)
target_link_libraries(ktx2check
  libktx_gl
)
unset(TOOLSET)
unset(TARGET)

#ktxtools.gyp:ktx2check-tests#target
set(TARGET "ktx2check-tests")
set(TOOLSET "target")
set(ktx2check-tests__ktx2check-tests__input "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/ktx2check-tests")
set(ktx2check-tests__ktx2check-tests__output "${CMAKE_CURRENT_LIST_DIR}/../../../../tools/ktx2check/testsrun")
add_custom_command(OUTPUT ${ktx2check-tests__ktx2check-tests__output}
  COMMAND ${CMAKE_COMMAND} -E make_directory tools/ktx2check
  COMMAND tests/ktx2check-tests "${builddir}/ktx2check"
  DEPENDS ${ktx2check-tests__ktx2check-tests__input}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT Running ktx2check tests
  VERBATIM
)
add_custom_target(ktx2check-tests__ktx2check-tests
  DEPENDS ${ktx2check-tests__ktx2check-tests__output}
  SOURCES ${ktx2check-tests__ktx2check-tests__input}
)
add_custom_target(ktx2check-tests SOURCES)
set_target_properties(ktx2check-tests PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(ktx2check-tests__ktx2check-tests PROPERTIES EXCLUDE_FROM_ALL "FALSE")
add_dependencies(ktx2check-tests__ktx2check-tests
  ktx2check
)
add_dependencies(ktx2check-tests
  ktx2check
  ktx2check-tests__ktx2check-tests
)
unset(TOOLSET)
unset(TARGET)

#ktxtools.gyp:ktxtools.doc#target
set(TARGET "ktxtools.doc")
set(TOOLSET "target")
list(APPEND ktxtools_doc__buildKtxtoolsDoc__input
  "${CMAKE_CURRENT_LIST_DIR}/../../../../ktxtools.doxy"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../runDoxygen"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tools/ktx2check/ktx2check.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tools/ktx2ktx2/ktx2ktx2.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tools/ktxinfo/ktxinfo.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tools/ktxsc/ktxsc.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tools/toktx/toktx.cc")
set(ktxtools_doc__buildKtxtoolsDoc__output "${CMAKE_CURRENT_LIST_DIR}/../../../../build/docs/.ktxtoolsdoc_gentimestamp")
add_custom_command(OUTPUT ${ktxtools_doc__buildKtxtoolsDoc__output}
  COMMAND ${CMAKE_COMMAND} -E make_directory build/docs
  COMMAND ./runDoxygen -t build/docs/.ktxtoolsdoc_gentimestamp -o build/docs/html ktxtools.doxy
  DEPENDS ${ktxtools_doc__buildKtxtoolsDoc__input}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT Generating KTX Tools documentation with Doxygen
  VERBATIM
)
add_custom_target(ktxtools_doc__buildKtxtoolsDoc
  DEPENDS ${ktxtools_doc__buildKtxtoolsDoc__output}
  SOURCES ${ktxtools_doc__buildKtxtoolsDoc__input}
)
add_custom_target(ktxtools_doc SOURCES)
set_target_properties(ktxtools_doc PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(ktxtools_doc__buildKtxtoolsDoc PROPERTIES EXCLUDE_FROM_ALL "FALSE")
add_dependencies(ktxtools_doc
  ktxtools_doc__buildKtxtoolsDoc
)
unset(TOOLSET)
unset(TARGET)

#ktxtools.gyp:install.tools#target
set(TARGET "install.tools")
set(TOOLSET "target")
list(APPEND install_tools__copies__input
  "${builddir}/ktx2ktx2"
  "${builddir}/ktx2check"
  "${builddir}/ktxinfo"
  "${builddir}/ktxsc"
  "${builddir}/toktx")
list(APPEND install_tools__copies__output
  "/tmp/ktx.dst//usr/local/bin/ktx2ktx2"
  "/tmp/ktx.dst//usr/local/bin/ktx2check"
  "/tmp/ktx.dst//usr/local/bin/ktxinfo"
  "/tmp/ktx.dst//usr/local/bin/ktxsc"
  "/tmp/ktx.dst//usr/local/bin/toktx")
set(install_tools__copies__input_dirs "${CMAKE_CURRENT_LIST_DIR}/../../../../build/docs/man/man1")
set(install_tools__copies__output_dirs "/tmp/ktx.dst//usr/local/share/man/man1")
add_custom_command(
OUTPUT ${install_tools__copies__output} ${install_tools__copies__output_dirs}
COMMAND ${CMAKE_COMMAND} -E copy ${builddir}/ktx2ktx2 /tmp/ktx.dst//usr/local/bin/ktx2ktx2
COMMAND ${CMAKE_COMMAND} -E copy ${builddir}/ktx2check /tmp/ktx.dst//usr/local/bin/ktx2check
COMMAND ${CMAKE_COMMAND} -E copy ${builddir}/ktxinfo /tmp/ktx.dst//usr/local/bin/ktxinfo
COMMAND ${CMAKE_COMMAND} -E copy ${builddir}/ktxsc /tmp/ktx.dst//usr/local/bin/ktxsc
COMMAND ${CMAKE_COMMAND} -E copy ${builddir}/toktx /tmp/ktx.dst//usr/local/bin/toktx
COMMAND ${CMAKE_COMMAND} -E copy_directory build/docs/man/man1/ /tmp/ktx.dst//usr/local/share/man/man1
DEPENDS ${install_tools__copies__input} ${install_tools__copies__input_dirs}
WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
COMMENT Copying for install_tools
VERBATIM
)
add_custom_target(install_tools__copies
  DEPENDS ${install_tools__copies__output} ${install_tools__copies__output_dirs}
  SOURCES ${install_tools__copies__input}
)
add_custom_target(install_tools SOURCES)
set_target_properties(install_tools PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(install_tools__copies PROPERTIES EXCLUDE_FROM_ALL "FALSE")
add_dependencies(install_tools__copies
  ktx2ktx2
  ktx2check
  ktxinfo
  ktxsc
  ktxtools_doc
  install_lib
  toktx
)
add_dependencies(install_tools
  ktx2ktx2
  ktx2check
  ktxinfo
  ktxsc
  ktxtools_doc
  install_lib
  toktx
  install_tools__copies
)
unset(TOOLSET)
unset(TARGET)

#ktxtools.gyp:package.tools#target
set(TARGET "package.tools")
set(TOOLSET "target")
add_custom_target(package_tools SOURCES)
set_target_properties(package_tools PROPERTIES EXCLUDE_FROM_ALL "FALSE")
add_dependencies(package_tools
  install_tools
)
unset(TOOLSET)
unset(TARGET)

#ktxdoc.gyp:ktxpkg.doc#target
set(TARGET "ktxpkg.doc")
set(TOOLSET "target")
list(APPEND ktxpkg_doc__buildPackageDoc__input
  "${CMAKE_CURRENT_LIST_DIR}/../../../../ktxpkg.doxy"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../interface/js_binding/ktx_wrapper.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../interface/js_binding/transcoder_wrapper.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../runDoxygen"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../TODO.md"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../pkgdoc/pages.md")
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
  version_h
  libktx_doc
  ktxtools_doc
)
add_dependencies(ktxpkg_doc
  version_h
  libktx_doc
  ktxtools_doc
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
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/cubemap_yokohama_astc_8x8_srgb.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/cubemap_yokohama_bc2_srgb.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/cubemap_yokohama_etc2_srgb.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/etc1.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/etc2-rgba1.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/etc2-rgba8.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/etc2-rgb.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/etc2-sRGBa1.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/etc2-sRGBa8.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/etc2-sRGB.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/hi_mark.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/hi_mark_sq.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/luminance-reference-metadata.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/metalplate-amg-rgba8.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/no-npot.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/not4_rgb888_srgb.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/orient-down-metadata.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/orient-up.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/orient-up-metadata.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/pattern_02_bc2.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/rgb-amg-reference.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/rgba-reference.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/rgb-mipmap-reference.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/rgb-reference.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/rgb-reference-metadata.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/texturearray_astc_8x8_unorm.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/texturearray_bc3_unorm.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/texturearray_etc2_unorm.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/alpha_simple_basis.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/cimg5293.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/color_grid_basis.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/cubemap_yokohama_basis_rd.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/FlightHelmet_baseColor_basis.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/kodim17_basis.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/ktx_app_basis.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/ktx_app-u.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/ktx_document_basis.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/orient-down-metadata-u.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/orient-up-metadata.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/orient-up-metadata-u.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/pattern_02_bc2.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/rgba-mipmap-reference-basis.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/rgba-reference-u.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/rgb-mipmap-reference-u.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/texturearray_astc_8x8_unorm.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/texturearray_bc3_unorm.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/texturearray_etc2_unorm.ktx2")
list(APPEND testimages__copies__output
  "${builddir}/testimages/conftestimage_R11_EAC.ktx"
  "${builddir}/testimages/conftestimage_RG11_EAC.ktx"
  "${builddir}/testimages/conftestimage_SIGNED_R11_EAC.ktx"
  "${builddir}/testimages/conftestimage_SIGNED_RG11_EAC.ktx"
  "${builddir}/testimages/cubemap_yokohama_astc_8x8_srgb.ktx"
  "${builddir}/testimages/cubemap_yokohama_bc2_srgb.ktx"
  "${builddir}/testimages/cubemap_yokohama_etc2_srgb.ktx"
  "${builddir}/testimages/etc1.ktx"
  "${builddir}/testimages/etc2-rgba1.ktx"
  "${builddir}/testimages/etc2-rgba8.ktx"
  "${builddir}/testimages/etc2-rgb.ktx"
  "${builddir}/testimages/etc2-sRGBa1.ktx"
  "${builddir}/testimages/etc2-sRGBa8.ktx"
  "${builddir}/testimages/etc2-sRGB.ktx"
  "${builddir}/testimages/hi_mark.ktx"
  "${builddir}/testimages/hi_mark_sq.ktx"
  "${builddir}/testimages/luminance-reference-metadata.ktx"
  "${builddir}/testimages/metalplate-amg-rgba8.ktx"
  "${builddir}/testimages/no-npot.ktx"
  "${builddir}/testimages/not4_rgb888_srgb.ktx"
  "${builddir}/testimages/orient-down-metadata.ktx"
  "${builddir}/testimages/orient-up.ktx"
  "${builddir}/testimages/orient-up-metadata.ktx"
  "${builddir}/testimages/pattern_02_bc2.ktx"
  "${builddir}/testimages/rgb-amg-reference.ktx"
  "${builddir}/testimages/rgba-reference.ktx"
  "${builddir}/testimages/rgb-mipmap-reference.ktx"
  "${builddir}/testimages/rgb-reference.ktx"
  "${builddir}/testimages/rgb-reference-metadata.ktx"
  "${builddir}/testimages/texturearray_astc_8x8_unorm.ktx"
  "${builddir}/testimages/texturearray_bc3_unorm.ktx"
  "${builddir}/testimages/texturearray_etc2_unorm.ktx"
  "${builddir}/testimages/alpha_simple_basis.ktx2"
  "${builddir}/testimages/cimg5293.ktx2"
  "${builddir}/testimages/color_grid_basis.ktx2"
  "${builddir}/testimages/cubemap_yokohama_basis_rd.ktx2"
  "${builddir}/testimages/FlightHelmet_baseColor_basis.ktx2"
  "${builddir}/testimages/kodim17_basis.ktx2"
  "${builddir}/testimages/ktx_app_basis.ktx2"
  "${builddir}/testimages/ktx_app-u.ktx2"
  "${builddir}/testimages/ktx_document_basis.ktx2"
  "${builddir}/testimages/orient-down-metadata-u.ktx2"
  "${builddir}/testimages/orient-up-metadata.ktx2"
  "${builddir}/testimages/orient-up-metadata-u.ktx2"
  "${builddir}/testimages/pattern_02_bc2.ktx2"
  "${builddir}/testimages/rgba-mipmap-reference-basis.ktx2"
  "${builddir}/testimages/rgba-reference-u.ktx2"
  "${builddir}/testimages/rgb-mipmap-reference-u.ktx2"
  "${builddir}/testimages/texturearray_astc_8x8_unorm.ktx2"
  "${builddir}/testimages/texturearray_bc3_unorm.ktx2"
  "${builddir}/testimages/texturearray_etc2_unorm.ktx2")
add_custom_command(
OUTPUT ${testimages__copies__output}
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/conftestimage_R11_EAC.ktx ${builddir}/testimages/conftestimage_R11_EAC.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/conftestimage_RG11_EAC.ktx ${builddir}/testimages/conftestimage_RG11_EAC.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/conftestimage_SIGNED_R11_EAC.ktx ${builddir}/testimages/conftestimage_SIGNED_R11_EAC.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/conftestimage_SIGNED_RG11_EAC.ktx ${builddir}/testimages/conftestimage_SIGNED_RG11_EAC.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/cubemap_yokohama_astc_8x8_srgb.ktx ${builddir}/testimages/cubemap_yokohama_astc_8x8_srgb.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/cubemap_yokohama_bc2_srgb.ktx ${builddir}/testimages/cubemap_yokohama_bc2_srgb.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/cubemap_yokohama_etc2_srgb.ktx ${builddir}/testimages/cubemap_yokohama_etc2_srgb.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/etc1.ktx ${builddir}/testimages/etc1.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/etc2-rgba1.ktx ${builddir}/testimages/etc2-rgba1.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/etc2-rgba8.ktx ${builddir}/testimages/etc2-rgba8.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/etc2-rgb.ktx ${builddir}/testimages/etc2-rgb.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/etc2-sRGBa1.ktx ${builddir}/testimages/etc2-sRGBa1.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/etc2-sRGBa8.ktx ${builddir}/testimages/etc2-sRGBa8.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/etc2-sRGB.ktx ${builddir}/testimages/etc2-sRGB.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/hi_mark.ktx ${builddir}/testimages/hi_mark.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/hi_mark_sq.ktx ${builddir}/testimages/hi_mark_sq.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/luminance-reference-metadata.ktx ${builddir}/testimages/luminance-reference-metadata.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/metalplate-amg-rgba8.ktx ${builddir}/testimages/metalplate-amg-rgba8.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/no-npot.ktx ${builddir}/testimages/no-npot.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/not4_rgb888_srgb.ktx ${builddir}/testimages/not4_rgb888_srgb.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/orient-down-metadata.ktx ${builddir}/testimages/orient-down-metadata.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/orient-up.ktx ${builddir}/testimages/orient-up.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/orient-up-metadata.ktx ${builddir}/testimages/orient-up-metadata.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/pattern_02_bc2.ktx ${builddir}/testimages/pattern_02_bc2.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/rgb-amg-reference.ktx ${builddir}/testimages/rgb-amg-reference.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/rgba-reference.ktx ${builddir}/testimages/rgba-reference.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/rgb-mipmap-reference.ktx ${builddir}/testimages/rgb-mipmap-reference.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/rgb-reference.ktx ${builddir}/testimages/rgb-reference.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/rgb-reference-metadata.ktx ${builddir}/testimages/rgb-reference-metadata.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/texturearray_astc_8x8_unorm.ktx ${builddir}/testimages/texturearray_astc_8x8_unorm.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/texturearray_bc3_unorm.ktx ${builddir}/testimages/texturearray_bc3_unorm.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/texturearray_etc2_unorm.ktx ${builddir}/testimages/texturearray_etc2_unorm.ktx
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/alpha_simple_basis.ktx2 ${builddir}/testimages/alpha_simple_basis.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/cimg5293.ktx2 ${builddir}/testimages/cimg5293.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/color_grid_basis.ktx2 ${builddir}/testimages/color_grid_basis.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/cubemap_yokohama_basis_rd.ktx2 ${builddir}/testimages/cubemap_yokohama_basis_rd.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/FlightHelmet_baseColor_basis.ktx2 ${builddir}/testimages/FlightHelmet_baseColor_basis.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/kodim17_basis.ktx2 ${builddir}/testimages/kodim17_basis.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/ktx_app_basis.ktx2 ${builddir}/testimages/ktx_app_basis.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/ktx_app-u.ktx2 ${builddir}/testimages/ktx_app-u.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/ktx_document_basis.ktx2 ${builddir}/testimages/ktx_document_basis.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/orient-down-metadata-u.ktx2 ${builddir}/testimages/orient-down-metadata-u.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/orient-up-metadata.ktx2 ${builddir}/testimages/orient-up-metadata.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/orient-up-metadata-u.ktx2 ${builddir}/testimages/orient-up-metadata-u.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/pattern_02_bc2.ktx2 ${builddir}/testimages/pattern_02_bc2.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/rgba-mipmap-reference-basis.ktx2 ${builddir}/testimages/rgba-mipmap-reference-basis.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/rgba-reference-u.ktx2 ${builddir}/testimages/rgba-reference-u.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/rgb-mipmap-reference-u.ktx2 ${builddir}/testimages/rgb-mipmap-reference-u.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/texturearray_astc_8x8_unorm.ktx2 ${builddir}/testimages/texturearray_astc_8x8_unorm.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/texturearray_bc3_unorm.ktx2 ${builddir}/testimages/texturearray_bc3_unorm.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/texturearray_etc2_unorm.ktx2 ${builddir}/testimages/texturearray_etc2_unorm.ktx2
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
  "../../../../utils/unused.h"
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
  "${CMAKE_CURRENT_LIST_DIR}/../../../../utils"
  "/$ENV{VULKAN_SDK}/include")
set_property(TARGET appfwSDL APPEND PROPERTY INCLUDE_DIRECTORIES ${appfwSDL__include_dirs})
set_target_properties(appfwSDL PROPERTIES COMPILE_DEFINITIONS "NDEBUG;")
set_target_properties(appfwSDL PROPERTIES COMPILE_FLAGS "-Wpedantic -O3 -std=c++11 -std=c++11 ")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_GCC_TREAT_WARNINGS_AS_ERRORS "YES")
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
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/common/models/cube.obj"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/common/models/sphere.obj"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/common/models/teapot.dae"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/common/models/torusknot.obj")
list(APPEND vkloadtests__copies__output
  "${builddir}/models/cube.obj"
  "${builddir}/models/sphere.obj"
  "${builddir}/models/teapot.dae"
  "${builddir}/models/torusknot.obj")
add_custom_command(
OUTPUT ${vkloadtests__copies__output}
COMMAND ${CMAKE_COMMAND} -E copy tests/loadtests/common/models/cube.obj ${builddir}/models/cube.obj
COMMAND ${CMAKE_COMMAND} -E copy tests/loadtests/common/models/sphere.obj ${builddir}/models/sphere.obj
COMMAND ${CMAKE_COMMAND} -E copy tests/loadtests/common/models/teapot.dae ${builddir}/models/teapot.dae
COMMAND ${CMAKE_COMMAND} -E copy tests/loadtests/common/models/torusknot.obj ${builddir}/models/torusknot.obj
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
  "../../../../tests/loadtests/vkloadtests/utils/VulkanTextureTranscoder.hpp"
  "../../../../tests/loadtests/vkloadtests/utils/VulkanMeshLoader.hpp"
  "../../../../tests/loadtests/vkloadtests/VulkanLoadTests.h"
  "../../../../tests/loadtests/vkloadtests/VulkanLoadTestSample.h")
link_directories( ../../../../other_lib/linux/${configuration}-x64
 $ENV{ASSIMP_HOME}/lib
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
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/vkloadtests/utils"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/appfwSDL"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/appfwSDL/VulkanAppSDL"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../$ENV{ASSIMP_HOME}/include"
  "/$ENV{VULKAN_SDK}/include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include")
set_property(TARGET vkloadtests APPEND PROPERTY INCLUDE_DIRECTORIES ${vkloadtests__include_dirs})
set_target_properties(vkloadtests PROPERTIES COMPILE_DEFINITIONS "NDEBUG;")
set_target_properties(vkloadtests PROPERTIES COMPILE_FLAGS "-Wpedantic -O3 -std=c++11 ")
set_target_properties(vkloadtests PROPERTIES LINK_FLAGS "-Wl,-rpath,. ")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "YES")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.khronos.ktx.${PRODUCT_NAME}")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES")
set_target_properties(vkloadtests PROPERTIES XCODE_ATTRIBUTE_GCC_TREAT_WARNINGS_AS_ERRORS "YES")
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

#ktxtests.gyp:gl3loadtests#target
set(TARGET "gl3loadtests")
set(TOOLSET "target")
list(APPEND gl3loadtests__copies__input
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/common/models/cube.obj"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/common/models/sphere.obj"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/common/models/teapot.dae"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/common/models/torusknot.obj")
list(APPEND gl3loadtests__copies__output
  "${builddir}/models/cube.obj"
  "${builddir}/models/sphere.obj"
  "${builddir}/models/teapot.dae"
  "${builddir}/models/torusknot.obj")
add_custom_command(
OUTPUT ${gl3loadtests__copies__output}
COMMAND ${CMAKE_COMMAND} -E copy tests/loadtests/common/models/cube.obj ${builddir}/models/cube.obj
COMMAND ${CMAKE_COMMAND} -E copy tests/loadtests/common/models/sphere.obj ${builddir}/models/sphere.obj
COMMAND ${CMAKE_COMMAND} -E copy tests/loadtests/common/models/teapot.dae ${builddir}/models/teapot.dae
COMMAND ${CMAKE_COMMAND} -E copy tests/loadtests/common/models/torusknot.obj ${builddir}/models/torusknot.obj
DEPENDS ${gl3loadtests__copies__input}
WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
COMMENT Copying for gl3loadtests
VERBATIM
)
add_custom_target(gl3loadtests__copies
  DEPENDS ${gl3loadtests__copies__output}
  SOURCES ${gl3loadtests__copies__input}
)
list(APPEND gl3loadtests__cxx_srcs
  "../../../../utils/argparser.cpp"
  "../../../../tests/loadtests/common/LoadTestSample.cpp"
  "../../../../tests/loadtests/common/SwipeDetector.cpp"
  "../../../../tests/loadtests/glloadtests/GLLoadTests.cpp"
  "../../../../tests/loadtests/glloadtests/shader-based/BasisuTest.cpp"
  "../../../../tests/loadtests/glloadtests/shader-based/DrawTexture.cpp"
  "../../../../tests/loadtests/glloadtests/shader-based/GL3LoadTests.cpp"
  "../../../../tests/loadtests/glloadtests/shader-based/GL3LoadTestSample.cpp"
  "../../../../tests/loadtests/glloadtests/shader-based/TextureArray.cpp"
  "../../../../tests/loadtests/glloadtests/shader-based/TextureCubemap.cpp"
  "../../../../tests/loadtests/glloadtests/shader-based/TexturedCube.cpp"
  "../../../../tests/loadtests/glloadtests/shader-based/shaders.cpp")
list(APPEND gl3loadtests__other_srcs
  "../../../../utils/argparser.h"
  "../../../../tests/loadtests/common/LoadTestSample.h"
  "../../../../tests/loadtests/common/SwipeDetector.h"
  "../../../../tests/loadtests/geom/cube.h"
  "../../../../tests/loadtests/geom/frame.h"
  "../../../../tests/loadtests/glloadtests/GLLoadTests.h"
  "../../../../tests/loadtests/common/ltexceptions.h"
  "../../../../tests/loadtests/glloadtests/shader-based/BasisuTest.h"
  "../../../../tests/loadtests/glloadtests/shader-based/DrawTexture.h"
  "../../../../tests/loadtests/glloadtests/shader-based/GL3LoadTestSample.h"
  "../../../../tests/loadtests/glloadtests/shader-based/TextureArray.h"
  "../../../../tests/loadtests/glloadtests/shader-based/TextureCubemap.h"
  "../../../../tests/loadtests/glloadtests/shader-based/TexturedCube.h"
  "../../../../tests/loadtests/glloadtests/shader-based/mygl.h"
  "../../../../tests/loadtests/glloadtests/utils/GLMeshLoader.hpp"
  "../../../../tests/loadtests/glloadtests/utils/GLTextureTranscoder.hpp")
link_directories( ../../../../other_lib/linux/${configuration}-x64
 $ENV{ASSIMP_HOME}/lib
)
add_executable(gl3loadtests ${gl3loadtests__cxx_srcs} ${gl3loadtests__other_srcs})
set_target_properties(gl3loadtests PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(gl3loadtests__copies PROPERTIES EXCLUDE_FROM_ALL "FALSE")
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
  "${CMAKE_CURRENT_LIST_DIR}/../../../../utils"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/glloadtests/utils"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/appfwSDL"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/appfwSDL/VulkanAppSDL"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../$ENV{ASSIMP_HOME}/include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include")
set_property(TARGET gl3loadtests APPEND PROPERTY INCLUDE_DIRECTORIES ${gl3loadtests__include_dirs})
set_target_properties(gl3loadtests PROPERTIES COMPILE_DEFINITIONS "GL_CONTEXT_PROFILE=SDL_GL_CONTEXT_PROFILE_CORE;GL_CONTEXT_MAJOR_VERSION=3;GL_CONTEXT_MINOR_VERSION=3;NDEBUG;")
set_target_properties(gl3loadtests PROPERTIES COMPILE_FLAGS "-Wpedantic -O3 -std=c++11 ")
set_target_properties(gl3loadtests PROPERTIES LINK_FLAGS "-Wl,-rpath,. ")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "YES")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.khronos.ktx.${PRODUCT_NAME}")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_GCC_TREAT_WARNINGS_AS_ERRORS "YES")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_INFOPLIST_FILE "tests/loadtests/glloadtests/resources/mac/Info.plist")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
set_target_properties(gl3loadtests PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
add_dependencies(gl3loadtests__copies
  appfwSDL
  libktx_gl
  libassimp
  libgl
  testimages
  libsdl
  vulkan_headers
)
add_dependencies(gl3loadtests
  libassimp
  libgl
  testimages
  libsdl
  vulkan_headers
  gl3loadtests__copies
)
target_link_libraries(gl3loadtests
  appfwSDL
  libktx_gl
  "-lSDL2-2.0"
  "-lSDL2main"
  "-ldl"
  "-lpthread"
  "-lassimp"
  "-lGL"
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
set_target_properties(gtest_main PROPERTIES COMPILE_FLAGS "-Wpedantic -O3 -std=c++11 ")
set_target_properties(gtest_main PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(gtest_main PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(gtest_main PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(gtest_main PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(gtest_main PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(gtest_main PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES")
set_target_properties(gtest_main PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(gtest_main PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES")
set_target_properties(gtest_main PROPERTIES XCODE_ATTRIBUTE_GCC_TREAT_WARNINGS_AS_ERRORS "YES")
set_target_properties(gtest_main PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
set_target_properties(gtest_main PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
unset(TOOLSET)
unset(TARGET)

#ktxtests.gyp:gtest#target
set(TARGET "gtest")
set(TOOLSET "target")
set(gtest__cxx_srcs "../../../../tests/gtest/src/gtest-all.cc")
list(APPEND gtest__other_srcs
  "../../../../tests/gtest/include/gtest/gtest-death-test.h"
  "../../../../tests/gtest/include/gtest/gtest.h"
  "../../../../tests/gtest/include/gtest/gtest-message.h"
  "../../../../tests/gtest/include/gtest/gtest-param-test.h"
  "../../../../tests/gtest/include/gtest/gtest_pred_impl.h"
  "../../../../tests/gtest/include/gtest/gtest-printers.h"
  "../../../../tests/gtest/include/gtest/gtest_prod.h"
  "../../../../tests/gtest/include/gtest/gtest-spi.h"
  "../../../../tests/gtest/include/gtest/gtest-test-part.h"
  "../../../../tests/gtest/include/gtest/gtest-typed-test.h"
  "../../../../tests/gtest/include/gtest/internal/gtest-death-test-internal.h"
  "../../../../tests/gtest/include/gtest/internal/gtest-filepath.h"
  "../../../../tests/gtest/include/gtest/internal/gtest-internal.h"
  "../../../../tests/gtest/include/gtest/internal/gtest-linked_ptr.h"
  "../../../../tests/gtest/include/gtest/internal/gtest-param-util-generated.h"
  "../../../../tests/gtest/include/gtest/internal/gtest-param-util.h"
  "../../../../tests/gtest/include/gtest/internal/gtest-port-arch.h"
  "../../../../tests/gtest/include/gtest/internal/gtest-port.h"
  "../../../../tests/gtest/include/gtest/internal/gtest-string.h"
  "../../../../tests/gtest/include/gtest/internal/gtest-tuple.h"
  "../../../../tests/gtest/include/gtest/internal/gtest-type-util.h")
add_library(gtest STATIC ${gtest__cxx_srcs} ${gtest__other_srcs})
set_target_properties(gtest PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_source_files_properties(${gtest__other_srcs} PROPERTIES HEADER_FILE_ONLY "TRUE")
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
set_target_properties(gtest PROPERTIES COMPILE_FLAGS "-Wpedantic -O3 -std=c++11 ")
set_target_properties(gtest PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(gtest PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(gtest PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(gtest PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(gtest PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(gtest PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES")
set_target_properties(gtest PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(gtest PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES")
set_target_properties(gtest PROPERTIES XCODE_ATTRIBUTE_GCC_TREAT_WARNINGS_AS_ERRORS "YES")
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
set_target_properties(unittests PROPERTIES COMPILE_DEFINITIONS "GTEST_HAS_PTHREAD=0;_HAS_EXCEPTIONS=1;NDEBUG;")
set_target_properties(unittests PROPERTIES COMPILE_FLAGS "-Wpedantic -O3 -std=c++11 ")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "YES")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES")
set_target_properties(unittests PROPERTIES XCODE_ATTRIBUTE_GCC_TREAT_WARNINGS_AS_ERRORS "YES")
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

#ktxtests.gyp:transcodetests#target
set(TARGET "transcodetests")
set(TOOLSET "target")
list(APPEND transcodetests__cxx_srcs
  "../../../../interface/basisu_c_binding/src/basisu_c_binding.cpp"
  "../../../../tests/transcodetests/transcodetests.cc")
set(transcodetests__other_srcs "../../../../interface/basisu_c_binding/inc/basisu_c_binding.h")
link_directories()
add_executable(transcodetests ${transcodetests__cxx_srcs} ${transcodetests__other_srcs})
set_target_properties(transcodetests PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_source_files_properties(${transcodetests__other_srcs} PROPERTIES HEADER_FILE_ONLY "TRUE")
set_target_properties(transcodetests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${builddir}")
set_target_properties(transcodetests PROPERTIES PREFIX "")
set_target_properties(transcodetests PROPERTIES RUNTIME_OUTPUT_NAME "transcodetests")
set_target_properties(transcodetests PROPERTIES SUFFIX "")
set_source_files_properties(${builddir}/transcodetests PROPERTIES GENERATED "TRUE")
list(APPEND transcodetests__include_dirs
  "${CMAKE_CURRENT_LIST_DIR}/../../../../interface/basisu_c_binding/inc"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib/basisu/transcoder"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../utils"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/gtest/include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/unittests"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include")
set_property(TARGET transcodetests APPEND PROPERTY INCLUDE_DIRECTORIES ${transcodetests__include_dirs})
set_target_properties(transcodetests PROPERTIES COMPILE_DEFINITIONS "GTEST_HAS_PTHREAD=0;_HAS_EXCEPTIONS=1;NDEBUG;")
set_target_properties(transcodetests PROPERTIES COMPILE_FLAGS "-Wpedantic -O3 -std=c++11 -Wno-pedantic ")
set_target_properties(transcodetests PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(transcodetests PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(transcodetests PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(transcodetests PROPERTIES XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "YES")
set_target_properties(transcodetests PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "")
set_target_properties(transcodetests PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(transcodetests PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(transcodetests PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES")
set_target_properties(transcodetests PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(transcodetests PROPERTIES XCODE_ATTRIBUTE_USE_HEADERMAP "NO")
set_target_properties(transcodetests PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic")
set_target_properties(transcodetests PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES")
set_target_properties(transcodetests PROPERTIES XCODE_ATTRIBUTE_GCC_TREAT_WARNINGS_AS_ERRORS "YES")
set_target_properties(transcodetests PROPERTIES XCODE_ATTRIBUTE_WARNING_CFLAGS "-Wno-nested-anon-types -Wno-gnu-anonymous-struct ")
set_target_properties(transcodetests PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
set_target_properties(transcodetests PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
add_dependencies(transcodetests
  libgl
)
target_link_libraries(transcodetests
  gtest
  libktx_gl
  "-lGL"
)
unset(TOOLSET)
unset(TARGET)

#ktxtests.gyp:texturetests#target
set(TARGET "texturetests")
set(TOOLSET "target")
set(texturetests__cxx_srcs "../../../../tests/texturetests/texturetests.cc")
set(texturetests__other_srcs "../../../../tests/unittests/wthelper.h")
link_directories()
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
set_target_properties(texturetests PROPERTIES COMPILE_DEFINITIONS "GTEST_HAS_PTHREAD=0;_HAS_EXCEPTIONS=1;NDEBUG;")
set_target_properties(texturetests PROPERTIES COMPILE_FLAGS "-Wpedantic -O3 -std=c++11 ")
set_target_properties(texturetests PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(texturetests PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "NO")
set_target_properties(texturetests PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(texturetests PROPERTIES XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "YES")
set_target_properties(texturetests PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "")
set_target_properties(texturetests PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(texturetests PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "3")
set_target_properties(texturetests PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES")
set_target_properties(texturetests PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(texturetests PROPERTIES XCODE_ATTRIBUTE_USE_HEADERMAP "NO")
set_target_properties(texturetests PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic")
set_target_properties(texturetests PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES")
set_target_properties(texturetests PROPERTIES XCODE_ATTRIBUTE_GCC_TREAT_WARNINGS_AS_ERRORS "YES")
set_target_properties(texturetests PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
set_target_properties(texturetests PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
add_dependencies(texturetests
  libgl
)
target_link_libraries(texturetests
-Wl,--start-group
  gtest
  gtest_main
-Wl,--end-group
  libktx_gl
  "-lGL"
)
unset(TOOLSET)
unset(TARGET)

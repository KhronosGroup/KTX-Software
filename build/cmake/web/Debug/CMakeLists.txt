cmake_minimum_required(VERSION 2.8.8 FATAL_ERROR)
cmake_policy(VERSION 2.8.8)
project(libassimp)
set(configuration "Debug")
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

#libktx.gyp:libktx.es3#target
set(TARGET "libktx.es3")
set(TOOLSET "target")
list(APPEND libktx_es3__c_srcs
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
  "../../../../lib/writer2.c")
list(APPEND libktx_es3__cxx_srcs
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
  "../../../../lib/basisu/lodepng.cpp"
  "../../../../lib/basisu/transcoder/basisu_transcoder.cpp"
  "../../../../lib/basisu/basisu_uastc_enc.cpp"
  "../../../../lib/basis_encode.cpp"
  "../../../../lib/basis_transcode.cpp"
  "../../../../lib/etcdec.cxx"
  "../../../../lib/etcunpack.cxx")
list(APPEND libktx_es3__other_srcs
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
  "../../../../lib/vkformat_enum.h")
add_library(libktx_es3 STATIC ${libktx_es3__c_srcs} ${libktx_es3__cxx_srcs} ${libktx_es3__other_srcs})
set_target_properties(libktx_es3 PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_source_files_properties(${libktx_es3__other_srcs} PROPERTIES HEADER_FILE_ONLY "TRUE")
set_target_properties(libktx_es3 PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${obj}.${TOOLSET}/")
set_target_properties(libktx_es3 PROPERTIES PREFIX "lib")
set_target_properties(libktx_es3 PROPERTIES ARCHIVE_OUTPUT_NAME "ktx.es3")
set_target_properties(libktx_es3 PROPERTIES SUFFIX ".a")
set_source_files_properties(${obj}.${TOOLSET}/libktx.es3.a PROPERTIES GENERATED "TRUE")
list(APPEND libktx_es3__include_dirs
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include")
set_property(TARGET libktx_es3 APPEND PROPERTY INCLUDE_DIRECTORIES ${libktx_es3__include_dirs})
set_target_properties(libktx_es3 PROPERTIES COMPILE_DEFINITIONS "KTX_OPENGL_ES3=1;KTX_USE_FUNCPTRS_FOR_VULKAN;KHRONOS_STATIC=1;LIBKTX=1;BASISU_NO_ITERATOR_DEBUG_LEVEL;KTX_OMIT_VULKAN=1;BASISD_SUPPORT_BC7=0;BASISD_SUPPORT_ATC=0;BASISD_SUPPORT_ASTC_HIGHER_OPAQUE_QUALITY=0;BASISD_SUPPORT_PVRTC2=0;BASISD_SUPPORT_FXT1=0;BASISD_SUPPORT_ETC2_EAC_RG11=0;DEBUG;_DEBUG;")
set_source_files_properties(${libktx_es3__c_srcs} PROPERTIES COMPILE_FLAGS "-Wpedantic -O0 -g -std=c99 ")
set_source_files_properties(${libktx_es3__cxx_srcs} PROPERTIES COMPILE_FLAGS "-Wpedantic -O0 -g -std=c++11 -Wno-nested-anon-types -Wno-gnu-anonymous-struct ")
set_target_properties(libktx_es3 PROPERTIES LINK_FLAGS "-g4 ")
set_target_properties(libktx_es3 PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(libktx_es3 PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES")
set_target_properties(libktx_es3 PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(libktx_es3 PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(libktx_es3 PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "0")
set_target_properties(libktx_es3 PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES")
set_target_properties(libktx_es3 PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(libktx_es3 PROPERTIES XCODE_ATTRIBUTE_GCC_TREAT_WARNINGS_AS_ERRORS "YES")
set_target_properties(libktx_es3 PROPERTIES XCODE_ATTRIBUTE_WARNING_CFLAGS "-Wno-nested-anon-types -Wno-gnu-anonymous-struct ")
set_target_properties(libktx_es3 PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
set_target_properties(libktx_es3 PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
add_dependencies(libktx_es3
  version_h
)
unset(TOOLSET)
unset(TARGET)

#libktx.gyp:msc_basis_transcoder.js#target
set(TARGET "msc_basis_transcoder.js")
set(TOOLSET "target")
set(msc_basis_transcoder_js__cxx_srcs "../../../../interface/js_binding/transcoder_wrapper.cpp")
add_executable(msc_basis_transcoder_js ${msc_basis_transcoder_js__cxx_srcs})
set_target_properties(msc_basis_transcoder_js PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(msc_basis_transcoder_js PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${builddir}")
set_target_properties(msc_basis_transcoder_js PROPERTIES PREFIX "")
set_target_properties(msc_basis_transcoder_js PROPERTIES RUNTIME_OUTPUT_NAME "msc_basis_transcoder.js")
set_target_properties(msc_basis_transcoder_js PROPERTIES SUFFIX "")
set_source_files_properties(${builddir}/msc_basis_transcoder.js PROPERTIES GENERATED "TRUE")
list(APPEND msc_basis_transcoder_js__include_dirs
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include")
set_property(TARGET msc_basis_transcoder_js APPEND PROPERTY INCLUDE_DIRECTORIES ${msc_basis_transcoder_js__include_dirs})
set_target_properties(msc_basis_transcoder_js PROPERTIES COMPILE_DEFINITIONS "BASISD_SUPPORT_BC7=0;BASISD_SUPPORT_ATC=0;BASISD_SUPPORT_ASTC_HIGHER_OPAQUE_QUALITY=0;BASISD_SUPPORT_UASTC=0;BASISD_SUPPORT_PVRTC2=0;BASISD_SUPPORT_FXT1=0;BASISD_SUPPORT_ETC2_EAC_RG11=0;KHRONOS_STATIC=1;DEBUG;_DEBUG;")
set_target_properties(msc_basis_transcoder_js PROPERTIES COMPILE_FLAGS "-Wpedantic -O0 -g -std=c++11 --bind -Wno-nested-anon-types -Wno-gnu-anonymous-struct ")
set_target_properties(msc_basis_transcoder_js PROPERTIES LINK_FLAGS "--bind --source-map-base ./ -s ALLOW_MEMORY_GROWTH=1 -s ASSERTIONS=0 -s MALLOC=emmalloc -s MODULARIZE=1 -s EXPORT_NAME=MSC_TRANSCODER -s FULL_ES3=1 -g4 ")
set_target_properties(msc_basis_transcoder_js PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(msc_basis_transcoder_js PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES")
set_target_properties(msc_basis_transcoder_js PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(msc_basis_transcoder_js PROPERTIES XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "NO")
set_target_properties(msc_basis_transcoder_js PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "")
set_target_properties(msc_basis_transcoder_js PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(msc_basis_transcoder_js PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "0")
set_target_properties(msc_basis_transcoder_js PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES")
set_target_properties(msc_basis_transcoder_js PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(msc_basis_transcoder_js PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic")
set_target_properties(msc_basis_transcoder_js PROPERTIES XCODE_ATTRIBUTE_GCC_TREAT_WARNINGS_AS_ERRORS "YES")
set_target_properties(msc_basis_transcoder_js PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
set_target_properties(msc_basis_transcoder_js PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
add_dependencies(msc_basis_transcoder_js
  version_h
)
target_link_libraries(msc_basis_transcoder_js
  libktx_es3
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

#libktx.gyp:libktx.js#target
set(TARGET "libktx.js")
set(TOOLSET "target")
set(libktx_js__cxx_srcs "../../../../interface/js_binding/ktx_wrapper.cpp")
link_directories()
add_executable(libktx_js ${libktx_js__cxx_srcs})
set_target_properties(libktx_js PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(libktx_js PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${builddir}")
set_target_properties(libktx_js PROPERTIES PREFIX "")
set_target_properties(libktx_js PROPERTIES RUNTIME_OUTPUT_NAME "libktx.js")
set_target_properties(libktx_js PROPERTIES SUFFIX "")
set_source_files_properties(${builddir}/libktx.js PROPERTIES GENERATED "TRUE")
list(APPEND libktx_js__include_dirs
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../lib"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include")
set_property(TARGET libktx_js APPEND PROPERTY INCLUDE_DIRECTORIES ${libktx_js__include_dirs})
set_target_properties(libktx_js PROPERTIES COMPILE_DEFINITIONS "BASISD_SUPPORT_BC7=0;BASISD_SUPPORT_ATC=0;BASISD_SUPPORT_ASTC_HIGHER_OPAQUE_QUALITY=0;BASISD_SUPPORT_UASTC=0;BASISD_SUPPORT_PVRTC2=0;BASISD_SUPPORT_FXT1=0;BASISD_SUPPORT_ETC2_EAC_RG11=0;KHRONOS_STATIC=1;DEBUG;_DEBUG;")
set_target_properties(libktx_js PROPERTIES COMPILE_FLAGS "-Wpedantic -s USE_WEBGL2=1 -O0 -g -std=c++11 --bind ")
set_target_properties(libktx_js PROPERTIES LINK_FLAGS "--bind --source-map-base ./ -s ALLOW_MEMORY_GROWTH=1 -s ASSERTIONS=2 -s MALLOC=emmalloc -s MODULARIZE=1 -s EXPORT_NAME=LIBKTX -s EXTRA_EXPORTED_RUNTIME_METHODS=['GL'] -s GL_PREINITIALIZED_CONTEXT=1 -s FULL_ES3=1 -s USE_WEBGL2=1 -g4 ")
set_target_properties(libktx_js PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(libktx_js PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES")
set_target_properties(libktx_js PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(libktx_js PROPERTIES XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "NO")
set_target_properties(libktx_js PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "")
set_target_properties(libktx_js PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(libktx_js PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "0")
set_target_properties(libktx_js PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES")
set_target_properties(libktx_js PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(libktx_js PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic")
set_target_properties(libktx_js PROPERTIES XCODE_ATTRIBUTE_GCC_TREAT_WARNINGS_AS_ERRORS "YES")
set_target_properties(libktx_js PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
set_target_properties(libktx_js PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
add_dependencies(libktx_js
  libgles3
  version_h
)
target_link_libraries(libktx_js
  libktx_es3
)
unset(TOOLSET)
unset(TARGET)

#libktx.gyp:install_js#target
set(TARGET "install_js")
set(TOOLSET "target")
set(install_js__cpjs__input "${builddir}")
list(APPEND install_js__cpjs__output
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/webgl/libktx.js"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/webgl/libktx.wasm"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/webgl/msc_basis_transcoder.js"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/webgl/msc_basis_transcoder.wasm")
add_custom_command(OUTPUT ${install_js__cpjs__output}
  COMMAND ${CMAKE_COMMAND} -E make_directory tests/webgl
  COMMAND cp "${builddir}/libktx.js" "${builddir}/libktx.wasm" "${builddir}/msc_basis_transcoder.js" "${builddir}/msc_basis_transcoder.wasm" tests/webgl
  DEPENDS ${install_js__cpjs__input}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../../../../
  COMMENT Copying .js & .wasm files to binding tests.
  VERBATIM
)
add_custom_target(install_js__cpjs
  DEPENDS ${install_js__cpjs__output}
  SOURCES ${install_js__cpjs__input}
)
add_custom_target(install_js SOURCES)
set_target_properties(install_js PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_target_properties(install_js__cpjs PROPERTIES EXCLUDE_FROM_ALL "FALSE")
add_dependencies(install_js__cpjs
  libktx_js
  msc_basis_transcoder_js
)
add_dependencies(install_js
  libktx_js
  msc_basis_transcoder_js
  install_js__cpjs
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
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/texturearray_etc2_unorm.ktx"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/FlightHelmet_baseColor_basis.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/alpha_simple_basis.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/color_grid_basis.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/cubemap_yokohama_basis_rd.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/kodim17_basis.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/ktx_app-u.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/ktx_app_basis.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/ktx_document_basis.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/orient-down-metadata-u.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/orient-up-metadata-u.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/orient-up-metadata.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/pattern_02_bc2.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/rgb-mipmap-reference-u.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/rgba-mipmap-reference-basis.ktx2"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/testimages/rgba-reference-u.ktx2"
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
  "${builddir}/testimages/texturearray_etc2_unorm.ktx"
  "${builddir}/testimages/FlightHelmet_baseColor_basis.ktx2"
  "${builddir}/testimages/alpha_simple_basis.ktx2"
  "${builddir}/testimages/color_grid_basis.ktx2"
  "${builddir}/testimages/cubemap_yokohama_basis_rd.ktx2"
  "${builddir}/testimages/kodim17_basis.ktx2"
  "${builddir}/testimages/ktx_app-u.ktx2"
  "${builddir}/testimages/ktx_app_basis.ktx2"
  "${builddir}/testimages/ktx_document_basis.ktx2"
  "${builddir}/testimages/orient-down-metadata-u.ktx2"
  "${builddir}/testimages/orient-up-metadata-u.ktx2"
  "${builddir}/testimages/orient-up-metadata.ktx2"
  "${builddir}/testimages/pattern_02_bc2.ktx2"
  "${builddir}/testimages/rgb-mipmap-reference-u.ktx2"
  "${builddir}/testimages/rgba-mipmap-reference-basis.ktx2"
  "${builddir}/testimages/rgba-reference-u.ktx2"
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
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/FlightHelmet_baseColor_basis.ktx2 ${builddir}/testimages/FlightHelmet_baseColor_basis.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/alpha_simple_basis.ktx2 ${builddir}/testimages/alpha_simple_basis.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/color_grid_basis.ktx2 ${builddir}/testimages/color_grid_basis.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/cubemap_yokohama_basis_rd.ktx2 ${builddir}/testimages/cubemap_yokohama_basis_rd.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/kodim17_basis.ktx2 ${builddir}/testimages/kodim17_basis.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/ktx_app-u.ktx2 ${builddir}/testimages/ktx_app-u.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/ktx_app_basis.ktx2 ${builddir}/testimages/ktx_app_basis.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/ktx_document_basis.ktx2 ${builddir}/testimages/ktx_document_basis.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/orient-down-metadata-u.ktx2 ${builddir}/testimages/orient-down-metadata-u.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/orient-up-metadata-u.ktx2 ${builddir}/testimages/orient-up-metadata-u.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/orient-up-metadata.ktx2 ${builddir}/testimages/orient-up-metadata.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/pattern_02_bc2.ktx2 ${builddir}/testimages/pattern_02_bc2.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/rgb-mipmap-reference-u.ktx2 ${builddir}/testimages/rgb-mipmap-reference-u.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/rgba-mipmap-reference-basis.ktx2 ${builddir}/testimages/rgba-mipmap-reference-basis.ktx2
COMMAND ${CMAKE_COMMAND} -E copy tests/testimages/rgba-reference-u.ktx2 ${builddir}/testimages/rgba-reference-u.ktx2
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
add_custom_target(libsdl SOURCES)
set_target_properties(libsdl PROPERTIES EXCLUDE_FROM_ALL "FALSE")
unset(TOOLSET)
unset(TARGET)

#ktxtests.gyp:appfwSDL#target
set(TARGET "appfwSDL")
set(TOOLSET "target")
set(appfwSDL__frag2spirv__input "")
add_custom_target(appfwSDL__frag2spirv DEPENDS
SOURCES ${appfwSDL__frag2spirv__input}
)
set(appfwSDL__vert2spirv__input "")
add_custom_target(appfwSDL__vert2spirv DEPENDS
SOURCES ${appfwSDL__vert2spirv__input}
)
list(APPEND appfwSDL__cxx_srcs
  "../../../../tests/loadtests/appfwSDL/main.cpp"
  "../../../../tests/loadtests/appfwSDL/AppBaseSDL.cpp"
  "../../../../tests/loadtests/appfwSDL/GLAppSDL.cpp")
list(APPEND appfwSDL__other_srcs
  "../../../../tests/loadtests/appfwSDL/AppBaseSDL.h"
  "../../../../tests/loadtests/appfwSDL/GLAppSDL.h")
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
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../utils")
set_property(TARGET appfwSDL APPEND PROPERTY INCLUDE_DIRECTORIES ${appfwSDL__include_dirs})
set_target_properties(appfwSDL PROPERTIES COMPILE_DEFINITIONS "DEBUG;_DEBUG;")
set_target_properties(appfwSDL PROPERTIES COMPILE_FLAGS "-Wpedantic -s USE_SDL=2 -O0 -g ")
set_target_properties(appfwSDL PROPERTIES LINK_FLAGS "-g4 ")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "0")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_GCC_TREAT_WARNINGS_AS_ERRORS "YES")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
set_target_properties(appfwSDL PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
add_dependencies(appfwSDL__frag2spirv
  libsdl
)
add_dependencies(appfwSDL__vert2spirv
  libsdl
)
add_dependencies(appfwSDL
  libsdl
  appfwSDL__frag2spirv
  appfwSDL__vert2spirv
)
unset(TOOLSET)
unset(TARGET)

#ktxtests.gyp:es3loadtests.html#target
set(TARGET "es3loadtests.html")
set(TOOLSET "target")
list(APPEND es3loadtests_html__cxx_srcs
  "../../../../utils/argparser.cpp"
  "../../../../tests/loadtests/common/LoadTestSample.cpp"
  "../../../../tests/loadtests/common/SwipeDetector.cpp"
  "../../../../tests/loadtests/glloadtests/GLLoadTests.cpp"
  "../../../../tests/loadtests/glloadtests/shader-based/BasisuTest.cpp"
  "../../../../tests/loadtests/glloadtests/shader-based/DrawTexture.cpp"
  "../../../../tests/loadtests/glloadtests/shader-based/GL3LoadTests.cpp"
  "../../../../tests/loadtests/glloadtests/shader-based/GL3LoadTestSample.cpp"
  "../../../../tests/loadtests/glloadtests/shader-based/TextureArray.cpp"
  "../../../../tests/loadtests/glloadtests/shader-based/TexturedCube.cpp"
  "../../../../tests/loadtests/glloadtests/shader-based/shaders.cpp")
list(APPEND es3loadtests_html__other_srcs
  "../../../../tests/loadtests/geom/quad.h"
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
  "../../../../tests/loadtests/glloadtests/shader-based/TexturedCube.h"
  "../../../../tests/loadtests/glloadtests/shader-based/mygl.h"
  "../../../../tests/loadtests/glloadtests/utils/GLMeshLoader.hpp"
  "../../../../tests/loadtests/glloadtests/utils/GLTextureTranscoder.hpp")
link_directories()
add_executable(es3loadtests_html ${es3loadtests_html__cxx_srcs} ${es3loadtests_html__other_srcs})
set_target_properties(es3loadtests_html PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_source_files_properties(${es3loadtests_html__other_srcs} PROPERTIES HEADER_FILE_ONLY "TRUE")
set_target_properties(es3loadtests_html PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${builddir}")
set_target_properties(es3loadtests_html PROPERTIES PREFIX "")
set_target_properties(es3loadtests_html PROPERTIES RUNTIME_OUTPUT_NAME "es3loadtests.html")
set_target_properties(es3loadtests_html PROPERTIES SUFFIX "")
set_source_files_properties(${builddir}/es3loadtests.html PROPERTIES GENERATED "TRUE")
list(APPEND es3loadtests_html__include_dirs
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/glloadtests"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/common"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/geom"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../utils"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/glloadtests/utils"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../tests/loadtests/appfwSDL"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../other_include"
  "${CMAKE_CURRENT_LIST_DIR}/../../../../include")
set_property(TARGET es3loadtests_html APPEND PROPERTY INCLUDE_DIRECTORIES ${es3loadtests_html__include_dirs})
set_target_properties(es3loadtests_html PROPERTIES COMPILE_DEFINITIONS "GL_CONTEXT_PROFILE=SDL_GL_CONTEXT_PROFILE_ES;GL_CONTEXT_MAJOR_VERSION=3;GL_CONTEXT_MINOR_VERSION=0;KHRONOS_STATIC=1;DEBUG;_DEBUG;")
set_target_properties(es3loadtests_html PROPERTIES COMPILE_FLAGS "-Wpedantic -s DISABLE_EXCEPTION_CATCHING=0 -s USE_SDL=2 -s USE_WEBGL2=1 -O0 -g -std=c++11 ")
set_target_properties(es3loadtests_html PROPERTIES LINK_FLAGS "--source-map-base ./ --preload-file testimages --exclude-file testimages/genref --exclude-file testimages/*.pgm --exclude-file testimages/*.ppm --exclude-file testimages/*.pam --exclude-file testimages/*.pspimage -s ALLOW_MEMORY_GROWTH=1 -s DISABLE_EXCEPTION_CATCHING=0 -s USE_SDL=2 -s USE_WEBGL2=1 -g4 ")
set_target_properties(es3loadtests_html PROPERTIES XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO")
set_target_properties(es3loadtests_html PROPERTIES XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES")
set_target_properties(es3loadtests_html PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES")
set_target_properties(es3loadtests_html PROPERTIES XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "c99")
set_target_properties(es3loadtests_html PROPERTIES XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "NO")
set_target_properties(es3loadtests_html PROPERTIES XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "ktx_app")
set_target_properties(es3loadtests_html PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
set_target_properties(es3loadtests_html PROPERTIES XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_LAUNCHIMAGE_NAME "LaunchImage")
set_target_properties(es3loadtests_html PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES")
set_target_properties(es3loadtests_html PROPERTIES XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "NO")
set_target_properties(es3loadtests_html PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.khronos.ktx.${PRODUCT_NAME}")
set_target_properties(es3loadtests_html PROPERTIES XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "0")
set_target_properties(es3loadtests_html PROPERTIES XCODE_ATTRIBUTE_GCC_TREAT_WARNINGS_AS_ERRORS "YES")
set_target_properties(es3loadtests_html PROPERTIES XCODE_ATTRIBUTE_INFOPLIST_FILE "tests/loadtests/glloadtests/resources/mac/Info.plist")
set_target_properties(es3loadtests_html PROPERTIES XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS "NO")
set_target_properties(es3loadtests_html PROPERTIES XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
add_dependencies(es3loadtests_html
  libgles3
  testimages
  libsdl
  version_h
)
target_link_libraries(es3loadtests_html
-Wl,--start-group
  appfwSDL
  libktx_es3
-Wl,--end-group
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

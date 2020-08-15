# Copyright 2015-2020 Mark Callow.
# SPDX-License-Identifier: Apache-2.0

##
# @internal
#
# @brief Generate project file for building the application framework.
#
{
  'includes': [
     '../../../gyp_include/libsdl.gypi',
  ],
  'targets': [
    {
      'target_name': 'appfwSDL',
      'type': 'static_library',
      'cflags_cc': [ '-std=c++11' ],
      'dependencies': [
        'libsdl',
        'libktx.gyp:vulkan_headers',
      ],
      'direct_dependent_settings': {
        'conditions': [
          ['OS == "mac" or OS == "ios"', {
            # Cause the dependent application to copy these shaders into its
            # app bundle. Can't use 'process_outputs_as_mac_bundle_resources'
            # in the glsl2spirv rules because that provides no way to put the
            # rule outputs in a sub-directory of "Resources".
            'copies': [{
              'destination': '<(shader_dest)',
              'files': [
                '<(SHARED_INTERMEDIATE_DIR)/textoverlay.frag.spv',
                '<(SHARED_INTERMEDIATE_DIR)/textoverlay.vert.spv',
              ],
            }], # copies
          }],
        ], # conditions
        'include_dirs': [
          '.',
          'VulkanAppSDL',
        ],
      },
      'export_dependent_settings': [ 'libsdl' ],
      'includes': [
         '../../../gyp_include/glsl2spirv.gypi'
      ],
      'include_dirs': [
        '.',
        'VulkanAppSDL',
        '../../../other_include',
        '../../../utils',
      ],
      'variables': {
        'vulkan_files': [
          '../../../utils/unused.h',
          'VulkanAppSDL/VulkanAppSDL.cpp',
          'VulkanAppSDL/VulkanAppSDL.h',
          'VulkanAppSDL/vulkancheckres.h',
          'VulkanAppSDL/VulkanContext.cpp',
          'VulkanAppSDL/VulkanContext.h',
          'VulkanAppSDL/VulkanSwapchain.cpp',
          'VulkanAppSDL/VulkanSwapchain.h',
          'VulkanAppSDL/vulkandebug.cpp',
          'VulkanAppSDL/vulkandebug.h',
          'VulkanAppSDL/vulkantextoverlay.hpp',
          'VulkanAppSDL/vulkantools.cpp',
          'VulkanAppSDL/vulkantools.h',
          'VulkanAppSDL/shaders/textoverlay.frag',
          'VulkanAppSDL/shaders/textoverlay.vert',
        ],
      },
      'sources': [
        # .h files are included so they will appear in IDEs' file lists.
        'main.cpp',
        'AppBaseSDL.cpp',
        'AppBaseSDL.h',
        'GLAppSDL.cpp',
        'GLAppSDL.h',
        '<@(vulkan_files)',
      ],
      'link_settings': {
        'conditions': [
          ['OS == "android"', {
            'libraries': [ '-lstlport_static' ],
            'library_dirs': [ '<(cxx-stl)/stlport/libs/$(TARGET_ABI)' ],
          }],
        ],
        # Ideally we should include in this 'link_settings' setting of
        # "-s NO_EXIT_RUNTIME" for Emscripten for html5 since the
        # no-loop framework needs the runtime to keep running when main
        # exits, but, as ldflags & VCLinker.AdditionalOptions are
        # strings, any setting made here will overwrite any settings
        # made by the application. Defer to the app for setting this.
      },
      'xcode_settings': {
        'CLANG_CXX_LANGUAGE_STANDARD': 'c++0x',
      }, # xcode_settings
      'conditions': [
        # Web platform does not support Vulkan.
        # Earlier MSVS Versions do not support C++11 so exclude VkAppSDL.
        ['OS == "web" or (GENERATOR == "msvs" and MSVS_VERSION != "2015" and MSVS_VERSION != "2017" and MSVS_VERSION != "2019")', {
          'cflags_cc!': [ '-std=c++11' ],
          'dependencies!': [ 'libktx.gyp:vulkan_headers' ],
          'direct_dependent_settings': {
            'include_dirs!': [ 'VulkanAppSDL' ],
          },
          'includes!': [
            '../../../gyp_include/glsl2spirv.gypi'
          ],
          'include_dirs!': [
            'VulkanAppSDL',
          ],
          'sources!': [ '<@(vulkan_files)' ],
        }]
      ],
    } # target appfwSDL
  ] #targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70

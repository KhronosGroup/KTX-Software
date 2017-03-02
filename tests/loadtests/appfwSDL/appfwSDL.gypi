##
# @internal
# @copyright © 2015, Mark Callow. For license see LICENSE.md.
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
      #'toolsets': [target', 'emscripten'],
      'cflags_cc': [ '-std=c++11' ],
      'dependencies': [
        'libsdl',
        'libktx.gyp:vulkan_headers',
     ],
      'direct_dependent_settings': {
        # This is here to avoid polluting the "Resources" or output folder with
        # all the .spv files. With the simpler choice of setting
        # 'process_outputs_as_mac_bundle_resources' in the glsl2spirv rules,
        # there is no way to set a subdir of "Resources" as the destination.
        'copies': [{
          'destination': '<(shader_dest)',
          'files': [
            '<(INTERMEDIATE_DIR)/shaders/textoverlay.frag.spv',
            '<(INTERMEDIATE_DIR)/shaders/textoverlay.vert.spv',
          ],
        }], # copies
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
        # These are so SDL_vulkan.c will compile without changes
        # from how it will be when eventually included in SDL source.
        '.',
        'VulkanAppSDL',
        '../../../other_include/SDL2',
        # For stb.
        '../../../other_include',
      ],
      'sources': [
        # .h files are included so they will appear in IDEs' file lists.
        'main.cpp',
        'AppBaseSDL.cpp',
        'AppBaseSDL.h',
        'GLAppSDL.cpp',
        'GLAppSDL.h',
        # These 2 are here to avoid rebuilds of SDL during development.
        'VulkanAppSDL/SDL_vulkan.c',
        'VulkanAppSDL/SDL_vulkan.h',
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
      'link_settings': {
        'conditions': [
          ['OS == "android"', {
            'libraries': [ '-lstlport_static' ],
            'library_dirs': [ '<(cxx-stl)/stlport/libs/$(TARGET_ABI)' ],
          }, 'OS == "linux"', {
            # This is because of SDL_vulkan
            'libraries': [ '-lX11-xcb' ],
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
      }
    } # target appfwSDL
  ] #targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70

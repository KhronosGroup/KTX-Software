##
# @internal
# @copyright © 2015, Mark Callow. For license see LICENSE.md.
#
# @brief Generate project file for building the application framework.
#
{
  'includes': [
     '../../../gyp_include/libsdl.gypi',
     '../../../gyp_include/libvulkan.gypi',
  ],
  'targets': [
    {
      'target_name': 'appfwSDL',
      'type': 'static_library',
      #'toolsets': [target', 'emscripten'],
      'sources': [
        # .h files are included so they will appear in IDEs' file lists.
        'main.cpp',
        'AppBaseSDL.cpp',
        'AppBaseSDL.h',
        'GLAppSDL.cpp',
        'GLAppSDL.h',
        # These 2 are here to avoid rebuilds of SDL during development.
        'SDL_vulkan.c',
        'SDL_vulkan.h',
        'VkAppSDL.cpp',
        'VkAppSDL.h',
      ],
      'cflags': [ '-std=c++11' ],
      'dependencies': [ 'libsdl', 'vulkan_headers' ],
      'direct_dependent_settings': {
        'include_dirs': [ '.' ],
      },
      'export_dependent_settings': [ 'libsdl' ],
      # This is so SDL_vulkan.c will compile without changes
      # from how it will be when eventually included in SDL source.
      'include_dirs': [
        '.',
        '../../../other_include/SDL2',
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

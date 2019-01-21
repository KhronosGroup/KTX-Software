##
# @internal
# @copyright © 2019, Mark Callow. For license see LICENSE.md.
#
# @brief Generate project to build KTX library for OpenGL.
#
{
  'includes': [
      '../gyp_include/libgl.gypi',
      '../gyp_include/libvulkan.gypi',
      'libktxdoc.gypi',
      'sources.gypi',
  ],
  'variables': {
    'include_dirs': [
      '../include',
      '../other_include',
    ],
  }, # variables
  'targets': [
    {
      'target_name': 'libktx.gl',
      'type': '<(library)',
      'defines': [ 'KTX_OPENGL=1' ],
      'direct_dependent_settings': {
         'include_dirs': [ '<@(include_dirs)' ],
      },
      'include_dirs': [ '<@(include_dirs)' ],
      'mac_bundle': 0,
      'dependencies': [ 'vulkan_headers' ],
      'sources': [
        '<@(sources)',
        '<@(vksource_files)',
      ],
      'conditions': [
        ['_type == "shared_library"', {
          'dependencies': [ 'libgl', 'libvulkan.lazy' ],
          'conditions': [
            ['OS == "mac" or OS == "ios"', {
              'direct_dependent_settings': {
                'target_conditions': [
                  ['_mac_bundle == 1', {
                    'copies': [{
                      'xcode_code_sign': 1,
                      'destination': '<(PRODUCT_DIR)/$(FRAMEWORKS_FOLDER_PATH)',
                      'files': [ '<(PRODUCT_DIR)/<(_target_name)<(SHARED_LIB_SUFFIX)' ],
                    }], # copies
                    'xcode_settings': {
                      # Tell DYLD where to search for this dylib.
                      # "man dyld" for more information.
                      'LD_RUNPATH_SEARCH_PATHS': [ '@executable_path/../Frameworks' ],
                    },
                  }, {
                    'xcode_settings': {
                      'LD_RUNPATH_SEARCH_PATHS': [ '@executable_path' ],
                    },
                  }], # _mac_bundle == 1
                ], # target_conditions
              }, # direct_dependent_settings
              'sources!': [
                'vk_funclist.inl',
                'vk_funcs.c',
                'vk_funcs.h',
              ],
              'xcode_settings': {
                # This is so dyld can find the dylib when it is installed by
                # the copy command above.
                'INSTALL_PATH': '@rpath',
              },
            }, 'OS == "linux"', {
              'defines': [ 'KTX_USE_FUNCPTRS_FOR_VULKAN' ],
              'dependencies!': [ 'libvulkan.lazy' ],
            }] # OS == "mac or OS == "ios"
          ], # conditions
        }] # _type == "shared_library"
      ], # conditions
    }, # libktx.gl target
    {
      'target_name': 'libktx.es1',
      'type': 'static_library',
      'defines': [ 'KTX_OPENGL_ES1=1' ],
      'direct_dependent_settings': {
        'include_dirs': [ '<@(include_dirs)' ],
      },
      'sources': [ '<@(sources)' ],
      'include_dirs': [ '<@(include_dirs)' ],
    }, # libktx.es1
    {
      'target_name': 'libktx.es3',
      'type': 'static_library',
      'defines': [ 'KTX_OPENGL_ES3=1' ],
      'dependencies': [ 'vulkan_headers' ],
      'direct_dependent_settings': {
         'include_dirs': [ '<@(include_dirs)' ],
      },
      'sources': [
        '<@(sources)',
        '<@(vksource_files)',
      ],
      'include_dirs': [ '<@(include_dirs)' ],
    }, # libktx.es3
  ], # targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70

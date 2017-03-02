##
# @internal
# @copyright © 2015, Mark Callow. For license see LICENSE.md.
#
# @brief Generate project to build KTX library for OpenGL.
#
{
  'variables': {
    'sources': [
      # .h files are included so they will appear in IDEs' file lists.
      '../include/ktx.h',
      'checkheader.c',
      'errstr.c',
      'etcdec.cxx',
      'etcunpack.cxx',
      'gl_format.h',
      'gl_funcptrs.h',
      'gles1_funcptrs.h',
      'gles2_funcptrs.h',
      'gles3_funcptrs.h',
      'glloader.c',
      'hashtable.c',
      'ktxcontext.c',
      'ktxcontext.h',
      'ktxfilestream.c',
      'ktxfilestream.h',
      'ktxint.h',
      'ktxmemstream.c',
      'ktxmemstream.h',
      'ktxstream.h',
      'reader.c',
      'swap.c',
      'uthash.h',
      'writer.c',
    ],
    # Use _files to get the names relativized
    'vksource_files': [
      '../include/ktxvulkan.h',
      'vk_format.h',
      'vkloader.c',
    ],
    'include_dirs': [
      '../include',
      '../other_include',
    ],
  }, # variables

  'includes': [
      '../gyp_include/libgl.gypi',
      '../gyp_include/libvulkan.gypi',
  ],
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
      'sources': [
        '<@(sources)',
        '<@(vksource_files)',
      ],
      'conditions': [
        ['_type == "shared_library"', {
          'dependencies': [ 'libgl', 'libvulkan' ],
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
              'xcode_settings': {
                # This is so dyld can find the dylib when it is installed by
                # the copy command above.
                'INSTALL_PATH': '@rpath',
              },
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
      'direct_dependent_settings': {
         'include_dirs': [ '<@(include_dirs)' ],
      },
      'sources': [ '<@(sources)' ],
      'include_dirs': [ '<@(include_dirs)' ],
    }, # libktx.es3
  ], # targets
  'conditions': [
    ['OS == "linux" or OS == "mac" or OS == "win"', {
      # Can only build doc on desktops
      'targets': [
        {
          'target_name': 'libktx.doc',
          'type': 'none',
          'variables': {
            'doxyConfig': 'ktxDoxy',
            'timestamp': 'build/doc/.gentimestamp',
          },
          'actions': [
            {
              'action_name': 'buildDoc',
              'message': 'Generating documentation with Doxygen',
              'inputs': [
                '../<@(doxyConfig)',
                '../LICENSE.md',
                '<@(sources)',
              ],
              'outputs': [
                '../build/doc/html',
                '../build/doc/latex',
                '../build/doc/man',
              ],
              # doxygen must be run in the top-level project directory
              # so that ancestors of that # directory will be removed
              # from paths displayed in the documentation. This is
              # the directory where the ktxDoxy file and .gyp files
              # are stored.
              #
              'conditions': [
                ['GENERATOR == "xcode"', {
                  # With Xcode, the current directory during project
                  # build is one we need so we're good to go. However
                  # we need to spawn another shell with -l so the
                  # startup (.bashrc, etc) files will be read.
                  #
                  # Actions in Xcode are run by 'sh' which does not
                  # read any startup files. Startup files must be
                  # read so that the user's normal $PATH will be set
                  # and, therefore, we can find Doxygen. It seems to
                  # be impossible to modify the $PATH variable used
                  # by Xcode; it will only inherit the user's path
                  # when started from the command line.
                  'action': [
                    'bash', '-l', '-c', 'doxygen <@(doxyConfig)'
                  ],
                }, {
                  # With `make`, cmake, etc, like Xcode,  the current
                  # directory during project build is the one we need.
                  #
                  # With MSVS the current directory will be that
                  # containing the vcxproj file. However when the
                  # action is using bash ('msvs_cygwin_shell': '1',
                  # the default, is set) a setup_env.bat file is run
                  # before the command. Our setup_env.bat cd's to the
                  # top-level directory to make this case look like the
                  # others.
                  #
                  # No path relativization is performed on any command
                  # arguments. We have to take care to provide paths that
                  # are relative to our cd location.
                  #
                  # Note that If using cmd.exe ('msvs_cygwin_shell': '0')
                  # the MSVS generator will relativize *all* command
                  # arguments, that do not look like options, to the
                  # vcxproj location.
                  'action': [
                    'doxygen', '<@(doxyConfig)'
                  ],
                }],
              ], # action conditional
            },
            # It is not possible to chain commands in an action with
            # && because the generators will quote such strings.
            # Instead we need this additional action to touch the
            # timestamp. The generators will chain these two actions
            {
              'action_name': 'touchTimestamp',
              'variables': {
                'timestamp': 'build/doc/.gentimestamp',
              },
              'message': 'setting generation timestamp',
              'inputs': [
                '../<@(doxyConfig)',
                '../LICENSE.md',
                '<@(sources)',
              ],
              'outputs': [
                '<(timestamp)',
              ],
              'action': [ 'touch', '<@(timestamp)', ],
            }
          ], # actions
        }, # libktx.doc
      ], # targets
    }],
  ],
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70

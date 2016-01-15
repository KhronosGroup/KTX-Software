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
      'gl_funcptrs.h',
      'gles1_funcptrs.h',
      'gles2_funcptrs.h',
      'gles3_funcptrs.h',
      'hashtable.c',
      'ktxfilestream.c',
      'ktxfilestream.h',
      'ktxint.h',
      'ktxmemstream.c',
      'ktxmemstream.h',
      'ktxstream.h',
      'loader.c',
      'swap.c',
      'uthash.h',
      'writer.c',
    ],
    'include_dirs': [
      '../include',
      '../other_include',
    ],
  }, # variables
  # As writer.c does not need OpenGL, do not add a dependency on
  # OpenGL{, ES} here.
  'targets': [
    {
      'target_name': 'libktx.gl',
      'variables': {
        # Because these must be specified in two places.
        'defines': [ 'KTX_OPENGL=1' ],
      },
      'type': 'static_library',
      'defines': [ '<@(defines)' ],
      'direct_dependent_settings': {
         'defines': [ '<@(defines)' ],
         'include_dirs': [ '<@(include_dirs)' ],
      },
      'sources': [ '<@(sources)' ],
      'include_dirs': [ '<@(include_dirs)' ],
    }, # libktx.gl target
    {
      'target_name': 'libktx.es1',
      'variables': {
        # Because these must be specified in two places.
        'defines': [ 'KTX_OPENGL_ES1=1' ],
      },
      'type': 'static_library',
      'defines': [ '<@(defines)' ],
      'direct_dependent_settings': {
        'defines': [ '<@(defines)' ],
        'include_dirs': [ '<@(include_dirs)' ],
      },
      'sources': [ '<@(sources)' ],
      'include_dirs': [ '<@(include_dirs)' ],
    }, # libktx.es1
    {
      'target_name': 'libktx.es3',
      'variables': {
        # Because these must be specified in two places.
        'defines': [ 'KTX_OPENGL_ES3=1' ],
      },
      'type': 'static_library',
      'defines': [ '<@(defines)' ],
      'direct_dependent_settings': {
         'defines': [ '<@(defines)' ],
         'include_dirs': [ '<@(include_dirs)' ],
      },
      'sources': [ '<@(sources)' ],
      'include_dirs': [ '<@(include_dirs)' ],
    }, # libktx.es3
  ], # targets
  'conditions': [
    ['OS == "linux" or OS == "mac"', {
      # Can only build doc on desktops
      'targets': [
        {
          'target_name': 'libktx.doc',
          'type': 'none',
          'actions': [
            {
              'action_name': 'buildDoc',
              'variables': {
                'doxyConfig': 'ktxDoxy',
              },
              'message': 'Generating documentation with Doxygen',
              'msvs_cygwin_shell': '0',
              'msvs_quote_cmd': '1',
              'inputs': [
                '../<@(doxyConfig)',
                '../LICENSE.md',
                '<@(sources)',
              ],
              'outputs': [
                '../build/doc/.gentimestamp',
                '../build/doc/html',
                '../build/doc/latex',
                '../build/doc/man',
              ],
              # doxygen must be run in the top-level project directory
              # containing the ktxDoxy file so that ancestors of that
              # directory will be removed from paths displayed in the
              # documentation.
              #
              'conditions': [
                ['GENERATOR == "xcode"', {
                  # With Xcode, like Linux, the current directory
                  # during project build is one holding the .gyp and
                  # ktxDoxy files. However we need to spawn another
                  # shell with -l so the startup (.bashrc, etc) files
                  # will be read.
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
                }, 'GENERATOR == "msvs"', {
                  # With MSVS the current directory will be that
                  # containing the vcxproj file.
                  #
                  # Spawn another cmd.exe so we can change directory.
                  # Unfortunately the MSVS generator relativizes any
                  # command argument that does not look like an
                  # option which, in the following, means it prefixes
                  # "cd" with the relative path. There is no way to
                  # avoid this so for now this target is only
                  # included for Xcode.
                  'action': [
                    'cmd', '/c', '/e:off cd', '..', '& doxygen <@(doxyConfig)'
                  ],
                }, {
                  # With `make`, cmake, etc. the current directory during
                  # project build is the directory containing the .gyp file,
                  # which is the same directory that holds the ktxDoxy
                  # file, so we're good to go.
                  'action': [
                    'doxygen', '<@(doxyConfig)'
                  ],
                }],
              ], # action conditional
            }
          ], # actions
        }, # libktx.doc
      ], # targets
    }],
  ],
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70

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
    ['OS == "mac"', {
      # Can only build on Mac for now. See comment below.
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
              # doxygen must be run in the top-level project directory so that
              # ancestors of that directory will be removed from paths displayed
              # in the documentation. With Xcode the current directory when a
              # project is run is the directory containing the .gyp file, which,
              # in this case is the top-level directory we need so the command
              # below works.
              #
              # With MSVS it is the directory containing the .vcxproj
              # file and the MSVS generator will "relativize the
              # doxyConfig attribute accordingly. I don't see a way to
              # make the action run in a different directory so for
              # now, the target is only included for Xcode.
              #
              # Spawn another shell with -l so the startup files will be read.
              # Actions in Xcode are run by 'sh' so no startup files are read.
              # Startup files must be read so that the user's normal $PATH will
              # set and, therefore, we can find Doxygen. It seems to be
              # impossible to modify the $PATH variable used by Xcode and it
              # only uses the user's path when started from the command line.
              'action': [
                'bash', '-l', '-c', 'doxygen <@(doxyConfig)'
              ],
            },
          ], # actions
        }, # libktx.doc
      ], # targets
    }],
  ],
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70

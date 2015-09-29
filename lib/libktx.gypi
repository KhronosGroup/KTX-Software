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
    },
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
          # doxygen is run in the top-level directory so that ancestors of that
          # directory will be removed from paths displayed in the documentation.
          'action': [
            '$DOXYGEN_BIN', '<@(doxyConfig)',
          ],
        },
      ], # actions
    }, # libktx.doc
  ], # targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70

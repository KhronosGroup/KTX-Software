##
# @internal
# @copyright © 2015, Mark Callow. For license see LICENSE.md.
#
# @brief Generate project to build KTX library.
#
{
 # As writer.c does not need OpenGL, do not add a dependency on
 # OpenGL{, ES} here.
 'targets': [
  {
    'target_name': 'libktx',
    'type': 'static_library',
    #'toolsets': [target', 'emscripten'],
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
    'direct_dependent_settings': {
      'include_dirs': [
        '../include',
        '../other_include',
      ],
    },
    'include_dirs': [
      '../include',  # Must be specified in both places.
      '../other_include',
    ],
    'conditions': [
      ['GL_VERSION[:2] == "gl"', {
        'dependencies': [ 'libgl' ],
        'export_dependent_settings': [ 'libgl' ],
        'sources!': [
          'gles1_funcptrs.h',
          'gles2_funcptrs.h',
          'gles3_funcptrs.h',
        ],
      }, 'GL_VERSION == "es1"', {
        'dependencies': [ 'libgles1' ],
        'export_dependent_settings': [ 'libgles1' ],
        'sources!': [
          'gl_funcptrs.h',
          'gles2_funcptrs.h',
          'gles3_funcptrs.h',
        ],
      }, 'GL_VERSION == "es2"', {
        'dependencies': [ 'libgles2' ],
        'export_dependent_settings': [ 'libgles2' ],
        'sources!': [
          'gl_funcptrs.h',
          'gles1_funcptrs.h',
          'gles3_funcptrs.h',
        ],
      }, 'GL_VERSION == "es3"', {
        'dependencies': [ 'libgles3' ],
        'export_dependent_settings': [ 'libgles3' ],
        'sources!': [
          'gl_funcptrs.h',
          'gles1_funcptrs.h',
          'gles2_funcptrs.h',
        ],
      }],
    ], # conditions
  }], # targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70

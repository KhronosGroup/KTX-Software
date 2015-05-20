##
# @internal
# @copyright © 2015, Mark Callow. For license see LICENSE.md.
#
# @brief Core of targets for building a version of the KTX library.
#
{
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
  'variables':  {
    # Because these must be specified in two places.
    'include_dirs': [
      '../include',
      '../other_include',
    ],
  },
  'direct_dependent_settings': {
    'include_dirs':  [ '<@(include_dirs)' ],
  },
  'include_dirs': [ '<@(include_dirs)' ],
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70

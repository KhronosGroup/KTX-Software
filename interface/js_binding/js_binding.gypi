##
# @internal
# @copyright © 2015, Mark Callow. For license see LICENSE.md.
#
# @brief Generate project to building the JavaScript binding for the
#        KTX library.
#
{
  'conditions': [
    ['OS == "web"', {
      'targets': [
        {
          'target_name': 'libktx.js',
          'type': 'executable',
          'cflags_cc': [ '--bind' ],
          'dependencies': [ 'libktx.es3', 'libgles3' ],
          'mac_bundle': 0,
          'sources': [
            'ktx_wrappers.cpp',
          ],
          'include_dirs': [ '../../include' ],
          'ldflags': [
             '--bind',
             '--source-map-base', './',
             '-s', 'ALLOW_MEMORY_GROWTH=1',
             '-s', 'ASSERTIONS=2',
             '-s', 'MALLOC=emmalloc',
             '-s', 'MODULARIZE=1',
             '-s', 'EXPORT_NAME=libktx',
             '-s', 'EXTRA_EXPORTED_RUNTIME_METHODS=[\'GL\']',
             '-s', 'FULL_ES3=1',
          ],
        }, # libktx.js
      ], #libktx.js target
    }], # OS = "web"
  ], # conditions
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70

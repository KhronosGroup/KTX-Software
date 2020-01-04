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
            'ktx_wrapper.cpp',
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
        {
          'target_name': 'msc_basis_transcoder.js',
          'type': 'executable',
          'cflags_cc': [
            '--bind',
            # The BasisU transcoder uses anon types and structs. They compile
            # ok in Emscripten so quiet the clang warnings.
            '-Wno-nested-anon-types',
            '-Wno-gnu-anonymous-struct',
          ],
          'defines': [
            'BASISD_SUPPORT_BC7=0',
            'BASISD_SUPPORT_ATC=0',
            'BASISD_SUPPORT_ASTC_HIGHER_OPAQUE_QUALITY=0',
            'BASISD_SUPPORT_PVRTC2=0',
            'BASISD_SUPPORT_FXT1=0',
            'BASISD_SUPPORT_ETC2_EAC_RG11=0',
          ],
          'dependencies': [ 'libktx.es3' ],
          'mac_bundle': 0,
          'sources': [
            'transcoder_wrapper.cpp',
          ],
          'include_dirs': [
              '../../include',
              '../../lib',
          ],
          'ldflags': [
             '--bind',
             '--source-map-base', './',
             '-s', 'ALLOW_MEMORY_GROWTH=1',
             '-s', 'ASSERTIONS=0',
             '-s', 'MALLOC=emmalloc',
             '-s', 'MODULARIZE=1',
             '-s', 'EXPORT_NAME=libktx',
             '-s', 'EXTRA_EXPORTED_RUNTIME_METHODS=[\'GL\']',
             '-s', 'FULL_ES3=1',
          ],
        }, # msc_basis_transcoder.js
        {
          'target_name': 'install_js',
          'type': 'none',
          'dependencies': [ 'libktx.js', 'msc_basis_transcoder.js' ],
#          'copies': [{
#            'destination': '../../tests/webgl',
#            'files': [
#              '<(PRODUCT_DIR)/libktx.js',
#              '<(PRODUCT_DIR)/libktx.wasm',
#              '<(PRODUCT_DIR)/msc_basis_transcoder.js',
#              '<(PRODUCT_DIR)/msc_basis_transcoder.wasm'
#            ],
#          }],
          # This is a gross hack to workaround the failure of cmake to generate
          # files due to the 'files' in the above copies not existing (when
          # setting up a new build environment). The same problem appears with
          # actions. To avoid it the missing inputs are not listed. How you
          # are supposed to install the products of a build, I don't know.
          'actions': [{
            'variables': {
              # Hack to get output dir relativized.
              'output_dir': '../../tests/webgl'
            },
            'action_name': 'cpjs',
            'message': 'Copying .js & .wasm files to binding tests.',
            'inputs': [
               '<(PRODUCT_DIR)',
#              '<(PRODUCT_DIR)/libktx.js',
#              '<(PRODUCT_DIR)/libktx.wasm',
#              '<(PRODUCT_DIR)/msc_basis_transcoder.js',
#              '<(PRODUCT_DIR)/msc_basis_transcoder.wasm'
            ],
            'outputs': [
              '../../tests/webgl/libktx.js',
              '../../tests/webgl/libktx.wasm',
              '../../tests/webgl/msc_basis_transcoder.js',
              '../../tests/webgl/msc_basis_transcoder.wasm'
            ],
            'action': [
              'cp',
              '<(PRODUCT_DIR)/libktx.js',
              '<(PRODUCT_DIR)/libktx.wasm',
              '<(PRODUCT_DIR)/msc_basis_transcoder.js',
              '<(PRODUCT_DIR)/msc_basis_transcoder.wasm',
              '<(output_dir)'
            ],
          }],
        }
      ], # targets
    }], # OS = "web"
  ], # conditions
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70

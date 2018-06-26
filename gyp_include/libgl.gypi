##
# @internal
# @copyright © 2015, Mark Callow. For license see LICENSE.md.
#
# @brief Target for adding dependency on OpenGL.
#
{
  'targets': [
  {
    'target_name': 'libgl',
    'type': 'none',
    'direct_dependent_settings': {
      'include_dirs': [
        '<(gl_includes_parent_dir)',
      ]
    },
    'variables': {
      'conditions': [
        ['OS == "mac"', {
          'lib_dirs': [ ],
          'libs': ['$(SDKROOT)/System/Library/Frameworks/OpenGL.framework'],
        }, 'OS == "linux"', {
          'lib_dirs': [ ],
          'libs': ['-lGL'],
        }, 'OS == "win"', {
          'lib_dirs': [ '<(glew_lib_dir)' ],
          'conditions': [
            ['GENERATOR == "msvs"', {
              'libs': [
                '-lopengl32.lib',
                '-lglew32.lib',
              ],
            }, {
              'libs': [
                '-lgl',
                '-lglew32',
              ],
            }],
            ['glew_dll_dir != ""', {
              'dlls': [ '<(glew_dll_dir)/glew32.dll' ],
            }, {
              'dlls': [ ],
            }],
          ],
        }, {
          # OpenGL not supported
          'lib_dirs': [ ],
          'libs': [ ],
        }],
      ],
    }, # variables
    'conditions': [
      ['OS == "win" and dlls.__len__() > 0', {
        'copies': [{
          # Files appearing in 'copies' cause gyp to generate a folder
          # hierarchy in Visual Studio filters reflecting the location
          # of each file. The folders will be empty.
          'destination': '<(PRODUCT_DIR)',
          'files': [
            '<@(dlls)',
          ],
        }],
      }], # OS == "win"
    ], # conditions
    'link_settings': {
     'libraries': [ '<@(libs)' ],
     'library_dirs': [ '<@(lib_dirs)' ],
    },
  }], # targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70

# Copyright 2015-2020 Mark Callow
# SPDX-License-Identifier: Apache-2.0

##
# @internal
#
# @brief Target for adding dependency on OpenGL ES 1.
#
{
  'targets': [
  {
    'target_name': 'libgles1',
    'type': 'none',
    'direct_dependent_settings': {
      'include_dirs': [
        '<(gl_includes_parent_dir)',
      ]
    },
    'conditions': [
      ['OS == "win"', {
        'variables' : {
          'lib_dirs': [ '<(gles1_lib_dir)' ],
          'conditions': [
            ['GENERATOR == "msvs"', {
              'libs': ['-llibGLES_CM', '-llibEGL'],
            }, {
              'libs': ['-lGLES_CM', '-lEGL'],
            }],
          ],
        }, # variables
        'copies': [{
          # Files appearing in 'copies' cause gyp to generate a folder
          # hierarchy in Visual Studio filters reflecting the location
          # of each file. The folders will be empty.
          'destination': '<(PRODUCT_DIR)',
          'files': [
            '<@(gles1_dlls)',
          ],
        }],
      }], # OS == "win"
      ['OS == "ios"', {
        'variables': {
          'lib_dirs': [ ],
          'libs': ['$(SDKROOT)/System/Library/Frameworks/OpenGLES.framework'],
        },
      }],
      ['OS == "android"', {
        'variables': {
          'lib_dirs': [ ],
          'libs': ['-lGLES_CM', '-lEGL'],
        }
      }],
    ], # conditions
    'link_settings': {
      'libraries': [ '<@(libs)' ],
      'library_dirs': [ '<@(lib_dirs)' ],
    }
  }], # targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70

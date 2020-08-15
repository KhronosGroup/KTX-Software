# Copyright 2015-2020 Mark Callow
# SPDX-License-Identifier: Apache-2.0

##
# @internal
#
# @brief Target for adding dependency on OpenGL ES 2.
#
{
# XXX TO BE COMPLETED.
  'targets': [
  {
    'target_name': 'libgles2',
    'type': 'none',
    'direct_dependent_settings': {
      'include_dirs': [
        '<(gl_includes_parent_dir)',
      ]
    },
    'conditions': [
      ['OS == "win"', {
        'variables' : {
          'dlls': [
            '<(winolib_dir)/libEGL.dll',
            '<(winolib_dir)/libGLESv2.dll',
            '<(winolib_dir)/d3dcompiler_46.dll',
          ],
          'lib_dirs': [ '<(winolib_dir)' ],
          'conditions': [
            ['GENERATOR == "msvs"', {
              'libs': ['-llibGLESv2', '-llibEGL'],
            }, {
              'libs': ['-lGLESv2', '-lEGL'],
            }],
          ],
        }, # variables
        # Configuration dependent copies are not possible, nor are
        # configuration dependent sources. Hence use of $(PlatformName)
        # that is set by the build environment. NOTE: $(PlatformName)
        # may not work with the make generator.
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
      ['OS == "ios"', {
        'variables': {
          'lib_dirs': [ ],
          'libs': ['$(SDKROOT)/System/Library/Frameworks/OpenGLES.framework'],
        },
      }],
      ['OS == "mac"', {
        # Uses GL_ARB_ES2_compatibility
        'variables': {
          'lib_dirs': [ ],
          'libs': ['$(SDKROOT)/System/Library/Frameworks/OpenGL.framework'],
        },
      }],
      ['OS == "android"', {
        'variables': {
          'lib_dirs': [ ],
          'libs': ['-lGLESv2', '-lEGL'],
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

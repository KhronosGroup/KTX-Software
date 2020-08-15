# Copyright 2015-2020 Mark Callow
# SPDX-License-Identifier: Apache-2.0

##
# @internal
#
# @brief Set configuration variables for ANGLE (OpenGL ES on D3D).
#
# You will need to build ANGLE yourself. Copy the libraries you
# build (.lib and .dll) into the appropriate directory of
# ../other_lib/win. Using the ANGLE libraries provided with Chrome
# requires a change to the source code of appfwSDL and including
# the d3d compiler dll.
{
  'variables': { # level 1
    'variables': { # level 2 defines variables to be used in level 1
      'conditions': [
        ['OS == "win"', {
          'gles1_bin_dir': 'nowhere',
          'gles1_lib_dir': 'nowhere',
          'gles2_bin_dir': '<(winolib_dir)',
          'gles2_lib_dir': '<(winolib_dir)',
          'gles3_bin_dir': '<(winolib_dir)',
          'gles3_lib_dir': '<(winolib_dir)',
        }, {
          'gles1_bin_dir': 'nowhere',
          'gles1_lib_dir': 'nowhere',
          'gles2_bin_dir': 'nowhere',
          'gles2_lib_dir': 'nowhere',
          'gles3_bin_dir': 'nowhere',
          'gles3_lib_dir': 'nowhere',
        }]
      ] # conditions
    }, # variables, level 2 
    'gles1_bin_dir%': '<(gles1_bin_dir)',
    'gles1_lib_dir%': '<(gles1_lib_dir)',
    'gles2_bin_dir%': '<(gles2_bin_dir)',
    'gles2_lib_dir%': '<(gles2_lib_dir)',
    'gles3_bin_dir%': '<(gles3_bin_dir)',
    'gles3_lib_dir%': '<(gles3_lib_dir)',

    'conditions': [
      ['OS == "win"', {
        'gles1_dlls': [ ],
        'gles3_dlls': [
           '<(gles2_bin_dir)/libEGL.dll',
           '<(gles2_bin_dir)/libGLESv2.dll',
           #'<(gles2_bin_dir)/d3dcompiler_46.dll',
        ], 
        'gles3_dlls': [
           '<(gles3_bin_dir)/libEGL.dll',
           '<(gles3_bin_dir)/libGLESv2.dll',
           #'<(gles2_bin_dir)/d3dcompiler_46.dll',
        ], 
      }, {
        'gles1_dlls': [ ],
        'gles2_dlls': [ ],
        'gles3_dlls': [ ],
      }],
    ], # conditions

    'es1support': 'false',
  },
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
